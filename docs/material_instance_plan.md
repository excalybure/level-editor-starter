# MaterialInstance Implementation Plan

**Date:** October 12, 2025  
**Milestone:** 3 (Data-Driven Material System)  
**Phase:** 7+ (Material System Abstraction)

---

## Overview

This document outlines the plan to create a `MaterialInstance` class that abstracts material pipeline state management (PSO, root signature, hot-reload) away from individual renderer systems. This eliminates code duplication and provides a consistent, testable interface for material lifecycle management.

---

## Problem Statement

Currently, renderer systems (e.g., `GridRenderer`) manually manage:
1. **Pipeline State Objects (PSO)** - `m_pipelineState`
2. **Root Signatures** - `m_rootSignature`
3. **Material System Integration** - `m_materialSystem`, `m_materialHandle`
4. **Dirty Tracking** - `m_pipelineStateDirty`
5. **Material Pass Queries** - Getting pass definitions, configs
6. **PSO Recreation Logic** - When shaders hot-reload

This creates significant duplication across renderer systems. Each new renderer (grid, outline, gizmo, etc.) must reimplement this boilerplate.

---

## Solution: MaterialInstance Class

Create a new `graphics::material_system::MaterialInstance` class that:
- Manages **all passes** from a single material definition
- Provides `setupCommandList(passName)` for dynamic pass selection at render time
- Handles PSO caching per pass (lazy creation)
- Shares a single root signature across all passes
- Automatically handles shader hot-reload via ShaderManager callbacks

### Responsibilities

**MaterialInstance owns:**
- ✅ Pipeline state lifecycle (per pass)
- ✅ Root signature reference (shared)
- ✅ Material handle and system integration
- ✅ Pass queries and validation
- ✅ Hot-reload/dirty state management (automatic)
- ✅ PSO recreation logic

**Renderer system still owns:**
- ✅ System-specific constant buffers
- ✅ System-specific rendering logic
- ✅ Binding constant buffers to command list
- ✅ Draw calls, viewport, scissor, topology

---

## Architecture Lessons from Existing Code

From `MeshRenderingSystem` evolution and `MaterialSystem` implementation:
1. **Root Signature** should be cached and shared (via `RootSignatureCache`)
2. **Pipeline States** should be cached per material definition
3. **MaterialSystem** provides data-driven material definitions
4. **PipelineBuilder** already abstracts PSO creation from material definitions
5. Systems should only manage **renderer-specific resources** (like constant buffers)

---

## Class Design

### Location
```
src/graphics/material_system/material_instance.h
src/graphics/material_system/material_instance.cpp
```

### Interface

