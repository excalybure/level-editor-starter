# glTF Texture Support - Comprehensive Implementation Plan

**Date:** 2025-10-15  
**Milestone:** M3 - Data-Driven Material System  
**Scope:** Add complete texture loading, bindless descriptor management, and textured mesh rendering for glTF scenes

---

## 🎯 Executive Summary

This plan details the implementation of texture support for glTF scenes, covering three major areas:
1. **Texture Loading** - Load images from glTF files (embedded or external) and upload to GPU
2. **Bindless Descriptor Management** - Manage a large descriptor heap for texture SRVs with efficient allocation
3. **Rendering Integration** - Bind textures to shaders and render textured meshes

**Estimated Scope:** Large (10-15 tasks across multiple systems)  
**Complexity:** High (GPU resource management, bindless descriptors, image decoding)  
**Dependencies:** Existing MaterialSystem, MeshRenderingSystem, dx12 abstractions

---

## 📋 Current State Analysis

### ✅ What We Have
- **glTF Loader:** Parses texture URIs from materials (`gltf_loader.cpp:extractMaterial()`)
- **Material System:** Stores texture paths in `PBRMaterial` structure (`assets.h`)
- **dx12 Texture Class:** Creates render targets and SRVs for viewport rendering (`dx12_texture.cpp`)
- **Descriptor Heap:** ImGui SRV heap with 80 descriptors (16 for ImGui, 64 for viewports)
- **MaterialGPU:** Framework for material constant buffers (no texture binding yet)
- **Shader Stubs:** Commented-out texture sampling in `unlit.hlsl`

### ❌ What We Need
- **Image Loading Library:** Decode PNG/JPEG/BMP/etc. from files or embedded data
- **Texture Upload:** CPU→GPU transfer with staging buffers and proper transitions
- **Bindless Heap Management:** Large SRV heap (thousands of textures) with allocation/deallocation
- **Texture Resource Manager:** Cached loading, lifetime management, handle-based access
- **MaterialInstance Integration:** Bind material textures to shader registers
- **Shader Updates:** Enable texture sampling in pixel shaders
- **glTF Integration:** Resolve texture paths relative to .gltf file location

---

## 🏗️ Architecture Overview

### High-Level Data Flow
```
.gltf file → GLTFLoader (parse URIs) → Scene/Material (store paths)
                                          ↓
Image Files → TextureLoader (decode) → TextureManager (upload to GPU)
                                          ↓
                                    GPU Texture Resource + SRV Index
                                          ↓
MaterialInstance → Bind SRV table → Shader (sample textures)
                        ↓
                MeshRenderingSystem (render)
```

### Key Components

#### 1. **TextureLoader** (New)
- **Location:** `src/graphics/texture/texture_loader.{h,cpp}`
- **Responsibility:** Decode image files using stb_image or DirectXTex
- **API:**
  ```cpp
  struct ImageData {
      std::vector<uint8_t> pixels;
      uint32_t width, height;
      DXGI_FORMAT format;
  };
  
  std::optional<ImageData> loadImageFromFile(const std::string& path);
  std::optional<ImageData> loadImageFromMemory(const uint8_t* data, size_t size);
  ```

#### 2. **TextureManager** (New)
- **Location:** `src/graphics/texture/texture_manager.{h,cpp}`
- **Responsibility:** Manage GPU textures, descriptor heap, handle allocation
- **API:**
  ```cpp
  using TextureHandle = uint32_t;
  constexpr TextureHandle INVALID_TEXTURE_HANDLE = 0;
  
  class TextureManager {
  public:
      bool initialize(dx12::Device* device, uint32_t maxTextures = 4096);
      TextureHandle loadTexture(const std::string& path, const std::string& basePath = "");
      void releaseTexture(TextureHandle handle);
      uint32_t getSrvIndex(TextureHandle handle) const;
      ID3D12DescriptorHeap* getSrvHeap() const;
  };
  ```

#### 3. **Bindless SRV Heap** (Enhancement)
- **Location:** Extend `dx12::Device` or create dedicated heap manager
- **Specs:**
  - **Capacity:** 4096 SRVs (overkill for now, room for growth)
  - **Allocation:** Free-list or slot-based allocation
  - **Separate from ImGui Heap:** New heap for game textures only

#### 4. **MaterialGPU Integration** (Enhancement)
- **Changes:**
  - Add texture handles to MaterialGPU constructor/state
  - Bind descriptor table in `bindToCommandList()`
  - Update root signature to include SRV descriptor table

#### 5. **Shader Updates** (Enhancement)
- **Files:** `shaders/unlit.hlsl`, `shaders/simple.hlsl`
- **Changes:**
  - Uncomment texture declarations
  - Add sampler state
  - Sample textures in pixel shader
  - Multiply with vertex colors/material factors

---

## 📦 Detailed Task Breakdown

### 🔹 Phase 1: Image Loading Foundation

#### **Task 1.1: Choose Image Loading Library**
**Decision:** Use **stb_image** (header-only, public domain, widely used)

