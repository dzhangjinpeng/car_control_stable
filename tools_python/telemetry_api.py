from __future__ import annotations

import argparse
import json
import mimetypes
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse


class TelemetryReader:
    def __init__(self, telemetry_file: Path) -> None:
        self.telemetry_file = telemetry_file

    def history(self, limit: int) -> list[dict]:
        if not self.telemetry_file.exists():
            return []
        lines = self.telemetry_file.read_text(encoding="utf-8", errors="replace").splitlines()
        frames: list[dict] = []
        for line in lines[-limit:]:
            if not line.strip():
                continue
            try:
                frames.append(json.loads(line))
            except json.JSONDecodeError:
                continue
        return frames

    def latest(self) -> dict | None:
        frames = self.history(1)
        return frames[-1] if frames else None


def load_json_file(path: Path) -> dict | None:
    if not path.exists():
        return None
    return json.loads(path.read_text(encoding="utf-8"))


class ApiHandler(BaseHTTPRequestHandler):
    server_version = "CarControlStableAPI/1.0"

    def do_OPTIONS(self) -> None:
        self.send_response(HTTPStatus.NO_CONTENT)
        self._cors()
        self.end_headers()

    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        reader: TelemetryReader = self.server.reader  # type: ignore[attr-defined]
        frontend_dir: Path = self.server.frontend_dir  # type: ignore[attr-defined]
        config_dir: Path = self.server.config_dir  # type: ignore[attr-defined]
        calibration_report: Path = self.server.calibration_report  # type: ignore[attr-defined]

        if parsed.path == "/api/v1/health":
            latest = reader.latest()
            self._json(
                {
                    "ok": True,
                    "data": {
                        "service": "car_control_stable",
                        "telemetry_ready": latest is not None,
                        "latest_loop_index": latest.get("loop_index") if latest else None,
                        "mode": latest.get("mode") if latest else None,
                        "input_source": latest.get("input_source") if latest else None,
                        "config_files": {
                            "control": (config_dir / "control.json").exists(),
                            "hardware": (config_dir / "hardware.json").exists(),
                            "input": (config_dir / "input.json").exists(),
                            "network": (config_dir / "network.json").exists(),
                            "safety": (config_dir / "safety.json").exists(),
                        },
                        "calibration_report_exists": calibration_report.exists(),
                    },
                }
            )
            return
        if parsed.path == "/api/v1/telemetry/latest":
            self._json({"ok": True, "data": reader.latest()})
            return
        if parsed.path == "/api/v1/telemetry/history":
            query = parse_qs(parsed.query)
            try:
                limit = max(1, min(500, int(query.get("limit", ["100"])[0])))
            except ValueError:
                limit = 100
            self._json({"ok": True, "data": reader.history(limit)})
            return
        if parsed.path.startswith("/api/v1/config/"):
            name = parsed.path.rsplit("/", 1)[-1]
            allowed = {
                "control": "control.json",
                "hardware": "hardware.json",
                "input": "input.json",
                "network": "network.json",
                "safety": "safety.json",
                "profiles": "control_profiles.json",
            }
            if name not in allowed:
                self._json({"ok": False, "error": "unknown config"}, HTTPStatus.NOT_FOUND)
                return
            self._json({"ok": True, "data": load_json_file(config_dir / allowed[name])})
            return
        if parsed.path == "/api/v1/calibration/latest":
            self._json({"ok": True, "data": load_json_file(calibration_report)})
            return
        if parsed.path == "/api/v1/meta":
            self._json(
                {
                    "ok": True,
                    "data": {
                        "service": "car_control_stable",
                        "endpoints": [
                            "/api/v1/health",
                            "/api/v1/telemetry/latest",
                            "/api/v1/telemetry/history?limit=100",
                            "/api/v1/config/control",
                            "/api/v1/config/hardware",
                            "/api/v1/config/input",
                            "/api/v1/config/network",
                            "/api/v1/config/safety",
                            "/api/v1/config/profiles",
                            "/api/v1/calibration/latest",
                            "/api/v1/meta",
                        ],
                    },
                }
            )
            return

        self._static(frontend_dir, parsed.path)

    def log_message(self, format: str, *args: object) -> None:  # noqa: A003
        return

    def _json(self, payload: dict, status: HTTPStatus = HTTPStatus.OK) -> None:
        body = json.dumps(payload, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
        self.send_response(status)
        self._cors()
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _static(self, frontend_dir: Path, raw_path: str) -> None:
        rel = raw_path.lstrip("/") or "index.html"
        if ".." in Path(rel).parts:
            self._json({"ok": False, "error": "not found"}, HTTPStatus.NOT_FOUND)
            return
        path = frontend_dir / rel
        if not path.exists() or not path.is_file():
            path = frontend_dir / "index.html"
        body = path.read_bytes()
        self.send_response(HTTPStatus.OK)
        self._cors()
        self.send_header("Content-Type", mimetypes.guess_type(path.name)[0] or "application/octet-stream")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _cors(self) -> None:
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Read-only telemetry API for car_control_stable")
    parser.add_argument("--telemetry-file", default="logs/mock_telemetry.jsonl")
    parser.add_argument("--frontend-dir", default="frontend")
    parser.add_argument("--config-dir", default="configs")
    parser.add_argument("--calibration-report", default="logs/calibration.json")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8765)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    server = ThreadingHTTPServer((args.host, args.port), ApiHandler)
    server.reader = TelemetryReader(Path(args.telemetry_file))  # type: ignore[attr-defined]
    server.frontend_dir = Path(args.frontend_dir)  # type: ignore[attr-defined]
    server.config_dir = Path(args.config_dir)  # type: ignore[attr-defined]
    server.calibration_report = Path(args.calibration_report)  # type: ignore[attr-defined]
    print(f"telemetry api: http://{args.host}:{args.port}")
    print(f"telemetry file: {args.telemetry_file}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
