@echo off
chcp 65001 >nul
set "ROOT_BAT=%~dp0..\..\AURA.bat"
if exist "%ROOT_BAT%" (
    call "%ROOT_BAT%" /debug %*
    exit /b %ERRORLEVEL%
)
cd /d "%~dp0"
echo [ERREUR] AURA.bat introuvable. Lancez build.ps1 depuis le dossier PFA.
pause
exit /b 1