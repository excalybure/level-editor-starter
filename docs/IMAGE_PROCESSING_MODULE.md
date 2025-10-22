# Image Processing Module - Design Document

**Status:** Phase 1 Complete ✅  
**Created:** 2025-10-21  
**Layer:** Graphics (Layer 2)  
**Namespace:** `graphics::image`  

---

## Overview

This document outlines the design and implementation of a new image processing module to handle CPU-side image operations before GPU upload. The module is separate from `graphics::texture` to maintain clear separation between CPU processing and GPU resource management.

## Motivation

### Current Problem
The Asset Browser loads full-resolution textures (potentially 4K+) and displays them at 100x100 pixels, causing:
- Excessive GPU memory usage (caching full-size textures for thumbnails)
- Slower rendering (GPU downscaling every frame)
- Wasted bandwidth (uploading multi-megabyte images for small previews)

### Broader Use Cases
Beyond thumbnails, image processing utilities are needed for:
- **Mipmap generation** - LOD chains for rendering
- **Texture compression** - BC1/BC3/BC7 for smaller GPU footprint
- **Channel manipulation** - Extract/pack channels for material workflows
- **Format conversion** - Prepare images for specific GPU formats
- **Image filters** - Blur, sharpen, edge detection (future tools/effects)

---

## Architecture

### Module Structure

```
src/graphics/image/
├── image_processor.h/cpp       # Core resize, mipmap, compression
├── image_filters.h/cpp         # Blur, sharpen, edge detection
├── image_converter.h/cpp       # Format conversions, channel ops
└── README.md                   # Module documentation
```

### Namespace

All classes and functions live in `graphics::image`:

```cpp
namespace graphics::image {
    class ImageProcessor;
    class ImageFilters;
    class ImageConverter;
}
```

### Dependencies

**Depends on:**
- `core` - Console logging, error handling
- `graphics::texture::ImageData` - Common image data structure

**Used by:**
- `editor` - Asset browser thumbnails
- `graphics::texture` - Texture loading pipeline
- `engine` - Asset import/processing

**External Libraries:**
- **stb_image_resize2.h** - High-quality image resizing (compatible with existing stb_image)
- **DirectXTex** (future) - BC compression, advanced format conversions

---

## API Design

### 1. Image Processor (`image_processor.h/cpp`)

Core image manipulation operations.

```cpp
namespace graphics::image {

enum class ResizeFilter {
    NearestNeighbor,  // Fast, blocky
    Bilinear,         // Good quality/speed balance
    Bicubic,          // Higher quality, slower
    Lanczos           // Best quality, slowest
};

enum class MipmapFilter {
    Box,              // Simple averaging
    Triangle,         // Linear
    Kaiser            // High quality
};

class ImageProcessor {
public:
    // ===== Resizing =====
    
    /// Resize image to exact dimensions
    /// @param source Source image data
    /// @param targetWidth Target width in pixels
    /// @param targetHeight Target height in pixels
    /// @param filter Resize filter algorithm
    /// @return Resized image, or nullopt on failure
    static std::optional<ImageData> resize(
        const ImageData& source,
        uint32_t targetWidth,
        uint32_t targetHeight,
        ResizeFilter filter = ResizeFilter::Bilinear
    );
    
    /// Resize image maintaining aspect ratio (fit within box)
    /// @param source Source image data
    /// @param maxWidth Maximum width
    /// @param maxHeight Maximum height
    /// @param filter Resize filter algorithm
    /// @return Resized image, or nullopt on failure
    static std::optional<ImageData> resizeAspect(
        const ImageData& source,
        uint32_t maxWidth,
        uint32_t maxHeight,
        ResizeFilter filter = ResizeFilter::Bilinear
    );
    
    /// Create thumbnail (square crop + resize)
    /// @param source Source image data
    /// @param size Target size (width and height)
    /// @param filter Resize filter algorithm
    /// @return Square thumbnail, or nullopt on failure
    static std::optional<ImageData> createThumbnail(
        const ImageData& source,
        uint32_t size,
        ResizeFilter filter = ResizeFilter::Bilinear
    );
    
    // ===== Mipmaps =====
    
    /// Generate full mipmap chain
    /// @param source Source image (mip level 0)
    /// @param maxLevels Maximum mip levels (0 = full chain to 1x1)
    /// @param filter Downsampling filter
    /// @return Vector of mip levels [0..N], or empty on failure
    static std::vector<ImageData> generateMipmaps(
        const ImageData& source,
        uint32_t maxLevels = 0,
        MipmapFilter filter = MipmapFilter::Kaiser
    );
    
    /// Calculate mip level count for given dimensions
    /// @param width Image width
    /// @param height Image height
    /// @return Number of mip levels in full chain
    static uint32_t calculateMipLevels(uint32_t width, uint32_t height);
    
    // ===== Compression (Future - requires DirectXTex) =====
    
    /// Compress to BC1 (DXT1) - 6:1 compression, no alpha
    static std::optional<ImageData> compressBC1(const ImageData& source);
    
    /// Compress to BC3 (DXT5) - 4:1 compression, sharp alpha
    static std::optional<ImageData> compressBC3(const ImageData& source);
    
    /// Compress to BC7 - 4:1 compression, best quality
    static std::optional<ImageData> compressBC7(const ImageData& source);
};

} // namespace graphics::image
```

