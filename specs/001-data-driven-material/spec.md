# Feature Specification: Data‑Driven Material & Pipeline System

**Feature Branch**: `001-data-driven-material`  
**Created**: 2025-10-08  
**Status**: Draft  
**Input**: User description: "I need to write a material system, a data driven system that is used to create of root signatures and pipeline state objects using data instead of explicitly in code. The end goal is to remove all explicit calls to CreateRootSignature & CreateGraphicsPipelineState from the code base. That system would be driven by one or more json files. materials.json is an example. In this file, we specify things such as depthStencilStates, rasterizerStates, blendStates & renderTargetStates. We also specify the render passes the application will support. Then we have a list of materials. This file should add 'includes' kvp, so that we can put states in say a states.json file. This will help keep things clean. All functionality in the code base should be able to use this file to create the appropriate root signature & pso, compile the required shaders. All the hard-coded shaders should be removed. json file supports defines. These defines can be at several levels. A define at a given level applies to shader at the current level or sub levels. In case of name duplication, use console::fatal. Do not try to recover. Similarly, if there is an error anywhere, print that error using error so that the application terminates. All rendering passes will be listed in an enum. Rendering code will iterate over the values of this enum as a function of the rendering mode. For example, with lit rendering, there will first be depth prepass followed by lit opaque, lit translucent, post-process and finally grid."
### User Story 1 - Author defines a material in JSON and it becomes usable (Priority: P1)

An editor or engine developer adds or edits entries in `materials.json` (and optionally split out files via `includes`) describing render passes, pipeline / state blocks, shader references, and material parameters. On application launch (or hot reload), the system parses the JSON, validates references, builds any missing root signatures & pipeline state objects, compiles required shaders, and registers the material so that existing scene assets referencing it render without code changes.

**Why this priority**: Core value proposition: replace scattered hard‑coded pipeline creation with centralized data so new materials / passes require zero C++ changes, accelerating iteration and reducing defects.

**Independent Test**: Provide a minimal `materials.json` with one pass and one material; delete all corresponding hard‑coded creation calls; application still renders the object with expected states.

**Acceptance Scenarios**:

1. **Given** a valid `materials.json` defining a material referencing existing shaders, **When** the application starts, **Then** the material is registered and objects tagged with it render correctly using specified states.
2. **Given** a modified JSON (after editing), **When** the application is restarted, **Then** the changed state (e.g., rasterizer setting) is reflected in rendering (hot reload deferred; restart required).
### User Story 2 - Configure existing engine render passes via data (Priority: P2)

An engineer adjusts configuration (targets, clear ops, materials bound) for passes that already exist in the engine's render pass enumeration. On launch the system applies JSON configuration to those known passes in their established order without requiring C++ changes.

**Why this priority**: Still delivers iteration gains by moving pass tuning to data, while acknowledging new pass creation requires a code enum addition (outside initial scope).

**Independent Test**: Modify JSON to change a pass's clear color and associated materials; restart application; verify change applied with no code modifications.

**Acceptance Scenarios**:

1. **Given** a JSON configuration for an existing enumerated pass, **When** the engine initializes, **Then** the pass uses the configured targets/states.
2. **Given** JSON attempts to configure a non-existent pass name, **When** loading, **Then** the system reports an error and ignores that configuration block.
### User Story 3 - Reuse common state & shader blocks via includes (Priority: P3)

A developer factors common depth / rasterizer / blend state presets and shared shader parameter blocks into separate JSON files referenced through an `includes` array. The loader merges them, allowing concise material definitions while still enabling override.

**Why this priority**: Improves maintainability / readability but can follow after core pipeline creation.

**Independent Test**: Place shared states in `states.json`, reference from `materials.json` via `includes`; verify materials using those named states resolve identically to pre‑refactor behavior.

**Acceptance Scenarios**:

1. **Given** an `includes` entry pointing to an existing file, **When** loading, **Then** its state blocks are available for reference before materials resolve.
2. **Given** two includes (or include + base) defining the same state id, **When** loading, **Then** the system emits a fatal validation error and aborts material system initialization until the duplication is resolved.
### Edge Cases

- Missing shader file or unreadable shader source → log error via `console::fatal`, then terminate (fail-fast policy).
- Duplicate id (material, state block, pass config, shader entry, define) → `console::fatal` and terminate immediately (no partial recovery).
- Unsupported render target format specified for an enumerated pass → error + terminate.
- Circular includes (A includes B, B includes A) → detection triggers `console::fatal` with include chain trace.
- Material references undefined state block or pass → error + terminate (since partial load is disallowed).
- Define shadowing at a lower level (e.g., material-level define duplicates a global define) → fatal duplication error.
- Invalid JSON schema (missing required section, wrong type) → fatal with path + JSON pointer where possible.
- Exceedingly large number of materials (stress case) → still must parse & build under SC performance budget; if memory threshold exceeded, fatal.
## Clarifications

### Session 2025-10-08
- Q1: Parameter types scope → A (Only float, int, bool, float4 permitted; others are fatal.)
- Q2: Shader variants strategy → A (Only explicitly listed shader entries; no automatic permutations from defines.)

### Functional Requirements

