$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
Push-Location $Root
try {
    python .\tools_python\remote_control_sender.py --host 127.0.0.1 --port 23333 --input demo --hz 30 --max-seconds 5
}
finally {
    Pop-Location
}
