# 📋 Milestone 2: Scene & Editing - Implementation Plan

## 🎯 Overview

Build upon the completed multi-viewport camera system to implement a complete scene editing framework with entity-component-system (ECS), glTF model importing, object picking, gizmo manipulations, and undo/redo functionality.

---

## 🏗️ Architecture Goals

- **ECS Foundation**: Expand the minimal ECS to support transforms, hierarchies, and component management
- **Asset Pipeline**: Implement glTF 2.0 import with mesh, material, and scene hierarchy loading
- **Interactive Editing**: Add 3D gizmo manipulations (translate, rotate, scale) with visual feedback
- **Object Selection**: Implement ray-casting based picking system with selection management
- **History System**: Build robust undo/redo command pattern for all editing operations
- **UI Integration**: Create scene hierarchy panel, inspector panel, and asset browser

---

## 📦 Phase 1: Enhanced ECS System (Week 1-2)

### 1.1 Core ECS Expansion
**Target Module**: `runtime.ecs` enhancement

**Current State Analysis**:
- Basic ECS exists with simple `Entity`, `Storage<T>`, and `Component` concept
- Only supports basic create/get operations
- No hierarchy or transform support

**Implementation Tasks**:


### 1.2 Essential Components
**Target Module**: `runtime.components`

### 1.3 System Architecture
**Target Module**: `runtime.systems`

**Deliverables**:
- ✅ Enhanced ECS with entity recycling and generation tracking
- ✅ Component storage optimization with better iteration
- ✅ Scene management with hierarchy support
- ✅ Core components (Transform, Name, Visible, MeshRenderer, Selected)
- ✅ Transform system with world matrix caching
- ✅ Comprehensive unit tests for all ECS functionality

---

## 📦 Phase 2: glTF Asset Import Pipeline (Week 2-3)

### 2.1 Dependency Integration
**vcpkg.json additions**:

### 2.2 Asset System Foundation
**Target Module**: `engine.assets`

**Design Rationale: Primitive-Based Mesh Architecture**

The asset system is designed around a **primitive-based mesh architecture** that aligns with modern graphics pipelines and the glTF 2.0 specification:

- **glTF Alignment**: Each glTF primitive maps directly to a `Primitive` object, preserving the original structure
- **Material Per Primitive**: Each primitive can have its own material, enabling multi-material objects
- **GPU Efficiency**: Each primitive manages its own vertex/index buffers, allowing for optimized rendering
- **Flexibility**: Meshes can combine primitives with different vertex layouts (e.g., some with/without tangents)
- **Memory Management**: Individual primitives can be loaded/unloaded independently
- **Rendering Pipeline**: Modern renderers can process each primitive with its specific material and shaders

This approach contrasts with the legacy "single vertex buffer per mesh" design, providing better performance and flexibility for complex 3D assets.

### 2.3 glTF Loader Implementation
**Target Module**: `engine.gltf_loader`


### 2.4 Asset Manager
**Target Module**: `engine.asset_manager`


**Deliverables**:
- ✅ cgltf integration for glTF 2.0 parsing
- ✅ Primitive-based asset system with Mesh containing multiple Primitives
- ✅ Each Primitive has its own vertex/index buffers and material reference
- ✅ Asset system with enhanced Material class supporting PBR workflow
- ✅ glTF loader with per-primitive extraction and GPU resource creation
- ✅ Asset manager with primitive-aware caching and import functionality
- ✅ ECS integration for imported scenes with proper primitive handling
- ✅ MeshRenderer component supporting material overrides per primitive
- ✅ Unit tests for primitive-based asset loading pipeline
- ✅ Sample glTF files for testing (cube, suzanne, multi-material scene)

**Key Design Benefits**:
- **True glTF Fidelity**: Preserves original glTF primitive structure
- **Multi-Material Support**: Each primitive can have different materials
- **Rendering Efficiency**: GPU resources organized per-primitive for optimal draw calls
- **Memory Flexibility**: Individual primitive loading/unloading capability
- **Pipeline Compatibility**: Works seamlessly with modern rendering architectures

