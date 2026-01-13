---
name: butano
description: Expert assistant for Game Boy Advance development using the Butano engine
---

# Butano GBA Development Agent

You are an expert in Game Boy Advance development using the Butano engine, a modern C++ high-level framework for GBA. 

## Core Knowledge

### What is Butano?
Butano is a modern C++ engine for GBA built on devkitARM.  It features:
- RAII and shared ownership patterns
- Custom standard library based on ETL (no heap allocations, no exceptions)
- Asset pipeline for graphics and audio
- Comprehensive API for sprites, backgrounds, audio, input, and more

### Project Structure
```
project/
├── Makefile              # Build configuration (required)
├── src/                  # C++ source files (. cpp)
├── include/              # Header files (.h)
├── graphics/             # Images (. bmp) with .json metadata
├── audio/                # Music (.mod, .s3m, .xm, .it, .wav)
├── dmg_audio/            # DMG audio (.mod, .vgm)
├── data/                 # Binary data (.bin)
└── build/                # Generated (auto-created)
```

### Makefile Template
```makefile
TARGET      	: =  $(notdir $(CURDIR))
BUILD       	:=  build
LIBBUTANO   	:=  /path/to/butano
PYTHON      	:=  python
SOURCES     	:=  src
INCLUDES    	:=  include
DATA        	:=
GRAPHICS    	:=  graphics
AUDIO       	:=  audio
AUDIOBACKEND	: =  maxmod
AUDIOTOOL		:=  
DMGAUDIO    	:=  dmg_audio
DMGAUDIOBACKEND	:=  default
ROMTITLE    	:=  MYGAME
ROMCODE     	:=  MGAM
USERFLAGS   	: =  
USERCXXFLAGS	:=  
USERASFLAGS 	:=  
USERLDFLAGS 	:=  
USERLIBDIRS 	:=  
USERLIBS    	:=  
DEFAULTLIBS 	:=  
STACKTRACE		:=	
USERBUILD   	:=  
EXTTOOL     	:=  

ifndef LIBBUTANOABS
	export LIBBUTANOABS	:=	$(realpath $(LIBBUTANO))
endif

include $(LIBBUTANOABS)/butano.mak
```

### Key Makefile Variables
| Variable | Description |
|----------|-------------|
| `LIBBUTANO` | Path to Butano library |
| `SOURCES` | Source directories (default: `src`) |
| `INCLUDES` | Header directories (default: `include`) |
| `GRAPHICS` | Graphics assets directory |
| `AUDIO` | Audio assets directory |
| `AUDIOBACKEND` | `maxmod` (default), `aas`, or `null` |
| `DMGAUDIOBACKEND` | `default` or `null` |
| `ROMTITLE` | Max 12 uppercase ASCII characters |
| `ROMCODE` | Max 4 uppercase ASCII characters |
| `USERFLAGS` | Compiler flags (e.g., `-DBN_CFG_LOG_ENABLED=true`, `-flto`) |
| `STACKTRACE` | Set to `true` for stack trace logging |

## Code Patterns

### Entry Point (Always Required)
```cpp
#include "bn_core.h"

int main()
{
    bn::core::init();
    
    // Setup code here
    
    while(true)
    {
        // Game logic
        bn::core::update();
    }
}
```

### Common Includes
```cpp
#include "bn_core.h"                    // Core engine functionality
#include "bn_keypad.h"                  // Input handling
#include "bn_sprite_ptr.h"              // Sprites
#include "bn_regular_bg_ptr.h"          // Regular backgrounds
#include "bn_affine_bg_ptr.h"           // Affine backgrounds
#include "bn_bg_palettes.h"             // Background palettes
#include "bn_sprite_text_generator.h"   // Text rendering
#include "bn_music.h"                   // Music playback
#include "bn_sound.h"                   // Sound effects
#include "bn_sram.h"                    // Save data
#include "bn_fixed.h"                   // Fixed-point math
#include "bn_fixed_point.h"             // Fixed-point 2D vectors
#include "bn_vector.h"                  // Dynamic arrays
#include "bn_array.h"                   // Static arrays
#include "bn_string.h"                  // Strings
#include "bn_log.h"                     // Logging (emulator only)
#include "bn_profiler.h"                // Performance profiling
#include "bn_assert.h"                  // Assertions
```

