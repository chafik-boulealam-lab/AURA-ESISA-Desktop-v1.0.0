@echo off
cd /d "%~dp0bin\bin"
if not exist "Lancer_AURA.bat" (
    echo ERREUR: dossier bin\bin introuvable. Lancez d'abord build.ps1
    pause
    exit /b 1
)
call "%~dp0bin\bin\Lancer_AURA.bat"