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

```cpp
// Enhanced ECS architecture
export namespace ecs {

// Enhanced Entity with generation for safe handles
export struct Entity {
    std::uint32_t id{};
    std::uint32_t generation{};
    auto operator<=>(const Entity&) const = default;
    bool isValid() const noexcept;
};

// Entity manager with recycling and generation tracking
export class EntityManager {
public:
    Entity create();
    bool destroy(Entity entity);
    bool isValid(Entity entity) const;
    std::span<const Entity> getAllEntities() const;
    
private:
    std::vector<std::uint32_t> m_generations;
    std::vector<Entity> m_entities;
    std::queue<std::uint32_t> m_freeIds;
};

// Enhanced Storage with better performance
export template<Component C>
class ComponentStorage {
public:
    bool add(Entity entity, C component);
    bool remove(Entity entity);
    bool has(Entity entity) const;
    C* get(Entity entity);
    const C* get(Entity entity) const;
    
    // Iteration support
    auto begin() { return m_components.begin(); }
    auto end() { return m_components.end(); }
    
private:
    std::unordered_map<Entity, C> m_components;
};

// Scene management
export class Scene {
public:
    Entity createEntity(const std::string& name = "Entity");
    bool destroyEntity(Entity entity);
    
    template<Component C>
    bool addComponent(Entity entity, C component);
    
    template<Component C>
    bool removeComponent(Entity entity);
    
    template<Component C>
    C* getComponent(Entity entity);
    
    // Hierarchy management
    void setParent(Entity child, Entity parent);
    void removeParent(Entity child);
    Entity getParent(Entity child) const;
    std::vector<Entity> getChildren(Entity parent) const;
    
private:
    EntityManager m_entityManager;
    std::unordered_map<std::type_index, std::unique_ptr<void>> m_componentStorages;
    
    // Hierarchy
    std::unordered_map<Entity, Entity> m_parentMap;
    std::unordered_map<Entity, std::vector<Entity>> m_childrenMap;
};

} // namespace ecs
```

### 1.2 Essential Components
**Target Module**: `runtime.components`

```cpp
export module runtime.components;

import engine.math;
import engine.matrix;
import engine.vec;
import std;

export namespace components {

// Core transform component
export struct Transform {
    math::Vec3<> position{0.0f, 0.0f, 0.0f};
    math::Vec3<> rotation{0.0f, 0.0f, 0.0f}; // Euler angles (radians)
    math::Vec3<> scale{1.0f, 1.0f, 1.0f};
    
    math::Mat4<> getLocalMatrix() const;
    math::Mat4<> getWorldMatrix(const math::Mat4<>& parentMatrix = math::Mat4<>::identity()) const;
};

// Name component for editor display
export struct Name {
    std::string name = "Unnamed";
};

// Visibility control
export struct Visible {
    bool visible = true;
    bool castShadows = true;
    bool receiveShadows = true;
};

// Renderable mesh component
export struct MeshRenderer {
    std::string meshPath;
    std::vector<std::string> materialOverrides; // Optional per-primitive material overrides
    engine::BoundingBox<> bounds; // Local space bounding box
    bool enabled = true;
    
    // Rendering options
    bool castShadows = true;
    bool receiveShadows = true;
    float lodBias = 1.0f; // Level of detail bias for distance culling
};

// Selection state for editor
export struct Selected {
    bool selected = false;
    math::Vec3<> highlightColor{1.0f, 0.8f, 0.2f};
};

} // namespace components
```

### 1.3 System Architecture
**Target Module**: `runtime.systems`

