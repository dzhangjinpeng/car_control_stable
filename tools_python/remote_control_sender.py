from __future__ import annotations

import argparse
import json
import socket
import time
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Callable


@dataclass
class DriverInput:
    left_x: float = 0.0
    left_y: float = 0.0
    right_x: float = 0.0
    right_y: float = 0.0
    mode_button: bool = False
    steering_lock_button: bool = False
    drive_direction_button: bool = False
    emergency_stop_button: bool = False


def clamp_axis(value: float) -> float:
    return max(-1.0, min(1.0, float(value)))


def packet_bytes(seq: int, active: bool, driver_input: DriverInput) -> bytes:
    payload = {
        "seq": seq,
        "timestamp": time.time(),
        "active": active,
        **asdict(driver_input),
    }
    return json.dumps(payload, ensure_ascii=False, separators=(",", ":")).encode("utf-8")


class ScriptedInput:
    def __init__(self) -> None:
        self._frames = [
            DriverInput(),
            DriverInput(left_y=-0.45),
            DriverInput(left_y=-0.45, left_x=0.35),
            DriverInput(left_y=0.25),
            DriverInput(),
        ]
        self._index = 0

    def poll(self) -> DriverInput:
        frame = self._frames[self._index % len(self._frames)]
        self._index += 1
        return frame


class PygameGamepadInput:
    def __init__(self, input_config_path: Path, index: int) -> None:
        import pygame

        self._pygame = pygame
        self._config = load_input_config(input_config_path)
        self._index = index
        self._joystick = None

    def open(self) -> None:
        self._pygame.init()
        self._pygame.joystick.init()
        if self._pygame.joystick.get_count() <= self._index:
            raise RuntimeError(f"gamepad index {self._index} not found")
        self._joystick = self._pygame.joystick.Joystick(self._index)
        self._joystick.init()

    def close(self) -> None:
        if self._joystick is not None:
            self._joystick.quit()
        self._pygame.quit()

    def poll(self) -> DriverInput:
        self._pygame.event.pump()
        joy = self._joystick
        if joy is None:
            raise RuntimeError("gamepad is not open")
        return DriverInput(
            left_x=clamp_axis(joy.get_axis(self._config["left_x_axis"])),
            left_y=clamp_axis(joy.get_axis(self._config["left_y_axis"])),
            right_x=clamp_axis(joy.get_axis(self._config["right_x_axis"])),
            right_y=clamp_axis(joy.get_axis(self._config["right_y_axis"])),
            mode_button=bool(joy.get_button(self._config["mode_button"])),
            steering_lock_button=bool(joy.get_button(self._config["steering_lock_button"])),
            drive_direction_button=bool(joy.get_button(self._config["drive_direction_button"])),
            emergency_stop_button=bool(joy.get_button(self._config["emergency_stop_button"])),
        )


def load_input_config(path: Path) -> dict[str, int]:
    defaults = {
        "left_x_axis": 0,
        "left_y_axis": 1,
        "right_x_axis": 3,
        "right_y_axis": 4,
        "mode_button": 1,
        "steering_lock_button": 4,
        "drive_direction_button": 5,
        "emergency_stop_button": 6,
    }
    if path.exists():
        raw = json.loads(path.read_text(encoding="utf-8"))
        defaults.update({key: int(raw.get(key, value)) for key, value in defaults.items()})
    return defaults


def describe_gamepads() -> list[str]:
    try:
        import pygame
    except ImportError:
        return ["pygame is not installed; run: python -m pip install pygame"]
    pygame.init()
    pygame.joystick.init()
    try:
        count = pygame.joystick.get_count()
        if count == 0:
            return ["no gamepad detected"]
        lines = []
        for index in range(count):
            joystick = pygame.joystick.Joystick(index)
            joystick.init()
            lines.append(f"{index}: {joystick.get_name()} axes={joystick.get_numaxes()} buttons={joystick.get_numbuttons()}")
            joystick.quit()
        return lines
    finally:
        pygame.quit()


def build_provider(args: argparse.Namespace) -> tuple[Callable[[], DriverInput], Callable[[], None] | None]:
    if args.input == "neutral":
        return DriverInput, None
    if args.input == "demo":
        scripted = ScriptedInput()
        return scripted.poll, None
    if args.input == "gamepad":
        gamepad = PygameGamepadInput(Path(args.input_config), args.gamepad_index)
        gamepad.open()
        return gamepad.poll, gamepad.close
    raise ValueError(f"unsupported input: {args.input}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="UDP remote input sender for car_control_stable")
    parser.add_argument("--host", default="127.0.0.1", help="board IP or hostname")
    parser.add_argument("--port", type=int, default=23333)
    parser.add_argument("--input", choices=["neutral", "demo", "gamepad"], default="demo")
    parser.add_argument("--input-config", default="configs/input.json")
    parser.add_argument("--gamepad-index", type=int, default=0)
    parser.add_argument("--hz", type=float, default=30.0)
    parser.add_argument("--max-seconds", type=float, default=5.0, help="0 means run forever")
    parser.add_argument("--inactive", action="store_true", help="send active=false packets")
    parser.add_argument("--list-gamepads", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.list_gamepads:
        for line in describe_gamepads():
            print(line)
        return 0

    provider, close_provider = build_provider(args)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    period_s = 1.0 / max(1.0, args.hz)
    deadline = None if args.max_seconds <= 0 else time.monotonic() + args.max_seconds
    seq = 0
    try:
        while deadline is None or time.monotonic() < deadline:
            start = time.monotonic()
            sock.sendto(packet_bytes(seq, not args.inactive, provider()), (args.host, args.port))
            seq += 1
            sleep_s = period_s - (time.monotonic() - start)
            if sleep_s > 0:
                time.sleep(sleep_s)
    except KeyboardInterrupt:
        sock.sendto(packet_bytes(seq, True, DriverInput(emergency_stop_button=True)), (args.host, args.port))
    finally:
        if close_provider is not None:
            close_provider()
        sock.close()
    print(f"sent {seq} UDP packets to {args.host}:{args.port}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
