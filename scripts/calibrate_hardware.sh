#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

if [[ ! -x build/core_cpp/car_control_core ]]; then
  echo "missing build/core_cpp/car_control_core; run scripts/build_board.sh first" >&2
  exit 1
fi

mkdir -p logs
ACTION="${CALIBRATE_ACTION:-verify}"

EXTRA=()
if [[ "${YES:-0}" == "1" ]]; then
  EXTRA+=(--yes)
fi
if [[ "${SAVE_FLASH:-0}" == "1" ]]; then
  EXTRA+=(--save-flash)
fi

./build/core_cpp/car_control_core \
  --calibrate "$ACTION" \
  --report-file "${REPORT_FILE:-logs/calibration.json}" \
  "${EXTRA[@]}"

