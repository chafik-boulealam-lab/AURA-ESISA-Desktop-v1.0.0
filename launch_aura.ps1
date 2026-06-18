$env:Path += ";C:\msys64\usr\bin;C:\msys64\mingw64\bin"

$auraDir = Join-Path $PSScriptRoot "bin\bin"
if (-not (Test-Path (Join-Path $auraDir "AURA.exe"))) {
    $auraDir = Join-Path $PSScriptRoot "bin"
}
if (-not (Test-Path (Join-Path $auraDir "AURA.exe"))) {
    Write-Error "AURA.exe introuvable. Lancez d'abord build.ps1"
    exit 1
}

$cfg = Join-Path $auraDir "config\aura.cfg"
if (Test-Path $cfg) {
    Get-Content $cfg | ForEach-Object {
        if ($_ -match '^\s*groq_api_key\s*=\s*(.+)\s*$') {
            $env:AURA_API_KEY = $matches[1].Trim()
        }
    }
}

Start-Process -FilePath (Join-Path $auraDir "AURA.exe") -WorkingDirectory $auraDir