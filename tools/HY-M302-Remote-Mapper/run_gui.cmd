@echo off
setlocal

cd /d "%~dp0\..\.."

where py >nul 2>nul
if %errorlevel%==0 (
  py ".\tools\HY-M302-Remote-Mapper\remote_mapper.py" gui
  goto :end
)

where python >nul 2>nul
if %errorlevel%==0 (
  python ".\tools\HY-M302-Remote-Mapper\remote_mapper.py" gui
  goto :end
)

echo.
echo ERROR: Python was not found in PATH.
echo Install Python 3 or add py/python to PATH.
echo.
pause

:end
endlocal
