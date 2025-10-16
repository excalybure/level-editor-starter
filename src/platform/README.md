# Platform Layer

The platform layer provides operating system and graphics API abstractions, isolating platform-specific code from the rest of the application.

## Purpose

This layer wraps platform-specific functionality:
- DirectX 12 device management and wrappers
- Win32 window creation and input handling
- PIX performance profiling integration
- Platform-specific utilities

## Key Components

### DirectX 12

#### `dx12/dx12_device.h/cpp`
DirectX 12 device initialization and management.
- **Key class**: `dx12::Device`
- **Responsibilities**: D3D12Device creation, command queues, swapchain, synchronization
- **Features**: Debug layer integration, adapter selection, feature level checking
- **Usage**: Single device instance for entire application

#### `dx12/dx12_context.h/cpp`
Command list recording and submission.
- **Key class**: `dx12::Context`
- **Features**: Command allocator pooling, list reset, submission, fence synchronization
- **Pattern**: One context per thread for parallel recording

#### `dx12/dx12_helpers.h`
DirectX 12 utility functions and wrappers.
- **Helpers**: Resource barriers, descriptor creation, format conversions
- **Macros**: `DX12_CHECK()` for error handling
- **Constants**: Common descriptor sizes, alignment values

#### `dx12/dx12_resource.h/cpp`
Resource creation and management helpers.
- **Functions**: Texture creation, buffer creation, resource transitions
- **Features**: Automatic initial state handling, heap type selection

### Win32

#### `win32/win32_window.h/cpp`
Windows window creation and message handling.
- **Key class**: `win32::Window`
- **Features**: Window creation, message pump, input events, resize handling
- **Integration**: Provides HWND for D3D12 swapchain creation
- **Events**: Mouse, keyboard, resize, close

#### `win32/win32_input.h/cpp`
Win32 input state management.
- **Key class**: `win32::Input`
- **Features**: Keyboard state, mouse state, button transitions
- **Usage**: Query input state for game logic

### PIX

#### `pix/pix.h/cpp`
PIX performance profiling marker integration.
- **Macros**: `PIXBeginEvent()`, `PIXEndEvent()`, `PIXSetMarker()`
- **Features**: GPU/CPU event markers, colored regions, hierarchical captures
- **Usage**: Wrap rendering commands for PIX analysis
- **Note**: No-op in release builds

## Dependencies

**Depends on**: `core`, `math`  
**Used by**: `graphics`, `runtime`, `engine`, `editor`

**External APIs**:
- **DirectX 12**: Graphics API
- **DXGI**: Swapchain and display management
- **Win32 API**: Window and input
- **PIX**: Performance profiling (WinPixEventRuntime)

## Directory Structure

```
platform/
├── dx12/                      # DirectX 12 abstraction
│   ├── dx12_device.h/cpp     # Device management
│   ├── dx12_context.h/cpp    # Command lists
│   ├── dx12_helpers.h        # Utility functions
│   └── dx12_resource.h/cpp   # Resource helpers
├── win32/                    # Windows platform
│   ├── win32_window.h/cpp   # Window management
│   └── win32_input.h/cpp    # Input handling
└── pix/                      # PIX profiling
    └── pix.h/cpp
```

## Usage Examples

### Device Initialization

```cpp
#include "platform/dx12/dx12_device.h"

// Create device
dx12::Device device;
if (!device.initialize(enableDebugLayer)) {
    console::fatal("Failed to initialize D3D12 device");
}

// Get D3D12 objects
auto* d3dDevice = device.getDevice();
auto* commandQueue = device.getCommandQueue();
```

### Window Creation

```cpp
#include "platform/win32/win32_window.h"

// Create window
win32::Window window;
const bool success = window.create(
    "Level Editor",
    1920, 1080,
    &windowProc  // Message callback
);

// Get HWND for swapchain
HWND hwnd = window.getHwnd();
```

### Command List Recording

