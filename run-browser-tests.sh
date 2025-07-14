#!/bin/bash

# Stranded GBA Browser Testing Script
# This script sets up and runs browser testing for the Stranded GBA game

set -e

echo "🎮 Stranded GBA Browser Testing Setup"
echo "======================================"

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

# Function to setup GBAjs3 emulator
setup_gbajs3() {
    print_status "Setting up GBAjs3 emulator..."
    
    if [ ! -d "/tmp/gbajs3" ]; then
        print_status "Cloning GBAjs3 repository..."
        cd /tmp
        git clone --recursive https://github.com/thenick775/gbajs3.git
        cd gbajs3
        
        if command -v npm &> /dev/null; then
            print_status "Installing GBAjs3 dependencies..."
            
            # Verify we're in the right directory
            if [ ! -f "package.json" ]; then
                print_error "package.json not found in $(pwd)"
                print_error "Directory contents:"
                ls -la
                return 1
            fi
            
            npm install
            print_success "GBAjs3 setup complete!"
        else
            print_error "Node.js/npm not found. Please install Node.js first."
            return 1
        fi
        cd - > /dev/null
    else
        print_success "GBAjs3 already installed"
    fi
}

# Check if we're in the right directory
if [ ! -f "Makefile" ]; then
    print_error "Makefile not found. Please run this script from the stranded project root."
    exit 1
fi

print_status "Setting up browser testing environment..."

# Check for required tools
print_status "Checking for required tools..."

if ! command -v arm-none-eabi-gcc &> /dev/null; then
    print_warning "ARM GCC toolchain not found. Installing..."
    sudo apt-get update
    sudo apt-get install -y gcc-arm-none-eabi
fi

if ! command -v node &> /dev/null; then
    print_warning "Node.js not found. Please install Node.js to run the web emulator."
fi

# Check if butano submodule is initialized
if [ ! -d "butano" ]; then
    print_status "Initializing butano submodule..."
    git submodule update --init --recursive
fi

# Try to build the ROM
print_status "Attempting to build ROM..."
if [ -n "$DEVKITARM" ]; then
    print_status "DEVKITARM found: $DEVKITARM"
    if make clean && make; then
        print_success "ROM built successfully!"
        ROM_FILE=$(find . -name "*.gba" | head -1)
        if [ -n "$ROM_FILE" ]; then
            print_success "ROM file: $ROM_FILE"
        fi
    else
        print_warning "ROM build failed. You can still test with an existing ROM file."
    fi
else
    print_warning "DEVKITARM not set. ROM build may fail."
    print_warning "To build the ROM, you need to install DevkitPro ARM toolchain."
    print_warning "Visit: https://devkitpro.org/wiki/Getting_Started"
fi

# Start browser testing options
echo ""
print_status "Browser Testing Options:"
echo "========================="

echo "1. 📄 Simple HTML Test Page"
echo "   - Open browser-test.html in your browser"
echo "   - Load ROM file manually"
echo "   - Basic emulation interface"

echo ""
echo "2. 🚀 Advanced GBAjs3 Emulator"
if [ -d "/tmp/gbajs3/gbajs3/gbajs3" ]; then
    print_success "GBAjs3 emulator is available"
    echo "   - Full-featured web-based GBA emulator"
    echo "   - Advanced debugging capabilities"
    echo "   - Save states and more features"
    
    # Check if server is running
    if pgrep -f "npm run dev" > /dev/null; then
        print_success "GBAjs3 server is already running"
        echo "   - Access at: http://localhost:3000"
    else
        print_status "Starting GBAjs3 server..."
        if [ -f "./start-gbajs3-server.sh" ]; then
            ./start-gbajs3-server.sh > /dev/null 2>&1
            if curl -s -o /dev/null -w "%{http_code}" http://localhost:3001 | grep -q "200"; then
                print_success "GBAjs3 server started on http://localhost:3001"
            else
                print_error "Failed to start GBAjs3 server"
                print_warning "Try running: ./start-gbajs3-server.sh"
            fi
        else
            print_warning "start-gbajs3-server.sh not found"
            print_status "Attempting direct startup..."
            if [ -d "/tmp/gbajs3/gbajs3/gbajs3" ]; then
                cd /tmp/gbajs3/gbajs3/gbajs3
                nohup npm run dev -- --host 0.0.0.0 --port 3001 > /tmp/gbajs3-server.log 2>&1 &
                sleep 3
                if curl -s -o /dev/null -w "%{http_code}" http://localhost:3001 | grep -q "200"; then
                    print_success "GBAjs3 server started on http://localhost:3001"
                else
                    print_error "Failed to start GBAjs3 server"
                fi
                cd - > /dev/null
            else
                print_error "GBAjs3 directory not found at /tmp/gbajs3/gbajs3/gbajs3"
                print_warning "Please run the GBAjs3 setup first"
            fi
        fi
    fi
else
    print_warning "GBAjs3 emulator not found"
    echo "   - Setting up GBAjs3 automatically..."
    if setup_gbajs3; then
        print_status "Starting GBAjs3 server after setup..."
        if [ -f "./start-gbajs3-server.sh" ]; then
            ./start-gbajs3-server.sh > /dev/null 2>&1
            if curl -s -o /dev/null -w "%{http_code}" http://localhost:3001 | grep -q "200"; then
                print_success "GBAjs3 server started on http://localhost:3001"
            else
                print_error "Failed to start GBAjs3 server after setup"
            fi
        else
            cd /tmp/gbajs3/gbajs3/gbajs3
            nohup npm run dev -- --host 0.0.0.0 --port 3001 > /tmp/gbajs3-server.log 2>&1 &
            sleep 3
            if curl -s -o /dev/null -w "%{http_code}" http://localhost:3001 | grep -q "200"; then
                print_success "GBAjs3 server started on http://localhost:3001"
            else
                print_error "Failed to start GBAjs3 server after setup"
            fi
            cd - > /dev/null
        fi
    else
        print_error "Failed to set up GBAjs3"
    fi
fi

echo ""
echo "3. 🧪 Automated Testing (Future)"
echo "   - Unit tests for game logic"
echo "   - Integration tests"
echo "   - Performance benchmarks"

echo ""
print_status "Testing Instructions:"
echo "===================="
echo "1. Build ROM: make clean && make"
echo "2. Open browser-test.html OR visit http://localhost:3001"
echo "3. Load the generated .gba file"
echo "4. Test game functionality"
echo "5. Report any issues found"

echo ""
print_status "Keyboard Controls:"
echo "=================="
echo "Arrow Keys: D-Pad"
echo "Z: A Button"
echo "X: B Button" 
echo "A: L Button"
echo "S: R Button"
echo "Enter: Start"
echo "Shift: Select"

echo ""
print_success "Browser testing setup complete!"
print_status "Happy testing! 🎮"
