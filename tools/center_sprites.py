import os
from PIL import Image, ImageOps

# Configuration
GRAPHICS_DIR = r'c:\Users\jefda\stranded\graphics\master\Stranded 04 - Hero sprite'
OUTPUT_DIR = r'c:\Users\jefda\stranded\graphics\sprite\player'
ITEM_OUTPUT_DIR = r'c:\Users\jefda\stranded\graphics\sprite\item'

# Ensure output directories exist
os.makedirs(OUTPUT_DIR, exist_ok=True)
os.makedirs(ITEM_OUTPUT_DIR, exist_ok=True)

def get_centering_offset(ref_file_path):
    """Calculate centering offset from reference image."""
    try:
        ref_img = Image.open(ref_file_path).convert("RGBA")
    except Exception as e:
        print(f"Error opening reference file {ref_file_path}: {e}")
        return (0, 0)

    # Extract first 64x64 frame to find the anchor
    first_frame = ref_img.crop((0, 0, 64, 64))
    bbox = first_frame.getbbox()
    
    offset_x = 0
    offset_y = 0
    
    if bbox:
        # Calculate current center
        cx = (bbox[0] + bbox[2]) / 2
        cy = (bbox[1] + bbox[3]) / 2
        
        # Calculate shift needed to move center to (32, 32)
        offset_x = 32 - cx
        offset_y = 32 - cy
    else:
        print(f"Warning: Reference frame is empty! No shifting applied.")
    
    return (offset_x, offset_y)

def extract_frames_from_source(source_file_info, offset_x, offset_y):
    """Extract and center frames from a source file."""
    frames = []
    path = os.path.join(GRAPHICS_DIR, source_file_info['file'])
    
    try:
        img = Image.open(path).convert("RGBA")
    except Exception as e:
        print(f"Could not open {path}: {e}")
        return frames

    # Assume source is a horizontal strip of 64x64 frames
    width, height = img.size
    num_frames = width // 64
    
    # Limit frames if specified
    if 'count' in source_file_info:
        num_frames = min(num_frames, source_file_info['count'])
        
    for i in range(num_frames):
        # Crop frame
        frame = img.crop((i * 64, 0, (i + 1) * 64, 64))
        
        # Create a new blank 64x64 frame
        centered_frame = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
        
        # Paste the content with the offset
        centered_frame.paste(frame, (int(offset_x), int(offset_y)), frame)
        
        frames.append(centered_frame)
    
    return frames

def process_sprite_sheet(output_filename, source_files, ref_image_index=0, is_item=False):
    """
    Combines source files into a single vertical strip BMP (8bpp indexed).
    Centers the content based on the first frame of the source_files[ref_image_index].
    """
    
    # 1. Determine the centering offset from the Reference Image (First frame)
    ref_file_info = source_files[ref_image_index]
    ref_path = os.path.join(GRAPHICS_DIR, ref_file_info['file'])
    offset_x, offset_y = get_centering_offset(ref_path)
    
    print(f"[{output_filename}] Reference: {ref_file_info['file']}")
    print(f"  Shift: ({offset_x}, {offset_y})")

    # 2. Process all frames
    all_frames = []
    
    for src in source_files:
        frames = extract_frames_from_source(src, offset_x, offset_y)
        all_frames.extend(frames)

    if not all_frames:
        print(f"No frames found for {output_filename}")
        return

    # 3. Create vertical strip
    total_height = 64 * len(all_frames)
    final_img = Image.new("RGB", (64, total_height), (255, 0, 255)) # Magenta background
    
    # Paste frames
    for idx, frame in enumerate(all_frames):
        # Paste centered frame onto magenta background
        bg = Image.new("RGBA", (64, 64), (255, 0, 255, 255))
        composite = Image.alpha_composite(bg, frame)
        final_img.paste(composite, (0, idx * 64))

    # 4. Convert to 8bpp indexed BMP
    final_img_p = final_img.quantize(colors=16, method=2) 
    
    out_dir = ITEM_OUTPUT_DIR if is_item else OUTPUT_DIR
    out_path = os.path.join(out_dir, output_filename)
    final_img_p.save(out_path)
    print(f"Saved {out_path} ({len(all_frames)} frames)")