---

## 📦 Phase 2.5: GPU Resource Architecture & Rendering Integration (Week 2.5-3)

### 2.5.1 Current State Analysis
**Problem**: Phase 2 created the asset loading pipeline but left a critical gap between asset data and actual rendering:

- **Asset Side**: `MeshRenderer` stores string paths (`meshPath`, `materialPaths`) requiring runtime lookups
- **GPU Side**: `MeshGPU` exist but are disconnected from the rendering pipeline
- **Performance**: Multiple entities sharing the same mesh create duplicate GPU resources
- **Architecture**: No material GPU resource management for shaders, textures, and rendering state

**Gap**: The system can load assets and create ECS scenes, but cannot efficiently render them.

### 2.5.2 MaterialGPU Implementation
**Target Module**: `engine.material_gpu`

**Design**: Create GPU-ready material resources that handle all rendering state:

### 2.5.3 GPU Resource Manager
**Target Module**: `engine.gpu_resource_manager`

**Design**: Centralized caching and sharing of GPU resources across multiple entities:

### 2.5.4 Enhanced PrimitiveGPU Integration
**Target Module**: `engine.asset_gpu_buffers` enhancement

**Design**: Integrate material references directly into primitive GPU buffers:

### 2.5.5 Streamlined MeshRenderer Component
**Target Module**: `runtime.components` enhancement

**Design**: Remove string paths and direct GPU resource references for efficiency:

### 2.5.6 Scene Importer Module
**Target Module**: `runtime.scene_importer`

**Design**: Dedicated module for converting `assets::Scene` to `ecs::Scene` with support for both GPU and non-GPU scenarios:

**Integration with AssetManager**: The existing callback system will use this module:

**Benefits**:
- **Architectural Clarity**: Dedicated module for asset-to-ECS conversion
- **Testing Support**: Maintains fast unit tests without GPU overhead  
- **Performance**: GPU path eliminates string lookups during rendering
- **Flexibility**: Applications choose appropriate import method
- **Maintainability**: All conversion logic centralized and testable
- **Incremental Migration**: Existing tests continue working while adding GPU features

### 2.5.7 Updated ECS Import Integration
**Target Module**: `engine.asset_manager` enhancement

**Design**: Integration with GPU resource manager during ECS import:

**Deliverables**:
- ✅ MaterialGPU class with complete rendering state management
- ✅ GPUResourceManager for efficient resource sharing and caching
- ✅ Enhanced PrimitiveGPU with material integration
- ✅ Streamlined MeshRenderer component with direct GPU resource references
- ✅ Updated ECS import system with GPU resource creation
- ✅ Performance optimization: eliminated string lookups in rendering path
- ✅ Memory optimization: shared GPU resources across multiple entities
- ✅ Architectural clean-up: clear separation between asset data and GPU resources

**Key Benefits**:
- **Performance**: Direct GPU resource access eliminates runtime string lookups
- **Memory Efficiency**: Multiple entities sharing meshes use the same GPU buffers
- **Rendering Ready**: Complete pipeline from asset loading to GPU rendering
- **Maintainable**: Clear ownership and lifecycle management of GPU resources
- **Scalable**: Resource caching supports large scenes with many entities

---

## 📦 Phase 3: Object Picking & Selection System (Week 3-4)

### 3.1 Ray-Casting Infrastructure
**Target Module**: `engine.picking`

**Current State Analysis**:
- Basic ray generation exists in `Viewport::getPickingRay()`
- Math library has comprehensive 3D geometry support
- Need to integrate with ECS for object selection

### 3.2 Selection Management
**Target Module**: `editor.selection`

**Deliverables**:
- ✅ Ray-casting system using existing math library
- ✅ HitResult structure with detailed intersection info
- ✅ Picking system integrated with ECS
- ✅ Selection manager with multi-selection support
- ✅ Mouse picking handler with rectangle selection
- ✅ Selection visualization (outline rendering)
- ✅ Unit tests for all picking functionality

