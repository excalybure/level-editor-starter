# Implementation Tasks — Data‑Driven Material & Pipeline System

**Branch**: `001-data-driven-material` | **Generated**: 2025-10-08 | **Spec**: `specs/001-data-driven-material/spec.md`

---

## Overview

This document breaks down the data‑driven material system implementation into atomic, testable tasks organized by **user story priority** (P1 → P2 → P3). Each task follows **strict TDD** (Red → Green → Refactor) and references **atomic functionalities (AFs)** from the specification and plan.

**MVP Scope**: Phase 3 (User Story 1 / P1) delivers the core value proposition—JSON-defined materials become usable with zero C++ changes. Phases 4–5 (P2–P3) add incremental features.

**Task Numbering**: Sequential T001–T0xx; tasks marked **[P]** can be parallelized with others in same phase.

---

## Phase 1: Project Setup & Foundation (Prerequisite)

These tasks establish the minimal infrastructure for TDD and data loading.

### T001: Add JSON parsing library to project [P] ✅
**User Story**: N/A (foundation)  
**AF**: Initialize JSON parsing capability  
**Reason**: Prerequisite for all JSON validation & loading; fail-fast needed  
**Blocking**: All subsequent tasks  
**Status**: COMPLETE (2025-01-18)  
**Acceptance**:
- `nlohmann/json` integrated via vcpkg or existing mechanism
- Test file loads & parses minimal JSON object without exception
- CMakeLists.txt references library in `material_system` target

**TDD Steps**:
1. Write test: load `{"test":1}` from string, assert key exists
2. Add nlohmann/json to vcpkg.json or existing dependency system
3. Update CMakeLists.txt for material_system target
4. Verify test passes with minimal JSON parse

**Estimate**: 30 min  
**Dependencies**: None  

---

### T002: Create material_system header scaffolding [P] ✅
**User Story**: N/A (foundation)  
**AF**: Establish code organization boundaries  
**Reason**: Defines file structure per plan.md (`src/graphics/material_system/`)  
**Blocking**: T003+  
**Status**: COMPLETE (2025-01-18)  
**Acceptance**:
- Directory `src/graphics/material_system/` exists
- Stub header files created (loader.h (JsonLoader), validator.h (Validator), builder.h (RootSignatureBuilder/PipelineBuilder), cache.h, material_system.h) with corresponding .cpp files
- CMakeLists.txt entry compiles empty implementation files
- Test target links against material_system library

**TDD Steps**:
1. Create directories & stub header files with forward declarations and empty .cpp files
2. Add source files to CMakeLists.txt target_sources
3. Write test including `material_system.h` header (empty stub)
4. Build & verify compilation

**Estimate**: 20 min  
**Dependencies**: None  

---

### T003: Implement console logging integration test [P] ✅
**User Story**: N/A (foundation)  
**AF**: Verify fail-fast termination works in test harness  
**Reason**: Constitution requires `console::fatal` for errors; tests must validate this  
**Blocking**: None (but informs all validation tasks)  
**Status**: COMPLETE (2025-01-18)  
**Acceptance**:
- Test can invoke `console::fatal` and detect termination (or exception in test context)
- Can verify error message content for clarity

**TDD Steps**:
1. Write test expecting fatal with specific message
2. Implement minimal fatal wrapper in test environment
3. Verify test fails when fatal not called
4. Verify test passes when fatal invoked with expected message

**Estimate**: 30 min  
**Dependencies**: None  

---

## Phase 2: Core Validation Infrastructure (User Story 1 Prerequisites)

These tasks implement foundational validation logic required before parsing materials.

### T004: Detect circular includes (AF: Include Cycle Detection) ✅
**User Story**: P1 (prerequisite for JSON loading)  
**AF**: Prevent infinite recursion in include resolution  
**Reason**: FR-011 mandates cycle detection with fatal termination  
**Blocking**: T007 (JSON merge)  
**Status**: COMPLETE (2025-01-18)  
**Acceptance**:
- Given two JSON files A (includes B) and B (includes A), loader detects cycle
- Fatal error logged with include chain trace
- Test verifies detection for direct cycle (A→B→A) and transitive (A→B→C→A)

