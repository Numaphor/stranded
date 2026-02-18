from PIL import Image
for name in ['eris_idle', 'eris_walk']:
    path = rf'd:\repo\stranded\graphics\sprite\player\{name}.bmp'
    img = Image.open(path).convert('RGB')
    img.save(path.replace('.bmp', '_check.png'))
    print(f'Saved {name}_check.png')