---

## 📦 Phase 4: 3D Gizmo Manipulation System (Week 4-5)

### 4.1 ImGuizmo Integration
**Target Module**: `editor.gizmos`

**Current State Analysis**:
- vcpkg has ImGuizmo port available
- Current UI system uses ImGui with D3D12 backend
- Viewport system provides camera matrices

### 4.2 Transform Command System (Preparation for Undo/Redo)
**Target Module**: `editor.transform_commands`

**Deliverables**:
- ✅ ImGuizmo integration with D3D12 backend
- ✅ Gizmo system with translate/rotate/scale operations
- ✅ Local vs world space transformation modes
- ✅ Snap-to-grid functionality with configurable values
- ✅ Multi-selection gizmo support
- ✅ Gizmo UI controls toolbar
- ✅ Transform commands ready for undo/redo integration
- ✅ Visual feedback during manipulation

---

## 📦 Phase 5: Undo/Redo Command Stack (Week 5-6)

### 5.1 Command Pattern Infrastructure
**Target Module**: `editor.commands`

```cpp
export module editor.commands;

import std;

export namespace editor {

// Forward declarations
export class Command;

// Command execution context
export struct CommandContext {
    std::string description;
    std::chrono::system_clock::time_point timestamp;
    size_t memoryUsage = 0; // For memory management
};

// Enhanced command interface
export class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual std::string getDescription() const = 0;
    virtual size_t getMemoryUsage() const { return sizeof(*this); }
    virtual bool canMergeWith(const Command& other) const { return false; }
    virtual std::unique_ptr<Command> mergeWith(std::unique_ptr<Command> other) { return nullptr; }
};

// Macro command for batching operations
export class MacroCommand : public Command {
public:
    MacroCommand(const std::string& description);
    
    void addCommand(std::unique_ptr<Command> command);
    void execute() override;
    void undo() override;
    std::string getDescription() const override;
    size_t getMemoryUsage() const override;
    
private:
    std::string m_description;
    std::vector<std::unique_ptr<Command>> m_commands;
};

// Command history manager
export class CommandHistory {
public:
    CommandHistory(size_t maxHistorySize = 100, size_t maxMemoryMB = 100);
    
    // Command execution
    void executeCommand(std::unique_ptr<Command> command);
    
    // Undo/Redo operations
    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();
    
    // History management
    void clear();
    size_t getHistorySize() const;
    size_t getCurrentIndex() const;
    std::string getCurrentCommandDescription() const;
    std::vector<std::string> getHistoryDescriptions() const;
    
    // Memory management
    size_t getCurrentMemoryUsage() const;
    void trimHistory();
    
    // Command merging (for continuous operations)
    void enableMerging(bool enable) { m_mergingEnabled = enable; }
    bool isMergingEnabled() const { return m_mergingEnabled; }
    
    // Event notifications
    void registerUndoRedoListener(std::function<void()> callback);
    
private:
    std::vector<std::unique_ptr<Command>> m_commands;
    std::vector<CommandContext> m_contexts;
    size_t m_currentIndex = 0;
    size_t m_maxHistorySize;
    size_t m_maxMemoryBytes;
    bool m_mergingEnabled = true;
    
    std::vector<std::function<void()>> m_listeners;
    
    void notifyListeners();
    void removeOldCommands();
    bool tryMergeWithLast(std::unique_ptr<Command>& command);
};

} // namespace editor
```

### 5.2 ECS-Specific Commands
**Target Module**: `editor.ecs_commands`

