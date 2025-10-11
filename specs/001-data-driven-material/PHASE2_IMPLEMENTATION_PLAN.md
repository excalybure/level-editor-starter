# Phase 2 Implementation Plan: Complete Data-Driven Material System
## Gap Analysis & Roadmap to Full D3D12 PSO Creation

**Date**: 2025-10-10  
**Branch**: `001-data-driven-material`  
**Status**: DRAFT - Awaiting Approval  
**Context**: P1 (T001-T016) delivered minimal material system (JSON load → MaterialDefinition storage → handle-based queries). Current implementation has hard-coded shaders/states in `PipelineBuilder::buildPSO()`. This document identifies all missing pieces needed to generate complete `D3D12_GRAPHICS_PIPELINE_STATE_DESC` from JSON data.

---

## Executive Summary

### Current State (P1 Complete)
✅ **What Works**:
- JSON loading with includes & circular dependency detection
- Material parsing: `id`, `pass`, `shaders` (stage → shaderId only), `parameters`, `states` (string references only)
- Validation: duplicate IDs, schema checks
- MaterialSystem API: handle-based queries, O(1) lookups
- PSO caching infrastructure (`PipelineCache`)
- Root signature builder (T013) - generates specs from material parameters
- Shader compiler integration (T012) - compiles with defines
- Basic PSO creation stub (T014) - uses hard-coded `simple.hlsl` and default D3D12 states

❌ **What's Missing**:
1. **Shader Information in JSON**: No file paths, entry points, profiles, or per-shader defines
2. **State Block Definitions**: JSON has state references (`"rasterizer": "solid_back"`) but no actual state block data (D3D12 field values)
3. **State Parsing & Translation**: No code to parse state blocks from JSON → D3D12 structs
4. **Input Layout**: Hard-coded `POSITION + COLOR`; needs data-driven vertex format specification
5. **Primitive Topology**: Hard-coded `D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE`
6. **Sample Mask**: Hard-coded `UINT_MAX`
7. **Root Signature Integration**: Generated spec (T013) not used in PSO creation

### Target State (P2 Complete)
🎯 **End Goal**: `PipelineBuilder::buildPSO()` constructs complete `D3D12_GRAPHICS_PIPELINE_STATE_DESC` **entirely from JSON**, eliminating all hard-coded values:
- Shaders compiled from JSON-specified files with entry points, profiles, and defines
- Rasterizer/Depth/Blend/RT states populated from JSON state block definitions
- Input layout derived from material vertex format specification
- Root signature built from material parameters (already implemented in T013)
- All 25+ PSO fields driven by data

---

## Gap 1: Shader Information Schema

### Current Limitation
`materials.json` only stores shader **stage → shaderId** mapping:
```json
"shaders": {
    "vertex": "mesh_vs",
    "pixel": "lit_ps"
}
```

`ShaderReference` struct (parser.h:29) has:
```cpp
struct ShaderReference {
    std::string stage;    // "vertex", "pixel", "compute"
    std::string shaderId; // References a ShaderEntry.id
};
```

**Problem**: No information about:
- Where shader source file is located
- Which entry point to compile
- Target shader model (vs_6_7, ps_6_7, etc.)
- Per-shader defines (currently supported at material level only)

### Required: ShaderEntry Schema
`materials-example.json` shows correct schema (lines 58-91):
```json
"materials": [{
    "passes": [{
        "shaders": {
            "vs": { 
                "file": "shaders/mesh.hlsl", 
                "entry": "VSMain", 
                "profile": "vs_6_7",
                "defines": ["IS_PREPASS"] 
            },
            "ps": { 
                "file": "shaders/lit.hlsl", 
                "entry": "PSMain", 
                "profile": "ps_6_7" 
            }
        }
    }]
}]
```

### Implementation Tasks

#### **T201: Extend ShaderReference Struct**
Modify `src/graphics/material_system/parser.h`:
```cpp
enum class ShaderStage {
    Vertex,
    Pixel,
    Domain,
    Hull,
    Geometry,
    Compute
};

struct ShaderReference {
    ShaderStage stage;          // Vertex, Pixel, Compute, etc.
    std::string file;           // Path to .hlsl file
    std::string entryPoint;     // Function name (default "main")
    std::string profile;        // "vs_6_7", "ps_6_7", etc.
    std::vector<std::string> defines; // Per-shader defines
};
```

**Tests**:
- Parse shader entry with all fields present → verify populated
- Parse shader entry with missing optional fields → apply defaults
- Parse shader with invalid profile string → console::fatal
- Parse shader with missing required file → console::fatal

---

