# Graphics Layer

The graphics layer provides rendering systems, GPU resource management, material systems, and shader management built on top of DirectX 12.

## Purpose

This layer abstracts DirectX 12 complexity and provides high-level rendering utilities:
- GPU resource management (buffers, textures, descriptors)
- Data-driven material system with reflection
- Shader compilation and hot-reloading
- Immediate mode rendering utilities
- Editor-specific rendering (grid, debug shapes)

## Key Components

### GPU Resource Management

#### `gpu/gpu_resource_manager.h/cpp`
Central GPU resource allocator and descriptor heap manager.
- **Key class**: `graphics::GPUResourceManager`
- **Responsibilities**: Buffer/texture allocation, descriptor heaps, staging resources
- **Features**: Automatic descriptor allocation, resource lifetime tracking
- **Usage**: All GPU resources should go through this manager

#### `gpu/buffer.h/cpp`
GPU buffer abstractions (vertex, index, constant, structured).
- **Key classes**: `GpuBuffer`, `ConstantBuffer`, `StructuredBuffer`
- **Features**: Upload helpers, mapping, views (CBV/SRV/UAV)

#### `gpu/texture.h/cpp`
Texture resource management.
- **Key class**: `Texture`
- **Features**: Loading from file, render targets, descriptor views (SRV/RTV/DSV)
- **Formats**: Support for common formats (RGBA8, BC1-7, depth formats)

### Material System

#### `material_system/material_system.h/cpp`
Data-driven material system with JSON definitions.
- **Key class**: `graphics::MaterialSystem`
- **Features**: Load materials from JSON, parameter binding, shader variant selection
- **Reflection**: Automatic root signature generation from shader reflection
- **Hot-reload**: Materials can be reloaded at runtime

#### `material_system/material.h/cpp`
Material instance representation.
- **Key class**: `Material`
- **Properties**: Shader parameters (textures, constants, colors)
- **Usage**: Attached to mesh components for rendering

#### `material_system/root_signature_builder.h/cpp`
Root signature construction from shader reflection.
- **Features**: Automatic layout generation, descriptor table setup
- **Integration**: Used by material system for PSO creation

### Shader Management

#### `shader_manager/shader_manager.h/cpp`
Shader compilation, caching, and hot-reloading.
- **Key class**: `graphics::ShaderManager`
- **Features**: Compile HLSL to DXIL, shader includes, watch for file changes
- **Caching**: Compiled shaders cached on disk
- **Reflection**: Extract shader reflection data (inputs, resources, constants)

### Rendering Utilities

#### `immediate_renderer/immediate_renderer.h/cpp`
Immediate mode rendering for debug shapes and UI overlays.
- **Key class**: `graphics::ImmediateRenderer`
- **Features**: Lines, boxes, spheres, text (2D/3D)
- **Usage**: Editor gizmos, debug visualization
- **Note**: Not for high-performance rendering

#### `grid/grid_renderer.h/cpp`
Editor viewport grid rendering.
- **Features**: Infinite grid with fade, grid snapping, configurable spacing
- **Shader**: `shaders/grid.hlsl`

## Dependencies

**Depends on**: `core`, `math`, `platform` (dx12)  
**Used by**: `runtime`, `engine`, `editor`

**External libraries**:
- **DirectX 12**: Core rendering API
- **D3D12 Shader Compiler**: HLSL compilation
- **DirectXTex**: Texture loading (via platform layer)

## Directory Structure

```
graphics/
├── gpu/                          # GPU resource management
│   ├── gpu_resource_manager.h/cpp
│   ├── buffer.h/cpp
│   ├── texture.h/cpp
│   └── descriptor_heap.h/cpp
├── material_system/              # Data-driven materials
│   ├── material_system.h/cpp
│   ├── material.h/cpp
│   ├── material_instance.h/cpp
│   └── root_signature_builder.h/cpp
├── shader_manager/               # Shader compilation
│   ├── shader_manager.h/cpp
│   └── shader_reflection.h/cpp
├── immediate_renderer/           # Immediate mode rendering
│   └── immediate_renderer.h/cpp
└── grid/                         # Grid rendering
    └── grid_renderer.h/cpp
```

## Usage Examples

### GPU Resource Creation

```cpp
#include "graphics/gpu/gpu_resource_manager.h"

// GPUResourceManager is typically instantiated at application level
// and passed to systems that need GPU resource management

// Example usage (mesh and material caching):
auto meshGPU = gpuResourceManager.getMeshGPU(meshAsset);
auto materialGPU = gpuResourceManager.getMaterialGPU(materialAsset);
```

