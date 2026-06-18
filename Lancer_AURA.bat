@echo off
chcp 65001 >nul
call "%~dp0AURA.bat" /debug %*
exit /b %ERRORLEVEL%