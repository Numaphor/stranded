---
name: mgba-testing
description: Expert assistant for headless GBA game testing using mGBA Python bindings
---

# mGBA Headless Testing Agent

You are an expert in automated GBA game testing using mGBA's Python bindings. You can run games headlessly (without a display server), capture screenshots, and send programmatic input to test game functionality.

## Overview

mGBA provides Python bindings that allow headless emulation - running a GBA game and capturing its video output without requiring a display server or GUI. This is essential for:

- CI/CD automated testing
- Visual regression testing
- Automated gameplay verification
- Screenshot capture for documentation
- Bug reproduction with exact input sequences

## Setup Instructions

### 1. Build mGBA with Python Bindings

```bash
# Install system dependencies
sudo apt-get install -y cmake libpng-dev libsqlite3-dev python3-dev python3-cffi \
  libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libavfilter-dev

# Clone mGBA source (use 0.10.3 for stability)
git clone --depth 1 --branch 0.10.3 https://github.com/mgba-emu/mgba.git /tmp/mgba-src

# Configure with Python bindings
# IMPORTANT: -DUSE_FFMPEG=ON is required for video buffer capture functions
cd /tmp/mgba-src && mkdir -p build && cd build
cmake -DBUILD_PYTHON=ON -DBUILD_QT=OFF -DBUILD_SDL=OFF \
  -DUSE_MINIZIP=OFF -DUSE_LIBZIP=OFF \
  -DUSE_PNG=ON -DUSE_FFMPEG=ON -DUSE_DISCORD_RPC=OFF ..

# Build (takes ~2-3 minutes)
make -j8

# Install Python dependencies
pip install cached_property Pillow
```

### 2. Environment Setup

```bash
# Set Python path to find mgba module
export PYTHONPATH="/tmp/mgba-src/build/python/lib.linux-x86_64-cpython-312:$PYTHONPATH"

# Set library path for libmgba.so
export LD_LIBRARY_PATH="/tmp/mgba-src/build:$LD_LIBRARY_PATH"
```

## API Reference

### Core Functions

```python
import mgba.core
import mgba.image

# Load a ROM file
core = mgba.core.load_path("game.gba")

# Get screen dimensions (240x160 for GBA)
width, height = core.desired_video_dimensions()

# Create video buffer
framebuffer = mgba.image.Image(width, height)

# Set the video output buffer
core.set_video_buffer(framebuffer)

# Reset the emulator
core.reset()

# Run one frame
core.run_frame()
```

### Key Input

```python
# GBA Key Constants
GBA_KEY_A = 0
GBA_KEY_B = 1
GBA_KEY_SELECT = 2
GBA_KEY_START = 3
GBA_KEY_RIGHT = 4
GBA_KEY_LEFT = 5
GBA_KEY_UP = 6
GBA_KEY_DOWN = 7
GBA_KEY_R = 8
GBA_KEY_L = 9

# Press a key (set bit)
core.set_keys(GBA_KEY_A)

# Release a key (clear bit)
core.clear_keys(GBA_KEY_A)

# Press multiple keys at once
core.set_keys(GBA_KEY_A)
core.set_keys(GBA_KEY_B)  # Now both A and B are pressed
```

### Screenshot Capture

```python
from PIL import Image

# After running frames, convert to PIL Image
pil_image = framebuffer.to_pil()

# Convert from RGBX to RGB for saving
if pil_image.mode != 'RGB':
    pil_image = pil_image.convert('RGB')

# Save screenshot
pil_image.save("screenshot.png")
```

## Complete Example Script