#### **T202: Update MaterialParser to Extract Shader Info**
Modify `src/graphics/material_system/parser.cpp`:
- Parse `shaders` object in JSON material
- For each stage (vs, ps, ds, hs, gs):
  - If value is object: extract `file`, `entry`, `profile`, `defines[]`
  - Validate `file` path exists (relative to project root)
  - Validate `profile` matches regex `(vs|ps|ds|hs|gs|cs)_\d+_\d+`
  - Default `entry` to "main" if absent
  
**Tests**:
- Parse material with inline shader objects → all fields extracted
- Parse material with missing shader file → fatal error
- Parse material with invalid profile → fatal error
- Parse material with duplicate shader stages → fatal error
- Remove support for legacy mode. Remove tests if needed

---

#### **T203: Update PipelineBuilder to Use Shader Info**
Modify `src/graphics/material_system/pipeline_builder.cpp`:
- Replace hard-coded `shaderPath = "shaders/simple.hlsl"` (line 32)
- Iterate `material.shaders` vector:
  - For each `ShaderReference`, call `MaterialShaderCompiler::CompileWithDefines()` with:
    - `shaderRef.file`
    - `shaderRef.entryPoint`
    - `shaderRef.profile`
    - Merged defines: global + pass + material + shader-level
- Populate `D3D12_SHADER_BYTECODE` for VS, PS, DS, HS, GS as present
- If required stage missing (e.g., no VS for graphics) → console::fatal

**Tests**:
- Build PSO with material specifying VS + PS in JSON → shaders compiled from correct files
- Build PSO with material missing VS → fatal error
- Build PSO with material having per-shader defines → defines applied during compilation
- Build PSO with material referencing non-existent shader file → fatal error

---

## Gap 2: State Block Definitions

### Current Limitation
`materials.json` references states by name:
```json
"states": {
    "rasterizer": "solid_back",
    "depthStencil": "depth_test_write",
    "blend": "opaque"
}
```

`StateReferences` struct (parser.h:36) only stores **names**:
```cpp
struct StateReferences {
    std::string rasterizer;
    std::string depthStencil;
    std::string blend;
    std::string renderTarget;
};
```

**Problem**: No actual D3D12 state data (fill mode, cull mode, depth func, blend factors, etc.). `PipelineBuilder::buildPSO()` uses hard-coded defaults (lines 109-149).

### Required: State Block Schema
`materials-example.json` shows correct schema (lines 1-45):
```json
"states": {
    "depthStencilStates": {
        "Default": {
            "depthEnable": true,
            "depthWriteMask": "All",
            "depthFunc": "LessEqual",
            "stencilEnable": false
        },
        "DepthReadOnly": {
            "base": "Default",
            "depthWriteMask": "Zero"
        }
    },
    "rasterizerStates": {
        "Default": {
            "fillMode": "Solid",
            "cullMode": "Back",
            "frontCounterClockwise": false
        },
        "Wireframe": {
            "base": "Default",
            "fillMode": "Wireframe",
            "cullMode": "None"
        }
    },
    "blendStates": {
        "Opaque": {
            "alphaToCoverage": false,
            "independentBlend": false,
            "renderTargets": [{
                "enable": false,
                "srcBlend": "One",
                "destBlend": "Zero",
                "blendOp": "Add",
                "writeMask": "All"
            }]
        }
    }
}
```

### Implementation Tasks

#### **T204: Define State Block Structs**
Create `src/graphics/material_system/state_blocks.h`:
```cpp
namespace graphics::material_system {

// Rasterizer state block
struct RasterizerStateBlock {
    std::string id;
    std::string base; // Optional inheritance
    D3D12_FILL_MODE fillMode = D3D12_FILL_MODE_SOLID;
    D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;
    BOOL frontCounterClockwise = FALSE;
    INT depthBias = 0;
    FLOAT depthBiasClamp = 0.0f;
    FLOAT slopeScaledDepthBias = 0.0f;
    BOOL depthClipEnable = TRUE;
    BOOL multisampleEnable = FALSE;
    BOOL antialiasedLineEnable = FALSE;
    UINT forcedSampleCount = 0;
    D3D12_CONSERVATIVE_RASTERIZATION_MODE conservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
};

// Depth stencil state block
struct DepthStencilStateBlock {
    std::string id;
    std::string base;
    BOOL depthEnable = TRUE;
    D3D12_DEPTH_WRITE_MASK depthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS;
    BOOL stencilEnable = FALSE;
    UINT8 stencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    UINT8 stencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    D3D12_DEPTH_STENCILOP_DESC frontFace = { /* defaults */ };
    D3D12_DEPTH_STENCILOP_DESC backFace = { /* defaults */ };
};

// Blend render target state
struct BlendRenderTargetState {
    BOOL blendEnable = FALSE;
    BOOL logicOpEnable = FALSE;
    D3D12_BLEND srcBlend = D3D12_BLEND_ONE;
    D3D12_BLEND destBlend = D3D12_BLEND_ZERO;
    D3D12_BLEND_OP blendOp = D3D12_BLEND_OP_ADD;
    D3D12_BLEND srcBlendAlpha = D3D12_BLEND_ONE;
    D3D12_BLEND destBlendAlpha = D3D12_BLEND_ZERO;
    D3D12_BLEND_OP blendOpAlpha = D3D12_BLEND_OP_ADD;
    D3D12_LOGIC_OP logicOp = D3D12_LOGIC_OP_NOOP;
    UINT8 renderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
};

// Blend state block
struct BlendStateBlock {
    std::string id;
    std::string base;
    BOOL alphaToCoverageEnable = FALSE;
    BOOL independentBlendEnable = FALSE;
    std::array<BlendRenderTargetState, 8> renderTargets;
};

// Render target state block
struct RenderTargetStateBlock {
    std::string id;
    std::vector<DXGI_FORMAT> rtvFormats;
    DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN;
    UINT sampleCount = 1;
    UINT sampleQuality = 0;
};

} // namespace
```