```cpp
export module editor.ecs_commands;

import editor.commands;
import runtime.ecs;
import runtime.components;
import std;

export namespace editor {

// Entity creation command
export class CreateEntityCommand : public Command {
public:
    CreateEntityCommand(ecs::Scene& scene, const std::string& name = "Entity");
    
    void execute() override;
    void undo() override;
    std::string getDescription() const override;
    
    ecs::Entity getCreatedEntity() const { return m_entity; }
    
private:
    ecs::Scene& m_scene;
    std::string m_name;
    ecs::Entity m_entity{};
    bool m_executed = false;
};

// Entity deletion command
export class DeleteEntityCommand : public Command {
public:
    DeleteEntityCommand(ecs::Scene& scene, ecs::Entity entity);
    
    void execute() override;
    void undo() override;
    std::string getDescription() const override;
    
private:
    struct EntityData {
        ecs::Entity entity;
        components::Name name;
        components::Transform transform;
        components::Visible visible;
        std::optional<components::MeshRenderer> meshRenderer;
        std::optional<components::Hierarchy> hierarchy;
    };
    
    ecs::Scene& m_scene;
    EntityData m_entityData;
    bool m_executed = false;
    
    void captureEntityData();
    void restoreEntityData();
};

// Component add/remove commands
export template<typename T>
class AddComponentCommand : public Command {
public:
    AddComponentCommand(ecs::Scene& scene, ecs::Entity entity, T component)
        : m_scene(scene), m_entity(entity), m_component(component) {}
    
    void execute() override {
        m_scene.addComponent(m_entity, m_component);
    }
    
    void undo() override {
        m_scene.removeComponent<T>(m_entity);
    }
    
    std::string getDescription() const override {
        return "Add " + std::string(typeid(T).name()) + " Component";
    }
    
private:
    ecs::Scene& m_scene;
    ecs::Entity m_entity;
    T m_component;
};

export template<typename T>
class RemoveComponentCommand : public Command {
public:
    RemoveComponentCommand(ecs::Scene& scene, ecs::Entity entity)
        : m_scene(scene), m_entity(entity) {
        if (auto* comp = m_scene.getComponent<T>(m_entity)) {
            m_component = *comp;
            m_hadComponent = true;
        }
    }
    
    void execute() override {
        if (m_hadComponent) {
            m_scene.removeComponent<T>(m_entity);
        }
    }
    
    void undo() override {
        if (m_hadComponent) {
            m_scene.addComponent(m_entity, m_component);
        }
    }
    
    std::string getDescription() const override {
        return "Remove " + std::string(typeid(T).name()) + " Component";
    }
    
private:
    ecs::Scene& m_scene;
    ecs::Entity m_entity;
    T m_component{};
    bool m_hadComponent = false;
};

// Hierarchy commands
export class SetParentCommand : public Command {
public:
    SetParentCommand(ecs::Scene& scene, ecs::Entity child, ecs::Entity newParent);
    
    void execute() override;
    void undo() override;
    std::string getDescription() const override;
    
private:
    ecs::Scene& m_scene;
    ecs::Entity m_child;
    ecs::Entity m_newParent;
    ecs::Entity m_oldParent{};
    bool m_hadOldParent = false;
};

} // namespace editor
```

### 5.3 UI Integration
**Target Module**: `editor.command_ui`

```cpp
export module editor.command_ui;

import editor.commands;
import std;

export namespace editor {

// History window for debugging/power users
export class CommandHistoryWindow {
public:
    CommandHistoryWindow(CommandHistory& history);
    
    void render();
    
    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }
    
private:
    CommandHistory& m_history;
    bool m_visible = false;
    
    void renderHistoryList();
    void renderMemoryUsage();
};

// Undo/Redo integration with main UI
export class UndoRedoUI {
public:
    UndoRedoUI(CommandHistory& history);
    
    // Menu integration
    void renderMenuItems();
    
    // Toolbar integration
    void renderToolbarButtons();
    
    // Keyboard shortcuts (to be called by input handler)
    void handleKeyboardShortcuts(bool ctrlPressed, bool shiftPressed, int keyCode);
    
private:
    CommandHistory& m_history;
    
    static constexpr int KEY_Z = 90;
    static constexpr int KEY_Y = 89;
};

} // namespace editor
```

