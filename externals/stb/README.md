# STB Image Library

## About
This directory contains the **stb_image.h** header-only library for image loading.

- **Version**: v2.16 (2017-07-23)
- **Author**: Sean Barrett (nothings.org)
- **License**: Public Domain (or MIT)
- **Source**: https://github.com/nothings/stb

## Supported Formats
- JPEG baseline & progressive
- PNG (1/2/4/8/16-bit per channel)
- TGA
- BMP (non-1bpp, non-RLE)
- PSD (composited view, 8/16-bit per channel)
- GIF
- HDR (radiance rgbE format)
- PIC (Softimage PIC)
- PNM (PPM and PGM binary only)

## Usage
To use stb_image in your code:

```cpp
// In ONE C++ file to create the implementation:
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// In other files, just include normally:
#include <stb_image.h>
```

## License Verification
stb_image is in the **public domain** - see the license section at the end of stb_image.h.
There are no license conflicts with this project.

## Why stb_image?
- **Header-only**: No build system complexity
- **Widely used**: Battle-tested in thousands of projects
- **Comprehensive**: Covers 99% of glTF texture formats
- **Simple API**: `stbi_load()` / `stbi_load_from_memory()`
- **Already available**: Referenced by ImGuizmo dependency