**Tests**:
- Default-construct each struct → verify D3D12 defaults match spec
- Struct sizes match expectations (no padding issues)

---

#### **T205: Create State Block Parser**
Create `src/graphics/material_system/state_parser.h/cpp`:
```cpp
class StateBlockParser {
public:
    static RasterizerStateBlock parseRasterizer(const nlohmann::json& j);
    static DepthStencilStateBlock parseDepthStencil(const nlohmann::json& j);
    static BlendStateBlock parseBlend(const nlohmann::json& j);
    static RenderTargetStateBlock parseRenderTarget(const nlohmann::json& j);
    
private:
    static D3D12_FILL_MODE parseFillMode(const std::string& str);
    static D3D12_CULL_MODE parseCullMode(const std::string& str);
    static D3D12_COMPARISON_FUNC parseComparisonFunc(const std::string& str);
    static D3D12_BLEND parseBlendFactor(const std::string& str);
    static D3D12_BLEND_OP parseBlendOp(const std::string& str);
    static DXGI_FORMAT parseFormat(const std::string& str);
    static UINT8 parseWriteMask(const std::string& str);
};
```

**String → Enum Mappings**:
- `"Solid"` → `D3D12_FILL_MODE_SOLID`, `"Wireframe"` → `D3D12_FILL_MODE_WIREFRAME`
- `"None"` → `D3D12_CULL_MODE_NONE`, `"Front"` → `D3D12_CULL_MODE_FRONT`, `"Back"` → `D3D12_CULL_MODE_BACK`
- `"Never"` → `D3D12_COMPARISON_FUNC_NEVER`, `"Less"` → `D3D12_COMPARISON_FUNC_LESS`, etc.
- `"One"` → `D3D12_BLEND_ONE`, `"Zero"` → `D3D12_BLEND_ZERO`, `"SrcAlpha"` → `D3D12_BLEND_SRC_ALPHA`, etc.
- `"Add"` → `D3D12_BLEND_OP_ADD`, `"Subtract"` → `D3D12_BLEND_OP_SUBTRACT`, etc.
- `"All"` → `D3D12_COLOR_WRITE_ENABLE_ALL`, `"Red"` → `D3D12_COLOR_WRITE_ENABLE_RED`, etc.
- `"R8G8B8A8_UNORM"` → `DXGI_FORMAT_R8G8B8A8_UNORM`, `"D32_FLOAT"` → `DXGI_FORMAT_D32_FLOAT`, etc.

**Inheritance (`base` field)**:
- If JSON has `"base": "ParentStateName"`:
  1. Load parent state block first
  2. Copy all parent fields to child
  3. Override with child's explicitly specified fields
- Circular base references → console::fatal
- Undefined base → console::fatal

**Tests**:
- Parse rasterizer state with all fields → struct populated correctly
- Parse depth stencil state with defaults → verify default values
- Parse blend state with 2 render targets → both configured
- Parse state with inheritance → child overrides parent fields
- Parse state with invalid enum string → console::fatal
- Parse state with circular base reference → console::fatal
- Parse state with undefined base → console::fatal

---

