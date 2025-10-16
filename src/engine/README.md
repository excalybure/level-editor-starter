# Engine Layer

The engine layer provides high-level game engine features including asset management, scene importing, camera systems, and integration between subsystems.

## Purpose

This layer bridges the gap between the low-level runtime/graphics systems and the high-level editor/game logic. It provides:
- Asset loading and management
- glTF scene importing
- Camera controllers
- Object picking
- Integration utilities between engine systems

## Key Components

### Asset Management

#### `assets/asset_manager.h/cpp`
Central asset registry and lifetime management.
- **Key class**: `engine::AssetManager`
- **Responsibilities**: Asset loading, caching, reference tracking, hot-reloading
- **Supported types**: Meshes, textures, materials, scenes
- **Features**: Async loading, dependency resolution, memory management

#### `assets/asset_resolution.h/cpp`
Path resolution and asset discovery.
- **Features**: Relative path resolution, asset search paths, file system monitoring
- **Usage**: Resolves asset references in scene files and material definitions

#### `integration/asset_gltf_integration.h/cpp`
Integration between asset manager and glTF loader.
- **Namespace**: `engine::integration`
- **Responsibilities**: Convert glTF data to engine assets, material translation, texture importing
- **Entry point**: Called by asset manager when loading `.gltf`/`.glb` files

### Scene Import

#### `gltf_loader/`
glTF 2.0 file format parser and importer.
- **Key classes**: `GltfLoader`, `GltfScene`, `GltfMesh`, etc.
- **Features**: Full glTF 2.0 spec support (meshes, materials, textures, animations, skins)
- **Extensions**: Supports common glTF extensions
- **Output**: Converts glTF data to engine's internal formats

### Camera

#### `camera/camera_controller.h/cpp`
Interactive camera control (orbit, FPS, fly).
- **Key class**: `engine::CameraController`
- **Modes**: Orbit (Maya-style), FPS, fly camera
- **Features**: Smooth movement, mouse look, keyboard navigation
- **Usage**: Used by editor viewport and game camera

#### `camera/camera.h/cpp`
Camera component and projection utilities.
- **Key class**: `engine::Camera`
- **Projections**: Perspective, orthographic
- **Features**: View matrix calculation, frustum definition
- **Integration**: Works with ECS camera component

### Picking

#### `picking.h/cpp`
Ray-based object selection in 3D scenes.
- **Key functions**: `engine::raycast()`, `engine::pickEntity()`
- **Algorithm**: BVH-accelerated ray-triangle intersection
- **Usage**: Used by editor for viewport selection
- **Returns**: Entity ID, world position, distance

## Dependencies

**Depends on**: `core`, `math`, `graphics`, `runtime`  
**Used by**: `editor`

**External libraries**:
- **cgltf**: glTF parsing (used by gltf_loader)
- **stb_image**: Texture loading

## Directory Structure

```
engine/
├── assets/                    # Asset management system
│   ├── asset_manager.h/cpp   # Central asset registry
│   └── asset_resolution.h/cpp # Path resolution
├── camera/                    # Camera systems
│   ├── camera.h/cpp          # Camera component
│   └── camera_controller.h/cpp # Interactive camera control
├── gltf_loader/              # glTF import
│   ├── gltf_loader.h/cpp     # Main loader
│   └── gltf_*.h/cpp          # glTF data structures
├── integration/              # System integration utilities
│   └── asset_gltf_integration.h/cpp
└── picking.h/cpp             # Ray-based selection
```

## Usage Examples

### Loading an Asset

```cpp
#include "engine/assets/asset_manager.h"

auto& assetMgr = engine::AssetManager::instance();
auto meshHandle = assetMgr.loadMesh("assets/models/cube.gltf");

if (meshHandle.isValid()) {
    const auto* mesh = assetMgr.getMesh(meshHandle);
    // Use mesh...
}
```

### Importing a glTF Scene

```cpp
#include "engine/gltf_loader/gltf_loader.h"
#include "engine/integration/asset_gltf_integration.h"

// Load glTF file
auto gltfScene = engine::loadGltf("assets/scenes/level.gltf");

// Convert to engine format
auto entities = engine::integration::importGltfScene(gltfScene, scene, assetMgr);
```

### Camera Controller

```cpp
#include "engine/camera/camera_controller.h"

engine::CameraController cameraCtrl;
cameraCtrl.setMode(engine::CameraController::Mode::Orbit);
cameraCtrl.setTarget(Vec3{0, 0, 0});

// In update loop
cameraCtrl.update(deltaTime, input);
Mat4 viewMatrix = cameraCtrl.getViewMatrix();
```

### Object Picking

```cpp
#include "engine/picking.h"

// Ray from mouse position
auto ray = camera.screenPointToRay(mouseX, mouseY);

// Pick entity
auto result = engine::pickEntity(scene, ray);
if (result.hit) {
    console::info("Picked entity: {}", result.entityId);
    console::info("World position: {}", result.worldPosition);
}
```

## Architecture Notes

### Asset Handle Pattern
The asset manager uses handles rather than raw pointers for memory safety:
```cpp
// Handle is lightweight, copyable, and invalidates if asset unloads
AssetHandle<Mesh> meshHandle = assetMgr.loadMesh(path);
```

### glTF Import Pipeline
1. **Parse**: cgltf parses the JSON/binary file
2. **Load**: Load buffers, textures from disk/data URIs
3. **Convert**: Transform glTF data to engine format
4. **Register**: Register assets with asset manager
5. **Instantiate**: Create ECS entities with components

### Integration Layer
The `integration/` directory contains code that bridges two or more systems:
- Asset manager ↔ glTF loader
- Asset manager ↔ Renderer
- ECS ↔ Asset system

## Testing

Engine systems are tested through both unit tests and integration tests:
- **Unit tests**: Asset resolution, path utilities, data conversion
- **Integration tests**: Asset loading, glTF import, scene creation

See `tests/asset_*_tests.cpp` and `tests/gltf_*_tests.cpp`.

## Performance Considerations

### Asset Loading
- Async loading prevents frame hitches
- Asset caching avoids redundant loads
- Reference counting enables safe unloading

### glTF Import
- Large scenes can take time to import
- Consider streaming for very large assets
- Material compilation can be expensive

### Picking
- BVH acceleration structure speeds up ray tests
- Only test visible entities
- Early-out on first hit for simple selection

## See Also

- `runtime/` - ECS and component systems
- `graphics/material_system/` - Material definitions
- `/docs/scene_format.md` - Scene serialization format
- `/docs/GLTF_TEXTURE_SUPPORT_PLAN.md` - glTF texture implementation details