**Rationale:**
- Already in ImGui dependencies (externals/ImGuizmo/example/stb_image.h)
- Supports PNG, JPEG, BMP, TGA, HDR (covers 99% of glTF textures)
- Simple API: `stbi_load()` / `stbi_load_from_memory()`
- No vcpkg dependency needed (header-only)

**Alternative:** DirectXTex (more features, but heavier dependency)

**Action Items:**
- [ ] Copy `stb_image.h` to `externals/stb/` or reference existing copy
- [ ] Update CMakeLists.txt to include stb headers
- [ ] Verify no license conflicts (stb is public domain)

#### **Task 1.2: Implement TextureLoader Module**
**Files:**
- `src/graphics/texture/texture_loader.h`
- `src/graphics/texture/texture_loader.cpp`
- `tests/texture_loader_tests.cpp`

**Functionality:**
- Decode image files (PNG, JPEG, BMP, TGA)
- Handle embedded glTF data URIs (base64-encoded)
- Convert to RGBA8 or BC compressed formats
- Error handling for missing/corrupt files

**Test Cases:**
- Load valid PNG file → returns correct dimensions and pixel data
- Load embedded data URI → decodes base64 and loads image
- Load missing file → returns std::nullopt
- Load corrupt image → returns std::nullopt
- Verify RGBA8 pixel layout (stride, byte order)

**API Design:**
```cpp
#pragma once

#include <vector>
#include <string>
#include <optional>
#include <cstdint>
#include <dxgiformat.h>

namespace graphics::texture {

struct ImageData {
    std::vector<uint8_t> pixels;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t channels = 4; // Always RGBA
    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
};

class TextureLoader {
public:
    // Load from file path (relative or absolute)
    static std::optional<ImageData> loadFromFile(const std::string& path);
    
    // Load from memory buffer (for embedded data)
    static std::optional<ImageData> loadFromMemory(const uint8_t* data, size_t size);
    
    // Load from glTF data URI (handles base64 decoding)
    static std::optional<ImageData> loadFromDataURI(const std::string& uri);
    
private:
    static std::optional<std::vector<uint8_t>> decodeBase64(const std::string& encoded);
};

} // namespace graphics::texture
```

---

### 🔹 Phase 2: GPU Texture Resource Management

#### **Task 2.1: Extend dx12::Texture for Texture Loading**
**Files:**
- `src/platform/dx12/dx12_texture.cpp` (enhance existing)
- `tests/dx12_texture_tests.cpp` (add new tests)

**New Methods:**
```cpp
class Texture {
public:
    // Existing: createRenderTarget(), createShaderResourceView()
    
    // NEW: Create texture from CPU data
    bool createFromImageData(
        Device* device, 
        const ImageData& imageData,
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    
    // NEW: Upload texture data using staging buffer
    bool uploadTextureData(
        ID3D12GraphicsCommandList* commandList,
        const uint8_t* data,
        uint32_t rowPitch,
        uint32_t slicePitch);
};
```

**Implementation Notes:**
- Use `D3D12_RESOURCE_DIMENSION_TEXTURE2D`
- Default heap type: `D3D12_HEAP_TYPE_DEFAULT` (GPU-only memory)
- Create staging buffer (UPLOAD heap) for CPU→GPU transfer
- Execute copy command on command queue
- Transition texture to `D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE`

**Test Cases:**
- Create texture from 256x256 RGBA8 data → succeeds
- Upload texture data via staging buffer → verifies no crashes
- Create SRV after upload → SRV handle is valid
- Invalid data (null pointer) → returns false

#### **Task 2.2: Design Bindless Descriptor Heap**
**Approach:** Dedicated SRV heap separate from ImGui

**Specs:**
- **Capacity:** 4096 descriptors
- **Type:** `D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV`
- **Flags:** `D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE`
- **Allocation:** Slot-based with free list

**Architecture:**
```cpp
class BindlessTextureHeap {
public:
    bool initialize(ID3D12Device* device, uint32_t maxDescriptors = 4096);
    void shutdown();
    
    // Allocate descriptor slot, returns index
    std::optional<uint32_t> allocate();
    
    // Free descriptor slot
    void deallocate(uint32_t index);
    
    // Create SRV at slot index
    void createSRV(uint32_t index, ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc);
    
    ID3D12DescriptorHeap* getHeap() const { return m_heap.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle(uint32_t index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE getGpuHandle(uint32_t index) const;

private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_heap;
    uint32_t m_descriptorSize = 0;
    std::vector<uint32_t> m_freeList; // Available slots
    uint32_t m_maxDescriptors = 0;
};
```

**Test Cases:**
- Initialize heap with 4096 slots → succeeds
- Allocate 100 descriptors → returns unique indices [0..99]
- Deallocate slot 50, reallocate → reuses slot 50
- Allocate 4097 descriptors → last allocation fails (heap full)
- Create SRV at valid index → no crashes