#### **T206: Integrate State Blocks into MaterialSystem**
Extend `MaterialSystem` class (material_system.h):
```cpp
class MaterialSystem {
public:
    // Existing...
    bool initialize(const std::string& jsonPath);
    
    // New state queries
    const RasterizerStateBlock* getRasterizerState(const std::string& id) const;
    const DepthStencilStateBlock* getDepthStencilState(const std::string& id) const;
    const BlendStateBlock* getBlendState(const std::string& id) const;
    const RenderTargetStateBlock* getRenderTargetState(const std::string& id) const;

private:
    std::unordered_map<std::string, RasterizerStateBlock> m_rasterizerStates;
    std::unordered_map<std::string, DepthStencilStateBlock> m_depthStencilStates;
    std::unordered_map<std::string, BlendStateBlock> m_blendStates;
    std::unordered_map<std::string, RenderTargetStateBlock> m_renderTargetStates;
    // Existing material storage...
};
```

Update `MaterialSystem::initialize()`:
- Parse `"states"` section from merged JSON
- For each category (`depthStencilStates`, `rasterizerStates`, etc.):
  - Parse all state blocks in category
  - Resolve inheritance (`base` references)
  - Store in category map
  - Detect duplicate IDs → console::fatal

**Tests**:
- Initialize with JSON containing state blocks → query by ID returns correct data
- Initialize with duplicate state ID → console::fatal
- Initialize with material referencing undefined state ID → console::fatal (validation phase)

---

#### **T207: Update PipelineBuilder to Use State Blocks**
Modify `PipelineBuilder::buildPSO()`:
- Replace hard-coded rasterizer state (lines 109-121) with:
  ```cpp
  const auto* rasterizerState = /* get from MaterialSystem */;
  if (!rasterizerState) { console::fatal("Undefined rasterizer state"); }
  psoDesc.RasterizerState = /* copy fields from rasterizerState */;
  ```
- Replace hard-coded blend state (lines 123-137)
- Replace hard-coded depth/stencil state (lines 139-152)
- Use render target state for formats/sample desc

**Pass `MaterialSystem*` to `buildPSO()`**:
```cpp
static ComPtr<ID3D12PipelineState> buildPSO(
    dx12::Device* device,
    const MaterialDefinition& material,
    const RenderPassConfig& passConfig,
    const MaterialSystem* materialSystem  // New parameter
);
```

**Tests**:
- Build PSO with material using "Wireframe" rasterizer → verify D3D12 desc has WIREFRAME fill mode
- Build PSO with material using "DepthReadOnly" → verify depth write mask is ZERO
- Build PSO with material using "AlphaBlend" → verify RT0 has blend enabled with correct factors
- Build PSO with material referencing undefined state → console::fatal

---

## Gap 3: Input Layout

### Current Limitation
`PipelineBuilder::buildPSO()` hard-codes input layout (lines 65-69):
```cpp
const D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, ... },
    { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, ... }
};
psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
```

**Problem**: Different materials need different vertex formats (with/without normals, UVs, tangents, etc.).

### Required: Vertex Format Schema
Add to JSON:
```json
"vertexFormats": {
    "PositionNormalUV": {
        "elements": [
            { "semantic": "POSITION", "format": "R32G32B32_FLOAT", "offset": 0 },
            { "semantic": "NORMAL", "format": "R32G32B32_FLOAT", "offset": 12 },
            { "semantic": "TEXCOORD", "format": "R32G32_FLOAT", "offset": 24 }
        ],
        "stride": 32
    }
}
```

Materials reference vertex format:
```json
"materials": [{
    "id": "standard_lit",
    "vertexFormat": "PositionNormalUV",
    ...
}]
```

### Implementation Tasks

#### **T208: Define VertexFormat Structs**
Add to `state_blocks.h`:
```cpp
struct VertexElement {
    std::string semantic;           // "POSITION", "NORMAL", "TEXCOORD", etc.
    UINT semanticIndex = 0;
    DXGI_FORMAT format;
    UINT inputSlot = 0;
    UINT alignedByteOffset;
    D3D12_INPUT_CLASSIFICATION inputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    UINT instanceDataStepRate = 0;
};

struct VertexFormat {
    std::string id;
    std::vector<VertexElement> elements;
    UINT stride;
};
```

---

#### **T209: Parse Vertex Formats**
Extend `StateBlockParser`:
```cpp
static VertexFormat parseVertexFormat(const nlohmann::json& j);
static DXGI_FORMAT parseVertexElementFormat(const std::string& str);
```

Integrate into `MaterialSystem::initialize()`:
- Parse `"vertexFormats"` section
- Store in `std::unordered_map<std::string, VertexFormat> m_vertexFormats;`
- Add query: `const VertexFormat* getVertexFormat(const std::string& id) const;`

---

#### **T210: Add vertexFormat to MaterialDefinition**
Modify `parser.h`:
```cpp
struct MaterialDefinition {
    std::string id;
    std::string pass;
    std::string vertexFormat; // NEW: references VertexFormat.id
    std::vector<ShaderReference> shaders;
    // ...
};
```

