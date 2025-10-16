# level-editor-starter Development Guidelines

Auto-generated from all feature plans. Last updated: 2025-10-15

## Active Technologies
- **Language**: C++23 (concepts, ranges, std::expected, etc.)
- **Graphics API**: DirectX 12
- **Testing**: Catch2
- **Serialization**: nlohmann/json
- **UI**: ImGui + ImGuizmo

## Project Structure

```
src/
├── core/          # Zero-dependency utilities (console, time, strings)
├── math/          # Mathematical types and operations
├── platform/      # OS and API abstractions (Win32, DirectX 12, PIX)
├── graphics/      # Rendering systems (GPU, materials, shaders)
├── runtime/       # Game runtime (ECS, components, scene)
├── engine/        # Asset management and engine features
├── editor/        # Editor-specific UI and tools
└── main.cpp       # Application entry point

tests/             # Unit and integration tests (Catch2)
docs/
├── archive/       # Completed tasks and historical documents
├── milestones/    # Milestone and phase tracking
├── planning/      # Future roadmaps and plans
├── migrations/    # Migration guides and refactoring plans
├── README.md      # Documentation index
└── *.md          # Current design documents and guides
shaders/           # HLSL shader source files
specs/             # Feature specifications (e.g., 001-data-driven-material/)
```

## Documentation Organization

### Finding Code Information
Each source directory has a **README.md** explaining its purpose, key components, and usage:
- `/src/README.md` - Architecture overview and dependency hierarchy
- `/src/core/README.md` - Console, time, strings utilities
- `/src/math/README.md` - Math types and operations
- `/src/platform/README.md` - DirectX 12, Win32, PIX wrappers
- `/src/graphics/README.md` - GPU resources, materials, shaders
- `/src/runtime/README.md` - ECS, components, systems
- `/src/engine/README.md` - Asset manager, glTF loader, camera
- `/src/editor/README.md` - UI, gizmos, commands, panels

### Finding Design Documentation
- `/docs/README.md` - Complete documentation index
- `/docs/` - Current design docs (materials, reflection, glTF, etc.)
- `/docs/milestones/` - Ongoing milestone tracking
- `/docs/archive/` - Completed work and historical documents

## Commands

**Build (Debug)**:
```cmd
cmake --build build/vs2022-x64 --config Debug
```

**Run Tests**:
```cmd
build\vs2022-x64\Debug\unit_test_runner.exe
```

**List Tests**:
```cmd
build\vs2022-x64\Debug\unit_test_runner.exe --list-tests "*pattern*"
```

**Run Specific Tests**:
```cmd
build\vs2022-x64\Debug\unit_test_runner.exe "*test_name*"
```

## Code Style

**Naming Conventions**:
- Types/Classes: `PascalCase`
- Functions/Methods: `camelCase`
- Variables: `camelCase`
- Members: `m_camelCase`
- Constants: `kPascalCase`
- Namespaces: `lowercase`
- Files: `lowercase_with_underscores`

**C++23 Standards**:
- Use concepts for template constraints
- Prefer `std::expected` for error handling
- Use ranges and algorithms
- All non-mutating functions/locals must be `const`
- Prefer `constexpr` where applicable
- Use `std::string_view` and `std::span` for non-owning views

**Dependencies**:
Follow the strict layer hierarchy:
```
core, math (Layer 0 - no dependencies)
    ↓
platform (Layer 1)
    ↓
graphics (Layer 2)
    ↓
runtime (Layer 3)
    ↓
engine (Layer 4)
    ↓
editor (Layer 5)
```

## Recent Changes
- **2025-10-15**: Reorganized documentation structure with folder READMEs
- **2025-10-08**: Added data-driven material system (001-data-driven-material)

<!-- MANUAL ADDITIONS START -->
<!-- MANUAL ADDITIONS END -->