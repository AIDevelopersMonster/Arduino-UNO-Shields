@echo off
setlocal EnableExtensions

cd /d "%~dp0\..\.."

set "JSON_FILE=%~1"

if not "%JSON_FILE%"=="" goto :have_json

for /f "usebackq delims=" %%I in (`powershell -NoProfile -STA -Command ^
  "Add-Type -AssemblyName System.Windows.Forms; $d=New-Object System.Windows.Forms.OpenFileDialog; $d.Title='Select HY-M302 remote_map.json'; $d.Filter='HY-M302 remote profile (remote_map.json)|remote_map.json|JSON files (*.json)|*.json|All files (*.*)|*.*'; if($d.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK){$d.FileName}"`) do set "JSON_FILE=%%I"

if "%JSON_FILE%"=="" (
  echo.
  echo No JSON file selected.
  echo.
  goto :end
)

:have_json
if not exist "%JSON_FILE%" (
  echo.
  echo ERROR: File not found:
  echo %JSON_FILE%
  echo.
  pause
  goto :end
)

echo.
echo Source JSON:
echo %JSON_FILE%
echo.

where py >nul 2>nul
if %errorlevel%==0 (
  py ".\tools\HY-M302-Remote-Mapper\remote_mapper.py" generate --json "%JSON_FILE%"
  goto :result
)

where python >nul 2>nul
if %errorlevel%==0 (
  python ".\tools\HY-M302-Remote-Mapper\remote_mapper.py" generate --json "%JSON_FILE%"
  goto :result
)

echo.
echo ERROR: Python was not found in PATH.
echo Install Python 3 or add py/python to PATH.
echo.
pause
goto :end

:result
if %errorlevel% neq 0 (
  echo.
  echo Generation failed.
  echo.
  pause
  goto :end
)

echo.
echo DONE.
echo HY_M302_RemoteMap.h and HY_M302_RemoteMap.cpp were generated
echo next to the selected remote_map.json file.
echo.
pause

:end
endlocal
