# Reflection-Based Root Signature Generation - Implementation Plan

**Date:** October 13, 2025  
**Branch:** 001-data-driven-material  
**Status:** In Progress - Phase 2

---

## Architecture Overview

**Core Principle:** Root signatures are derived from `MaterialPass` (not `MaterialDefinition`) via shader reflection. Parameters go into `MaterialConstants` CBV, not individual bindings.

**Key Changes:**
1. `RootSignatureBuilder::Build` accepts `MaterialPass` + `ShaderManager*`
2. Shader bytecode is reflected to extract all resource bindings
3. Reflection results are cached by bytecode hash (supports hot-reload)
4. Descriptor tables for textures/samplers (bindless-ready architecture)

---

## Phase 1: Shader Reflection Infrastructure

### **Task 1.1: Add Reflection Helper Module** ✅ COMPLETE
**File:** `src/graphics/material_system/shader_reflection.h/cpp`

**Key Functions:**
```cpp
namespace graphics::material_system
{

// Reflection result for a single shader
struct ShaderResourceBindings
{
    std::vector<ResourceBinding> bindings;
    bool success = false;
};

class ShaderReflection
{
public:
    // Reflect on compiled shader bytecode
    // Returns all resource bindings (CBV, SRV, UAV, Sampler)
    static ShaderResourceBindings Reflect(const shader_manager::ShaderBlob* blob);
    
private:
    // Map D3D shader input type to our enum
    static ResourceBindingType MapBindingType(D3D_SHADER_INPUT_TYPE d3dType);
};

}
```

**Implementation Details:**
- Use `D3DReflect()` from `<d3dcompiler.h>` on `blob->blob`
- Iterate `ID3D12ShaderReflection::GetResourceBindingDesc()` for all bound resources
- Extract: name, type (CBV/SRV/UAV/Sampler), register slot, space
- Return structured `ResourceBinding` list

**Mapping:**
```cpp
D3D_SIT_CBUFFER    → ResourceBindingType::CBV
D3D_SIT_TEXTURE    → ResourceBindingType::SRV
D3D_SIT_SAMPLER    → ResourceBindingType::Sampler
D3D_SIT_UAV_*      → ResourceBindingType::UAV
D3D_SIT_STRUCTURED → ResourceBindingType::SRV (structured buffer)
D3D_SIT_BYTEADDRESS → ResourceBindingType::SRV (byte address buffer)
```

---

### **Task 1.2: Implement Reflection Cache** ✅ COMPLETE
**File:** `src/graphics/material_system/shader_reflection.h/cpp` (extend)

**Key Structures:**
```cpp
class ShaderReflectionCache
{
public:
    // Get cached reflection or perform reflection + cache
    // Uses bytecode hash as key (supports hot-reload)
    ShaderResourceBindings GetOrReflect(
        const shader_manager::ShaderBlob* blob,
        shader_manager::ShaderHandle handle);
    
    // Invalidate cache entry (called on shader hot-reload)
    void Invalidate(shader_manager::ShaderHandle handle);
    
    // Clear entire cache
    void Clear();
    
private:
    // Cache key: hash of bytecode content
    struct CacheKey
    {
        size_t bytecodeHash;
        
        bool operator==(const CacheKey& other) const {
            return bytecodeHash == other.bytecodeHash;
        }
    };
    
    struct CacheKeyHash {
        size_t operator()(const CacheKey& key) const {
            return key.bytecodeHash;
        }
    };
    
    std::unordered_map<CacheKey, ShaderResourceBindings, CacheKeyHash> m_cache;
    
    // Compute hash of bytecode bytes
    static size_t HashBytecode(const shader_manager::ShaderBlob* blob);
};
```

**Hash Computation:**
```cpp
size_t HashBytecode(const ShaderBlob* blob) {
    // Hash the actual compiled bytes
    const void* data = blob->blob->GetBufferPointer();
    size_t size = blob->blob->GetBufferSize();
    
    // Use std::hash or FNV-1a/xxHash for speed
    // Example with simple std::hash (production: use better hasher)
    return std::hash<std::string_view>{}(
        std::string_view(static_cast<const char*>(data), size)
    );
}
```

**Why bytecode hash:** When shader hot-reloads, new bytecode → new hash → cache miss → re-reflect. Perfect for development workflow.

---

## Phase 2: Refactor RootSignatureBuilder

### **Task 2.1: Change Build() Signature**
**File:** `src/graphics/material_system/root_signature_builder.h/cpp`

