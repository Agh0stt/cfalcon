#!/bin/bash

# Detect bin path
if [ -n "$PREFIX" ]; then
    BIN_PATH="$PREFIX/bin"   # Termux
else
    BIN_PATH="/usr/bin"      # Linux
fi

# Compile
echo "Compiling cfalcon..."
gcc cfalcon.c -o cfalcon || { echo "Compilation failed!"; exit 1; }

# Move binary
echo "Installing to $BIN_PATH..."
if mv cfalcon "$BIN_PATH/" 2>/dev/null; then
    echo "thanks for installing cfalcon!"
else
    echo "Permission denied. Trying with sudo..."
    sudo mv cfalcon "$BIN_PATH/" && echo "thanks for installing cfalcon!"
fi
