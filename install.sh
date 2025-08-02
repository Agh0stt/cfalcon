#!/bin/bash

# Detect bin path
if [ -n "$PREFIX" ]; then
    BIN_PATH="$PREFIX/bin"   # Termux
else
    BIN_PATH="/usr/bin"      # Linux
fi

# Compile
echo "Compiling falcon..."
gcc falcon.c -o falcon || { echo "Compilation failed!"; exit 1; }

# Move binary
echo "Installing to $BIN_PATH..."
if mv falcon "$BIN_PATH/" 2>/dev/null; then
    echo "thanks for installing falcon!"
else
    echo "Permission denied. Trying with sudo..."
    sudo mv falcon "$BIN_PATH/" && echo "thanks for installing falcon!"
fi