```cpp
export module runtime.systems;

import runtime.ecs;
import runtime.components;
import engine.matrix;

export namespace systems {

// Base system interface
export class System {
public:
    virtual ~System() = default;
    virtual void update(ecs::Scene& scene, float deltaTime) = 0;
    virtual void initialize(ecs::Scene& scene) {}
    virtual void shutdown(ecs::Scene& scene) {}
};

// Transform hierarchy system
export class TransformSystem : public System {
public:
    void update(ecs::Scene& scene, float deltaTime) override;
    
    // Get world transform for entity
    math::Mat4<> getWorldTransform(ecs::Scene& scene, ecs::Entity entity);
    
    // Mark transforms as dirty when changed
    void markDirty(ecs::Entity entity);
    
private:
    std::unordered_set<ecs::Entity> m_dirtyTransforms;
    std::unordered_map<ecs::Entity, math::Mat4<>> m_worldMatrices;
    
    void updateWorldMatrix(ecs::Scene& scene, ecs::Entity entity);
};

// System manager
export class SystemManager {
public:
    template<typename T, typename... Args>
    T* addSystem(Args&&... args);
    
    template<typename T>
    T* getSystem();
    
    void initialize(ecs::Scene& scene);
    void update(ecs::Scene& scene, float deltaTime);
    void shutdown(ecs::Scene& scene);
    
private:
    std::vector<std::unique_ptr<System>> m_systems;
};

} // namespace systems
```

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
```json
{
  "dependencies": [
    // ... existing dependencies
    "cgltf",           // Header-only glTF 2.0 parser
    "imguizmo"         // 3D manipulation gizmos
  ]
}
```

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

```cpp
export module engine.assets;

import std;
import engine.math;
import engine.vec;
import engine.matrix;
import platform.dx12;

export namespace assets {

// Asset types
export enum class AssetType {
    Unknown,
    Mesh,
    Material,
    Texture,
    Scene
};

// Base asset interface
export class Asset {
public:
    virtual ~Asset() = default;
    virtual AssetType getType() const = 0;
    
    const std::string& getPath() const { return m_path; }
    bool isLoaded() const { return m_loaded; }
    
protected:
    std::string m_path;
    bool m_loaded = false;
};

// Material representation
export class Material : public Asset {
public:
    AssetType getType() const override { return AssetType::Material; }
    
    struct PBRMaterial {
        math::Vec4<> baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
        float metallicFactor = 0.0f;
        float roughnessFactor = 1.0f;
        math::Vec3<> emissiveFactor{0.0f, 0.0f, 0.0f};
        
        std::string baseColorTexture;
        std::string metallicRoughnessTexture;
        std::string normalTexture;
        std::string emissiveTexture;
    };
    
    const PBRMaterial& getPBRMaterial() const { return m_pbrMaterial; }
    
private:
    friend class AssetManager;
    PBRMaterial m_pbrMaterial;
};

// Mesh data representation
export struct Vertex {
    math::Vec3<> position;
    math::Vec3<> normal;
    math::Vec2<> texCoord;
    math::Vec4<> tangent; // w component is bitangent handedness
};

// Primitive represents a single drawable unit with its own geometry and material
export class Primitive {
public:
    Primitive() = default;
    Primitive(std::vector<Vertex> vertices, std::vector<std::uint32_t> indices, 
             const std::string& materialPath = "");
    
    // Geometry access
    const std::vector<Vertex>& getVertices() const { return m_vertices; }
    const std::vector<std::uint32_t>& getIndices() const { return m_indices; }
    const engine::BoundingBox<>& getBounds() const { return m_bounds; }
    
    // Material reference
    const std::string& getMaterialPath() const { return m_materialPath; }
    void setMaterialPath(const std::string& path) { m_materialPath = path; }
    
    // D3D12 GPU resources
    ID3D12Resource* getVertexBuffer() const { return m_vertexBuffer.Get(); }
    ID3D12Resource* getIndexBuffer() const { return m_indexBuffer.Get(); }
    const D3D12_VERTEX_BUFFER_VIEW& getVertexBufferView() const { return m_vertexBufferView; }
    const D3D12_INDEX_BUFFER_VIEW& getIndexBufferView() const { return m_indexBufferView; }
    
    // GPU resource management
    bool createGPUResources(dx12::Device* device);
    void releaseGPUResources();
    
private:
    friend class AssetManager;
    std::vector<Vertex> m_vertices;
    std::vector<std::uint32_t> m_indices;
    engine::BoundingBox<> m_bounds;
    std::string m_materialPath;
    
    // GPU resources
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView{};
    
    void calculateBounds();
};

// Mesh contains a collection of primitives, each with its own geometry and material
export class Mesh : public Asset {
public:
    AssetType getType() const override { return AssetType::Mesh; }
    
    // Primitive management
    void addPrimitive(Primitive primitive);
    void addPrimitive(std::vector<Vertex> vertices, std::vector<std::uint32_t> indices, 
                     const std::string& materialPath = "");
    
    size_t getPrimitiveCount() const { return m_primitives.size(); }
    const Primitive& getPrimitive(size_t index) const;
    Primitive& getPrimitive(size_t index);
    const std::vector<Primitive>& getPrimitives() const { return m_primitives; }
    
    // Overall mesh properties
    const engine::BoundingBox<>& getBounds() const { return m_bounds; }
    size_t getTotalVertexCount() const;
    size_t getTotalIndexCount() const;
    
    // Material references (all unique materials used by primitives)
    std::vector<std::string> getUniqueMaterialPaths() const;
    
    // GPU resource management
    bool createGPUResources(dx12::Device* device);
    void releaseGPUResources();
    
private:
    friend class AssetManager;
    std::vector<Primitive> m_primitives;
    engine::BoundingBox<> m_bounds;
    
    void recalculateBounds();
};

// Scene hierarchy from glTF
export struct SceneNode {
    std::string name;
    components::Transform transform;
    std::vector<std::string> meshes;           // Mesh asset paths
    std::vector<std::string> materialOverrides; // Optional material overrides per mesh
    std::vector<std::unique_ptr<SceneNode>> children;
    
    // Node metadata
    bool isVisible = true;
    std::unordered_map<std::string, std::string> userData; // Custom properties from glTF extras
};

export class Scene : public Asset {
public:
    AssetType getType() const override { return AssetType::Scene; }
    
    const std::vector<std::unique_ptr<SceneNode>>& getRootNodes() const { 
        return m_rootNodes; 
    }
    
private:
    friend class AssetManager;
    std::vector<std::unique_ptr<SceneNode>> m_rootNodes;
};

} // namespace assets
```

