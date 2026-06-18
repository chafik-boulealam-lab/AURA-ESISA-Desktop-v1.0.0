@echo off
chcp 65001 >nul
title ESISA - AURA Interview Simulator
setlocal EnableDelayedExpansion

set "DEBUG_MODE=0"
if /I "%~1"=="/debug" set "DEBUG_MODE=1"
if /I "%~1"=="-debug" set "DEBUG_MODE=1"
if defined AURA_DEBUG if /I "!AURA_DEBUG!"=="1" set "DEBUG_MODE=1"

set "SCRIPT_DIR=%~dp0"
set "APP_DIR=%SCRIPT_DIR%"

if exist "%SCRIPT_DIR%bin\bin\AURA.exe" (
    set "APP_DIR=%SCRIPT_DIR%bin\bin"
) else if not exist "%APP_DIR%\AURA.exe" (
    if exist "%SCRIPT_DIR%bin\AURA.exe" (
        set "APP_DIR=%SCRIPT_DIR%bin"
    )
)

cd /d "%APP_DIR%"

echo.
echo  ========================================
if "!DEBUG_MODE!"=="1" (
    echo   ESISA AURA - Mode debug ^(console^)
) else (
    echo   ESISA AURA - Simulateur d'entretiens
)
echo  ========================================
echo.
echo  Dossier: %APP_DIR%
echo.

if not exist "%APP_DIR%\AURA.exe" (
    echo [ERREUR] AURA.exe introuvable.
    echo Lancez build.ps1 depuis le dossier PFA.
    echo.
    pause
    exit /b 1
)

set "DLL_OK=1"
for %%D in (libgtk-3-0.dll libglib-2.0-0.dll libcurl-4.dll) do (
    if not exist "%APP_DIR%\%%D" (
        echo [ERREUR] DLL manquante: %%D
        set "DLL_OK=0"
    )
)
if "!DLL_OK!"=="0" (
    echo Relancez build.ps1 ou package_dlls.ps1.
    echo.
    pause
    exit /b 1
)

if not exist "%APP_DIR%\data\questions.csv" (
    echo [ATTENTION] data\questions.csv manquant - banque de questions indisponible.
    echo.
)

set "HAS_KEY=0"
if defined AURA_API_KEY (
    if not "!AURA_API_KEY!"=="" set "HAS_KEY=1"
)
if "!HAS_KEY!"=="0" if exist "%APP_DIR%\config\aura.cfg" (
    findstr /I /R "groq_api_key.*=.*gsk_" "%APP_DIR%\config\aura.cfg" >nul 2>&1
    if not errorlevel 1 set "HAS_KEY=1"
)
if "!HAS_KEY!"=="0" (
    echo [ATTENTION] Cle API Groq non configuree.
    echo Ajoutez groq_api_key dans config\aura.cfg ou la variable AURA_API_KEY.
    echo L'evaluation IA ne fonctionnera pas sans cle.
    echo.
)

if "!DEBUG_MODE!"=="1" (
    echo [DEBUG] Les erreurs s'affichent ci-dessous.
    echo Fermez la fenetre AURA puis revenez ici pour le code de sortie.
    echo.
    "%APP_DIR%\AURA.exe"
    set "EXIT_CODE=!ERRORLEVEL!"
    echo.
    echo Code de sortie: !EXIT_CODE!
    pause
    exit /b !EXIT_CODE!
)

start "AURA" /D "%APP_DIR%" "%APP_DIR%\AURA.exe"

set "LAUNCHED=0"
for /L %%i in (1,1,10) do (
    timeout /t 1 /nobreak >nul
    tasklist /FI "IMAGENAME eq AURA.exe" 2>nul | find /I "AURA.exe" >nul
    if not errorlevel 1 (
        set "LAUNCHED=1"
        goto :launch_ok
    )
)
:launch_ok

if "!LAUNCHED!"=="0" (
    echo [ERREUR] AURA ne s'est pas lance ou s'est arrete immediatement.
    echo Lancez Lancer_AURA.bat ou AURA.bat /debug pour voir les erreurs.
    echo.
    pause
    exit /b 1
)

echo [OK] AURA demarre - cherchez la fenetre "AURA - Connexion ESISA".
echo.
timeout /t 2 >nul
exit /b 0