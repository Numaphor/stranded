# 🎮 Stranded GBA Browser Testing

This document describes how to run browser testing for the Stranded GBA game.

## Quick Start

```bash
# Run the browser testing setup (automatically installs GBAjs3 if needed)
./run-browser-tests.sh

# Or set up GBAjs3 emulator separately
./setup-gbajs3.sh

# Or start GBAjs3 server directly (robust startup)
./start-gbajs3-server.sh
```

## Testing Options

### 1. 📄 Simple HTML Test Page

A lightweight HTML page with basic GBA emulation interface:

- **File**: `browser-test.html`
- **Usage**: Open in any modern web browser
- **Features**:
  - ROM file loading
  - Basic playback controls
  - Keyboard input mapping
  - ROM header information display

**Steps**:
1. Build the ROM: `make clean && make`
2. Open `browser-test.html` in your browser
3. Load the generated `.gba` file
4. Use controls to test the game

### 2. 🚀 Advanced GBAjs3 Emulator

Full-featured web-based GBA emulator with advanced capabilities:

- **URL**: http://localhost:3001 (when server is running)
- **Features**:
  - Complete GBA emulation
  - Save states
  - Debugging tools
  - Audio support
  - Full-screen mode
  - Touch controls for mobile

**Steps**:
1. The setup script automatically starts the GBAjs3 server
2. Visit http://localhost:3001 in your browser
3. Load your ROM file
4. Enjoy full GBA emulation features

## Building the ROM

### Prerequisites

- ARM GCC toolchain (`gcc-arm-none-eabi`)
- DevkitPro ARM (recommended) or compatible toolchain
- Make build system

### Build Commands

```bash
# Clean previous builds
make clean

# Build the ROM
make

# The output will be stranded.gba
```

### Environment Setup

For proper building, you may need to set up DevkitPro:

```bash
# Install DevkitPro (recommended method)
# Visit: https://devkitpro.org/wiki/Getting_Started

# Or set DEVKITARM environment variable
export DEVKITARM=/path/to/devkitarm
```

## Controls

### Keyboard Mapping

| Key | GBA Button |
|-----|------------|
| Arrow Keys | D-Pad |
| Z | A Button |
| X | B Button |
| A | L Button |
| S | R Button |
| Enter | Start |
| Shift | Select |

### Touch Controls (Mobile)

The GBAjs3 emulator provides on-screen touch controls for mobile devices.

## Testing Checklist

When testing the game, verify:

- [ ] Game loads without errors
- [ ] All controls respond correctly
- [ ] Graphics render properly
- [ ] Audio plays (if implemented)
- [ ] Game logic functions as expected
- [ ] Performance is acceptable
- [ ] No memory leaks or crashes

## Troubleshooting

### ROM Build Issues

1. **"DEVKITARM not found"**:
   - Install DevkitPro ARM toolchain
   - Set DEVKITARM environment variable

2. **"arm-none-eabi-gcc not found"**:
   - Install: `sudo apt-get install gcc-arm-none-eabi`

3. **Submodule issues**:
   - Run: `git submodule update --init --recursive`

### Browser Issues

1. **ROM won't load**:
   - Check file format (.gba extension)
   - Verify ROM was built successfully
   - Try a different browser

2. **Controls not working**:
   - Click on the emulator canvas first
   - Check browser console for errors
   - Verify keyboard focus

3. **Performance issues**:
   - Close other browser tabs
   - Try Chrome/Firefox for best performance
   - Check browser developer tools

### GBAjs3 Setup Issues

1. **"No such file or directory" error**:
   - Run `./setup-gbajs3.sh` to install GBAjs3
   - Or let `./run-browser-tests.sh` install it automatically

2. **Server won't start**:
   - Check if Node.js is installed: `node --version`
   - Install Node.js from https://nodejs.org/
   - Kill existing processes: `pkill -f 'npm run dev'`
   - Try the robust startup: `./start-gbajs3-server.sh`

3. **Port already in use**:
   - The script will automatically find an available port
   - Check the output for the actual port number
   - Use the dedicated startup script: `./start-gbajs3-server.sh`

4. **npm ENOENT errors**:
   - Use the robust startup script: `./start-gbajs3-server.sh`
   - This script verifies directory structure before running npm
   - Check server logs: `tail -f /tmp/gbajs3-server.log`

## Development Testing

### Automated Testing (Future)

Plans for automated testing include:

- Unit tests for game logic
- Integration tests for core systems
- Performance benchmarks
- Regression testing
- CI/CD integration

### Debug Features

The GBAjs3 emulator provides:

- Memory viewer
- CPU state inspection
- Breakpoint support
- Step-by-step execution
- Performance profiling

## Contributing

When adding new features or fixing bugs:

1. Test with both emulation methods
2. Verify controls work correctly
3. Check performance impact
4. Update this documentation if needed

## Resources

- [GBAjs3 Emulator](https://github.com/thenick775/gbajs3)
- [Butano Engine](https://github.com/GValiente/butano)
- [DevkitPro](https://devkitpro.org/)
- [GBA Development Resources](https://www.gbadev.org/)

---

Happy testing! 🎮✨
