cmake_minimum_required(VERSION 3.29)
project(LevelEditorStarter LANGUAGES CXX)

# Standard & modules
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Help CMake/Ninja with MSVC module dependency scanning
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Libraries (modules)
add_library(engine.math)
target_sources(engine.math PUBLIC
  FILE_SET CXX_MODULES FILES
    src/engine/math.ixx
)
target_compile_features(engine.math PUBLIC cxx_std_23)

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
)
target_compile_features(platform.win32 PUBLIC cxx_std_23)

add_library(editor.ui)
target_sources(editor.ui PUBLIC
  FILE_SET CXX_MODULES FILES
    src/editor/ui.ixx
)
target_compile_features(editor.ui PUBLIC cxx_std_23)

# Executable
add_executable(level_editor src/main.cpp)
target_link_libraries(level_editor PRIVATE
  engine.math runtime.ecs runtime.app platform.win32 editor.ui
)
target_compile_features(level_editor PUBLIC cxx_std_23)

if (MSVC)
  target_compile_options(level_editor PRIVATE /W4 /permissive- /std:c++latest)
  foreach(tgt IN ITEMS engine.math runtime.ecs runtime.app platform.win32 editor.ui)
    target_compile_options(${tgt} PRIVATE /W4 /permissive- /std:c++latest)
  endforeach()
endif()

# -----------------------------------------------------------------------------
# Testing (Catch2 via vcpkg manifest)
# -----------------------------------------------------------------------------
include(CTest)
if(BUILD_TESTING)
  find_package(Catch2 3 REQUIRED)
  add_executable(math_tests tests/math_tests.cpp)
  target_link_libraries(math_tests PRIVATE engine.math Catch2::Catch2WithMain)
  target_compile_features(math_tests PUBLIC cxx_std_23)
  if (MSVC)
    target_compile_options(math_tests PRIVATE /W4 /permissive- /std:c++latest)
  endif()
  add_test(NAME math COMMAND math_tests)
endif()