#### **Task 2.3: Implement TextureManager**
**Files:**
- `src/graphics/texture/texture_manager.h`
- `src/graphics/texture/texture_manager.cpp`
- `tests/texture_manager_tests.cpp`

**Responsibilities:**
- Load images via TextureLoader
- Create GPU textures via dx12::Texture
- Allocate SRV descriptors from BindlessTextureHeap
- Cache textures by path (avoid duplicate loads)
- Reference counting for shared textures

**API:**
```cpp
#pragma once

#include <d3d12.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <dxgiformat.h>

// Forward declarations
namespace dx12 {
    class Device;
    class Texture;
}

namespace graphics::texture {

struct ImageData; // From texture_loader.h
class BindlessTextureHeap;

using TextureHandle = uint32_t;
constexpr TextureHandle INVALID_TEXTURE_HANDLE = 0;

struct TextureInfo {
    uint32_t width = 0;
    uint32_t height = 0;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    uint32_t srvIndex = 0; // Index in bindless heap
};

class TextureManager {
public:
    bool initialize(dx12::Device* device, uint32_t maxTextures = 4096);
    void shutdown();
    
    // Load texture from file path (caches by path)
    TextureHandle loadTexture(const std::string& path, const std::string& basePath = "");
    
    // Load texture from memory (no caching)
    TextureHandle loadTextureFromMemory(const uint8_t* data, size_t size, const std::string& debugName = "");
    
    // Release texture (decrements refcount)
    void releaseTexture(TextureHandle handle);
    
    // Query texture info
    const TextureInfo* getTextureInfo(TextureHandle handle) const;
    uint32_t getSrvIndex(TextureHandle handle) const;
    
    // Get bindless heap (for binding to command list)
    ID3D12DescriptorHeap* getSrvHeap() const;
    
private:
    struct TextureEntry {
        std::shared_ptr<dx12::Texture> texture;
        TextureInfo info;
        std::string path; // For caching
        uint32_t refCount = 0;
    };
    
    dx12::Device* m_device = nullptr;
    std::unique_ptr<BindlessTextureHeap> m_bindlessHeap;
    std::vector<TextureEntry> m_textures; // Indexed by handle
    std::unordered_map<std::string, TextureHandle> m_pathCache;
    std::vector<TextureHandle> m_freeHandles; // Recycled handles
};

} // namespace graphics::texture
```

**Test Cases:**
- Initialize TextureManager → succeeds
- Load texture "test.png" twice → returns same handle (cached)
- Get SRV index for valid handle → returns valid index [0..4095]
- Release texture decrements refcount → texture freed when refcount=0
- Load 4096 unique textures → all succeed
- Load 4097th texture → fails (heap full)
- Get info for invalid handle → returns nullptr

---

### 🔹 Phase 3: glTF Integration

#### **Task 3.1: Update GLTFLoader to Store Base Path**
**Files:**
- `src/engine/gltf_loader/gltf_loader.cpp`
- `tests/gltf_loader_tests.cpp`

**Changes:**
- Extract directory path from glTF file path
- Store base path in Scene or Material metadata
- Support both relative and absolute texture paths

**Example:**
```cpp
// In GLTFLoader::loadScene()
std::filesystem::path gltfPath(filePath);
std::string basePath = gltfPath.parent_path().string();

// Pass base path to extractMaterial()
auto material = extractMaterial(&data->materials[i], data, basePath);
```

**Test Cases:**
- Load glTF from "assets/models/cube/cube.gltf" → base path = "assets/models/cube"
- Material with texture "textures/albedo.png" → full path = "assets/models/cube/textures/albedo.png"
- Material with absolute path "C:/textures/test.png" → uses absolute path

#### **Task 3.2: Load Textures in Scene Asset Loading**
**Files:**
- `src/engine/assets/assets.h` (add texture handles to Material)
- Scene loading code (where Scene is constructed)

**Changes:**
```cpp
// In assets::Material::PBRMaterial
struct PBRMaterial {
    // Existing fields...
    std::string baseColorTexture;
    
    // NEW: GPU texture handles (populated after loading)
    graphics::texture::TextureHandle baseColorTextureHandle = INVALID_TEXTURE_HANDLE;
    graphics::texture::TextureHandle metallicRoughnessTextureHandle = INVALID_TEXTURE_HANDLE;
    graphics::texture::TextureHandle normalTextureHandle = INVALID_TEXTURE_HANDLE;
    graphics::texture::TextureHandle emissiveTextureHandle = INVALID_TEXTURE_HANDLE;
};
```

**Loading Flow:**
```cpp
// After GLTFLoader::loadScene() creates Scene:
for (auto& material : scene->getMaterials()) {
    auto& pbr = material->getPBRMaterial();
    
    if (!pbr.baseColorTexture.empty()) {
        pbr.baseColorTextureHandle = textureManager->loadTexture(
            pbr.baseColorTexture, 
            scene->getBasePath());
    }
    // Repeat for other textures...
}
```