```cpp
namespace graphics::material_system
{

// Represents a runtime instance of a material with cached GPU resources for all passes
// Handles PSO/root signature lifecycle, hot-reloading, and multi-pass management
// Note: Does NOT manage constant buffers - that's the caller's responsibility
class MaterialInstance
{
public:
    // Create material instance from material ID
    // device: DX12 device for PSO/root signature creation
    // materialSystem: Material system for querying definitions
    // shaderManager: Optional shader manager for hot-reload registration
    // materialId: Material ID to look up (e.g., "grid_material", "pbr_material")
    MaterialInstance(
        dx12::Device* device,
        MaterialSystem* materialSystem,
        shader_manager::ShaderManager* shaderManager,
        const std::string& materialId
    );

    ~MaterialInstance();

    // No copy (manages GPU resources and callbacks)
    MaterialInstance(const MaterialInstance&) = delete;
    MaterialInstance& operator=(const MaterialInstance&) = delete;

    // Check if material instance is valid (material found with at least one pass)
    bool isValid() const;

    // Check if material has specific pass
    bool hasPass(const std::string& passName) const;

    // Get pipeline state for specific pass (recreates if dirty, returns nullptr if failed/not found)
    ID3D12PipelineState* getPipelineState(const std::string& passName);

    // Get root signature (cached, shared across all passes)
    ID3D12RootSignature* getRootSignature() const;

    // Query material definition (nullptr if invalid)
    const MaterialDefinition* getMaterial() const;

    // Query specific material pass (nullptr if not found)
    const MaterialPass* getPass(const std::string& passName) const;

    // Get material handle
    MaterialHandle getHandle() const { return m_materialHandle; }

    // Setup command list for specific pass (sets PSO and root signature)
    // passName: Which pass to render (e.g., "forward", "depth_prepass", "shadow")
    // Returns false if pass not found or PSO/root signature unavailable
    // Note: Does NOT bind constant buffers - that's the caller's responsibility
    bool setupCommandList(ID3D12GraphicsCommandList* commandList, const std::string& passName);

private:
    dx12::Device* m_device = nullptr;
    MaterialSystem* m_materialSystem = nullptr;
    shader_manager::ShaderManager* m_shaderManager = nullptr;
    MaterialHandle m_materialHandle;

    // Pipeline states per pass (lazy-created, cached)
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> m_pipelineStates;
    
    // Single root signature shared by all passes
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;

    // Dirty flags per pass (for hot-reload)
    std::unordered_set<std::string> m_dirtyPasses;

    shader_manager::CallbackHandle m_hotReloadCallback = shader_manager::INVALID_CALLBACK_HANDLE;

    // Internal PSO creation for specific pass
    bool createPipelineStateForPass(const std::string& passName);
    
    // Shader hot-reload callback (marks all passes dirty)
    void onShaderReloaded();
};

} // namespace graphics::material_system
```

---

## Implementation Tasks (TDD Workflow)

All tasks follow strict **Red → Green → Refactor** workflow per atomic functionality.

### Phase 1: Core MaterialInstance

#### **T301: Create MaterialInstance with Material Lookup**
**Objective:** Basic MaterialInstance construction and material validation

**Atomic Functionalities:**
- **AF1**: Create `material_instance.h/cpp` files with class skeleton
  - Define MaterialInstance class with constructor/destructor
  - Add member variables (device, materialSystem, materialHandle)
  - No implementation yet, just structure
  
- **AF2**: Implement constructor with material ID lookup
  - Query `MaterialHandle` from MaterialSystem using material ID
  - Store material handle if valid
  - No shader manager integration yet
  
- **AF3**: Implement `isValid()` to check material exists with at least one pass
  - Return false if material handle invalid
  - Query material definition from MaterialSystem
  - Check that material has at least one pass
  
- **AF4**: Implement `hasPass(passName)` to check specific pass availability
  - Query material definition
  - Use `MaterialDefinition::hasPass()` to check pass existence
  
- **AF5**: Add getters for `getMaterial()`, `getPass(passName)`, `getHandle()`
  - `getMaterial()`: Query from MaterialSystem using handle
  - `getPass(passName)`: Query specific pass from MaterialSystem
  - `getHandle()`: Return stored material handle

**Tests:**
```cpp
TEST_CASE("MaterialInstance with valid material ID is valid", "[material-instance][T301][unit]")
TEST_CASE("MaterialInstance with invalid material ID is invalid", "[material-instance][T301][unit]")
TEST_CASE("MaterialInstance hasPass returns true for existing pass", "[material-instance][T301][unit]")
TEST_CASE("MaterialInstance hasPass returns false for non-existing pass", "[material-instance][T301][unit]")
TEST_CASE("MaterialInstance getPass returns correct pass definition", "[material-instance][T301][unit]")
TEST_CASE("MaterialInstance getPass returns nullptr for invalid pass", "[material-instance][T301][unit]")
```

---

#### **T302: MaterialInstance Root Signature Integration**
**Objective:** Integrate root signature caching from PipelineBuilder

**Atomic Functionalities:**
- **AF1**: Call `PipelineBuilder::getRootSignature()` in constructor
  - After material handle is validated
  - Pass device and material definition
  - Store result in `m_rootSignature`
  