```cpp
#include "platform/dx12/dx12_context.h"

// Get context
auto& ctx = device.getContext();

// Begin frame
ctx.beginFrame();
auto* cmdList = ctx.getCommandList();

// Record commands
cmdList->SetGraphicsRootSignature(rootSig);
cmdList->DrawInstanced(3, 1, 0, 0);

// Submit
ctx.submitFrame();
ctx.waitForGpu();  // Synchronize if needed
```

### PIX Markers

```cpp
#include "platform/pix/pix.h"

PIXBeginEvent(cmdList, PIX_COLOR_DEFAULT, "Render Scene");
{
    PIXBeginEvent(cmdList, PIX_COLOR_INDEX(1), "Opaque Pass");
    // Render opaque geometry...
    PIXEndEvent(cmdList);
    
    PIXBeginEvent(cmdList, PIX_COLOR_INDEX(2), "Transparent Pass");
    // Render transparent geometry...
    PIXEndEvent(cmdList);
}
PIXEndEvent(cmdList);
```

## Architecture Notes

### Device Singleton Pattern
The D3D12 device is effectively a singleton:
```cpp
// Initialize once at startup
dx12::Device g_device;
g_device.initialize(true);

// Access throughout application
auto* d3dDevice = g_device.getDevice();
```

### Command List Pool
Contexts maintain a pool of command allocators:
- One allocator per frame-in-flight
- Reuse allocators after GPU completes frame
- Thread-local contexts for parallel recording

### Resource Barriers
Use helpers for common barrier patterns:
```cpp
#include "platform/dx12/dx12_helpers.h"

// Transition resource
auto barrier = dx12::transitionBarrier(
    resource,
    D3D12_RESOURCE_STATE_COMMON,
    D3D12_RESOURCE_STATE_RENDER_TARGET
);
cmdList->ResourceBarrier(1, &barrier);
```

### Window Message Handling
Window messages are dispatched through a callback:
```cpp
LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_SIZE:
            // Handle resize...
            break;
        case WM_CLOSE:
            // Handle close...
            break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
```

## Performance Considerations

### GPU Synchronization
Minimize CPU-GPU synchronization:
```cpp
// Good: Only wait when needed
ctx.submitFrame();
// Continue CPU work...

// Bad: Waiting every frame
ctx.submitFrame();
ctx.waitForGpu();  // Stalls CPU
```

### Command Allocator Recycling
Reuse allocators to avoid allocation overhead:
```cpp
// Context automatically manages allocator pool
// No manual allocation needed
ctx.beginFrame();  // Gets allocator from pool
```

### Descriptor Heap Caching
Cache descriptor heaps rather than recreating:
```cpp
// Create once
auto heap = device.createDescriptorHeap(1000, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

// Reuse each frame
cmdList->SetDescriptorHeaps(1, &heap);
```

## Debugging

### Debug Layer
Enable D3D12 debug layer in development:
```cpp
device.initialize(true);  // Enable debug layer
```

Benefits:
- Validation errors in output window
- Object naming for PIX
- Resource leak detection

### PIX Captures
Capture frames with PIX:
1. Run application with PIX attached
2. Use PIX markers to organize captures
3. Analyze GPU timing and resource usage

### Window Debugging
Handle window messages explicitly:
```cpp
case WM_PAINT:
    console::debug("Window paint requested");
    break;
case WM_SIZE:
    console::debug("Window resized: {}x{}", width, height);
    break;
```

## Testing

Platform layer is primarily tested through integration tests:
- Device initialization
- Window creation
- Resource creation
- Command submission

See `tests/dx12_tests.cpp`, `tests/device_tests.cpp`.

## Error Handling

Use `DX12_CHECK()` macro for D3D12 calls:
```cpp
#include "platform/dx12/dx12_helpers.h"

DX12_CHECK(device->CreateCommittedResource(...));
// Macro logs error and location on failure
```

For non-fatal errors:
```cpp
HRESULT hr = device->CreateSomething(...);
if (FAILED(hr)) {
    console::error("Failed to create resource: 0x{:08X}", hr);
    return false;
}
```

## See Also

- `graphics/gpu/` - Uses platform layer for GPU resources
- Microsoft DirectX 12 documentation
- PIX download and documentation
- `/docs/` - Platform-specific implementation notes
