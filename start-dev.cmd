@echo off
rem Dev launcher, see tools\dev\start-dev.ps1 for options (-Build, -NoServer, -NoGame, -GamePath, -Ip, -Port)
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\dev\start-dev.ps1" %*
