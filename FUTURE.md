# Future Development Plans

## AssetManager: loadMaterial & loadMesh Implementation

**Date:** September 11, 2025  
**Status:** Analysis Complete - Implementation Recommended

### Current State Analysis

The `AssetManager` uses a template-based `load<T>()` method that delegates to internal type-specific methods:
- `load<Scene>()` → `loadScene()` ✅ **Implemented** (uses GLTFLoader callback)
- `load<Material>()` → `loadMaterial()` ⚠️ **Stub** (creates basic Material, marked TODO)
- `load<Mesh>()` → `loadMesh()` ⚠️ **Stub** (returns nullptr, marked TODO)

### Current Usage Patterns

1. **Tests**: Use `manager.load<Material>("test.mtl")` for cache validation
2. **Scene Loading**: GLTFLoader extracts materials/meshes as part of scene import
3. **Standalone Loading**: Currently no standalone material/mesh file loading

### Architectural Benefits of Implementation

1. **Consistent API**: Uniform `load<T>(path)` calls regardless of asset type
2. **Future Extensibility**: Support for standalone `.mtl`, `.obj`, `.ply` file loading
3. **Separation of Concerns**: Scene loading vs. individual asset loading
4. **Cache Consistency**: All assets use the same caching mechanism

### Implementation Recommendations

#### Priority 1: loadMaterial() - Medium Priority

**Purpose**: Implement standalone material file parsing
- Support `.mtl` files (Wavefront Material)
- Enable material libraries separate from scenes
- Already has test infrastructure via template interface

**Implementation Notes**:
```cpp
std::shared_ptr<Material> AssetManager::loadMaterial(const std::string& path)
{
    // Parse .mtl file format
    // Create PBR Material with proper properties
    // Handle texture references and material properties
    // Set path and loaded status
}
```

**Use Cases**:
- Material libraries shared across multiple scenes
- Procedural material creation and export
- External material authoring tools integration

#### Priority 2: loadMesh() - Lower Priority

**Purpose**: Implement standalone mesh file parsing
- Support `.obj`, `.ply`, and other common mesh formats
- Enable mesh imports outside of scene context
- Complement scene-based mesh loading

**Implementation Notes**:
```cpp
std::shared_ptr<Mesh> AssetManager::loadMesh(const std::string& path)
{
    // Parse mesh file formats (.obj, .ply, etc.)
    // Create Mesh with proper geometry data
    // Handle vertex attributes, materials references
    // Set path and loaded status
}
```

**Use Cases**:
- Standalone geometry imports
- Procedural mesh export/import
- External modeling tool integration
- Simple geometry files for testing/prototyping

### Implementation Strategy

1. **Keep Existing Architecture**: The template pattern works well
2. **Implement Incrementally**: Start with `loadMaterial()`, then `loadMesh()`
3. **Maintain Compatibility**: Existing GLTFLoader scene loading continues unchanged
4. **Add File Format Support**: 
   - Materials: `.mtl` (Wavefront Material)
   - Meshes: `.obj` (Wavefront OBJ), `.ply` (Stanford PLY)

### Current Code Structure

**Template Delegation Pattern** (in `asset_manager.ixx`):
```cpp
template<>
std::shared_ptr<Material> AssetManager::load<Material>(const std::string& path)
{
    // Check cache first
    auto it = m_cache.find(path);
    if (it != m_cache.end()) {
        return std::static_pointer_cast<Material>(it->second);
    }
    
    // Delegate to internal method
    auto material = loadMaterial(path);  // ← Currently stub
    if (material) {
        m_cache[path] = material;
    }
    return material;
}
```

**Current Stub Implementation**:
```cpp
std::shared_ptr<Material> AssetManager::loadMaterial(const std::string& path)
{
    // TODO: Implement material loading from files
    // For now, create a basic material
    auto material = std::make_shared<Material>();
    material->setPath(path);
    material->setLoaded(true);
    return material;
}

std::shared_ptr<Mesh> AssetManager::loadMesh(const std::string& path)
{
    // TODO: Implement standalone mesh loading
    // For now, this would be part of scene loading
    return nullptr;
}
```

### Testing Infrastructure

The template interface is already tested in `asset_manager_tests.cpp`:
- Cache consistency tests
- Load/unload lifecycle tests
- Memory management validation

**Existing Test Pattern**:
```cpp
auto material1 = manager.load<Material>("test_material.mtl");
auto material2 = manager.load<Material>("test_material.mtl");
REQUIRE(material1 == material2); // Same cached instance
```

### Dependencies

**For loadMaterial()**:
- Material file format parsers (`.mtl`)
- Texture path resolution
- PBR property mapping

**For loadMesh()**:
- Mesh file format parsers (`.obj`, `.ply`)
- Vertex attribute handling
- Index buffer management
- Material reference resolution

### Conclusion

**Recommendation**: Implement both methods to complete the asset system architecture.

**Benefits**:
- ✅ Architectural completeness
- ✅ Future flexibility for non-scene workflows  
- ✅ Consistent API through template pattern
- ✅ Existing test infrastructure ready
- ✅ Separation of concerns (scene vs. standalone loading)

**Timeline**: Implement when asset system expansion is prioritized, with `loadMaterial()` taking precedence over `loadMesh()` due to higher utility in material authoring workflows.