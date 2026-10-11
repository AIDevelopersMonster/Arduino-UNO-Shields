@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Relay-Serial-GUI.ps1" -Port COM4