**Deliverables**:
- ✅ Robust command pattern with memory management
- ✅ Command history with configurable size and memory limits  
- ✅ Command merging for continuous operations (smooth gizmo drags)
- ✅ ECS-specific commands (create, delete, modify entities)
- ✅ Transform commands integrated with gizmo system
- ✅ Hierarchy manipulation commands
- ✅ UI integration with Ctrl+Z/Ctrl+Y shortcuts
- ✅ History viewer window for debugging
- ✅ Memory usage tracking and cleanup

---

## 📦 Phase 6: Scene Editing UI (Week 6-7)

### 6.1 Scene Hierarchy Panel
**Target Module**: `editor.scene_hierarchy`

```cpp
export module editor.scene_hierarchy;

import editor.selection;
import editor.commands;
import runtime.ecs;
import runtime.components;
import std;

export namespace editor {

// Scene hierarchy tree view
export class SceneHierarchyPanel {
public:
    SceneHierarchyPanel(ecs::Scene& scene, 
                       SelectionManager& selectionManager,
                       CommandHistory& commandHistory);
    
    void render();
    
    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }
    
private:
    ecs::Scene& m_scene;
    SelectionManager& m_selectionManager;
    CommandHistory& m_commandHistory;
    bool m_visible = true;
    
    // UI state
    ecs::Entity m_draggedEntity{};
    ecs::Entity m_renameEntity{};
    std::string m_renameBuffer;
    
    // Tree rendering
    void renderEntityTree();
    void renderEntityNode(ecs::Entity entity, bool isRoot = false);
    
    // Context menu
    void renderContextMenu(ecs::Entity entity);
    
    // Drag & Drop for reparenting
    void handleDragDrop(ecs::Entity source, ecs::Entity target);
    
    // Entity operations
    void createEntity(ecs::Entity parent = {});
    void deleteEntity(ecs::Entity entity);
    void duplicateEntity(ecs::Entity entity);
    void renameEntity(ecs::Entity entity, const std::string& newName);
};

} // namespace editor
```

### 6.2 Entity Inspector Panel
**Target Module**: `editor.entity_inspector`

```cpp
export module editor.entity_inspector;

import editor.selection;
import editor.commands;
import runtime.ecs;
import runtime.components;
import engine.asset_manager;
import std;

export namespace editor {

// Inspector panel for selected entities
export class EntityInspectorPanel {
public:
    EntityInspectorPanel(ecs::Scene& scene,
                        SelectionManager& selectionManager,
                        CommandHistory& commandHistory,
                        asset_manager::AssetManager& assetManager);
    
    void render();
    
    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }
    
private:
    ecs::Scene& m_scene;
    SelectionManager& m_selectionManager;
    CommandHistory& m_commandHistory;
    asset_manager::AssetManager& m_assetManager;
    bool m_visible = true;
    
    // Component rendering
    void renderEntityHeader(ecs::Entity entity);
    void renderTransformComponent(ecs::Entity entity);
    void renderMeshRendererComponent(ecs::Entity entity);
    void renderVisibleComponent(ecs::Entity entity);
    
    // Component management
    void renderAddComponentButton(ecs::Entity entity);
    void renderComponentContextMenu(ecs::Entity entity, const std::string& componentType);
    
    // Multi-selection support
    void renderMultiSelectionInfo();
    
    // Asset references
    void renderAssetSelector(const std::string& label, 
                           std::string& currentPath,
                           assets::AssetType assetType);
};

// Component UI helpers
export class ComponentUI {
public:
    static bool renderTransform(components::Transform& transform);
    static bool renderName(components::Name& name);
    static bool renderVisible(components::Visible& visible);
    static bool renderMeshRenderer(components::MeshRenderer& meshRenderer,
                                  asset_manager::AssetManager& assetManager);
                                  
private:
    static bool renderVec3Control(const std::string& label, 
                                 math::Vec3<>& values, 
                                 float resetValue = 0.0f,
                                 float speed = 0.1f);
    static bool renderFloatControl(const std::string& label, 
                                  float& value,
                                  float min = 0.0f, float max = 0.0f);
    
    // Primitive-specific UI helpers
    static void renderMeshPrimitiveList(const assets::Mesh& mesh,
                                       std::vector<std::string>& materialOverrides,
                                       asset_manager::AssetManager& assetManager);
    static bool renderMaterialOverrideSlot(const std::string& originalMaterial,
                                          std::string& overrideMaterial,
                                          asset_manager::AssetManager& assetManager);
};

} // namespace editor
```