**TDD Steps**:
1. **Red**: Write test with A→B→A cycle in temp files; expect fatal with chain trace
2. **Green**: Implement DFS-based cycle detector in `JsonLoader` (tracks include stack)
3. **Refactor**: Extract cycle detection into pure function; improve error message format

**Estimate**: 1 hour  
**Dependencies**: T001, T002  

---

### T005: Validate JSON schema structure (AF: Schema Validator Base) ✅
**User Story**: P1 (core validation)  
**AF**: Ensure top-level sections exist & are correct type  
**Reason**: FR-002 requires schema validation before entity resolution  
**Blocking**: T006, T007  
**Status**: COMPLETE (2025-01-18)  
**Acceptance**:
- Validator checks for required sections: `materials` (array), `renderPasses` (array), state blocks (objects)
- Missing section → fatal with JSON path
- Wrong type (e.g., materials is object not array) → fatal
- Optional sections (`includes`, `defines`) validated if present

**TDD Steps**:
1. **Red**: Write tests for missing `materials`, wrong type for `renderPasses`, etc.
2. **Green**: Implement `JsonValidator::validateSchema(json)` with section checks
3. **Refactor**: Use JSON schema library or manual checks; ensure clear error paths

**Estimate**: 1.5 hours  
**Dependencies**: T001, T002  

---

### T006: Validate parameter type constraints (AF: Parameter Type Validation) ✅
**User Story**: P1 (material parameters)  
**AF**: Enforce allowed types {float, int, bool, float4}  
**Reason**: FR-017, FR-018 restrict parameter types; fatal on others  
**Blocking**: T009  
**Status**: COMPLETE (2025-01-18)  
**Acceptance**:
- Test declares parameter with type "string" → fatal
- Test declares parameter with type "float4" and valid default → pass
- Test default value mismatch (int default for float param) → fatal

**TDD Steps**:
1. **Red**: Write test with invalid type; expect fatal
2. **Green**: Implement `ParameterValidator::validateType(param)` checking enum
3. **Refactor**: Add default value type checking; improve error messages

**Estimate**: 1 hour  
**Dependencies**: T005  

---

### T007: Merge JSON documents from includes (AF: Include Merge) ✅
**User Story**: P3 (reuse state blocks) + P1 (dependency)  
**AF**: Combine base + included files into single document  
**Reason**: FR-001 requires recursive include processing before validation  
**Blocking**: T008  
**Status**: COMPLETE (2025-01-18)  
**Acceptance**:
- Given `materials.json` with `"includes": ["states.json"]`, loader merges states
- State blocks from states.json available in merged document
- Duplicate ids across files → fatal (tested in T008)

**TDD Steps**:
1. **Red**: Write test with materials.json + states.json; verify merged content accessible
2. **Green**: Implement `JsonLoader::mergeIncludes(rootPath)` recursively loading files
3. **Refactor**: Cache loaded files; ensure file path resolution is safe (no traversal outside root)

**Estimate**: 1.5 hours  
**Dependencies**: T004, T005  

---

### T008: Detect duplicate IDs across all scopes (AF: Duplicate ID Detection) ✅
**User Story**: P1, P3 (validation)  
**AF**: Enforce unique identifiers globally after merge  
**Reason**: FR-008 mandates fatal on any duplicate id (materials, states, shaders, passes)  
**Blocking**: T009  
**Status**: COMPLETE (2025-01-18)  
**Acceptance**:
- Two materials with same id → fatal
- Material id duplicates state block id → fatal
- Duplicate detected across base + include → fatal
- Error message includes both locations (file, line if available)

**TDD Steps**:
1. **Red**: Write tests for duplicate material ids, cross-category duplicates
2. **Green**: Implement `DuplicateDetector::scan(mergedJson)` tracking all ids
3. **Refactor**: Use hash set for O(1) lookup; ensure clear error reporting with JSON paths

**Estimate**: 2 hours  
**Dependencies**: T007  

---

## Phase 3: User Story 1 (P1) — Core Material Loading & Usage

These tasks implement the MVP: JSON materials become usable without C++ changes.

