$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$PackageDir = Join-Path $Root "dist"
$PackagePath = Join-Path $PackageDir "car_control_stable_release.zip"

New-Item -ItemType Directory -Force -Path $PackageDir | Out-Null
if (Test-Path -LiteralPath $PackagePath) {
    Remove-Item -LiteralPath $PackagePath -Force
}

$Items = @(
    "configs",
    "core_cpp",
    "docs",
    "protocols",
    "scripts",
    "tools_python",
    "README.md"
)

$Temp = Join-Path $PackageDir "car_control_stable"
if (Test-Path -LiteralPath $Temp) {
    Remove-Item -LiteralPath $Temp -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $Temp | Out-Null

foreach ($item in $Items) {
    Copy-Item -LiteralPath (Join-Path $Root $item) -Destination $Temp -Recurse -Force
}

Get-ChildItem -Path $Temp -Recurse -Directory -Filter build | Remove-Item -Recurse -Force
Get-ChildItem -Path $Temp -Recurse -Directory -Filter .git | Remove-Item -Recurse -Force
Get-ChildItem -Path $Temp -Recurse -File -Filter "*.jsonl" | Remove-Item -Force

Compress-Archive -Path $Temp -DestinationPath $PackagePath -Force
Remove-Item -LiteralPath $Temp -Recurse -Force

Write-Output "release package: $PackagePath"