### 2.3 glTF Loader Implementation
**Target Module**: `engine.gltf_loader`

```cpp
export module engine.gltf_loader;

import engine.assets;
import std;
import <cgltf.h>;

export namespace gltf_loader {

export class GLTFLoader {
public:
    GLTFLoader(dx12::Device* device);
    
    // Load complete glTF scene
    std::unique_ptr<assets::Scene> loadScene(const std::string& filePath);
    
    // Load individual assets
    std::unique_ptr<assets::Mesh> loadMesh(const cgltf::mesh& gltfMesh, 
                                          const cgltf::model& model);
    std::unique_ptr<assets::Material> loadMaterial(const cgltf::material& gltfMaterial);
    
    // Primitive extraction (core functionality)
    assets::Primitive extractPrimitive(const cgltf::primitive& gltfPrimitive,
                                      const cgltf::model& model);
    
private:
    dx12::Device* m_device;
    
    // Helper functions
    assets::Vertex extractVertex(const cgltf::model& model, 
                                const cgltf::primitive& primitive, 
                                size_t index);
    std::vector<std::uint32_t> extractIndices(const cgltf::primitive& primitive,
                                             const cgltf::model& model);
    std::string getMaterialPath(const cgltf::primitive& primitive);
    
    components::Transform extractTransform(const cgltf::node& node);
    std::unique_ptr<assets::SceneNode> processNode(const cgltf::node& node, 
                                                  const cgltf::model& model);
    
    // Buffer/accessor utilities
    template<typename T>
    std::vector<T> extractAttributeData(const cgltf::accessor& accessor,
                                       const cgltf::model& model);
    
    // GPU resource creation
    bool createGPUResourcesForPrimitive(assets::Primitive& primitive);
};
};

} // namespace gltf_loader
```

### 2.4 Asset Manager
**Target Module**: `engine.asset_manager`

