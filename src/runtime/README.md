# Runtime Layer

The runtime layer provides the core game runtime systems including the Entity-Component-System (ECS), scene management, component definitions, and runtime systems.

## Purpose

This layer is the foundation for game logic and scene representation:
- Entity-Component-System architecture
- Core game components (Transform, Mesh, Camera, etc.)
- Runtime systems (rendering, animation, physics integration points)
- Scene serialization and deserialization
- Application lifecycle management

## Key Components

### Entity-Component-System

#### `ecs.h/cpp`
Core ECS implementation.
- **Namespace**: `ecs::`
- **Key types**: `Entity` (ID type), `Scene` (entity container)
- **Features**: Fast component access, entity lifecycle, component iteration
- **Concepts**: `Component` concept for compile-time validation
- **Usage**: All game objects are entities with components

#### `entity.h`
Entity ID type and utilities.
- **Type**: `using Entity = uint64_t;`
- **Special values**: `kInvalidEntity`, `kRootEntity`
- **Features**: Entity generation, validity checking

#### `components.h`
Core component definitions.
- **Components**:
  - `TransformComponent`: Position, rotation, scale
  - `MeshComponent`: Geometry and material references
  - `CameraComponent`: Camera projection parameters
  - `NameComponent`: Entity name for editor
  - `ParentComponent`: Hierarchy relationships
  - `AnimationComponent`: Animation state
  - `LightComponent`: Light properties
- **Note**: Components are plain data structures (POD preferred)

### Systems

#### `systems.h`
System trait and update interface.
- **Pattern**: Systems operate on entities with specific component sets
- **Update order**: Defined by system dependencies
- **Features**: Parallel system execution (future)

#### `mesh_rendering_system.h/cpp`
Renders all entities with mesh components.
- **Key function**: `runtime::updateMeshRenderingSystem()`
- **Process**: Collect meshes → cull → sort → submit to GPU
- **Integration**: Works with graphics layer for rendering

### Scene Management

#### `scene_serialization/`
Scene save/load functionality.
- **Format**: JSON-based scene format
- **Features**: Entity serialization, component data, asset references
- **Usage**: Save editor scenes, load game levels

#### `scene_importer.h/cpp`
Import external scenes into the ECS.
- **Formats**: glTF scenes (via engine layer)
- **Process**: Convert external format → create entities → assign components

### Application

#### `app.h`
Application lifecycle and main loop abstractions.
- **Key class**: `runtime::Application`
- **Lifecycle**: `init()` → `update()` → `shutdown()`
- **Integration**: Platform layer creates windows, runtime manages app state

## Dependencies

**Depends on**: `core`, `math`, `graphics`  
**Used by**: `engine`, `editor`

## Directory Structure

```
runtime/
├── ecs.h/cpp                      # Core ECS implementation
├── entity.h                       # Entity ID type
├── components.h                   # Component definitions
├── systems.h                      # System interfaces
├── mesh_rendering_system.h/cpp   # Mesh rendering system
├── scene_importer.h/cpp          # Scene import
├── app.h                         # Application lifecycle
└── scene_serialization/          # Scene save/load
    ├── scene_serializer.h/cpp
    └── scene_deserializer.h/cpp
```

## Usage Examples

### Creating Entities

```cpp
#include "runtime/ecs.h"
#include "runtime/components.h"

// Create scene
ecs::Scene scene;

// Create entity
const auto entity = scene.createEntity();

// Add components
scene.addComponent<NameComponent>(entity, "My Entity");
scene.addComponent<TransformComponent>(entity, 
    Vec3{0, 0, 0},  // position
    Quat::identity(), // rotation
    Vec3{1, 1, 1}   // scale
);
```

### Iterating Components

```cpp
// Iterate all entities with Transform and Mesh
scene.view<TransformComponent, MeshComponent>().each(
    [](auto entity, auto& transform, auto& mesh) {
        // Process entity...
    }
);
```

### Systems

