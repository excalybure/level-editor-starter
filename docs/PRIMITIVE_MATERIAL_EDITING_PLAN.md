# Primitive Material Editing - Hybrid Approach Plan

**Date:** October 18, 2025  
**Milestone:** Material System Enhancement  
**Status:** Planning Phase

---

## Overview

Enable per-primitive material property editing in the level editor, allowing artists to customize material parameters (baseColorFactor, metallicFactor, roughnessFactor, emissiveFactor, textures) for individual mesh primitives without creating duplicate material definitions.

## Naming Resolution

✅ **Completed:** Renamed `MaterialInstance` → `CompiledMaterial` to free the name
- `CompiledMaterial` = GPU-compiled rendering state (PSO, root signature) for a material definition
- `MaterialInstance` = Now available for per-primitive material property overrides

---

## Architecture: Hybrid Approach (1 + 2)

Combines lightweight UI expansion with a proper material override system.

### Phase 1: Lightweight UI Expansion (Quick Win)
**Goal:** Show primitives in inspector and display current material properties (read-only initially)

**Components:**
1. **EntityInspectorPanel Enhancement**
   - Expand `MeshRenderer` component UI to show primitive tree
   - For each primitive: display index, vertex/index counts, assigned material name
   - Show material properties in collapsible sections (read-only)

2. **Data Flow:**
   ```
   Entity → MeshRenderer component → MeshGPU → PrimitiveGPU → MaterialGPU
                                                                    ↓
                                                            Material properties
   ```

3. **Implementation Tasks:**
   - Add primitive tree view under `MeshRenderer` in inspector
   - Query `MeshGPU::getPrimitive(index)` for each primitive
   - Display `MaterialGPU` properties for each primitive
   - Style with ImGui tree nodes and color coding

**Benefits:**
- No new data structures
- Immediate visual feedback of current state
- Foundation for Phase 2 editing

---

### Phase 2: Material Override System (Full Solution)
**Goal:** Enable editing per-primitive material properties with undo/redo support

**New Concept: MaterialInstance**
```cpp
namespace assets
{
    // Per-primitive material property overrides
    struct MaterialInstance
    {
        MaterialHandle baseMaterial = INVALID_MATERIAL_HANDLE;
        
        // Optional overrides (nullopt = use base material value)
        std::optional<math::Vec4f> baseColorFactorOverride;
        std::optional<float> metallicFactorOverride;
        std::optional<float> roughnessFactorOverride;
        std::optional<math::Vec3f> emissiveFactorOverride;
        
        // Texture overrides (empty string = use base material texture)
        std::optional<std::string> baseColorTextureOverride;
        std::optional<std::string> metallicRoughnessTextureOverride;
        std::optional<std::string> normalTextureOverride;
        std::optional<std::string> emissiveTextureOverride;
        
        // Helper: Get effective value (override if present, else base material)
        math::Vec4f getEffectiveBaseColor(const Material* baseMat) const;
        float getEffectiveMetallic(const Material* baseMat) const;
        float getEffectiveRoughness(const Material* baseMat) const;
        math::Vec3f getEffectiveEmissive(const Material* baseMat) const;
        
        // Check if any overrides are present
        bool hasOverrides() const;
        
        // Clear all overrides
        void clearOverrides();
    };
}
```

**Integration Points:**

1. **Primitive Storage:**
   ```cpp
   class Primitive
   {
   private:
       MaterialHandle m_materialHandle = INVALID_MATERIAL_HANDLE;
       MaterialInstance m_materialInstance; // NEW: Override storage
       
   public:
       const MaterialInstance& getMaterialInstance() const;
       void setMaterialInstance(const MaterialInstance& instance);
       bool hasOverrides() const { return m_materialInstance.hasOverrides(); }
   };
   ```