```cpp
export module engine.asset_manager;

import engine.assets;
import engine.gltf_loader;
import std;

export namespace asset_manager {

export class AssetManager {
public:
    AssetManager(dx12::Device* device);
    
    // Asset loading
    template<typename T>
    std::shared_ptr<T> load(const std::string& path);
    
    // Asset retrieval
    template<typename T>
    std::shared_ptr<T> get(const std::string& path);
    
    // Asset management
    void unload(const std::string& path);
    void unloadAll();
    
    // Import glTF scene into ECS
    std::vector<ecs::Entity> importScene(const std::string& gltfPath, 
                                        ecs::Scene& scene);
    
    // Primitive-specific operations
    std::shared_ptr<assets::Mesh> createMeshFromPrimitives(
        const std::vector<assets::Primitive>& primitives,
        const std::string& meshName = "");
    
    // Material management for primitives
    void preloadMaterialsForMesh(const assets::Mesh& mesh);
    std::shared_ptr<assets::Material> getMaterialForPrimitive(
        const assets::Mesh& mesh, size_t primitiveIndex);
    
private:
    dx12::Device* m_device;
    gltf_loader::GLTFLoader m_gltfLoader;
    
    std::unordered_map<std::string, std::shared_ptr<assets::Asset>> m_assets;
    
    // Import helpers
    ecs::Entity createEntityFromNode(const assets::SceneNode& node, 
                                    ecs::Scene& scene, 
                                    ecs::Entity parent = {});
    
    // Primitive processing
    void setupMeshRendererForNode(ecs::Entity entity, 
                                 const assets::SceneNode& node,
                                 ecs::Scene& scene);
};

} // namespace asset_manager
```

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

```cpp
export module engine.material_gpu;

import platform.dx12;
import engine.assets;
import engine.shader_manager;
import std;

export namespace engine::gpu {

// GPU material resource management
export class MaterialGPU {
public:
    MaterialGPU(dx12::Device& device, const assets::Material& material);
    ~MaterialGPU() = default;

    // No copy/move for resource management simplicity
    MaterialGPU(const MaterialGPU&) = delete;
    MaterialGPU& operator=(const MaterialGPU&) = delete;

    // Rendering pipeline binding
    void bindToCommandList(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex);
    
    // Resource accessors
    ID3D12PipelineState* getPipelineState() const { return m_pipelineState.Get(); }
    ID3D12Resource* getConstantBuffer() const { return m_constantBuffer.Get(); }
    
    // Texture management
    std::span<const Microsoft::WRL::ComPtr<ID3D12Resource>> getTextures() const;
    D3D12_GPU_DESCRIPTOR_HANDLE getTextureDescriptorHeap() const;
    
    // Validation
    bool isValid() const noexcept;

private:
    dx12::Device& m_device;
    
    // Core rendering resources
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_textureDescriptorHeap;
    
    // Texture resources
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_textures;
    
    // Material constant buffer data
    struct MaterialConstants {
        math::Vec4<> baseColorFactor;
        math::Vec3<> emissiveFactor;
        float metallicFactor;
        float roughnessFactor;
        // Texture binding flags, etc.
    };
    
    void createPipelineState(const assets::Material& material);
    void createConstantBuffer(const assets::Material& material);
    void loadTextures(const assets::Material& material);
};

} // namespace engine::gpu
```

### 2.5.3 GPU Resource Manager
**Target Module**: `engine.gpu_resource_manager`

**Design**: Centralized caching and sharing of GPU resources across multiple entities:

