# Material System Migration Guide

## Overview

As of **Phase 2F Task T307**, the material system no longer supports the legacy single-pass format. All materials must use the **multi-pass format** with a `passes` array.

This guide explains how to migrate legacy materials to the new format.

---

## Breaking Changes

### Removed Legacy Fields

The following fields have been **removed** from `MaterialDefinition`:

- `pass` (string) — Use `passes` array instead
- `shaders` (vector) — Move to `passes[i].shaders`
- `parameters` (vector) — Move to `passes[i].parameters` (pass-level only)
- `states` (StateReferences) — Move to `passes[i].states`
- `primitiveTopology` (enum) — Move to `passes[i].topology`

### Parser Changes

- **Legacy format is no longer parsed.** The parser will log an error: `"Material '<id>' missing 'passes' array. Only multi-pass format is supported."`
- **No backward compatibility.** Materials using the old format will fail to load.

### Validator Changes

- Validates `material.passes` instead of `material.pass`
- Validates `pass.shaders` and `pass.states` for each pass
- Each pass's `passName` must reference a valid `renderPasses[].id`

---

## Migration Steps

### 1. Legacy Format (Before)

**Single-pass material with top-level fields:**

```json
{
  "id": "grid_material",
  "pass": "grid",
  "shaders": {
    "vertex": {
      "file": "shaders/grid.hlsl",
      "entry": "VSMain",
      "profile": "vs_6_0"
    },
    "pixel": {
      "file": "shaders/grid.hlsl",
      "entry": "PSMain",
      "profile": "ps_6_0"
    }
  },
  "states": {
    "rasterizer": "default_rasterizer",
    "depthStencil": "default_depth",
    "blend": "default_blend"
  },
  "primitiveTopology": "Triangle"
}
```

### 2. Multi-Pass Format (After)

**Same material converted to multi-pass:**

```json
{
  "id": "grid_material",
  "passes": [
    {
      "name": "grid",
      "shaders": {
        "vertex": {
          "file": "shaders/grid.hlsl",
          "entry": "VSMain",
          "profile": "vs_6_0"
        },
        "pixel": {
          "file": "shaders/grid.hlsl",
          "entry": "PSMain",
          "profile": "ps_6_0"
        }
      },
      "states": {
        "rasterizer": "default_rasterizer",
        "depthStencil": "default_depth",
        "blend": "default_blend"
      },
      "topology": "Triangle"
    }
  ]
}
```

### Key Changes

1. **Replace `"pass"` with `"passes"` array** — Single pass becomes array with one element
2. **Move `"shaders"` into `passes[0]`** — Shader definitions are now pass-specific
3. **Move `"states"` into `passes[0]`** — State references are now pass-specific
4. **Rename `"primitiveTopology"` to `"topology"`** and move into `passes[0]`
5. **Set `passes[0].name` to the old `pass` value** — Pass name references the render pass ID

---

## Common Patterns

### Standard Opaque Material

**Before:**
```json
{
  "id": "standard_opaque",
  "pass": "forward",
  "shaders": {
    "vertex": { "file": "shaders/standard.hlsl", "entry": "VS", "profile": "vs_6_0" },
    "pixel": { "file": "shaders/standard.hlsl", "entry": "PS", "profile": "ps_6_0" }
  },
  "states": {
    "rasterizer": "solid_cull_back",
    "depthStencil": "depth_test_write",
    "blend": "opaque"
  }
}
```

**After:**
```json
{
  "id": "standard_opaque",
  "passes": [
    {
      "name": "forward",
      "shaders": {
        "vertex": { "file": "shaders/standard.hlsl", "entry": "VS", "profile": "vs_6_0" },
        "pixel": { "file": "shaders/standard.hlsl", "entry": "PS", "profile": "ps_6_0" }
      },
      "states": {
        "rasterizer": "solid_cull_back",
        "depthStencil": "depth_test_write",
        "blend": "opaque"
      }
    }
  ]
}
```

### Debug Wireframe Material

