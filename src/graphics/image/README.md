# Image Processing Module

**Layer:** Graphics (Layer 2)  
**Namespace:** `graphics::image`

## Purpose

CPU-side image processing operations before GPU upload. Separate from `graphics::texture` to maintain clear separation between CPU processing and GPU resource management.

## Key Components

### `image_processor.h/cpp`
Core image manipulation operations:
- **Resizing**: resize(), resizeAspect(), createThumbnail()
- **Mipmaps**: generateMipmaps(), calculateMipLevels() (future)
- **Compression**: BC1/BC3/BC7 compression (future)

### Dependencies
- `graphics::texture::ImageData` - Common image data structure
- `core` - Console logging, error handling
- `stb_image_resize2.h` - High-quality image resizing

## Usage Examples

### Create Thumbnail
```cpp
#include "graphics/image/image_processor.h"

auto imageData = TextureLoader::loadFromFile(filePath);
if (imageData) {
    auto thumbnail = ImageProcessor::createThumbnail(
        *imageData, 
        100,  // 100x100 pixels
        ResizeFilter::Bilinear
    );
    
    if (thumbnail) {
        // Upload to GPU...
    }
}
```

### Resize with Aspect Ratio
```cpp
auto resized = ImageProcessor::resizeAspect(
    *imageData,
    512,  // Max width
    512,  // Max height
    ResizeFilter::Bicubic
);
```

## Implementation Status

**Phase 1 (Current):**
- [x] Module structure
- [x] resize() function
- [x] resizeAspect() function
- [x] createThumbnail() function

**Phase 2 (Future):**
- [x] generateMipmaps()
- [x] calculateMipLevels()

**Phase 3+ (Future):**
- [ ] Image filters (blur, sharpen)
- [ ] Channel operations (extract, pack, swizzle)
- [ ] Texture compression (BC1/BC3/BC7)

## See Also

- `/docs/IMAGE_PROCESSING_MODULE.md` - Full design document
- `graphics/texture/` - GPU texture management