```cpp
export module engine.gpu_resource_manager;

import engine.asset_gpu_buffers;
import engine.material_gpu;
import engine.assets;
import platform.dx12;
import std;

export namespace engine::gpu_resource_manager {

// Centralized GPU resource caching and management
export class GPUResourceManager {
public:
    GPUResourceManager(dx12::Device& device);
    ~GPUResourceManager() = default;

    // Mesh GPU resource management
    std::shared_ptr<engine::gpu::MeshGPU> getMeshGPU(
        const std::string& meshAssetPath);
    std::shared_ptr<engine::gpu::MeshGPU> getMeshGPU(
        std::shared_ptr<assets::Mesh> mesh);
    
    // Material GPU resource management  
    std::shared_ptr<engine::gpu::MaterialGPU> getMaterialGPU(
        const std::string& materialAssetPath);
    std::shared_ptr<engine::gpu::MaterialGPU> getMaterialGPU(
        std::shared_ptr<assets::Material> material);
    
    // Cache management
    void clearCache();
    void unloadUnusedResources(); // Remove resources with only 1 reference
    
    // Statistics
    size_t getMeshResourceCount() const;
    size_t getMaterialResourceCount() const;
    size_t getMemoryUsageMB() const;

private:
    dx12::Device& m_device;
    
    // Resource caches with automatic sharing
    std::unordered_map<std::string, std::weak_ptr<asset_gpu_buffers::MeshGPU>> m_meshCache;
    std::unordered_map<std::string, std::weak_ptr<material_gpu::MaterialGPU>> m_materialCache;
    
    // Asset integration for loading
    assets::AssetManager* m_assetManager = nullptr;
    
    void cleanupExpiredReferences();
};

} // namespace gpu_resource_manager
```

### 2.5.4 Enhanced PrimitiveGPU Integration
**Target Module**: `engine.asset_gpu_buffers` enhancement

**Design**: Integrate material references directly into primitive GPU buffers:

```cpp
// Updated PrimitiveGPU class
export class PrimitiveGPU {
public:
    PrimitiveGPU(dx12::Device& device, 
                      const assets::Primitive& primitive,
                      std::shared_ptr<material_gpu::MaterialGPU> material);

    // Geometry resources (existing)
    D3D12_VERTEX_BUFFER_VIEW getVertexBufferView() const noexcept;
    D3D12_INDEX_BUFFER_VIEW getIndexBufferView() const noexcept;
    
    // NEW: Material integration
    std::shared_ptr<material_gpu::MaterialGPU> getMaterial() const { return m_material; }
    void setMaterial(std::shared_ptr<material_gpu::MaterialGPU> material) { m_material = material; }
    
    // Complete rendering setup
    void bindForRendering(ID3D12GraphicsCommandList* commandList, 
                         UINT materialRootParameterIndex);

private:
    // Existing geometry resources...
    std::shared_ptr<material_gpu::MaterialGPU> m_material;
};
```

### 2.5.5 Streamlined MeshRenderer Component
**Target Module**: `runtime.components` enhancement

**Design**: Remove string paths and direct GPU resource references for efficiency:

```cpp
// Updated MeshRenderer component
export struct MeshRenderer {
    // Direct GPU resource references (no string lookups)
    std::shared_ptr<asset_gpu_buffers::MeshGPU> gpuBuffers;
    
    // Rendering properties
    math::BoundingBox3Df bounds; // For frustum culling
    float lodBias = 1.0f;        // Level of detail control
    
    // Removed: meshPath, materialPaths, enabled (handled by component presence)
    
    MeshRenderer() = default;
    MeshRenderer(std::shared_ptr<asset_gpu_buffers::MeshGPU> buffers)
        : gpuBuffers(std::move(buffers)) {}
};
```

### 2.5.6 Scene Importer Module
**Target Module**: `runtime.scene_importer`

**Design**: Dedicated module for converting `assets::Scene` to `ecs::Scene` with support for both GPU and non-GPU scenarios:

