$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$OutDir = Join-Path $Root "build\manual"
$OutExe = Join-Path $OutDir "car_control_core.exe"

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$Sources = @(
    "core_cpp\src\app_config.cpp",
    "core_cpp\src\car_controller.cpp",
    "core_cpp\src\damiao_motor_client.cpp",
    "core_cpp\src\drive_smoothing.cpp",
    "core_cpp\src\input_filter.cpp",
    "core_cpp\src\input_source.cpp",
    "core_cpp\src\main.cpp",
    "core_cpp\src\mock_motor_client.cpp",
    "core_cpp\src\safety_manager.cpp",
    "core_cpp\src\telemetry.cpp"
)

Push-Location $Root
try {
    $LinkArgs = @()
    if ($IsWindows -or $env:OS -eq "Windows_NT") {
        $LinkArgs += "-lws2_32"
    }
    & g++ -std=c++17 -O2 -Wall -Wextra -Icore_cpp\src @Sources -o $OutExe @LinkArgs
    if ($LASTEXITCODE -ne 0) {
        throw "g++ failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

Write-Output "built: $OutExe"
