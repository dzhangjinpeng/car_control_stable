$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
Push-Location $Root
try {
    python tools_python\telemetry_api.py --telemetry-file logs\mock_telemetry.jsonl --frontend-dir frontend --host 127.0.0.1 --port 8765
}
finally {
    Pop-Location
}