2. **MaterialGPU Update:**
   ```cpp
   class MaterialGPU
   {
   public:
       // NEW: Constructor taking MaterialInstance for overrides
       MaterialGPU(
           dx12::Device* device,
           const Material* baseMaterial,
           const MaterialInstance* instance = nullptr
       );
       
       // Update GPU constants from MaterialInstance overrides
       void updateFromInstance(const MaterialInstance* instance);
   };
   ```

3. **Scene Serialization:**
   ```json
   {
       "meshes": [
           {
               "name": "Cube",
               "primitives": [
                   {
                       "material": "pbr_base",
                       "materialInstance": {
                           "baseColorFactor": [1.0, 0.0, 0.0, 1.0],
                           "metallicFactor": 0.8
                       }
                   }
               ]
           }
       ]
   }
   ```

4. **Editor Commands (Undo/Redo):**
   ```cpp
   class SetPrimitiveMaterialPropertyCommand : public EditorCommand
   {
   public:
       SetPrimitiveMaterialPropertyCommand(
           ecs::Entity entity,
           uint32_t primitiveIndex,
           PropertyType type,
           std::variant<float, Vec3f, Vec4f, std::string> value
       );
       
       void execute() override;
       void undo() override;
   };
   ```

**Data Flow with Overrides:**
```
User Edit → Command → Primitive.m_materialInstance → MaterialGPU update → GPU buffer
                                                                              ↓
                                                                         Rendering
```

---

## Implementation Roadmap

### Milestone 1: Read-Only Primitive Display (Quick Win)
**Estimated:** 1-2 days

- [x] **T1.1:** Add primitive tree view to EntityInspectorPanel
  - Collapsible tree under MeshRenderer component
  - Show: Primitive index, vertex count, index count
  
- [x] **T1.2:** Display current material name per primitive
  - Query MaterialGPU for base material
  - Show material ID as text
  
- [x] **T1.3:** Show material properties (read-only)
  - baseColorFactor (color picker disabled)
  - metallicFactor, roughnessFactor (sliders disabled)
  - emissiveFactor (color picker disabled)
  - Texture names (text only)

**Success Criteria:**
- [x] Can expand MeshRenderer component and see all primitives
- [x] Each primitive shows current material properties
- [x] UI is read-only but visually clear

---

### Milestone 2: MaterialInstance Data Structure
**Estimated:** 2-3 days

- [x] **T2.1:** Create `assets::MaterialInstance` struct
  - Add to `assets.h` with all override fields
  - Implement helper methods (getEffective*, hasOverrides, clear)
  - Unit tests for override resolution logic
  
- [x] **T2.2:** Integrate MaterialInstance into Primitive
  - Add `m_materialInstance` member
  - Add getter/setter methods
  - Update copy/move constructors
  
- [x] **T2.3:** Update MaterialGPU to accept MaterialInstance
  - Add optional MaterialInstance* parameter to constructor
  - Implement `updateFromInstance()` method
  - Apply overrides to GPU constants buffer
  
- [x] **T2.4:** Update MeshGPU::configureMaterials ✅ COMPLETED (Oct 19)
  - Pass MaterialInstance pointer when creating MaterialGPU
  - Ensure override values reach GPU buffer

**Success Criteria:**
- [x] MaterialInstance struct compiles and passes unit tests
- [x] Primitive can store and retrieve MaterialInstance
- [x] MaterialGPU respects override values when present

---

### Milestone 3: Scene Serialization Support
**Estimated:** 2-3 days

- [x] **T3.1:** Extend SceneSerializer for MaterialInstance
  - Add materialInstance object serialization
  - Only serialize non-empty overrides
  - Parse materialInstance from JSON
  
- [x] **T3.2:** Update glTF loader (if needed)
  - Check if glTF supports per-primitive material properties
  - Map to MaterialInstance if applicable
  
- [x] **T3.3:** Scene loading/saving tests ✅ COMPLETED (Oct 19)
  - Save scene with overrides, load, verify values
  - Test partial overrides (only some properties set)

**Success Criteria:**
- ✅ Scenes with material overrides save/load correctly
- ✅ JSON format is clean and minimal (only overrides saved)
- ✅ Backward compatibility maintained (scenes without overrides still work)

