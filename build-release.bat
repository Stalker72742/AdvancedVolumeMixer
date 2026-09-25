@echo off
rem ---------------------------------------------------------------------------
rem  Release build + clean deploy into .\dist (only what the app needs to run)
rem
rem  Usage: build-release.bat [options] [QtKitFolder]
rem    QtKitFolder     Qt kit, e.g. C:\Qt\6.8.3\mingw_64 (saved to build.local)
rem    --reconfigure   forget the saved Qt folder and the build cache, ask again
rem    --zip           also pack dist into AdvancedVolumeMixer.zip
rem    --no-pause      don't wait for a key at the end
rem
rem  Qt folder lookup: argument > build.local > %%QTDIR%% > ask.
rem  MinGW, Ninja and CMake are picked up from the same Qt installation
rem  (Tools\mingw*_64, Tools\Ninja, Tools\CMake_64) or from PATH.
rem ---------------------------------------------------------------------------
setlocal EnableExtensions EnableDelayedExpansion

set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"
set "BUILD=%ROOT%\build-release"
set "DIST=%ROOT%\dist"
set "LOCAL=%ROOT%\build.local"
set "ZIP=0"
set "RECONF=0"
set "PAUSE_AT_END=0"
set "QT_DIR="

rem Double-clicked from Explorer -> keep the window open at the end
set "CMDLINE=%cmdcmdline%"
if /i not "!CMDLINE:/c=!"=="!CMDLINE!" set "PAUSE_AT_END=1"

:args
if "%~1"=="" goto args_done
if /i "%~1"=="--zip"         (set "ZIP=1"          & shift & goto args)
if /i "%~1"=="--reconfigure" (set "RECONF=1"       & shift & goto args)
if /i "%~1"=="--no-pause"    (set "PAUSE_AT_END=0" & shift & goto args)
set "QT_DIR=%~1"
shift
goto args
:args_done

rem ---- Qt kit folder ----------------------------------------------------------
if "%RECONF%"=="1" (
    if exist "%LOCAL%" del "%LOCAL%"
    if exist "%BUILD%" rmdir /s /q "%BUILD%"
)
if not defined QT_DIR if exist "%LOCAL%" (
    for /f "usebackq tokens=1,* delims==" %%A in ("%LOCAL%") do (
        if /i "%%A"=="QT_DIR" set "QT_DIR=%%B"
    )
)
if not defined QT_DIR if defined QTDIR set "QT_DIR=%QTDIR%"

set /a ATTEMPTS=0
:validate
if not defined QT_DIR goto ask
set "QT_DIR=!QT_DIR:"=!"
if "!QT_DIR:~-1!"=="\" set "QT_DIR=!QT_DIR:~0,-1!"
if exist "!QT_DIR!\lib\cmake\Qt6\Qt6Config.cmake" goto qt_ok
echo [x] "!QT_DIR!" is not a Qt kit folder (no lib\cmake\Qt6\Qt6Config.cmake inside).
set "QT_DIR="

:ask
set /a ATTEMPTS+=1
if !ATTEMPTS! gtr 3 (
    echo [x] No valid Qt kit folder, giving up.
    goto fail
)
echo.
echo Where is Qt? Give the kit folder: it has bin\ and lib\cmake\Qt6 inside,
echo for example C:\Qt\6.8.3\mingw_64 ^(Qt 6.8+, MinGW 64-bit kit^).
set "QT_DIR="
set /p "QT_DIR=Qt kit folder: "
goto validate

:qt_ok
> "%LOCAL%" echo QT_DIR=!QT_DIR!
for %%I in ("!QT_DIR!\..\..") do set "QT_ROOT=%%~fI"
echo Qt kit : !QT_DIR!

rem ---- Toolchain --------------------------------------------------------------
rem MinGW must match the Qt kit, so the one shipped next to it wins over PATH
set "MINGW_BIN="
for /d %%D in ("!QT_ROOT!\Tools\mingw*_64") do if exist "%%~fD\bin\g++.exe" set "MINGW_BIN=%%~fD\bin"
if not defined MINGW_BIN for /f "delims=" %%G in ('where g++ 2^>nul') do if not defined MINGW_BIN set "MINGW_BIN=%%~dpG"
if not defined MINGW_BIN (
    echo [x] MinGW g++ not found. Install "MinGW 64-bit" from the Qt Maintenance Tool ^(Developer and Designer Tools^).
    goto fail
)
echo MinGW  : !MINGW_BIN!

set "NINJA_DIR="
if exist "!QT_ROOT!\Tools\Ninja\ninja.exe" set "NINJA_DIR=!QT_ROOT!\Tools\Ninja"
if not defined NINJA_DIR for /f "delims=" %%G in ('where ninja 2^>nul') do if not defined NINJA_DIR set "NINJA_DIR=%%~dpG"
if defined NINJA_DIR (set "GENERATOR=Ninja") else (set "GENERATOR=MinGW Makefiles")
echo Build  : !GENERATOR!

set "CMAKE_DIR="
for /f "delims=" %%G in ('where cmake 2^>nul') do if not defined CMAKE_DIR set "CMAKE_DIR=%%~dpG"
if not defined CMAKE_DIR if exist "!QT_ROOT!\Tools\CMake_64\bin\cmake.exe" set "CMAKE_DIR=!QT_ROOT!\Tools\CMake_64\bin"
if not defined CMAKE_DIR (
    echo [x] CMake not found. Install CMake 3.21+ or add it from the Qt Maintenance Tool.
    goto fail
)
echo CMake  : !CMAKE_DIR!

set "PATH=!MINGW_BIN!;!QT_DIR!\bin;!NINJA_DIR!;!CMAKE_DIR!;!PATH!"
set "QT_DIR_CMAKE=!QT_DIR:\=/!"

rem ---- Build + deploy ---------------------------------------------------------
echo.
echo ==^> Configure (Release)
cmake -S "%ROOT%" -B "%BUILD%" -G "!GENERATOR!" -DCMAKE_BUILD_TYPE=Release "-DAVM_QT_DIR=!QT_DIR_CMAKE!"
if errorlevel 1 goto fail

echo.
echo ==^> Build
cmake --build "%BUILD%" --parallel
if errorlevel 1 goto fail

echo.
echo ==^> Deploy to %DIST%
if exist "%DIST%" rmdir /s /q "%DIST%"
cmake --install "%BUILD%" --prefix "%DIST%"
if errorlevel 1 goto fail

if "%ZIP%"=="1" (
    echo.
    echo ==^> Zip
    powershell -NoProfile -Command "Compress-Archive -Path '%DIST%\*' -DestinationPath '%ROOT%\AdvancedVolumeMixer.zip' -Force"
    if errorlevel 1 goto fail
    echo Archive: %ROOT%\AdvancedVolumeMixer.zip
)

echo.
echo Done: %DIST%\AdvancedVolumeMixer.exe
if "%PAUSE_AT_END%"=="1" pause
exit /b 0

:fail
echo.
echo Build failed.
if "%PAUSE_AT_END%"=="1" pause
exit /b 1