**Test Cases:**
- Load glTF with texture → Material contains valid texture handle
- Load glTF without texture → Material has INVALID_TEXTURE_HANDLE
- Load glTF with missing texture file → logs error, handle = INVALID

---

### 🔹 Phase 4: Shader and Root Signature Updates

#### **Task 4.1: Update Root Signature for Texture Binding**
**Files:**
- `src/runtime/mesh_rendering_system.cpp` (root signature creation)

**Current Root Signature:**
```
[0] CBV: Frame constants (b0)
[1] ROOT_CONSTANTS: Object constants (b1, 16 DWORDS)
```

**New Root Signature:**
```
[0] CBV: Frame constants (b0)
[1] ROOT_CONSTANTS: Object constants (b1, 16 DWORDS)
[2] DESCRIPTOR_TABLE: SRVs (t0-t3, space0) → Base color, Normal, MetallicRoughness, Emissive
[3] DESCRIPTOR_TABLE: Sampler (s0, space0) → Linear wrap sampler
```

**Implementation:**
```cpp
// In MeshRenderingSystem::MeshRenderingSystem()
D3D12_DESCRIPTOR_RANGE srvRange = {};
srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
srvRange.NumDescriptors = 4; // t0-t3
srvRange.BaseShaderRegister = 0;
srvRange.RegisterSpace = 0;
srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

D3D12_ROOT_PARAMETER srvParameter = {};
srvParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
srvParameter.DescriptorTable.NumDescriptorRanges = 1;
srvParameter.DescriptorTable.pDescriptorRanges = &srvRange;
srvParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

// Static sampler (more efficient than descriptor table)
D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
samplerDesc.MipLODBias = 0.0f;
samplerDesc.MaxAnisotropy = 16;
samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
samplerDesc.MinLOD = 0.0f;
samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
samplerDesc.ShaderRegister = 0;
samplerDesc.RegisterSpace = 0;
samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
```

**Test Cases:**
- Create root signature with texture parameters → succeeds
- PSO creation with new root signature → succeeds
- Shader compilation with texture declarations → succeeds

#### **Task 4.2: Update Shaders to Sample Textures**
**Files:**
- `shaders/unlit.hlsl`
- `shaders/simple.hlsl` (if used)

**Changes:**
```hlsl
// Uncomment texture declarations
Texture2D baseColorTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D metallicRoughnessTexture : register(t2);
Texture2D emissiveTexture : register(t3);
SamplerState textureSampler : register(s0);

// In pixel shader:
PSOutput main(VSOutput input) {
    PSOutput output;
    
    // Sample base color texture
    float4 baseColor = baseColorTexture.Sample(textureSampler, input.texCoord);
    
    // Multiply with material factor
    baseColor *= g_MaterialConstants.baseColorFactor;
    
    // Multiply with vertex color
    baseColor *= input.color;
    
    output.color = baseColor;
    return output;
}
```

**Test Cases:**
- Compile shader with texture sampling → succeeds
- Render textured quad → displays texture correctly
- Render mesh without texture → uses white default (1,1,1,1)

---

### 🔹 Phase 5: MaterialGPU and Rendering Integration

#### **Task 5.1: Update MaterialGPU to Manage Texture Handles**
**Files:**
- `src/graphics/gpu/material_gpu.h`
- `src/graphics/gpu/material_gpu.cpp`
- `tests/material_gpu_tests.cpp`

**Changes:**
```cpp
class MaterialGPU {
public:
    MaterialGPU(
        dx12::Device* device, 
        const assets::Material* material,
        graphics::texture::TextureManager* textureManager); // NEW param
    
    // Bind material textures to command list
    void bindTextures(ID3D12GraphicsCommandList* commandList);
    
private:
    graphics::texture::TextureHandle m_baseColorTexture = INVALID_TEXTURE_HANDLE;
    graphics::texture::TextureHandle m_metallicRoughnessTexture = INVALID_TEXTURE_HANDLE;
    graphics::texture::TextureHandle m_normalTexture = INVALID_TEXTURE_HANDLE;
    graphics::texture::TextureHandle m_emissiveTexture = INVALID_TEXTURE_HANDLE;
    
    graphics::texture::TextureManager* m_textureManager = nullptr;
};
```

**Implementation:**
```cpp
void MaterialGPU::bindTextures(ID3D12GraphicsCommandList* commandList) {
    if (!m_textureManager) return;
    
    // Get SRV indices for material textures
    uint32_t srvIndices[4] = {
        m_textureManager->getSrvIndex(m_baseColorTexture),
        m_textureManager->getSrvIndex(m_normalTexture),
        m_textureManager->getSrvIndex(m_metallicRoughnessTexture),
        m_textureManager->getSrvIndex(m_emissiveTexture)
    };
    
    // Set descriptor heap
    ID3D12DescriptorHeap* heaps[] = { m_textureManager->getSrvHeap() };
    commandList->SetDescriptorHeaps(1, heaps);
    
    // Bind descriptor table to root parameter [2]
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = 
        m_textureManager->getSrvHeap()->GetGPUDescriptorHandleForHeapStart();
    gpuHandle.ptr += srvIndices[0] * descriptorSize; // Offset to first texture
    
    commandList->SetGraphicsRootDescriptorTable(2, gpuHandle);
}
```

