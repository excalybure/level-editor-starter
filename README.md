# Level Editor Starter

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