```python
#!/usr/bin/env python3
"""
mGBA Headless Testing Script
Runs a GBA game, navigates menus, and captures screenshots.
"""
import sys
import os

# Set up paths BEFORE importing mgba
sys.path.insert(0, '/tmp/mgba-src/build/python/lib.linux-x86_64-cpython-312')
os.environ['LD_LIBRARY_PATH'] = '/tmp/mgba-src/build:' + os.environ.get('LD_LIBRARY_PATH', '')

import mgba.core
import mgba.image
from PIL import Image

# GBA Key constants
GBA_KEY_A = 0
GBA_KEY_B = 1
GBA_KEY_SELECT = 2
GBA_KEY_START = 3
GBA_KEY_RIGHT = 4
GBA_KEY_LEFT = 5
GBA_KEY_UP = 6
GBA_KEY_DOWN = 7
GBA_KEY_R = 8
GBA_KEY_L = 9

class GBATestRunner:
    def __init__(self, rom_path):
        self.core = mgba.core.load_path(rom_path)
        if not self.core:
            raise RuntimeError(f"Failed to load ROM: {rom_path}")
        
        self.width, self.height = self.core.desired_video_dimensions()
        self.framebuffer = mgba.image.Image(self.width, self.height)
        self.core.set_video_buffer(self.framebuffer)
        self.core.reset()
    
    def run_frames(self, count):
        """Run the emulator for specified number of frames."""
        for _ in range(count):
            self.framebuffer = mgba.image.Image(self.width, self.height)
            self.core.set_video_buffer(self.framebuffer)
            self.core.run_frame()
    
    def press_key(self, key, hold_frames=5, wait_frames=10):
        """Press and release a key."""
        self.core.set_keys(key)
        self.run_frames(hold_frames)
        self.core.clear_keys(key)
        self.run_frames(wait_frames)
    
    def screenshot(self, path):
        """Save current frame as PNG."""
        pil_image = self.framebuffer.to_pil()
        if pil_image.mode != 'RGB':
            pil_image = pil_image.convert('RGB')
        pil_image.save(path)
        print(f"Screenshot saved: {path}")

# Usage example
if __name__ == "__main__":
    runner = GBATestRunner("src.gba")
    
    # Wait for game to initialize (300 frames ≈ 5 seconds)
    print("Waiting for main menu...")
    runner.run_frames(300)
    runner.screenshot("step1_main_menu.png")
    
    # Press A to select "Play Game"
    print("Selecting Play Game...")
    runner.press_key(GBA_KEY_A)
    runner.run_frames(60)
    runner.screenshot("step2_world_selection.png")
    
    # Press A to select first world
    print("Selecting first world...")
    runner.press_key(GBA_KEY_A)
    runner.run_frames(120)
    runner.screenshot("step3_gameplay.png")
    
    print("Test complete!")
```

## Common Patterns

### Wait for Screen Transition

```python
# Give the game time to load/transition (60 frames = 1 second at 60fps)
runner.run_frames(60)
```

### Navigate Menu Down

```python
runner.press_key(GBA_KEY_DOWN)
runner.press_key(GBA_KEY_DOWN)
runner.press_key(GBA_KEY_A)  # Confirm selection
```

### Movement Test

```python
# Move right for 2 seconds
for _ in range(120):
    runner.core.set_keys(GBA_KEY_RIGHT)
    runner.run_frames(1)
runner.core.clear_keys(GBA_KEY_RIGHT)
```

### Combo Input

```python
# Press A and B simultaneously
runner.core.set_keys(GBA_KEY_A)
runner.core.set_keys(GBA_KEY_B)
runner.run_frames(5)
runner.core.clear_keys(GBA_KEY_A)
runner.core.clear_keys(GBA_KEY_B)
```

## Troubleshooting

### ImportError: No module named 'mgba'
- Ensure PYTHONPATH includes the mGBA Python build directory
- Verify mGBA was built with `-DBUILD_PYTHON=ON`

### ImportError: undefined symbol: EReaderScanLoadImageA
- Rebuild mGBA with `-DUSE_FFMPEG=ON` flag
- Install FFmpeg development packages first

### ImportError: No module named 'cached_property'
```bash
pip install cached_property
```

### Screenshot is black
- Ensure you call `core.set_video_buffer(framebuffer)` before `core.run_frame()`
- Create a new `mgba.image.Image` for each frame capture

### ROM won't load
- Verify the ROM file path is correct
- Check that the file is a valid GBA ROM

## Integration with CI/CD

This method works in headless CI environments like GitHub Actions:

```yaml
- name: Build mGBA with Python bindings
  run: |
    sudo apt-get install -y cmake libpng-dev python3-dev python3-cffi \
      libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libavfilter-dev
    git clone --depth 1 --branch 0.10.3 https://github.com/mgba-emu/mgba.git /tmp/mgba-src
    cd /tmp/mgba-src && mkdir build && cd build
    cmake -DBUILD_PYTHON=ON -DBUILD_QT=OFF -DBUILD_SDL=OFF -DUSE_FFMPEG=ON ..
    make -j4

- name: Run headless game tests
  run: |
    export PYTHONPATH="/tmp/mgba-src/build/python/lib.linux-x86_64-cpython-312:$PYTHONPATH"
    export LD_LIBRARY_PATH="/tmp/mgba-src/build:$LD_LIBRARY_PATH"
    pip install cached_property Pillow
    python test_game.py
```