**Test Cases:**
- Create MaterialGPU with textures → stores valid handles
- Bind textures to command list → sets descriptor heap and table
- Render with textured material → displays texture correctly

#### **Task 5.2: Integrate Texture Binding in MeshRenderingSystem**
**Files:**
- `src/runtime/mesh_rendering_system.cpp`
- `tests/mesh_rendering_system_tests.cpp`

**Changes:**
```cpp
void MeshRenderingSystem::renderEntity(...) {
    // Existing code: setup PSO, bind constants
    
    // NEW: Bind material textures
    if (materialGPU) {
        materialGPU->bindTextures(commandList);
    }
    
    // Existing code: draw indexed instanced
}
```

**Test Cases:**
- Render mesh with textured material → displays texture
- Render mesh without texture → uses material color
- Render multiple meshes with different textures → each uses correct texture

---

### 🔹 Phase 6: Testing and Validation

#### **Task 6.1: Create Test Assets**
**Assets Needed:**
- Simple glTF file with single textured cube (e.g., `test_textured_cube.gltf`)
- Test textures: 256x256 PNG files (checkerboard, solid colors)
- Place in `assets/test/` directory

**Test Scenarios:**
1. Load glTF with external PNG texture
2. Load glTF with embedded data URI texture
3. Load glTF with missing texture (graceful fallback)
4. Render textured mesh in viewport

#### **Task 6.2: Integration Tests**
**Files:**
- `tests/texture_integration_tests.cpp`

**Test Cases:**
- End-to-end: Load glTF → Load textures → Render → Verify GPU state
- Multiple materials with shared textures → texture cached correctly
- Texture release after scene unload → SRV slots freed

#### **Task 6.3: Performance Validation**
**Metrics:**
- Texture load time: <100ms for 2048x2048 PNG
- SRV allocation: O(1) time complexity
- Memory usage: Track GPU heap size
- Render overhead: <1ms per textured mesh

---

## 🔧 Implementation Strategy

### TDD Workflow (Per Task)
1. **Red:** Write failing unit test for atomic functionality
2. **Green:** Implement minimal code to pass test
3. **Refactor:** Clean up, optimize, add comments
4. **Repeat:** Move to next atomic functionality

### Suggested Task Order
1. **Phase 1** (Image Loading) → Foundation, no GPU dependencies
2. **Phase 2** (GPU Resources) → Builds on Phase 1, testable in isolation
3. **Phase 4** (Shaders) → Can test with hardcoded textures
4. **Phase 3** (glTF Integration) → Connects data pipeline
5. **Phase 5** (Rendering) → Final integration
6. **Phase 6** (Validation) → Polish and optimization

### Parallelization Opportunities
- **Phase 1 & 2** can be developed simultaneously (different developers)
- **Phase 4** (shaders) can proceed independently once root signature is designed
- **Phase 3** depends on Phase 1 (TextureLoader API)

---

## 🎯 Success Criteria

### Must-Have (MVP)
- [x] Load PNG/JPEG textures from glTF files
- [x] Upload textures to GPU and create SRVs
- [x] Bind textures to shaders via root signature
- [x] Render textured meshes in viewport
- [x] Handle missing textures gracefully (white default)

### Nice-to-Have (Future)
- [ ] Mipmap generation
- [ ] Compressed texture formats (BC1/BC3/BC7)
- [ ] Texture streaming for large assets
- [ ] Async texture loading (loading screen support)
- [ ] Texture hot-reload (for artist iteration)

---

## 🚧 Known Challenges and Mitigations

### Challenge 1: Bindless Descriptor Indexing
**Issue:** Shader expects SRV indices, but D3D12 requires GPU handles  
**Mitigation:** Use descriptor table with contiguous SRV range, index by offset

### Challenge 2: Texture Upload Synchronization
**Issue:** CPU→GPU copy requires command list execution and fence wait  
**Mitigation:** Batch texture uploads, execute once before first frame

### Challenge 3: glTF Base Path Resolution
**Issue:** Texture paths can be relative, absolute, or data URIs  
**Mitigation:** Normalize paths in GLTFLoader, support all three modes

### Challenge 4: Descriptor Heap Limit
**Issue:** 4096 textures might not be enough for large scenes  
**Mitigation:** Start with 4096, add virtual texturing in M4 if needed

### Challenge 5: Root Signature Compatibility
**Issue:** Changing root signature breaks existing PSOs  
**Mitigation:** Update MaterialInstance and MeshRenderingSystem in lockstep

---

## 📊 Estimated Timeline

