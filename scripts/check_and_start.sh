#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

TARGET="${TARGET:-mock}"
INPUT_MODE="${CAR_INPUT:-demo}"
CAR_MODE="${CAR_MODE:-mode2}"
CONTROL_PROFILE="${CONTROL_PROFILE:-}"
MAX_LOOPS="${MAX_LOOPS:-0}"
API_HOST="${API_HOST:-0.0.0.0}"
API_PORT="${API_PORT:-8765}"
TELEMETRY_FILE="${TELEMETRY_FILE:-logs/${TARGET}_telemetry.jsonl}"

mkdir -p logs

CHECK_ARGS=(tools_python/config_check.py --target "$TARGET" --input "$INPUT_MODE")
if [[ -n "$CONTROL_PROFILE" ]]; then
  CHECK_ARGS+=(--control-profile "$CONTROL_PROFILE")
fi
python3 "${CHECK_ARGS[@]}"

if [[ "$TARGET" == "hardware" ]]; then
  ./scripts/build_board.sh
  CORE="./build/core_cpp/car_control_core"
  CORE_ARGS=(--input "$INPUT_MODE" --mode "$CAR_MODE" --max-loops "$MAX_LOOPS" --telemetry-file "$TELEMETRY_FILE")
else
  cmake -S core_cpp -B build/core_cpp -DCMAKE_BUILD_TYPE=Release
  cmake --build build/core_cpp -j"$(nproc)"
  CORE="./build/core_cpp/car_control_core"
  CORE_ARGS=(--mock --input "$INPUT_MODE" --mode "$CAR_MODE" --max-loops "$MAX_LOOPS" --telemetry-file "$TELEMETRY_FILE")
fi

if [[ -n "$CONTROL_PROFILE" ]]; then
  CORE_ARGS+=(--control-profile "$CONTROL_PROFILE")
fi

echo "starting core: $CORE ${CORE_ARGS[*]}"
"$CORE" "${CORE_ARGS[@]}" &
CORE_PID=$!
echo "core pid: $CORE_PID"

echo "starting API: http://${API_HOST}:${API_PORT}/"
python3 tools_python/telemetry_api.py \
  --telemetry-file "$TELEMETRY_FILE" \
  --frontend-dir frontend \
  --config-dir configs \
  --calibration-report "${CALIBRATION_REPORT:-logs/calibration.json}" \
  --host "$API_HOST" \
  --port "$API_PORT" &
API_PID=$!
echo "api pid: $API_PID"

echo "dashboard: http://${API_HOST}:${API_PORT}/"
echo "stop with: kill $CORE_PID $API_PID"
wait "$CORE_PID"