- **AF2**: Store single root signature ComPtr (shared by all passes)
  - Root signature created once for the material
  - Same root signature used for all passes (same parameters)
  
- **AF3**: Implement `getRootSignature()` getter
  - Return raw pointer from ComPtr
  - Return nullptr if root signature not created

**Tests:**
```cpp
TEST_CASE("MaterialInstance retrieves root signature on construction", "[material-instance][T302][integration]")
TEST_CASE("MaterialInstance getRootSignature returns valid pointer", "[material-instance][T302][integration]")
TEST_CASE("MaterialInstance with invalid material has no root signature", "[material-instance][T302][integration]")
```

---

#### **T303: MaterialInstance Multi-Pass PSO Management**
**Objective:** Implement per-pass PSO caching with lazy creation

**Atomic Functionalities:**
- **AF1**: Implement `createPipelineStateForPass(passName)` using `PipelineBuilder::buildPSO()`
  - Query material pass from MaterialSystem
  - Query render pass config from MaterialSystem
  - Call `PipelineBuilder::buildPSO()` with device, material, config, materialSystem, passName
  - Store result in `m_pipelineStates[passName]`
  - Return true on success, false on failure
  
- **AF2**: Implement `getPipelineState(passName)` with lazy creation + per-pass caching
  - Check if pass exists using `hasPass()`
  - Check if PSO already cached in `m_pipelineStates`
  - If not cached or dirty, call `createPipelineStateForPass()`
  - Remove pass from dirty set if recreation succeeded
  - Return raw pointer from ComPtr (or nullptr if failed)
  
- **AF3**: Add dirty flag management per pass (`std::unordered_set<std::string>`)
  - Initialize `m_dirtyPasses` as empty
  - When creating PSO, remove pass from dirty set on success
  
- **AF4**: Handle pass not found gracefully
  - Return nullptr from `getPipelineState()` if pass doesn't exist
  - Log warning/error as appropriate

**Tests:**
```cpp
TEST_CASE("MaterialInstance getPipelineState creates PSO on first access", "[material-instance][T303][integration]")
TEST_CASE("MaterialInstance getPipelineState returns cached PSO on second access", "[material-instance][T303][integration]")
TEST_CASE("MaterialInstance getPipelineState for different passes creates separate PSOs", "[material-instance][T303][integration]")
TEST_CASE("MaterialInstance getPipelineState for invalid pass returns nullptr", "[material-instance][T303][unit]")
TEST_CASE("MaterialInstance getPipelineState recreates PSO when marked dirty", "[material-instance][T303][integration]")
```

---

#### **T304: MaterialInstance Command List Setup**
**Objective:** Provide convenient API for setting up command list with material

**Atomic Functionalities:**
- **AF1**: Implement `setupCommandList(commandList, passName)`
  - Validate command list is not nullptr
  - Call `getPipelineState(passName)` internally
  - If PSO is nullptr, return false
  
- **AF2**: Set PSO and root signature on command list
  - Call `commandList->SetPipelineState(pso)`
  - Call `commandList->SetGraphicsRootSignature(rootSig)`
  - Return true on success
  
- **AF3**: Handle nullptr cases gracefully
  - Return false if command list is nullptr
  - Return false if PSO unavailable (not ready, pass not found)
  - Return false if root signature unavailable

**Tests:**
```cpp
TEST_CASE("MaterialInstance setupCommandList sets PSO and root signature", "[material-instance][T304][integration]")
TEST_CASE("MaterialInstance setupCommandList returns false for invalid pass", "[material-instance][T304][unit]")
TEST_CASE("MaterialInstance setupCommandList returns false for nullptr command list", "[material-instance][T304][unit]")
TEST_CASE("MaterialInstance setupCommandList with different passes sets different PSOs", "[material-instance][T304][integration]")
```

---

#### **T305: MaterialInstance Hot-Reload Integration**
**Objective:** Automatically handle shader hot-reload via ShaderManager callbacks

