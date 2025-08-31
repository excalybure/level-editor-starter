# 📋 Milestone 1: Multi-Viewport Cameras - Implementation Plan

## 🎯 Core Objectives
Implement a multi-viewport camera system with four docked views (Perspective, Top, Front, Side) using ImGui docking, camera controllers with Z-up right-handed coordinate system, and grid rendering with snapping functionality.

## 🏗️ Architecture Overview

### **Camera System Design**
- **Coordinate System**: Right-handed, Z-up (Z=up, Y=forward, X=right)
- **Camera Types**: 
  - Perspective (free-look with orbit/pan/zoom)
  - Orthographic views (Top, Front, Side) with 2D navigation
- **View Management**: Each viewport maintains independent camera state
- **Synchronization**: Optional view linking for coordinated navigation

### **UI Layout Structure**
```
┌─────────────────────────────────────┐
│              Menu Bar               │
├─────────────┬───────────────────────┤
│             │                       │
│ Perspective │        Top            │
│   View      │      (XY plane)       │
│             │                       │
├─────────────┼───────────────────────┤
│             │                       │
│   Front     │       Side            │
│ (XZ plane)  │    (YZ plane)         │
│             │                       │
└─────────────┴───────────────────────┘
```

## 📦 Implementation Phases

### **Phase 1: Graphics Foundation** 
**Estimated Time**: 2-3 days

#### **1.1 Graphics Dependencies Setup**
- Update `vcpkg.json` with graphics dependencies:
  ```json
  {
    "dependencies": [
      "catch2",
      "imgui[docking-experimental,win32-binding,dx12-binding]",
      "directx-headers", 
      "directx-dxc",
      "glm"
    ]
  }
  ```

#### **1.2 D3D12 Device & Context Module** (`platform.dx12`)
- Basic D3D12 device initialization
- Command queue, allocator, and command list setup
- Swap chain creation for window rendering
- Basic resource management (vertex/index buffers)

#### **1.3 Minimal Renderer Module** (`engine.renderer`)
- Immediate-mode rendering interface
- Basic shader compilation and caching
- Vertex formats for grid/wireframe rendering
- Simple render states (depth test, wireframe, etc.)

### **Phase 2: Camera Mathematics**
**Estimated Time**: 1-2 days

#### **2.1 Camera Module** (`engine.camera`)
- **Base Camera Class**:
  ```cpp
  export class Camera {
      Vec3f position{0,0,5};
      Vec3f target{0,0,0}; 
      Vec3f up{0,0,1};      // Z-up
      float fov = 45.0f;
      float nearPlane = 0.1f, farPlane = 1000.0f;
      
      Mat4f getViewMatrix() const;
      Mat4f getProjectionMatrix(float aspect) const;
      // etc.
  };
  ```

- **Specialized Camera Types**:
  - `PerspectiveCamera` - Free-look with orbit controls
  - `OrthographicCamera` - 2D navigation for top/front/side views

#### **2.2 Camera Controllers**
- Mouse input handling (orbit, pan, zoom)
- Keyboard shortcuts (focus, frame selection)
- Smooth interpolation for camera transitions
- View-specific constraints (orthographic locked to planes)

### **Phase 3: Multi-Viewport UI System**
**Estimated Time**: 2-3 days

#### **3.1 Viewport Management** (`editor.viewport`)
- **Viewport Class**:
  ```cpp
  export class Viewport {
      std::unique_ptr<Camera> camera;
      ViewportType type; // Perspective, Top, Front, Side
      ImTextureID renderTarget;
      Vec2i size;
      bool isActive = false;
      
      void render();
      void handleInput();
      Ray getPickingRay(Vec2f mousePos) const;
  };
  ```

#### **3.2 ImGui Integration**
- Dockspace setup with four locked panes
- Render target integration (D3D12 → ImGui texture)
- Input forwarding to active viewport
- Resize handling and aspect ratio management

### **Phase 4: Grid Rendering System**
**Estimated Time**: 1-2 days

#### **4.1 Grid Renderer** (`engine.grid`)
- Infinite grid shader (world-space grid lines)
- Adaptive grid density based on zoom level
- Major/minor grid line distinction
- Axis highlighting (X=red, Y=green, Z=blue)

#### **4.2 Coordinate System Helpers**
- Origin marker rendering
- Coordinate axis widgets
- Unit scale indicators
- Grid snapping utilities

### **Phase 5: Integration & Polish**
**Estimated Time**: 1-2 days

#### **5.1 Application Integration**
- Update `runtime.app` to integrate multi-viewport system
- Event routing between UI and viewports
- Frame timing and render loop optimization

#### **5.2 User Experience**
- Viewport focus management
- Consistent navigation across views
- View synchronization options
- Basic preferences (grid size, colors, etc.)

## 🔧 Technical Requirements

### **New Modules to Create**:
1. `platform.dx12` - D3D12 rendering backend
2. `engine.camera` - Camera mathematics and controllers
3. `engine.renderer` - Basic rendering primitives
4. `engine.grid` - Grid rendering system
5. `editor.viewport` - Multi-viewport management