**Test Coverage (6 tests, 62 assertions):**
1. Scene save/load with full material overrides on multiple primitives
2. Partial material overrides serialize cleanly (only non-empty fields)
3. Material instance overrides load correctly from JSON
4. Round-trip save/load preserves all override values
5. Backward compatibility: scenes without overrides still work
6. Multiple primitives with different override patterns serialize cleanly

---

### Milestone 4: Editor UI for Editing
**Estimated:** 3-4 days

- [ ] **T4.1:** Make material property widgets editable
  - Enable color pickers for baseColorFactor, emissiveFactor
  - Enable sliders for metallicFactor, roughnessFactor
  - Add "Reset to Base" button per property
  
- [ ] **T4.2:** Implement SetPrimitiveMaterialPropertyCommand
  - Create command class with execute/undo
  - Store old/new values
  - Update MaterialInstance on execute
  - Trigger MaterialGPU update
  
- [ ] **T4.3:** Wire up commands to UI widgets
  - Detect value changes in ImGui widgets
  - Create and execute command
  - Add to command history
  
- [ ] **T4.4:** Add visual indicators for overrides
  - Show icon/color when property is overridden
  - "Clear All Overrides" button per primitive
  - Tooltip showing base vs override value

**Success Criteria:**
- Can edit all material properties per primitive
- Changes are undoable/redoable
- Visual feedback shows which properties are overridden
- Changes persist across save/load

---

### Milestone 5: Advanced Features (Optional)
**Estimated:** 2-3 days

- [ ] **T5.1:** Texture picker UI
  - Browse/select textures for override
  - Preview texture thumbnails
  - "Use Base Material Texture" option
  
- [ ] **T5.2:** Copy/Paste material overrides
  - Copy MaterialInstance from one primitive
  - Paste to another primitive
  - Multi-select support
  
- [ ] **T5.3:** Material presets
  - Save common override combinations
  - Apply preset to selected primitives
  - Preset library management

---

## Technical Considerations

### Memory Impact
- **MaterialInstance per Primitive:** ~60 bytes per primitive with overrides
- **Empty Optimization:** Use `std::optional` to avoid memory cost when no overrides present
- **Typical Scene:** 1000 primitives × 60 bytes = ~60 KB (negligible)

### Performance Impact
- **GPU Upload:** Only updated primitives need buffer re-upload
- **MaterialGPU Update:** O(1) per modified primitive
- **Rendering:** No overhead (same GPU buffer structure)

### Undo/Redo Strategy
- **Granular Commands:** One command per property change (fine-grained undo)
- **Batch Option:** Support batch edits for multi-select (future)
- **Memory:** Store old/new values as variants (minimal overhead)

---

## Code Organization

```
src/engine/assets/
  assets.h                    # Add MaterialInstance struct, integrate into Primitive

src/graphics/gpu/
  material_gpu.h/cpp          # Add MaterialInstance* parameter, updateFromInstance()

src/runtime/scene_serialization/
  SceneSerializer.h/cpp       # Add materialInstance JSON serialization

src/editor/
  entity_inspector_panel.h/cpp  # Add primitive tree view, property editors
  commands/
    primitive_material_commands.h/cpp  # Material override commands

tests/
  material_instance_override_tests.cpp  # Unit tests for MaterialInstance
  scene_serialization_override_tests.cpp  # Scene save/load with overrides
```

---

## Example Usage

### Before (Current State)
```cpp
// All primitives of mesh use base material "pbr_base"
entity.getComponent<MeshRenderer>().meshHandle = loadMesh("cube.gltf");
// All 6 cube faces have same material properties
```