**Before:**
```cpp
static RootSignatureSpec Build(
    const MaterialDefinition& material,
    bool includeFrameConstants = false,
    bool includeObjectConstants = false,
    bool includeMaterialConstants = false);
```

**After:**
```cpp
static RootSignatureSpec Build(
    const MaterialPass& pass,
    shader_manager::ShaderManager* shaderManager,
    ShaderReflectionCache* reflectionCache);
```

**Rationale:**
- Pass determines shaders → determines resources
- ShaderManager provides compiled blobs
- ReflectionCache avoids redundant reflection

---

### **Task 2.2: Implement New Build() Logic**
**Pseudocode Flow:**
```cpp
RootSignatureSpec Build(pass, shaderManager, reflectionCache)
{
    spec = RootSignatureSpec{};
    
    // Step 1: Reflect all shaders in the pass
    for (shaderRef : pass.shaders) {
        // Register shader if not already (idempotent)
        handle = shaderManager->registerShader(
            shaderRef.file, shaderRef.entryPoint, shaderRef.profile, mapShaderStage(shaderRef.stage));
        
        blob = shaderManager->getShaderBlob(handle);
        if (!blob || !blob->isValid()) {
            error("Shader compilation failed");
            continue;
        }
        
        // Reflect (cached)
        bindings = reflectionCache->GetOrReflect(blob, handle);
        spec.resourceBindings.insert(bindings);
    }
    
    // Step 2: Merge bindings across shaders
    MergeAndValidateBindings(spec.resourceBindings);
    
    // Step 3: Group bindings by type for root signature layout
    GroupBindingsForRootSignature(spec);
    
    return spec;
}
```

---

### **Task 2.3: Merge & Validate Bindings**
**File:** `root_signature_builder.cpp` (new private method)

```cpp
void MergeAndValidateBindings(std::vector<ResourceBinding>& bindings)
{
    // Track seen bindings: name → (type, slot)
    std::unordered_map<std::string, ResourceBinding> uniqueBindings;
    
    for (const auto& binding : bindings) {
        auto it = uniqueBindings.find(binding.name);
        
        if (it != uniqueBindings.end()) {
            // Duplicate name - validate consistency
            if (it->second.type != binding.type) {
                console::errorAndThrow(
                    "Binding '{}' has conflicting types: {} vs {}",
                    binding.name, it->second.type, binding.type);
            }
            if (it->second.slot != binding.slot) {
                console::errorAndThrow(
                    "Binding '{}' has conflicting slots: b{} vs b{}",
                    binding.name, it->second.slot, binding.slot);
            }
            // Same binding in multiple shaders (e.g., VS and PS both use FrameConstants) - OK
        } else {
            uniqueBindings[binding.name] = binding;
        }
    }
    
    // Replace with de-duplicated list
    bindings.clear();
    for (const auto& [name, binding] : uniqueBindings) {
        bindings.push_back(binding);
    }
}
```

---

### **Task 2.4: Group Bindings for Root Signature Layout**
**Rationale:** Efficient root signature uses descriptor tables for textures/samplers.

**New Structure:**
```cpp
struct RootSignatureSpec
{
    // Root descriptors (2 DWORDS each)
    std::vector<ResourceBinding> cbvRootDescriptors;  // b0, b1, b2, etc.
    
    // Descriptor tables (1 DWORD each)
    struct DescriptorTable {
        ResourceBindingType type;  // SRV or Sampler
        std::vector<ResourceBinding> bindings;  // All SRVs t0-tN or all Samplers s0-sN
    };
    std::vector<DescriptorTable> descriptorTables;
};
```

**Grouping Logic:**
```cpp
void GroupBindingsForRootSignature(RootSignatureSpec& spec)
{
    std::vector<ResourceBinding> srvBindings;
    std::vector<ResourceBinding> samplerBindings;
    
    for (const auto& binding : spec.resourceBindings) {
        switch (binding.type) {
            case ResourceBindingType::CBV:
                // CBVs as root descriptors (FrameConstants, ObjectConstants, MaterialConstants)
                spec.cbvRootDescriptors.push_back(binding);
                break;
                
            case ResourceBindingType::SRV:
                // Group all SRVs into single descriptor table
                srvBindings.push_back(binding);
                break;
                
            case ResourceBindingType::Sampler:
                // Group all Samplers into single descriptor table
                samplerBindings.push_back(binding);
                break;
                
            case ResourceBindingType::UAV:
                // For now, error (compute shaders future work)
                console::error("UAV bindings not yet supported");
                break;
        }
    }
    
    // Create descriptor tables if needed
    if (!srvBindings.empty()) {
        spec.descriptorTables.push_back({ResourceBindingType::SRV, srvBindings});
    }
    if (!samplerBindings.empty()) {
        spec.descriptorTables.push_back({ResourceBindingType::Sampler, samplerBindings});
    }
    
    // Sort for deterministic hashing
    std::sort(spec.cbvRootDescriptors.begin(), spec.cbvRootDescriptors.end());
    for (auto& table : spec.descriptorTables) {
        std::sort(table.bindings.begin(), table.bindings.end());
    }
}
```