Update `MaterialParser::parse()` to extract `"vertexFormat"` field (optional; default to "Default" if absent).

---

#### **T211: Use Vertex Format in PSO Creation**
Modify `PipelineBuilder::buildPSO()`:
```cpp
const auto* vertexFormat = materialSystem->getVertexFormat(material.vertexFormat);
if (!vertexFormat) { console::fatal("Undefined vertex format: {}", material.vertexFormat); }

std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
for (const auto& elem : vertexFormat->elements) {
    inputElements.push_back({
        elem.semantic.c_str(),
        elem.semanticIndex,
        elem.format,
        elem.inputSlot,
        elem.alignedByteOffset,
        elem.inputSlotClass,
        elem.instanceDataStepRate
    });
}
psoDesc.InputLayout = { inputElements.data(), (UINT)inputElements.size() };
```

**Tests**:
- Build PSO with material using "PositionNormalUV" format → verify 3 input elements
- Build PSO with material using undefined vertex format → console::fatal

---

## Gap 4: Primitive Topology & Other PSO Fields

### Current Limitation
Hard-coded values:
- Primitive topology type: `D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE` (line 157)
- Sample mask: `UINT_MAX` (line 155)
- Sample desc: `{ 1, 0 }` (lines 167-168)
- IB strip cut value: (not set, defaults to disabled)

### Implementation Tasks

#### **T212: Add Primitive Topology to Materials**
Extend `MaterialDefinition`:
```cpp
struct MaterialDefinition {
    // ...
    D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    D3D12_INDEX_BUFFER_STRIP_CUT_VALUE ibStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    UINT sampleMask = UINT_MAX;
};
```

JSON schema:
```json
"materials": [{
    "primitiveTopology": "Triangle",  // "Triangle", "Line", "Point", "Patch"
    "ibStripCutValue": "Disabled",    // Optional
    "sampleMask": "0xFFFFFFFF"        // Optional, hex or decimal
}]
```

Update parser to extract these fields with defaults.

---

#### **T213: Sample Desc from Render Target State**
`RenderTargetStateBlock` already has `sampleCount` and `sampleQuality`. Use in PSO:
```cpp
psoDesc.SampleDesc.Count = renderTargetState->sampleCount;
psoDesc.SampleDesc.Quality = renderTargetState->sampleQuality;
```

---

## Gap 5: Root Signature Integration

### Current Limitation
`RootSignatureBuilder` (T013) generates root signature specs, but `PipelineBuilder::buildPSO()` creates a **minimal hard-coded root signature** (lines 73-106) instead of using the generated spec.

### Implementation Tasks

#### **T214: Build Root Signature from Spec**
Create `src/graphics/material_system/root_signature_cache.h/cpp`:
```cpp
class RootSignatureCache {
public:
    ComPtr<ID3D12RootSignature> getOrCreate(
        dx12::Device* device,
        const RootSignatureSpec& spec
    );
    
private:
    std::unordered_map<uint64_t, ComPtr<ID3D12RootSignature>> m_cache;
    uint64_t hashSpec(const RootSignatureSpec& spec);
};
```

---

#### **T215: Use Root Signature from Cache in PSO**
Modify `PipelineBuilder::buildPSO()`:
```cpp
// Generate root signature spec from material parameters (already implemented in T013)
const auto rootSigSpec = RootSignatureBuilder::Build(material);

// Get or create cached root signature
static RootSignatureCache rootSigCache;
auto rootSignature = rootSigCache.getOrCreate(device, rootSigSpec);
if (!rootSignature) { console::fatal("Failed to create root signature"); }

psoDesc.pRootSignature = rootSignature.Get();
```

Remove hard-coded root signature creation (lines 73-106).

**Tests**:
- Build PSO with material having parameters → root signature spec includes CBV bindings
- Build PSO with material having no parameters → empty root signature
- Build two PSOs with identical root sig specs → cache hit (only one D3D12 root sig created)

---

## Gap 6: Render Pass Configuration

### Current Limitation
`RenderPassConfig` struct (pipeline_builder.h:19) is manually constructed by caller:
```cpp
RenderPassConfig passConfig;
passConfig.name = "forward";
passConfig.rtvFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
passConfig.dsvFormat = DXGI_FORMAT_D32_FLOAT;
passConfig.numRenderTargets = 1;
```

**Problem**: Should come from JSON `renderPasses` section.

