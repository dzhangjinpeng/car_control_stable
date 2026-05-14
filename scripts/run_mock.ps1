$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
Push-Location $Root
try {
    New-Item -ItemType Directory -Force -Path "logs" | Out-Null
    .\core_cpp\build.ps1
    .\build\manual\car_control_core.exe --mock --input demo --mode mode2 --max-loops 1000 --telemetry-file logs\mock_telemetry.jsonl
}
finally {
    Pop-Location
}