---

### 2. Image Filters (`image_filters.h/cpp`)

Image processing effects and filters.

```cpp
namespace graphics::image {

enum class EdgeMode {
    Clamp,      // Extend edge pixels
    Mirror,     // Reflect at edges
    Wrap        // Tile/wrap around
};

class ImageFilters {
public:
    // ===== Convolution Filters =====
    
    /// Apply Gaussian blur
    /// @param source Source image
    /// @param radius Blur radius in pixels (1-10 typical)
    /// @param sigma Gaussian sigma (0 = auto-calculate from radius)
    /// @return Blurred image, or nullopt on failure
    static std::optional<ImageData> blur(
        const ImageData& source,
        float radius,
        float sigma = 0.0f,
        EdgeMode edge = EdgeMode::Clamp
    );
    
    /// Apply box blur (fast, lower quality than Gaussian)
    /// @param source Source image
    /// @param radius Blur radius in pixels
    /// @return Blurred image, or nullopt on failure
    static std::optional<ImageData> boxBlur(
        const ImageData& source,
        uint32_t radius,
        EdgeMode edge = EdgeMode::Clamp
    );
    
    /// Apply sharpen filter
    /// @param source Source image
    /// @param amount Sharpen amount (0.0 = none, 1.0 = standard, >1.0 = aggressive)
    /// @return Sharpened image, or nullopt on failure
    static std::optional<ImageData> sharpen(
        const ImageData& source,
        float amount = 1.0f,
        EdgeMode edge = EdgeMode::Clamp
    );
    
    /// Apply unsharp mask (sharpen via blur subtraction)
    /// @param source Source image
    /// @param radius Blur radius for mask
    /// @param amount Sharpen strength
    /// @param threshold Luminance threshold (avoid sharpening noise)
    /// @return Sharpened image, or nullopt on failure
    static std::optional<ImageData> unsharpMask(
        const ImageData& source,
        float radius,
        float amount,
        float threshold = 0.0f
    );
    
    // ===== Edge Detection =====
    
    /// Sobel edge detection
    /// @param source Source image
    /// @param threshold Edge strength threshold (0-255)
    /// @return Edge map, or nullopt on failure
    static std::optional<ImageData> sobelEdges(
        const ImageData& source,
        uint8_t threshold = 128
    );
    
    // ===== Custom Kernel =====
    
    /// Apply custom convolution kernel
    /// @param source Source image
    /// @param kernel Convolution kernel (must be square, odd size)
    /// @param kernelSize Kernel dimension (3, 5, 7, etc.)
    /// @param divisor Normalization divisor (0 = auto-calculate)
    /// @return Filtered image, or nullopt on failure
    static std::optional<ImageData> applyKernel(
        const ImageData& source,
        const float* kernel,
        uint32_t kernelSize,
        float divisor = 0.0f,
        EdgeMode edge = EdgeMode::Clamp
    );
};

} // namespace graphics::image
```

---

### 3. Image Converter (`image_converter.h/cpp`)

Format conversions and channel operations.

