#!/bin/bash

SRC_FILE="falcon.c"

# Detect install path
if [ -n "$PREFIX" ]; then
    INSTALL_PATH="$PREFIX/bin"   # Termux
else
    INSTALL_PATH="/usr/bin"      # Linux
fi

# Function to install compiler silently
install_compiler() {
    if [ -n "$PREFIX" ]; then
        pkg install -y clang
    elif command -v apt >/dev/null 2>&1; then
        sudo apt update -y && sudo apt install -y gcc || sudo apt install -y clang
    elif command -v pacman >/dev/null 2>&1; then
        sudo pacman -Sy --noconfirm gcc || sudo pacman -Sy --noconfirm clang
    elif command -v dnf >/dev/null 2>&1; then
        sudo dnf install -y gcc || sudo dnf install -y clang
    else
        echo "No compiler found. Please install gcc or clang manually."
        exit 1
    fi
}

# Check for compiler
if command -v gcc >/dev/null 2>&1; then
    COMPILER="gcc"
elif command -v clang >/dev/null 2>&1; then
    COMPILER="clang"
else
    install_compiler
    if command -v gcc >/dev/null 2>&1; then
        COMPILER="gcc"
    elif command -v clang >/dev/null 2>&1; then
        COMPILER="clang"
    else
        echo "Compiler installation failed."
        exit 1
    fi
fi

# Compile
$COMPILER "$SRC_FILE" -o falcon || { echo "Compilation failed."; exit 1; }

# Install
echo "Installing Falcon."
if mv falcon "$INSTALL_PATH/" 2>/dev/null; then
    echo "Thanks for installing Falcon."
else
    sudo mv falcon "$INSTALL_PATH/" && echo "Thanks for installing Falcon."
fi
