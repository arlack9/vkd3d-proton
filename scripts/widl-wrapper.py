#!/usr/bin/env python3
"""
WIDL Wrapper for WSL - Calls Wine IDL compiler from WSL on Windows

This script:
1. Takes WIDL (or MIDL-style) arguments
2. Converts Windows paths to WSL format
3. Calls WSL WIDL with converted arguments
4. Returns the same exit code

Usage:
    widl-wrapper.py [widl arguments...]
    widl-wrapper.py -h -o output.h input.idl
    widl-wrapper.py /h /out output.h input.idl
"""

import subprocess
import sys
import os
import re
from pathlib import Path


def windows_to_wsl(path: str) -> str:
    """
    Convert Windows path to WSL format.
    Examples:
        C:\\Users\\alice\\file.idl -> /mnt/c/Users/alice/file.idl
        C:/Users/alice/file.idl -> /mnt/c/Users/alice/file.idl
    """
    # Remove quotes if present
    path = path.strip('"').strip("'")
    
    # Normalize to forward slashes
    path = path.replace("\\", "/")
    
    # Match drive letter pattern (C:, D:, etc.)
    match = re.match(r'^([a-zA-Z]):(.*)$', path)
    if match:
        drive_letter = match.group(1).lower()
        rest = match.group(2)
        return f"/mnt/{drive_letter}{rest}"
    
    # Handle UNC paths (\\server\share)
    if path.startswith("//"):
        return path  # Keep as-is for now
    
    return path


def wsl_to_windows(path: str) -> str:
    """
    Convert WSL path back to Windows format (if needed).
    Examples:
        /mnt/c/Users/alice/file.idl -> C:\\Users\\alice\\file.idl
    """
    match = re.match(r'^/mnt/([a-z])(.*)$', path)
    if match:
        drive_letter = match.group(1).upper()
        rest = match.group(2).replace("/", "\\")
        return f"{drive_letter}:{rest}"
    return path


def is_path_argument(arg: str) -> bool:
    """
    Heuristic to detect if an argument is a file path.
    Returns True if it looks like a path.
    """
    # Check for drive letter or UNC path
    if re.match(r'^[a-zA-Z]:[/\\]', arg):
        return True
    if arg.startswith("\\\\"):
        return True
    
    # Check for file extensions that are common in IDL workflows
    if arg.endswith(('.idl', '.h', '.hpp', '.c', '.cpp', '.tlb')):
        return True
    
    # Check if file exists (will be false for output files)
    if os.path.exists(arg):
        return True
    
    return False


def convert_midl_to_widl_args(args: list) -> list:
    """
    Convert MIDL-style arguments to WIDL equivalents.
    MIDL: /h output.h /out dir input.idl
    WIDL: -h -o output.h input.idl
    """
    converted = []
    i = 0
    while i < len(args):
        arg = args[i]
        
        if arg == '/h':
            # MIDL: /h output.h
            # WIDL: -h -o output.h
            converted.append('-h')
            if i + 1 < len(args) and not args[i + 1].startswith('/'):
                output_file = args[i + 1]
                converted.append('-o')
                converted.append(windows_to_wsl(output_file))
                i += 2
            else:
                i += 1
        elif arg == '/out':
            # Skip MIDL /out directory (WIDL doesn't have this)
            if i + 1 < len(args) and not args[i + 1].startswith('/'):
                i += 2
            else:
                i += 1
        elif arg.startswith('/'):
            # Other MIDL flags - convert to WIDL equivalents
            # /o -> -o, /I -> -I, etc.
            converted.append(arg.replace('/', '-'))
            i += 1
        else:
            # Regular argument (could be path or value)
            if is_path_argument(arg):
                converted.append(windows_to_wsl(arg))
            else:
                converted.append(arg)
            i += 1
    
    return converted


def main():
    """Main entry point."""
    if len(sys.argv) < 2:
        print("Usage: widl-wrapper.py [widl arguments...]", file=sys.stderr)
        print("Example: widl-wrapper.py -h -o output.h input.idl", file=sys.stderr)
        sys.exit(1)
    
    # Get all arguments except program name
    args = sys.argv[1:]
    
    # Detect if these are MIDL-style or WIDL-style arguments
    is_midl_style = any(arg.startswith('/') for arg in args)
    
    # Convert arguments
    if is_midl_style:
        # Convert MIDL to WIDL format
        converted_args = convert_midl_to_widl_args(args)
    else:
        # Already in WIDL format, just convert paths
        converted_args = [windows_to_wsl(arg) if is_path_argument(arg) else arg 
                         for arg in args]
    
    # Debug output
    if os.environ.get('WIDL_WRAPPER_DEBUG'):
        print(f"Original args: {args}", file=sys.stderr)
        print(f"Converted args: {converted_args}", file=sys.stderr)
    
    # Call WSL WIDL with proper output directory handling
    try:
        # Ensure output directory exists (convert back to Windows path for os.makedirs)
        output_file = None
        for i, arg in enumerate(converted_args):
            if arg == '-o' and i + 1 < len(converted_args):
                output_file = converted_args[i + 1]
                break
        
        if output_file:
            # Convert WSL path back to Windows to create directory
            windows_output = wsl_to_windows(output_file)
            output_dir = os.path.dirname(windows_output)
            if output_dir and not os.path.exists(output_dir):
                os.makedirs(output_dir, exist_ok=True)
                if os.environ.get('WIDL_WRAPPER_DEBUG'):
                    print(f"Created output directory: {output_dir}", file=sys.stderr)
        
        cmd = ['wsl', 'widl'] + converted_args
        if os.environ.get('WIDL_WRAPPER_DEBUG'):
            print(f"Running: {' '.join(cmd)}", file=sys.stderr)
        
        result = subprocess.run(cmd, capture_output=False)
        
        if os.environ.get('WIDL_WRAPPER_DEBUG'):
            print(f"WIDL exit code: {result.returncode}", file=sys.stderr)
        
        sys.exit(result.returncode)
    
    except FileNotFoundError:
        print("Error: 'wsl' command not found. Is WSL installed?", file=sys.stderr)
        print("Install WSL: https://learn.microsoft.com/en-us/windows/wsl/install", 
              file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc(file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