**Before:**
```json
{
  "id": "debug_wireframe",
  "pass": "forward",
  "shaders": {
    "vertex": { "file": "shaders/simple.hlsl", "entry": "VSMain", "profile": "vs_6_0" },
    "pixel": { "file": "shaders/simple.hlsl", "entry": "PSMain", "profile": "ps_6_0" }
  },
  "states": {
    "rasterizer": "wireframe",
    "depthStencil": "depth_test",
    "blend": "opaque"
  }
}
```

**After:**
```json
{
  "id": "debug_wireframe",
  "passes": [
    {
      "name": "forward",
      "shaders": {
        "vertex": { "file": "shaders/simple.hlsl", "entry": "VSMain", "profile": "vs_6_0" },
        "pixel": { "file": "shaders/simple.hlsl", "entry": "PSMain", "profile": "ps_6_0" }
      },
      "states": {
        "rasterizer": "wireframe",
        "depthStencil": "depth_test",
        "blend": "opaque"
      }
    }
  ]
}
```

---

## Multi-Pass Materials (New Capability)

The new format enables **true multi-pass rendering**. Example: Deferred rendering with prepass + lighting pass.

```json
{
  "id": "deferred_lit",
  "passes": [
    {
      "name": "prepass",
      "shaders": {
        "vertex": { "file": "shaders/gbuffer.hlsl", "entry": "VSPrepass", "profile": "vs_6_0" },
        "pixel": { "file": "shaders/gbuffer.hlsl", "entry": "PSPrepass", "profile": "ps_6_0" }
      },
      "states": {
        "rasterizer": "solid_cull_back",
        "depthStencil": "depth_write",
        "renderTarget": "gbuffer_rt"
      }
    },
    {
      "name": "lighting",
      "shaders": {
        "vertex": { "file": "shaders/lighting.hlsl", "entry": "VSLighting", "profile": "vs_6_0" },
        "pixel": { "file": "shaders/lighting.hlsl", "entry": "PSLighting", "profile": "ps_6_0" }
      },
      "states": {
        "rasterizer": "solid_cull_back",
        "depthStencil": "depth_test",
        "blend": "additive",
        "renderTarget": "lighting_rt"
      }
    }
  ]
}
```

**Usage in code:**
```cpp
// Query specific pass
const MaterialPass *prepass = materialSystem.getMaterialPass(handle, "prepass");
const MaterialPass *lighting = materialSystem.getMaterialPass(handle, "lighting");

// Build PSOs per pass
auto prepassPSO = pipelineBuilder.buildPSO(device, *material, prepass->passName, passConfig, materialSystem);
auto lightingPSO = pipelineBuilder.buildPSO(device, *material, lighting->passName, passConfig, materialSystem);
```

---

## Code API Changes

### Querying Materials

**Before (Legacy - no longer works):**
```cpp
const auto &shaders = material->shaders;  // ERROR: field removed
const auto &states = material->states;    // ERROR: field removed
```

**After (Multi-Pass):**
```cpp
// Query pass by name
const MaterialPass *pass = materialSystem->getMaterialPass(materialHandle, "forward");
if (!pass) {
    console::error("Pass 'forward' not found in material '{}'", material->id);
    return;
}

// Access pass-specific data
const auto &shaders = pass->shaders;
const auto &states = pass->states;
const auto topology = pass->topology;
```

### Building PSOs

**Before:**
```cpp
// Legacy - implicitly used material.pass
auto pso = pipelineBuilder.buildPSO(device, *material, passConfig, materialSystem);
```

**After:**
```cpp
// Multi-Pass - explicitly specify pass name
const MaterialPass *forwardPass = materialSystem->getMaterialPass(handle, "forward");
auto pso = pipelineBuilder.buildPSO(device, *material, "forward", passConfig, materialSystem);
```

### GridRenderer Example

**Before (T306 - Legacy):**
```cpp
// Used material->pass directly
const auto &passName = material->pass;
```