### T009: Parse material definitions from JSON (AF: Material JSON Parser) ✅
**Status**: COMPLETE (2025-01-18)  
**User Story**: P1  
**AF**: Convert JSON material objects into runtime MaterialDefinition structs  
**Reason**: Core data ingestion for pipeline creation  
**Blocking**: T010, T011  
**Acceptance**:
- Given valid material JSON (id, pass, shaders, states, parameters), parser produces MaterialDefinition ✅
- Invalid field (missing required key) → error with path and empty return ✅
- Test parsing minimal material + material with all optional fields ✅
- Parse disabled material; verify enabled=false parsed correctly ✅

**TDD Steps**:
1. **Red**: Write test parsing valid material JSON; assert fields match ✅
2. **Green**: Implement `MaterialParser::parse(jsonMaterial)` populating struct ✅
3. **Refactor**: Const correctness, remove debug logging ✅

**Estimate**: 2 hours  
**Actual**: ~2 hours  
**Dependencies**: T006, T008 (both complete)  
**Implementation**: MaterialParser class in parser.h/cpp with parse() and parseParameterType() methods

---

### T010: Validate material references (AF: Reference Resolution) ✅
**Status**: COMPLETE (2025-01-18)  
**User Story**: P1  
**AF**: Ensure material references (pass, states, shaders) exist  
**Reason**: FR-012 requires resolving references; fatal on undefined  
**Blocking**: T013  
**Acceptance**:
- Material references pass "depthPrepass" which doesn't exist in enum → error + false return ✅
- Material references rasterizer state "myState" not in merged document → error + false return ✅
- Material references undefined shader → error + false return ✅
- Valid references resolve successfully ✅

**TDD Steps**:
1. **Red**: Write tests with undefined pass, undefined state block, undefined shader; expect false ✅
2. **Green**: Implement `ReferenceValidator::validateReferences(material, knownPasses, document)` ✅
3. **Refactor**: Const correctness, run full material system test suite ✅

**Estimate**: 1.5 hours  
**Actual**: ~1.5 hours  
**Dependencies**: T009 (complete)  
**Implementation**: ReferenceValidator class in validator.h/cpp with pass/shader/state validation

---

### T011: Enforce hierarchical define uniqueness (AF: Define Hierarchy Validator) ✅
**Status**: COMPLETE (2025-01-18)  
**User Story**: P1 (shader defines)  
**AF**: Detect duplicate define names across global/pass/material scopes  
**Reason**: FR-019 requires fatal on duplicates; hierarchical propagation without overrides  
**Blocking**: T012  
**Acceptance**:
- Global define "FOO" + material define "FOO" → error + false return ✅
- Pass define "BAR" + material using that pass defines "BAR" → error + false return ✅
- Global define + pass define duplicate → error + false return ✅
- Unique defines at each level propagate correctly via getMergedDefines() ✅
- Only shader entries explicitly listed in JSON are compiled (no permutation generation) ✅

**TDD Steps**:
1. **Red**: Write tests with duplicate defines at different levels; expect false ✅
2. **Green**: Implement `DefineValidator::checkHierarchy(globalDefines, passDefines, materialDefines, materialId)` ✅
3. **Refactor**: Const correctness, set-based tracking, getMergedDefines() utility ✅

**Estimate**: 2 hours  
**Actual**: ~1.5 hours  
**Dependencies**: T009 (complete)  
**Implementation**: DefineValidator class in validator.h/cpp with hierarchy checking and merge utility

---

### T012: Compile shader with hierarchical defines (AF: Shader Compilation Integration)
**User Story**: P1  
**AF**: Pass merged defines to existing shader compiler  
**Reason**: FR-006, FR-019 require explicit shader list + hierarchical defines  
**Blocking**: T013  
**Acceptance**:
- Given material with global, pass, and material-level defines, shader compiler receives all merged defines
- Compiled shader bytecode stored and reused
- Missing shader file → fatal with path

**TDD Steps**:
1. **Red**: Write test compiling minimal shader with defines; verify bytecode non-empty
2. **Green**: Implement `ShaderCompiler::compile(shaderEntry, mergedDefines)` invoking existing compiler
3. **Refactor**: Cache compiled shaders by (file + defines) hash; ensure deterministic ordering

