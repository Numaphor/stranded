# 🎮 Stranded GBA Browser Testing

This document describes how to run browser testing for the Stranded GBA game.

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

### 2. 🚀 Advanced GBAjs3 Emulator

Full-featured web-based GBA emulator with advanced capabilities:

- **URL**: http://localhost:3000 (when server is running)
- **Features**:
  - Complete GBA emulation
  - Save states
  - Debugging tools
  - Audio support
  - Full-screen mode
  - Touch controls for mobile

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
