param(
    [ValidateSet("mock", "hardware")]
    [string]$Target = "mock",
    [ValidateSet("neutral", "demo", "gamepad", "remote", "hybrid")]
    [string]$InputMode = "demo",
    [ValidateSet("mode0", "mode1", "mode2")]
    [string]$Mode = "mode2",
    [string]$ControlProfile = "",
    [int]$MaxLoops = 0,
    [string]$ApiHost = "127.0.0.1",
    [int]$ApiPort = 8765,
    [switch]$SkipBuild,
    [switch]$SkipApi
)

$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
Push-Location $Root
try {
    if ($Target -eq "hardware") {
        throw "Windows one-key startup only supports -Target mock. Use scripts/check_and_start.sh on the board for hardware."
    }

    New-Item -ItemType Directory -Force -Path "logs" | Out-Null

    $CheckArgs = @("tools_python\config_check.py", "--target", $Target, "--input", $InputMode)
    if ($ControlProfile) {
        $CheckArgs += @("--control-profile", $ControlProfile)
    }
    python @CheckArgs

    if (-not $SkipBuild) {
        .\core_cpp\build.ps1
    }

    $TelemetryFile = "logs\mock_telemetry.jsonl"
    $CoreArgs = @("--mock", "--input", $InputMode, "--mode", $Mode, "--max-loops", "$MaxLoops", "--telemetry-file", $TelemetryFile)
    if ($ControlProfile) {
        $CoreArgs += @("--control-profile", $ControlProfile)
    }

    Write-Output "starting core: .\build\manual\car_control_core.exe $($CoreArgs -join ' ')"
    $CoreProcess = Start-Process -FilePath ".\build\manual\car_control_core.exe" -ArgumentList $CoreArgs -PassThru
    Write-Output "core pid: $($CoreProcess.Id)"

    if (-not $SkipApi) {
        Start-Sleep -Milliseconds 500
        $ApiArgs = @(
            "tools_python\telemetry_api.py",
            "--telemetry-file", $TelemetryFile,
            "--frontend-dir", "frontend",
            "--config-dir", "configs",
            "--calibration-report", "logs\calibration.json",
            "--host", $ApiHost,
            "--port", "$ApiPort"
        )
        Write-Output "starting API: http://$ApiHost`:$ApiPort/"
        $ApiProcess = Start-Process -FilePath "python" -ArgumentList $ApiArgs -PassThru
        Write-Output "api pid: $($ApiProcess.Id)"
        Write-Output "dashboard: http://$ApiHost`:$ApiPort/"
    } else {
        Write-Output "api skipped"
    }

    Write-Output "telemetry: $TelemetryFile"
}
finally {
    Pop-Location
}
