from PIL import Image
import os

src_dir = r"C:\Users\numap\Downloads\Eris Esra's Character Template 4.0\16x16"

# Check source
for name in ['16x16 Idle-Sheet.png', '16x16 Walk-Sheet.png']:
    img = Image.open(os.path.join(src_dir, name))
    print(f'{name}: {img.size} mode={img.mode}')
    if img.mode == 'RGBA':
        data = list(img.getdata())
        unique = set()
        for p in data:
            if p[3] > 0:
                unique.add(p[:3])
        print(f'  Unique opaque colors: {sorted(unique)}')

print()

# Check converted BMPs
for name in ['eris_idle.bmp', 'eris_walk.bmp']:
    path = os.path.join(r'd:\repo\stranded\graphics\sprite\player', name)
    img = Image.open(path)
    pal = img.getpalette()
    print(f'{name}: {img.size} mode={img.mode}')
    for i in range(8):
        print(f'  palette[{i}]: ({pal[i*3]}, {pal[i*3+1]}, {pal[i*3+2]})')
    # Show pixel data for first frame (first 16x16)
    w = img.width
    data = list(img.getdata())
    print(f'  First frame (16x16):')
    for row in range(16):
        print(f'    row{row:2d}: {data[row*w:row*w+16]}')

# Save as PNG for visual inspection
for name in ['eris_idle.bmp', 'eris_walk.bmp']:
    path = os.path.join(r'd:\repo\stranded\graphics\sprite\player', name)
    img = Image.open(path).convert('RGB')
    out = path.replace('.bmp', '_check.png')
    img.save(out)
    print(f'\nSaved check: {out}')
