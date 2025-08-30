# 🎯 Project Summary: C++23 world Editor (Windows)

## Goals
- Build a **3D world editor** as a continuous self-learning project.
- Use **modern C++23 features** (modules, concepts, ranges).
- Run on **Windows**; build in **VS Code** with MSVC + CMake.
- Use **unit tests** (Catch2) to validate subsystems.
- Learn by integrating graphics + engine concepts:
  - Multiple camera views (Perspective, Top, Front(Y), Side(X)).
  - Scene editing with gizmos (Z up, Y forward, X right).
  - Object placement & picking.
  - Terrain system with **virtual texturing** (D3D12 Tiled Resources).
  - Material system like Unreal (node graph → HLSL codegen).

---

## Tech Stack
- **Language/Build:** C++23, MSVC, CMake ≥ 3.29, Ninja or MSBuild.
- **Dependencies:** vcpkg (Catch2, ImGui, GLFW, GLM, DirectX headers + DXC, cgltf).
- **Graphics:** Direct3D 12 (with tiled resources for VT).
- **UI:** Dear ImGui (docking + viewports), ImGuizmo (manipulators).
- **Assets:** glTF 2.0 for mesh import.
- **Testing:** Catch2 unit tests + GitHub Actions CI (later).
- **Building:** You will use cmake to build the code base

---

## Naming Conventions

This project follows **Modern C++ conventions** (avoiding snake_case) for consistency and readability:

### 🏷️ Types & Classes: **PascalCase**
```cpp
class ViewportLayout {};
struct Vec3 {};
enum class MouseButton { Left, Right, Middle };
template<typename T> class BoundingBox {};
```

### 🔧 Functions & Methods: **camelCase**
```cpp
void beginFrame();           // Lifecycle functions
void endFrame();
bool initialize();
void shutdown();
ID3D12Device* getDevice();   // Getters/accessors
float lengthSquared();       // Math operations
void setupDockspace();       // Internal functions
```

### 📦 Variables: **camelCase**
```cpp
float clearColor = 0.0f;
int windowWidth = 800;
auto lengthSquared = dot(v, v);
Vec3 cameraPosition{0, 0, 10};
```

### 👥 Member Variables: **m_** prefix + **camelCase**
```cpp
class UI {
    ViewportLayout m_layout;
    std::unique_ptr<Impl> m_impl;
    ComPtr<ID3D12Device> m_device;
};
```

### 🔢 Constants: **kPascalCase** (modern style)
```cpp
static constexpr float kPi = 3.14159f;
static constexpr int kMaxEntities = 1000;
static constexpr Vec3 kWorldUp{0, 0, 1};  // Z-up convention
```

### 🗂️ Namespaces: **lowercase**
```cpp
namespace math {}    // Core math operations
namespace ecs {}     // Entity-component system  
namespace dx12 {}    // DirectX 12 abstractions
namespace editor {}  // Editor-specific code
```

### 📁 Files & Modules: **lowercase + underscores**
```cpp
// Module names
export module engine.math;
export module runtime.ecs;
export module platform.win32.window;

// File names
math_tests.cpp
win32_window.ixx
dx12_device.cpp
```

**Rationale:** This convention aligns with Unreal Engine, modern DirectX APIs, and contemporary C++ codebases while avoiding snake_case as requested.

---

## Project Layout (modules)
```
/src
  /engine       # math, renderer, GPU resources
  /runtime      # ECS, app loop, scene, serialization
  /platform     # Win32 window + D3D12 device
  /editor       # UI, multi-viewport, gizmos, commands
  main.cpp
/tests          # Catch2 unit tests
/shaders        # HLSL + material codegen output
```

---

## Key Modules
- **engine.math.ixx** – Vec3, Mat4, dot, cross (RH Z-up).
- **runtime.ecs.ixx** – minimal ECS with `Component` concept + storage.
- **runtime.app.ixx** – main loop (`tick()`).
- **platform.win32.win32_window.ixx** – window creation + pump (global module fragment for `windows.h`).
- **editor.ui.ixx** – ImGui docking + panels (stubbed).

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
- Node graph UI (ImGui).
- IR DAG → HLSL codegen (baseColor, normal, roughness, metallic).
- DXC compile + caching.
- Unit tests for graph validation.

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
- **Math:** dot, cross, transforms.
- **ECS:** create/destroy/get.
- **Scene:** pick ray vs AABB.
- **Material graph:** DAG validation, HLSL snippets.
- **Terrain/VT:** page mapping, residency tests.

---

# World Editor

C++23 modular scaffold for a future 3D world editor (Windows / MSVC / CMake). Provides:

- C++23 modules (engine.math, runtime.ecs, runtime.app, platform.win32.win32_window, editor.ui)
- Catch2 unit test example for math
- vcpkg manifest (currently only Catch2; extend later with imgui, imguizmo, glfw3, glm, directx-headers, directx-dxc, cgltf)

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
cmake --build build --config Debug --target math_tests
ctest --test-dir build --output-on-failure -C Debug
```

## Next Steps (Roadmap)

Milestone 1: multi-viewport cameras, grid & snapping.

Add dependencies when needed by editing `vcpkg.json` and re-configuring.

## License

MIT (add LICENSE file as needed).
