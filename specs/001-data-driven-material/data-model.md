# Data Model — Data‑Driven Material & Pipeline System

## Entities

### MaterialDefinition
Fields:
- id : string (unique, fatal duplicate)
- pass : string (must match existing enum pass name)
- shaders : object { stageName -> ShaderEntryId } (required stages vary by pass; at minimum vertex+pixel for graphics)
- parameters : array<Parameter>
- states : object { rasterizer: id?, depthStencil: id?, blend: id?, renderTarget: id? }
- enabled : bool (default true)
- versionHash : string (optional, informational)

Validation:
- id non-empty, matches regex `[A-Za-z0-9_\-]+`.
- pass exists in enum; failure fatal.
- All referenced state ids exist.
- Only allowed parameter types.
- No duplicate parameter names.

### Parameter
Fields:
- name : string (unique within material)
- type : enum { float, int, bool, float4 }
- defaultValue : JSON scalar/array (type-specific)

Validation:
- Name regex `[A-Za-z0-9_]+`.
- Type must be allowed set; else fatal.
- float4 default must be array length 4 of numbers.

### ShaderEntry
Fields:
- id : string (unique)
- stage : enum { vertex, pixel, compute (future) }
- file : path string
- entryPoint : string (default "main" if absent)
- defines : map<string,string> (optional, merged with inherited defines)

Validation:
- File must exist & be readable.
- stage permitted set.
- No duplicate define keys (including inherited) — fatal.

### StateBlock (DepthStencil / Rasterizer / Blend / RenderTarget)
Common Fields:
- id : string (unique per category)
- category-specific fields (omitted here; defined by rendering subsystem)

Validation:
- Id unique (category scope).
- All required category fields present (enforced in category-specific validator).

### RenderPassConfig
Fields:
- name : string (must match enum value)
- targets : array<RenderTargetSpec>
- depth : DepthSpec? (presence depends on pass type)
- clearOps : object (color, depth, stencil) optional
- materials : array<MaterialRef> (optional convenience list)

Validation:
- Name must correspond to existing enum; no new creation.
- Formats consistent with materials referencing pass.

### DefineScope
Fields:
- defines : map<string,string>
- children : (pass/material definitions)

Validation:
- Duplicate key across ancestor chain → fatal.

### Include
Fields:
- path : relative path string

Validation:
- Path resolves inside project root (no traversal outside); file readable.
- Cycle detection via DFS stack.

### PipelineCacheEntry
Fields:
- hash : uint64 (derived from pass, material id, shader ids, state block ids, root signature spec)
- rootSignatureId : uint64
- psoHandle : backend handle (opaque)

Validation:
- Hash collision (different inputs → same hash) → fatal.

### RootSignatureSpec (Derived)
Fields:
- resourceBindings : array<ResourceBinding> (ordered for hashing)

### ResourceBinding
Fields:
- name : string
- type : enum { CBV, SRV, UAV, Sampler }
- slot : int (assigned deterministically)

## Relationships
- MaterialDefinition references: RenderPassConfig (by pass name), ShaderEntry (by id), StateBlock categories, Parameter list.
- RenderPassConfig references: materials (indirect), target formats.
- PipelineCacheEntry derived from MaterialDefinition + RenderPassConfig.
- DefineScope hierarchical: Global → Pass → Material.

## Derived Data
- rootSignatureSpec generated from union of all shader resource declarations for the material.
- pipeline hash recomputed on any contributing input change (no hot reload initial — recompute only at startup).

## Validation Sequence
1. Parse includes (collect raw JSON docs).
2. Merge with detection of duplicate ids (fail-fast).
3. Build define scopes, check collisions.
4. Validate schema & entity-level constraints.
5. Resolve references (states, shaders, passes).
6. Generate root signature specs.
7. Build / cache PSOs.

## Invariants
- No duplicate ids anywhere after merge.
- All materials refer to existing pass enum names.
- All parameter types within allowed set.
- No resource binding name duplication within a root signature spec.

## Assumptions
- State block field specifics enforced by existing renderer; this system only routes ids through.
- Shader reflection (if used) returns resource binding info deterministically.