def process_combined_hero_sheet(output_filename, all_mappings, ref_image_index=0):
    """
    Combines all hero sprites into a single grid-based sprite sheet (like hero).
    Arranges frames in a grid layout (rows and columns) instead of a vertical strip.
    """
    
    # 1. Determine the centering offset from the first reference image
    # REMOVED: calculating global offset here.
    
    print(f"[{output_filename}] Combining all hero sprites into grid layout")

    # 2. Extract all frames from all mappings
    all_frames = []
    frame_ranges = {}  # Track frame ranges for each animation set
    
    current_frame = 0
    
    for mapping in all_mappings:
        # Calculate offset for THIS mapping using its first source file as reference.
        # This ensures each direction (Down, Up, Side) is centered independently to (32,32).
        if mapping['sources']:
            ref_file_info = mapping['sources'][0]
            ref_path = os.path.join(GRAPHICS_DIR, ref_file_info['file'])
            offset_x, offset_y = get_centering_offset(ref_path)
        else:
            offset_x, offset_y = 0, 0

        print(f"  Mapping: {mapping['output']}")
        print(f"    Reference: {ref_file_info['file'] if mapping['sources'] else 'None'}")
        print(f"    Shift: ({offset_x}, {offset_y})")

        set_name = mapping['output'].replace('.bmp', '').replace('player_', '')
        start_frame = current_frame
        
        for src in mapping['sources']:
            frames = extract_frames_from_source(src, offset_x, offset_y)
            print(f"    File: {os.path.basename(src['file'])} -> {len(frames)} frames (Start: {current_frame})")
            all_frames.extend(frames)
            current_frame += len(frames)
        
        end_frame = current_frame - 1
        frame_ranges[set_name] = (start_frame, end_frame)
        print(f"  {set_name}: frames {start_frame}-{end_frame} ({end_frame - start_frame + 1} frames)")

    if not all_frames:
        print(f"No frames found for {output_filename}")
        return

    # 3. Arrange frames in a grid layout
    # Use 16 columns (1024 pixels wide for 64x64 sprites)
    # This creates a more square-like layout similar to hero
    cols = 16
    rows = (len(all_frames) + cols - 1) // cols  # Ceiling division
    
    sheet_width = cols * 64
    sheet_height = rows * 64
    
    print(f"  Grid layout: {cols} columns x {rows} rows = {len(all_frames)} frames")
    print(f"  Sheet size: {sheet_width}x{sheet_height} pixels")
    
    final_img = Image.new("RGB", (sheet_width, sheet_height), (255, 0, 255)) # Magenta background
    
    # Paste frames in grid layout (left to right, top to bottom)
    for idx, frame in enumerate(all_frames):
        col = idx % cols
        row = idx // cols
        x = col * 64
        y = row * 64
        
        # Paste centered frame onto magenta background
        bg = Image.new("RGBA", (64, 64), (255, 0, 255, 255))
        composite = Image.alpha_composite(bg, frame)
        final_img.paste(composite, (x, y))

    # 4. Convert to 8bpp indexed BMP
    final_img_p = final_img.quantize(colors=16, method=2) 
    
    # Force Magenta (255, 0, 255) to be at index 0
    palette = final_img_p.getpalette()
    colors = [tuple(palette[i:i+3]) for i in range(0, len(palette), 3)]
    
    transparent_index = -1
    for i, color in enumerate(colors):
        if i >= 16: break # Only check first 16
        if color == (255, 0, 255):
            transparent_index = i
            break
            
    if transparent_index != -1 and transparent_index != 0:
        print(f"  Fixing palette: Swapping index 0 and {transparent_index} (Magenta)")
        # Swap in palette
        colors[0], colors[transparent_index] = colors[transparent_index], colors[0]
        
        # Reconstruct palette
        new_palette = []
        for c in colors:
            new_palette.extend(c)
        final_img_p.putpalette(new_palette)
        
        # Swap in data
        data = list(final_img_p.getdata())
        new_data = []
        for d in data:
            if d == 0:
                new_data.append(transparent_index)
            elif d == transparent_index:
                new_data.append(0)
            else:
                new_data.append(d)
        final_img_p.putdata(new_data)
    elif transparent_index == -1:
        print("  Warning: Magenta not found in palette!")

    out_path = os.path.join(OUTPUT_DIR, output_filename)
    final_img_p.save(out_path)
    print(f"Saved {out_path} ({len(all_frames)} frames in grid layout)")
    
    # Print C++ Animation Config
    print("\n  // C++ Animation Config (Copy to player.cpp):")
    
    # Helper to get start frame for a set
    def get_start(set_name, sub_name=None):
        # set_name: 'down', 'up', 'side'
        # sub_name: 'Idle', 'Move', etc.
        # We need to look into ranges.
        # But ranges are flat.
        # We need to know the order of animations within a set.
        pass
        
    # We know the structure:
    # Sets: down, up, side
    # Sub-anims in order: Idle, Move, Run, Roll, Chop, Slash
    # (Side roll is different count)
    
    # Let's just print the ranges found
    for name, (start, end) in frame_ranges.items():
        print(f"  // {name}: {start}-{end} (Count: {end-start+1})")

    # 5. Create JSON file with grid dimensions
    json_path = os.path.splitext(out_path)[0] + ".json"
    import json
    
    json_data = {
        "type": "sprite",
        "width": 64,
        "height": 64
    }
    
    with open(json_path, 'w') as f:
        json.dump(json_data, f, indent=4)
    
    print(f"Saved {json_path}")


