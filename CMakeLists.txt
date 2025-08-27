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
  engine.vec engine.matrix runtime.ecs runtime.app platform.win32 editor.ui
)
target_compile_features(level_editor PUBLIC cxx_std_23)

if (MSVC)
  target_compile_options(level_editor PRIVATE /W4 /permissive- /std:c++latest)
  foreach(tgt IN ITEMS engine.math engine.vec engine.matrix engine.quat engine.color runtime.ecs runtime.app platform.win32 editor.ui)
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
  )
  target_link_libraries(math_tests PRIVATE 
    engine.math
    engine.vec 
    engine.matrix 
    engine.quat 
    engine.color
    engine.random
    Catch2::Catch2WithMain
  )
  target_compile_features(math_tests PUBLIC cxx_std_23)
  if (MSVC)
    target_compile_options(math_tests PRIVATE /W4 /permissive- /std:c++latest)
  endif()

  add_test(NAME math COMMAND math_tests)
endif()

