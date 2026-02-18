"""Check BMP format details."""
import struct
import os

def check_bmp(path):
    with open(path, 'rb') as f:
        # BMP header
        magic = f.read(2)
        file_size = struct.unpack('<I', f.read(4))[0]
        f.read(4)  # reserved
        offset = struct.unpack('<I', f.read(4))[0]
        # DIB header
        dib_size = struct.unpack('<I', f.read(4))[0]
        width = struct.unpack('<i', f.read(4))[0]
        height = struct.unpack('<i', f.read(4))[0]
        planes = struct.unpack('<H', f.read(2))[0]
        bpp = struct.unpack('<H', f.read(2))[0]
        compression = struct.unpack('<I', f.read(4))[0]

        print(f"{os.path.basename(path)}:")
        print(f"  Size: {width}x{abs(height)}, bpp={bpp}, compression={compression}")
        print(f"  File size: {file_size}, pixel offset: {offset}")
        print(f"  Height sign: {'top-down' if height < 0 else 'bottom-up'}")

for path in [
    r'd:\repo\stranded\graphics\sprite\player\hero.bmp',
    r'd:\repo\stranded\graphics\sprite\player\eris_idle.bmp',
    r'd:\repo\stranded\graphics\sprite\player\eris_walk.bmp',
]:
    check_bmp(path)
    print()
