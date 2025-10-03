# Scene File Format Specification

## Version 1.0

This document describes the JSON file format used for saving and loading scenes in the level editor.

## File Extension
`.scene` - JSON-formatted text file

## Schema Overview

```json
{
  "version": "1.0",
  "metadata": {
    "name": "Scene Name",
    "created": "2025-10-03T10:30:00Z",
    "modified": "2025-10-03T14:45:00Z",
    "author": "Editor User"
  },
  "entities": [
    {
      "id": 1,
      "name": "Entity Name",
      "parent": null,
      "components": {
        "transform": { ... },
        "visible": { ... },
        "meshRenderer": { ... }
      }
    }
  ]
}
```

## Top-Level Fields

### `version` (string, required)
Format version identifier. Current version is `"1.0"`.

### `metadata` (object, optional)
Scene metadata for display and tracking purposes.

**Fields:**
- `name` (string): Human-readable scene name
- `created` (string): ISO 8601 timestamp of scene creation
- `modified` (string): ISO 8601 timestamp of last modification
- `author` (string): Creator/editor name

### `entities` (array, required)
Array of all entities in the scene. Each entity is an object with the following structure.

## Entity Schema

### `id` (integer, required)
Unique entity identifier within this scene file. Used for parent references.

**Note:** IDs are remapped on load to match the runtime ECS entity handles. The ID field is only used for preserving relationships within the file.

### `name` (string, optional)
Entity name from the `Name` component. If omitted, entity has no Name component.

### `parent` (integer or null, required)
ID of the parent entity, or `null` if this is a root entity.

**Hierarchy Rules:**
- Root entities have `parent: null`
- Child entities reference their parent by ID
- Parent entity must appear before children in the array (forward references not allowed)
- Circular references are invalid and rejected on load

### `components` (object, required)
Map of component type names to component data. Component presence is indicated by inclusion in this object.

## Component Schemas

### `transform` Component

**Type:** `Transform` struct from `components.h`

```json
"transform": {
  "position": [0.0, 0.0, 0.0],
  "rotation": [0.0, 0.0, 0.0],
  "scale": [1.0, 1.0, 1.0]
}
```

**Fields:**
- `position` (array of 3 floats): World position (x, y, z)
- `rotation` (array of 3 floats): Euler angles in radians (x, y, z)
- `scale` (array of 3 floats): Scale factors (x, y, z)

**Notes:**
- Transform component is serialized for all entities (required by ECS)
- Cached matrices (`localMatrix`, `localMatrixDirty`) are not serialized and will be recomputed on load

### `visible` Component

**Type:** `Visible` struct from `components.h`

```json
"visible": {
  "visible": true,
  "castShadows": true,
  "receiveShadows": true
}
```

**Fields:**
- `visible` (boolean): Entity visibility flag
- `castShadows` (boolean): Whether entity casts shadows
- `receiveShadows` (boolean): Whether entity receives shadows

**Default Values:** All fields default to `true` if component is present but fields are omitted.

### `meshRenderer` Component

**Type:** `MeshRenderer` struct from `components.h`

```json
"meshRenderer": {
  "meshPath": "assets/models/cube.gltf",
  "lodBias": 0.0
}
```

**Fields:**
- `meshPath` (string): Relative path to the mesh asset file (GLTF/GLB format)
- `lodBias` (float, optional): Level of detail bias, default 0.0

**Notes:**
- `meshHandle` is resolved from `meshPath` on load via `AssetManager`
- `gpuMesh` is reconstructed from the loaded asset
- `bounds` are recalculated from the mesh geometry

**Asset Loading:**
When loading a scene, the serializer will:
1. Load the mesh asset via `AssetManager::loadAsset(meshPath)`
2. Store the resulting handle in `meshHandle`
3. Create GPU resources via the rendering system

## Example Scene File

```json
{
  "version": "1.0",
  "metadata": {
    "name": "Test Scene",
    "created": "2025-10-03T10:00:00Z",
    "modified": "2025-10-03T12:30:00Z",
    "author": "Level Editor"
  },
  "entities": [
    {
      "id": 1,
      "name": "Root Container",
      "parent": null,
      "components": {
        "transform": {
          "position": [0.0, 0.0, 0.0],
          "rotation": [0.0, 0.0, 0.0],
          "scale": [1.0, 1.0, 1.0]
        }
      }
    },
    {
      "id": 2,
      "name": "Cube",
      "parent": 1,
      "components": {
        "transform": {
          "position": [0.0, 1.0, 0.0],
          "rotation": [0.0, 0.785398, 0.0],
          "scale": [1.0, 1.0, 1.0]
        },
        "visible": {
          "visible": true,
          "castShadows": true,
          "receiveShadows": true
        },
        "meshRenderer": {
          "meshPath": "assets/models/cube.gltf",
          "lodBias": 0.0
        }
      }
    },
    {
      "id": 3,
      "name": "Sphere Child",
      "parent": 2,
      "components": {
        "transform": {
          "position": [2.0, 0.0, 0.0],
          "rotation": [0.0, 0.0, 0.0],
          "scale": [0.5, 0.5, 0.5]
        },
        "visible": {
          "visible": true,
          "castShadows": true,
          "receiveShadows": false
        },
        "meshRenderer": {
          "meshPath": "assets/models/sphere.gltf",
          "lodBias": 0.0
        }
      }
    }
  ]
}
```

## Excluded Components

The following components are **not serialized**:

### `Selected` Component
**Reason:** Selection state is editor-specific runtime state, not part of the persistent scene data. Selection is cleared when loading a scene.

### Future Components
As new components are added to the ECS, this format will be extended. The version field allows backward compatibility checking.

## Error Handling

### Invalid Hierarchy
- Forward references (child before parent): **Rejected** - entities must be ordered topologically
- Circular references: **Rejected** - detected during load
- Missing parent ID: **Warning** - treat as root entity

### Missing Assets
- If `meshPath` cannot be loaded: **Warning** - entity is created but MeshRenderer component is not added
- Editor displays error indicator for entities with missing assets

### Unknown Components
- Components in the file that don't exist in the current runtime: **Ignored** with warning
- Allows loading scenes created with future editor versions (with feature loss)

## Versioning Strategy

Future versions will maintain backward compatibility where possible:

- **Minor version increments (1.0 → 1.1):** New optional fields, new component types
- **Major version increments (1.0 → 2.0):** Breaking changes to schema structure

The loader will check the version field and:
- Accept same major version (e.g., 1.x loads into 1.y runtime)
- Reject different major version with clear error message
- Log warnings for minor version mismatches

## Implementation Notes

### Entity ID Mapping
During deserialization:
1. Read JSON entities in order
2. Create ECS entity for each JSON entity
3. Build map: `oldID → newEntity`
4. Apply parent references using the map
5. Call `Scene::setParent(newEntity, mappedParent)`

### Component Serialization Order
Components should be serialized in consistent order for readability:
1. `transform` (always present)
2. `visible` (if present)
3. `meshRenderer` (if present)

### File I/O
- Use UTF-8 encoding
- Pretty-print JSON with 2-space indentation for human readability
- Validate JSON syntax before attempting deserialization
- Provide helpful error messages with line numbers for parsing failures

## Future Extensions

Planned additions for future versions:

### Version 1.1 (Planned)
- Camera component serialization
- Light component serialization
- Material override data in MeshRenderer
- Entity tags and layers

### Version 2.0 (Planned)
- Asset dependencies list (for bundling/validation)
- Prefab instance references
- Script component data
- Physics component data