- **FR-001**: System MUST load one or more JSON files beginning with `materials.json` and recursively process any `includes` array entries (relative paths) before resolving references.
- **FR-002**: System MUST validate JSON schema: sections for `includes`, `depthStencilStates`, `rasterizerStates`, `blendStates`, `renderTargetStates`, `renderPasses`, and `materials`.
- **FR-003**: System MUST allow each state block to be referenced by a unique string id within its category.
- **FR-004**: System MUST construct root signatures automatically from material declarations of shader resources (textures, samplers, constant buffers / parameter blocks) without manual code additions.
- **FR-005**: System MUST construct graphics pipeline state objects (PSOs) from material + pass + state block references (input layout, shaders, states, render target formats, depth-stencil usage).
-- **FR-006**: System MUST compile or load only the explicitly listed shader entries (per stage) present in JSON;
-- **FR-007**: System MUST eliminate all direct calls in code to explicit root signature and graphics PSO creation for materials covered by the data system (legacy code removed or routed through data loader).
-- **FR-008**: System MUST treat duplicate ids (materials, passes, state blocks, shaders, defines) across base + includes as fatal validation errors; initialization fails without partial load.
-- **FR-009**: System MUST, on any validation / resolution error, log via `console::fatal` and terminate the application (no attempt at partial recovery).
-- **FR-010**: (Deferred) Hot reload support is explicitly out of initial scope; restart required for changes. (Tracked for future iteration, not part of success criteria.)
-- **FR-011**: System MUST detect cycles in includes and terminate with a descriptive chain in the log.
-- **FR-012**: System MUST map materials to runtime handles/ids accessible by existing rendering components with no change to draw submission besides referencing the material id.
-- **FR-013**: System MUST capture performance metrics (count of pipelines built, time spent) for diagnostics (non-user facing).
-- **FR-014**: System MUST map JSON configuration onto the existing compile‑time render pass enumeration; ordering derives solely from that enumeration (no dynamic reordering in initial scope).
-- **FR-015**: System MUST validate compatibility: blend target count matches render target formats; depth state only when depth buffer present; shader outputs match target formats.
-- **FR-016**: System MUST allow marking a material as disabled (boolean flag) so it is skipped during load (still validated; disabled state cannot mask errors in required fields; basic validation: shader compilation not required).
-- **FR-017**: System MUST support parameter defaults only for allowed types {float,int,bool,float4}; defaults applied if not overridden in entity/component data.
-- **FR-018**: System MUST reject (fatal) any declared parameter whose type is outside {float,int,bool,float4}.
-- **FR-019**: System MUST support hierarchical shader defines: global-level, pass-level, material-level; each level applies to shaders at its level and below unless overridden (overrides are disallowed in initial scope; duplication is fatal).
-- **FR-020**: System MUST put implementation in src/graphics/material_systems
### Key Entities

- **Material Definition**: Declarative object referencing a render pass, shaders (by logical name or file path), state block ids, and parameter list (name, type, default value). Includes enabled flag and version/hash for change detection.
  - Allowed parameter types (initial scope): float, int, bool, float4 (used for colors & generic 4D vectors). Any other type declaration → fatal.
- **Render Pass**: Describes target formats / attachments, depth usage, clear ops, order/dependencies, and list of materials participating or referencing it.
- **State Block (DepthStencil / Rasterizer / Blend / RenderTarget)**: Named preset of API-agnostic state fields consumed when building PSOs.
- **Shader Entry**: Logical identifier mapping to shader source path + stage + optional defines.
- **Root Signature Spec**: Derived (not authored) structure enumerating resource bindings required by all shaders in a material; used to build / cache the root signature.
- **Pipeline Cache Entry**: Record including hash of contributing inputs (pass, states, shaders, root signature spec) and handle to runtime PSO.
- **Include Set**: Ordered list of external JSON files merged into root document.
### Measurable Outcomes

- **SC-001**: 100% of existing hard‑coded root signature & graphics PSO creation sites are removed or routed through the data system (auditable diff) for targeted feature set.
- **SC-002**: Loading a project with <= 25 materials builds all required pipelines in under 2 seconds on a standard dev machine (cold cache).
- **SC-003**: Incremental addition of a new material (JSON edit only) requires zero C++ changes and results in a rendered object on next launch with no code modifications.
- **SC-004**: Any invalid input scenario (schema error, duplicate id, missing shader, undefined reference) results in a single termination after logging a clear error message containing file path + identifier (100% coverage in negative test suite).
- **SC-005**: No frame time regression > 1% in steady state compared to pre‑integration baseline for unchanged scenes.
- **SC-006**: Total define expansion + application time adds < 5% overhead to overall pipeline build phase in benchmark project.
### Assumptions

- Shader compilation mechanism already exists (See ShaderManager.h); system will invoke it using logical shader definitions.
- Initial scope targets graphics (no compute pipelines unless added later).
- Hot reload deferred.
- Strict fatal policy replaces earlier partial‑load concept; no fallback material.
- Define precedence is hierarchical but duplicates are disallowed (fatal) rather than overridden.

### Out of Scope (Initial Release)

- Authoring UI for materials (manual JSON editing only).
- Automatic shader permutation reduction / specialization heuristics.
- Cross-platform backend abstraction beyond current renderer.

### Removed Clarifications (Resolved)

Hot reload deferred (restart required). Duplicate ids and define name collisions are fatal errors (abort initialization). Pass ordering fixed by existing enumeration; data config cannot add new passes in initial scope.

### Shader Define Hierarchy

Defines may be declared at:
1. Global section (top-level in `materials.json`).
2. Pass configuration block.
3. Material definition block.

Accumulation rules:
- All defines in ancestor scopes apply to descendant shaders.
- Duplicate names at any lower scope are forbidden (fatal) rather than overridden.
- Defines propagate only downward; materials cannot introduce names already used globally or by their pass.
- Future iteration may allow explicit override with an `override: true` flag.

### Future Considerations

- Introduce hot reload with dependency diffing.
- Allow controlled define overrides via explicit opt-in.
- Data‑only creation of new passes (dynamic registry) beyond enum.
- Per‑material shader permutation optimization / caching heuristics.
- Optional structured error aggregation mode (collect all errors before terminating) for authoring workflows.

# End of Specification
*** End Patch
  Think of each story as a standalone slice of functionality that can be:
