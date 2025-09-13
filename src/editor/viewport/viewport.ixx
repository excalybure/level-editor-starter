// Viewport Management module for multi-viewport 3D editor
// Manages individual viewport instances with cameras, render targets, and input handling
export module editor.viewport;

import std;
import engine.vec;
import engine.matrix;
import engine.camera;
import engine.camera.controller;
import runtime.ecs;
import runtime.systems;
import runtime.mesh_rendering_system;
import platform.dx12;
import engine.grid;
import engine.shader_manager;

export namespace editor
{

// Viewport types corresponding to different camera views
enum class ViewportType
{
	Perspective, // Free-look 3D perspective view
	Top,		 // Orthographic XY plane (looking down Z-axis)
	Front,		 // Orthographic XZ plane (looking down Y-axis)
	Side		 // Orthographic YZ plane (looking down X-axis)
};

// Input event data for viewports
struct ViewportInputEvent
{
	enum class Type
	{
		MouseMove,
		MouseButton,
		MouseWheel,
		KeyPress,
		KeyRelease,
		Resize
	};

	Type type;

	// Mouse data
	struct MouseData
	{
		float x = 0.0f, y = 0.0f;			// Position in viewport coordinates
		float deltaX = 0.0f, deltaY = 0.0f; // Delta movement
		int button = 0;						// Button index (0=left, 1=right, 2=middle)
		bool pressed = false;				// Button state
		float wheelDelta = 0.0f;			// Wheel scroll delta
	} mouse;

	// Keyboard data
	struct KeyboardData
	{
		int keyCode = 0;							   // Virtual key code
		bool shift = false, ctrl = false, alt = false; // Modifier states
	} keyboard;

	// Resize data
	struct ResizeData
	{
		int width = 0, height = 0; // New viewport size
	} resize;
};

// Ray for picking operations in 3D space
struct ViewportRay
{
	math::Vec3<> origin;
	math::Vec3<> direction;
	float length = 1000.0f; // Maximum ray distance
};

// Individual viewport managing a camera view with render target
export class Viewport
{
public:
	explicit Viewport( ViewportType type );
	~Viewport() = default;

	// No copy/move for now (render targets are not copyable)
	Viewport( const Viewport & ) = delete;
	Viewport &operator=( const Viewport & ) = delete;

	// Viewport properties
	ViewportType getType() const noexcept { return m_type; }
	math::Vec2<int> getSize() const noexcept; // Returns pending size if resize pending
	float getAspectRatio() const noexcept;

	// Active state management
	void setActive( bool active ) noexcept { m_isActive = active; }
	bool isActive() const noexcept { return m_isActive; }

	// Focus management (for input handling)
	void setFocused( bool focused ) noexcept { m_isFocused = focused; }
	bool isFocused() const noexcept { return m_isFocused; }

	// Camera access
	camera::Camera *getCamera() const noexcept { return m_camera.get(); }
	camera::CameraController *getController() const noexcept { return m_controller.get(); }

	// Render target management
	void setRenderTargetSize( int width, int height );
	void applyPendingResize( dx12::Device *device ); // Apply deferred resize
	void *getRenderTargetHandle() const noexcept { return m_renderTargetHandle; }
	bool createRenderTarget( dx12::Device *device, int width, int height );
	bool clearRenderTarget( dx12::Device *device, const float clearColor[4] );
	void *getImGuiTextureId() const noexcept;

	// Frame update and rendering
	void update( float deltaTime );
	void render( dx12::Device *device );

	// Input handling
	void handleInput( const ViewportInputEvent &event );

	// 3D picking operations
	ViewportRay getPickingRay( const math::Vec2<> &screenPos ) const noexcept;
	math::Vec3<> screenToWorld( const math::Vec2<> &screenPos, float depth = 0.0f ) const noexcept;
	math::Vec2<> worldToScreen( const math::Vec3<> &worldPos ) const noexcept;

	// View operations
	void frameAll() noexcept;
	void frameSelection( const math::Vec3<> &center, const math::Vec3<> &size ) noexcept;
	void resetView() noexcept;

	// Grid and gizmo settings
	void setGridVisible( bool visible ) noexcept { m_showGrid = visible; }
	bool isGridVisible() const noexcept { return m_showGrid; }

	void setGizmosVisible( bool visible ) noexcept { m_showGizmos = visible; }
	bool areGizmosVisible() const noexcept { return m_showGizmos; }

	// Grid settings management
	bool initializeGrid( dx12::Device *device, std::shared_ptr<shader_manager::ShaderManager> shaderManager );
	void setGridSettings( const grid::GridSettings &settings );
	const grid::GridSettings &getGridSettings() const;

	// View synchronization (for linked views)
	void setViewSyncEnabled( bool enabled ) noexcept { m_viewSyncEnabled = enabled; }
	bool isViewSyncEnabled() const noexcept { return m_viewSyncEnabled; }

private:
	ViewportType m_type;
	math::Vec2<int> m_size{ 800, 600 };

	// Pending resize to avoid resource deletion during command list building
	math::Vec2<int> m_pendingSize{ 0, 0 };
	bool m_resizePending = false;

	// State flags
	bool m_isActive = false;
	bool m_isFocused = false;
	bool m_showGrid = true;
	bool m_showGizmos = true;
	bool m_viewSyncEnabled = false;

