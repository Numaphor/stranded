from PIL import Image
import os

base = r"C:\Users\numap\Downloads\Eris Esra's Character Template 4.0\16x16"
sheets = ['Idle', 'Walk', 'Run', 'Jump', 'Interact', 'Rotate', 'All Animations']
for s in sheets:
    path = os.path.join(base, f"16x16 {s}-Sheet.png")
    img = Image.open(path)
    w, h = img.size
    cols = w // 16
    rows = h // 16
    pixels = list(img.getdata())
    unique = set(pixels)
    non_transparent = [p for p in unique if len(p) < 4 or p[3] > 0]
    print(f"{s}: {w}x{h} mode={img.mode} grid={cols}x{rows}={cols*rows}frames colors={len(non_transparent)}")
