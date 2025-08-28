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

Ready to transform the math foundation into a functional 3D editor interface! 🎯
