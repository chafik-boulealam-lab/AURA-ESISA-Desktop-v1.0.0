taskkill /IM AURA.exe /F 2>$null | Out-Null
C:\msys64\usr\bin\bash.exe -lc "export PATH=/mingw64/bin:/usr/bin:`$PATH; export PKG_CONFIG_PATH=/mingw64/lib/pkgconfig; cd /c/Users/ok/Downloads/PFA && mingw32-make clean && mingw32-make all"
if ($LASTEXITCODE -ne 0) {
    Write-Host "Clean echoue, build sans clean..."
    C:\msys64\usr\bin\bash.exe -lc "export PATH=/mingw64/bin:/usr/bin:`$PATH; export PKG_CONFIG_PATH=/mingw64/lib/pkgconfig; cd /c/Users/ok/Downloads/PFA && mingw32-make all"
}
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
New-Item -ItemType Directory -Force -Path "$PSScriptRoot\bin\bin\data" | Out-Null
New-Item -ItemType Directory -Force -Path "$PSScriptRoot\bin\bin\config" | Out-Null
Copy-Item "$PSScriptRoot\bin\AURA.exe" "$PSScriptRoot\bin\bin\AURA.exe" -Force
Copy-Item "$PSScriptRoot\data\questions.csv" "$PSScriptRoot\bin\bin\data\questions.csv" -Force
Copy-Item "$PSScriptRoot\data\local.db" "$PSScriptRoot\bin\bin\data\local.db" -Force -ErrorAction SilentlyContinue
powershell -ExecutionPolicy Bypass -File "$PSScriptRoot\package_dlls.ps1"
@'
@echo off
title ESISA Interview Simulator
cd /d "%~dp0"
if not exist "AURA.exe" (
    echo ERREUR: AURA.exe introuvable dans ce dossier.
    pause
    exit /b 1
)
start "" "%~dp0AURA.exe"
'@ | Set-Content -Path "$PSScriptRoot\bin\bin\Lancer_AURA.bat" -Encoding ASCII
if (Test-Path "$PSScriptRoot\config\aura.cfg") {
    Copy-Item "$PSScriptRoot\config\aura.cfg" "$PSScriptRoot\bin\bin\config\aura.cfg" -Force
    Write-Host "Config: cle Groq copiee depuis config\aura.cfg"
} elseif (-not (Test-Path "$PSScriptRoot\bin\bin\config\aura.cfg")) {
    Copy-Item "$PSScriptRoot\config\aura.cfg.example" "$PSScriptRoot\bin\bin\config\aura.cfg" -Force -ErrorAction SilentlyContinue
    Write-Host "ATTENTION: ajoutez groq_api_key dans config\aura.cfg"
}
Write-Host "Build OK: bin\bin\AURA.exe"
Write-Host "Lancement: double-cliquez bin\bin\Lancer_AURA.bat"