**After (T306 - Multi-Pass):**
```cpp
// Query grid pass explicitly
const MaterialPass *gridPass = materialSystem->getMaterialPass(m_materialHandle, "grid");
if (!gridPass) {
    console::error("GridRenderer: 'grid' pass not found in material");
    return false;
}
const auto &passName = gridPass->passName;
```

---

## Validation Errors

### Error: Missing 'passes' array
```
[ERROR] MaterialParser: Material '<id>' missing 'passes' array. Only multi-pass format is supported.
```

**Solution:** Add `"passes": [...]` array to your material definition.

---

### Error: Undefined pass name
```
[ERROR] Material '<id>': pass '<name>' references undefined pass name '<name>'
```

**Solution:** Ensure `passes[i].name` matches a `renderPasses[].id` in your materials.json.

Example renderPasses definition:
```json
{
  "renderPasses": [
    { "id": "forward", "queue": 1000 },
    { "id": "grid", "queue": 5000 }
  ]
}
```

---

### Error: Pass not found in code
```
[ERROR] GridRenderer: 'grid' pass not found in material
```

**Solution:** Verify your material has a pass with the correct name:
```json
{
  "id": "grid_material",
  "passes": [
    { "name": "grid", ...}  // Must match query
  ]
}
```

---

## Automated Migration Script

For large projects with many materials, consider this PowerShell script to automate conversion:

```powershell
# migrate-materials.ps1
param(
    [string]$MaterialsFile = "materials.json"
)

$json = Get-Content $MaterialsFile | ConvertFrom-Json

foreach ($material in $json.materials) {
    if ($material.pass) {
        # Legacy format detected
        $passName = $material.pass
        
        # Create new pass object
        $newPass = @{
            name = $passName
            shaders = $material.shaders
            states = $material.states
            topology = if ($material.primitiveTopology) { $material.primitiveTopology } else { "Triangle" }
        }
        
        # Add parameters if present
        if ($material.parameters) {
            $newPass.parameters = $material.parameters
        }
        
        # Replace material structure
        $material | Add-Member -MemberType NoteProperty -Name passes -Value @($newPass) -Force
        $material.PSObject.Properties.Remove('pass')
        $material.PSObject.Properties.Remove('shaders')
        $material.PSObject.Properties.Remove('states')
        $material.PSObject.Properties.Remove('primitiveTopology')
        $material.PSObject.Properties.Remove('parameters')
        
        Write-Host "Migrated material: $($material.id)"
    }
}

# Save migrated file
$json | ConvertTo-Json -Depth 10 | Set-Content "${MaterialsFile}.migrated.json"
Write-Host "Migration complete: ${MaterialsFile}.migrated.json"
```

**Usage:**
```bash
.\migrate-materials.ps1 -MaterialsFile "materials.json"
```

---

## Testing After Migration

1. **Build the project** — Verify compilation succeeds with no errors
2. **Run unit tests** — Material system tests should pass (201 assertions)
3. **Launch application** — Verify materials load without errors
4. **Visual verification** — Check that rendering appears correct

**Test commands:**
```bash
# Build
cmake --build build/vs2022-x64 --config Debug

# Run material system tests
build/vs2022-x64/Debug/unit_test_runner.exe "[material-system]"

# Launch application
build/vs2022-x64/Debug/level_editor.exe
```

---

## Support

For migration issues or questions:
- Check `PROGRESS_2.md` for Phase 2F implementation details
- Review `M2_P7.md` for task-by-task breakdown
- See `tests/material_system_tests.cpp` for multi-pass examples
- See `tests/grid_tests.cpp` for GridRenderer multi-pass integration

**Phase 2F Tasks:**
- T301: MaterialPass Structure
- T302: Parser Multi-Pass Support
- T303: PipelineBuilder Pass-Specific PSO
- T304: MaterialSystem Pass Queries
- T305: Update Test Suite
- T306: Update Renderer Integration
- **T307: Migration & Documentation (this guide)**

---

## Revision History

| Date | Description |
|------|-------------|
| 2025-10-12 | Initial migration guide for Phase 2F Task T307 |
