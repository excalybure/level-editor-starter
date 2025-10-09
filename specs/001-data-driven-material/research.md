# Phase 0 Research — Data‑Driven Material & Pipeline System

## Decisions & Rationale

### Parameter Types
- **Decision**: Limit to float, int, bool, float4.
- **Rationale**: Simplifies uniform packing & validation; covers immediate needs (scalars + colors/vectors) without matrix/array complexity.
- **Alternatives**: (a) Include float2/float3/matrices (adds alignment rules), (b) Allow textures/samplers as parameters (complicates root signature synthesis). Deferred for later.

### Shader Variant Strategy
- **Decision**: Only explicitly listed shader entries compiled.
- **Rationale**: Predictable build time, avoids combinatorial explosion, aligns with fail‑fast philosophy.
- **Alternatives**: Automatic permutation expansion (cartesian or flagged) — high complexity and caching burden; deferred.

### Error Handling Model
- **Decision**: Fail‑fast (fatal on any validation/duplication/schema error).
- **Rationale**: Prevents inconsistent runtime state; simplifies reasoning and testing; aligns with constitution deterministic error handling.
- **Alternatives**: Partial load with warnings (complex recovery paths) — rejected.

### Hot Reload
- **Decision**: Deferred.
- **Rationale**: Minimizes initial scope & complexity; pipeline change invalidation logic not yet needed.
- **Alternatives**: Full live reload (complex dependency tracking) or brute force reload — deferred.

### Hierarchical Defines
- **Decision**: Global → Pass → Material; duplicates fatal; no overrides.
- **Rationale**: Enforces strict clarity; avoids silent divergence.
- **Alternatives**: Allow overriding with precedence chain; requires override markers & conflict UI later.

### Pass Handling
- **Decision**: JSON config only for existing enum passes; no dynamic pass creation.
- **Rationale**: Keeps frame graph stable; avoids injection ordering issues.
- **Alternatives**: Dynamic pass registry — more complex scheduling code.

### Root Signature & PSO Generation
- **Decision**: Derive root signature from declared resources; hash inputs to cache PSO creations.
- **Rationale**: Eliminates duplicate pipeline creation; ensures consistency.
- **Alternatives**: Manual per-material code — current status quo; rejected.

### JSON Validation
- **Decision**: Central schema + structural validation (presence, types, uniqueness) before any GPU calls.
- **Rationale**: Fail early; reduces debugging time.
- **Alternatives**: Ad-hoc per-field checks — error scattering.

### Performance Targets
- **Decision**: <=2s build for 25 materials; define overhead <5%; steady frame cost <1% regression.
- **Rationale**: Enforces guardrails; measurable acceptance.
- **Alternatives**: No explicit targets — risks unnoticed regressions.

## Open (Deferred) Topics
- Hot reload architecture.
- Extended parameter types (matrices, arrays, textures, samplers).
- Dynamic pass registration.
- Error aggregation mode (collect all issues before fatal).
- Shader permutation optimization heuristics.

## Research Summary
All clarifications resolved; no outstanding NEEDS CLARIFICATION items blocking design. Proceed to Phase 1 design & contracts.