### Material System

```cpp
#include "graphics/material_system/material_system.h"

auto& matSys = graphics::MaterialSystem::instance();

// Load material definition from JSON
auto material = matSys.loadMaterial("materials/pbr_material.json");

// Set material parameters
material->setTexture("albedoMap", albedoTexture);
material->setFloat("metallic", 0.8f);
material->setFloat("roughness", 0.3f);

// Use material for rendering
material->bind(cmdList);
cmdList->DrawInstanced(...);
```

### Shader Compilation

```cpp
#include "graphics/shader_manager/shader_manager.h"

auto& shaderMgr = graphics::ShaderManager::instance();

// Compile shader
auto vsBlob = shaderMgr.compileShader(
    "shaders/pbr.hlsl",
    "VSMain",
    "vs_6_0",
    defines
);

// Get reflection data
auto reflection = shaderMgr.getReflection(vsBlob);
console::info("Inputs: {}", reflection.inputs.size());
```

### Immediate Rendering

```cpp
#include "graphics/immediate_renderer/immediate_renderer.h"

auto& renderer = graphics::ImmediateRenderer::instance();

// Begin frame
renderer.begin(cmdList, viewProj);

// Draw debug shapes
renderer.drawLine(start, end, Color::red());
renderer.drawBox(center, size, Color::green());
renderer.drawSphere(center, radius, Color::blue());

// End and submit
renderer.end();
```

## Architecture Notes

### Resource Lifetime
GPU resources are ref-counted and automatically released when no longer referenced:
```cpp
auto buffer = gpuMgr.createBuffer(...);  // Ref count = 1
auto buffer2 = buffer;                   // Ref count = 2
buffer.reset();                          // Ref count = 1
// Destroyed when buffer2 goes out of scope
```

### Material Pipeline
1. **Definition**: JSON file describes shader, parameters, render state
2. **Loading**: Material system parses JSON and compiles shaders
3. **Reflection**: Root signature auto-generated from shader reflection
4. **Instance**: Material instances can override parameters
5. **Binding**: Parameters bound to GPU via root signature layout

### Descriptor Management
The descriptor heap manager automatically allocates descriptors:
```cpp
// Request descriptor
auto srvHandle = gpuMgr.allocateDescriptor(DescriptorType::SRV);

// Create SRV
device->CreateShaderResourceView(resource, &desc, srvHandle.cpu);

// Use in shader
cmdList->SetGraphicsRootDescriptorTable(0, srvHandle.gpu);
```

### Shader Hot-Reloading
The shader manager watches shader files for changes:
1. File modified
2. Shader recompiled in background
3. Materials using shader invalidated
4. Next frame uses new shader

## Performance Considerations

### Descriptor Heaps
- Pre-allocate large descriptor heaps
- Use descriptor tables for batch binding
- Avoid excessive descriptor copies

### Material Batching
- Sort draws by material to minimize state changes
- Use instancing for identical materials
- Consider material variants for common permutations

### Buffer Updates
- Use upload buffers for frequently updated data
- Batch constant buffer updates
- Ring buffer for per-frame constants

## Testing

Graphics systems use a mix of unit tests (logic) and integration tests (GPU operations):
- **Unit tests**: Shader reflection parsing, material JSON parsing
- **Integration tests**: Resource creation, rendering pipelines, material binding

See `tests/material_system_tests.cpp`, `tests/shader_*_tests.cpp`.

## Debugging

### PIX Integration
Use PIX for DirectX 12 debugging:
```cpp
#include "platform/pix/pix.h"

PIXBeginEvent(cmdList, PIX_COLOR_DEFAULT, "Draw Scene");
// Rendering commands...
PIXEndEvent(cmdList);
```

### Validation Layer
Enable D3D12 debug layer in debug builds:
```cpp
#ifdef _DEBUG
    device->EnableDebugLayer();
#endif
```

## See Also

- `platform/dx12/` - DirectX 12 device and context
- `shaders/` - HLSL shader source files
- `/docs/MATERIAL_SYSTEM_INTEGRATION.md` - Material system architecture
- `/docs/REFLECTION_BASED_ROOT_SIGNATURE_PLAN.md` - Root signature reflection details
- `/specs/001-data-driven-material/` - Material system specification