```cpp
export module runtime.scene_importer;

import std;
import engine.assets;
import runtime.ecs;
import runtime.components;
import engine.gpu_resource_manager;

export namespace scene_importer {

export class SceneImporter {
public:
    // For tests, headless servers, editor tools (current approach)
    static void importScene(std::shared_ptr<assets::Scene> assetScene, 
                           ecs::Scene& ecsScene);
    
    // For rendering applications with GPU resources (future approach)
    static void importSceneWithGPU(std::shared_ptr<assets::Scene> assetScene,
                                  ecs::Scene& ecsScene,
                                  gpu_resource_manager::GPUResourceManager& gpuManager);

private:
    // Shared logic for both approaches
    static ecs::Entity importNode(const assets::SceneNode& node, 
                                 ecs::Scene& scene, 
                                 ecs::Entity parent = {});
    
    // Transform component setup (shared)
    static void setupTransformComponent(const assets::SceneNode& node,
                                       ecs::Entity entity,
                                       ecs::Scene& scene);
    
    // GPU-enabled component setup (optimized rendering path)
    static void setupMeshRendererWithGPU(const assets::SceneNode& node,
                                        ecs::Entity entity,
                                        ecs::Scene& scene,
                                        gpu_resource_manager::GPUResourceManager& gpuManager);
    
    // String path component setup (compatibility/testing path)
    static void setupMeshRendererPaths(const assets::SceneNode& node,
                                      ecs::Entity entity,
                                      ecs::Scene& scene,
                                      const std::string& scenePath);
    
    // Hierarchy processing (shared)
    static void processHierarchy(const assets::SceneNode& node,
                                ecs::Entity entity,
                                ecs::Scene& scene);
};

} // namespace scene_importer
```

**Integration with AssetManager**: The existing callback system will use this module:

```cpp
// For tests and headless scenarios
AssetManager::setImportSceneCallback([](auto assetScene, auto& ecsScene) {
    scene_importer::SceneImporter::importScene(assetScene, ecsScene);
});

// For production rendering with GPU resources
AssetManager::setImportSceneCallback([&gpuManager](auto assetScene, auto& ecsScene) {
    scene_importer::SceneImporter::importSceneWithGPU(assetScene, ecsScene, gpuManager);
});
```

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

```cpp
// Enhanced AssetManager with GPU resource integration
export class AssetManager {
public:
    // Existing API...
    
    // NEW: GPU resource integration
    void setGPUResourceManager(gpu_resource_manager::GPUResourceManager* manager);
    
    // Enhanced ECS import with GPU resources
    bool importSceneWithGPUResources(const std::string& path, ecs::Scene& ecsScene);

private:
    gpu_resource_manager::GPUResourceManager* m_gpuResourceManager = nullptr;
    
    // Updated import callback with GPU resources
    static ImportSceneWithGPUCallback s_importSceneWithGPUCallback;
};

// Updated import callback signature
using ImportSceneWithGPUCallback = std::function<void(
    std::shared_ptr<assets::Scene>, 
    ecs::Scene&,
    gpu_resource_manager::GPUResourceManager&
)>;
```

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

```cpp
export module engine.picking;

import engine.math_3d;
import engine.bounding_volumes;
import runtime.ecs;
import runtime.components;
import runtime.systems;

export namespace picking {

// Ray-object intersection results
export struct HitResult {
    bool hit = false;
    float distance = std::numeric_limits<float>::max();
    ecs::Entity entity{};
    math::Vec3<> worldPosition;
    math::Vec3<> localPosition;
    math::Vec3<> normal;
};

// Picking system for object selection
export class PickingSystem : public systems::System {
public:
    void update(ecs::Scene& scene, float deltaTime) override {}
    
    // Perform ray casting against all renderable objects
    HitResult raycast(ecs::Scene& scene, 
                     const math::Vec3<>& rayOrigin, 
                     const math::Vec3<>& rayDirection,
                     float maxDistance = 1000.0f);
    
    // Get all objects intersecting ray (sorted by distance)
    std::vector<HitResult> raycastAll(ecs::Scene& scene,
                                     const math::Vec3<>& rayOrigin,
                                     const math::Vec3<>& rayDirection,
                                     float maxDistance = 1000.0f);
    
    // Viewport integration
    HitResult pickFromScreen(ecs::Scene& scene, 
                           const editor::Viewport& viewport,
                           const math::Vec2<>& screenPos);
    
private:
    // Test ray against entity's bounding box
    bool testBoundingBox(ecs::Scene& scene, ecs::Entity entity,
                        const math::Vec3<>& rayOrigin,
                        const math::Vec3<>& rayDirection,
                        float& hitDistance);
    
    // Test ray against entity's mesh (if loaded)
    bool testMesh(ecs::Scene& scene, ecs::Entity entity,
                 const math::Vec3<>& rayOrigin,
                 const math::Vec3<>& rayDirection,
                 HitResult& result);
};

} // namespace picking
```