| Phase | Tasks | Estimated Effort | Dependencies |
|-------|-------|------------------|--------------|
| Phase 1: Image Loading | 2 tasks | 1-2 days | None |
| Phase 2: GPU Resources | 3 tasks | 3-4 days | Phase 1 |
| Phase 3: glTF Integration | 2 tasks | 1-2 days | Phase 1 |
| Phase 4: Shaders | 2 tasks | 1-2 days | None (parallel) |
| Phase 5: Rendering | 2 tasks | 2-3 days | Phases 2,3,4 |
| Phase 6: Testing | 3 tasks | 1-2 days | Phase 5 |
| **Total** | **14 tasks** | **9-15 days** | - |

**Notes:** Assumes single developer, full-time work, with testing overhead

---

## 📝 Future Enhancements (Post-MVP)

### Milestone 4 Candidates
1. **Texture Compression:** BC1/BC3/BC7 support via DirectXTex
2. **Mipmap Generation:** Automatic mip chain generation on load
3. **Texture Streaming:** Virtual texturing for large scenes (>10GB textures)
4. **Sampler Variety:** Point, Linear, Anisotropic, Border modes
5. **Texture Hot-Reload:** File watcher + GPU resource update
6. **Async Loading:** Background thread for texture decoding
7. **Texture Arrays:** Bindless texture array for fewer descriptor tables
8. **sRGB Handling:** Proper gamma correction for albedo textures

---

## 🔗 Dependencies and References

### External Libraries
- **stb_image:** Public domain image loader (already in project)
- **DirectXTex:** Alternative for advanced features (not required for MVP)

