# MaterialSystem Integration Guide

## Overview
The MaterialSystem has been integrated into `main.cpp` and is now available throughout the application. This guide shows how to use it in your code.

## Initialization (Already Done)
The MaterialSystem is initialized in `main.cpp` after device and shader manager creation:

```cpp
graphics::material_system::MaterialSystem materialSystem;
if (!materialSystem.initialize("materials_p1.json")) {
    console::error("Failed to initialize material system from materials_p1.json");
    console::info("Application will continue without data-driven materials");
} else {
    console::info("Material system initialized successfully");
}
```

## Available Materials
The system loads materials from `materials_p1.json`. Currently available materials:

| Material ID | Description | Shaders | Use Case |
|-------------|-------------|---------|----------|
| `standard_opaque` | PBR lit material | mesh_vs, lit_ps | Standard 3D objects |
| `unlit_opaque` | Unlit colored material | mesh_vs, unlit_ps | Flat colored objects |
| `grid_material` | Grid rendering | grid_vs, grid_ps | Scene grid |
| `wireframe` | Wireframe visualization | mesh_vs, unlit_ps | Debug visualization |
| `debug_normals` | Normal visualization | mesh_vs, debug_normals_ps | Debug normals |

## Usage Examples

### Example 1: Query Material by ID
```cpp
#include "graphics/material_system/material_system.h"

// Get material handle (lightweight, can be cached)
auto handle = materialSystem.getMaterialHandle("standard_opaque");

// Check if material exists
if (!handle.isValid()) {
    console::error("Material 'standard_opaque' not found");
    return;
}

// Retrieve material definition (for inspection or rendering setup)
const auto* material = materialSystem.getMaterial(handle);
if (material) {
    console::info("Material ID: {}", material->id);
    console::info("Pass: {}", material->pass);
    console::info("Shader count: {}", material->shaders.size());
}
```

### Example 2: Iterate All Materials
```cpp
// Note: MaterialSystem doesn't expose iteration in P1
// Materials are accessed by ID only
// If you need to list all materials, access via known IDs:

std::vector<std::string> knownMaterialIds = {
    "standard_opaque",
    "unlit_opaque",
    "grid_material",
    "wireframe",
    "debug_normals"
};

for (const auto& materialId : knownMaterialIds) {
    auto handle = materialSystem.getMaterialHandle(materialId);
    if (handle.isValid()) {
        const auto* material = materialSystem.getMaterial(handle);
        // Use material...
    }
}
```

### Example 3: Use Material in Entity Component
```cpp
// In your entity/component system:
struct MaterialComponent {
    graphics::material_system::MaterialHandle handle;
    std::string materialId; // For re-querying if needed
};

// Initialize component
void createEntity(const std::string& materialId) {
    MaterialComponent matComp;
    matComp.materialId = materialId;
    matComp.handle = materialSystem.getMaterialHandle(materialId);
    
    if (!matComp.handle.isValid()) {
        console::error("Failed to get material: {}", materialId);
        matComp.handle = materialSystem.getMaterialHandle("standard_opaque"); // Fallback
    }
    
    // Add component to entity...
}

// Render system
void renderEntity(const MaterialComponent& matComp) {
    const auto* material = materialSystem.getMaterial(matComp.handle);
    if (!material) {
        console::error("Invalid material handle for entity");
        return;
    }
    
    // Use material->pass, material->shaders, material->states for rendering
    // Setup PSO, bind resources, etc.
}
```

### Example 4: Build PSO from Material
```cpp
#include "graphics/material_system/pipeline_builder.h"

// Get material
auto handle = materialSystem.getMaterialHandle("standard_opaque");
const auto* material = materialSystem.getMaterial(handle);

if (material) {
    // Create render pass config
    graphics::material_system::RenderPassConfig passConfig;
    passConfig.name = "forward";
    passConfig.rtvFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    passConfig.dsvFormat = DXGI_FORMAT_D32_FLOAT;
    passConfig.numRenderTargets = 1;
    
    // Build PSO (automatically cached)
    auto pso = graphics::material_system::PipelineBuilder::buildPSO(
        &device,
        *material,
        passConfig
    );
    
    if (pso) {
        console::info("PSO created successfully for material: {}", material->id);
        // Use PSO for rendering...
        // commandList->SetPipelineState(pso.Get());
    }
}
```

## Material Parameters
Each material can define parameters (PBR properties, colors, etc.):

```cpp
const auto* material = materialSystem.getMaterial(handle);

// Parameters are stored in material->parameters vector
for (const auto& param : material->parameters) {
    console::info("Parameter: {} (type: {})", param.name, static_cast<int>(param.type));
    
    // Access default value from param.defaultValue (nlohmann::json)
    if (param.type == graphics::material_system::ParameterType::Float4) {
        // param.defaultValue is a JSON array [r, g, b, a]
        auto color = param.defaultValue.get<std::vector<float>>();
        console::info("  Default color: [{}, {}, {}, {}]", 
            color[0], color[1], color[2], color[3]);
    }
}
```

## Material States
Each material references state blocks:

```cpp
const auto* material = materialSystem.getMaterial(handle);

console::info("Rasterizer state: {}", material->states.rasterizer);
console::info("Depth/Stencil state: {}", material->states.depthStencil);
console::info("Blend state: {}", material->states.blend);
// Note: renderTarget state is material->states.renderTarget
```

## Error Handling
MaterialSystem uses graceful error handling:

```cpp
// Invalid material ID returns invalid handle (no exceptions)
auto handle = materialSystem.getMaterialHandle("nonexistent_material");
if (!handle.isValid()) {
    // Handle gracefully, use fallback material
    handle = materialSystem.getMaterialHandle("standard_opaque");
}

// Invalid handle returns nullptr (no exceptions)
const auto* material = materialSystem.getMaterial(handle);
if (!material) {
    console::error("Material lookup failed");
    return;
}
```