# Define Mappings
# Structure: Output Filename -> List of Source Files
# Each Source File: {'file': path, 'count': optional_limit}

# Hero with sword mappings
# Note: These mappings are used to create the combined hero.bmp file.
# Individual hero_*.bmp files are no longer generated or used.
HERO_WITH_SWORD = [
    {
        'output': 'hero_down.bmp',  # Used only for hero.bmp generation
        'sources': [
            {'file': r'With Sword\Down\04 Stranded - Pack 4 back up-Idle Down.png'},   # 0-11 (12)
            {'file': r'With Sword\Down\04 Stranded - Pack 4 back up-Move Down.png'},   # 12-19 (8)
            {'file': r'With Sword\Down\04 Stranded - Pack 4 back up-Run Down.png'},    # 20-27 (8)
            {'file': r'With Sword\Down\04 Stranded - Pack 4 back up-Roll Down.png'},   # 28-35 (8)
            {'file': r'With Sword\Down\04 Stranded - Pack 4 back up-Chop Down.png'},   # 36-39 (4)
            {'file': r'With Sword\Down\04 Stranded - Pack 4 back up-Slash Down.png'}   # 40-46 (7)
        ]
    },
    {
        'output': 'hero_up.bmp',
        'sources': [
            {'file': r'With Sword\Up\04 Stranded - Pack 4 back up-Idle Up.png'},       # 0-11
            {'file': r'With Sword\Up\04 Stranded - Pack 4 back up-Move Up.png'},       # 12-19
            {'file': r'With Sword\Up\04 Stranded - Pack 4 back up-Run UP.png'},        # 20-27
            {'file': r'With Sword\Up\04 Stranded - Pack 4 back up-Roll Up.png'},       # 28-35
            {'file': r'With Sword\Up\04 Stranded - Pack 4 back up-Chop Up.png'},       # 36-39
            {'file': r'With Sword\Up\attack slash up.png'}                             # 40-46 (7)
        ]
    },
    {
        'output': 'hero_side.bmp',
        'sources': [
            {'file': r'With Sword\Right Left\Idle left right.png'},                    # 0-11
            {'file': r'With Sword\Right Left\R move.png'},                             # 12-19
            {'file': r'With Sword\Right Left\R Run.png'},                              # 20-27
            {'file': r'With Sword\Right Left\R Roll.png'},                             # 28-33 (6 frames!) Note: Down/Up had 8. JSON says 6 (28-33).
            {'file': r'With Sword\Right Left\R CHop.png'},                             # 34-37 (4 frames)
            {'file': r'With Sword\Right Left\R Slash.png'}                             # 38-42 (5 frames)
        ]
    },
    {
        'output': 'hero_death.bmp',
        'sources': [
            {'file': r'With Sword\04 Stranded - Pack 4 back up-Death.png'}
        ]
    }
]