### After (With MaterialInstance)
```cpp
// Load mesh, then customize per-primitive
auto& meshRenderer = entity.getComponent<MeshRenderer>();
meshRenderer.meshHandle = loadMesh("cube.gltf");

// Override front face to be red
auto& primitive0 = scene.getMesh(meshRenderer.meshHandle).getPrimitive(0);
MaterialInstance instance;
instance.baseMaterial = getMaterialHandle("pbr_base");
instance.baseColorFactorOverride = Vec4f{1.0f, 0.0f, 0.0f, 1.0f};
instance.metallicFactorOverride = 0.8f;
primitive0.setMaterialInstance(instance);

// Other faces keep base material properties
```

### Scene JSON
```json
{
  "entities": [
    {
      "name": "ColoredCube",
      "components": {
        "MeshRenderer": {
          "mesh": "assets/cube.gltf"
        }
      },
      "primitives": [
        {
          "index": 0,
          "materialInstance": {
            "baseMaterial": "pbr_base",
            "baseColorFactor": [1.0, 0.0, 0.0, 1.0],
            "metallicFactor": 0.8
          }
        }
      ]
    }
  ]
}
```

---

## Benefits Summary

### For Artists
- ✅ Edit material properties per primitive without creating duplicate materials
- ✅ Instant visual feedback in viewport
- ✅ Undo/redo support for all edits
- ✅ Clear visual indicators of overridden properties

### For Developers
- ✅ Clean data model (MaterialInstance separate from base Material)
- ✅ Minimal performance overhead (GPU buffer unchanged)
- ✅ Scene serialization built-in
- ✅ Backward compatible (existing scenes unchanged)

### Architecture
- ✅ Separation of concerns (base material vs instance overrides)
- ✅ Composable (can mix/match base materials and overrides)
- ✅ Extensible (easy to add new override properties)

---

## Design Decisions

### Why MaterialInstance instead of MaterialOverride?
- **MaterialInstance** better conveys "an instance with customizations"
- Industry standard naming (Unity, Unreal use "Material Instance")
- Clearer mental model: base material + instance = final appearance

### Why std::optional for overrides?
- Memory efficient (no storage cost when not overriding)
- Explicit semantics (nullopt = use base, value = override)
- Easy to check `hasOverrides()` by checking any optional is set

### Why store in Primitive instead of new component?
- Primitives already reference materials (natural extension)
- Keeps override data with geometric data
- Simpler serialization (primitive owns instance)
- No need for separate component lookup

---

## Testing Strategy

### Unit Tests
- MaterialInstance override resolution (getEffective* methods)
- Primitive MaterialInstance getter/setter
- MaterialGPU update from MaterialInstance
- Scene serialization round-trip

### Integration Tests
- Load scene with overrides, render, verify visual output
- Edit property in UI, verify GPU buffer updated
- Undo/redo material edits, verify state consistency
- Save scene with overrides, reload, verify persistence

### Manual Tests
- Multi-primitive mesh (cube with 6 faces)
- Override different properties per face
- Verify visual appearance matches overrides
- Test with various material types (PBR, unlit, etc.)

---

## Migration Path

### Phase 1: Read-Only Display
- ✅ No data model changes
- ✅ UI-only implementation
- ✅ Zero risk to existing functionality
- ✅ Can ship to users immediately

### Phase 2: Data Model + Serialization
- ⚠️ Extends Primitive class (backward compatible)
- ⚠️ Scene format change (additive, old scenes still work)
- ✅ Can be tested independently of UI

### Phase 3: Editing UI + Commands
- ✅ Builds on Phase 1 + 2
- ✅ Full undo/redo integration
- ✅ Complete feature

---

## Success Criteria

### Milestone 1 Complete When:
- [x] Primitive tree visible in inspector
- [x] All primitives show current material properties
- [x] UI is clean and intuitive

### Milestone 2 Complete When:
- [ ] MaterialInstance struct implemented and tested
- [ ] Primitive stores MaterialInstance
- [ ] MaterialGPU respects overrides
- [ ] Unit tests pass

### Milestone 3 Complete When:
- [ ] Scenes with overrides save/load correctly
- [ ] JSON format is clean
- [ ] Backward compatibility verified