	// Camera and controller
	std::unique_ptr<camera::Camera> m_camera;
	std::unique_ptr<camera::CameraController> m_controller;

	// D3D12 render target for this viewport
	std::shared_ptr<dx12::Texture> m_renderTarget;
	void *m_renderTargetHandle = nullptr; // For backward compatibility

	// Grid rendering system
	std::unique_ptr<grid::GridRenderer> m_gridRenderer;
	grid::GridSettings m_gridSettings;

	// Input state conversion
	camera::InputState convertToInputState( const ViewportInputEvent &event ) const;
	void updateInputState( const ViewportInputEvent &event );

	// Current input state for controller
	camera::InputState m_currentInput;

	// Mouse tracking for input deltas
	math::Vec2<> m_lastMousePos{ 0.0f, 0.0f };
	bool m_mouseTracking = false;

	// Initialize camera based on viewport type
	void initializeCamera();
	void setupOrthographicView();
};

// Multi-viewport manager coordinating multiple viewports
export class ViewportManager
{
public:
	ViewportManager() = default;
	~ViewportManager() = default;

	// No copy/move for now
	ViewportManager( const ViewportManager & ) = delete;
	ViewportManager &operator=( const ViewportManager & ) = delete;

	// Initialize with D3D12 device for render target creation
	bool initialize( dx12::Device *device, std::shared_ptr<shader_manager::ShaderManager> shaderManager );
	void shutdown();

	// Set scene and system manager for 3D content rendering
	void setSceneAndSystems( ecs::Scene *scene, systems::SystemManager *systemManager );

	// Viewport management
	Viewport *createViewport( ViewportType type );
	void destroyViewport( Viewport *viewport );
	void destroyAllViewports();

	// Access viewports
	Viewport *getActiveViewport() const noexcept { return m_activeViewport; }
	Viewport *getFocusedViewport() const noexcept { return m_focusedViewport; }
	const std::vector<std::unique_ptr<Viewport>> &getViewports() const noexcept { return m_viewports; }

	// Active viewport management
	void setActiveViewport( Viewport *viewport ) noexcept;
	void setFocusedViewport( Viewport *viewport ) noexcept;

	// Frame update
	void update( float deltaTime );
	void render();

	// Global input handling and routing
	void handleGlobalInput( const ViewportInputEvent &event );

	// View synchronization across viewports
	void synchronizeViews( Viewport *sourceViewport );

	// Global viewport operations
	void frameAllInAllViewports() noexcept;
	void resetAllViews() noexcept;

	// Settings
	void setGlobalGridVisible( bool visible ) noexcept;
	void setGlobalGizmosVisible( bool visible ) noexcept;

private:
	std::vector<std::unique_ptr<Viewport>> m_viewports;
	Viewport *m_activeViewport = nullptr;
	Viewport *m_focusedViewport = nullptr;

	// D3D12 device for render target creation
	dx12::Device *m_device = nullptr;

	// Shader manager for hot reloading
	std::shared_ptr<shader_manager::ShaderManager> m_shaderManager;

	// Scene and system manager for 3D content rendering
	ecs::Scene *m_scene = nullptr;
	systems::SystemManager *m_systemManager = nullptr;

	// Find viewport by pointer
	auto findViewport( Viewport *viewport ) -> decltype( m_viewports.begin() );
};

// Factory functions for creating standard viewport layouts
export namespace ViewportFactory
{
// Create standard 4-viewport layout (Perspective, Top, Front, Side)
struct StandardLayout
{
	Viewport *perspective = nullptr;
	Viewport *top = nullptr;
	Viewport *front = nullptr;
	Viewport *side = nullptr;
};

StandardLayout createStandardLayout( ViewportManager &manager );

// Create single viewport of specified type
Viewport *createSingleViewport( ViewportManager &manager, ViewportType type );
} // namespace ViewportFactory

// Utility functions for viewport operations
export namespace ViewportUtils
{
// Convert between viewport coordinate systems
math::Vec2<> normalizedToPixel( const math::Vec2<> &normalized, const math::Vec2<int> &size ) noexcept;
math::Vec2<> pixelToNormalized( const math::Vec2<> &pixel, const math::Vec2<int> &size ) noexcept;

// Viewport type utilities
const char *getViewportTypeName( ViewportType type ) noexcept;
bool isOrthographicType( ViewportType type ) noexcept;
camera::ViewType getCameraViewType( ViewportType viewportType ) noexcept;

// Calculate optimal viewport sizes for multi-viewport layouts
struct ViewportLayout
{
	math::Vec2<int> perspectivePos, perspectiveSize;
	math::Vec2<int> topPos, topSize;
	math::Vec2<int> frontPos, frontSize;
	math::Vec2<int> sidePos, sideSize;
};

ViewportLayout calculateStandardLayout( const math::Vec2<int> &totalSize ) noexcept;

// Input event creation helpers
ViewportInputEvent createMouseMoveEvent( float x, float y, float deltaX, float deltaY ) noexcept;
ViewportInputEvent createMouseButtonEvent( int button, bool pressed, float x, float y ) noexcept;
ViewportInputEvent createMouseWheelEvent( float delta, float x, float y ) noexcept;
ViewportInputEvent createKeyEvent( int keyCode, bool pressed, bool shift, bool ctrl, bool alt ) noexcept;
ViewportInputEvent createResizeEvent( int width, int height ) noexcept;
} // namespace ViewportUtils

} // namespace editor