### **Enhanced Modules**:
1. `runtime.app` - Integration with graphics and UI systems
2. `editor.ui` - Extended with docking and viewport panels
3. `platform.win32` - Input handling integration

### **Shader Assets**:
```
/shaders/
├── grid.hlsl          # Infinite grid shader
├── wireframe.hlsl     # Basic wireframe rendering  
├── common.hlsli       # Shared constants and utilities
└── compiled/          # DXC compiled output
```

## 📋 Detailed Task Breakdown

### **Week 1: Foundation**
- [x] Update vcpkg dependencies and rebuild
- [ ] Create basic D3D12 device initialization
- [ ] Implement Camera math classes with unit tests
- [ ] Create basic renderer interface

### **Week 2: Viewport System**  
- [ ] Implement multi-viewport UI layout
- [ ] Create render target → ImGui texture pipeline
- [ ] Implement camera controllers with input handling
- [ ] Add viewport focus and input routing

### **Week 3: Grid & Polish**
- [ ] Implement infinite grid shader
- [ ] Add coordinate system visualization
- [ ] Integrate snapping functionality
- [ ] Polish UI and add basic preferences

## 🧪 Testing Strategy

### **Unit Tests**:
- Camera matrix calculations (view, projection)
- Coordinate system conversions
- Ray casting from screen coordinates
- Grid snapping mathematics

### **Integration Tests**:
- Multi-viewport rendering consistency
- Input routing correctness
- Performance with four simultaneous viewports

### **Visual Validation**:
- Grid alignment across all views
- Coordinate system consistency (Z-up verification)
- Camera synchronization accuracy

## 🎯 Success Criteria

### **Functional Requirements**:
- ✅ Four functional viewport windows (Perspective, Top, Front, Side)
- ✅ Independent camera navigation in each viewport
- ✅ Infinite grid rendering with proper scaling
- ✅ Coordinate system axes clearly visible
- ✅ Input handled correctly in focused viewport

### **Quality Requirements**:
- 60+ FPS with all four viewports rendering
- Smooth camera interpolation and controls  
- Clean, professional UI layout
- Consistent Z-up coordinate system throughout

## 🔄 Risk Mitigation

### **Technical Risks**:
- **D3D12 Complexity**: Start with minimal implementation, expand gradually
- **ImGui Integration**: Use established patterns, test incremental changes
- **Performance**: Profile early, optimize render target sharing

### **Schedule Risks**:
- **Scope Creep**: Focus on core functionality first, defer polish features
- **Dependency Issues**: Test vcpkg integration early, have fallback plans

## 📚 Reference Materials

