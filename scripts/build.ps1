$ProjectRoot = Split-Path $PSScriptRoot -Parent
$DesktopDir = Join-Path $ProjectRoot "apps\desktop"
$DistDir = Join-Path $ProjectRoot "dist\bin"
$DesktopBin = Join-Path $DesktopDir "bin\AURA.exe"

$driveLetter = $DesktopDir.Substring(0, 1).ToLower()
$bashPath = "/$driveLetter/" + ($DesktopDir.Substring(3) -replace '\\', '/')

taskkill /IM AURA.exe /F 2>$null | Out-Null
C:\msys64\usr\bin\bash.exe -lc "export PATH=/mingw64/bin:/usr/bin:`$PATH; export PKG_CONFIG_PATH=/mingw64/lib/pkgconfig; cd '$bashPath' && mingw32-make clean && mingw32-make all"
if ($LASTEXITCODE -ne 0) {
    Write-Host "Clean echoue, build sans clean..."
    C:\msys64\usr\bin\bash.exe -lc "export PATH=/mingw64/bin:/usr/bin:`$PATH; export PKG_CONFIG_PATH=/mingw64/lib/pkgconfig; cd '$bashPath' && mingw32-make all"
}
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

New-Item -ItemType Directory -Force -Path "$DistDir\data" | Out-Null
New-Item -ItemType Directory -Force -Path "$DistDir\config" | Out-Null
Copy-Item $DesktopBin "$DistDir\AURA.exe" -Force
Copy-Item "$ProjectRoot\data\questions.csv" "$DistDir\data\questions.csv" -Force
Copy-Item "$ProjectRoot\data\local.db" "$DistDir\data\local.db" -Force -ErrorAction SilentlyContinue
powershell -ExecutionPolicy Bypass -File "$PSScriptRoot\package_dlls.ps1"

$caDest = "$DistDir\cacert.pem"
if (-not (Test-Path $caDest)) {
    $caSrc = "C:\msys64\mingw64\etc\ssl\certs\ca-bundle.crt"
    if (Test-Path $caSrc) {
        Copy-Item $caSrc $caDest -Force
        Write-Host "CA bundle: cacert.pem"
    }
}

if (Test-Path "$ProjectRoot\config\aura.cfg") {
    Copy-Item "$ProjectRoot\config\aura.cfg" "$DistDir\config\aura.cfg" -Force
    Write-Host "Config: cle Groq copiee depuis config\aura.cfg"
} elseif (-not (Test-Path "$DistDir\config\aura.cfg")) {
    Copy-Item "$ProjectRoot\config\aura.cfg.example" "$DistDir\config\aura.cfg" -Force -ErrorAction SilentlyContinue
    Write-Host "ATTENTION: ajoutez groq_api_key dans config\aura.cfg"
}

Copy-Item "$ProjectRoot\AURA.bat" "$DistDir\AURA.bat" -Force
Copy-Item "$PSScriptRoot\Lancer_AURA_console.bat" "$DistDir\Lancer_AURA.bat" -Force

$assetSrc = Join-Path $ProjectRoot "assets"
$assetDest = Join-Path $DistDir "assets"
if (Test-Path $assetSrc) {
    New-Item -ItemType Directory -Force -Path $assetDest | Out-Null
    Copy-Item "$assetSrc\*" $assetDest -Recurse -Force
    Write-Host "Assets: assets\ -> dist\bin\assets"
}

Write-Host "Build OK: dist\bin\AURA.exe"
Write-Host "Lancement: AURA.bat (racine) ou dist\bin\AURA.bat"
Write-Host "Debug:     AURA.bat /debug | Lancer_AURA.bat | .\scripts\launch_aura.ps1 -Debug"
Write-Host "Release:   .\scripts\package_release.ps1"