**Atomic Functionalities:**
- **AF1**: Add ShaderManager parameter to constructor
  - Accept `shader_manager::ShaderManager*` parameter (can be nullptr)
  - Store in `m_shaderManager` member
  
- **AF2**: Implement `onShaderReloaded()` callback
  - Mark ALL passes dirty (add all pass names to `m_dirtyPasses`)
  - Clear cached PSOs from `m_pipelineStates` map
  
- **AF3**: Register callback in constructor if ShaderManager provided
  - Check if `m_shaderManager != nullptr`
  - Register lambda that calls `onShaderReloaded()`
  - Store callback handle in `m_hotReloadCallback`
  
- **AF4**: Unregister callback in destructor
  - Check if callback handle is valid
  - Unregister from ShaderManager
  
- **AF5**: Verify PSO recreation on next access after hot-reload
  - After `onShaderReloaded()` called, `m_dirtyPasses` contains all passes
  - Next `getPipelineState()` call recreates PSO for dirty passes

**Tests:**
```cpp
TEST_CASE("MaterialInstance registers hot-reload callback with ShaderManager", "[material-instance][T305][integration]")
TEST_CASE("MaterialInstance hot-reload marks all passes dirty", "[material-instance][T305][integration]")
TEST_CASE("MaterialInstance recreates PSOs after hot-reload", "[material-instance][T305][integration]")
TEST_CASE("MaterialInstance without ShaderManager does not register callback", "[material-instance][T305][unit]")
TEST_CASE("MaterialInstance unregisters callback on destruction", "[material-instance][T305][integration]")
```

---

### Phase 2: Integrate into GridRenderer

#### **T306: Refactor GridRenderer to Use MaterialInstance**
**Objective:** Replace manual material management in GridRenderer with MaterialInstance

**Atomic Functionalities:**
- **AF1**: Add ShaderManager to GridRenderer constructor/initialize signature
  - Add `shader_manager::ShaderManager*` parameter to `initialize()`
  - Update all call sites (editor, tests)
  
- **AF2**: Replace manual material members with `unique_ptr<MaterialInstance>`
  - Remove `m_pipelineState`, `m_rootSignature`, `m_materialSystem`, `m_materialHandle`, `m_pipelineStateDirty`
  - Add `std::unique_ptr<material_system::MaterialInstance> m_material`
  
- **AF3**: Update `initialize()` to create MaterialInstance
  - Create MaterialInstance with device, materialSystem, shaderManager, "grid_material"
  - Check `isValid()` and `hasPass("grid")`
  - Remove manual material queries and PSO creation code
  
- **AF4**: Update `render()` to use `setupCommandList()`
  - Replace manual PSO dirty checking with `m_material->setupCommandList(commandList, "grid")`
  - Remove manual PSO recreation logic
  - Keep constant buffer binding (system-specific)
  
- **AF5**: Remove manual PSO dirty checking logic
  - Delete `m_pipelineStateDirty` checks
  - Delete PSO recreation code in `render()`
  
- **AF6**: Remove manual material pass queries
  - Delete calls to `getMaterialPass()`, `getRenderPassConfig()`
  - MaterialInstance handles internally

**Tests:**
```cpp
// Existing grid tests should pass unchanged:
TEST_CASE("Grid renders with valid camera", "[grid][integration]")
TEST_CASE("Grid updates adaptive spacing", "[grid][unit]")
// Add new test for MaterialInstance integration:
TEST_CASE("GridRenderer uses MaterialInstance for PSO management", "[grid][T306][integration]")
```

**Files Modified:**
- `src/graphics/grid/grid.h` - Update class members
- `src/graphics/grid/grid.cpp` - Update initialize() and render()
- Call sites in editor/tests

---

### Phase 3: Documentation

#### **T307: Document MaterialInstance Usage**
**Objective:** Comprehensive documentation for MaterialInstance API and usage patterns

**Documentation Sections:**
1. **Overview**
   - What MaterialInstance does
   - When to use vs. manual PSO management
   - Responsibilities (what it owns vs. what caller owns)

