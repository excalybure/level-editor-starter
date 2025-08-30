cmake_minimum_required(VERSION 3.29)
project(LevelEditorStarter LANGUAGES CXX)

# Standard & modules
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Help CMake/Ninja with MSVC module dependency scanning
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Find vcpkg packages
find_package(imgui CONFIG REQUIRED)
find_package(directx-headers CONFIG REQUIRED)

# Libraries (modules)
add_library(engine.math)
target_sources(engine.math PUBLIC
  FILE_SET CXX_MODULES FILES
    src/engine/math/math.ixx
)
target_compile_features(engine.math PUBLIC cxx_std_23)

add_library(engine.vec)
target_sources(engine.vec PUBLIC
  FILE_SET CXX_MODULES FILES
    src/engine/math/vec.ixx
)
target_compile_features(engine.vec PUBLIC cxx_std_23)
target_link_libraries(engine.vec PUBLIC engine.math)

add_library(engine.matrix)
target_sources(engine.matrix PUBLIC
  FILE_SET CXX_MODULES FILES
    src/engine/math/matrix.ixx
)
target_compile_features(engine.matrix PUBLIC cxx_std_23)
target_link_libraries(engine.matrix PUBLIC engine.vec engine.math)

add_library(engine.quat)
target_sources(engine.quat PUBLIC
  FILE_SET CXX_MODULES FILES
    src/engine/math/quat.ixx
)
target_compile_features(engine.quat PUBLIC cxx_std_23)
target_link_libraries(engine.quat PUBLIC engine.vec engine.math)

add_library(engine.color)
target_sources(engine.color PUBLIC
  FILE_SET CXX_MODULES FILES
    src/engine/math/color.ixx
)
target_compile_features(engine.color PUBLIC cxx_std_23)
target_link_libraries(engine.color PUBLIC engine.math)

add_library(engine.random)
target_sources(engine.random PUBLIC
  FILE_SET CXX_MODULES FILES
    src/engine/math/random.ixx
)
target_compile_features(engine.random PUBLIC cxx_std_23)
target_link_libraries(engine.random PUBLIC engine.math engine.vec)

add_library(engine.animation)
target_sources(engine.animation PUBLIC
  FILE_SET CXX_MODULES FILES
    src/engine/math/animation.ixx
)
target_compile_features(engine.animation PUBLIC cxx_std_23)
target_link_libraries(engine.animation PUBLIC engine.math)

add_library(engine.curves)
target_sources(engine.curves PUBLIC
  FILE_SET CXX_MODULES FILES
    src/engine/math/curves.ixx
)
target_compile_features(engine.curves PUBLIC cxx_std_23)
target_link_libraries(engine.curves PUBLIC engine.math engine.vec)

add_library(engine.bounding_box_2d)
target_sources(engine.bounding_box_2d PUBLIC
  FILE_SET CXX_MODULES FILES
    src/engine/math/bounding_box_2d.ixx
)
target_compile_features(engine.bounding_box_2d PUBLIC cxx_std_23)
target_link_libraries(engine.bounding_box_2d PUBLIC engine.vec)

add_library(engine.bounding_box_3d)
target_sources(engine.bounding_box_3d PUBLIC
  FILE_SET CXX_MODULES FILES
    src/engine/math/bounding_box_3d.ixx
)
target_compile_features(engine.bounding_box_3d PUBLIC cxx_std_23)
target_link_libraries(engine.bounding_box_3d PUBLIC engine.vec)

add_library(engine.bounding_sphere)
target_sources(engine.bounding_sphere PUBLIC
  FILE_SET CXX_MODULES FILES
    src/engine/math/bounding_sphere.ixx
)
target_compile_features(engine.bounding_sphere PUBLIC cxx_std_23)
target_link_libraries(engine.bounding_sphere PUBLIC engine.vec engine.math)

add_library(engine.oriented_bounding_box)
target_sources(engine.oriented_bounding_box PUBLIC
  FILE_SET CXX_MODULES FILES
    src/engine/math/oriented_bounding_box.ixx
)
target_compile_features(engine.oriented_bounding_box PUBLIC cxx_std_23)
target_link_libraries(engine.oriented_bounding_box PUBLIC engine.vec)

