# Core Library

The `core` library is a foundational utility library with **zero dependencies** (except the C++ standard library and basic OS APIs). It provides essential utilities that are used throughout the project.

## Philosophy

- **No external dependencies**: Only uses C++23 standard library and OS APIs
- **Platform agnostic**: Works on Windows, Linux, and macOS (where applicable)
- **Foundation layer**: All other libraries (engine, runtime, editor, platform) can depend on core

## Components

### 🖨️ Console (`core/console.h`)
Colored console output with multiple severity levels:
- `console::fatal()` - Fatal errors (exits program)
- `console::error()` - Non-fatal errors
- `console::errorAndThrow()` - Errors that throw exceptions
- `console::warning()` - Warnings
- `console::info()` - Informational messages
- `console::debug()` - Debug messages

Supports `std::format` syntax:
```cpp
#include "core/console.h"

console::info("Loading file: {}", filename);
console::error("Failed to load {} with error code {}", filename, errorCode);
```

### 🕐 Time (`core/time.h`)
High-resolution timing utilities:
```cpp
#include "core/time.h"

float elapsed = core::time::getCurrentTime(); // Seconds since app start
```

### 🔤 Strings (`core/strings.h`)
String manipulation utilities:
```cpp
#include "core/strings.h"

std::string base = core::strings::getBaseFilename("assets/models/cube.gltf"); // Returns "cube"
```

## Usage

Add `core` to your target's dependencies in `CMakeLists.txt`:
```cmake
target_link_libraries(your_target PUBLIC core)
```

Then include the headers you need:
```cpp
#include "core/console.h"
#include "core/time.h"
#include "core/strings.h"
```

## Backward Compatibility

For convenience, namespace aliases are provided:
- `namespace console = core::console`
- `namespace strings = core::strings`
- `namespace runtime::time = time`

This allows existing code to continue using `console::info()` instead of `core::console::info()`.

## Design Goals

1. **Self-contained**: Can be extracted and used in other projects
2. **Modern C++**: Uses C++23 features (concepts, ranges, `std::format`, etc.)
3. **Const-correct**: All functions properly mark const-correctness
4. **Well-tested**: Comprehensive unit tests for all functionality
5. **Cross-platform**: Works on Windows, Linux, and macOS
