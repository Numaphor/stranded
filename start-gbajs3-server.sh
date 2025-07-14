#!/bin/bash

# GBAjs3 Server Startup Script
# This script ensures the GBAjs3 server starts correctly from the right directory

set -e

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

# Kill any existing processes
print_status "Stopping any existing GBAjs3 processes..."
pkill -f "npm run dev" 2>/dev/null || true
pkill -f "vite" 2>/dev/null || true

# Verify directory structure
GBAJS3_DIR="/tmp/gbajs3/gbajs3/gbajs3"
if [ ! -d "$GBAJS3_DIR" ]; then
    print_error "GBAjs3 directory not found at $GBAJS3_DIR"
    print_error "Please run ./setup-gbajs3.sh first"
    exit 1
fi

# Change to the correct directory
cd "$GBAJS3_DIR"
print_status "Working directory: $(pwd)"

# Verify package.json exists
if [ ! -f "package.json" ]; then
    print_error "package.json not found in $(pwd)"
    print_error "Directory contents:"
    ls -la
    exit 1
fi

# Verify node_modules exists
if [ ! -d "node_modules" ]; then
    print_warning "node_modules not found. Installing dependencies..."
    npm install
fi

# Start the server
print_status "Starting GBAjs3 server on port 3001..."
nohup npm run dev -- --host 0.0.0.0 --port 3001 > /tmp/gbajs3-server.log 2>&1 &
SERVER_PID=$!

# Wait for server to start
sleep 3

# Check if server is running
if ps -p $SERVER_PID > /dev/null 2>&1; then
    # Test if server is responding
    if curl -s -o /dev/null -w "%{http_code}" http://localhost:3001 | grep -q "200"; then
        print_success "GBAjs3 server started successfully!"
        print_status "🌐 Access at: http://localhost:3001"
        print_status "📋 Server PID: $SERVER_PID"
        print_status "📄 Logs: /tmp/gbajs3-server.log"
        
        # Show recent log output
        echo ""
        print_status "Recent server output:"
        tail -n 5 /tmp/gbajs3-server.log
    else
        print_error "Server started but not responding on port 3001"
        print_error "Check logs: /tmp/gbajs3-server.log"
        exit 1
    fi
else
    print_error "Failed to start GBAjs3 server"
    print_error "Check logs: /tmp/gbajs3-server.log"
    cat /tmp/gbajs3-server.log
    exit 1
fi

echo ""
print_success "🎮 GBAjs3 server is ready!"
print_status "To stop: pkill -f 'npm run dev'"
print_status "To view logs: tail -f /tmp/gbajs3-server.log"

