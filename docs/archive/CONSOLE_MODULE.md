# Console Module Documentation

## Overview
The console module provides colored console logging functionality with different severity levels.

## Files
- `src/runtime/console.ixx` - Module interface 
- `src/runtime/console.cpp` - Module implementation
- `tests/console_tests.cpp` - Unit tests

## Functions

### `console::fatal(message)`
- **Color**: Red
- **Action**: Prints message and exits the application with code 1
- **Format**: `[FATAL] <message>`
- **Use case**: Unrecoverable errors

### `console::error(message)`  
- **Color**: Red
- **Action**: Prints message and continues execution
- **Format**: `[ERROR] <message>`
- **Use case**: Recoverable errors

### `console::warning(message)`
- **Color**: Yellow  
- **Action**: Prints message and continues execution
- **Format**: `[WARNING] <message>`
- **Use case**: Potential issues or deprecated functionality

### `console::info(message)`
- **Color**: Gray
- **Action**: Prints message and continues execution  
- **Format**: `[INFO] <message>`
- **Use case**: General information messages

### `console::debug(message)`
- **Color**: Blue
- **Action**: Prints message and continues execution
- **Format**: `[DEBUG] <message>`
- **Use case**: Development and debugging information

## Parameter Types
- `const std::string&` - Standard string objects
- `const char*` - C-style string literals

## Platform Support
- **Windows**: Uses Windows Console API for colors
- **Other platforms**: Uses ANSI escape codes

## Usage Examples

```cpp
import runtime.console;

int main() {
    console::info("Application starting");
    console::debug("Debug information");
    console::warning("This is deprecated");
    console::error("Non-fatal error occurred");
    
    // Only use fatal for unrecoverable errors
    if (criticalError) {
        console::fatal("Cannot continue execution");
        // This line will never execute
    }
    
    return 0;
}
```

## Integration
The console module is integrated into:
- Main application (`level_editor`)  
- Unit test runner
- Available for import in any module via `import runtime.console;`