**Estimate**: 2 hours  
**Dependencies**: T011  
**Status**: ✅ COMPLETE (2025-01-10)

---

### T013: Generate root signature from material resources (AF: Root Signature Synthesis)
**User Story**: P1  
**AF**: Build root signature from shader reflection (DXIL) / explicit parameter declarations  
**Reason**: FR-004 mandates automatic root signature construction from material data  
**Blocking**: T014  
**Acceptance**:
- Given material with parameters & shader resources, system generates root signature spec
- Spec hashed deterministically for caching
- Duplicate resource binding names within single material → fatal

**TDD Steps**:
1. **Red**: Write test with material declaring CBV & SRV; assert root sig spec includes both
2. **Green**: Implement `RootSignatureBuilder::build(materialDef, shaderReflections)` aggregating bindings
3. **Refactor**: Sort bindings for deterministic hashing; validate no duplicates

**Estimate**: 3 hours  
**Dependencies**: T012  
**Status**: ✅ COMPLETE (2025-01-10)

---

### T014: Build & cache pipeline state objects (AF: PSO Construction & Caching)
**User Story**: P1  
**AF**: Create DirectX 12 PSO from material + pass + states  
**Reason**: FR-005, FR-007 require automatic PSO creation replacing hard-coded calls  
**Blocking**: T015  
**Acceptance**:
- Given MaterialDefinition + RenderPassConfig + state block references, system builds PSO
- PSO cached by hash (material id + pass + states + shaders)
- Hash collision (different inputs → same hash) → fatal
- Test PSO handle is valid & reused on second request

**TDD Steps**:
1. **Red**: Write test requesting PSO for material+pass; assert handle valid
2. **Green**: Implement `PipelineBuilder::buildPSO(materialDef, passConfig, stateBlocks, rootSig)`
3. **Refactor**: Add cache layer (`PipelineCache::get(hash) -> PSO?`); ensure threadsafe if needed

**Estimate**: 3 hours  
**Dependencies**: T013  

---

### T015: Expose material system API to renderer (AF: Public Interface)
**User Story**: P1  
**AF**: Provide handles for existing renderer to query materials & PSOs  
**Reason**: FR-012 requires runtime access with no draw submission changes  
**Blocking**: T016  
**Acceptance**:
- Renderer can call `MaterialSystem::getMaterialHandle("myMaterial")` → handle
- Renderer can call `MaterialSystem::getPSO(handle, pass)` → PSO
- Undefined material id → returns null/error (fatal during load, not query)

**TDD Steps**:
1. **Red**: Write test querying material handle & PSO; assert non-null
2. **Green**: Implement `MaterialSystem::initialize(materialsJsonPath)` + query APIs
3. **Refactor**: Ensure handle type is opaque (internal index); validate handle in queries

**Estimate**: 1.5 hours  
**Dependencies**: T014  

---

### T016: Integration test — Load material JSON and render object (AF: End-to-End P1)
**User Story**: P1 (acceptance scenario 1)  
**AF**: Verify complete flow from JSON to rendered frame  
**Reason**: Success criterion SC-003; validate zero C++ changes required  
**Blocking**: None (milestone)  
**Acceptance**:
- Test provides `materials.json` with one material
- Application initializes material system
- Existing renderer uses material handle to draw object
- Rendered output validated (framebuffer pixel test or manual check)
- Performance: completes in <2s for 25 materials (SC-002)

**TDD Steps**:
1. **Red**: Write integration test loading minimal JSON; verify material available
2. **Green**: Wire MaterialSystem::initialize into app startup
3. **Refactor**: Remove hard-coded PSO/root signature creation for tested material; route through system

**Estimate**: 2 hours  
**Dependencies**: T015  

---

## Phase 4: User Story 2 (P2) — Render Pass Configuration

These tasks add data-driven configuration for existing render passes.

### T017: Parse render pass configurations from JSON (AF: RenderPass JSON Parser)
**User Story**: P2  
**AF**: Load pass-specific settings (targets, clear ops, materials list)  
**Reason**: FR-014 maps JSON config onto existing pass enum  
**Blocking**: T018  
**Acceptance**:
- Given JSON with `renderPasses` array entry for "depthPrepass", parser extracts config
- Pass name not in enum → fatal (FR-014)
- Test parsing pass with all fields vs minimal pass

