param(
    [string]$OutputName = "AURA-ESISA"
)

$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$binDir = Join-Path $root "bin\bin"
$releaseDir = Join-Path $root $OutputName

if (-not (Test-Path (Join-Path $binDir "AURA.exe"))) {
    Write-Host "Build manquant - lancement de build.ps1..."
    & (Join-Path $root "build.ps1")
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

if (Test-Path $releaseDir) {
    Remove-Item $releaseDir -Recurse -Force
}

Write-Host "Assemblage release: $releaseDir"
Copy-Item $binDir $releaseDir -Recurse

$readmeLines = @(
    "AURA - ESISA Interview Simulator",
    "================================",
    "",
    "Lancement: double-cliquez AURA.bat",
    "Debug:     AURA.bat /debug",
    "Config:    config\aura.cfg (groq_api_key=...)",
    "",
    "Projet academique ESISA - usage educatif."
)
Set-Content -Path (Join-Path $releaseDir "LISEZMOI.txt") -Value $readmeLines -Encoding UTF8

$zipPath = Join-Path $root "$OutputName.zip"
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
Compress-Archive -Path $releaseDir -DestinationPath $zipPath -Force

Write-Host "Release OK:"
Write-Host "  Dossier: $releaseDir"
Write-Host "  Archive: $zipPath"