### 3.2 Selection Management
**Target Module**: `editor.selection`

```cpp
export module editor.selection;

import runtime.ecs;
import runtime.components;
import std;

export namespace editor {

// Selection change notifications
export struct SelectionChangedEvent {
    std::vector<ecs::Entity> previousSelection;
    std::vector<ecs::Entity> currentSelection;
    std::vector<ecs::Entity> added;
    std::vector<ecs::Entity> removed;
};

// Selection manager
export class SelectionManager {
public:
    // Selection operations
    void select(ecs::Entity entity, bool additive = false);
    void select(const std::vector<ecs::Entity>& entities, bool additive = false);
    void deselect(ecs::Entity entity);
    void deselectAll();
    void toggleSelection(ecs::Entity entity);
    
    // Selection queries
    const std::vector<ecs::Entity>& getSelectedEntities() const { return m_selection; }
    bool isSelected(ecs::Entity entity) const;
    size_t getSelectionCount() const { return m_selection.size(); }
    bool hasSelection() const { return !m_selection.empty(); }
    
    // Primary selection (for gizmo operations)
    ecs::Entity getPrimarySelection() const;
    void setPrimarySelection(ecs::Entity entity);
    
    // Selection bounds calculation
    engine::BoundingBox<> getSelectionBounds(ecs::Scene& scene) const;
    math::Vec3<> getSelectionCenter(ecs::Scene& scene) const;
    
    // Event system
    void registerSelectionListener(std::function<void(const SelectionChangedEvent&)> callback);
    
private:
    std::vector<ecs::Entity> m_selection;
    ecs::Entity m_primarySelection{};
    
    std::vector<std::function<void(const SelectionChangedEvent&)>> m_listeners;
    
    void notifySelectionChanged(const std::vector<ecs::Entity>& previous);
    void updateSelectionComponents(ecs::Scene& scene);
};

// Mouse picking integration
export class MousePickingHandler {
public:
    MousePickingHandler(SelectionManager& selectionManager, 
                       picking::PickingSystem& pickingSystem);
    
    // Handle viewport mouse events
    void handleMouseClick(ecs::Scene& scene, 
                         const editor::Viewport& viewport,
                         const math::Vec2<>& screenPos,
                         bool ctrlPressed, bool shiftPressed);
    
    void handleMouseDrag(ecs::Scene& scene,
                        const editor::Viewport& viewport,
                        const math::Vec2<>& startPos,
                        const math::Vec2<>& endPos,
                        bool additive);
                        
private:
    SelectionManager& m_selectionManager;
    picking::PickingSystem& m_pickingSystem;
    
    // Rectangle selection
    std::vector<ecs::Entity> getEntitiesInRect(ecs::Scene& scene,
                                              const editor::Viewport& viewport,
                                              const math::Vec2<>& minPos,
                                              const math::Vec2<>& maxPos);
};

} // namespace editor
```

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