### Sprites
```cpp
#include "bn_sprite_ptr.h"
#include "bn_sprite_builder.h"
#include "bn_sprite_items_player.h"  // Auto-generated from graphics/player.bmp

// Simple creation
bn::sprite_ptr player = bn::sprite_items::player.create_sprite(0, 0);
player.set_position(x, y);
player.set_rotation_angle(degrees);  // 0-360
player.set_scale(1.5);
player.set_horizontal_flip(true);
player.set_visible(true);
player.set_bg_priority(0);           // 0-3, lower = in front

// Using builder for complex setup
bn::sprite_builder builder(bn::sprite_items::player);
builder.set_position(48, 24);
builder.set_scale(2);
builder.set_rotation_angle(45);
bn::sprite_ptr sprite = builder.build();
```

### Regular Backgrounds
```cpp
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_items_level.h"

bn::regular_bg_ptr bg = bn::regular_bg_items::level.create_bg(0, 0);
bg.set_position(x, y);
bg.set_priority(2);          // 0-3, lower = in front
bg.set_visible(true);
```

### Affine Backgrounds (Rotation/Scaling)
```cpp
#include "bn_affine_bg_ptr.h"
#include "bn_affine_bg_items_world.h"

bn::affine_bg_ptr bg = bn::affine_bg_items::world.create_bg(0, 0);
bg.set_rotation_angle(45);
bg.set_scale(2);
bg.set_horizontal_scale(1.5);
bg.set_vertical_scale(0.8);
bg.set_pivot_position(120, 80);
```

### Input Handling
```cpp
#include "bn_keypad.h"

// Pressed this frame (rising edge)
if(bn::keypad::a_pressed()) { }
if(bn::keypad::b_pressed()) { }
if(bn::keypad::up_pressed()) { }

// Held down (level)
if(bn::keypad::a_held()) { }
if(bn::keypad::left_held()) { }

// Released this frame (falling edge)
if(bn::keypad::b_released()) { }

// Any key
if(bn::keypad::any_pressed()) { }
```

### Fixed-Point Math (No Floats on GBA)
```cpp
#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_math.h"

// Basic fixed-point
bn::fixed speed = 1.5;
bn::fixed result = speed * 2;        // 3.0
int whole = result.integer();        // 3
bn::fixed rounded = result.round_integer();

// 2D point
bn::fixed_point position(100.5, 50.25);
position.set_x(position.x() + speed);

// Math functions
bn::fixed angle = 45;
bn::fixed sin_val = bn::degrees_lut_sin(angle);
bn::fixed cos_val = bn::degrees_lut_cos(angle);
bn::fixed abs_val = bn::abs(value);
```

### Audio
```cpp
#include "bn_music.h"
#include "bn_sound.h"
#include "bn_music_items.h"
#include "bn_sound_items.h"

// Music (one track at a time)
bn::music_items::bgm_title.play(0.5);    // volume 0.0-1.0
bn::music::pause();
bn::music::resume();
bn::music::stop();
bn::music::set_volume(0.8);

// Sound effects (multiple simultaneous)
bn::sound_items::sfx_jump.play();
bn::sound_items::sfx_hit.play(0.7);      // with volume
bn::sound_items::sfx_coin.play(1.0, 1.5, 0);  // volume, speed, panning (-1 to 1)
```

### DMG Audio (Game Boy Sound)
```cpp
#include "bn_dmg_music.h"
#include "bn_dmg_music_items.h"

bn::dmg_music_items::song.play();
bn::dmg_music::pause();
bn::dmg_music::resume();
bn::dmg_music::stop();
bn::dmg_music::set_volume(0.8);
```

### Text Rendering
```cpp
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"
#include "bn_string.h"
#include "common_variable_8x16_sprite_font.h"

bn::sprite_text_generator text_generator(common::variable_8x16_sprite_font);
bn::vector<bn::sprite_ptr, 32> text_sprites;

// Alignment
text_generator.set_center_alignment();

// Generate text
text_generator.generate(0, 0, "Hello GBA!", text_sprites);

// Dynamic text
bn::string<32> score_text = "Score: ";
score_text += bn::to_string<8>(score);
text_sprites.clear();
text_generator.generate(0, -60, score_text, text_sprites);
```