**TDD Steps**:
1. **Red**: Write test parsing pass config; assert fields populated
2. **Green**: Implement `RenderPassParser::parse(jsonPass, knownEnumPasses)`
3. **Refactor**: Validate pass name against enum before parsing rest

**Estimate**: 1.5 hours  
**Dependencies**: T009 (similar parser structure)  

---

### T018: Validate render pass compatibility (AF: Pass Compatibility Validation)
**User Story**: P2  
**AF**: Ensure blend target count matches RT formats; depth state only when depth buffer present  
**Reason**: FR-015 requires compatibility checks  
**Blocking**: T019  
**Acceptance**:
- Pass with 2 render targets + blend state for 3 targets → fatal
- Pass without depth buffer + material using depth test → fatal
- Valid configurations pass

**TDD Steps**:
1. **Red**: Write tests with mismatched target counts, depth usage conflicts; expect fatal
2. **Green**: Implement `PassCompatibilityValidator::validate(passConfig, materials)`
3. **Refactor**: Reuse material state references from T010; generate clear error messages

**Estimate**: 2 hours  
**Dependencies**: T017  

---

### T019: Apply pass configuration to existing pass enum iteration (AF: Pass Config Application)
**User Story**: P2  
**AF**: Wire JSON pass settings into renderer's pass execution logic  
**Reason**: FR-014 requires mapped configuration without reordering  
**Blocking**: T020  
**Acceptance**:
- Renderer iterates pass enum in fixed order
- For each pass, system provides config (clear ops, materials)
- Test changing pass clear color in JSON → reflected in render

**TDD Steps**:
1. **Red**: Write test retrieving pass config; assert clear color matches JSON
2. **Green**: Implement `MaterialSystem::getPassConfig(passEnum) -> PassConfig?`
3. **Refactor**: Cache pass configs indexed by enum; ensure ordering unchanged

**Estimate**: 1.5 hours  
**Dependencies**: T018  

---

### T020: Integration test — Modify pass config and verify render change (AF: End-to-End P2)
**User Story**: P2 (acceptance scenario)  
**AF**: Validate no C++ changes required for pass tuning  
**Reason**: Success criterion SC-003 extended to pass config  
**Blocking**: None (milestone)  
**Acceptance**:
- Edit pass JSON (change clear color)
- Restart application
- Verify clear color changed (framebuffer test)
- No code modifications

**TDD Steps**:
1. **Red**: Write integration test with two configs (before/after); verify output difference
2. **Green**: Ensure MaterialSystem::initialize applies pass configs
3. **Refactor**: Document pass config fields in quickstart.md

**Estimate**: 1 hour  
**Dependencies**: T019  

---

## Phase 5: User Story 3 (P3) — State Block Includes & Reuse

These tasks enable splitting common states into separate files.

### T021: Parse includes array and load external files (AF: Includes Loading)
**User Story**: P3  
**AF**: Process `includes` section and merge external JSON  
**Reason**: FR-001, User Story 3 scenario 1  
**Blocking**: T022  
**Acceptance**:
- Given `materials.json` with `"includes": ["states.json"]`, loader reads states.json
- State blocks from states.json available for material reference
- File not found → fatal with path

**TDD Steps**:
1. **Red**: Write test with materials.json + states.json; verify state block accessible
2. **Green**: Enhance `JsonLoader::mergeIncludes` from T007 to handle top-level includes array
3. **Refactor**: Ensure relative path resolution secure (no directory traversal)

**Estimate**: 1 hour  
**Dependencies**: T007 (merge logic exists)  

---

### T022: Detect duplicate state IDs across includes (AF: Include Duplicate Detection)
**User Story**: P3  
**AF**: Fatal on same state block id in base + include  
**Reason**: User Story 3 scenario 2; FR-008 extended  
**Blocking**: T023  
**Acceptance**:
- states.json defines rasterizer "myState", materials.json also defines "myState" → fatal
- Error message includes both file paths

**TDD Steps**:
1. **Red**: Write test with duplicate state id across files; expect fatal
2. **Green**: Extend `DuplicateDetector` from T008 to track source file per id
3. **Refactor**: Improve error message format with file locations

