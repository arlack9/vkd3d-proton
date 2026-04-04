@echo off
REM ============================================================================
REM WIDL Wrapper for WSL - Calls Wine IDL compiler from WSL on Windows
REM ============================================================================
REM Usage: widl-wrapper.bat [widl arguments...]
REM This wrapper converts Windows paths to WSL format and calls wsl widl
REM ============================================================================

setlocal enabledelayedexpansion

REM Enable error handling
if errorlevel 1 (
    echo Error: Failed to enable delayed expansion
    exit /b 1
)

REM Store original arguments
set "ORIGINAL_ARGS=%*"

REM Initialize converted arguments
set "WSL_ARGS="

REM Process each argument
for %%A in (%*) do (
    set "ARG=%%A"
    
    REM Check if argument is a MIDL-style flag that needs conversion
    if "!ARG!"=="/h" (
        set "WSL_ARGS=!WSL_ARGS! -h"
    ) else if "!ARG!"=="/o" (
        set "WSL_ARGS=!WSL_ARGS! -o"
    ) else if "!ARG!"=="/out" (
        set "WSL_ARGS=!WSL_ARGS! -out"
    ) else (
        REM This argument might be a path - convert if it looks like one
        if "!ARG:~1,1!"==":" (
            REM Looks like a Windows path with drive letter
            call :convert_path "!ARG!" CONVERTED_PATH
            set "WSL_ARGS=!WSL_ARGS! !CONVERTED_PATH!"
        ) else if "!ARG:~0,2!"=="\\" (
            REM UNC path
            call :convert_path "!ARG!" CONVERTED_PATH
            set "WSL_ARGS=!WSL_ARGS! !CONVERTED_PATH!"
        ) else if "!ARG!"=="" (
            REM Skip empty arguments
        ) else (
            REM Assume it's a regular argument
            set "WSL_ARGS=!WSL_ARGS! !ARG!"
        )
    )
)

REM Debug output (uncomment for troubleshooting)
REM echo Original args: %ORIGINAL_ARGS% 1>&2
REM echo Converted args: %WSL_ARGS% 1>&2

REM Call WSL WIDL with converted arguments
wsl widl %WSL_ARGS%
set "EXIT_CODE=%ERRORLEVEL%"

REM Clean up and exit with same code as wsl command
endlocal & exit /b %EXIT_CODE%

REM ============================================================================
REM Helper function: convert_path - Converts Windows path to WSL format
REM Input: %~1 - Windows path
REM Output: %2 - WSL path variable name
REM ============================================================================
:convert_path
setlocal enabledelayedexpansion

set "WIN_PATH=%~1"
set "WSL_PATH=!WIN_PATH!"

REM Replace backslashes with forward slashes
set "WSL_PATH=!WSL_PATH:\=/!"

REM Handle drive letters: C:/ -> /mnt/c/
if "!WSL_PATH:~1,2!"==":/" (
    set "DRIVE_LETTER=!WSL_PATH:~0,1!"
    
    REM Convert to lowercase
    for %%D in (A B C D E F G H I J K L M N O P Q R S T U V W X Y Z) do (
        if "!DRIVE_LETTER!"=="%%D" set "DRIVE_LETTER=%%D"
    )
    
    REM More reliable lowercase conversion
    if "!DRIVE_LETTER!"=="A" set "DRIVE_LOWER=a"
    if "!DRIVE_LETTER!"=="B" set "DRIVE_LOWER=b"
    if "!DRIVE_LETTER!"=="C" set "DRIVE_LOWER=c"
    if "!DRIVE_LETTER!"=="D" set "DRIVE_LOWER=d"
    if "!DRIVE_LETTER!"=="E" set "DRIVE_LOWER=e"
    if "!DRIVE_LETTER!"=="F" set "DRIVE_LOWER=f"
    if "!DRIVE_LETTER!"=="G" set "DRIVE_LOWER=g"
    if "!DRIVE_LETTER!"=="H" set "DRIVE_LOWER=h"
    if "!DRIVE_LETTER!"=="I" set "DRIVE_LOWER=i"
    if "!DRIVE_LETTER!"=="J" set "DRIVE_LOWER=j"
    if "!DRIVE_LETTER!"=="K" set "DRIVE_LOWER=k"
    if "!DRIVE_LETTER!"=="L" set "DRIVE_LOWER=l"
    if "!DRIVE_LETTER!"=="M" set "DRIVE_LOWER=m"
    if "!DRIVE_LETTER!"=="N" set "DRIVE_LOWER=n"
    if "!DRIVE_LETTER!"=="O" set "DRIVE_LOWER=o"
    if "!DRIVE_LETTER!"=="P" set "DRIVE_LOWER=p"
    if "!DRIVE_LETTER!"=="Q" set "DRIVE_LOWER=q"
    if "!DRIVE_LETTER!"=="R" set "DRIVE_LOWER=r"
    if "!DRIVE_LETTER!"=="S" set "DRIVE_LOWER=s"
    if "!DRIVE_LETTER!"=="T" set "DRIVE_LOWER=t"
    if "!DRIVE_LETTER!"=="U" set "DRIVE_LOWER=u"
    if "!DRIVE_LETTER!"=="V" set "DRIVE_LOWER=v"
    if "!DRIVE_LETTER!"=="W" set "DRIVE_LOWER=w"
    if "!DRIVE_LETTER!"=="X" set "DRIVE_LOWER=x"
    if "!DRIVE_LETTER!"=="Y" set "DRIVE_LOWER=y"
    if "!DRIVE_LETTER!"=="Z" set "DRIVE_LOWER=z"
    
    set "WSL_PATH=/mnt/!DRIVE_LOWER!!WSL_PATH:~2!"
)

REM Return the converted path
endlocal & set "%~2=!WSL_PATH!"
goto :eof