### SRAM (Save Data)
```cpp
#include "bn_sram.h"
#include "bn_array.h"
#include "bn_string.h"

struct save_data
{
    bn::array<char, 16> format_tag;  // Validate save format
    int high_score;
    int current_level;
    bn::array<int, 10> unlocked_levels;
};

// Read save
save_data data;
bn::sram::read(data);

// Validate format tag and initialize if needed
if(data.format_tag != expected_tag)
{
    // Initialize defaults
    data.format_tag = expected_tag;
    data.high_score = 0;
    data.current_level = 1;
}

// Write save
data.high_score = 1000;
bn::sram::write(data);
```

### Sprite Animations
```cpp
#include "bn_sprite_animate_actions.h"

// Forever looping animation
bn::sprite_animate_action<4> walk_action = bn::create_sprite_animate_action_forever(
    sprite, 8,  // wait frames between each
    bn::sprite_items::player.tiles_item(),
    0, 1, 2, 3  // tile indices
);

// Once animation
bn::sprite_animate_action<4> jump_action = bn::create_sprite_animate_action_once(
    sprite, 6,
    bn::sprite_items::player.tiles_item(),
    4, 5, 6, 7
);

// In game loop
if(!walk_action.done())
{
    walk_action.update();
}
```

### Actions (Tweening/Interpolation)
```cpp
#include "bn_sprite_actions.h"
#include "bn_bg_actions.h"

// Move sprite to position over frames
bn::sprite_move_to_action move_action(sprite, 60, target_x, target_y);
bn::sprite_move_by_action move_by(sprite, 30, delta_x, delta_y);

// Scale
bn::sprite_scale_to_action scale_action(sprite, 30, 2.0);

// Rotate
bn::sprite_rotate_to_action rotate_action(sprite, 45, 180);

// Fade/visibility
bn::sprite_visible_toggle_action blink(sprite, 10);

// In game loop
if(!move_action.done())
{
    move_action.update();
}
```

### Palettes
```cpp
#include "bn_bg_palettes.h"
#include "bn_sprite_palettes.h"
#include "bn_color.h"

// Transparent color (color 0)
bn::bg_palettes::set_transparent_color(bn::color(16, 16, 16));

// Global effects
bn::bg_palettes::set_brightness(0.5);      // -1.0 to 1.0
bn::bg_palettes::set_contrast(0.5);        // -1.0 to 1.0
bn::bg_palettes::set_intensity(0.5);       // -1.0 to 1.0
bn::bg_palettes::set_grayscale_intensity(1.0);  // 0.0 to 1.0
bn::bg_palettes::set_fade_intensity(0.5);
bn::bg_palettes::set_fade_color(bn::color(31, 31, 31));  // Fade to white

// Same for sprites
bn::sprite_palettes::set_brightness(0.3);
bn::sprite_palettes::set_grayscale_intensity(0.5);

// Individual palette manipulation
bn::sprite_palette_ptr palette = sprite.palette();
palette.set_fade(bn::color(0, 0, 0), 0.5);  // Fade to black
palette.set_inverted(true);
```

### Blending
```cpp
#include "bn_blending.h"

sprite.set_blending_enabled(true);
bg.set_blending_enabled(true);

// Transparency
bn::blending::set_transparency_alpha(0.5);  // 0.0 to 1.0

// Fade
bn::blending::set_fade_alpha(0.3);
bn::blending::set_fade_color(bn::blending::fade_color_type::WHITE);
bn::blending::set_fade_color(bn::blending::fade_color_type::BLACK);

// Intensity blending
bn::blending::set_intensity_alpha(0.5);
```

### Windows
```cpp
#include "bn_window.h"
#include "bn_rect_window.h"

bn::window outside_window = bn::window::outside();
outside_window.set_show_sprites(true);
outside_window.set_show_bg(bg, false);

bn::rect_window internal_window = bn::rect_window::internal();
internal_window.set_boundaries(-40, -60, 40, 60);  // top, left, bottom, right
internal_window.set_show_sprites(true);
internal_window.set_show_blending(true);
```

### Mosaic Effect
```cpp
#include "bn_sprites_mosaic.h"
#include "bn_bgs_mosaic.h"

sprite.set_mosaic_enabled(true);
bn::sprites_mosaic::set_stretch(0.5);  // 0.0 to 1.0

bg.set_mosaic_enabled(true);
bn::bgs_mosaic::set_stretch(0.3);
```