### Required: Parse Render Passes
`materials-example.json` shows schema (lines 46-57):
```json
"renderPasses": [
    {
        "name": "lit_opaque",
        "queue": "Opaque",
        "attachments": { /* ... */ },
        "states": {
            "renderTarget": "MainColor"
        }
    }
]
```

### Implementation Tasks

#### **T216: Parse Render Passes**
Create `src/graphics/material_system/render_pass_parser.h/cpp`:
```cpp
struct RenderPassDefinition {
    std::string name;
    std::string renderTargetStateId; // References RenderTargetStateBlock
};

class RenderPassParser {
public:
    static RenderPassDefinition parse(const nlohmann::json& j);
};
```

Integrate into `MaterialSystem::initialize()`:
- Parse `"renderPasses"` array
- Store in `std::unordered_map<std::string, RenderPassDefinition> m_renderPasses;`
- Validate: material's `pass` field must reference existing render pass

---

#### **T217: Generate RenderPassConfig from Definition**
Add to `MaterialSystem`:
```cpp
RenderPassConfig getRenderPassConfig(const std::string& passName) const {
    const auto* passDef = getRenderPass(passName);
    if (!passDef) { console::fatal("Undefined render pass: {}", passName); }
    
    const auto* rtState = getRenderTargetState(passDef->renderTargetStateId);
    if (!rtState) { console::fatal("Undefined RT state: {}", passDef->renderTargetStateId); }
    
    RenderPassConfig config;
    config.name = passDef->name;
    config.numRenderTargets = (UINT)rtState->rtvFormats.size();
    for (size_t i = 0; i < rtState->rtvFormats.size(); ++i) {
        config.rtvFormats[i] = rtState->rtvFormats[i];
    }
    config.dsvFormat = rtState->dsvFormat;
    return config;
}
```

Update callers to use `materialSystem.getRenderPassConfig(material.pass)` instead of manual construction.

---

## Implementation Phases

### Phase 2A: Shader Information (T201-T203)
**Goal**: Replace hard-coded `simple.hlsl` with JSON-specified shader files, entry points, profiles.

**Dependencies**: None (can start immediately)

**Estimated Effort**: 3 tasks, ~2-3 days

**Tests**: 12 test cases (4 per task)

**Validation**: Build PSO for material in `materials-example.json` → shaders compiled from `shaders/mesh.hlsl` and `shaders/lit.hlsl`

---

### Phase 2B: State Blocks (T204-T207)
**Goal**: Replace hard-coded D3D12 states with JSON-defined state blocks.

**Dependencies**: T201-T203 (optional; can run in parallel)

**Estimated Effort**: 4 tasks, ~4-5 days

**Tests**: 20+ test cases (enum parsing, inheritance, PSO creation)

**Validation**: Build PSO with "Wireframe" rasterizer → verify fillMode is WIREFRAME; build PSO with "AlphaBlend" → verify RT0 blend enabled

---

### Phase 2C: Vertex Formats (T208-T211)
**Goal**: Replace hard-coded input layout with JSON-defined vertex formats.

**Dependencies**: T204-T207 (state blocks must be implemented first)

**Estimated Effort**: 4 tasks, ~3-4 days

**Tests**: 8 test cases

**Validation**: Build PSO with "PositionNormalUV" format → verify 3 input elements with correct offsets

---

### Phase 2D: Root Signature Integration (T214-T215)
**Goal**: Use generated root signature specs (from T013) in PSO creation.

**Dependencies**: T201-T211 (all previous P2 work)

**Estimated Effort**: 2 tasks, ~2 days

**Tests**: 4 test cases

**Validation**: Two materials with identical parameter lists share one root signature (cache hit)

---

### Phase 2E: Render Pass Config & Finalization (T216-T217, T212-T213)
**Goal**: Parse render passes from JSON; finalize all PSO fields.

**Dependencies**: T214-T215

**Estimated Effort**: 4 tasks, ~2-3 days

**Tests**: 6 test cases

**Validation**: End-to-end test: load `materials-example.json`, build PSO for "opaque_standard" material → all 25+ PSO fields populated from JSON

---

## Success Criteria (Phase 2 Complete)

### SC-P2-001: Zero Hard-Coded Values in PSO Creation
✅ **Criteria**: `PipelineBuilder::buildPSO()` contains **no literal D3D12 enum values** (e.g., `D3D12_FILL_MODE_SOLID`, `D3D12_CULL_MODE_BACK`, etc.). All values derived from JSON.

**Verification**: Code audit + grep search for `D3D12_FILL_MODE|D3D12_CULL_MODE|D3D12_BLEND_` in `pipeline_builder.cpp` returns zero matches.

---