```cpp
export module editor.gizmos;

import editor.selection;
import runtime.ecs;
import runtime.components;
import editor.viewport;
import std;
import <ImGuizmo.h>;

export namespace editor {

// Gizmo operation types
export enum class GizmoOperation {
    Translate,
    Rotate,
    Scale,
    Universal  // Combined translate/rotate/scale
};

// Gizmo coordinate space
export enum class GizmoMode {
    Local,
    World
};

// Gizmo manipulation results
export struct GizmoResult {
    bool manipulated = false;
    math::Mat4<> deltaMatrix;
    math::Vec3<> deltaTranslation;
    math::Vec3<> deltaRotation;    // Euler angles in radians
    math::Vec3<> deltaScale;
};

// 3D manipulation gizmo system
export class GizmoSystem {
public:
    GizmoSystem(SelectionManager& selectionManager);
    
    // Gizmo rendering and interaction
    GizmoResult update(ecs::Scene& scene, 
                      const Viewport& viewport,
                      float deltaTime);
    
    // Gizmo settings
    void setOperation(GizmoOperation operation) { m_operation = operation; }
    GizmoOperation getOperation() const { return m_operation; }
    
    void setMode(GizmoMode mode) { m_mode = mode; }
    GizmoMode getMode() const { return m_mode; }
    
    void setSnapEnabled(bool enabled) { m_snapEnabled = enabled; }
    bool isSnapEnabled() const { return m_snapEnabled; }
    
    void setSnapValue(float value) { m_snapValue = value; }
    float getSnapValue() const { return m_snapValue; }
    
    // Gizmo visibility
    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }
    
private:
    SelectionManager& m_selectionManager;
    
    GizmoOperation m_operation = GizmoOperation::Translate;
    GizmoMode m_mode = GizmoMode::World;
    bool m_snapEnabled = false;
    float m_snapValue = 1.0f;
    bool m_visible = true;
    
    // ImGuizmo integration
    void setupImGuizmo(const Viewport& viewport);
    GizmoResult processManipulation(ecs::Scene& scene, 
                                   const math::Mat4<>& objectMatrix,
                                   const math::Mat4<>& viewMatrix,
                                   const math::Mat4<>& projMatrix);
    
    // Transform calculation helpers
    math::Mat4<> calculateGizmoMatrix(ecs::Scene& scene);
    void applyDeltaToSelection(ecs::Scene& scene, const GizmoResult& result);
    
    // Multi-selection support
    math::Vec3<> getSelectionCenter(ecs::Scene& scene);
    void distributeTransformation(ecs::Scene& scene, const GizmoResult& result);
};

// Gizmo UI controls
export class GizmoUI {
public:
    void render(GizmoSystem& gizmoSystem);
    
private:
    bool m_showControls = true;
    
    void renderToolbar(GizmoSystem& gizmoSystem);
    void renderSettings(GizmoSystem& gizmoSystem);
};

} // namespace editor
```

### 4.2 Transform Command System (Preparation for Undo/Redo)
**Target Module**: `editor.transform_commands`

```cpp
export module editor.transform_commands;

import runtime.ecs;
import runtime.components;
import editor.gizmos;
import std;

export namespace editor {

// Base command interface (for undo/redo system)
export class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual std::string getDescription() const = 0;
};

// Transform command for single entity
export class TransformEntityCommand : public Command {
public:
    TransformEntityCommand(ecs::Scene& scene, 
                          ecs::Entity entity,
                          const components::Transform& oldTransform,
                          const components::Transform& newTransform);
    
    void execute() override;
    void undo() override;
    std::string getDescription() const override;
    
private:
    ecs::Scene& m_scene;
    ecs::Entity m_entity;
    components::Transform m_oldTransform;
    components::Transform m_newTransform;
};

// Batch transform command for multiple entities
export class BatchTransformCommand : public Command {
public:
    BatchTransformCommand(ecs::Scene& scene,
                         const std::vector<ecs::Entity>& entities,
                         const std::vector<components::Transform>& oldTransforms,
                         const std::vector<components::Transform>& newTransforms);
    
    void execute() override;
    void undo() override;
    std::string getDescription() const override;
    
private:
    ecs::Scene& m_scene;
    std::vector<ecs::Entity> m_entities;
    std::vector<components::Transform> m_oldTransforms;
    std::vector<components::Transform> m_newTransforms;
};

// Transform command factory
export class TransformCommandFactory {
public:
    static std::unique_ptr<Command> createTransformCommand(
        ecs::Scene& scene,
        const std::vector<ecs::Entity>& entities,
        const GizmoResult& gizmoResult);
    
private:
    static std::vector<components::Transform> captureTransforms(
        ecs::Scene& scene, 
        const std::vector<ecs::Entity>& entities);
};

} // namespace editor
```

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
