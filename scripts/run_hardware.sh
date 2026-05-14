#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

if [[ ! -x build/core_cpp/car_control_core ]]; then
  echo "missing build/core_cpp/car_control_core; run scripts/build_board.sh first" >&2
  exit 1
fi

mkdir -p logs
INPUT_MODE="${CAR_INPUT:-gamepad}"
EXTRA=()
if [[ -n "${CONTROL_PROFILE:-}" ]]; then
  EXTRA+=(--control-profile "$CONTROL_PROFILE")
fi
./build/core_cpp/car_control_core \
  --input "$INPUT_MODE" \
  --gamepad-device "${GAMEPAD_DEVICE:-/dev/input/js0}" \
  --network-config "${NETWORK_CONFIG:-configs/network.json}" \
  --mode "${CAR_MODE:-mode2}" \
  --max-loops "${MAX_LOOPS:-0}" \
  --telemetry-file "${TELEMETRY_FILE:-logs/hardware_telemetry.jsonl}" \
  "${EXTRA[@]}"
