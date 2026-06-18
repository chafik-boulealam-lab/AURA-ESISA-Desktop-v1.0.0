@echo off
chcp 65001 >nul
set "ROOT_BAT=%~dp0..\AURA.bat"
if not exist "%ROOT_BAT%" set "ROOT_BAT=%~dp0..\..\AURA.bat"
if exist "%ROOT_BAT%" (
    call "%ROOT_BAT%" /debug %*
    exit /b %ERRORLEVEL%
)
echo [ERREUR] AURA.bat introuvable. Lancez scripts\build.ps1
pause
exit /b 1