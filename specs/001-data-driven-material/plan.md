# Implementation Plan: Data‑Driven Material & Pipeline System

**Branch**: `001-data-driven-material` | **Date**: 2025-10-08 | **Spec**: `specs/001-data-driven-material/spec.md`
**Input**: Feature specification from `/specs/001-data-driven-material/spec.md`

## Summary

Introduce a fail‑fast, data‑driven material & pipeline system that replaces hard‑coded root signature and graphics pipeline state creation with JSON‑declared definitions (`materials.json` + includes). The system parses hierarchical state & material definitions, validates them strictly (fatal on any duplication or schema violation), constructs root signatures and PSOs automatically, compiles only explicitly listed shader entries, and exposes material handles to existing rendering code. Initial scope limits parameter types (float, int, bool, float4) and does not implement hot reload or automatic shader variant generation.

## Technical Context

<!--
  ACTION REQUIRED: Replace the content in this section with the technical details
  for the project. The structure here is presented in advisory capacity to guide
  the iteration process.
-->

**Language/Version**: C++23 (existing project standard)  
**Primary Dependencies**: DirectX 12 API wrappers (existing), Catch2 (tests), nlohmann/json (assumed existing or to add)  
**Storage**: JSON files on disk (materials.json + included files)  
**Testing**: Catch2 unit & integration tests (Red→Green→Refactor)  
**Target Platform**: Windows (DirectX 12 level editor runtime)  
**Project Type**: Single native application + tests  
**Performance Goals**: Build <=25 material pipelines <2s cold; define expansion overhead <5% of pipeline build time; steady frame time regression <1%  
**Constraints**: Fail‑fast on any invalid data; no hot reload; parameter types limited; explicit shader list only  
**Scale/Scope**: Initial: up to ~25 materials (baseline), design for future growth (100+) without structural changes  

## Constitution Check

Gate Review (Pre-Phase 0):
- TDD: Will introduce tests for parser, validator, root signature synthesis, PSO caching before implementation code (PASS planned).
- Atomic changes: Plan phases decompose into small AFs (PASS planned).
- Const correctness: Enforced (PASS planned).
- No exceptions for flow: Use `console::error` / `console::fatal` + status enums / expected (PASS planned).
- Ownership: Use smart pointers / values; no raw owning pointers (PASS planned).
- Deterministic error handling: Fail‑fast with single termination point (PASS alignment).
- Progress updates: Will add progress entry after feature increments (PASS planned).
- Minimal public surface: Export only loader API + query handles (PASS planned).

Result: All gates satisfied or planned; proceed to Phase 0.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

[Gates determined based on constitution file]

## Project Structure

### Documentation (this feature)

```
specs/[###-feature]/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

### Source Code (repository root)
<!--
  ACTION REQUIRED: Replace the placeholder tree below with the concrete layout
  for this feature. Delete unused options and expand the chosen structure with
  real paths (e.g., apps/admin, packages/something). The delivered plan must
  not include Option labels.
-->

```
src/
├── core/
├── math/
├── graphics/
│   ├── gpu
│   ├── grid
│   ├── renderer
│   ├── material_system
│   └── shader_manager
├── editor/
├── engine/
├── platform/
└── runtime/

tests/

```

**Structure Decision**: Use existing single-project structure. New code added under `src/graphics/material_system/` (loader, validator, builder, cache) and `tests/` for unit/integration tests. No new top-level projects required.

## Complexity Tracking

*Fill ONLY if Constitution Check has violations that must be justified*

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| Fail-fast termination instead of partial recovery | Ensures deterministic state & early surfacing of authoring errors | Partial load introduces inconsistent runtime, increases test matrix |
| Added JSON schema validation layer | Prevents undefined behavior later in pipeline construction | Ad-hoc field checks lead to fragmented error handling |

## Constitution Re-Check (Post Design)

- TDD coverage points identified: parsing, include cycle detection, duplicate detection, schema validation, define hierarchy enforcement, root signature synthesis hashing, PSO cache, parameter default application.
- Atomic AF list feasible (each bullet above becomes separate AF group of small tests).
- Const correctness: All planned builder & validator helper functions will be pure/const where possible.
- No exceptions: Using logging + status returns; fatal paths centralized.
- Minimal surface: Only `MaterialSystem` interface exposed; subordinate components internal.
- Performance gates: Hashing & caching strategy defined early to avoid retrofitting.

Status: Ready to proceed to implementation tasks planning (`/speckit.tasks`).