### SC-P2-002: Complete materials-example.json Support
✅ **Criteria**: Load `materials-example.json` and build PSO for "opaque_standard" material → PSO creation succeeds with all fields populated:
- Shaders: VS from `shaders/mesh.hlsl`, PS from `shaders/lit.hlsl`
- Rasterizer: From "Default" state (solid, back-face culling)
- Depth: From "Default" state (depth test enabled, write enabled, less-equal func)
- Blend: From "Opaque" state (blend disabled)
- RT formats: From "MainColor" state (R8G8B8A8_UNORM + D32_FLOAT depth)
- Root signature: Generated from material parameters (Scene CBV slot 0, Object CBV slot 1, Material CBV slot 2)
- Input layout: From vertex format

**Verification**: Integration test loads full example JSON, builds PSO, validates all fields match expectations.

---

### SC-P2-003: State Inheritance Works
✅ **Criteria**: Define state block with `"base": "ParentState"` → child inherits parent fields and overrides specified fields.

**Verification**: Unit test creates "DepthReadOnly" state with `base: "Default"` and `depthWriteMask: "Zero"` → verify depth enable inherited from Default, but write mask is ZERO.

---

### SC-P2-004: Error Handling for Undefined References
✅ **Criteria**: All undefined references (shader files, state IDs, vertex formats, render passes) → `console::fatal` with clear message.

**Verification**: Negative test suite (8+ test cases) covering:
- Material references undefined rasterizer state
- Material references undefined vertex format
- Material shader file does not exist
- State block has undefined base
- Material pass references undefined render pass

---

### SC-P2-005: Performance — No Regression
✅ **Criteria**: PSO creation time with full JSON system ≤ 110% of P1 baseline (allowing 10% overhead for state lookups).

**Verification**: Benchmark test builds 25 PSOs with P2 system, compare to P1 baseline. Should be <500ms total (currently projected <400ms).

---

## Risk Assessment

### Risk 1: Large Enum String Mapping Tables
**Impact**: Medium  
**Probability**: High  
**Mitigation**: Use code generation or macros to define mappings (e.g., `X-macro` pattern). Write once, reuse for all enum types.

---

### Risk 2: State Inheritance Complexity
**Impact**: Medium  
**Probability**: Medium  
**Mitigation**: Strict TDD — write failing tests for inheritance before implementation. Limit inheritance depth to 1 level initially (can extend later if needed).

---

