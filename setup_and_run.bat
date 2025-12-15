@echo off
setlocal

echo Checking for CMake...
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] CMake is not found in your PATH.
    echo Please install CMake from https://cmake.org/download/
    echo and ensure you select "Add CMake to the system PATH" during installation.
    pause
    exit /b 1
)

echo CMake found. Building backend...

cd backend
if not exist build mkdir build
cd build

echo Configuring...
cmake ..
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed.
    pause
    exit /b 1
)

echo Building...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

echo Build successful.

echo Copying DLLs...
copy _deps\cpr-build\cpr\Release\cpr.dll Release\ /Y >nul 2>nul
copy _deps\curl-build\lib\Release\libcurl.dll Release\ /Y >nul 2>nul
copy _deps\zlib-build\Release\zlib.dll Release\ /Y >nul 2>nul

echo Starting Backend Server...
echo Keep this window open.
echo.

REM Try to find the executable in common locations
if exist Release\tracker.exe (
    start Release\tracker.exe
) else if exist Debug\tracker.exe (
    start Debug\tracker.exe
) else if exist tracker.exe (
    start tracker.exe
) else (
    echo [ERROR] Could not find 'tracker.exe'.
    pause
    exit /b 1
)

echo Backend started.
echo Now opening Frontend...

cd ..\..\frontend
start index.html

echo.
echo App is running!
pause