**Root Signature Layout Example:**
```
Root Parameter 0: CBV (FrameConstants) - b0
Root Parameter 1: CBV (ObjectConstants) - b1  
Root Parameter 2: CBV (MaterialConstants) - b2
Root Parameter 3: Descriptor Table (SRV) - [t0: baseColorTexture, t1: normalTexture, t2: metallicRoughness]
Root Parameter 4: Descriptor Table (Sampler) - [s0: linearSampler, s1: pointSampler]
```

---

## Phase 3: Update RootSignatureCache

### **Task 3.1: Extend buildRootSignature() for Descriptor Tables**
**File:** `src/graphics/material_system/root_signature_cache.cpp`

**Current code only handles root descriptors.** Need to add descriptor table support:

```cpp
Microsoft::WRL::ComPtr<ID3D12RootSignature> buildRootSignature(
    dx12::Device* device,
    const RootSignatureSpec& spec)
{
    std::vector<D3D12_ROOT_PARAMETER> rootParameters;
    
    // Storage for descriptor ranges (must persist until serialization)
    std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> tableRanges;
    
    // Add root descriptor CBVs
    for (const auto& binding : spec.cbvRootDescriptors) {
        D3D12_ROOT_PARAMETER param = {};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        param.Descriptor.ShaderRegister = binding.slot;
        param.Descriptor.RegisterSpace = 0;
        param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters.push_back(param);
    }
    
    // Add descriptor tables
    for (const auto& table : spec.descriptorTables) {
        std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
        
        for (const auto& binding : table.bindings) {
            D3D12_DESCRIPTOR_RANGE range = {};
            range.RangeType = (table.type == ResourceBindingType::SRV) 
                ? D3D12_DESCRIPTOR_RANGE_TYPE_SRV 
                : D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            range.NumDescriptors = 1;
            range.BaseShaderRegister = binding.slot;
            range.RegisterSpace = 0;
            range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
            ranges.push_back(range);
        }
        
        tableRanges.push_back(ranges);
        
        D3D12_ROOT_PARAMETER param = {};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param.DescriptorTable.NumDescriptorRanges = ranges.size();
        param.DescriptorTable.pDescriptorRanges = tableRanges.back().data();
        param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters.push_back(param);
    }
    
    // Serialize and create (existing code continues...)
}
```

**Key Points:**
- Descriptor ranges must persist until `D3D12SerializeRootSignature` completes
- Use `tableRanges` vector to keep ranges alive
- Each descriptor table can have multiple ranges (we use 1 range per table for simplicity)

---

### **Task 3.2: Update Hash Computation**
**File:** `root_signature_cache.cpp`

**Current hash only considers old `resourceBindings`.** Update to hash new structure:

```cpp
size_t computeHash(const RootSignatureSpec& spec)
{
    size_t hash = 0;
    
    // Hash root descriptors
    for (const auto& binding : spec.cbvRootDescriptors) {
        hashCombine(hash, binding.name);
        hashCombine(hash, static_cast<int>(binding.type));
        hashCombine(hash, binding.slot);
    }
    
    // Hash descriptor tables
    for (const auto& table : spec.descriptorTables) {
        hashCombine(hash, static_cast<int>(table.type));
        for (const auto& binding : table.bindings) {
            hashCombine(hash, binding.name);
            hashCombine(hash, binding.slot);
        }
    }
    
    return hash;
}
```

---

## Phase 4: Update PSOBuilder Integration

### **Task 4.1: Change PSOBuilder::build() Call Site** ✅ COMPLETE
**File:** `src/graphics/material_system/pso_builder.cpp`

**Before:**
```cpp
// Old approach - material-level root signature
auto rootSigSpec = RootSignatureBuilder::Build(material, true, true, true);
auto rootSig = rootSigCache->getOrCreate(device, rootSigSpec);
```

**After:**
```cpp
// New approach - pass-level root signature via reflection
const MaterialPass* pass = material.getPass(passName);
if (!pass) {
    console::error("Pass '{}' not found in material '{}'", passName, material.id);
    return nullptr;
}

auto rootSigSpec = RootSignatureBuilder::Build(
    *pass, 
    shaderManager,      // Need to pass this in
    reflectionCache);   // Need to pass this in

auto rootSig = rootSigCache->getOrCreate(device, rootSigSpec);
```