### Risk 3: Vertex Format Stride Calculation
**Impact**: Low  
**Probability**: Low  
**Mitigation**: Explicitly specify stride in JSON (don't auto-calculate). Validation test verifies stride matches sum of element sizes.

---

### Risk 4: Root Signature Cache Thrashing
**Impact**: Low  
**Probability**: Low  
**Mitigation**: Use existing `RootSignatureSpec` hash from T013. Cache keyed by hash. Collision detection (different specs → same hash) → console::fatal.

---

## Summary Table

| Gap | Tasks | Estimated Effort | Tests | Priority |
|-----|-------|-----------------|-------|----------|
| Shader Information | T201-T203 | 3 tasks, 2-3 days | 12 | **P0 (Critical)** |
| State Blocks | T204-T207 | 4 tasks, 4-5 days | 20+ | **P0 (Critical)** |
| Vertex Formats | T208-T211 | 4 tasks, 3-4 days | 8 | **P1 (High)** |
| Root Sig Integration | T214-T215 | 2 tasks, 2 days | 4 | **P1 (High)** |
| Render Pass Config | T216-T217 | 2 tasks, 2 days | 4 | **P2 (Medium)** |
| Misc PSO Fields | T212-T213 | 2 tasks, 1 day | 4 | **P2 (Medium)** |
| **TOTAL** | **17 tasks** | **~14-17 days** | **52+ test cases** | - |

---

## Appendix A: D3D12_GRAPHICS_PIPELINE_STATE_DESC Field Checklist

| Field | Current Status | P2 Status | Driven By |
|-------|---------------|-----------|-----------|
| `pRootSignature` | ❌ Hard-coded minimal | ✅ Generated from params | T214-T215 |
| `VS` | ❌ Hard-coded simple.hlsl | ✅ JSON shader entry | T201-T203 |
| `PS` | ❌ Hard-coded simple.hlsl | ✅ JSON shader entry | T201-T203 |
| `DS` | ❌ Not used | ✅ JSON shader entry (optional) | T201-T203 |
| `HS` | ❌ Not used | ✅ JSON shader entry (optional) | T201-T203 |
| `GS` | ❌ Not used | ✅ JSON shader entry (optional) | T201-T203 |
| `StreamOutput` | ❌ Default (disabled) | ⚠️ Not supported (future) | - |
| `BlendState` | ❌ Hard-coded opaque | ✅ JSON blend state block | T204-T207 |
| `SampleMask` | ❌ Hard-coded UINT_MAX | ✅ JSON material field | T212 |
| `RasterizerState` | ❌ Hard-coded solid/back | ✅ JSON rasterizer state block | T204-T207 |
| `DepthStencilState` | ❌ Hard-coded defaults | ✅ JSON depth stencil state block | T204-T207 |
| `InputLayout` | ❌ Hard-coded pos+color | ✅ JSON vertex format | T208-T211 |
| `IBStripCutValue` | ❌ Default (disabled) | ✅ JSON material field | T212 |
| `PrimitiveTopologyType` | ❌ Hard-coded triangle | ✅ JSON material field | T212 |
| `NumRenderTargets` | ✅ From RenderPassConfig | ✅ From JSON render pass | T216-T217 |
| `RTVFormats[8]` | ✅ From RenderPassConfig | ✅ From JSON RT state | T216-T217 |
| `DSVFormat` | ✅ From RenderPassConfig | ✅ From JSON RT state | T216-T217 |
| `SampleDesc` | ❌ Hard-coded { 1, 0 } | ✅ From JSON RT state | T213 |
| `NodeMask` | ❌ Default (0) | ✅ Default (multi-GPU deferred) | - |
| `CachedPSO` | ❌ Not used | ✅ Not used (cache via PipelineCache) | - |
| `Flags` | ❌ Default (0) | ✅ Default (no special flags) | - |

**P2 Complete Coverage**: 21/21 fields driven by data or explicitly deferred.

---

## Appendix B: Example Complete Material JSON

```json
{
    "vertexFormats": {
        "PositionNormalUV": {
            "elements": [
                { "semantic": "POSITION", "format": "R32G32B32_FLOAT", "offset": 0 },
                { "semantic": "NORMAL", "format": "R32G32B32_FLOAT", "offset": 12 },
                { "semantic": "TEXCOORD", "format": "R32G32_FLOAT", "offset": 24 }
            ],
            "stride": 32
        }
    },
    "states": {
        "rasterizerStates": {
            "Default": {
                "fillMode": "Solid",
                "cullMode": "Back",
                "frontCounterClockwise": false
            },
            "Wireframe": {
                "base": "Default",
                "fillMode": "Wireframe",
                "cullMode": "None"
            }
        },
        "depthStencilStates": {
            "Default": {
                "depthEnable": true,
                "depthWriteMask": "All",
                "depthFunc": "LessEqual"
            },
            "DepthReadOnly": {
                "base": "Default",
                "depthWriteMask": "Zero"
            }
        },
        "blendStates": {
            "Opaque": {
                "renderTargets": [{ "enable": false }]
            },
            "AlphaBlend": {
                "base": "Opaque",
                "renderTargets": [{ 
                    "enable": true, 
                    "srcBlend": "SrcAlpha", 
                    "destBlend": "InvSrcAlpha" 
                }]
            }
        },
        "renderTargetStates": {
            "MainColor": {
                "rtvFormats": ["R8G8B8A8_UNORM"],
                "dsvFormat": "D32_FLOAT",
                "samples": 1
            }
        }
    },
    "renderPasses": [
        {
            "name": "lit_opaque",
            "states": { "renderTarget": "MainColor" }
        }
    ],
    "materials": [
        {
            "id": "standard_lit",
            "pass": "lit_opaque",
            "vertexFormat": "PositionNormalUV",
            "primitiveTopology": "Triangle",
            "shaders": {
                "vs": { 
                    "file": "shaders/mesh.hlsl", 
                    "entry": "VSMain", 
                    "profile": "vs_6_7" 
                },
                "ps": { 
                    "file": "shaders/lit.hlsl", 
                    "entry": "PSMain", 
                    "profile": "ps_6_7" 
                }
            },
            "states": {
                "rasterizer": "Default",
                "depthStencil": "Default",
                "blend": "Opaque"
            },
            "parameters": [
                { "name": "baseColor", "type": "float4", "defaultValue": [1,1,1,1] },
                { "name": "roughness", "type": "float", "defaultValue": 0.5 }
            ]
        }
    ]
}
```

**Result**: Complete PSO with:
- Shaders compiled from `shaders/mesh.hlsl` (VS) and `shaders/lit.hlsl` (PS)
- Solid fill, back-face culling rasterizer
- Depth test enabled, writes enabled
- Opaque blend (disabled)
- 3 input elements (position, normal, UV)
- Triangle list topology
- Root signature with 2 CBV bindings (for baseColor and roughness parameters)
- R8G8B8A8_UNORM render target, D32_FLOAT depth buffer

---

**End of Phase 2 Implementation Plan**
