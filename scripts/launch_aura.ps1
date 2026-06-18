param(
    [switch]$Debug
)

$ErrorActionPreference = "Stop"
$script:RequiredDlls = @(
    "libgtk-3-0.dll",
    "libglib-2.0-0.dll",
    "libcurl-4.dll"
)

function Resolve-AuraAppDir {
    $root = Split-Path $PSScriptRoot -Parent
    $candidates = @(
        (Join-Path $root "dist\bin"),
        (Join-Path $root "apps\desktop\bin")
    )
    foreach ($dir in $candidates) {
        if (Test-Path (Join-Path $dir "AURA.exe")) {
            return $dir
        }
    }
    return $null
}

function Import-AuraApiKey {
    param([string]$AppDir)

    if ($env:AURA_API_KEY -and $env:AURA_API_KEY.Trim()) {
        return $true
    }

    $cfg = Join-Path $AppDir "config\aura.cfg"
    if (-not (Test-Path $cfg)) {
        return $false
    }

    $hasKey = $false
    Get-Content $cfg | ForEach-Object {
        if ($_ -match '^\s*groq_api_key\s*=\s*(.+)\s*$') {
            $value = $matches[1].Trim()
            if ($value) {
                $env:AURA_API_KEY = $value
                $hasKey = $true
            }
        }
    }
    return $hasKey
}

function Test-AuraLaunchPrerequisites {
    param([string]$AppDir)

    Write-Host ""
    Write-Host " ========================================"
    if ($Debug) {
        Write-Host "  ESISA AURA - Mode debug (console)"
    } else {
        Write-Host "  ESISA AURA - Simulateur d'entretiens"
    }
    Write-Host " ========================================"
    Write-Host ""
    Write-Host " Dossier: $AppDir"
    Write-Host ""

    if (-not (Test-Path (Join-Path $AppDir "AURA.exe"))) {
        Write-Host "[ERREUR] AURA.exe introuvable."
        Write-Host "Lancez build.ps1 depuis le dossier PFA."
        return $false
    }

    foreach ($dll in $script:RequiredDlls) {
        if (-not (Test-Path (Join-Path $AppDir $dll))) {
            Write-Host "[ERREUR] DLL manquante: $dll"
            Write-Host "Relancez build.ps1 ou package_dlls.ps1."
            return $false
        }
    }

    if (-not (Test-Path (Join-Path $AppDir "data\questions.csv"))) {
        Write-Host "[ATTENTION] data\questions.csv manquant - banque de questions indisponible."
        Write-Host ""
    }

    if (-not (Import-AuraApiKey -AppDir $AppDir)) {
        Write-Host "[ATTENTION] Cle API Groq non configuree."
        Write-Host "Ajoutez groq_api_key dans config\aura.cfg ou la variable AURA_API_KEY."
        Write-Host "L'evaluation IA ne fonctionnera pas sans cle."
        Write-Host ""
    }

    return $true
}

function Wait-AuraProcess {
    param(
        [int]$MaxSeconds = 10
    )

    for ($i = 0; $i -lt $MaxSeconds; $i++) {
        Start-Sleep -Seconds 1
        if (Get-Process -Name "AURA" -ErrorAction SilentlyContinue) {
            return $true
        }
    }
    return $false
}

$env:Path += ";C:\msys64\usr\bin;C:\msys64\mingw64\bin"

$auraDir = Resolve-AuraAppDir
if (-not $auraDir) {
    Write-Error "AURA.exe introuvable. Lancez d'abord build.ps1"
    exit 1
}

if (-not (Test-AuraLaunchPrerequisites -AppDir $auraDir)) {
    exit 1
}

$exePath = Join-Path $auraDir "AURA.exe"

if ($Debug) {
    Write-Host "[DEBUG] Les erreurs s'affichent ci-dessous."
    Write-Host "Fermez la fenetre AURA puis revenez ici pour le code de sortie."
    Write-Host ""
    Push-Location $auraDir
    try {
        & $exePath
        $exitCode = $LASTEXITCODE
    } finally {
        Pop-Location
    }
    Write-Host ""
    Write-Host "Code de sortie: $exitCode"
    exit $exitCode
}

$process = Start-Process -FilePath $exePath -WorkingDirectory $auraDir -PassThru
if (-not (Wait-AuraProcess)) {
    Write-Host "[ERREUR] AURA ne s'est pas lance ou s'est arrete immediatement."
    Write-Host "Relancez avec: .\launch_aura.ps1 -Debug"
    exit 1
}

Write-Host "[OK] AURA demarre - cherchez la fenetre 'AURA - Connexion ESISA'."
exit 0