**Required:** `PSOBuilder::build()` needs new parameters:
```cpp
static ComPtr<ID3D12PipelineState> build(
    dx12::Device* device,
    const MaterialDefinition& material,
    const RenderPassConfig& passConfig,
    const MaterialSystem* materialSystem,
    const std::string& passName,
    shader_manager::ShaderManager* shaderManager,      // NEW
    ShaderReflectionCache* reflectionCache);           // NEW
```

---

### **Task 4.2: Remove AddParameterBindings**
**File:** `root_signature_builder.cpp`

**Delete entirely:**
- `AddParameterBindings()` method
- Related logic in `Build()`

**Rationale:** Parameters are NOT bindings—they're data for the `MaterialConstants` CBV. Reflection detects if shader uses `cbuffer MaterialConstants : register(b2)` and includes it automatically.

---

## Phase 5: MaterialSystem Integration

### **Task 5.1: Add ShaderReflectionCache to MaterialSystem** ✅ COMPLETE
**File:** `src/graphics/material_system/material_system.h/cpp`

```cpp
class MaterialSystem
{
private:
    shader_manager::ShaderManager* m_shaderManager;  // Already exists?
    ShaderReflectionCache m_reflectionCache;         // NEW
    
public:
    // Expose for PSOBuilder
    ShaderReflectionCache* getReflectionCache() { return &m_reflectionCache; }
    shader_manager::ShaderManager* getShaderManager() { return m_shaderManager; }
};
```

---

### **Task 5.2: Register Shader Hot-Reload Callback** ✅ COMPLETE
**File:** `material_system.cpp` (initialization)

```cpp
void MaterialSystem::initialize(dx12::Device* device, shader_manager::ShaderManager* shaderManager)
{
    m_shaderManager = shaderManager;
    
    // Register callback to invalidate reflection cache on shader reload
    m_shaderManager->registerReloadCallback(
        [this](shader_manager::ShaderHandle handle, const shader_manager::ShaderBlob&) {
            // Shader recompiled - invalidate cached reflection
            m_reflectionCache.Invalidate(handle);
            
            // TODO: Also invalidate PSO cache for materials using this shader
        });
}
```

**Impact:** When shader hot-reloads → reflection cache cleared → next PSO build re-reflects → correct root signature for new shader.

---

## Phase 6: Testing Strategy

### **Task 6.1: Unit Tests for Reflection** ✅ COMPLETE
**File:** `tests/material_system_tests.cpp`

```cpp
TEST_CASE("ShaderReflection extracts CBV bindings", "[reflection][unit]") {
    // Arrange: Compile simple shader with known CBV
    // shaders/test_reflection.hlsl:
    //   cbuffer FrameConstants : register(b0) { float4x4 viewProj; }
    
    // Act: Reflect
    auto bindings = ShaderReflection::Reflect(blob);
    
    // Assert
    REQUIRE(bindings.success);
    REQUIRE(bindings.bindings.size() == 1);
    REQUIRE(bindings.bindings[0].name == "FrameConstants");
    REQUIRE(bindings.bindings[0].type == ResourceBindingType::CBV);
    REQUIRE(bindings.bindings[0].slot == 0);
}

TEST_CASE("ShaderReflection extracts SRV and Sampler", "[reflection][unit]") {
    // Shader: Texture2D tex : register(t0); SamplerState samp : register(s0);
    // Verify correct types and slots
}

TEST_CASE("RootSignatureBuilder merges bindings from VS+PS", "[reflection][integration]") {
    // Pass with VS (uses FrameConstants b0) and PS (uses FrameConstants b0 + MaterialConstants b2)
    // Verify merged spec has both, no duplicates
}
```

---

### **Task 6.2: Integration Test with Real Material** ✅ COMPLETE
```cpp
TEST_CASE("PSOBuilder creates PSO with reflection-based root signature", "[pso][integration]") {
    // Arrange: Material with forward pass using unlit.hlsl
    // unlit.hlsl declares: FrameConstants b0, ObjectConstants b1, MaterialConstants b2
    
    // Act: Build PSO (internally uses reflection)
    auto pso = PSOBuilder::build(...);
    
    // Assert: PSO created successfully
    REQUIRE(pso != nullptr);
}
```

---