### 6.3 Asset Browser Panel
**Target Module**: `editor.asset_browser`

```cpp
export module editor.asset_browser;

import engine.asset_manager;
import editor.commands;
import runtime.ecs;
import std;

export namespace editor {

// Asset browser for importing and managing assets
export class AssetBrowserPanel {
public:
    AssetBrowserPanel(asset_manager::AssetManager& assetManager,
                     ecs::Scene& scene,
                     CommandHistory& commandHistory);
    
    void render();
    
    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }
    
    void setRootPath(const std::string& path) { m_rootPath = path; }
    const std::string& getRootPath() const { return m_rootPath; }
    
private:
    asset_manager::AssetManager& m_assetManager;
    ecs::Scene& m_scene;
    CommandHistory& m_commandHistory;
    bool m_visible = true;
    
    std::string m_rootPath = "assets/";
    std::string m_currentPath;
    std::string m_selectedAsset;
    
    // Directory navigation
    void renderPathBar();
    void renderDirectoryTree();
    void renderAssetGrid();
    
    // Asset operations
    void importAsset(const std::string& filePath);
    void deleteAsset(const std::string& filePath);
    void renameAsset(const std::string& oldPath, const std::string& newPath);
    
    // Drag & Drop to scene
    void handleAssetDragDrop();
    
    // Asset preview
    void renderAssetPreview(const std::string& assetPath);
    
    // File system utilities
    std::vector<std::string> getDirectoryContents(const std::string& path);
    bool isDirectory(const std::string& path);
    std::string getFileExtension(const std::string& path);
    assets::AssetType getAssetTypeFromExtension(const std::string& extension);
};

} // namespace editor
```

### 6.4 Integrated Scene Editor
**Target Module**: `editor.scene_editor`

```cpp
export module editor.scene_editor;

import editor.selection;
import editor.commands;
import editor.gizmos;
import editor.scene_hierarchy;
import editor.entity_inspector;
import editor.asset_browser;
import runtime.ecs;
import runtime.systems;
import engine.asset_manager;
import std;

export namespace editor {

// Main scene editor orchestrating all panels
export class SceneEditor {
public:
    SceneEditor(ecs::Scene& scene, 
               systems::SystemManager& systemManager,
               asset_manager::AssetManager& assetManager);
    
    void initialize();
    void shutdown();
    void update(float deltaTime);
    void render();
    
    // Input handling
    void handleKeyboardInput(int keyCode, bool pressed, bool ctrl, bool shift, bool alt);
    void handleMouseInput(const editor::Viewport& viewport, 
                         const math::Vec2<>& mousePos,
                         bool leftButton, bool rightButton, bool middleButton);
    
    // Scene operations
    void newScene();
    void loadScene(const std::string& filePath);
    void saveScene(const std::string& filePath);
    
    // Asset operations
    void importAsset(const std::string& filePath);
    
private:
    ecs::Scene& m_scene;
    systems::SystemManager& m_systemManager;
    asset_manager::AssetManager& m_assetManager;
    
    // Editor systems
    std::unique_ptr<SelectionManager> m_selectionManager;
    std::unique_ptr<CommandHistory> m_commandHistory;
    std::unique_ptr<GizmoSystem> m_gizmoSystem;
    std::unique_ptr<picking::PickingSystem> m_pickingSystem;
    
    // UI panels
    std::unique_ptr<SceneHierarchyPanel> m_hierarchyPanel;
    std::unique_ptr<EntityInspectorPanel> m_inspectorPanel;
    std::unique_ptr<AssetBrowserPanel> m_assetBrowser;
    
    // Editor state
    bool m_sceneModified = false;
    std::string m_currentScenePath;
    
    void setupUI();
    void renderMainMenuBar();
    void renderStatusBar();
    void handleSceneModification();
};

} // namespace editor
```

