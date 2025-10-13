# MeshRenderingSystem Migration to MaterialInstance

**Date:** 2025-10-12  
**Status:** Planning Phase  
**Milestone:** M3 (Data-Driven Material System)  
**Branch:** 001-data-driven-material

---

## 🎯 Objective

Migrate `MeshRenderingSystem` from manual `MaterialGPU` + PSO management to using `MaterialInstance` for automatic, data-driven material handling with hot-reload support.

---

## 📊 Current State Analysis

### Current Architecture (Legacy)
```
MeshRenderingSystem
├── MaterialGPU (old asset-based materials)
├── Manual root signature creation
├── Manual PSO creation & caching
├── Hardcoded shader registration ("unlit.hlsl")
├── Hardcoded input layout
└── Custom shader reload callback
```

### Problems with Current System
1. **Hardcoded materials**: Only supports "unlit.hlsl" shader
2. **No data-driven support**: Cannot load materials from JSON
3. **Manual PSO management**: ~150 lines of boilerplate code
4. **Duplicated logic**: Root signature + PSO code duplicates MaterialInstance functionality
5. **Limited flexibility**: Cannot switch materials at runtime
6. **No multi-pass support**: Cannot render shadow passes, depth prepass, etc.

---

## 🎨 Target Architecture (MaterialInstance)

### New Architecture
```
MeshRenderingSystem
├── MaterialInstance (per material type)
│   ├── Automatic PSO creation
│   ├── Automatic root signature management
│   ├── Hot-reload support
│   └── Multi-pass support
├── MaterialSystem (global material registry)
└── Data-driven material selection per mesh
```

### Benefits
- ✅ **Data-driven**: Materials loaded from `materials.json`
- ✅ **Multi-material support**: Different materials per mesh
- ✅ **Hot-reload**: Shader changes automatically rebuild PSOs
- ✅ **Multi-pass**: Support for forward, shadow, wireframe passes
- ✅ **Cleaner code**: Remove ~150 lines of boilerplate
- ✅ **Consistent**: Uses same system as GridRenderer

---

## 🔧 Implementation Plan

### Phase 1: Add MaterialSystem Dependency ✅ (Already Available)
**Status:** MaterialSystem is already initialized in `main.cpp`

**Verification:**
```cpp
// In main.cpp (line 106-107):
graphics::material_system::MaterialSystem materialSystem;
if (!materialSystem.initialize("materials.json")) { ... }
```

**Action:** Pass MaterialSystem pointer to MeshRenderingSystem constructor

---

### Phase 2: Define Material Types in JSON

**Create/Update `materials.json`:**
```json
{
  "materials": [
    {
      "id": "mesh_unlit",
      "passes": [
        {
          "name": "forward",
          "shaders": {
            "vertex": {
              "file": "shaders/unlit.hlsl",
              "entry": "VSMain",
              "profile": "vs_5_0"
            },
            "pixel": {
              "file": "shaders/unlit.hlsl",
              "entry": "PSMain",
              "profile": "ps_5_0"
            }
          },
          "vertexFormat": "PositionNormalUVTangentColor",
          "states": {
            "rasterizer": "solid_back",
            "depthStencil": "depth_disabled",
            "blend": "opaque"
          },
          "primitiveTopology": "Triangle",
          "parameters": [
            {
              "name": "FrameConstants",
              "binding": "b0",
              "type": "cbv"
            },
            {
              "name": "ObjectConstants",
              "binding": "b1",
              "type": "cbv"
            },
            {
              "name": "MaterialConstants",
              "binding": "b2",
              "type": "cbv"
            }
          ]
        }
      ]
    }
  ],
  "vertexFormats": [
    {
      "id": "PositionNormalUVTangentColor",
      "stride": 64,
      "elements": [
        { "semantic": "POSITION", "format": "R32G32B32_FLOAT", "offset": 0 },
        { "semantic": "NORMAL", "format": "R32G32B32_FLOAT", "offset": 12 },
        { "semantic": "TEXCOORD", "format": "R32G32_FLOAT", "offset": 24 },
        { "semantic": "TANGENT", "format": "R32G32B32A32_FLOAT", "offset": 32 },
        { "semantic": "COLOR", "format": "R32G32B32A32_FLOAT", "offset": 48 }
      ]
    }
  ]
}
```