2. **Integration Guide**
   - Step-by-step GridRenderer refactoring example
   - Constructor parameters explained
   - Error handling patterns

3. **Multi-Pass Rendering**
   - Example: PBR material with shadow/depth/forward passes
   - Pass selection at render time
   - Performance implications (lazy PSO creation)

4. **Constant Buffer Patterns**
   - System-specific constant buffer management
   - Binding after `setupCommandList()`
   - Root signature slot conventions

5. **Hot-Reload Behavior**
   - Automatic via ShaderManager
   - All passes marked dirty
   - PSOs recreated on next access

6. **Best Practices**
   - One MaterialInstance per material definition
   - Check `hasPass()` before rendering
   - Handle `setupCommandList()` failure gracefully

7. **When NOT to Use MaterialInstance**
   - Very custom pipeline needs (compute shaders, etc.)
   - Materials not managed by MaterialSystem
   - Temporary/debugging renderers

**File Created:**
- `docs/material_instance_guide.md`

---

## GridRenderer Refactoring Example

### Before (Manual Management)
```cpp
class GridRenderer {
    dx12::Device* m_device;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    MaterialSystem* m_materialSystem;
    MaterialHandle m_materialHandle;
    bool m_pipelineStateDirty;
    
    bool initialize(dx12::Device* device, MaterialSystem* materialSystem) {
        // Manual material queries
        m_materialHandle = m_materialSystem->getMaterialHandle("grid_material");
        const auto* material = m_materialSystem->getMaterial(m_materialHandle);
        
        // Manual root signature creation
        m_rootSignature = PipelineBuilder::getRootSignature(device, *material);
        
        // Manual PSO creation
        const auto* gridPass = m_materialSystem->getMaterialPass(m_materialHandle, "grid");
        const auto passConfig = m_materialSystem->getRenderPassConfig(gridPass->passName);
        m_pipelineState = PipelineBuilder::buildPSO(device, *material, passConfig, materialSystem, "grid");
        
        // Manual dirty tracking
        m_pipelineStateDirty = !m_pipelineState;
    }
    
    bool render(...) {
        // Manual PSO recreation
        if (m_pipelineStateDirty) {
            const auto* material = m_materialSystem->getMaterial(m_materialHandle);
            const auto* gridPass = m_materialSystem->getMaterialPass(m_materialHandle, "grid");
            const auto passConfig = m_materialSystem->getRenderPassConfig(gridPass->passName);
            m_pipelineState = PipelineBuilder::buildPSO(m_device, *material, passConfig, m_materialSystem, "grid");
            m_pipelineStateDirty = false;
        }
        
        // Manual command list setup
        commandList->SetPipelineState(m_pipelineState.Get());
        commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        
        // Draw...
    }
};
```

### After (MaterialInstance)
```cpp
class GridRenderer {
    dx12::Device* m_device;
    std::unique_ptr<material_system::MaterialInstance> m_material;
    
    // System-specific resources only
    Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
    void* m_constantBufferData;
    GridSettings m_settings;
    
    bool initialize(dx12::Device* device, MaterialSystem* materialSystem, shader_manager::ShaderManager* shaderManager) {
        // Create material instance (handles everything)
        m_material = std::make_unique<material_system::MaterialInstance>(
            device, materialSystem, shaderManager, "grid_material"
        );
        
        if (!m_material->isValid() || !m_material->hasPass("grid")) {
            console::error("GridRenderer: Invalid material or missing 'grid' pass");
            return false;
        }
        
        // Create system-specific constant buffer
        return createConstantBuffer();
    }
    
    bool render(...) {
        // Update system-specific constant buffer
        updateConstantBuffer(...);
        
        // Setup material (PSO + root signature, auto hot-reload)
        if (!m_material->setupCommandList(commandList, "grid")) {
            console::warning("Grid material not ready");
            return false;
        }
        
        // Bind system-specific constant buffer
        commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
        
        // Draw...
    }
};
```