```cpp
#include "runtime/mesh_rendering_system.h"

// Update mesh rendering system
void updateFrame(float deltaTime) {
    // Update all systems
    runtime::updateMeshRenderingSystem(scene, camera, deltaTime);
    // ... other systems
}
```

### Scene Serialization

```cpp
#include "runtime/scene_serialization/scene_serializer.h"

// Save scene
runtime::SceneSerializer serializer;
serializer.serialize(scene, "scenes/level1.json");

// Load scene
runtime::SceneDeserializer deserializer;
auto loadedScene = deserializer.deserialize("scenes/level1.json");
```

## Architecture Notes

### ECS Design Philosophy
- **Entities**: Just IDs, no behavior
- **Components**: Pure data, no logic
- **Systems**: Pure logic, operate on component data
- **Benefits**: Cache-friendly, parallelizable, data-driven

### Component Guidelines
Components should be:
- Small and focused (single responsibility)
- POD when possible (trivially copyable)
- Serializable
- Const-correct member access

Good component:
```cpp
struct TransformComponent {
    Vec3 position{0, 0, 0};
    Quat rotation = Quat::identity();
    Vec3 scale{1, 1, 1};
    
    Mat4 getLocalMatrix() const;  // Computed, not stored
};
```

### Entity Hierarchy
Parent-child relationships use `ParentComponent`:
```cpp
// Child entity
scene.addComponent<ParentComponent>(childEntity, parentEntity);

// Traverse hierarchy
auto* parent = scene.tryGetComponent<ParentComponent>(entity);
if (parent && parent->parentId != kInvalidEntity) {
    // Has parent...
}
```

### System Update Order
Systems should update in dependency order:
1. Input systems
2. Animation/Physics systems
3. Transform hierarchy update
4. Camera systems
5. Rendering systems

## Performance Considerations

### Component Storage
- Components stored in contiguous arrays (cache-friendly)
- Entity iteration is fast (linear memory access)
- Random component access has small overhead (map lookup)

### System Parallelization
Future enhancement: systems can run in parallel if they don't share components:
```cpp
// Can run in parallel
updateAnimationSystem();  // Writes AnimationComponent
updatePhysicsSystem();    // Writes PhysicsComponent
```

### Component Size
Keep components small:
- Large data (mesh geometry) should be referenced by handle
- Transform = 40 bytes ✓
- Entire mesh = 100KB ✗

## Testing

Runtime systems are tested through unit tests and integration tests:
- **Unit tests**: ECS operations, component access, entity lifecycle
- **Integration tests**: System updates, scene serialization

See `tests/ecs_tests.cpp`, `tests/components_tests.cpp`, `tests/scene_*_tests.cpp`.

## Common Patterns

### Component Access Pattern
```cpp
// Check if entity has component
if (auto* transform = scene.tryGetComponent<TransformComponent>(entity)) {
    // Use transform...
}

// Get component (asserts if not present)
auto& mesh = scene.getComponent<MeshComponent>(entity);
```

### Entity Creation Pattern
```cpp
// Helper to create common entity types
Entity createMeshEntity(Scene& scene, const char* name, 
                       MeshHandle mesh, MaterialHandle material) {
    const auto entity = scene.createEntity();
    scene.addComponent<NameComponent>(entity, name);
    scene.addComponent<TransformComponent>(entity);
    scene.addComponent<MeshComponent>(entity, mesh, material);
    return entity;
}
```

### System Pattern
```cpp
void updateMySystem(Scene& scene, float deltaTime) {
    // Process all entities with required components
    scene.view<MyComponent, TransformComponent>().each(
        [&](Entity entity, MyComponent& my, TransformComponent& transform) {
            // System logic...
            my.value += deltaTime;
            transform.position.x += my.velocity * deltaTime;
        }
    );
}
```

## See Also

- `engine/` - Higher-level engine features built on runtime
- `editor/` - Editor uses runtime for scene manipulation
- `/docs/scene_format.md` - Scene serialization format details
- `/docs/adding_objects_guide.md` - Guide for adding new entity types
