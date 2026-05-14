#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

cmake -S core_cpp -B build/core_cpp \
  -DCAR_CONTROL_WITH_DAMIAO=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/core_cpp -j"$(nproc)"

echo "built: $ROOT/build/core_cpp/car_control_core"

