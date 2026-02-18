"""Copy source PNGs to project for visual comparison."""
from PIL import Image
import os

src_dir = r"C:\Users\numap\Downloads\Eris Esra's Character Template 4.0\16x16"
out_dir = r"d:\repo\stranded\graphics\sprite\player"

for name in ['16x16 Idle-Sheet.png', '16x16 Walk-Sheet.png']:
    img = Image.open(os.path.join(src_dir, name))
    # Save copy
    out_name = name.replace(' ', '_').replace('16x16_', 'source_').lower()
    img.save(os.path.join(out_dir, out_name))
    print(f"Copied {name} -> {out_name}")
