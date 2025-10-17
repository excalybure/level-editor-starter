"""
Generate test texture images for glTF texture support testing.
Creates 256x256 PNG files: checkerboard, solid colors (red, green, blue).
Uses only Python standard library (no PIL/Pillow required).
"""

import os
import struct
import zlib

def create_png(width, height, pixels):
    """
    Create a minimal PNG file from RGB pixel data.
    pixels: list of (r,g,b) tuples, row by row
    """
    # PNG signature
    png_signature = b'\x89PNG\r\n\x1a\n'
    
    # IHDR chunk (image header)
    ihdr_data = struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0)
    ihdr_chunk = create_chunk(b'IHDR', ihdr_data)
    
    # IDAT chunk (image data)
    # Build scanlines: each row starts with filter byte (0 = no filter)
    scanlines = b''
    for y in range(height):
        scanlines += b'\x00'  # Filter type: None
        for x in range(width):
            r, g, b = pixels[y * width + x]
            scanlines += struct.pack('BBB', r, g, b)
    
    # Compress scanlines
    compressed = zlib.compress(scanlines, 9)
    idat_chunk = create_chunk(b'IDAT', compressed)
    
    # IEND chunk (end of file)
    iend_chunk = create_chunk(b'IEND', b'')
    
    return png_signature + ihdr_chunk + idat_chunk + iend_chunk

def create_chunk(chunk_type, data):
    """Create a PNG chunk with length, type, data, and CRC."""
    length = struct.pack('>I', len(data))
    crc = zlib.crc32(chunk_type + data) & 0xffffffff
    crc_bytes = struct.pack('>I', crc)
    return length + chunk_type + data + crc_bytes

def create_checkerboard(size=256, square_size=32):
    """Create a black and white checkerboard pattern."""
    pixels = []
    for y in range(size):
        for x in range(size):
            # Determine which square we're in
            square_x = x // square_size
            square_y = y // square_size
            # Alternate black and white
            if (square_x + square_y) % 2 == 0:
                pixels.append((255, 255, 255))  # White
            else:
                pixels.append((0, 0, 0))  # Black
    return pixels

def create_solid_color(size=256, color=(255, 0, 0)):
    """Create a solid color image."""
    return [color] * (size * size)

def main():
    output_dir = os.path.join('assets', 'test', 'textures')
    os.makedirs(output_dir, exist_ok=True)
    
    size = 256
    
    # Generate checkerboard
    pixels = create_checkerboard(size, 32)
    png_data = create_png(size, size, pixels)
    filepath = os.path.join(output_dir, 'checkerboard_256.png')
    with open(filepath, 'wb') as f:
        f.write(png_data)
    print(f"Created: {filepath}")
    
    # Generate solid colors
    colors = {
        'red': (255, 0, 0),
        'green': (0, 255, 0),
        'blue': (0, 0, 255),
        'white': (255, 255, 255)
    }
    
    for name, color in colors.items():
        pixels = create_solid_color(size, color)
        png_data = create_png(size, size, pixels)
        filepath = os.path.join(output_dir, f'{name}_256.png')
        with open(filepath, 'wb') as f:
            f.write(png_data)
        print(f"Created: {filepath}")
    
    print("\n✓ All test textures generated successfully")

if __name__ == '__main__':
    main()