### Milestone 4 Complete When:
- [ ] Can edit all properties per primitive
- [ ] Changes are undoable
- [ ] Visual indicators work
- [ ] Changes persist

---

## Open Questions

1. **Texture Override Implementation:**
   - Should texture overrides reference texture paths or texture handles?
   - How to handle texture loading/unloading for overrides?
   
2. **Multi-Select Editing:**
   - Should editing multiple primitives create individual commands or batched command?
   - How to show "mixed values" in UI when multiple primitives selected?

3. **Material Compatibility:**
   - Should we validate that overrides are compatible with base material's shader?
   - E.g., can't override baseColor on unlit material that doesn't use it?

4. **Performance Scaling:**
   - What happens with 10,000+ primitives with overrides?
   - Need spatial culling/LOD for override updates?

---

## Next Steps

1. ✅ Finalize naming (MaterialInstance confirmed)
2. ⏳ Review plan with team
3. ⏳ Start Milestone 1 (Read-Only Display)
4. ⏳ Prototype MaterialInstance struct
5. ⏳ Design scene JSON format for overrides

---

## References

- Original Discussion: Conversation on October 18, 2025
- Related: `CompiledMaterial` (formerly MaterialInstance) implementation
- Related: `MaterialGPU` and `PrimitiveGPU` architecture
- Related: EntityInspectorPanel current implementation

---

## Appendix: Alternative Approaches Considered

### Approach A: Duplicate Materials
**Rejected:** Creates material explosion, hard to manage

### Approach B: Primitive-as-Entity
**Rejected:** Too heavyweight, breaks ECS hierarchy model

### Approach C: Material Variants
**Rejected:** Still creates multiple material definitions, not artist-friendly

### ✅ Chosen: Hybrid (Lightweight UI + MaterialInstance)
**Why:** Best of both worlds - quick iteration path + proper long-term solution

---

## T3.2 Findings: glTF Loader Analysis

### glTF 2.0 Per-Primitive Material Support

**Status:** ✅ **No loader changes required**

### Key Findings

1. **glTF Already Supports Per-Primitive Materials**
   - Each primitive in a mesh can reference a different material
   - Material references are by index into the materials array
   - Perfect alignment with our Primitive-based mesh architecture
   - GLTFLoader already handles this correctly

2. **glTF Does NOT Support Per-Primitive Material Property Overrides**
   - glTF 2.0 spec only supports assigning different base materials
   - No built-in mechanism for per-primitive property overrides
   - To customize a primitive's appearance in standard glTF:
     - Create a new material definition with desired properties
     - Assign that material to the primitive
     - Results in material duplication for unique customizations

3. **Our Solution: MaterialInstance as Custom Extension Layer**
   - Separate from glTF base materials
   - Stored in our scene serialization format
   - Bridges the gap between glTF limitations and artist workflow
   - Allows editing primitives without duplicating materials

### Architecture

```
glTF File          → GLTFLoader → Mesh { 
                                   primitives: [
                                     { materialHandle: 0 },
                                     { materialHandle: 1 }
                                   ]
                                 }

Our Scene JSON  → SceneDeserializer → Mesh {
                                       primitives: [
                                         { materialHandle: 0,
                                           materialInstance: { baseColorFactor: [1,0,0,1] } },
                                         { materialHandle: 1,
                                           materialInstance: { metallicFactor: 0.8 } }
                                       ]
                                     }
```

### Implementation Status

- ✅ glTF loader correctly assigns per-primitive base materials
- ✅ GLTFLoader::extractPrimitive() maps material indices to handles
- ✅ MaterialInstance layer is separate and not tied to glTF loading
- ✅ Scene serialization (T3.1) handles persistence of overrides
- ✅ No changes required to GLTFLoader for T3.2

### Future Enhancement (Optional)

If we want to persist MaterialInstance overrides in glTF files:
- Define custom glTF extension: `VENDOR_primitive_material_overrides`
- Extend GLTFLoader to parse extension data
- Populate MaterialInstance during glTF loading
- Not required for current workflow (overrides stored in scene format)