**Lines Removed:** ~50  
**Lines Added:** ~15  
**Complexity Reduction:** Material lifecycle abstracted away

---

## Multi-Pass Rendering Example

```cpp
// Example: PBR material with multiple passes
class MeshRenderer {
    std::unique_ptr<MaterialInstance> m_pbrMaterial;
    
    void initialize() {
        m_pbrMaterial = std::make_unique<MaterialInstance>(
            device, materialSystem, shaderManager,
            "pbr_material"  // Has passes: "shadow", "depth_prepass", "forward"
        );
        
        // Verify material has required passes
        if (!m_pbrMaterial->hasPass("shadow") || 
            !m_pbrMaterial->hasPass("forward")) {
            console::error("PBR material missing required passes");
        }
    }
    
    void renderShadowPass() {
        // Uses "shadow" pass (minimal shaders, depth only)
        if (m_pbrMaterial->setupCommandList(commandList, "shadow")) {
            // Bind shadow-specific constants (light matrices)
            commandList->SetGraphicsRootConstantBufferView(1, shadowConstants);
            // Draw...
        }
    }
    
    void renderDepthPrepass() {
        // Uses "depth_prepass" pass (Z-buffer fill)
        if (m_pbrMaterial->setupCommandList(commandList, "depth_prepass")) {
            // Bind minimal constants (world matrix only)
            commandList->SetGraphicsRoot32BitConstants(1, 16, &worldMatrix, 0);
            // Draw...
        }
    }
    
    void renderForwardPass() {
        // Uses "forward" pass (full lighting)
        if (m_pbrMaterial->setupCommandList(commandList, "forward")) {
            // Bind full lighting constants
            commandList->SetGraphicsRootConstantBufferView(1, lightingConstants);
            commandList->SetGraphicsRootConstantBufferView(2, materialConstants);
            // Draw...
        }
    }
};
```

---

## Benefits Summary

### Reduced Duplication
- ✅ No more copying PSO/root signature management across renderers
- ✅ Centralized dirty tracking and recreation logic
- ✅ ~50 lines removed from each renderer system

### Consistency
- ✅ All systems use same PSO cache and root signature cache
- ✅ Uniform hot-reload behavior
- ✅ Consistent error handling

### Simplicity
- ✅ Renderers focus on their specific logic (constant buffers, draw calls)
- ✅ Material lifecycle handled automatically
- ✅ Clear API: `setupCommandList(passName)`

### Testability
- ✅ MaterialInstance can be unit tested independently
- ✅ Mock material system for renderer tests
- ✅ Easier to test hot-reload scenarios

### Flexibility
- ✅ Single MaterialInstance supports multiple passes
- ✅ Pass selection at render time (dynamic)
- ✅ Easy to add new passes without code changes

### Performance
- ✅ PSOs lazy-created only for used passes
- ✅ Root signature shared across all passes
- ✅ PSO cache per pass avoids redundant creation

---

## Architecture Comparison

| **Aspect** | **Before (Manual)** | **After (MaterialInstance)** |
|------------|---------------------|------------------------------|
| **PSO Storage** | Per renderer system | Centralized in MaterialInstance |
| **Root Signature** | Per renderer system | Shared via MaterialInstance |
| **Hot-Reload** | Manual dirty flag checking | Automatic via ShaderManager |
| **Pass Management** | Manual queries | `setupCommandList(passName)` |
| **Code per Renderer** | ~80 lines boilerplate | ~15 lines |
| **Multi-Pass Support** | Difficult (multiple PSOs) | Natural (one instance) |
| **Testing** | Coupled to material system | Decoupled via MaterialInstance |

---

## Design Decisions

### 1. Parameter Binding (Deferred)
**Decision:** MaterialInstance does NOT manage parameter binding  
**Rationale:**
- Constant buffer layouts too system-specific
- Each renderer has different binding requirements
- Future milestone can add `bindParameter(name, value)` API if needed
- Keeps MaterialInstance focused and simple

