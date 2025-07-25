from PIL import Image, ImageDraw, ImageFont
import os

# Create output directory if it doesn't exist
os.makedirs('graphics/fonts', exist_ok=True)

# Create a new image with a transparent background
img = Image.new('RGBA', (128, 48), (0, 0, 0, 0))
draw = ImageDraw.Draw(img)

# Use a small font
font = ImageFont.load_default()

# Draw characters
text = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"

for i, char in enumerate(text):
    x = (i % 16) * 8
    y = (i // 16) * 8
    draw.text((x, y), char, fill=(255, 255, 255), font=font)

# Save the image
img.save('graphics/fonts/fixed_8x8_font.png')

print("Font image generated successfully!")
