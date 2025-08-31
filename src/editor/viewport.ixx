// Viewport module for multi-viewport 3D editor
// Manages individual viewport rendering, input handling, and camera integration
export module editor.viewport;

import std;
import engine.vec;
import engine.matrix;
import engine.camera;
import engine.camera.controller;

export namespace editor
{

// Viewport types matching camera ViewType
enum class ViewportType
{
	Perspective, // Free-look perspective camera
	Top,		 // XY plane (looking down Z-axis)
	Front,		 // XZ plane (looking down Y-axis)
	Side		 // YZ plane (looking down X-axis)
};

// Ray structure for 3D picking
struct Ray
{
	math::Vec3<> origin;
	math::Vec3<> direction;

	Ray() = default;
	Ray( const math::Vec3<> &origin_, const math::Vec3<> &direction_ )
		: origin( origin_ ), direction( direction_ ) {}
};

// Viewport size and state
struct ViewportInfo
{
	int width = 800;
	int height = 600;
	bool isActive = false;
	bool hasFocus = false;
};

// Individual viewport managing camera, rendering, and input
export class Viewport
{
public:
	// Constructor
	explicit Viewport( ViewportType type );
	~Viewport() = default;

	// No copy/move for now
	Viewport( const Viewport & ) = delete;
	Viewport &operator=( const Viewport & ) = delete;

	// Viewport properties
	ViewportType getType() const noexcept { return m_type; }
	const ViewportInfo &getInfo() const noexcept { return m_info; }
	ViewportInfo &getInfo() noexcept { return m_info; }

	// Size management
	void resize( int width, int height );
	float getAspectRatio() const noexcept;

	// Camera access
	camera::Camera *getCamera() const noexcept { return m_camera.get(); }
	camera::CameraController *getCameraController() const noexcept { return m_controller.get(); }

	// Rendering (placeholder for now - will integrate with D3D12 later)
	void beginRender();
	void endRender();

	// Get render target texture ID for ImGui integration
	// Returns nullptr for now - will be implemented when D3D12 integration is added
	void *getRenderTargetTextureID() const noexcept { return nullptr; }

	// Input handling
	void handleInput( const camera::InputState &input );
	void setFocus( bool hasFocus );

	// 3D picking - convert screen coordinates to world ray
	Ray getPickingRay( float screenX, float screenY ) const;

	// Viewport-specific utility functions
	const char *getDisplayName() const noexcept;
	camera::ViewType getCameraViewType() const noexcept;

private:
	ViewportType m_type;
	ViewportInfo m_info;

	// Camera system
	std::unique_ptr<camera::Camera> m_camera;
	std::unique_ptr<camera::CameraController> m_controller;

	// Render targets (will be added when D3D12 integration is implemented)
	// TODO: Add D3D12 render target, descriptor heaps, etc.

	// Initialize camera based on viewport type
	void initializeCamera();
	void initializeCameraController();
};

// Utility functions for viewport management
export namespace ViewportUtils
{
// Convert ViewportType to display string
const char *getViewportTypeName( ViewportType type );

// Convert ViewportType to camera ViewType
camera::ViewType toCameraViewType( ViewportType type );

// Get default camera position for viewport type
math::Vec3<> getDefaultCameraPosition( ViewportType type );
math::Vec3<> getDefaultCameraTarget( ViewportType type );
math::Vec3<> getDefaultCameraUp( ViewportType type );
} // namespace ViewportUtils

} // namespace editor
