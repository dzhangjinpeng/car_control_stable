#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

cmake -S core_cpp -B build/core_cpp -DCMAKE_BUILD_TYPE=Release
cmake --build build/core_cpp -j"$(nproc)"

mkdir -p logs
EXTRA=()
if [[ -n "${CONTROL_PROFILE:-}" ]]; then
  EXTRA+=(--control-profile "$CONTROL_PROFILE")
fi
./build/core_cpp/car_control_core \
  --mock \
  --input demo \
  --mode mode2 \
  --max-loops 1000 \
  --telemetry-file logs/mock_telemetry.jsonl \
  "${EXTRA[@]}"
