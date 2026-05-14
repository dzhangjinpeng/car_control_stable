#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

export TARGET="${TARGET:-hardware}"
export CAR_INPUT="${CAR_INPUT:-gamepad}"
export CAR_MODE="${CAR_MODE:-mode2}"
export CONTROL_PROFILE="${CONTROL_PROFILE:-${STARTUP_PRESET:-conservative}}"

exec ./scripts/check_and_start.sh