```cpp
namespace graphics::image {

enum class Channel {
    R = 0,
    G = 1,
    B = 2,
    A = 3
};

enum class ColorSpace {
    Linear,         // Linear color space
    sRGB           // Standard sRGB gamma
};

class ImageConverter {
public:
    // ===== Channel Operations =====
    
    /// Extract single channel as grayscale image
    /// @param source Source image (must have target channel)
    /// @param channel Channel to extract
    /// @return Single-channel image (R8_UNORM), or nullopt on failure
    static std::optional<ImageData> extractChannel(
        const ImageData& source,
        Channel channel
    );
    
    /// Pack up to 4 single-channel images into multi-channel image
    /// @param r Red channel (required)
    /// @param g Green channel (optional)
    /// @param b Blue channel (optional)
    /// @param a Alpha channel (optional)
    /// @return Packed image (R8/RG8/RGB8/RGBA8), or nullopt on failure
    static std::optional<ImageData> packChannels(
        const ImageData* r,
        const ImageData* g = nullptr,
        const ImageData* b = nullptr,
        const ImageData* a = nullptr
    );
    
    /// Swizzle channels (e.g., RGBA -> BGRA)
    /// @param source Source image
    /// @param r Source channel for output red
    /// @param g Source channel for output green
    /// @param b Source channel for output blue
    /// @param a Source channel for output alpha
    /// @return Swizzled image, or nullopt on failure
    static std::optional<ImageData> swizzle(
        const ImageData& source,
        Channel r, Channel g, Channel b, Channel a
    );
    
    // ===== Format Conversion =====
    
    /// Convert RGB to RGBA (add alpha channel)
    /// @param source RGB image
    /// @param alpha Alpha value to add (0-255)
    /// @return RGBA image, or nullopt on failure
    static std::optional<ImageData> addAlpha(
        const ImageData& source,
        uint8_t alpha = 255
    );
    
    /// Convert RGBA to RGB (discard alpha)
    /// @param source RGBA image
    /// @return RGB image, or nullopt on failure
    static std::optional<ImageData> discardAlpha(
        const ImageData& source
    );
    
    /// Convert to grayscale (luminance)
    /// @param source Source image
    /// @return Grayscale image (R8_UNORM), or nullopt on failure
    static std::optional<ImageData> toGrayscale(
        const ImageData& source
    );
    
    // ===== Color Space =====
    
    /// Convert between linear and sRGB
    /// @param source Source image
    /// @param from Source color space
    /// @param to Target color space
    /// @return Converted image, or nullopt on failure
    static std::optional<ImageData> convertColorSpace(
        const ImageData& source,
        ColorSpace from,
        ColorSpace to
    );
    
    // ===== Flip/Rotate =====
    
    /// Flip image vertically
    static std::optional<ImageData> flipVertical(const ImageData& source);
    
    /// Flip image horizontally
    static std::optional<ImageData> flipHorizontal(const ImageData& source);
    
    /// Rotate image 90 degrees clockwise
    static std::optional<ImageData> rotate90CW(const ImageData& source);
    
    /// Rotate image 90 degrees counter-clockwise
    static std::optional<ImageData> rotate90CCW(const ImageData& source);
};

} // namespace graphics::image
```

---

## Implementation Plan

### Phase 1: Core Resizing ✅ COMPLETE
**Goal:** Enable asset browser thumbnails

1. **Setup** (`image_processor.h/cpp`) ✅
   - ✅ Create module structure
   - ✅ Integrate `stb_image_resize2.h`
   - ✅ Implement `resize()` function (TDD: 4 test cases, 24 assertions)
   - ✅ Implement `resizeAspect()` function (TDD: 5 test sections, 19 assertions)
   - ✅ Implement `createThumbnail()` function (TDD: 5 test sections, 17 assertions)

2. **Asset Browser Integration** ⏳ PENDING
   - Modify `AssetBrowserPanel::renderAssetGrid()` to call `ImageProcessor::createThumbnail()` before GPU upload
   - Cache thumbnails, not full-resolution images
   - Test with various image sizes (64x64 to 4096x4096)

3. **Testing** ✅
   - ✅ Unit tests for resize operations
   - ✅ Test aspect ratio preservation
   - ✅ Test thumbnail generation (square crop)
   - Benchmark resize performance

**Deliverable:** Core image processing functions implemented and tested. Ready for asset browser integration.