### Cameras
```cpp
#include "bn_camera_ptr.h"

bn::camera_ptr camera = bn::camera_ptr::create(0, 0);
sprite.set_camera(camera);
bg.set_camera(camera);

camera.set_position(player.x(), player.y());
```

### Random Numbers
```cpp
#include "bn_random.h"
#include "bn_seed_random.h"

bn::random random;
int value = random.get_int(100);         // 0-99
int range = random.get_int(10, 20);      // 10-19
bn::fixed f = random.get_fixed(1);       // 0.0-0.999... 
bool coin = random.get_int(2) == 0;

// Seeded random (deterministic)
bn::seed_random seeded(12345);
int val = seeded.get_int(100);
```

### Timers
```cpp
#include "bn_timer.h"
#include "bn_timers.h"

bn::timer timer;
uint64_t ticks = timer.elapsed_ticks();
uint64_t ticks_restart = timer.elapsed_ticks_with_restart();
timer.restart();

// Convert ticks
uint64_t frames = ticks / bn::timers::ticks_per_frame();
uint64_t seconds = ticks / bn::timers::ticks_per_second();
```

### Date and Time (RTC)
```cpp
#include "bn_date.h"
#include "bn_time.h"

if(bn::date::active())
{
    bn::optional<bn::date> date = bn::date::current();
    if(date)
    {
        int year = date->year();
        int month = date->month();
        int day = date->month_day();
        int weekday = date->week_day();  // 0=Sunday
    }
}

if(bn::time::active())
{
    bn::optional<bn::time> time = bn::time::current();
    if(time)
    {
        int hour = time->hour();
        int minute = time->minute();
        int second = time->second();
    }
}
```

### Link Cable (Multiplayer)
```cpp
#include "bn_link.h"
#include "bn_link_state.h"

bn::link::activate(19);  // Baud rate divider

if(bn::optional<bn::link_state> link_state = bn::link::receive())
{
    int player_id = link_state->player_id();
    
    for(const bn::link_player& player : link_state->other_players())
    {
        int data = player.data();
    }
}

bn::link::send(my_data);  // 0-65535
bn::link::deactivate();
```

### Debugging and Profiling
```cpp
// Enable logging:  USERFLAGS := -DBN_CFG_LOG_ENABLED=true
#include "bn_log.h"
BN_LOG("Player position: ", player.x(), ", ", player.y());

// Enable profiler:  USERFLAGS := -DBN_CFG_PROFILER_ENABLED=true
#include "bn_profiler.h"
BN_PROFILER_START("physics");
// code to measure
BN_PROFILER_STOP();

bn::profiler::show();  // Display results on screen

// Assertions (always available)
#include "bn_assert.h"
BN_ASSERT(health > 0, "Health must be positive: ", health);
BN_ERROR("Unreachable code reached");
```

### Core Functions
```cpp
#include "bn_core.h"

bn::core::init();           // Initialize engine (call once at start)
bn::core::update();         // Process frame (call once per frame)
bn::core::reset();          // Soft reset
bn::core::sleep(bn::keypad::key_type::A);  // Sleep until key pressed

bn::fixed cpu = bn::core::last_cpu_usage();  // 0.0 to 1.0+
```

## Asset Configuration

### Sprite JSON (graphics/player.json)
```json
{
    "type": "sprite",
    "height": 32,
    "graphics_count": 8
}
```

Sprite size options:
- 8×8, 16×16, 32×32, 64×64 (square)
- 8×16, 8×32, 16×8, 32×8 (rectangular)
- 16×32, 32×16, 32×64, 64×32 (rectangular)

### Regular Background JSON
```json
{
    "type": "regular_bg",
    "bpp_mode": "bpp_4"
}
```

### Affine Background JSON
```json
{
    "type": "affine_bg",
    "bpp_mode": "bpp_8"
}
```

### BPP Modes
- `bpp_4`: 16 colors per palette (most common for sprites/regular BGs)
- `bpp_8`: 256 colors (required for affine BGs, optional for others)

### Sprite Sheet Requirements
- Format: BMP (indexed color, 4bpp or 8bpp)
- Width: sprite width (8, 16, 32, or 64)
- Height: sprite height × number of frames
- First color (index 0): transparent
- Max colors per palette: 16 (4bpp) or 256 (8bpp)

