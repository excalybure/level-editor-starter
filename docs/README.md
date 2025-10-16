# Documentation Index

This directory contains all project documentation, organized by purpose.

## Directory Structure

```
docs/
├── archive/          # Completed tasks and historical progress tracking
├── milestones/       # Milestone and phase tracking documents
├── planning/         # Future plans and high-level roadmaps
├── migrations/       # Migration guides and refactoring plans
└── *.md             # Current design documents and guides
```

## Current Documentation

### Design Documents
- **[material_instance_plan.md](material_instance_plan.md)** - Material instance system design
- **[MATERIAL_SYSTEM_INTEGRATION.md](MATERIAL_SYSTEM_INTEGRATION.md)** - Material system architecture and integration
- **[REFLECTION_BASED_ROOT_SIGNATURE_PLAN.md](REFLECTION_BASED_ROOT_SIGNATURE_PLAN.md)** - Reflection-based root signature generation
- **[REFLECTION_IMPLEMENTATION_PROGRESS.md](REFLECTION_IMPLEMENTATION_PROGRESS.md)** - Reflection system implementation status
- **[GLTF_TEXTURE_SUPPORT_PLAN.md](GLTF_TEXTURE_SUPPORT_PLAN.md)** - glTF texture loading implementation plan

### Guides
- **[adding_objects_guide.md](adding_objects_guide.md)** - How to add new object types to the editor
- **[scene_format.md](scene_format.md)** - Scene serialization format specification
- **[editor_config.md](editor_config.md)** - Editor configuration system
- **[EDITOR_CONFIG_PROGRESS.md](EDITOR_CONFIG_PROGRESS.md)** - Editor config implementation status

### Analysis
- **[T4.0_UI_Integration_Analysis.md](T4.0_UI_Integration_Analysis.md)** - UI integration analysis

## Archive

Historical documents (completed work, old progress tracking):
- See [archive/](archive/) directory

## Milestones

Milestone and phase tracking documents:
- See [milestones/](milestones/) directory
- Active milestone documents track ongoing work
- Move to archive/ when completed

## Planning

Future roadmap and high-level planning:
- See [planning/](planning/) directory
- **PLAN.md** - Overall project plan
- **FUTURE.md** - Future feature ideas
- **MILESTONE_2.md** - Milestone 2 overview

## Migrations

Migration guides and refactoring plans:
- See [migrations/](migrations/) directory
- Guides for transitioning between system versions
- Refactoring plans for major changes

## Source Code Documentation

Each major source directory has its own README:
- **[/src/README.md](/src/README.md)** - Overall architecture overview
- **[/src/core/README.md](/src/core/README.md)** - Core utilities (console, time, strings)
- **[/src/math/README.md](/src/math/README.md)** - Math library
- **[/src/platform/README.md](/src/platform/README.md)** - Platform abstractions (DX12, Win32)
- **[/src/graphics/README.md](/src/graphics/README.md)** - Rendering systems
- **[/src/runtime/README.md](/src/runtime/README.md)** - ECS and game runtime
- **[/src/engine/README.md](/src/engine/README.md)** - Asset management and engine features
- **[/src/editor/README.md](/src/editor/README.md)** - Editor UI and tools

## Finding Information

### By Topic

- **Architecture**: `/src/README.md`, layer-specific READMEs
- **Materials**: `MATERIAL_SYSTEM_INTEGRATION.md`, `material_instance_plan.md`
- **Assets**: `/src/engine/README.md`, `GLTF_TEXTURE_SUPPORT_PLAN.md`
- **Editor**: `/src/editor/README.md`, `adding_objects_guide.md`
- **Scene Format**: `scene_format.md`
- **Configuration**: `editor_config.md`

### By Layer

- **Core utilities**: `/src/core/README.md`
- **Math operations**: `/src/math/README.md`
- **Platform/DX12**: `/src/platform/README.md`
- **Rendering**: `/src/graphics/README.md`
- **ECS/Runtime**: `/src/runtime/README.md`
- **Assets/Engine**: `/src/engine/README.md`
- **Editor/UI**: `/src/editor/README.md`

## Contributing Documentation

### When to Create New Documentation

- **Design documents**: Major new features or architectural changes
- **Guides**: Workflow or usage instructions for developers
- **Plans**: Implementation roadmaps for complex features

### Where to Put Documentation

- **Design docs**: `docs/*.md`
- **Progress tracking**: `docs/milestones/*.md`
- **Completed work**: `docs/archive/*.md`
- **Code docs**: README.md in relevant `src/` subdirectory

### Documentation Standards

- Use Markdown format
- Include code examples where relevant
- Link to related documents
- Keep focused and concise
- Update when implementation changes

## Maintenance

### Regular Review

- Move completed milestone docs to archive/
- Update implementation status docs as work progresses
- Remove stale "PROGRESS" docs once work is integrated
- Keep planning docs current with project direction

### Archive Policy

Move documents to archive/ when:
- Tasks are completed
- Milestones are finished
- Information is superseded by newer docs
- Document is purely historical

Retain in main docs/ when:
- Actively referenced
- Ongoing implementation
- Current architectural reference
- Living design document

## See Also

- **[/README.md](/README.md)** - Project overview and build instructions
- **[/specs/](/specs/)** - Feature specifications
- **[/.github/instructions/](/.github/instructions/)** - AI coding guidelines
