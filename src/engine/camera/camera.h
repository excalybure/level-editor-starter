#pragma once

#include "../math/math.h"
#include "../math/vec.h"
#include "../math/matrix.h"
#include <limits>

// Camera module for 3D viewport management
// Implements Z-up right-handed coordinate system
namespace camera
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
class Camera
{
public:
	Camera();
	virtual ~Camera() = default;

	// No copy/move for now
	Camera( const Camera & ) = delete;
	Camera &operator=( const Camera & ) = delete;

	// Camera positioning (Z-up coordinate system)
	void setPosition( const math::Vec3<> &position ) noexcept;
	void setTarget( const math::Vec3<> &target ) noexcept;
	void setUp( const math::Vec3<> &up ) noexcept;

	const math::Vec3<> &getPosition() const noexcept { return m_position; }
	const math::Vec3<> &getTarget() const noexcept { return m_target; }
	const math::Vec3<> &getUp() const noexcept { return m_up; }

	// View and projection matrices
	virtual math::Mat4<> getViewMatrix() const noexcept;
	virtual math::Mat4<> getProjectionMatrix( float aspectRatio ) const noexcept = 0;

	// Camera properties
	void setNearPlane( float nearPlane ) noexcept { m_nearPlane = nearPlane; }
	void setFarPlane( float farPlane ) noexcept { m_farPlane = farPlane; }

	float getNearPlane() const noexcept { return m_nearPlane; }
	float getFarPlane() const noexcept { return m_farPlane; }

	// Camera type identification
	virtual CameraType getType() const noexcept = 0;
	virtual ViewType getViewType() const noexcept { return ViewType::Perspective; }

	// Camera vectors (derived from position, target, up)
	math::Vec3<> getForwardVector() const noexcept;
	math::Vec3<> getRightVector() const noexcept;
	math::Vec3<> getUpVector() const noexcept;

	// Distance from position to target
	float getDistance() const noexcept;

protected:
	// Z-up coordinate system (Z=up, Y=forward, X=right)
	math::Vec3<> m_position{ 0.0f, -5.0f, 5.0f }; // Start behind and above origin
	math::Vec3<> m_target{ 0.0f, 0.0f, 0.0f };	  // Look at origin
	math::Vec3<> m_up{ 0.0f, 0.0f, 1.0f };		  // Z-up

	float m_nearPlane = 0.1f;
	float m_farPlane = 1000.0f;
};

// Perspective camera for free-look navigation
class PerspectiveCamera : public Camera
{
public:
	PerspectiveCamera();
	explicit PerspectiveCamera( float fov );
	~PerspectiveCamera() override = default;

	// Field of view in degrees
	void setFieldOfView( float fovDegrees ) noexcept;
	float getFieldOfView() const noexcept { return m_fov; }

	// Projection matrix for perspective projection
	math::Mat4<> getProjectionMatrix( float aspectRatio ) const noexcept override;

	// Camera type identification
	CameraType getType() const noexcept override { return CameraType::Perspective; }

	// Orbit controls
	void orbit( float deltaYaw, float deltaPitch ) noexcept;
	void pan( float deltaX, float deltaY ) noexcept;
	void zoom( float deltaDistance ) noexcept;

	// Focus on target (frame selection)
	void focusOnPoint( const math::Vec3<> &point, float distance = 10.0f ) noexcept;
	void focusOnBounds( const math::Vec3<> &center, const math::Vec3<> &size ) noexcept;

private:
	float m_fov = 65.0f; // Field of view in degrees
};

// Orthographic camera for 2D-style navigation in specific planes
class OrthographicCamera : public Camera
{
public:
	OrthographicCamera();
	explicit OrthographicCamera( ViewType viewType );
	~OrthographicCamera() override = default;


	// Orthographic projection settings
	void setOrthographicSize( float size ) noexcept;
	float getOrthographicSize() const noexcept { return m_orthographicSize; }

	// Projection matrix for orthographic projection
	math::Mat4<> getProjectionMatrix( float aspectRatio ) const noexcept override;

	// Camera type identification
	CameraType getType() const noexcept override { return CameraType::Orthographic; }
	ViewType getViewType() const noexcept override { return m_viewType; }

	// 2D navigation (pan and zoom)
	void pan( float deltaX, float deltaY ) noexcept;
	void zoom( float deltaSize ) noexcept;

	// Set up camera for specific orthographic view
	void setupView( ViewType viewType ) noexcept;

	// Frame content (fit view to bounds)
	void frameBounds( const math::Vec3<> &center, const math::Vec3<> &size ) noexcept;

private:
	ViewType m_viewType = ViewType::Top;
	float m_orthographicSize = 10.0f; // Half-height of the view volume

	// Update camera position and orientation based on view type
	void updateCameraForViewType() noexcept;
};

// Utility functions for camera mathematics
namespace CameraUtils
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
