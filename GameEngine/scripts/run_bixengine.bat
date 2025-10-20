@echo off
setlocal enabledelayedexpansion

REM ---------------------------------------------------------------------------
REM Launcher script used by JetBrains Rider (Windows) to execute the BixEngine sample.
REM It rebuilds the xmake target before running it, mirroring the Unix launcher.
REM ---------------------------------------------------------------------------

set SCRIPT_DIR=%~dp0
if "%SCRIPT_DIR:~-1%"=="\" set SCRIPT_DIR=%SCRIPT_DIR:~0,-1%
set REPO_ROOT=%SCRIPT_DIR%\..
set ENGINE_ROOT=%REPO_ROOT%\BixEngine

if not exist "%ENGINE_ROOT%" (
    echo [run_bixengine] Error: Unable to locate BixEngine folder at %ENGINE_ROOT%.
    exit /b 1
)

pushd "%ENGINE_ROOT%" >nul

where xmake >nul 2>nul
if errorlevel 1 (
    echo [run_bixengine] Error: xmake is not available in PATH.
    echo Install xmake from https://xmake.io or ensure it is added to PATH.
    popd >nul
    exit /b 1
)

xmake build BixEngine
if errorlevel 1 (
    popd >nul
    exit /b 1
)

if "%*"=="" (
    xmake run BixEngine
) else (
    xmake run BixEngine -- %*
)
set EXIT_CODE=%ERRORLEVEL%

popd >nul
exit /b %EXIT_CODE%
