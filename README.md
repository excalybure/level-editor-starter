# 🎯 Project Summary: C++23 world Editor (Windows)

## Goals
- Build a **3D world editor** as a continuous self-learning project.
- Use **modern C++23 features** (concepts, ranges, constexpr).
- Run on **Windows**; build in **VS Code** with MSVC + CMake.
- Use **unit tests** (Catch2) to validate subsystems.
- Learn by integrating graphics + engine concepts:
  - Multiple camera views (Perspective, Top, Front(Y), Side(X)).
  - Scene editing with gizmos (Z up, Y forward, X right).
  - Object placement & picking.
  - Terrain system with **virtual texturing** (D3D12 Tiled Resources).
  - Data-driven material system with HLSL shader management.

---

## Tech Stack
- **Language/Build:** C++23, MSVC, CMake ≥ 3.29, MSBuild.
- **Dependencies:** vcpkg (Catch2, ImGui, DirectX headers, WinPixEventRuntime, cgltf, nlohmann_json).
- **Graphics:** Direct3D 12 (with tiled resources for VT).
- **UI:** Dear ImGui (docking + viewports), ImGuizmo (manipulators).
- **Assets:** glTF 2.0 for mesh import.
- **Testing:** Catch2 unit tests (5,374 assertions across 71+ test cases).
- **Building:** CMake static libraries (core, graphics, engine, runtime, platform, editor).

---

## Project Layout
```
/src
  /math         # Header-only math library (Vec3, Mat4, Quat, bounding volumes, curves)
  /core         # Foundation utilities (console, time, strings) - zero dependencies
  /graphics     # Rendering, GPU resources, shader management
  /engine       # Assets, camera, glTF loader, picking
  /runtime      # ECS, mesh rendering, scene serialization
  /platform     # Win32 window + D3D12 device
  /editor       # UI, multi-viewport, gizmos, commands, panels
  main.cpp
/tests          # Catch2 unit tests
/shaders        # HLSL shaders
```

---

## Key Libraries
- **core** – Foundation utilities (console, time, strings) with zero dependencies.
- **math** – Header-only math library (Vec3, Mat4, Quat, bounding volumes, curves, RH Z-up).
- **platform** – Win32 window, DirectX 12 device & texture management.
- **graphics** – GPU resource manager, shader compiler/manager, renderer, grid rendering.
- **engine** – Asset management, camera controllers, glTF loader, picking.
- **runtime** – ECS with Component concept, mesh rendering system, scene serialization.
- **editor** – ImGui UI, multi-viewport, gizmos, command history, scene hierarchy, entity inspector.

---

## Implementation Roadmap
**Milestone 0 – Scaffolding**
- Build system with C++23 modules working in VS Code.
- Unit test scaffold (Catch2).

**Milestone 1 – Multi-Viewport Cameras**
- Four docked views (Perspective, Top, Front, Side).
- Camera controllers with Z-up RH system.
- Grid + snapping.

**Milestone 2 – Scene & Editing**
- ECS for transforms, components.
- glTF import, picking, gizmo manipulations.
- Undo/redo command stack.

**Milestone 3 – Material System**
- Data-driven material definitions (JSON).
- Shader management with compilation & caching.
- PBR material properties (baseColor, normal, roughness, metallic).
- Runtime shader reloading & hot-reload support.

**Milestone 4 – Terrain**
- Quadtree LOD heightmap.
- Integrate with material system.
- Add **virtual texturing**: tiled resources, page table, residency, feedback pass.

**Milestone 5 – Polish**
- Serialization (scene save/load).
- Camera bookmarks, duplication, delete.
- Performance & usability fixes.

---

## Math Library Status

### ✅ Completed Modules
- **Random Systems** (`engine.random`) - PCG32 with seeding, floats, ints, dice rolls
- **Animation Systems** (`engine.animation`) - Easing functions, keyframes, bezier curves  
- **Advanced Math** (`engine.math`) - Interpolation, trigonometry, constants, utilities
- **2D Geometry** (`engine.math_2d`) - Point-in-shape tests, line intersections, distance calculations
- **3D Geometry** (`engine.math_3d`) - Point-in-shape tests, ray intersections, distance calculations, geometric utilities
- **Bounding Volumes** - Modular design with individual modules:
  - `engine.bounding_box_2d` - 2D axis-aligned bounding boxes
  - `engine.bounding_box_3d` - 3D axis-aligned bounding boxes  
  - `engine.bounding_sphere` - 3D bounding spheres with expansion
  - `engine.oriented_bounding_box` - 3D oriented bounding boxes (SAT collision)
  - `engine.plane` - 3D planes with distance calculations
  - `engine.frustum` - 3D view frustums for culling
  - `engine.bounding_volumes` - Convenience module importing all bounding structures
- **Curve & Spline Systems** (`engine.curves`) - Bezier curves, Catmull-Rom splines, arc length parameterization

### 📊 Test Coverage
- **5,374 assertions** across **71 test cases** - all passing ✅
- Comprehensive validation of all mathematical operations
- Edge case handling for degenerate geometries
- Performance verification for critical functions

### 🏗️ Architecture Highlights
- **Modular Design**: Each bounding structure in its own module for maximum flexibility
- **Import Convenience**: `engine.bounding_volumes` provides single import for all structures
- **Template-based**: Type-generic implementations supporting float, double precision  
- **C++23 Modern**: Uses modules, concepts, constexpr extensively
- **Free Functions**: Vector operations use free functions (dot, cross, length) for consistency

---

## Testing Strategy
- **Math:** Vec3/Mat4/Quat operations, bounding volumes, curves, 2D/3D geometry.
- **ECS:** Entity creation/destruction, component storage, systems.
- **Graphics:** GPU buffers, shader compilation, material management.
- **Scene:** glTF loading, picking, serialization, asset resolution.
- **Editor:** Commands (undo/redo), gizmos, viewport input, selection.
- **Integration:** Multi-viewport rendering, scene import, object creation workflows.

---

# World Editor

C++23 3D world editor scaffold (Windows / MSVC / CMake). Provides:

- Static library architecture (core, graphics, engine, runtime, platform, editor)
- Header-only math library with comprehensive 2D/3D geometry
- Catch2 unit tests (5,374+ assertions, all passing)
- vcpkg manifest (imgui, directx-headers, winpixevent, cgltf, nlohmann_json)

## Prerequisites

- Visual Studio 2022 (17.8+) with MSVC toolset and C++20/23 modules support
- CMake >= 3.29
- vcpkg (environment variable `VCPKG_ROOT` pointing to clone)

## Configure & Build

From the repo root (PowerShell):

```powershell
$env:VCPKG_ROOT = "C:/path/to/vcpkg"  # if not already set
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --target level_editor
```

Run the executable:
```powershell
./build/Debug/level_editor.exe
```

## Run Tests

```powershell
cmake --build build --config Debug --target unit_test_runner
.\build\vs2022-x64\Debug\unit_test_runner.exe
```

## Next Steps (Roadmap)

Milestone 1: multi-viewport cameras, grid & snapping.

Add dependencies when needed by editing `vcpkg.json` and re-configuring.

## License

MIT (add LICENSE file as needed).