---

### Phase 3: Modify MeshRenderingSystem Class

#### 3.1 Update Constructor Signature
**File:** `src/runtime/mesh_rendering_system.h`

```cpp
// OLD:
MeshRenderingSystem(
    renderer::Renderer& renderer,
    std::shared_ptr<shader_manager::ShaderManager> shaderManager,
    systems::SystemManager* systemManager
);

// NEW:
MeshRenderingSystem(
    renderer::Renderer& renderer,
    graphics::material_system::MaterialSystem* materialSystem,
    std::shared_ptr<shader_manager::ShaderManager> shaderManager,
    systems::SystemManager* systemManager
);
```

#### 3.2 Update Member Variables
**File:** `src/runtime/mesh_rendering_system.h`

```cpp
// REMOVE:
shader_manager::ShaderHandle m_vertexShaderHandle;
shader_manager::ShaderHandle m_pixelShaderHandle;
shader_manager::CallbackHandle m_callbackHandle;
Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> m_pipelineStateCache;

// REMOVE METHODS:
void createRootSignature();
bool registerShaders();
ID3D12PipelineState* getMaterialPipelineState(const engine::gpu::MaterialGPU& material);
Microsoft::WRL::ComPtr<ID3D12PipelineState> createMaterialPipelineState(const engine::gpu::MaterialGPU& material);
void setRootSignature(ID3D12GraphicsCommandList* commandList);

// ADD:
graphics::material_system::MaterialSystem* m_materialSystem;
std::unique_ptr<graphics::material_system::MaterialInstance> m_defaultMaterialInstance;
// Optional: Cache per-material instances if needed
std::unordered_map<std::string, std::unique_ptr<graphics::material_system::MaterialInstance>> m_materialInstanceCache;
```

---

### Phase 4: Refactor Constructor Implementation
**File:** `src/runtime/mesh_rendering_system.cpp`

```cpp
MeshRenderingSystem::MeshRenderingSystem(
    renderer::Renderer& renderer,
    graphics::material_system::MaterialSystem* materialSystem,
    std::shared_ptr<shader_manager::ShaderManager> shaderManager,
    systems::SystemManager* systemManager
)
    : m_renderer(renderer)
    , m_materialSystem(materialSystem)
    , m_shaderManager(shaderManager)
    , m_systemManager(systemManager)
{
    if (!materialSystem) {
        console::error("MeshRenderingSystem: MaterialSystem is required");
        return;
    }

    // Create default MaterialInstance for mesh rendering
    m_defaultMaterialInstance = std::make_unique<graphics::material_system::MaterialInstance>(
        &renderer.getDevice(),
        materialSystem,
        shaderManager.get(),
        "mesh_unlit"  // Material ID from materials.json
    );

    if (!m_defaultMaterialInstance->isValid()) {
        console::error("MeshRenderingSystem: Failed to create default material instance");
    }

    if (!systemManager) {
        console::warning("MeshRenderingSystem: Created without SystemManager - hierarchy transforms will not work");
    }
}
```

**Code to Remove:**
- `createRootSignature()` - MaterialInstance handles this
- `registerShaders()` - MaterialInstance handles this
- Shader handle initialization
- Manual reload callback registration

---

### Phase 5: Refactor Render Method
**File:** `src/runtime/mesh_rendering_system.cpp`

```cpp
void MeshRenderingSystem::render(ecs::Scene& scene, const camera::Camera& camera)
{
    auto* commandContext = m_renderer.getCommandContext();
    if (!commandContext) return;

    auto* commandList = commandContext->get();
    if (!commandList) return;

    // Setup default material for rendering
    if (!m_defaultMaterialInstance->setupCommandList(commandList, "forward")) {
        console::warning("MeshRenderingSystem: Failed to setup material instance");
        return;
    }

    // Iterate through all entities with MeshRenderer + Transform
    const auto allEntities = scene.getAllEntities();
    for (const auto entity : allEntities) {
        if (!entity.isValid()) continue;

        const auto* transform = scene.getComponent<components::Transform>(entity);
        const auto* meshRenderer = scene.getComponent<components::MeshRenderer>(entity);

        if (transform && meshRenderer) {
            // Check hierarchical visibility
            if (!isEffectivelyVisible(scene, entity)) {
                continue;
            }

            // Render the entity
            renderEntity(scene, entity, camera);
        }
    }
}
```

