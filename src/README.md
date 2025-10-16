# Source Code Architecture

This is the main source directory for the Level Editor project. The codebase is organized into distinct layers with clear dependency relationships.

## Directory Structure

```
src/
├── core/          # Zero-dependency utilities (console, time, strings)
├── math/          # Mathematical types and operations  
├── platform/      # OS and API abstractions (Win32, DirectX 12, PIX)
├── graphics/      # Rendering systems (GPU resources, materials, shaders)
├── runtime/       # Game runtime systems (ECS, components, scene)
├── engine/        # Asset management and high-level engine features
├── editor/        # Editor-specific UI and tools
└── main.cpp       # Application entry point
```

## Dependency Hierarchy

The layers follow a strict dependency order (lower layers cannot depend on higher layers):

```
Layer 0: core, math (no dependencies)
    ↓
Layer 1: platform (depends on: core, math)
    ↓
Layer 2: graphics (depends on: core, math, platform)
    ↓
Layer 3: runtime (depends on: core, math, graphics)
    ↓
Layer 4: engine (depends on: core, math, graphics, runtime)
    ↓
Layer 5: editor (depends on: all lower layers)
```

## Key Systems Overview

### Core Layer
- **Console**: Colored logging with multiple severity levels
- **Time**: High-resolution timing utilities
- **Strings**: String manipulation helpers

### Math Layer
- Vectors, matrices, quaternions
- Curves and geometric operations
- Transform utilities

### Platform Layer
- **dx12/**: DirectX 12 device management and wrappers
- **win32/**: Windows window creation and message handling
- **pix/**: PIX profiling integration

### Graphics Layer
- **gpu/**: GPU resource management (buffers, textures, descriptors)
- **material_system/**: Data-driven material system
- **shader_manager/**: Shader compilation and management
- **renderer/**: Immediate mode rendering utilities
- **grid/**: Grid rendering for editor viewport

### Runtime Layer
- **ECS**: Entity-component system
- **Components**: Transform, mesh, camera, etc.
- **Systems**: Mesh rendering, animation, etc.
- **Scene serialization**: Save/load scene data

### Engine Layer
- **assets/**: Asset management and resource loading
- **gltf_loader/**: glTF 2.0 file parsing and import
- **camera/**: Camera controller and systems
- **picking/**: Ray-based object selection
- **integration/**: Integration between engine systems

### Editor Layer
- **ui.h**: Main editor UI and viewport layout
- **selection**: Entity selection management
- **gizmos**: Transform manipulation tools (ImGuizmo)
- **commands/**: Undo/redo command pattern infrastructure
- **viewport/**: Viewport rendering and input handling
- **scene_hierarchy/**: Scene tree panel
- **entity_inspector/**: Entity properties panel
- **asset_browser/**: Asset browser panel
- **config/**: Editor configuration and settings

## Entry Point

`main.cpp` initializes all systems and runs the main application loop:
1. Window creation
2. DirectX 12 device initialization
3. Editor UI setup
4. Main loop (input → update → render)
5. Cleanup and shutdown

## Coding Standards

- **Language**: C++23 (concepts, ranges, `std::expected`, etc.)
- **Naming**: 
  - Types/Classes: `PascalCase`
  - Functions: `camelCase`
  - Variables: `camelCase`
  - Members: `m_camelCase`
  - Constants: `kPascalCase`
- **Const correctness**: All non-mutating functions and locals must be `const`
- **Error handling**: Prefer `std::expected` or `console::error` over exceptions
- **Dependencies**: Respect the layer hierarchy strictly

## Testing

Unit tests are located in the `tests/` directory at the project root. Use Catch2 for all unit tests.

Run tests:
```cmd
D:\cod_users\setienne\level-editor-starter\build\vs2022-x64\Debug\unit_test_runner.exe
```

## See Also

- Each subdirectory has its own `README.md` with detailed documentation
- `/docs/` - Design documents and architectural plans
- `/specs/` - Feature specifications
- `/.github/instructions/` - Coding guidelines for AI assistants
