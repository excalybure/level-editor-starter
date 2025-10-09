# Contracts

No network or external service API endpoints are introduced by this feature.

Instead, internal C++ interface contracts will be defined during implementation:

Planned key interfaces (pseudo-IDL):

```text
MaterialSystem
  initialize(MaterialSystemConfig) -> Status
  loadMaterials(FilePath rootJson) -> Status (fail-fast: returns Fatal on first error)
  getMaterialHandle(string id) -> MaterialHandle | null
  getRootSignature(MaterialHandle) -> RootSignatureHandle
  getPipeline(MaterialHandle) -> PipelineHandle
  listMaterials() -> span<const MaterialInfo>
```

Status enum to align with fail-fast: { Ok, Error, Fatal }. Fatal triggers console::fatal before returning.

Additional helper components:
- JsonLoader (parse + include merge + cycle detect)
- Validator (schema + uniqueness + reference resolution)
- DefineExpander (hierarchical defines enforcement)
- RootSignatureBuilder
- PipelineBuilder / Cache

Detailed C++ signatures will be produced during implementation TDD.