### 2. Multi-Pass Support (One Instance Per Material)
**Decision:** Single MaterialInstance manages all passes for one material  
**Rationale:**
- Natural mapping (one material definition = one instance)
- Root signature shared across passes (same parameters)
- Pass selection at render time (flexible)
- PSO cache per pass (efficient)

### 3. Automatic Hot-Reload (Via ShaderManager)
**Decision:** MaterialInstance registers for shader hot-reload automatically  
**Rationale:**
- Consistent behavior across all systems
- No manual `markDirty()` calls needed
- ShaderManager already has callback infrastructure
- Simplifies renderer code

---

## Migration Strategy

1. ✅ **Implement MaterialInstance** (T301-T305) - Core infrastructure
2. ✅ **Migrate GridRenderer** (T306) - First client, proves API
3. ✅ **Document MaterialInstance** (T307) - Enable other migrations
4. 🔜 **Migrate MeshRenderingSystem** (future) - More complex, PSO cache
5. 🔜 **Migrate Other Renderers** (future) - Selection outline, gizmos, etc.

---

## Testing Strategy

- **Unit Tests:** MaterialInstance API (validity, pass queries, getters)
- **Integration Tests:** PSO creation, root signature caching, hot-reload
- **Regression Tests:** Existing grid tests pass unchanged after refactoring
- **Manual Tests:** Visual verification in editor (grid renders, hot-reload works)

---

## Open Questions & Future Work

### Resolved
- ✅ Should MaterialInstance manage parameter binding? **No (deferred)**
- ✅ Should MaterialInstance cache multiple passes? **Yes (all passes)**
- ✅ Should hot-reload be automatic? **Yes (via ShaderManager)**

### Future Considerations
- Add `bindParameter(name, value)` API for uniform constant buffer binding?
- Support for compute materials (different pipeline setup)?
- Thread-safety for multi-threaded rendering?
- Texture binding abstraction?

---

## Success Criteria

### Phase 1 Complete When:
- ✅ All T301-T305 tests pass
- ✅ MaterialInstance can be instantiated with valid material
- ✅ PSOs lazy-created per pass
- ✅ Hot-reload marks all passes dirty

### Phase 2 Complete When:
- ✅ GridRenderer refactored to use MaterialInstance
- ✅ All existing grid tests pass
- ✅ Grid renders correctly in editor
- ✅ Hot-reload works in editor

### Phase 3 Complete When:
- ✅ Documentation complete and reviewed
- ✅ Example code verified
- ✅ Ready for other systems to migrate

---

## References

- **Existing Code:**
  - `src/graphics/material_system/pso_builder.h/cpp` - PSO creation
  - `src/graphics/material_system/material_system.h/cpp` - Material queries
  - `src/graphics/grid/grid.h/cpp` - Current manual management
  - `src/runtime/mesh_rendering_system.h/cpp` - PSO cache pattern

- **Related Tasks:**
  - T203: PipelineBuilder uses shader info from material
  - T207: PipelineBuilder uses state blocks
  - T215: Root signature from cache in PSO

- **Documentation:**
  - `docs/material_system.md` - Material system overview
  - `PROGRESS_2.md` - Historical context on material system evolution

---

## Commit Message Template

```
material: add MaterialInstance for pipeline state abstraction

- Add MaterialInstance class for material lifecycle management
- Implements multi-pass PSO caching with lazy creation
- Automatic hot-reload via ShaderManager callbacks
- Refactor GridRenderer to use MaterialInstance (~50 lines removed)
- Add comprehensive tests for MaterialInstance API
- Add docs/material_instance_guide.md

MaterialInstance abstracts PSO, root signature, and hot-reload logic
away from individual renderer systems. Each instance manages all passes
for one material definition. Pass selection happens at render time via
setupCommandList(passName). Reduces boilerplate and ensures consistent
material handling across all renderers.

Refs: M2-P7+ (Material System Abstraction)
```

---

**End of Plan**
