
from PIL import Image
import math

def generate_spawn_animation():
    try:
        # Load the target heart image
        source_path = 'c:/Users/jefda/stranded/graphics/sprite/hud/heart_empty.bmp'
        target_path = 'c:/Users/jefda/stranded/graphics/sprite/hud/heart_empty_spawn.bmp'
        
        src_img = Image.open(source_path)
        if src_img.mode != 'P':
            print("Error: Source image is not indexed (Mode P)")
            return

        # Get palette
        palette = src_img.getpalette()
        
        # Dimensions
        width, height = src_img.size
        num_frames = 14
        
        # Create output image
        out_img = Image.new('P', (width, height * num_frames))
        out_img.putpalette(palette)
        
        # Transparent color index (usually 0)
        trans_index = 0
        # Fill with transparency
        out_img.paste(trans_index, (0, 0, width, height * num_frames))

        for i in range(num_frames):
            # Frame 0 is empty (already filled with trans_index)
            if i == 0:
                continue
                
            # Frame 13 is full
            if i == 13:
                out_img.paste(src_img, (0, i * height))
                continue

            # Intermediate frames: Scale up
            # Scale from 0.1 to 1.0
            scale = i / 13.0
            
            # New dimensions
            new_w = int(width * scale)
            new_h = int(height * scale)
            
            # Avoid 0 size
            if new_w < 1: new_w = 1
            if new_h < 1: new_h = 1
            
            # Resize
            # nearest neighbor to preserve palette indices mostly, but resizing indexed images can be tricky.
            # Convert to RGBA for resizing to handle transparency correctly, then quantize back?
            # Or just resize the index map? Resizing index map with NEAREST is safe.
            frame = src_img.resize((new_w, new_h), Image.NEAREST)
            
            # Center position
            x = (width - new_w) // 2
            y = (height - new_h) // 2
            
            # Paste into the strip
            # We need to paste only the non-transparent pixels or just paste the rect?
            # Since we initialized with transparency, pasting the rect is fine as long as the resized image has transparency where needed.
            # But wait, src_img background is index 0. Resize might preserve it.
            
            frame_box = (0, 0, new_w, new_h)
            paste_box = (x, i * height + y)
            
            out_img.paste(frame, paste_box)

        out_img.save(target_path)
        print(f"Successfully generated {target_path} ({width}x{height*num_frames})")

    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    generate_spawn_animation()
