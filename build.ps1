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

$caDest = "$PSScriptRoot\bin\bin\cacert.pem"
if (-not (Test-Path $caDest)) {
    $caSrc = "C:\msys64\mingw64\etc\ssl\certs\ca-bundle.crt"
    if (Test-Path $caSrc) {
        Copy-Item $caSrc $caDest -Force
        Write-Host "CA bundle: cacert.pem"
    }
}

if (Test-Path "$PSScriptRoot\config\aura.cfg") {
    Copy-Item "$PSScriptRoot\config\aura.cfg" "$PSScriptRoot\bin\bin\config\aura.cfg" -Force
    Write-Host "Config: cle Groq copiee depuis config\aura.cfg"
} elseif (-not (Test-Path "$PSScriptRoot\bin\bin\config\aura.cfg")) {
    Copy-Item "$PSScriptRoot\config\aura.cfg.example" "$PSScriptRoot\bin\bin\config\aura.cfg" -Force -ErrorAction SilentlyContinue
    Write-Host "ATTENTION: ajoutez groq_api_key dans config\aura.cfg"
}

Copy-Item "$PSScriptRoot\AURA.bat" "$PSScriptRoot\bin\bin\AURA.bat" -Force
Copy-Item "$PSScriptRoot\scripts\Lancer_AURA_console.bat" "$PSScriptRoot\bin\bin\Lancer_AURA.bat" -Force

$assetSrc = "$PSScriptRoot\release_templates\assets"
$assetDest = "$PSScriptRoot\bin\bin\assets"
if (Test-Path $assetSrc) {
    New-Item -ItemType Directory -Force -Path $assetDest | Out-Null
    Copy-Item "$assetSrc\*" $assetDest -Recurse -Force
    Write-Host "Assets: release_templates\assets -> bin\bin\assets"
}

Write-Host "Build OK: bin\bin\AURA.exe"
Write-Host "Lancement: AURA.bat (racine) ou bin\bin\AURA.bat"
Write-Host "Debug:     AURA.bat /debug | Lancer_AURA.bat | .\launch_aura.ps1 -Debug"
Write-Host "Release:   .\scripts\package_release.ps1"