@echo off
title ESISA Interview Simulator
cd /d "%~dp0"
if not exist "AURA.exe" (
    echo ERREUR: AURA.exe introuvable dans ce dossier.
    pause
    exit /b 1
)
start "" "%~dp0AURA.exe"