add_library(engine.plane)
target_sources(engine.plane PUBLIC
  FILE_SET CXX_MODULES FILES
    src/engine/math/plane.ixx
)
target_compile_features(engine.plane PUBLIC cxx_std_23)
target_link_libraries(engine.plane PUBLIC engine.vec)

add_library(engine.frustum)
target_sources(engine.frustum PUBLIC
  FILE_SET CXX_MODULES FILES
    src/engine/math/frustum.ixx
)
target_compile_features(engine.frustum PUBLIC cxx_std_23)
target_link_libraries(engine.frustum PUBLIC engine.vec engine.plane engine.bounding_box_3d engine.bounding_sphere)

add_library(engine.bounding_volumes)
target_sources(engine.bounding_volumes PUBLIC
  FILE_SET CXX_MODULES FILES
    src/engine/math/bounding_volumes.ixx
)
target_compile_features(engine.bounding_volumes PUBLIC cxx_std_23)
target_link_libraries(engine.bounding_volumes PUBLIC 
  engine.bounding_box_2d 
  engine.bounding_box_3d 
  engine.bounding_sphere 
  engine.oriented_bounding_box 
  engine.plane 
  engine.frustum)

add_library(engine.math_2d)
target_sources(engine.math_2d PUBLIC
  FILE_SET CXX_MODULES FILES
    src/engine/math/math_2d.ixx
)
target_compile_features(engine.math_2d PUBLIC cxx_std_23)
target_link_libraries(engine.math_2d PUBLIC engine.math engine.vec engine.bounding_box_2d)

add_library(engine.math_3d)
target_sources(engine.math_3d PUBLIC
  FILE_SET CXX_MODULES FILES
    src/engine/math/math_3d.ixx
)
target_compile_features(engine.math_3d PUBLIC cxx_std_23)
target_link_libraries(engine.math_3d PUBLIC engine.math engine.vec engine.bounding_volumes)

add_library(runtime.ecs)
target_sources(runtime.ecs PUBLIC
  FILE_SET CXX_MODULES FILES
    src/runtime/ecs.ixx
)
target_compile_features(runtime.ecs PUBLIC cxx_std_23)

add_library(runtime.app)
target_sources(runtime.app PUBLIC
  FILE_SET CXX_MODULES FILES
    src/runtime/app.ixx
)
target_compile_features(runtime.app PUBLIC cxx_std_23)

add_library(platform.win32)
target_sources(platform.win32 PUBLIC
  FILE_SET CXX_MODULES FILES
    src/platform/win32/win32_window.ixx
  PRIVATE
    src/platform/win32/win32_window.cpp
)
target_compile_features(platform.win32 PUBLIC cxx_std_23)

add_library(editor.ui)
target_sources(editor.ui PUBLIC
  FILE_SET CXX_MODULES FILES
    src/editor/ui.ixx
  PRIVATE
    src/editor/ui.cpp
)
target_compile_features(editor.ui PUBLIC cxx_std_23)
target_link_libraries(editor.ui PRIVATE 
  imgui::imgui
  Microsoft::DirectX-Headers
  d3d12
)
target_link_libraries(editor.ui PUBLIC 
  editor.viewport
)

add_library(platform.dx12)
target_sources(platform.dx12 PUBLIC
  FILE_SET CXX_MODULES FILES
    src/platform/dx12/dx12_device.ixx
  PRIVATE
    src/platform/dx12/dx12_device.cpp
)
target_compile_features(platform.dx12 PUBLIC cxx_std_23)
# Link DirectX 12 libraries
target_link_libraries(platform.dx12 PUBLIC d3d12 dxgi d3dcompiler)

add_library(engine.renderer)
target_sources(engine.renderer PUBLIC
  FILE_SET CXX_MODULES FILES
    src/engine/renderer/renderer.ixx
  PRIVATE
    src/engine/renderer/renderer.cpp
)
target_compile_features(engine.renderer PUBLIC cxx_std_23)
target_link_libraries(engine.renderer PUBLIC platform.dx12 engine.vec engine.matrix engine.color d3d12 dxgi d3dcompiler)