**Estimate**: 1 hour  
**Dependencies**: T008, T021  

---

### T023: Integration test — Refactor states to separate file (AF: End-to-End P3)
**User Story**: P3 (acceptance scenario)  
**AF**: Verify materials resolve identically pre/post include refactor  
**Reason**: Confirm maintainability improvement without behavior change  
**Blocking**: None (milestone)  
**Acceptance**:
- Start with monolithic materials.json
- Extract state blocks to states.json + add include reference
- Restart application
- Verify materials still render identically (hash pipelines match)

**TDD Steps**:
1. **Red**: Write integration test with two JSON layouts (monolithic vs split); assert PSO hashes identical
2. **Green**: Ensure merging preserves semantics
3. **Refactor**: Document include patterns in quickstart.md

**Estimate**: 1 hour  
**Dependencies**: T022  

---

## Phase 6: Polish, Performance & Documentation

Final tasks ensuring production readiness and constitution compliance.

### T024: Measure & log material system initialization time (AF: Performance Instrumentation)
**User Story**: All (non-functional)  
**AF**: Validate SC-002 (<2s for 25 materials) and SC-006 (<5% define overhead)  
**Reason**: Success criteria require measurable performance  
**Blocking**: T025  
**Acceptance**:
- System logs total init time and per-phase breakdown (parsing, validation, PSO build, define expansion)
- Test with 25 material JSON verifies <2s on dev machine
- Define expansion overhead isolated and measured

**TDD Steps**:
1. **Red**: Write test loading 25 materials; assert total time <2s
2. **Green**: Add timers around init phases; log results via `console::info`
3. **Refactor**: Aggregate metrics (PSO count, shader compile count) for diagnostics

**Estimate**: 1.5 hours  
**Dependencies**: T016, T020, T023  

---

### T025: Full negative test suite (AF: Comprehensive Error Coverage)
**User Story**: All (validation)  
**AF**: Validate SC-004 (100% error scenarios handled)  
**Reason**: Constitution requires fail-fast on any invalid input  
**Blocking**: T026  
**Acceptance**:
- Tests cover: schema errors, duplicate ids, missing files, undefined references, type violations, circular includes, compatibility mismatches
- Each test verifies fatal termination with clear error message
- 100% coverage of validation paths

**TDD Steps**:
1. **Red**: Write comprehensive test suite with all edge cases from spec.md
2. **Green**: Ensure each validation path triggers fatal
3. **Refactor**: Organize tests by error category; verify error messages include JSON paths

**Estimate**: 3 hours  
**Dependencies**: All validation tasks (T004–T011, T017–T018, T021–T022)  

---

### T026: Remove hard-coded pipeline creation calls (AF: Legacy Code Removal)
**User Story**: P1 (cleanup)  
**AF**: Validate SC-001 (100% removal for targeted materials)  
**Reason**: End goal is eliminating explicit CreateRootSignature/CreateGraphicsPipelineState calls  
**Blocking**: T027  
**Acceptance**:
- Audit codebase for `CreateRootSignature`, `CreateGraphicsPipelineState` calls related to materials covered by system
- Replace with MaterialSystem queries or delete
- Build passes with no remaining hard-coded calls for covered materials

**TDD Steps**:
1. **Red**: Write test querying material PSO; verify non-null (system provides it)
2. **Green**: Remove hard-coded creation in renderer
3. **Refactor**: Audit via grep; document remaining calls (if any) for future migration

**Estimate**: 2 hours  
**Dependencies**: T015  

---

### T027: Update documentation and quickstart guide (AF: User Documentation)
**User Story**: All  
**AF**: Ensure developers can author materials without code changes  
**Reason**: Success criterion SC-003 requires usability validation  
**Blocking**: None (milestone)  
**Acceptance**:
- quickstart.md includes complete JSON examples (minimal, full-featured, with includes)
- Common errors documented with solutions
- Performance tips included

**TDD Steps**:
1. Review quickstart.md (already created in Phase 1 design)
2. Add any missing examples based on integration tests
3. Include troubleshooting section for fatal errors

**Estimate**: 1 hour  
**Dependencies**: T023  

---

