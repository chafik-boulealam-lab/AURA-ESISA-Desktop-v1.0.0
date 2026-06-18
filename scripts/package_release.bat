@echo off
REM Packaging script for AURA release (Windows)
setlocal enabledelayedexpansion

if "%~1"=="" (
  echo Usage: package_release.bat [path-to-bin]
  echo Example: package_release.bat bin
  exit /b 1
)

set BIN_DIR=%~1
set RELEASE_DIR=AURA

if exist "%RELEASE_DIR%" rmdir /S /Q "%RELEASE_DIR%"
mkdir "%RELEASE_DIR%"
mkdir "%RELEASE_DIR%\assets"
mkdir "%RELEASE_DIR%\assets\images"
mkdir "%RELEASE_DIR%\assets\sounds"
mkdir "%RELEASE_DIR%\assets\videos"
mkdir "%RELEASE_DIR%\assets\fonts"
mkdir "%RELEASE_DIR%\assets\animations"
mkdir "%RELEASE_DIR%\data"
mkdir "%RELEASE_DIR%\config"
mkdir "%RELEASE_DIR%\cache"
mkdir "%RELEASE_DIR%\logs"

echo Copying executable...
if exist "%BIN_DIR%\AURA.exe" (
  copy /Y "%BIN_DIR%\AURA.exe" "%RELEASE_DIR%\AURA.exe"
) else if exist "%BIN_DIR%\AURA.exe" (
  copy /Y "%BIN_DIR%\AURA.exe" "%RELEASE_DIR%\AURA.exe"
) else (
  echo Built binary not found in %BIN_DIR%\AURA.exe
  echo Make sure to run "make" first.
)

REM Copy existing data or templates
if exist "%BIN_DIR%\..\data\accounts.txt" (
  copy /Y "%BIN_DIR%\..\data\accounts.txt" "%RELEASE_DIR%\data\accounts.txt"
) else (
  copy /Y "release_templates\accounts.txt" "%RELEASE_DIR%\data\accounts.txt" >NUL 2>&1 || echo No accounts template
)

copy /Y "release_templates\scores.txt" "%RELEASE_DIR%\data\scores.txt" >NUL 2>&1
copy /Y "release_templates\reports.txt" "%RELEASE_DIR%\data\reports.txt" >NUL 2>&1
copy /Y "release_templates\questions.txt" "%RELEASE_DIR%\data\questions.txt" >NUL 2>&1

copy /Y "release_templates\aura_config.cfg" "%RELEASE_DIR%\config\aura_config.cfg" >NUL 2>&1
copy /Y "release_templates\aura_config.cfg" "%RELEASE_DIR%\config\aura.cfg" >NUL 2>&1

REM Copy placeholder assets if no built assets exist
if exist "bin\assets" (
  xcopy /E /I /Y "bin\assets" "%RELEASE_DIR%\assets" >NUL 2>&1
) else (
  copy /Y "release_templates\assets\images\logo.png" "%RELEASE_DIR%\assets\images\logo.png" >NUL 2>&1
  copy /Y "release_templates\assets\sounds\intro.mp3" "%RELEASE_DIR%\assets\sounds\intro.mp3" >NUL 2>&1
  copy /Y "release_templates\assets\videos\intro.mp4" "%RELEASE_DIR%\assets\videos\intro.mp4" >NUL 2>&1
  copy /Y "release_templates\assets\fonts\Inter-Regular.ttf" "%RELEASE_DIR%\assets\fonts\Inter-Regular.ttf" >NUL 2>&1
  copy /Y "release_templates\assets\animations\intro.anim" "%RELEASE_DIR%\assets\animations\intro.anim" >NUL 2>&1
)

echo Release assembled in %RELEASE_DIR%
echo You can distribute the folder or create an installer.
endlocal
