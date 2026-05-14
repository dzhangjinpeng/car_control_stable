#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
python3 tools_python/remote_control_sender.py --host "${REMOTE_HOST:-127.0.0.1}" --port "${REMOTE_PORT:-23333}" --input demo --hz 30 --max-seconds 5
