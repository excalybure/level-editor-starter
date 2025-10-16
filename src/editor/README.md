# Editor Layer

The editor layer provides all editor-specific functionality, UI, and tools for content creation and scene manipulation.

## Purpose

This layer implements the level editor application that runs on top of the runtime engine. It provides:
- ImGui-based user interface
- Visual manipulation tools (gizmos, selection)
- Editor panels (scene hierarchy, entity inspector, asset browser)
- Command pattern for undo/redo
- Viewport rendering with editor overlays
- Editor-specific input handling

## Key Components

### Core Editor Systems

#### `ui.h/cpp`
Main editor UI and viewport layout management.
- **Key functions**: `editor::initialize()`, `editor::shutdown()`, `editor::beginFrame()`, `editor::endFrame()`
- **Responsibilities**: ImGui docking setup, viewport layout, main menu bar, editor state management
- **Entry point**: Called from `main.cpp`

#### `selection.h/cpp`
Entity selection state management.
- **Key class**: `editor::Selection`
- **Features**: Single/multi-selection, selection events, selection queries
- **Used by**: Scene hierarchy, entity inspector, gizmos, picking

#### `gizmos.h/cpp`
Transform manipulation tools using ImGuizmo.
- **Key functions**: `editor::renderGizmos()`, `editor::handleGizmoInput()`
- **Operations**: Translate, rotate, scale in local/world space
- **Integration**: Works with selection and command system

### Rendering

#### `selection_renderer.h/cpp`
Visual feedback for selected entities.
- **Features**: Outline rendering, selection rectangles
- **Shaders**: `selection_outline.hlsl`, `selection_rect.hlsl`

#### `viewport/`
Viewport rendering and input handling.
- **viewport_input.h/cpp**: Mouse picking, camera controls, viewport interaction
- **Responsibilities**: Ray casting, viewport-to-world transformations

### UI Panels

#### `scene_hierarchy/`
Scene tree panel showing entity hierarchy.
- **Key class**: `editor::SceneHierarchyPanel`
- **Features**: Tree view, drag-drop, entity creation/deletion, selection sync

#### `entity_inspector/`
Entity properties and component editing.
- **Key classes**: `editor::EntityInspectorPanel`, component UI widgets
- **Features**: Component display, property editing, add/remove components
- **Integration**: Uses reflection system for component properties

#### `asset_browser/`
Asset browsing and import interface.
- **Features**: File system browsing, asset preview, drag-drop import
- **Supported formats**: glTF, textures, materials

### Commands

#### `commands/`
Undo/redo infrastructure and command implementations.
- **Base**: Command pattern with execute/undo
- **Built-in commands**: Transform, entity creation/deletion, component modifications
- **Usage**: All editor operations that modify scene state should use commands

#### `transform_commands.h/cpp`
Transform manipulation commands (move, rotate, scale).
- **Key classes**: `TranslateCommand`, `RotateCommand`, `ScaleCommand`
- **Integration**: Used by gizmos and property editors

### Configuration

#### `config/EditorConfig.h/cpp`
Editor settings and preferences.
- **Persistence**: JSON-based configuration file (`editor_config.json`)
- **Settings**: Viewport options, grid settings, gizmo preferences, recent files

## Dependencies

**Depends on**: `core`, `math`, `platform`, `graphics`, `runtime`, `engine`

**External libraries**:
- **ImGui**: UI framework
- **ImGuizmo**: 3D gizmo widgets
- **nlohmann/json**: Configuration serialization

## Usage Example

```cpp
#include "editor/ui.h"
#include "editor/selection.h"
#include "editor/gizmos.h"

// Initialize editor (called from main.cpp)
editor::initialize(device, window);

// Main loop
while (running) {
    editor::beginFrame();
    
    // Handle input
    editor::handleInput();
    
    // Render gizmos
    editor::renderGizmos(camera, viewProjection);
    
    // Render UI panels
    editor::renderUI();
    
    editor::endFrame();
}

editor::shutdown();
```

## Architecture Notes

### Separation of Concerns
- **UI rendering**: ImGui widgets and panels
- **Visual feedback**: Selection renderer, gizmos
- **State management**: Selection, command history
- **Input handling**: Viewport input, keyboard shortcuts

### Command Pattern
All user actions that modify state should go through the command system:
```cpp
auto cmd = std::make_unique<TranslateCommand>(entityId, deltaPosition);
editor::executeCommand(std::move(cmd));  // Supports undo/redo
```

### Selection System
The selection system is the central authority for selected entities:
```cpp
auto& selection = editor::getSelection();
selection.select(entityId);
selection.selectMultiple({id1, id2, id3});
if (selection.contains(entityId)) { /* ... */ }
```

## Testing

Editor code that depends on ImGui context is typically tested through integration tests rather than unit tests. However, logic components (commands, selection state, config) should have unit tests.

See `tests/editor_*_tests.cpp` for examples.

## See Also

- `runtime/` - Scene and ECS that editor manipulates
- `engine/assets/` - Asset management system
- `graphics/renderer/` - Rendering primitives used by editor
- `/docs/adding_objects_guide.md` - Guide for adding new object types
