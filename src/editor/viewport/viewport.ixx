// Viewport Management module for multi-viewport 3D editor
// Manages individual viewport instances with cameras, render targets, and input handling
export module editor.viewport;

import std;
import engine.vec;
import engine.matrix;
import engine.camera;
import engine.camera.controller;

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
	ViewportType GetType() const noexcept { return m_type; }
	const math::Vec2<int> &GetSize() const noexcept { return m_size; }
	float GetAspectRatio() const noexcept;

	// Active state management
	void SetActive( bool active ) noexcept { m_isActive = active; }
	bool IsActive() const noexcept { return m_isActive; }

	// Focus management (for input handling)
	void SetFocused( bool focused ) noexcept { m_isFocused = focused; }
	bool IsFocused() const noexcept { return m_isFocused; }

	// Camera access
	camera::Camera *GetCamera() const noexcept { return m_camera.get(); }
	camera::CameraController *GetController() const noexcept { return m_controller.get(); }

	// Render target management
	void SetRenderTargetSize( int width, int height );
	void *GetRenderTargetHandle() const noexcept { return m_renderTargetHandle; }

	// Frame update and rendering
	void Update( float deltaTime );
	void Render();

	// Input handling
	void HandleInput( const ViewportInputEvent &event );

	// 3D picking operations
	ViewportRay GetPickingRay( const math::Vec2<> &screenPos ) const noexcept;
	math::Vec3<> ScreenToWorld( const math::Vec2<> &screenPos, float depth = 0.0f ) const noexcept;
	math::Vec2<> WorldToScreen( const math::Vec3<> &worldPos ) const noexcept;

	// View operations
	void FrameAll() noexcept;
	void FrameSelection( const math::Vec3<> &center, const math::Vec3<> &size ) noexcept;
	void ResetView() noexcept;

	// Grid and gizmo settings
	void SetGridVisible( bool visible ) noexcept { m_showGrid = visible; }
	bool IsGridVisible() const noexcept { return m_showGrid; }

	void SetGizmosVisible( bool visible ) noexcept { m_showGizmos = visible; }
	bool AreGizmosVisible() const noexcept { return m_showGizmos; }

	// View synchronization (for linked views)
	void SetViewSyncEnabled( bool enabled ) noexcept { m_viewSyncEnabled = enabled; }
	bool IsViewSyncEnabled() const noexcept { return m_viewSyncEnabled; }

private:
	ViewportType m_type;
	math::Vec2<int> m_size{ 800, 600 };

	// State flags
	bool m_isActive = false;
	bool m_isFocused = false;
	bool m_showGrid = true;
	bool m_showGizmos = true;
	bool m_viewSyncEnabled = false;

	// Camera and controller
	std::unique_ptr<camera::Camera> m_camera;
	std::unique_ptr<camera::CameraController> m_controller;

	// Render target (will be D3D12 texture/ImGui integration)
	void *m_renderTargetHandle = nullptr;

	// Input state conversion
	camera::InputState ConvertToInputState( const ViewportInputEvent &event ) const;
	void UpdateInputState( const ViewportInputEvent &event );

	// Current input state for controller
	camera::InputState m_currentInput;

	// Mouse tracking for input deltas
	math::Vec2<> m_lastMousePos{ 0.0f, 0.0f };
	bool m_mouseTracking = false;

	// Initialize camera based on viewport type
	void InitializeCamera();
	void SetupOrthographicView();
};

// Multi-viewport manager coordinating multiple viewports
export class ViewportManager
{
public:
	ViewportManager() {}
	~ViewportManager() = default;

	// No copy/move for now
	ViewportManager( const ViewportManager & ) = delete;
	ViewportManager &operator=( const ViewportManager & ) = delete;

	// Viewport management
	Viewport *CreateViewport( ViewportType type );
	void DestroyViewport( Viewport *viewport );
	void DestroyAllViewports();

	// Access viewports
	Viewport *GetActiveViewport() const noexcept { return m_activeViewport; }
	Viewport *GetFocusedViewport() const noexcept { return m_focusedViewport; }
	const std::vector<std::unique_ptr<Viewport>> &GetViewports() const noexcept { return m_viewports; }

	// Active viewport management
	void SetActiveViewport( Viewport *viewport ) noexcept;
	void SetFocusedViewport( Viewport *viewport ) noexcept;

	// Frame update
	void Update( float deltaTime );
	void Render();

	// Global input handling and routing
	void HandleGlobalInput( const ViewportInputEvent &event );

	// View synchronization across viewports
	void SynchronizeViews( Viewport *sourceViewport );

	// Global viewport operations
	void FrameAllInAllViewports() noexcept;
	void ResetAllViews() noexcept;

	// Settings
	void SetGlobalGridVisible( bool visible ) noexcept;
	void SetGlobalGizmosVisible( bool visible ) noexcept;

private:
	std::vector<std::unique_ptr<Viewport>> m_viewports;
	Viewport *m_activeViewport = nullptr;
	Viewport *m_focusedViewport = nullptr;

	// Find viewport by pointer
	auto FindViewport( Viewport *viewport ) -> decltype( m_viewports.begin() );
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

StandardLayout CreateStandardLayout( ViewportManager &manager );

// Create single viewport of specified type
Viewport *CreateSingleViewport( ViewportManager &manager, ViewportType type );
} // namespace ViewportFactory

// Utility functions for viewport operations
export namespace ViewportUtils
{
// Convert between viewport coordinate systems
math::Vec2<> NormalizedToPixel( const math::Vec2<> &normalized, const math::Vec2<int> &size ) noexcept;
math::Vec2<> PixelToNormalized( const math::Vec2<> &pixel, const math::Vec2<int> &size ) noexcept;

// Viewport type utilities
const char *GetViewportTypeName( ViewportType type ) noexcept;
bool IsOrthographicType( ViewportType type ) noexcept;
camera::ViewType GetCameraViewType( ViewportType viewportType ) noexcept;

// Calculate optimal viewport sizes for multi-viewport layouts
struct ViewportLayout
{
	math::Vec2<int> perspectivePos, perspectiveSize;
	math::Vec2<int> topPos, topSize;
	math::Vec2<int> frontPos, frontSize;
	math::Vec2<int> sidePos, sideSize;
};

ViewportLayout CalculateStandardLayout( const math::Vec2<int> &totalSize ) noexcept;

// Input event creation helpers
ViewportInputEvent CreateMouseMoveEvent( float x, float y, float deltaX, float deltaY ) noexcept;
ViewportInputEvent CreateMouseButtonEvent( int button, bool pressed, float x, float y ) noexcept;
ViewportInputEvent CreateMouseWheelEvent( float delta, float x, float y ) noexcept;
ViewportInputEvent CreateKeyEvent( int keyCode, bool pressed, bool shift, bool ctrl, bool alt ) noexcept;
ViewportInputEvent CreateResizeEvent( int width, int height ) noexcept;
} // namespace ViewportUtils

} // namespace editor