## Adding New Materials

### Step 1: Edit materials_p1.json
Add a new material definition to the `materials` array:

```json
{
    "id": "my_custom_material",
    "pass": "forward",
    "shaders": {
        "vertex": "custom_vs",
        "pixel": "custom_ps"
    },
    "states": {
        "rasterizer": "solid_back",
        "depthStencil": "depth_test_write",
        "blend": "opaque"
    },
    "parameters": [
        {
            "name": "tintColor",
            "type": "float4",
            "defaultValue": [1.0, 0.5, 0.2, 1.0]
        }
    ],
    "enabled": true
}
```

### Step 2: Restart Application
No C++ code changes needed! Just:
1. Save `materials_p1.json`
2. Restart the application
3. MaterialSystem automatically loads the new material

### Step 3: Use the Material
```cpp
auto handle = materialSystem.getMaterialHandle("my_custom_material");
if (handle.isValid()) {
    console::info("Custom material loaded successfully!");
}
```

## Performance Notes

### Material Queries
- `getMaterialHandle()`: O(1) via `std::unordered_map` lookup
- `getMaterial()`: O(1) bounds check + vector access
- **Cache handles** in components/systems when possible

### PSO Caching
- PSOs are automatically cached by hash (material + pass + states)
- Second call to `buildPSO()` with same inputs returns cached PSO
- No performance penalty for repeated PSO requests

### Initialization
- Material loading happens once at startup (~16ms for 5 materials)
- Projected: 25 materials load in <400ms (well under 2s budget)

## Troubleshooting

### Material Not Found
```
[ERROR] Material 'my_material' not found
```
**Solution**: Check `materials_p1.json` for correct material ID spelling.

### Failed to Initialize MaterialSystem
```
[ERROR] Failed to initialize material system from materials_p1.json
```
**Solutions**:
1. Verify `materials_p1.json` exists in working directory
2. Check JSON syntax (use JSON validator)
3. Verify all required fields present (id, pass, shaders)

### Invalid Handle After Restart
**Cause**: MaterialSystem is recreated on restart, handles become invalid.
**Solution**: Always query fresh handles after initialization, or re-query via material ID.

## Integration with Existing Systems

### With AssetManager
```cpp
// Load scene assets
assetManager.loadScene("scene.gltf");

// Query material for loaded meshes
auto handle = materialSystem.getMaterialHandle("standard_opaque");
// Assign to mesh components...
```

### With Renderer
```cpp
// In your render loop
renderer.beginFrame();

// Query material
const auto* material = materialSystem.getMaterial(entityMaterialHandle);

// Build/get PSO
auto pso = PipelineBuilder::buildPSO(&device, *material, passConfig);

// Bind and render
commandList->SetPipelineState(pso.Get());
// ... bind resources, draw calls ...

renderer.endFrame();
```

### With ECS Systems
```cpp
class MaterialRenderingSystem : public systems::System {
    void update(ecs::Scene& scene, float deltaTime) override {
        scene.forEach<MaterialComponent, Transform>([this](auto entity, auto& mat, auto& transform) {
            const auto* material = materialSystem.getMaterial(mat.handle);
            if (material) {
                // Render with material...
            }
        });
    }
    
private:
    graphics::material_system::MaterialSystem& materialSystem;
};
```

## Next Steps

### Phase 2 (P2): Render Pass Configuration
- Extended pass config (clear colors, materials per pass)
- Pass compatibility validation
- Dynamic pass configuration via JSON

### Phase 3 (P3): State Block Includes
- Factor common states into separate files
- Include mechanism for state reuse
- Override semantics

### Phase 4: Polish & Features
- Hot reload support
- Multi-threaded loading
- Material editor GUI
- Shader variant generation

## API Reference

### MaterialSystem Class
```cpp
namespace graphics::material_system {
    class MaterialSystem {
    public:
        // Initialize from JSON file
        bool initialize(const std::string& jsonPath);
        
        // Get opaque handle for material (O(1))
        MaterialHandle getMaterialHandle(const std::string& materialId) const;
        
        // Get material definition (O(1))
        const MaterialDefinition* getMaterial(MaterialHandle handle) const;
    };
}
```

### MaterialHandle Struct
```cpp
struct MaterialHandle {
    uint32_t index = UINT32_MAX; // UINT32_MAX = invalid
    
    bool isValid() const { return index != UINT32_MAX; }
};
```

### MaterialDefinition Struct
```cpp
struct MaterialDefinition {
    std::string id;                           // Unique material ID
    std::string pass;                         // Render pass name
    std::vector<ShaderReference> shaders;     // Vertex, pixel, etc.
    std::vector<Parameter> parameters;        // Material parameters
    StateReferences states;                   // Rasterizer, depth, blend
    bool enabled = true;                      // Enable/disable flag
    std::string versionHash;                  // For change detection
};
```

## Summary

✅ **MaterialSystem is now integrated** into the application  
✅ **5 materials available** for use (standard, unlit, grid, wireframe, debug)  
✅ **Zero C++ changes** required to add/modify materials  
✅ **Handle-based API** for efficient queries  
✅ **PSO caching** for optimal performance  
✅ **Graceful error handling** with fallback strategies  

**Key Benefit**: Materials can now be authored entirely in JSON, accelerating iteration and reducing code complexity.

---

**Last Updated**: 2025-10-10  
**Related Files**: `main.cpp`, `materials_p1.json`, `P1_COMPLETION_REPORT.md`