**Changes:**
- Replace manual PSO setup with `setupCommandList()`
- Remove `setRootSignature()` call
- Remove per-material PSO lookup

---

### Phase 6: Simplify renderEntity Method
**File:** `src/runtime/mesh_rendering_system.cpp`

```cpp
void MeshRenderingSystem::renderEntity(ecs::Scene& scene, ecs::Entity entity, const camera::Camera& camera)
{
    // Get components
    const auto* transform = scene.getComponent<components::Transform>(entity);
    const auto* meshRenderer = scene.getComponent<components::MeshRenderer>(entity);
    if (!transform || !meshRenderer) return;

    // Get command list
    auto* commandList = m_renderer.getCommandContext()->get();

    // Get mesh GPU resource
    const auto& gpuMesh = meshRenderer->gpuMesh;
    if (!gpuMesh) return;

    // Calculate world transform (supports hierarchy)
    math::Mat4f worldMatrix;
    if (m_systemManager) {
        auto* transformSystem = m_systemManager->getSystem<TransformSystem>();
        if (transformSystem) {
            worldMatrix = transformSystem->getWorldTransform(scene, entity);
        } else {
            worldMatrix = transform->getLocalMatrix();
        }
    } else {
        worldMatrix = transform->getLocalMatrix();
    }

    // Update frame constants (view/projection)
    // ... (keep existing frame constant logic)

    // Update object constants (world matrix)
    // ... (keep existing object constant logic)

    // Render each primitive
    for (size_t i = 0; i < gpuMesh->getPrimitiveCount(); ++i) {
        const auto& primitive = gpuMesh->getPrimitive(i);
        if (!primitive.isValid()) continue;

        // Bind primitive buffers
        primitive.bindForRendering(commandList);

        // REMOVE: Material-specific PSO selection
        // OLD CODE:
        // if (primitive.hasMaterial()) {
        //     auto* pipelineState = getMaterialPipelineState(*primitive.getMaterial());
        //     commandList->SetPipelineState(pipelineState);
        // }

        // NEW: Material is already set via setupCommandList()
        // If we need per-primitive materials later, we can look up MaterialInstance here

        // Draw call
        if (primitive.hasIndexBuffer()) {
            commandList->DrawIndexedInstanced(primitive.getIndexCount(), 1, 0, 0, 0);
        } else {
            commandList->DrawInstanced(primitive.getVertexCount(), 1, 0, 0);
        }
    }
}
```

---

### Phase 7: Update main.cpp Integration
**File:** `src/main.cpp`

```cpp
// Pass MaterialSystem to MeshRenderingSystem
auto meshRenderingSystem = std::make_unique<systems::MeshRenderingSystem>(
    renderer,
    &materialSystem,  // ADD THIS
    shaderManager,
    systemManager.get()
);
```

---

### Phase 8: Future Enhancements (Optional)

#### 8.1 Per-Mesh Material Support
Allow each mesh to specify its own material:

```cpp
// In components.h:
struct MeshRenderer {
    std::shared_ptr<engine::gpu::MeshGPU> gpuMesh;
    std::string materialId = "mesh_unlit";  // NEW: Material override
};

// In renderEntity():
auto* materialInstance = getOrCreateMaterialInstance(meshRenderer->materialId);
materialInstance->setupCommandList(commandList, "forward");
```

#### 8.2 Multi-Pass Support
Render shadow passes, wireframe, selection outlines:

```cpp
void MeshRenderingSystem::renderShadowPass(ecs::Scene& scene, const camera::Camera& light) {
    m_defaultMaterialInstance->setupCommandList(commandList, "shadow");
    // ... render logic
}
```

#### 8.3 Material Instance Caching
Cache MaterialInstance objects per material ID:

```cpp
MaterialInstance* MeshRenderingSystem::getOrCreateMaterialInstance(const std::string& materialId) {
    auto it = m_materialInstanceCache.find(materialId);
    if (it != m_materialInstanceCache.end()) {
        return it->second.get();
    }

    auto instance = std::make_unique<MaterialInstance>(
        &m_renderer.getDevice(),
        m_materialSystem,
        m_shaderManager.get(),
        materialId
    );

    if (!instance->isValid()) {
        return m_defaultMaterialInstance.get();
    }

    auto* ptr = instance.get();
    m_materialInstanceCache[materialId] = std::move(instance);
    return ptr;
}
```