**Test Results:**
```
All tests passed (60 assertions in 6 test cases)
- ImageProcessor::resize: 24 assertions (downscale, upscale, multi-channel, invalid inputs)
- ImageProcessor::resizeAspect: 19 assertions (wide/tall/square/no-upscale/very-wide)
- ImageProcessor::createThumbnail: 17 assertions (wide/tall/square crops, upscale, invalid)
```

---

### Phase 2: Mipmap Generation
**Goal:** Improve texture rendering quality

1. **Implementation** (`image_processor.h/cpp`)
   - Implement `generateMipmaps()` function
   - Implement `calculateMipLevels()` helper
   - Support multiple filter types (Box, Triangle, Kaiser)

2. **Texture Loading Integration**
   - Modify `TextureLoader` to optionally generate mipmaps on load
   - Extend `ImageData` to support mip chains
   - Update `dx12::Texture` to accept mip data

3. **Testing**
   - Test full mip chain generation (down to 1x1)
   - Test partial mip chains (maxLevels parameter)
   - Visual verification (no aliasing at distance)

**Deliverable:** Textures load with full mipmap chains.

---

### Phase 3: Channel Operations
**Goal:** Enable material authoring workflows

1. **Implementation** (`image_converter.h/cpp`)
   - Implement `extractChannel()` function
   - Implement `packChannels()` function
   - Implement `swizzle()` function
   - Implement basic format conversions

2. **Use Cases**
   - Extract roughness from metallic-roughness packed texture
   - Pack custom material maps (R=AO, G=Roughness, B=Metallic)
   - Convert normal maps between coordinate systems

3. **Testing**
   - Test channel extraction (R, G, B, A)
   - Test packing 1-4 channels
   - Verify format conversions

**Deliverable:** Material editor can manipulate texture channels.

---

### Phase 4: Image Filters (Future Enhancement)
**Goal:** Provide image editing capabilities

1. **Implementation** (`image_filters.h/cpp`)
   - Implement Gaussian blur
   - Implement sharpen filter
   - Implement convolution kernel system
   - Support edge modes (clamp, mirror, wrap)

2. **Use Cases**
   - Generate blur for ambient occlusion
   - Sharpen imported textures
   - Custom post-processing effects

3. **Testing**
   - Visual quality tests
   - Performance benchmarks
   - Edge case handling

**Deliverable:** Editor has basic image filtering tools.

---

### Phase 5: Texture Compression (Future Enhancement)
**Goal:** Reduce GPU memory usage

1. **DirectXTex Integration**
   - Add DirectXTex dependency (already planned in docs)
   - Wrap BC1/BC3/BC7 compression APIs
   - Handle sRGB vs linear correctly

2. **Asset Pipeline Integration**
   - Asset import wizard with compression options
   - Automatic compression for certain texture types
   - Quality presets (fast, balanced, best)

3. **Testing**
   - Visual quality comparison (compressed vs uncompressed)
   - Memory usage benchmarks
   - Compression time measurements

**Deliverable:** Assets can be imported as compressed textures.

---

## Testing Strategy

### Unit Tests (`tests/image_processing_tests.cpp`)

```cpp
TEST_CASE("ImageProcessor::resize downscales correctly", "[image][resize]") {
    // Create test image 256x256
    // Resize to 64x64
    // Verify dimensions and format
}

TEST_CASE("ImageProcessor::resizeAspect maintains aspect ratio", "[image][resize]") {
    // Test portrait and landscape images
    // Verify no distortion
}

TEST_CASE("ImageProcessor::createThumbnail produces square output", "[image][thumbnail]") {
    // Test non-square inputs
    // Verify 1:1 aspect ratio
}

TEST_CASE("ImageProcessor::generateMipmaps creates full chain", "[image][mipmap]") {
    // Input: 512x512
    // Verify mip count = 10 (512 -> 1)
    // Check each level dimensions
}

TEST_CASE("ImageConverter::extractChannel isolates correct data", "[image][channel]") {
    // Create RGBA image with known values
    // Extract R, G, B, A separately
    // Verify each matches expected
}

TEST_CASE("ImageFilters::blur smooths image", "[image][filter]") {
    // Create sharp test pattern
    // Apply blur
    // Verify high-frequency content reduced
}
```

