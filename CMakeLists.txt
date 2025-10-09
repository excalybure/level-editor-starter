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
find_package(winpixevent CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_path(CGLTF_INCLUDE_DIRS "cgltf.h")

# Libraries

# Core library - foundation utilities with zero dependencies
add_library(core STATIC
  src/core/console.cpp
  src/core/time.cpp
  src/core/strings.cpp
)

target_include_directories(core PUBLIC 
  src
)
target_compile_features(core PUBLIC cxx_std_23)
target_compile_definitions(core PUBLIC NOMINMAX)
if (MSVC)
  target_compile_options(core PRIVATE /W4 /permissive- /std:c++latest)
endif()

# ImGuizmo library (local)
add_library(ImGuizmo STATIC
  externals/ImGuizmo/ImGuizmo.cpp
  externals/ImGuizmo/ImCurveEdit.cpp
  externals/ImGuizmo/ImGradient.cpp
  externals/ImGuizmo/ImSequencer.cpp
  externals/ImGuizmo/GraphEditor.cpp
)

target_include_directories(ImGuizmo PUBLIC 
  externals/ImGuizmo
)
target_compile_features(ImGuizmo PUBLIC cxx_std_23)
target_compile_definitions(ImGuizmo PUBLIC NOMINMAX)
target_link_libraries(ImGuizmo PUBLIC 
  imgui::imgui
)

if (MSVC)
  target_compile_options(ImGuizmo PRIVATE /W4 /permissive- /std:c++latest)
endif()

# Graphics library - rendering, GPU resources, shader management
add_library(graphics STATIC
  src/graphics/gpu/gpu_resource_manager.cpp
  src/graphics/gpu/material_gpu.cpp
  src/graphics/gpu/mesh_gpu.cpp
  src/graphics/grid/grid.cpp
  src/graphics/renderer/renderer.cpp
  src/graphics/shader_manager/shader_compiler.cpp
  src/graphics/shader_manager/shader_manager.cpp
)

target_include_directories(graphics PUBLIC 
  src
)
target_link_libraries(graphics PUBLIC
  core
  platform
)
target_compile_features(graphics PUBLIC cxx_std_23)
target_compile_definitions(graphics PUBLIC NOMINMAX)
if (MSVC)
  target_compile_options(graphics PRIVATE /W4 /permissive- /std:c++latest)
endif()

# Engine library
add_library(engine STATIC
  src/engine/assets/asset_manager.cpp
  src/engine/assets/assets.cpp
  src/engine/camera/camera.cpp
  src/engine/camera/camera_controller.cpp
  src/engine/gltf_loader/gltf_loader.cpp
  src/engine/picking.cpp
)

target_include_directories(engine PUBLIC 
  src
  ${CGLTF_INCLUDE_DIRS}
)
target_link_libraries(engine PUBLIC
  core
)
target_compile_features(engine PUBLIC cxx_std_23)
target_compile_definitions(engine PUBLIC NOMINMAX)
if (MSVC)
  target_compile_options(engine PRIVATE /W4 /permissive- /std:c++latest)
endif()


# Runtime library - contains core runtime systems
add_library(runtime STATIC
  src/runtime/mesh_rendering_system.cpp
  src/runtime/scene_importer.cpp
  src/runtime/scene_serialization/SceneSerializer.cpp
)

target_include_directories(runtime PUBLIC 
  src
)
target_compile_features(runtime PUBLIC cxx_std_23)
target_compile_definitions(runtime PUBLIC NOMINMAX)
target_link_libraries(runtime PUBLIC
  core
  graphics
  nlohmann_json::nlohmann_json
)
if (MSVC)
  target_compile_options(runtime PRIVATE /W4 /permissive- /std:c++latest)
endif()


# Platform library - contains core platform systems
add_library(platform STATIC
  src/platform/dx12/dx12_device.cpp
  src/platform/dx12/dx12_texture.cpp
  src/platform/win32/win32_window.cpp
)

target_include_directories(platform PUBLIC 
  src
  vcpkg_installed/x64-windows/include
)
target_compile_features(platform PUBLIC cxx_std_23)
target_compile_definitions(platform PUBLIC NOMINMAX PIX_AVAILABLE)
target_link_libraries(platform PUBLIC 
  core
  Microsoft::DirectX-Headers
  Microsoft::WinPixEventRuntime
  d3d12
  dxgi
  d3dcompiler
)

if (MSVC)
  target_compile_options(platform PRIVATE /W4 /permissive- /std:c++latest)
endif()

add_library(editor STATIC
  src/editor/asset_browser/AssetBrowserPanel.cpp
  src/editor/commands/CommandHistory.cpp
  src/editor/commands/CommandProfiler.cpp
  src/editor/commands/CommandUI.cpp
  src/editor/commands/EcsCommands.cpp
  src/editor/config/EditorConfig.cpp
  src/editor/entity_inspector/ComponentUI.cpp
  src/editor/entity_inspector/EntityInspectorPanel.cpp
  src/editor/gizmos.cpp
  src/editor/scene_hierarchy/SceneHierarchyPanel.cpp
  src/editor/selection.cpp
  src/editor/selection_renderer.cpp
  src/editor/transform_commands.cpp
  src/editor/ui.cpp
  src/editor/viewport_input.cpp
  src/editor/viewport/viewport.cpp
)

target_include_directories(editor PUBLIC 
  src
  vcpkg_installed/x64-windows/include
)
target_compile_features(editor PUBLIC cxx_std_23)
target_compile_definitions(editor PUBLIC NOMINMAX PIX_AVAILABLE)
target_link_libraries(editor PUBLIC 
  imgui::imgui
  ImGuizmo
  graphics
  Microsoft::DirectX-Headers
  Microsoft::WinPixEventRuntime
  d3d12
  dxgi
  d3dcompiler
  core
  runtime
  platform
  nlohmann_json::nlohmann_json
)

if (MSVC)
  target_compile_options(editor PRIVATE /W4 /permissive- /std:c++latest)
endif()

# Executable
add_executable(level_editor src/main.cpp)
target_link_libraries(level_editor PRIVATE
  editor
  engine
  graphics
  platform
  runtime
  imgui::imgui
)
target_compile_features(level_editor PUBLIC cxx_std_23)

if (MSVC)
  target_compile_options(level_editor PRIVATE /W4 /permissive- /std:c++latest)
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
  add_executable(unit_test_runner 
    tests/string_util_tests.cpp
    tests/vec_tests.cpp
    tests/matrix_tests.cpp
    tests/quat_tests.cpp
    tests/math_utils_tests.cpp
    tests/random_tests.cpp
    tests/animation_tests.cpp
    tests/curves_tests.cpp
    tests/math_2d_tests.cpp
    tests/math_3d_tests.cpp
    tests/picking_tests.cpp
    tests/picking_selection_integration_tests.cpp
    tests/dx12_tests.cpp
    tests/renderer_tests.cpp
    tests/camera_tests.cpp
    tests/camera_controller_tests.cpp
    tests/viewport_tests.cpp
    tests/viewport_input_tests.cpp
    tests/viewport_input_handler_tests.cpp
    tests/viewport_input_integration_tests.cpp
    tests/grid_tests.cpp
    tests/ecs_tests.cpp
    tests/systems_tests.cpp
    tests/components_tests.cpp
    tests/reparenting_transform_tests.cpp
    tests/selection_tests.cpp
    tests/selection_renderer_tests.cpp
    tests/app_tests.cpp
    tests/renderer_integration_tests.cpp
    tests/integration_tests.cpp
    tests/ui_tests.cpp
    tests/ui_scene_operations_tests.cpp
    tests/win32_window_tests.cpp
    tests/console_tests.cpp
    tests/shader_manager_tests.cpp
    tests/shader_manager_performance_test.cpp
    tests/shader_include_dependency_tests.cpp
    tests/assets_tests.cpp
    tests/asset_manager_tests.cpp
    tests/ecs_import_tests.cpp
    tests/scene_importer_tests.cpp
    tests/scene_importer_gpu_integration_tests.cpp
    tests/gltf_loader_tests.cpp
    tests/gltf_node_naming_tests.cpp
    tests/gltf_accessor_tests.cpp
    tests/mesh_extraction_tdd_test.cpp
    tests/primitive_tests.cpp
    tests/gpu_buffer_tests.cpp
    tests/material_gpu_tests.cpp
    tests/gpu_resource_manager_tests.cpp
    tests/mesh_rendering_system_tests.cpp
    tests/asset_rendering_integration_tests.cpp
    tests/imguizmo_integration_tests.cpp
    tests/gizmos_tests.cpp
    tests/gizmo_integration_tests.cpp
    tests/gizmo_selection_integration_test.cpp
    tests/gizmo_operation_switching_tests.cpp
    tests/gizmo_keyboard_blocking_tests.cpp
    tests/transform_commands_tests.cpp
    tests/editor_command_tests.cpp
    tests/ecs_command_tests.cpp
    tests/command_ui_tests.cpp
    tests/command_performance_tests.cpp
    tests/command_integration_tests.cpp
    tests/command_history_callback_tests.cpp
    tests/entity_fixup_test.cpp
    tests/duplicate_entity_tests.cpp
    tests/gizmo_command_integration_tests.cpp
    tests/scene_hierarchy_tests.cpp
    tests/entity_inspector_tests.cpp
    tests/scene_serialization_tests.cpp
    tests/scene_serialization_meshpath_tests.cpp
    tests/asset_resolution_tests.cpp
    tests/visibility_integration_tests.cpp
    tests/scene_editor_integration_tests.cpp
    tests/object_creation_integration_tests.cpp
    tests/editor_config_tests.cpp
  )
  target_link_libraries(unit_test_runner PRIVATE 
    editor
    engine
    graphics
    platform
    runtime
    imgui::imgui
    Catch2::Catch2WithMain
  )
  target_compile_features(unit_test_runner PUBLIC cxx_std_23)
  if (MSVC)
    target_compile_options(unit_test_runner PRIVATE /W4 /permissive- /std:c++latest)
  endif()

  add_test(NAME unit_tests COMMAND unit_test_runner)
endif()