### **Task 6.3: Hot-Reload Test** ✅ COMPLETE
```cpp
TEST_CASE("Reflection cache invalidates on shader hot-reload", "[reflection][hot-reload]") {
    // 1. Build PSO with shader v1
    // 2. Modify shader file (add new SRV binding)
    // 3. Trigger ShaderManager::update()
    // 4. Verify reflection cache cleared
    // 5. Rebuild PSO - should have new binding
}
```

---

## Phase 7: Backward Compatibility & Migration

### **Task 7.1: Update Existing Tests**
**Impact:** Many tests currently call `RootSignatureBuilder::Build(material, bool, bool, bool)`.

**Migration:**
```cpp
// Old
auto spec = RootSignatureBuilder::Build(material, true, true, true);

// New
const auto* pass = material.getPass("forward");
auto spec = RootSignatureBuilder::Build(*pass, shaderManager, reflectionCache);
```

**Affected tests:** Search for `RootSignatureBuilder::Build` calls—likely ~10-15 test cases.

---

### **Task 7.2: Deprecation Path**
**Option 1:** Keep old `Build()` with `[[deprecated]]` attribute, forward to new implementation.
**Option 2:** Breaking change—update all call sites immediately (cleaner, but requires more work).

**Recommendation:** Option 2 if tests are the only consumers.

---

## Summary of Key Deliverables

| Component | File(s) | Description |
|-----------|---------|-------------|
| **Reflection** | `shader_reflection.h/cpp` | `D3DReflect()` wrapper, extracts bindings |
| **Cache** | `shader_reflection.h/cpp` | Bytecode-hash-keyed cache, hot-reload aware |
| **Builder** | `root_signature_builder.h/cpp` | Refactored to accept `MaterialPass`, calls reflection, groups bindings |
| **RootSigCache** | `root_signature_cache.cpp` | Descriptor table support, updated hash |
| **PSOBuilder** | `pso_builder.cpp` | Pass ShaderManager + cache, use pass-level root sig |
| **MaterialSystem** | `material_system.h/cpp` | Own reflection cache, register hot-reload callback |
| **Tests** | `material_system_tests.cpp` | Unit tests for reflection, integration tests for full pipeline |

---

## Estimated Complexity

| Phase | Effort | Risk |
|-------|--------|------|
| 1 (Reflection) | **High** (new D3D12 API usage) | Medium (reflection API well-documented) |
| 2 (Builder refactor) | **Medium** | Low (mostly restructuring) |
| 3 (Descriptor tables) | **High** (DX12 root sig complexity) | High (easy to get wrong, hard to debug) |
| 4 (PSOBuilder) | **Low** | Low (just pass new params) |
| 5 (MaterialSystem) | **Low** | Low (simple integration) |
| 6 (Tests) | **Medium** | Medium (need representative test shaders) |
| 7 (Migration) | **Medium** | Low (mechanical changes) |

**Total:** ~3-5 days for experienced DX12 developer, ~1-2 weeks for learning curve.

---

## Open Questions Before Implementation

1. **Bindless textures:** You mentioned wanting bindless. Should we design descriptor tables to support unbounded arrays (`Texture2D textures[]`)? This affects layout significantly.

2. **Static samplers:** Should common samplers (linear, point, anisotropic) be static samplers in root signature (no descriptor heap needed)?

3. **Descriptor heap management:** Who creates/manages the descriptor heaps for SRV/Sampler tables? MaterialInstance? Renderer?

4. **UAV support:** Do you need UAV bindings for compute shaders soon, or can we defer?

5. **Multiple register spaces:** All bindings currently use `space0`. Do you need multi-space support?

---

## Progress Tracking

- [x] Plan created
- [x] Phase 1.1: Reflection helper (COMPLETE)
- [x] Phase 1.2: Reflection cache (COMPLETE)
- [x] Phase 2: Builder refactor (COMPLETE)
- [x] Phase 3: Root signature cache updates (COMPLETE)
- [x] Phase 4.1: PSO builder integration verification (COMPLETE - already done in Phase 3)
- [ ] Phase 4.2: Remove AddParameterBindings (DEFERRED - legacy methods needed for fallback)
- [x] Phase 5.1: MaterialSystem ShaderReflectionCache (COMPLETE - done in Phase 3)
- [x] Phase 5.2: Hot-reload callback registration (COMPLETE)
- [x] Phase 6.1: Unit tests for reflection (COMPLETE - 4 Phase 2 tests)
- [x] Phase 6.2: Integration test with real material (COMPLETE)
- [x] Phase 6.3: Hot-reload test (COMPLETE)
- [ ] Phase 7: Migration (ONGOING - legacy fallback retained)
