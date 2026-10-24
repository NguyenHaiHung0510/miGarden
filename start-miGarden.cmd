@echo off
setlocal
cd /d "%~dp0"
where node >nul 2>nul
if errorlevel 1 (
  echo Node.js is required. Install Node.js 22.8 or newer, then run this file again.
  pause
  exit /b 1
)
if not exist "viewer\node_modules\three\package.json" (
  echo Installing locked viewer dependencies...
  call npm ci --ignore-scripts --prefix viewer
  if errorlevel 1 (
    echo Dependency installation failed. Your model and feedback were not changed.
    pause
    exit /b 1
  )
)
node "viewer\start.mjs" %*
if errorlevel 1 pause
