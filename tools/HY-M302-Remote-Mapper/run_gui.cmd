@echo off
setlocal

cd /d "%~dp0"

where py >nul 2>nul
if %errorlevel%==0 (
  py remote_mapper.py gui
  goto :end
)

where python >nul 2>nul
if %errorlevel%==0 (
  python remote_mapper.py gui
  goto :end
)

echo.
echo ERROR: Python was not found in PATH.
echo.
pause

:end
endlocal
