// Camera implementation
#include "camera.h"
#include "math/math.h"
#include "math/vec.h"
#include "math/matrix.h"
#include "core/console.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace camera
{

//=============================================================================
// Base Camera Implementation
//=============================================================================

Camera::Camera()
{
	// Default Z-up camera positioned to look at origin
	m_position = math::Vec3<>{ 0.0f, -5.0f, 5.0f };
	m_target = math::Vec3<>{ 0.0f, 0.0f, 0.0f };
	m_up = math::Vec3<>{ 0.0f, 0.0f, 1.0f };
}

void Camera::setPosition( const math::Vec3<> &position ) noexcept
{
	m_position = position;
}

void Camera::setTarget( const math::Vec3<> &target ) noexcept
{
	m_target = target;
}

void Camera::setUp( const math::Vec3<> &up ) noexcept
{
	m_up = math::normalize( up );
}

math::Mat4<> Camera::getViewMatrix() const noexcept
{
	return math::Mat4<>::lookAt( m_position, m_target, m_up );
}

math::Vec3<> Camera::getForwardVector() const noexcept
{
	return math::normalize( m_target - m_position );
}

math::Vec3<> Camera::getRightVector() const noexcept
{
	const auto forward = getForwardVector();
	return math::normalize( math::cross( forward, m_up ) );
}

math::Vec3<> Camera::getUpVector() const noexcept
{
	const auto forward = getForwardVector();
	const auto right = getRightVector();
	return math::cross( right, forward );
}

float Camera::getDistance() const noexcept
{
	return math::length( m_target - m_position );
}

//=============================================================================
// Perspective Camera Implementation
//=============================================================================

PerspectiveCamera::PerspectiveCamera()
	: Camera(), m_fov( 65.0f )
{
}

PerspectiveCamera::PerspectiveCamera( float fov )
	: Camera(), m_fov( fov )
{
}

void PerspectiveCamera::setFieldOfView( float fovDegrees ) noexcept
{
	m_fov = std::clamp( fovDegrees, 1.0f, 179.0f );
}

math::Mat4<> PerspectiveCamera::getProjectionMatrix( float aspectRatio ) const noexcept
{
	return math::Mat4<>::perspective( math::radians( m_fov ), aspectRatio, m_nearPlane, m_farPlane );
}

void PerspectiveCamera::orbit( float deltaYaw, float deltaPitch ) noexcept
{
	const float distance = getDistance();
	if ( distance < 0.001f )
		return; // Avoid singularity

	// Convert current position relative to target to spherical coordinates
	const math::Vec3<> offset = m_position - m_target;

	// Calculate current spherical angles (Z-up system)
	const float currentRadius = math::length( offset );
	float currentYaw = std::atan2( offset.x, offset.y );		// Rotation around Z-axis
	float currentPitch = std::asin( offset.z / currentRadius ); // Elevation angle

	// Apply deltas
	currentYaw += math::radians( deltaYaw );
	currentPitch = std::clamp( currentPitch + math::radians( deltaPitch ),
		-math::pi<float> * 0.49f,
		math::pi<float> * 0.49f ); // Prevent gimbal lock by not allowing -PI/2 & PI/2

	// Convert back to Cartesian coordinates (Z-up)
	const float cosYaw = std::cos( currentYaw );
	const float sinYaw = std::sin( currentYaw );
	const float cosPitch = std::cos( currentPitch );
	const float sinPitch = std::sin( currentPitch );

	const math::Vec3<> newOffset{
		currentRadius * cosPitch * sinYaw, // X
		currentRadius * cosPitch * cosYaw, // Y
		currentRadius * sinPitch		   // Z
	};

	m_position = m_target + newOffset;
}

void PerspectiveCamera::pan( float deltaX, float deltaY ) noexcept
{
	const float distance = getDistance();
	const float panSpeed = distance * 0.001f; // Scale panning with distance

	const math::Vec3<> right = getRightVector();
	const math::Vec3<> up = getUpVector();

	const math::Vec3<> offset = right * ( -deltaX * panSpeed ) + up * ( deltaY * panSpeed );

	m_position += offset;
	m_target += offset;
}

void PerspectiveCamera::zoom( float deltaDistance ) noexcept
{
	const float currentDistance = getDistance();
	const float newDistance = std::max( 0.1f, currentDistance + deltaDistance );

	const math::Vec3<> direction = math::normalize( m_position - m_target );
	m_position = m_target + direction * newDistance;
	console::info( "Camera zoomed to distance: {} and position is now ({}, {}, {})", newDistance, m_position.x, m_position.y, m_position.z );
}

void PerspectiveCamera::focusOnPoint( const math::Vec3<> &point, float distance ) noexcept
{
	const math::Vec3<> direction = math::normalize( m_position - m_target );
	m_target = point;
	m_position = m_target + direction * distance;
}

void PerspectiveCamera::focusOnBounds( const math::Vec3<> &center, const math::Vec3<> &size ) noexcept
{
	const float distance = CameraUtils::CalculateFramingDistance( size, m_fov, 1.0f ); // Assume square aspect

	focusOnPoint( center, distance );
}

//=============================================================================
// Orthographic Camera Implementation
//=============================================================================

OrthographicCamera::OrthographicCamera()
	: Camera(), m_viewType( ViewType::Top ), m_orthographicSize( 10.0f )
{
	updateCameraForViewType();
}

OrthographicCamera::OrthographicCamera( ViewType viewType )
	: Camera(), m_viewType( viewType ), m_orthographicSize( 10.0f )
{
	updateCameraForViewType();
}

void OrthographicCamera::setOrthographicSize( float size ) noexcept
{
	m_orthographicSize = std::max( 0.1f, size );
}

math::Mat4<> OrthographicCamera::getProjectionMatrix( float aspectRatio ) const noexcept
{
	const float halfHeight = m_orthographicSize;
	const float halfWidth = halfHeight * aspectRatio;

	return math::Mat4<>::orthographic( -halfWidth, halfWidth, -halfHeight, halfHeight, m_nearPlane, m_farPlane );
}

void OrthographicCamera::pan( float deltaX, float deltaY ) noexcept
{
	const float panSpeed = m_orthographicSize * 0.001f;

	const math::Vec3<> right = getRightVector();
	const math::Vec3<> up = getUpVector();

	const math::Vec3<> offset = right * ( -deltaX * panSpeed ) + up * ( deltaY * panSpeed );

	m_position += offset;
	m_target += offset;
}

void OrthographicCamera::zoom( float deltaSize ) noexcept
{
	setOrthographicSize( m_orthographicSize + deltaSize );
}

void OrthographicCamera::setupView( ViewType viewType ) noexcept
{
	m_viewType = viewType;
	updateCameraForViewType();
}

void OrthographicCamera::frameBounds( const math::Vec3<> &center, const math::Vec3<> &size ) noexcept
{
	// Calculate appropriate orthographic size based on view type
	float requiredSize = 0.0f;
	switch ( m_viewType )
	{
	case ViewType::Top: // XY plane
		requiredSize = std::max( size.x, size.y ) * 0.6f;
		break;
	case ViewType::Front: // XZ plane
		requiredSize = std::max( size.x, size.z ) * 0.6f;
		break;
	case ViewType::Side: // YZ plane
		requiredSize = std::max( size.y, size.z ) * 0.6f;
		break;
	default:
		requiredSize = std::max( { size.x, size.y, size.z } ) * 0.6f;
		break;
	}

	setOrthographicSize( requiredSize );

	// Update camera positioning first, then override target to center on bounds
	updateCameraForViewType();

	// Set target to the actual bounds center (this should override any constraints)
	m_target = center;

	// Recalculate position to maintain proper distance and orientation from the new target
	const float distance = 50.0f; // Same distance as updateCameraForViewType
	switch ( m_viewType )
	{
	case ViewType::Top: // XY plane, looking down Z-axis
		m_position = m_target + math::Vec3<>{ 0.0f, 0.0f, distance };
		break;
	case ViewType::Front: // XZ plane, looking down Y-axis
		m_position = m_target + math::Vec3<>{ 0.0f, -distance, 0.0f };
		break;
	case ViewType::Side: // YZ plane, looking down X-axis
		m_position = m_target + math::Vec3<>{ distance, 0.0f, 0.0f };
		break;
	default:
		std::unreachable();
		break;
	}
}

void OrthographicCamera::updateCameraForViewType() noexcept
{
	const float distance = 50.0f; // Fixed distance for orthographic projection

	switch ( m_viewType )
	{
	case ViewType::Top: // XY plane, looking down Z-axis
		// Constrain target to the XY plane so we never drift below/above unintentionally
		m_target.z = 0.0f;
		m_position = m_target + math::Vec3<>{ 0.0f, 0.0f, distance };
		m_up = math::Vec3<>{ 0.0f, 1.0f, 0.0f }; // Y is up in top view
		break;

	case ViewType::Front: // XZ plane, looking down Y-axis
		// Constrain target to the XZ plane
		m_target.y = 0.0f;
		m_position = m_target + math::Vec3<>{ 0.0f, -distance, 0.0f };
		m_up = math::Vec3<>{ 0.0f, 0.0f, 1.0f }; // Z is up in front view
		break;

	case ViewType::Side: // YZ plane, looking down X-axis
		// Constrain target to the YZ plane
		m_target.x = 0.0f;
		m_position = m_target + math::Vec3<>{ distance, 0.0f, 0.0f };
		m_up = math::Vec3<>{ 0.0f, 0.0f, 1.0f }; // Z is up in side view
		break;

	default:
		std::unreachable();
		break;
	}
}

//=============================================================================
// Camera Utilities Implementation
//=============================================================================

namespace CameraUtils
{

Ray ScreenToWorldRay(
	const math::Vec2<> &screenPos,
	const math::Vec2<> &screenSize,
	const math::Mat4<> &viewMatrix,
	const math::Mat4<> &projMatrix ) noexcept
{
	// Convert screen coordinates to normalized device coordinates (-1 to 1)
	const float x = ( 2.0f * screenPos.x ) / screenSize.x - 1.0f;
	const float y = 1.0f - ( 2.0f * screenPos.y ) / screenSize.y; // Flip Y

	// Create ray in clip space
	const math::Vec4<> rayClip{ x, y, -1.0f, 1.0f };

	// Transform to eye space
	const math::Mat4<> invProj = projMatrix.inverse();
	math::Vec4<> rayEye = invProj * rayClip;
	rayEye.z = -1.0f; // Forward direction
	rayEye.w = 0.0f;  // Direction, not position

	// Transform to world space
	const math::Mat4<> invView = viewMatrix.inverse();
	const math::Vec4<> rayWorld = invView * rayEye;

	// Extract origin from inverse view matrix (camera position)
	const math::Vec3<> origin = invView.row3.xyz();

	// Normalize direction
	const math::Vec3<> direction = math::normalize( rayWorld.xyz() );

	return Ray{ origin, direction };
}

math::Vec3<> WorldToScreen(
	const math::Vec3<> &worldPos,
	const math::Vec2<> &screenSize,
	const math::Mat4<> &viewMatrix,
	const math::Mat4<> &projMatrix ) noexcept
{
	// Transform world position to clip space
	const math::Vec4<> worldPos4{ worldPos.x, worldPos.y, worldPos.z, 1.0f };
	const math::Vec4<> clipPos = projMatrix * viewMatrix * worldPos4;

	// Perspective division
	if ( std::abs( clipPos.w ) < 0.0001f )
		return math::Vec3<>{ -1.0f, -1.0f, -1.0f }; // Invalid

	const math::Vec3<> ndc = clipPos.xyz() / clipPos.w;

	// Convert to screen coordinates
	const float screenX = ( ndc.x + 1.0f ) * 0.5f * screenSize.x;
	const float screenY = ( 1.0f - ndc.y ) * 0.5f * screenSize.y; // Flip Y

	return math::Vec3<>{ screenX, screenY, ndc.z };
}

float CalculateFramingDistance(
	const math::Vec3<> &boundsSize,
	float fovDegrees,
	float aspectRatio ) noexcept
{
	const float fovRadians = math::radians( fovDegrees );
	const float verticalHalfFov = fovRadians * 0.5f;
	const float horizontalHalfFov = std::atan( std::tan( verticalHalfFov ) * aspectRatio );

	// Calculate required distance for both horizontal and vertical extents
	// We need to consider all three dimensions as they project differently
	const float maxHorizontalExtent = std::max( boundsSize.x, boundsSize.y );
	const float maxVerticalExtent = std::max( boundsSize.y, boundsSize.z );

	// Calculate distance needed for horizontal and vertical fitting
	const float horizontalDistance = ( maxHorizontalExtent * 0.5f ) / std::tan( horizontalHalfFov );
	const float verticalDistance = ( maxVerticalExtent * 0.5f ) / std::tan( verticalHalfFov );

	// Use the larger distance to ensure the object fits in both dimensions
	const float distance = std::max( horizontalDistance, verticalDistance );

	// Add some padding
	return distance * 1.2f;
}

math::Vec3<> SmoothDamp(
	const math::Vec3<> &current,
	const math::Vec3<> &target,
	math::Vec3<> &velocity,
	float smoothTime,
	float deltaTime,
	float maxSpeed ) noexcept
{
	// Smooth damping algorithm based on Game Programming Gems 4 Chapter 1.10
	smoothTime = std::max( 0.0001f, smoothTime );
	const float omega = 2.0f / smoothTime;
	const float x = omega * deltaTime;
	const float exp = 1.0f / ( 1.0f + x + 0.48f * x * x + 0.235f * x * x * x );

	math::Vec3<> change = current - target;
	const math::Vec3<> originalTo = target;

	// Clamp maximum speed
	const float maxChange = maxSpeed * smoothTime;
	const float changeLength = math::length( change );
	if ( changeLength > maxChange )
	{
		change = ( change / changeLength ) * maxChange;
	}

	const math::Vec3<> newTarget = current - change;
	const math::Vec3<> temp = ( velocity + omega * change ) * deltaTime;
	velocity = ( velocity - omega * temp ) * exp;

	math::Vec3<> result = newTarget + ( change + temp ) * exp;

	// Prevent overshooting
	const math::Vec3<> resultChange = result - originalTo;
	const math::Vec3<> originalChange = current - originalTo;
	if ( math::dot( resultChange, originalChange ) > 0 )
	{
		result = originalTo;
		velocity = math::Vec3<>{ 0.0f, 0.0f, 0.0f };
	}

	return result;
}

} // namespace CameraUtils

} // namespace camera