### Integration Tests
- Asset browser thumbnail display
- Texture loading with mipmaps
- Material channel packing workflow

### Performance Benchmarks
- Resize operations (various sizes and filters)
- Mipmap generation
- Compression (when implemented)

---

## Example Usage

### Asset Browser Thumbnails

```cpp
// In AssetBrowserPanel::renderAssetGrid()
if (assetType == AssetType::Texture) {
    auto imageData = TextureLoader::loadFromFile(filePath);
    if (imageData) {
        // Generate 100x100 thumbnail (CPU-side)
        auto thumbnail = ImageProcessor::createThumbnail(
            *imageData, 
            kThumbnailSize, 
            ResizeFilter::Bilinear
        );
        
        if (thumbnail) {
            // Upload small image to GPU
            texture = device->createTextureFromImageData(*thumbnail);
            m_textureCache[filePath] = texture;
        }
    }
}
```

### Material Channel Packing

```cpp
// Pack AO, Roughness, Metallic into single RGB texture
auto ao = ImageConverter::extractChannel(aoMap, Channel::R);
auto roughness = ImageConverter::extractChannel(roughnessMap, Channel::G);
auto metallic = ImageConverter::extractChannel(metallicMap, Channel::B);

auto packed = ImageConverter::packChannels(
    ao.has_value() ? &ao.value() : nullptr,
    roughness.has_value() ? &roughness.value() : nullptr,
    metallic.has_value() ? &metallic.value() : nullptr,
    nullptr // No alpha
);

// Upload packed texture to GPU
auto texture = device->createTextureFromImageData(*packed);
```

### Texture Import with Mipmaps

```cpp
// In texture import wizard
auto imageData = TextureLoader::loadFromFile(filePath);
if (imageData) {
    // Generate full mipmap chain
    auto mips = ImageProcessor::generateMipmaps(*imageData);
    
    // Upload all mip levels to GPU
    auto texture = device->createTextureWithMipmaps(mips);
}
```

---

## Open Questions

1. **Performance:**
   - Should resize operations be multi-threaded for large images?
   - Cache intermediate mip levels during generation?

2. **API Design:**
   - Should `ImageData` be extended to hold multiple mip levels, or return `vector<ImageData>`?
   - Should filters be in-place or always allocate new ImageData?

3. **Dependencies:**
   - Use stb_image_resize2 or DirectXTex for resizing?
   - When to pull in DirectXTex? (adds significant dependency)

4. **Integration:**
   - Should texture loader automatically generate mipmaps, or require explicit request?
   - Should compression be part of asset import or runtime?

---

## Dependencies

### External Libraries

**Immediate:**
- **stb_image_resize2.h** - Single-header resize library
  - Compatible with existing stb_image
  - High-quality filters (Mitchell, Catmull-Rom, etc.)
  - No additional build config needed

**Future:**
- **DirectXTex** - Microsoft texture processing library
  - BC compression (BC1/BC3/BC4/BC5/BC7)
  - DDS file I/O
  - Advanced format conversions
  - Mipmap generation with custom filters

### Internal Dependencies

```
graphics::image
    ↓
graphics::texture (ImageData struct)
    ↓
core (console, error handling)
```

---

## References

- **stb_image_resize2:** https://github.com/nothings/stb/blob/master/stb_image_resize2.h
- **DirectXTex:** https://github.com/microsoft/DirectXTex
- **Block Compression Formats:** https://learn.microsoft.com/en-us/windows/win32/direct3d11/texture-block-compression-in-direct3d-11
- **glTF Texture Support Plan:** `/docs/GLTF_TEXTURE_SUPPORT_PLAN.md`

---

## Commit Message Template

```
graphics: add image processing module for CPU-side operations

- New module: src/graphics/image/ (Layer 2)
- ImageProcessor: resize, mipmaps, compression (future)
- ImageFilters: blur, sharpen, edge detection
- ImageConverter: channel ops, format conversion
- Namespace: graphics::image

Phase 1: Core resizing for asset browser thumbnails
- Implement resize(), resizeAspect(), createThumbnail()
- Integrate stb_image_resize2.h
- Add unit tests for resize operations

Fixes asset browser memory usage (thumbnails now 100x100, not 4K)

Refs: IMAGE_PROCESSING_MODULE.md
```
