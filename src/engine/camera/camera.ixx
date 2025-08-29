// Camera module for 3D viewport management
// Implements Z-up right-handed coordinate system
export module engine.camera;

import std;
import engine.vec;
import engine.matrix;

export namespace camera
{

// Forward declarations
class Camera;
class PerspectiveCamera;
class OrthographicCamera;

// Camera types enumeration
enum class CameraType
{
	Perspective,
	Orthographic
};

// View types for orthographic cameras
enum class ViewType
{
	Perspective, // Free-look perspective
	Top,		 // XY plane (looking down Z-axis)
	Front,		 // XZ plane (looking down Y-axis)
	Side		 // YZ plane (looking down X-axis)
};

// Base camera class with Z-up coordinate system
export class Camera
{
public:
	Camera();
	virtual ~Camera() = default;

	// No copy/move for now
	Camera( const Camera & ) = delete;
	Camera &operator=( const Camera & ) = delete;

	// Camera positioning (Z-up coordinate system)
	void SetPosition( const math::Vec3<> &position ) noexcept;
	void SetTarget( const math::Vec3<> &target ) noexcept;
	void SetUp( const math::Vec3<> &up ) noexcept;

	const math::Vec3<> &GetPosition() const noexcept { return m_position; }
	const math::Vec3<> &GetTarget() const noexcept { return m_target; }
	const math::Vec3<> &GetUp() const noexcept { return m_up; }

	// View and projection matrices
	virtual math::Mat4<> GetViewMatrix() const noexcept;
	virtual math::Mat4<> GetProjectionMatrix( float aspectRatio ) const noexcept = 0;

	// Camera properties
	void SetNearPlane( float nearPlane ) noexcept { m_nearPlane = nearPlane; }
	void SetFarPlane( float farPlane ) noexcept { m_farPlane = farPlane; }

	float GetNearPlane() const noexcept { return m_nearPlane; }
	float GetFarPlane() const noexcept { return m_farPlane; }

	// Camera type identification
	virtual CameraType GetType() const noexcept = 0;
	virtual ViewType GetViewType() const noexcept { return ViewType::Perspective; }

	// Camera vectors (derived from position, target, up)
	math::Vec3<> GetForwardVector() const noexcept;
	math::Vec3<> GetRightVector() const noexcept;
	math::Vec3<> GetUpVector() const noexcept;

	// Distance from position to target
	float GetDistance() const noexcept;

protected:
	// Z-up coordinate system (Z=up, Y=forward, X=right)
	math::Vec3<> m_position{ 0.0f, -5.0f, 5.0f }; // Start behind and above origin
	math::Vec3<> m_target{ 0.0f, 0.0f, 0.0f };	  // Look at origin
	math::Vec3<> m_up{ 0.0f, 0.0f, 1.0f };		  // Z-up

	float m_nearPlane = 0.1f;
	float m_farPlane = 1000.0f;
};

// Perspective camera for free-look navigation
export class PerspectiveCamera : public Camera
{
public:
	PerspectiveCamera();
	explicit PerspectiveCamera( float fov );
	~PerspectiveCamera() override = default;

	// Field of view in degrees
	void SetFieldOfView( float fovDegrees ) noexcept;
	float GetFieldOfView() const noexcept { return m_fov; }

	// Projection matrix for perspective projection
	math::Mat4<> GetProjectionMatrix( float aspectRatio ) const noexcept override;

	// Camera type identification
	CameraType GetType() const noexcept override { return CameraType::Perspective; }

	// Orbit controls
	void Orbit( float deltaYaw, float deltaPitch ) noexcept;
	void Pan( float deltaX, float deltaY ) noexcept;
	void Zoom( float deltaDistance ) noexcept;

	// Focus on target (frame selection)
	void FocusOnPoint( const math::Vec3<> &point, float distance = 10.0f ) noexcept;
	void FocusOnBounds( const math::Vec3<> &center, const math::Vec3<> &size ) noexcept;

private:
	float m_fov = 65.0f; // Field of view in degrees
};

// Orthographic camera for 2D-style navigation in specific planes
export class OrthographicCamera : public Camera
{
public:
	OrthographicCamera();
	explicit OrthographicCamera( ViewType viewType );
	~OrthographicCamera() override = default;

	// Orthographic projection settings
	void SetOrthographicSize( float size ) noexcept;
	float GetOrthographicSize() const noexcept { return m_orthographicSize; }

	// Projection matrix for orthographic projection
	math::Mat4<> GetProjectionMatrix( float aspectRatio ) const noexcept override;

	// Camera type identification
	CameraType GetType() const noexcept override { return CameraType::Orthographic; }
	ViewType GetViewType() const noexcept override { return m_viewType; }

	// 2D navigation (pan and zoom)
	void Pan( float deltaX, float deltaY ) noexcept;
	void Zoom( float deltaSize ) noexcept;

	// Set up camera for specific orthographic view
	void SetupView( ViewType viewType ) noexcept;

	// Frame content (fit view to bounds)
	void FrameBounds( const math::Vec3<> &center, const math::Vec3<> &size ) noexcept;

private:
	ViewType m_viewType = ViewType::Top;
	float m_orthographicSize = 10.0f; // Half-height of the view volume

	// Update camera position and orientation based on view type
	void UpdateCameraForViewType() noexcept;
};

// Utility functions for camera mathematics
export namespace CameraUtils
{
// Convert screen coordinates to world ray
struct Ray
{
	math::Vec3<> origin;
	math::Vec3<> direction;
};

Ray ScreenToWorldRay(
	const math::Vec2<> &screenPos,	// Screen coordinates (0,0 to width,height)
	const math::Vec2<> &screenSize, // Screen dimensions
	const math::Mat4<> &viewMatrix,
	const math::Mat4<> &projMatrix ) noexcept;

// Project world point to screen coordinates
math::Vec3<> WorldToScreen(
	const math::Vec3<> &worldPos,
	const math::Vec2<> &screenSize,
	const math::Mat4<> &viewMatrix,
	const math::Mat4<> &projMatrix ) noexcept;

// Calculate optimal camera distance for framing bounds
float CalculateFramingDistance(
	const math::Vec3<> &boundsSize,
	float fovDegrees,
	float aspectRatio ) noexcept;

// Smooth camera interpolation
math::Vec3<> SmoothDamp(
	const math::Vec3<> &current,
	const math::Vec3<> &target,
	math::Vec3<> &velocity,
	float smoothTime,
	float deltaTime,
	float maxSpeed = std::numeric_limits<float>::infinity() ) noexcept;
} // namespace CameraUtils

} // namespace camera