### **D3D12 Resources**:
- [Microsoft D3D12 Programming Guide](https://docs.microsoft.com/en-us/windows/win32/direct3d12/)
- [DirectX-Graphics-Samples](https://github.com/Microsoft/DirectX-Graphics-Samples)
- [D3D12 Memory Management Best Practices](https://developer.nvidia.com/dx12-dos-and-donts)

### **ImGui Integration**:
- [Dear ImGui Docking Branch](https://github.com/ocornut/imgui/tree/docking)
- [ImGui D3D12 Backend](https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_dx12.cpp)
- [Multi-Viewport Examples](https://github.com/ocornut/imgui/blob/master/examples/example_win32_directx12/)

### **Camera Mathematics**:
- [Real-Time Rendering 4th Edition](https://www.realtimerendering.com/) - Chapters 4-5
- [3D Math Primer for Graphics and Game Development](https://gamemath.com/)
- [Understanding the View Matrix](https://learnopengl.com/Getting-started/Camera)

### **Grid Rendering Techniques**:
- [Infinite Grid Shaders](https://asliceofrendering.com/scene%20helper/2020/01/05/InfiniteGrid/)
- [Adaptive Grid Systems](https://github.com/martin-pr/possumwood/wiki/Infinite-ground-plane-using-GLSL-shaders)
- [World-Space Grid Rendering](https://bgolus.medium.com/the-best-darn-grid-shader-yet-727f9278b9d8)

---

## 🚀 Getting Started

1. **Phase 1**: Begin with updating `vcpkg.json` and setting up basic D3D12 device
2. **Incremental Development**: Build and test each phase before moving to the next
3. **Documentation**: Update this plan as implementation details emerge
4. **Testing**: Create unit tests alongside each module
5. **Const correctness**: Ensure that all variable that can be const are indeed const

Ready to transform the math foundation into a functional 3D editor interface! 🎯

---

## ✅ Progress Tracking

### **Milestone 0**: Math Foundation ✅ **COMPLETE**
- ✅ C++23 Module System: 17+ math modules implemented
- ✅ Comprehensive Testing: 5,384 test assertions across 75 test cases
- ✅ Full Math Library: Vector, matrix, quaternion, curves, bounding volumes

### **Milestone 1**: Multi-Viewport Cameras

#### **Phase 1**: Graphics Foundation
- ✅ **1.1 Graphics Dependencies Setup** - COMPLETE (vcpkg.json updated, dependencies verified)
- ✅ **1.2 D3D12 Device & Context Module** - COMPLETE (platform.dx12 module created with device, command queue, swap chain, tests)
- ✅ **1.3 Minimal Renderer Module** - COMPLETE (engine.renderer module with immediate-mode rendering, shaders, vertex/index buffers, tests)

#### **Phase 2**: Camera Mathematics  
- ✅ **2.1 Camera Module** - COMPLETE (Camera base class, PerspectiveCamera, OrthographicCamera, CameraUtils with comprehensive tests)
- ✅ **2.2 Camera Controllers** - COMPLETE (PerspectiveCameraController, OrthographicCameraController with input handling, smooth focusing, auto-rotation)

#### **Phase 3**: Multi-Viewport UI System - **~40% COMPLETE**
- ❌ **3.1 Viewport Management** - NOT STARTED
  - Missing: `editor.viewport` module with Viewport class
  - Missing: Individual viewport cameras and render targets
  - Missing: D3D12 → ImGui texture integration
  - Missing: Per-viewport input handling and picking rays
- ✅ **3.2 ImGui Docking Integration** - COMPLETE
  - ✅ **Dockspace Setup**: Full DockBuilder implementation with 2x2 grid layout
  - ✅ **Four Viewport Panes**: Perspective, Top (XY), Front (XZ), Side (YZ) viewports
  - ✅ **Input Forwarding**: ImGui input handling integrated through Win32 message procedure
  - ✅ **UI Framework**: Complete ImGui integration with D3D12 backend
  - ✅ **Draw Data Rendering**: Proper ImGui draw data submission to D3D12 command list
  - ✅ **Menu System**: Functional File menu with Exit capability
  - ✅ **Application Lifecycle**: Exit functionality with state management

#### **Phase 4**: Grid Rendering
- ⏸️ **4.1 Grid Shader System** - PENDING
- ⏸️ **4.2 Grid Visual Settings** - PENDING

#### **Phase 5**: Integration & Polish
- ⏸️ **5.1 Window Integration** - PENDING
- ⏸️ **5.2 Performance & Polish** - PENDING

---

## 📊 Phase 3 Detailed Status Assessment

### **✅ What We've Implemented (ImGui Infrastructure)**

**Complete UI Shell with Professional Docking:**
- **DockBuilder Integration**: Automatic 2x2 grid layout using ImGui DockBuilder API
- **Viewport Panes**: Four properly named and positioned panes (Perspective, Top XY, Front XZ, Side YZ)
- **Input System**: Full ImGui input handling through Win32 message procedure integration
- **Rendering Pipeline**: Complete D3D12 backend integration with proper draw data submission
- **Menu System**: Functional File menu with working Exit functionality
- **Application Lifecycle**: Proper state management and clean shutdown

**Technical Implementation Details:**
- `src/editor/ui.{ixx,cpp}`: Complete UI class with setupInitialLayout(), renderDrawData(), shouldExit()
- `src/platform/win32/win32_window.cpp`: ImGui_ImplWin32_WndProcHandler integration
- `src/main.cpp`: Integrated UI rendering and exit handling in main loop
- **Testing**: Comprehensive unit tests (6,054 assertions across 148 test cases)

### **❌ What We Still Need (Actual Viewport Functionality)**

**Missing Core Viewport System:**
- **`editor.viewport` Module**: No Viewport class implementation yet
- **Camera Integration**: No cameras assigned to viewport panes
- **3D Rendering**: No actual 3D content rendering within panes
- **Render Targets**: No D3D12 render targets for 3D scenes
- **Texture Binding**: No D3D12 → ImGui texture integration
- **Input Handling**: No per-viewport mouse/keyboard input routing
- **Ray Casting**: No picking ray generation for 3D interaction

**Architecture Gap:**
```cpp
// What we have: Empty viewport panes with ImGui docking
// What we need: Functional 3D viewports like this:
export class Viewport {
    std::unique_ptr<Camera> camera;           // ❌ Missing
    ViewportType type;                        // ❌ Missing
    ImTextureID renderTarget;                 // ❌ Missing
    Vec2i size;                              // ❌ Missing
    bool isActive = false;                   // ❌ Missing
    
    void render();                           // ❌ Missing
    void handleInput();                      // ❌ Missing
    Ray getPickingRay(Vec2f mousePos) const;// ❌ Missing
};
```

### **🎯 Next Steps to Complete Phase 3**

**Priority 1: Viewport Class & Module**
1. Create `src/editor/viewport.{ixx,cpp}` with Viewport class
2. Integrate with existing camera system from Phase 2
3. Add ViewportType enum (Perspective, Top, Front, Side)

**Priority 2: Render Target Integration**
1. Create D3D12 render targets for each viewport
2. Implement D3D12 → ImGui texture binding
3. Integrate render targets with ImGui Image() calls in viewport panes

**Priority 3: Input & Interaction**
1. Add per-viewport input handling in UI system
2. Implement picking ray generation for 3D interaction
3. Connect viewport input to camera controllers

**Current Status**: We have an excellent **UI foundation** but need the **3D rendering core** to make it a true multi-viewport editor.