add_library(engine.camera)
target_sources(engine.camera PUBLIC
  FILE_SET CXX_MODULES FILES
    src/engine/camera/camera.ixx
    src/engine/camera/camera_controller.ixx
  PRIVATE
    src/engine/camera/camera.cpp
    src/engine/camera/camera_controller.cpp
)
target_compile_features(engine.camera PUBLIC cxx_std_23)
target_link_libraries(engine.camera PUBLIC engine.vec engine.matrix engine.math)

add_library(editor.viewport)
target_sources(editor.viewport PUBLIC
  FILE_SET CXX_MODULES FILES
    src/editor/viewport/viewport.ixx
  PRIVATE
    src/editor/viewport/viewport.cpp
)
target_compile_features(editor.viewport PUBLIC cxx_std_23)
target_link_libraries(editor.viewport PUBLIC engine.vec engine.matrix engine.camera)

# Executable
add_executable(level_editor src/main.cpp)
target_link_libraries(level_editor PRIVATE
  engine.vec engine.matrix runtime.ecs runtime.app platform.win32 platform.dx12 editor.ui
)
target_compile_features(level_editor PUBLIC cxx_std_23)

if (MSVC)
  target_compile_options(level_editor PRIVATE /W4 /permissive- /std:c++latest)
  foreach(tgt IN ITEMS 
    engine.math engine.vec engine.matrix engine.quat engine.color engine.random engine.animation 
    engine.bounding_box_2d engine.bounding_box_3d engine.bounding_sphere engine.oriented_bounding_box engine.plane engine.frustum engine.bounding_volumes
    engine.math_2d engine.math_3d engine.renderer engine.camera
    runtime.ecs runtime.app platform.win32 platform.dx12 editor.ui editor.viewport)
    target_compile_options(${tgt} PRIVATE /W4 /permissive- /std:c++latest)
  endforeach()
endif()

# -----------------------------------------------------------------------------
# Testing (Catch2 via vcpkg manifest)
# -----------------------------------------------------------------------------
include(CTest)

# Allow user to force-disable tests: -DENABLE_TESTS=OFF
option(ENABLE_TESTS "Build unit tests" ON)
if(NOT BUILD_TESTING)
  set(ENABLE_TESTS OFF)
endif()

if(ENABLE_TESTS)
  # Try to locate Catch2 via vcpkg (toolchain) first; fall back gracefully.
  find_package(Catch2 3 QUIET CONFIG)
  if(NOT Catch2_FOUND)
    message(WARNING "Catch2 v3 not found via toolchain. Tests will be skipped. To enable tests ensure vcpkg toolchain is used or install Catch2 and re-configure.")
    set(ENABLE_TESTS OFF)
  endif()
endif()

if(ENABLE_TESTS AND Catch2_FOUND)
  add_executable(math_tests 
    tests/vec_tests.cpp
    tests/matrix_tests.cpp
    tests/quat_tests.cpp
    tests/math_utils_tests.cpp
    tests/random_tests.cpp
    tests/animation_tests.cpp
    tests/curves_tests.cpp
    tests/math_2d_tests.cpp
    tests/math_3d_tests.cpp
    tests/dx12_tests.cpp
    tests/renderer_tests.cpp
    tests/camera_tests.cpp
    tests/camera_controller_tests.cpp
    tests/viewport_tests.cpp
  tests/ui_tests.cpp
  )
  target_link_libraries(math_tests PRIVATE 
    engine.math
    engine.vec 
    engine.matrix 
    engine.quat 
    engine.color
    engine.random
    engine.animation
    engine.curves
    engine.bounding_volumes
    engine.bounding_box_2d
    engine.math_2d
    engine.math_3d
    platform.dx12
    engine.renderer
    engine.camera
    editor.viewport
  editor.ui
  platform.win32
    Catch2::Catch2WithMain
  )
  target_compile_features(math_tests PUBLIC cxx_std_23)
  if (MSVC)
    target_compile_options(math_tests PRIVATE /W4 /permissive- /std:c++latest)
  endif()

  add_test(NAME math COMMAND math_tests)
endif()

