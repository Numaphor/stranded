#!/bin/bash
set -e

echo "Setting up Wonderful Toolchain..."

# Define install location
TOOLCHAIN_DIR="/opt/wonderful"
BOOTSTRAP_ARCHIVE="tools/wf-bootstrap-x86_64.tar.gz"
BOOTSTRAP_URL="https://wonderful.asie.pl/bootstrap/wf-bootstrap-x86_64.tar.gz"

# Check for sudo/root
if [ "$EUID" -ne 0 ]; then
    if command -v sudo &> /dev/null; then
        SUDO="sudo"
    else
        echo "Error: This script requires root privileges or sudo to write to /opt."
        exit 1
    fi
else
    SUDO=""
fi

# Create directory
if [ ! -d "$TOOLCHAIN_DIR" ]; then
    echo "Creating $TOOLCHAIN_DIR..."
    $SUDO mkdir -p "$TOOLCHAIN_DIR"
    $SUDO chown $USER "$TOOLCHAIN_DIR"
else
    echo "$TOOLCHAIN_DIR already exists."
fi

# Extract bootstrap
if [ -f "$BOOTSTRAP_ARCHIVE" ]; then
    echo "Extracting bootstrap from local file $BOOTSTRAP_ARCHIVE..."
    tar -xzf "$BOOTSTRAP_ARCHIVE" -C "$TOOLCHAIN_DIR"
else
    echo "Local bootstrap not found at $BOOTSTRAP_ARCHIVE."
    echo "Downloading bootstrap from $BOOTSTRAP_URL..."
    # Create a temporary file
    TEMP_BOOTSTRAP=$(mktemp)
    if command -v wget &> /dev/null; then
        wget -O "$TEMP_BOOTSTRAP" "$BOOTSTRAP_URL"
    elif command -v curl &> /dev/null; then
        curl -L -o "$TEMP_BOOTSTRAP" "$BOOTSTRAP_URL"
    else
        echo "Error: Neither wget nor curl found. Cannot download bootstrap."
        rm -f "$TEMP_BOOTSTRAP"
        exit 1
    fi

    echo "Extracting downloaded bootstrap..."
    tar -xzf "$TEMP_BOOTSTRAP" -C "$TOOLCHAIN_DIR"
    rm -f "$TEMP_BOOTSTRAP"
fi

# Set environment for the rest of the script
export WONDERFUL_TOOLCHAIN="$TOOLCHAIN_DIR"
export PATH="$TOOLCHAIN_DIR/bin:$PATH"

echo "Updating wf-pacman..."
wf-pacman -Syu --noconfirm

echo "Installing wf-tools..."
wf-pacman -S --noconfirm wf-tools

echo "Enabling BlocksDS repository..."
wf-config repo enable blocksds
wf-pacman -Syu --noconfirm

echo "Installing GBA toolchain and BlocksDS..."
wf-pacman -S --noconfirm target-gba blocksds-toolchain

echo "Setup complete!"
echo "Please run: source env.sh"