# Hero without sword mappings (for gun)
HERO_WITHOUT_SWORD = [
    {
        'output': 'hero_no_sword_down.bmp',
        'sources': [
            {'file': r'Without Sword (for gun)\Down\04 Stranded - Pack 4 back up-Idle Down.png'},   # 0-11 (12)
            {'file': r'Without Sword (for gun)\Down\04 Stranded - Pack 4 back up-Move Down.png'},   # 12-19 (8)
            {'file': r'Without Sword (for gun)\Down\04 Stranded - Pack 4 back up-Run Down.png'},    # 20-27 (8)
            {'file': r'Without Sword (for gun)\Down\04 Stranded - Pack 4 back up-Roll Down.png'}   # 28-35 (8)
        ]
    },
    {
        'output': 'hero_no_sword_up.bmp',
        'sources': [
            {'file': r'Without Sword (for gun)\Up\04 Stranded - Pack 4 back up-Idle Up.png'},       # 0-11
            {'file': r'Without Sword (for gun)\Up\04 Stranded - Pack 4 back up-Move Up.png'},       # 12-19
            {'file': r'Without Sword (for gun)\Up\04 Stranded - Pack 4 back up-Run UP.png'},        # 20-27
            {'file': r'Without Sword (for gun)\Up\04 Stranded - Pack 4 back up-Roll Up.png'}       # 28-35
        ]
    },
    {
        'output': 'hero_no_sword_side.bmp',
        'sources': [
            {'file': r'Without Sword (for gun)\left right\R.png'},                    # 0-11 (idle)
            {'file': r'Without Sword (for gun)\left right\R move.png'},                             # 12-19
            {'file': r'Without Sword (for gun)\left right\R Run.png'},                              # 20-27
            {'file': r'Without Sword (for gun)\left right\R Roll.png'}                             # 28-33 (6 frames)
        ]
    },
    {
        'output': 'hero_no_sword_death.bmp',
        'sources': [
            {'file': r'Without Sword (for gun)\04 Stranded - Pack 4 back up-Death.png'}
        ]
    }
]

# Buffs (separate, not included in combined sheet)
HERO_BUFFS = [
    {
        'output': 'hero_buffs.bmp',
        'sources': [
            {'file': r'With Sword\Buffs\04 Stranded - Pack 4 back up-Heal.png'},       # 0-15
            {'file': r'With Sword\Buffs\04 Stranded - Pack 4 back up-Defence.png'},    # 16-31
            {'file': r'With Sword\Buffs\04 Stranded - Pack 4 back up-Flames.png'},     # 32-47 (Power)
            {'file': r'With Sword\Buffs\04 Stranded - Pack 4 back up-Buff.png'}        # 48-63 (Energy)
        ]
    }
]

# Combined mappings for the main hero sheet (with sword + without sword + buffs)
COMBINED_HERO_MAPPINGS = HERO_WITH_SWORD + HERO_WITHOUT_SWORD + HERO_BUFFS

# Run processing - create combined hero sheet
print("\n=== Creating combined hero sprite sheet (with and without sword + buffs) ===")
process_combined_hero_sheet('hero.bmp', COMBINED_HERO_MAPPINGS)

# Create buffs separately (Optional, maybe not needed anymore if merged)
# But keeping it doesn't hurt, just overwrites hero_buffs.bmp
# print("\n=== Creating hero buffs sprite sheet (separate) ===")
# for m in HERO_BUFFS:
#    process_sprite_sheet(m['output'], m['sources'])

# Individual hero_*.bmp files are no longer created - only hero.bmp (combined sheet) is used
# The mappings above are only used to generate the combined hero.bmp file

# Process Gun separately (no centering needed usually, or just center normally)
# Gun sprite is 32x32? No, existing gun.bmp is 1654 bytes.
# Let's check gun.png size.
# If gun needs centering, we can do it. But gun is an item/overlay.
# Re-running the gun gen just in case.
# Note: Gun sprites in master are likely 64x64 or smaller strips.
# process_sprite_sheet('gun.bmp', [{'file': r'Guns\gun1.png'}], is_item=True) # Just a placeholder
