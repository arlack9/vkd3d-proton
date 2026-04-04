@echo off
REM ============================================================================
REM VKD3D-Proton Complete Build Script
REM ============================================================================
REM This script:
REM   1. Cleans old build directories
REM   2. Configures Meson with WSL WIDL wrapper support
REM   3. Compiles the project
REM ============================================================================

setlocal enabledelayedexpansion

echo.
echo ============================================================================
echo VKD3D-Proton Complete Build
echo ============================================================================
echo.

REM Check if we're in the right directory
if not exist "meson.build" (
    echo ERROR: meson.build not found. Please run this script from the vkd3d-proton root directory.
    exit /b 1
)

echo [Step 1] Checking Prerequisites...
echo.

REM Check Python
python --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python not found. Please ensure Python is installed and in PATH.
    exit /b 1
)
echo   ✓ Python found

REM Check Meson
meson --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Meson not found. Please install with: pip install meson
    exit /b 1
)
echo   ✓ Meson found

REM Check Ninja
ninja --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Ninja not found. Please install with: pip install ninja
    exit /b 1
)
echo   ✓ Ninja found

echo.

echo [Step 2] Cleaning Old Build Directories...
echo.

REM Remove old build directories
for %%D in (builddir build build.64 build-win32 build-win64) do (
    if exist "%%D" (
        echo Removing %%D...
        rmdir /s /q "%%D" >nul 2>&1
        if !errorlevel! equ 0 (
            echo   ✓ Removed %%D
        ) else (
            echo   ⚠ Failed to remove %%D (may be in use)
        )
    )
)

REM Clean Meson cache files
if exist "meson-private" (
    echo Removing meson-private cache...
    rmdir /s /q "meson-private" >nul 2>&1
    if !errorlevel! equ 0 (
        echo   ✓ Removed meson-private
    )
)

REM Clean CMake cache (if it exists)
if exist "CMakeCache.txt" (
    echo Removing CMakeCache.txt...
    del /q "CMakeCache.txt" >nul 2>&1
    if !errorlevel! equ 0 (
        echo   ✓ Removed CMakeCache.txt
    )
)

REM Clean CMake directory
if exist "CMakeFiles" (
    echo Removing CMakeFiles...
    rmdir /s /q "CMakeFiles" >nul 2>&1
    if !errorlevel! equ 0 (
        echo   ✓ Removed CMakeFiles
    )
)

echo.

REM Check if WSL is available for WIDL wrapper
echo [Step 3] Checking WSL WIDL Wrapper Setup...
echo.

set WIDL_FROM_WSL=false

wsl --version >nul 2>&1
if not errorlevel 1 (
    echo   ✓ WSL detected
    
    REM Check if WIDL is available in WSL
    wsl which widl >nul 2>&1
    if not errorlevel 1 (
        echo   ✓ WIDL found in WSL
        set WIDL_FROM_WSL=true
    ) else (
        echo WARNING: WIDL not found in WSL
    )
) else (
    echo WARNING: WSL not detected
)
echo.

echo [Step 4] Configuring Meson...
echo.

mkdir builddir >nul 2>&1

REM Use WSL WIDL wrapper to avoid MIDL path issues
echo Configuring with WSL WIDL wrapper and clang compiler...
meson setup builddir --native-file clang-native.ini -Dwidl_from_wsl=true

if errorlevel 1 (
    echo ERROR: Meson configuration failed!
    exit /b 1
)

echo.
echo [Step 5] Compiling...
echo.

meson compile -C builddir

if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    exit /b 1
)

echo.
echo ============================================================================
echo ✓ Build Complete!
echo ============================================================================
echo.
echo To install:
echo   meson install -C builddir
echo.

endlocal
exit /b 0