---

## 🧪 Testing Strategy

### Unit Tests (T-series naming)
1. **T-MRS-001**: Test MaterialInstance creation in constructor
2. **T-MRS-002**: Test fallback to default material if "mesh_unlit" not found
3. **T-MRS-003**: Test rendering with MaterialInstance
4. **T-MRS-004**: Test hot-reload triggers PSO rebuild
5. **T-MRS-005**: Test per-mesh material override (future enhancement)

### Integration Tests
1. Load scene with multiple meshes
2. Verify all meshes render with correct material
3. Hot-reload shader and verify visual update
4. Switch material at runtime

### Regression Tests
1. Verify existing scenes still render correctly
2. Check performance (MaterialInstance should be comparable or better)
3. Validate memory usage (no leaks from MaterialInstance lifecycle)

---

## 📋 Migration Checklist

### Code Changes
- [ ] Add MaterialSystem parameter to constructor
- [ ] Create default MaterialInstance in constructor
- [ ] Remove `createRootSignature()` method
- [ ] Remove `registerShaders()` method
- [ ] Remove `getMaterialPipelineState()` method
- [ ] Remove `createMaterialPipelineState()` method
- [ ] Remove `setRootSignature()` method
- [ ] Remove shader handle members
- [ ] Remove root signature member
- [ ] Remove PSO cache member
- [ ] Update `render()` to use `setupCommandList()`
- [ ] Update `renderEntity()` to remove per-material PSO lookup
- [ ] Update `main.cpp` to pass MaterialSystem

### JSON Configuration
- [ ] Add "mesh_unlit" material to `materials.json`
- [ ] Define "PositionNormalUVTangentColor" vertex format
- [ ] Configure "forward" pass with correct shaders
- [ ] Set appropriate render states (depth disabled for viewport)

### Testing
- [ ] Write unit tests for MaterialInstance integration
- [ ] Test hot-reload functionality
- [ ] Verify existing scenes render correctly
- [ ] Performance benchmark (should be comparable)

### Documentation
- [ ] Update code comments
- [ ] Document material selection mechanism
- [ ] Add examples for per-mesh material overrides

---

## 🚀 Execution Order

### Atomic Functionalities (TDD)
1. **AF1**: Add MaterialSystem to constructor signature
2. **AF2**: Create default MaterialInstance in constructor
3. **AF3**: Replace `render()` PSO setup with `setupCommandList()`
4. **AF4**: Remove manual root signature binding
5. **AF5**: Remove PSO cache and creation methods
6. **AF6**: Update main.cpp integration
7. **AF7**: Add "mesh_unlit" material to JSON
8. **AF8**: Test and validate rendering works

Each AF follows **Red → Green → Refactor**:
- Write failing test
- Implement minimal change
- Refactor and clean up

---

## 📊 Success Criteria

✅ **Functional:**
- MeshRenderingSystem uses MaterialInstance
- All existing meshes render correctly
- Shader hot-reload works

✅ **Code Quality:**
- Remove at least 150 lines of boilerplate
- No manual PSO/root signature management
- Consistent with GridRenderer pattern

✅ **Performance:**
- No measurable performance regression
- PSO caching still effective

✅ **Maintainability:**
- Materials configurable via JSON
- Easy to add new material types
- Clear separation of concerns

---

## 🔗 References

- GridRenderer implementation (already migrated)
- MaterialInstance documentation (`docs/material_instance_plan.md`)
- MaterialSystem integration guide (`docs/MATERIAL_SYSTEM_INTEGRATION.md`)
- Phase 1 validation (`specs/001-data-driven-material/P1_VALIDATION_SUMMARY.md`)

---

## 📝 Notes

- GridRenderer is already using MaterialInstance successfully - use as reference
- MaterialInstance handles all PSO/root signature lifecycle automatically
- Hot-reload support comes for free via ShaderManager integration
- Multi-pass support enables future shadow/wireframe passes
- Per-mesh material selection can be added incrementally

---

**Status:** Ready for implementation ✅  
**Estimated effort:** 4-6 hours (including tests)  
**Risk level:** Low (proven pattern from GridRenderer)