### API Documentation
- [D3D12 Descriptor Heaps](https://learn.microsoft.com/en-us/windows/win32/direct3d12/descriptor-heaps)
- [glTF 2.0 Texture Specification](https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#textures)
- [stb_image GitHub](https://github.com/nothings/stb)

### Code References
- `dx12_texture.cpp` - Existing texture creation patterns
- `material_gpu.cpp` - Material resource binding patterns
- `mesh_rendering_system.cpp` - Root signature and PSO management

---

## ✅ Acceptance Checklist

Before marking this feature complete:
- [ ] All unit tests pass (texture loading, GPU resources, descriptors)
- [ ] Integration tests pass (end-to-end glTF texture rendering)
- [ ] Load and render glTF sample models (BoomBox, DamagedHelmet)
- [ ] No D3D12 validation errors or warnings
- [ ] No memory leaks (verify with PIX or D3D12 debug layer)
- [ ] Texture caching works (same texture loaded once)
- [ ] Missing textures handled gracefully (white fallback)
- [ ] Code reviewed and documented
- [ ] Performance metrics meet targets

---

## 📌 Notes

- **Start Simple:** Begin with single-texture materials, expand to multi-texture later
- **Test Early:** Write unit tests before implementing complex systems
- **Incremental Integration:** Test each phase independently before combining
- **Use PIX:** GPU debugging tool invaluable for descriptor issues
- **Reference Assets:** Use glTF sample models for validation

---

## 📚 Appendix A: Bindless Texture Implementation Options

This appendix documents the two main approaches to texture binding in D3D12. The main plan uses **Option 2 (Traditional)** for simplicity, but this information is preserved for future optimization to **Option 1 (True Bindless)**.

### **Option 1: True Bindless - Texture Indices in Root Constants**

This is the high-performance approach used by modern engines for minimal material-switching overhead.

**Root Signature:**
```cpp
// In MeshRenderingSystem::MeshRenderingSystem()
[0] CBV: Frame constants (b0)
[1] ROOT_CONSTANTS: Object constants (b1, 16 DWORDs)
[2] ROOT_CONSTANTS: Material texture indices (b2, 4 DWORDs) ← NEW
[3] DESCRIPTOR_TABLE: All textures (t0-t4095, space0) ← Set once per frame
[4] STATIC_SAMPLER: Linear wrap sampler (s0)

D3D12_DESCRIPTOR_RANGE srvRange = {};
srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
srvRange.NumDescriptors = 4096; // ENTIRE texture heap
srvRange.BaseShaderRegister = 0;
srvRange.RegisterSpace = 0;
srvRange.OffsetInDescriptorsFromTableStart = 0;

D3D12_ROOT_PARAMETER srvParameter = {};
srvParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
srvParameter.DescriptorTable.NumDescriptorRanges = 1;
srvParameter.DescriptorTable.pDescriptorRanges = &srvRange;
srvParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

D3D12_ROOT_PARAMETER indicesParameter = {};
indicesParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
indicesParameter.Constants.ShaderRegister = 2;
indicesParameter.Constants.RegisterSpace = 0;
indicesParameter.Constants.Num32BitValues = 4; // 4 texture indices
indicesParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
```

**Shader Code:**
```hlsl
// Unbounded texture array (SM 5.1+)
Texture2D textures[] : register(t0, space0);
SamplerState textureSampler : register(s0);

// Material texture indices passed as root constants
cbuffer MaterialTextureIndices : register(b2) {
    uint g_BaseColorIndex;
    uint g_NormalIndex;
    uint g_MetallicRoughnessIndex;
    uint g_EmissiveIndex;
};

// Pixel shader samples by dynamic index
PSOutput main(VSOutput input) {
    PSOutput output;
    
    // Sample textures using material-specific indices
    float4 baseColor = textures[g_BaseColorIndex].Sample(textureSampler, input.texCoord);
    baseColor *= g_MaterialConstants.baseColorFactor;
    baseColor *= input.color;
    
    output.color = baseColor;
    return output;
}
```

**Material Binding (Fast Path):**
```cpp
class MaterialGPU {
public:
    void bindTextures(ID3D12GraphicsCommandList* commandList) {
        // Just update root constants - NO descriptor table binding!
        uint32_t textureIndices[4] = {
            m_textureManager->getSrvIndex(m_baseColorTexture),
            m_textureManager->getSrvIndex(m_normalTexture),
            m_textureManager->getSrvIndex(m_metallicRoughnessTexture),
            m_textureManager->getSrvIndex(m_emissiveTexture)
        };
        
        commandList->SetGraphicsRoot32BitConstants(2, 4, textureIndices, 0);
    }
};
```

**Frame Setup (Once):**
```cpp
void MeshRenderingSystem::render(...) {
    // Set descriptor heap ONCE for entire frame
    ID3D12DescriptorHeap* heaps[] = { m_textureManager->getSrvHeap() };
    commandList->SetDescriptorHeaps(1, heaps);
    
    // Bind entire texture array ONCE
    D3D12_GPU_DESCRIPTOR_HANDLE heapStart = 
        m_textureManager->getSrvHeap()->GetGPUDescriptorHandleForHeapStart();
    commandList->SetGraphicsRootDescriptorTable(3, heapStart);
    
    // Render all entities (materials just update root constants)
    for (auto entity : entities) {
        materialGPU->bindTextures(commandList); // Fast: just 4 integers
        // ... draw
    }
}
```

**Advantages:**
- ✅ **Zero overhead** for material switching (~10 CPU cycles vs ~100)
- ✅ Descriptor table set **once per frame**, not per material
- ✅ Scales to thousands of unique textures efficiently
- ✅ Matches modern engine architectures (Unreal 5, Unity HDRP)

**Disadvantages:**
- ❌ Requires **Shader Model 5.1+** and **Resource Binding Tier 2**
- ❌ More complex to implement and debug
- ❌ Invalid texture indices cause silent failures (need validation)
- ❌ Validation layer slower due to unbounded array tracking

**Hardware Requirements Check:**
```cpp
// Check for bindless support
D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));

if (options.ResourceBindingTier >= D3D12_RESOURCE_BINDING_TIER_2) {
    // Can use unbounded arrays
    console::info("Bindless textures supported (Tier 2+)");
} else {
    // Fall back to Option 2
    console::warning("Bindless textures not supported, using descriptor tables per material");
}
```

---

### **Option 2: Traditional - Descriptor Table Per Material (Current Plan)**

This is the simpler approach used in the main implementation plan.

**Root Signature:**
```cpp
[0] CBV: Frame constants (b0)
[1] ROOT_CONSTANTS: Object constants (b1, 16 DWORDs)
[2] DESCRIPTOR_TABLE: This material's 4 textures (t0-t3, space0) ← Per material
[3] STATIC_SAMPLER: Linear wrap sampler (s0)

D3D12_DESCRIPTOR_RANGE srvRange = {};
srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
srvRange.NumDescriptors = 4; // Just this material's textures
srvRange.BaseShaderRegister = 0;
srvRange.RegisterSpace = 0;
srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
```

**Shader Code:**
```hlsl
// Fixed texture slots (traditional)
Texture2D baseColorTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D metallicRoughnessTexture : register(t2);
Texture2D emissiveTexture : register(t3);
SamplerState textureSampler : register(s0);

PSOutput main(VSOutput input) {
    PSOutput output;
    
    // Direct texture sampling
    float4 baseColor = baseColorTexture.Sample(textureSampler, input.texCoord);
    baseColor *= g_MaterialConstants.baseColorFactor;
    
    output.color = baseColor;
    return output;
}
```

**Material Binding:**
```cpp
void MaterialGPU::bindTextures(ID3D12GraphicsCommandList* commandList) {
    // Set descriptor heap
    ID3D12DescriptorHeap* heaps[] = { m_textureManager->getSrvHeap() };
    commandList->SetDescriptorHeaps(1, heaps);
    
    // Get GPU handle for this material's textures
    uint32_t srvIndex = m_textureManager->getSrvIndex(m_baseColorTexture);
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = 
        m_textureManager->getSrvHeap()->GetGPUDescriptorHandleForHeapStart();
    gpuHandle.ptr += srvIndex * m_descriptorSize;
    
    // Bind descriptor table (one call per material)
    commandList->SetGraphicsRootDescriptorTable(2, gpuHandle);
}
```

**Advantages:**
- ✅ **Simple to implement** and debug
- ✅ Works on **all D3D12 hardware** (no tier requirements)
- ✅ Easier to reason about resource lifetimes
- ✅ Better for tools like PIX (descriptor tables are explicit)
- ✅ Validated by D3D12 debug layer more thoroughly

**Disadvantages:**
- ❌ `SetGraphicsRootDescriptorTable()` overhead per material
- ❌ Descriptor heap may need to be set multiple times
- ❌ Limited to small descriptor ranges (typically 4-16 textures per material)
- ❌ Requires contiguous descriptor allocation per material

---

### **Performance Comparison**

| Metric | Option 1 (Bindless) | Option 2 (Per-Material) |
|--------|---------------------|-------------------------|
| Material switch cost | ~10 CPU cycles | ~100 CPU cycles |
| Descriptor table bindings | 1 per frame | 1 per material |
| Heap bindings | 1 per frame | 1+ per frame |
| Texture limit per material | 4096 | 4-16 (practical) |
| Hardware requirement | Tier 2+ (2016+) | Tier 1 (all D3D12) |
| Implementation complexity | High | Low |
| Debug/validation speed | Slower | Faster |

**When to use each:**
- **Option 1** if you have 100+ materials per frame and need maximum performance
- **Option 2** if you're starting out, have <50 materials, or need broad hardware support

---

### **Migration Path: Option 2 → Option 1**

The document's implementation plan uses Option 2 initially. To migrate to Option 1 later:

**Phase 1: Infrastructure (Option 2)**
1. Implement TextureLoader ✓
2. Implement TextureManager ✓
3. Implement BindlessTextureHeap ✓
4. Implement MaterialGPU texture binding ✓

**Phase 2: Runtime Check (Option 2 + Fallback)**
5. Add hardware tier detection
6. Select binding strategy at runtime
7. Add validation for texture indices

**Phase 3: Bindless Optimization (Option 1)**
8. Add texture indices to MaterialConstants cbuffer
9. Update root signature to use unbounded array + root constants
10. Modify shaders to use dynamic indexing
11. Update MaterialGPU::bindTextures() to use SetGraphicsRoot32BitConstants()
12. Remove per-material descriptor table bindings
13. Add single descriptor table binding in frame setup

**Estimated effort:** 1-2 days if Option 2 is already working

---

### **Code Examples: Full Implementation**

**Option 1 - Complete Material Binding:**
```cpp
// MaterialGPU.h
class MaterialGPU {
public:
    void bindTextures(ID3D12GraphicsCommandList* commandList) {
        uint32_t indices[4] = {
            m_textureManager->getSrvIndex(m_baseColorTexture),
            m_textureManager->getSrvIndex(m_normalTexture),
            m_textureManager->getSrvIndex(m_metallicRoughnessTexture),
            m_textureManager->getSrvIndex(m_emissiveTexture)
        };
        commandList->SetGraphicsRoot32BitConstants(2, 4, indices, 0);
    }
};

// MeshRenderingSystem.cpp - Frame setup
void MeshRenderingSystem::render(...) {
    // ONCE per frame
    ID3D12DescriptorHeap* heaps[] = { m_textureManager->getSrvHeap() };
    commandList->SetDescriptorHeaps(1, heaps);
    commandList->SetGraphicsRootDescriptorTable(3, 
        m_textureManager->getSrvHeap()->GetGPUDescriptorHandleForHeapStart());
    
    // Per entity - just constants
    for (auto entity : entities) {
        materialGPU->bindTextures(commandList); // Fast!
        primitiveGPU->draw(commandList);
    }
}
```

**Option 2 - Complete Material Binding (Current Plan):**
```cpp
// MaterialGPU.h
class MaterialGPU {
public:
    void bindTextures(ID3D12GraphicsCommandList* commandList) {
        ID3D12DescriptorHeap* heaps[] = { m_textureManager->getSrvHeap() };
        commandList->SetDescriptorHeaps(1, heaps);
        
        uint32_t baseIndex = m_textureManager->getSrvIndex(m_baseColorTexture);
        D3D12_GPU_DESCRIPTOR_HANDLE handle = 
            m_textureManager->getSrvHeap()->GetGPUDescriptorHandleForHeapStart();
        handle.ptr += baseIndex * m_descriptorSize;
        
        commandList->SetGraphicsRootDescriptorTable(2, handle);
    }
};

// MeshRenderingSystem.cpp - Per entity
void MeshRenderingSystem::renderEntity(...) {
    materialGPU->bindTextures(commandList); // Per material overhead
    primitiveGPU->draw(commandList);
}
```

---

### **Recommendation**

**Start with Option 2** (current plan) because:
1. Get textures working quickly with minimal complexity
2. Works on all hardware without feature checks
3. Easy to debug and validate
4. Proven pattern in many production engines

**Migrate to Option 1** in Milestone 4 if you observe:
- Material switching is a bottleneck (profile with PIX)
- Need to support 1000+ unique materials per frame
- Target hardware supports Tier 2+ (post-2016 GPUs)

The infrastructure (TextureManager, BindlessTextureHeap) supports both approaches equally well.

---

**End of Plan**
