#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

python3 tools_python/telemetry_api.py \
  --telemetry-file "${TELEMETRY_FILE:-logs/hardware_telemetry.jsonl}" \
  --frontend-dir frontend \
  --host "${API_HOST:-0.0.0.0}" \
  --port "${API_PORT:-8765}"

