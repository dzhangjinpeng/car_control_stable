from __future__ import annotations

import argparse
import json
import os
import socket
import sys
from pathlib import Path


WHEEL_ROLES = {"front_left", "front_right", "rear_left", "rear_right"}


def read_json(path: Path) -> dict:
    if not path.exists():
        raise FileNotFoundError(str(path))
    return json.loads(path.read_text(encoding="utf-8-sig"))


def check(condition: bool, ok: str, fail: str, issues: list[str]) -> None:
    if condition:
        print(f"PASS {ok}")
    else:
        print(f"FAIL {fail}")
        issues.append(fail)


def validate_hardware(config_dir: Path, issues: list[str]) -> None:
    data = read_json(config_dir / "hardware.json")
    motor_ids = data.get("motor_ids", [])
    drive_ids = data.get("drive_motor_ids", [])
    steer_ids = data.get("steer_motor_ids", [])
    inverted = data.get("inverted_drive_motor_ids", [])
    drive_roles = data.get("drive_motor_roles", {})
    steer_roles = data.get("steer_motor_roles", {})
    all_ids = set(motor_ids)

    check(bool(data.get("serial_number")), "hardware serial_number exists", "hardware.serial_number is empty", issues)
    check(len(all_ids) == len(motor_ids), "motor_ids has no duplicates", "motor_ids has duplicate values", issues)
    check(set(drive_ids).issubset(all_ids), "drive_motor_ids are known", "drive_motor_ids contain unknown ids", issues)
    check(set(steer_ids).issubset(all_ids), "steer_motor_ids are known", "steer_motor_ids contain unknown ids", issues)
    check(not (set(drive_ids) & set(steer_ids)), "drive/steer ids do not overlap", "drive_motor_ids and steer_motor_ids overlap", issues)
    check(set(inverted).issubset(set(drive_ids)), "inverted drive ids are valid", "inverted_drive_motor_ids contain non-drive ids", issues)
    check(set(drive_roles.keys()) == WHEEL_ROLES, "drive_motor_roles has 4 wheel roles", "drive_motor_roles must contain four wheel roles", issues)
    check(set(steer_roles.keys()) == WHEEL_ROLES, "steer_motor_roles has 4 wheel roles", "steer_motor_roles must contain four wheel roles", issues)
    check(set(drive_roles.values()).issubset(all_ids), "drive roles map to known ids", "drive_motor_roles contain unknown ids", issues)
    check(set(steer_roles.values()).issubset(all_ids), "steer roles map to known ids", "steer_motor_roles contain unknown ids", issues)


def validate_control(config_dir: Path, profile: str, issues: list[str]) -> None:
    data = read_json(config_dir / "control.json")
    positive_keys = [
        "loop_period_s",
        "telemetry_interval_s",
        "max_linear_speed_mps",
        "drive_speed_ramp_mps_per_s",
        "wheel_radius_m",
        "wheelbase_m",
        "track_width_m",
        "gear_ratio",
        "steering_motor_speed_limit_rad_s",
    ]
    for key in positive_keys:
        check(float(data.get(key, 0)) > 0, f"control.{key} is positive", f"control.{key} must be positive", issues)
    deadzone = float(data.get("deadzone", -1))
    check(0 <= deadzone <= 1, "control.deadzone is in 0..1", "control.deadzone must be in 0..1", issues)

    profiles = read_json(config_dir / "control_profiles.json")
    if profile:
        check(profile in profiles, f"control profile '{profile}' exists", f"unknown control profile: {profile}", issues)
    else:
        print("PASS no control profile override requested")


def validate_input(config_dir: Path, input_mode: str, target: str, issues: list[str]) -> None:
    data = read_json(config_dir / "input.json")
    for key in (
        "left_x_axis",
        "left_y_axis",
        "right_x_axis",
        "right_y_axis",
        "mode_button",
        "steering_lock_button",
        "drive_direction_button",
        "emergency_stop_button",
    ):
        check(isinstance(data.get(key), int), f"input.{key} is configured", f"input.{key} must be an integer", issues)

    device_path = data.get("device_path", "/dev/input/js0")
    if input_mode in {"gamepad", "hybrid"} and target == "hardware":
        check(Path(device_path).exists(), f"gamepad device exists: {device_path}", f"gamepad device not found: {device_path}", issues)
    elif input_mode in {"gamepad", "hybrid"}:
        print(f"WARN gamepad device is not checked for target={target}; configured path={device_path}")
    else:
        print(f"PASS input mode {input_mode} does not require local gamepad")


def validate_network(config_dir: Path, input_mode: str, issues: list[str]) -> None:
    data = read_json(config_dir / "network.json")
    port = int(data.get("port", 0))
    check(1 <= port <= 65535, "network.port is valid", "network.port must be in 1..65535", issues)
    check(float(data.get("timeout_s", 0)) > 0, "network.timeout_s is positive", "network.timeout_s must be positive", issues)
    if input_mode in {"remote", "hybrid"}:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            sock.bind(("127.0.0.1", port))
            print(f"PASS UDP port {port} is currently free on 127.0.0.1")
        except OSError as exc:
            print(f"WARN UDP port {port} may already be in use on 127.0.0.1: {exc}")
        finally:
            sock.close()


def validate_safety(config_dir: Path, target: str, issues: list[str]) -> None:
    data = read_json(config_dir / "safety.json")
    check(float(data.get("max_drive_motor_speed_rad_s", 0)) > 0, "safety drive speed limit is positive", "safety.max_drive_motor_speed_rad_s must be positive", issues)
    check(float(data.get("max_steer_motor_position_rad", 0)) > 0, "safety steering limit is positive", "safety.max_steer_motor_position_rad must be positive", issues)
    if target == "hardware" and data.get("require_mock_for_now", False):
        issues.append("safety.require_mock_for_now is true; hardware start is blocked")
        print("FAIL safety.require_mock_for_now is true; hardware start is blocked")
    else:
        print("PASS safety target gate is compatible")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Validate car_control_stable config before startup")
    parser.add_argument("--config-dir", default="configs")
    parser.add_argument("--target", choices=["mock", "hardware"], default="mock")
    parser.add_argument("--input", choices=["neutral", "demo", "gamepad", "remote", "hybrid"], default="demo")
    parser.add_argument("--control-profile", default="")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    config_dir = Path(args.config_dir)
    issues: list[str] = []

    print("=== car_control_stable config check ===")
    print(f"config_dir={config_dir.resolve()}")
    print(f"target={args.target} input={args.input} profile={args.control_profile or '-'}")

    try:
        validate_hardware(config_dir, issues)
        validate_control(config_dir, args.control_profile, issues)
        validate_input(config_dir, args.input, args.target, issues)
        validate_network(config_dir, args.input, issues)
        validate_safety(config_dir, args.target, issues)
    except Exception as exc:  # noqa: BLE001
        print(f"FAIL config check crashed: {exc}")
        return 1

    if issues:
        print("\nCONFIG CHECK FAILED")
        for issue in issues:
            print(f"- {issue}")
        return 1

    print("\nCONFIG CHECK PASSED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