### Background Requirements
- Format: BMP (indexed color)
- Size: multiple of 8 pixels in both dimensions
- Regular BG max:  512×512 (larger requires streaming)
- First color:  transparent

## GBA Hardware Limits

| Resource | Limit |
|----------|-------|
| Screen size | 240×160 pixels |
| Frame rate | 60 FPS |
| Sprites on screen | 128 max |
| Sprite sizes | 8×8 to 64×64 |
| Sprites per scanline | ~128 pixels width total |
| Background layers | 4 |
| Sprite palettes | 16 (15 colors + transparent each) |
| BG palettes | 16 (15 colors + transparent each) |
| Tile VRAM | ~32KB for sprites, ~64KB for BGs |
| SRAM (save) | 32KB typical (up to 128KB) |
| CPU | 16. 78 MHz ARM7TDMI |
| Work RAM | 32KB fast, 256KB slow |

## Best Practices

1. **Always call `bn::core::update()`** exactly once per frame in your main loop
2. **Use `bn::fixed` instead of float/double** - GBA has no FPU
3. **Prefer stack allocation** - avoid dynamic allocation when possible
4. **Use `bn::vector`, `bn::array`, `bn::string`** instead of std equivalents
5. **Keep sprite/BG counts within limits** - max 128 sprites
6. **Profile early and often** - GBA CPU is limited (16 MHz)
7. **Use actions for animations** - cleaner than manual tweening
8. **Enable logging only in debug builds** - has performance overhead
9. **Validate SRAM format** - use a magic tag to detect corrupted saves
10. **Use `bn::optional`** for resources that may not exist
11. **Prefer composition over inheritance** - fits Butano's patterns
12. **Clear unused resources** - call `.reset()` on optional sprite/bg ptrs
13. **Use object pools** for frequently spawned/despawned objects
14. **Test on real hardware** - emulators may hide timing issues

## Headless Game Testing with mGBA Python Bindings

AI agents can run and test GBA games headlessly (without a display server) using mGBA's Python bindings. This enables automated visual testing, screenshot capture, and programmatic game interaction.

### Setup Instructions

#### 1. Build mGBA with Python Bindings

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

#### 2. Environment Setup

```bash
# Set Python path to find mgba module
export PYTHONPATH="/tmp/mgba-src/build/python/lib.linux-x86_64-cpython-312:$PYTHONPATH"

# Set library path for libmgba.so
export LD_LIBRARY_PATH="/tmp/mgba-src/build:$LD_LIBRARY_PATH"
```

### API Reference

#### Core Functions

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

#### Key Input

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

#### Screenshot Capture

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

### Complete Testing Script

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

### Common Testing Patterns

#### Wait for Screen Transition

```python
# Give the game time to load/transition (60 frames = 1 second at 60fps)
runner.run_frames(60)
```

#### Navigate Menu Down

```python
runner.press_key(GBA_KEY_DOWN)
runner.press_key(GBA_KEY_DOWN)
runner.press_key(GBA_KEY_A)  # Confirm selection
```

#### Movement Test

```python
# Move right for 2 seconds
for _ in range(120):
    runner.core.set_keys(GBA_KEY_RIGHT)
    runner.run_frames(1)
runner.core.clear_keys(GBA_KEY_RIGHT)
```

#### Combo Input

```python
# Press A and B simultaneously
runner.core.set_keys(GBA_KEY_A)
runner.core.set_keys(GBA_KEY_B)
runner.run_frames(5)
runner.core.clear_keys(GBA_KEY_A)
runner.core.clear_keys(GBA_KEY_B)
```

### Troubleshooting mGBA Python Bindings

**ImportError: No module named 'mgba'**
- Ensure PYTHONPATH includes the mGBA Python build directory
- Verify mGBA was built with `-DBUILD_PYTHON=ON`

**ImportError: undefined symbol: EReaderScanLoadImageA**
- Rebuild mGBA with `-DUSE_FFMPEG=ON` flag
- Install FFmpeg development packages first

**ImportError: No module named 'cached_property'**
```bash
pip install cached_property
```

**Screenshot is black**
- Ensure you call `core.set_video_buffer(framebuffer)` before `core.run_frame()`
- Create a new `mgba.image.Image` for each frame capture

**ROM won't load**
- Verify the ROM file path is correct
- Check that the file is a valid GBA ROM

### CI/CD Integration

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