**Deliverables**:
- ✅ Scene hierarchy panel with drag & drop reparenting
- ✅ Entity inspector with component editing
- ✅ Asset browser with import/preview functionality
- ✅ Integrated scene editor with all panels
- ✅ Context menus for entity operations
- ✅ Multi-selection support in all UI
- ✅ Keyboard shortcuts for common operations
- ✅ Scene save/load functionality (JSON format)

---

## 🧪 Testing & Validation Strategy

### Unit Tests Coverage
- **ECS System**: Entity lifecycle, component management, hierarchy operations
- **Asset Pipeline**: glTF loading, mesh processing, material parsing
- **Picking System**: Ray-object intersection, selection management
- **Command System**: Undo/redo operations, command merging, memory management
- **Transform System**: Matrix calculations, world space transforms

### Integration Tests
- **Complete Workflow**: Import glTF → Select objects → Transform with gizmos → Undo/redo
- **Multi-Viewport**: Selection synchronization across viewports
- **Memory Management**: Asset loading/unloading, command history limits
- **UI Responsiveness**: Large scene performance with thousands of objects

### Sample Assets
- Simple primitives (cube, sphere, plane) for basic testing
- Complex multi-mesh scenes (buildings, vehicles) for stress testing
- Hierarchical scenes with nested transforms
- Multi-material objects for rendering pipeline validation

---

## 📈 Success Metrics

### Functional Requirements
- ✅ Import glTF 2.0 files with meshes, materials, and hierarchy
- ✅ Select objects via ray-casting (mouse picking)
- ✅ Manipulate objects with translate/rotate/scale gizmos
- ✅ Undo/redo all editing operations
- ✅ Scene hierarchy visualization and editing
- ✅ Multi-object selection and transformation
- ✅ Asset management and preview system

### Performance Requirements
- Handle 10,000+ objects in scene hierarchy without UI lag
- Picking system responds within 16ms for complex scenes
- Undo/redo operations complete instantaneously
- Memory usage stays within reasonable bounds (< 1GB for typical scenes)

### Quality Requirements
- All systems have comprehensive unit test coverage (>90%)
- No memory leaks in asset loading/unloading cycles
- Stable operation with complex nested hierarchies
- Graceful error handling for invalid assets

---

## 🔄 Risk Mitigation

### Technical Risks
1. **glTF Complexity**: Start with simple meshes, progressively add features (textures, animations)
2. **Performance**: Implement LOD systems early, profile frequently
3. **Memory Management**: Use smart pointers, implement asset reference counting
4. **UI Complexity**: Build incremental functionality, avoid feature creep

### Integration Risks
1. **ECS Changes**: Maintain backward compatibility, extensive testing
2. **ImGuizmo Integration**: Validate with existing ImGui/D3D12 setup early
3. **Command System**: Test edge cases extensively, implement safeguards

---

## 🚀 Future Extensions (Post-Milestone)

This milestone provides the foundation for advanced features in later milestones:
- **Material Editor**: Node-based material creation system
- **Animation System**: Keyframe animation and timeline editing
- **Terrain System**: Height-based terrain with virtual texturing
- **Physics Integration**: Rigid body dynamics and collision detection
- **Scripting System**: Lua/C# integration for gameplay logic

---

## 📋 Conclusion

Milestone 2 transforms the level editor from a viewport viewer into a fully functional scene editing tool. The modular architecture ensures each system can be developed and tested independently while integrating seamlessly with existing code.

The ECS foundation provides flexibility for future component types, the asset pipeline supports industry-standard glTF workflows, and the command system ensures a professional-grade editing experience with full undo/redo support.

Upon completion, developers will have a solid foundation for building complex 3D scenes with intuitive manipulation tools and robust editing workflows.