### T028: Final integration test — Baseline frame time regression (AF: Performance Validation)
**User Story**: All (non-functional)  
**AF**: Validate SC-005 (<1% steady-state regression)  
**Reason**: Ensure system doesn't degrade runtime performance  
**Blocking**: None (milestone)  
**Acceptance**:
- Benchmark scene with materials pre-integration (hard-coded PSOs)
- Run same scene with data-driven system
- Verify frame time difference <1%
- Steady-state measurement (after first frame caching)

**TDD Steps**:
1. **Red**: Write benchmark test measuring frame time delta
2. **Green**: Ensure MaterialSystem queries are O(1) at runtime (no parsing per-frame)
3. **Refactor**: Profile hotspots if needed; document cache hit rates

**Estimate**: 1.5 hours  
**Dependencies**: T024, T026  

---

## Task Summary by User Story

| User Story | Phase | Task Range | Count | MVP? |
|------------|-------|------------|-------|------|
| Foundation | 1–2 | T001–T008 | 8 | Yes |
| **P1** — Core Material Loading | 3 | **T009–T016** | **8** | **Yes (MVP)** |
| **P2** — Pass Configuration | 4 | **T017–T020** | **4** | No |
| **P3** — Includes & Reuse | 5 | **T021–T023** | **3** | No |
| Polish & Validation | 6 | T024–T028 | 5 | Partial |

**Total Tasks**: 28  
**MVP Scope**: Phases 1–3 + T024, T025, T026, T028 (critical validation/perf)  
**MVP Task Count**: ~20 tasks  
**Estimated MVP Effort**: ~35–40 hours (assuming 1–3 hour tasks)

---

## Parallelization Opportunities

Tasks marked **[P]** can be worked in parallel within same phase (no inter-dependencies):

- **Phase 1**: T001 [P], T002 [P], T003 [P]
- **Phase 2**: T005 [P] (after T001–T002), T006 [P] (after T005)
- **Phase 3**: Sequential due to dependencies (T009 → T010 → T011 → T012 → T013 → T014 → T015 → T016)
- **Phase 4**: T017 [P] (after T009), T018 → T019 → T020
- **Phase 5**: T021 → T022 → T023
- **Phase 6**: T024, T025, T026 can overlap; T027 independent; T028 requires all prior

**Recommended Approach**: Single developer should complete sequentially per user story; team of 2–3 could parallelize phases (one on P1, one on P2 prep, one on foundation).

---

## MVP Recommendation

**Ship After**: Phase 3 (T016) + T024 (perf measurement) + T025 (negative tests) + T026 (legacy removal) + T028 (regression validation)

**Rationale**: User Story 1 (P1) delivers core value (data-driven materials with zero C++ changes). P2 and P3 are incremental features that don't block initial adoption. Validation and performance tasks (T024, T025, T028) ensure production quality.

**Deferred to Next Iteration**: User Story 2 (render pass config) and User Story 3 (includes) can follow MVP based on user feedback.

---

## Constitution Compliance Checklist

- ✅ **TDD**: Every implementation task includes Red→Green→Refactor steps. Tag test with [material-system]..."
- ✅ **Atomic functionality**: Each task is a single AF with focused scope
- ✅ **Fail-fast**: All validation tasks enforce fatal termination (console::fatal)
- ✅ **Const correctness**: Planned in refactor steps (validators are pure functions where possible)
- ✅ **No exceptions for flow**: Status returns + logging used throughout
- ✅ **Minimal surface**: Only MaterialSystem API exposed (T015)
- ✅ **Performance gates**: T024, T028 validate SC-002, SC-005, SC-006
- ✅ **Progress updates**: After each user story phase, update PROGRESS_2.md

---

## Next Steps

1. Review tasks with team/stakeholders
2. Prioritize MVP scope (recommend Phases 1–3 + validation tasks)
3. Begin TDD implementation starting with **T001** (JSON library integration)
4. After each user story phase, update `PROGRESS_2.md` with completion notes
5. Generate commit messages per task completion using constitution template

**Ready to implement**: Yes — all prerequisites (spec, plan, design, tasks) complete.

---

*End of tasks.md — Generated via `/speckit.tasks` workflow*
