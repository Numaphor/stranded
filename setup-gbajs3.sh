#!/bin/bash

# GBAjs3 Emulator Setup Script
# This script sets up the GBAjs3 web-based GBA emulator

set -e

echo "🚀 GBAjs3 Emulator Setup"
echo "========================"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check for Node.js
if ! command -v node &> /dev/null; then
    print_error "Node.js not found. Please install Node.js first."
    print_status "Visit: https://nodejs.org/ to download Node.js"
    exit 1
fi

if ! command -v npm &> /dev/null; then
    print_error "npm not found. Please install npm first."
    exit 1
fi

print_success "Node.js $(node --version) and npm $(npm --version) found"

# Check if GBAjs3 is already installed
if [ -d "/tmp/gbajs3" ]; then
    print_warning "GBAjs3 directory already exists at /tmp/gbajs3"
    read -p "Do you want to reinstall? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        print_status "Removing existing GBAjs3 installation..."
        rm -rf /tmp/gbajs3
    else
        print_status "Using existing installation"
        cd /tmp/gbajs3/gbajs3/gbajs3
        print_status "Starting GBAjs3 server..."
        npm run dev -- --host 0.0.0.0 --port 3000 &
        print_success "GBAjs3 server started on http://localhost:3000"
        exit 0
    fi
fi

# Clone GBAjs3 repository
print_status "Cloning GBAjs3 repository..."
cd /tmp
git clone --recursive https://github.com/thenick775/gbajs3.git

if [ ! -d "/tmp/gbajs3/gbajs3/gbajs3" ]; then
    print_error "Failed to clone GBAjs3 repository"
    exit 1
fi

# Install dependencies
print_status "Installing GBAjs3 dependencies..."
cd /tmp/gbajs3/gbajs3/gbajs3

# Verify we're in the right directory
if [ ! -f "package.json" ]; then
    print_error "package.json not found in $(pwd)"
    print_error "Directory contents:"
    ls -la
    exit 1
fi

npm install

if [ $? -eq 0 ]; then
    print_success "GBAjs3 dependencies installed successfully!"
else
    print_error "Failed to install GBAjs3 dependencies"
    exit 1
fi

# Start the development server
print_status "Starting GBAjs3 development server..."
npm run dev -- --host 0.0.0.0 --port 3000 &
SERVER_PID=$!

# Wait a moment for the server to start
sleep 3

# Check if server is running
if ps -p $SERVER_PID > /dev/null; then
    print_success "GBAjs3 server started successfully!"
    echo ""
    print_status "🌐 Access the emulator at: http://localhost:3000"
    print_status "📱 Network access at: http://$(hostname -I | awk '{print $1}'):3000"
    echo ""
    print_status "To stop the server, run: pkill -f 'npm run dev'"
    print_status "To restart, run this script again or use: ./run-browser-tests.sh"
else
    print_error "Failed to start GBAjs3 server"
    exit 1
fi

echo ""
print_success "🎮 GBAjs3 setup complete! Happy testing!"
