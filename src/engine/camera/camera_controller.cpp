// Camera Controller implementation
module engine.camera.controller;

import std;
import engine.math;

namespace camera
{

//=============================================================================
// Base Camera Controller Implementation
//=============================================================================

CameraController::CameraController( Camera *camera )
	: m_camera( camera )
{
	if ( !m_camera )
	{
		throw std::invalid_argument( "Camera cannot be null" );
	}
}

//=============================================================================
// Perspective Camera Controller Implementation
//=============================================================================

PerspectiveCameraController::PerspectiveCameraController( PerspectiveCamera *camera )
	: CameraController( camera ), m_perspectiveCamera( camera )
{
	if ( !m_perspectiveCamera )
	{
		throw std::invalid_argument( "PerspectiveCamera cannot be null" );
	}
}

void PerspectiveCameraController::Update( const InputState &input )
{
	if ( !m_enabled || !m_perspectiveCamera )
		return;

	// Handle focusing transition
	if ( m_focusState.isFocusing )
	{
		UpdateFocusing( input.deltaTime );
	}
	else
	{
		// Handle input-based camera control
		HandleOrbitInput( input );
		HandlePanInput( input );
		HandleZoomInput( input );
		HandleKeyboardInput( input );
	}

	// Handle auto-rotation
	if ( m_autoRotate )
	{
		UpdateAutoRotation( input.deltaTime );
	}
}

void PerspectiveCameraController::HandleOrbitInput( const InputState &input )
{
	// Left mouse button for orbiting
	if ( input.mouse.leftButton && !input.keyboard.shift && !input.keyboard.ctrl )
	{
		if ( !m_isDragging )
		{
			m_isDragging = true;
			m_lastMousePos = math::Vec2<>{ input.mouse.x, input.mouse.y };
		}
		else
		{
			const float deltaYaw = -( input.mouse.x - m_lastMousePos.x ) * m_orbitSensitivity;
			const float deltaPitch = ( input.mouse.y - m_lastMousePos.y ) * m_orbitSensitivity;

			m_perspectiveCamera->Orbit( deltaYaw, deltaPitch );
			m_lastMousePos = math::Vec2<>{ input.mouse.x, input.mouse.y };
		}
	}
	else
	{
		m_isDragging = false;
	}
}

void PerspectiveCameraController::HandlePanInput( const InputState &input )
{
	// Middle mouse button or Shift+Left mouse button for panning
	const bool shouldPan = input.mouse.middleButton ||
		( input.mouse.leftButton && input.keyboard.shift );

	if ( shouldPan )
	{
		if ( !m_isPanning )
		{
			m_isPanning = true;
			m_lastMousePos = math::Vec2<>{ input.mouse.x, input.mouse.y };
		}
		else
		{
			const float deltaX = ( input.mouse.x - m_lastMousePos.x ) * m_panSensitivity;
			const float deltaY = ( input.mouse.y - m_lastMousePos.y ) * m_panSensitivity;

			m_perspectiveCamera->Pan( deltaX, deltaY );
			m_lastMousePos = math::Vec2<>{ input.mouse.x, input.mouse.y };
		}
	}
	else
	{
		m_isPanning = false;
	}
}

void PerspectiveCameraController::HandleZoomInput( const InputState &input )
{
	// Mouse wheel for zooming
	if ( std::abs( input.mouse.wheelDelta ) > 0.001f )
	{
		const float distance = m_perspectiveCamera->GetDistance();
		const float zoomAmount = -input.mouse.wheelDelta * distance * 0.1f * m_zoomSensitivity;
		m_perspectiveCamera->Zoom( zoomAmount );
	}
}

void PerspectiveCameraController::HandleKeyboardInput( const InputState &input )
{
	if ( !input.keyboard.ctrl ) // Don't move when Ctrl is pressed (for shortcuts)
	{
		const float speed = m_keyboardMoveSpeed * input.deltaTime;
		const auto forward = m_perspectiveCamera->GetForwardVector();
		const auto right = m_perspectiveCamera->GetRightVector();
		const auto up = math::Vec3<>{ 0.0f, 0.0f, 1.0f }; // World up in Z-up system

		math::Vec3<> movement{ 0.0f, 0.0f, 0.0f };

		// WASD for horizontal movement
		if ( input.keyboard.w )
			movement += forward * speed;
		if ( input.keyboard.s )
			movement -= forward * speed;
		if ( input.keyboard.d )
			movement += right * speed;
		if ( input.keyboard.a )
			movement -= right * speed;

		// QE for vertical movement
		if ( input.keyboard.e )
			movement += up * speed;
		if ( input.keyboard.q )
			movement -= up * speed;

		if ( math::length( movement ) > 0.001f )
		{
			const auto currentPos = m_perspectiveCamera->GetPosition();
			const auto currentTarget = m_perspectiveCamera->GetTarget();

			m_perspectiveCamera->SetPosition( currentPos + movement );
			m_perspectiveCamera->SetTarget( currentTarget + movement );
		}
	}

	// F key for focus/frame
	if ( input.keyboard.f )
	{
		FocusOnPoint( math::Vec3<>{ 0.0f, 0.0f, 0.0f }, 10.0f ); // Focus on origin
	}
}

void PerspectiveCameraController::UpdateFocusing( float deltaTime )
{
	m_focusState.focusTime += deltaTime;
	const float t = std::min( m_focusState.focusTime / m_focusState.focusDuration, 1.0f );

	// Use smooth ease-out interpolation
	const float easeT = 1.0f - ( 1.0f - t ) * ( 1.0f - t );

	const auto currentPos = math::lerp( m_focusState.startPosition, m_focusState.targetPosition, easeT );
	const auto currentTarget = math::lerp( m_focusState.startLookAt, m_focusState.targetLookAt, easeT );

	m_perspectiveCamera->SetPosition( currentPos );
	m_perspectiveCamera->SetTarget( currentTarget );

	if ( t >= 1.0f )
	{
		m_focusState.isFocusing = false;
	}
}

void PerspectiveCameraController::UpdateAutoRotation( float deltaTime )
{
	const float rotationAmount = m_autoRotateSpeed * deltaTime;
	m_perspectiveCamera->Orbit( rotationAmount, 0.0f );
}

void PerspectiveCameraController::FocusOnPoint( const math::Vec3<> &point, float distance ) noexcept
{
	const auto currentPos = m_perspectiveCamera->GetPosition();
	const auto currentTarget = m_perspectiveCamera->GetTarget();

	// Calculate new position maintaining current viewing angle
	const auto currentDirection = math::normalize( currentPos - currentTarget );
	const auto newPosition = point + currentDirection * distance;

	// Setup focus transition
	m_focusState.isFocusing = true;
	m_focusState.startPosition = currentPos;
	m_focusState.startLookAt = currentTarget;
	m_focusState.targetPosition = newPosition;
	m_focusState.targetLookAt = point;
	m_focusState.focusTime = 0.0f;
	m_focusState.focusDuration = 1.0f;
}

void PerspectiveCameraController::FocusOnBounds( const math::Vec3<> &center, const math::Vec3<> &size ) noexcept
{
	// Use the camera's FocusOnBounds method directly
	m_perspectiveCamera->FocusOnBounds( center, size );

	// Could add smooth transition here if desired
}

//=============================================================================
// Orthographic Camera Controller Implementation
//=============================================================================

OrthographicCameraController::OrthographicCameraController( OrthographicCamera *camera )
	: CameraController( camera ), m_orthographicCamera( camera )
{
	if ( !m_orthographicCamera )
	{
		throw std::invalid_argument( "OrthographicCamera cannot be null" );
	}
}

void OrthographicCameraController::Update( const InputState &input )
{
	if ( !m_enabled || !m_orthographicCamera )
		return;

	HandlePanInput( input );
	HandleZoomInput( input );
}

void OrthographicCameraController::HandlePanInput( const InputState &input )
{
	// Left mouse button or middle mouse button for panning
	const bool shouldPan = input.mouse.leftButton || input.mouse.middleButton;

	if ( shouldPan )
	{
		if ( !m_isDragging )
		{
			m_isDragging = true;
			m_lastMousePos = math::Vec2<>{ input.mouse.x, input.mouse.y };
		}
		else
		{
			const float deltaX = ( input.mouse.x - m_lastMousePos.x ) * m_panSensitivity;
			const float deltaY = ( input.mouse.y - m_lastMousePos.y ) * m_panSensitivity;

			m_orthographicCamera->Pan( deltaX, deltaY );
			m_lastMousePos = math::Vec2<>{ input.mouse.x, input.mouse.y };
		}
	}
	else
	{
		m_isDragging = false;
	}
}

void OrthographicCameraController::HandleZoomInput( const InputState &input )
{
	// Mouse wheel for zooming
	if ( std::abs( input.mouse.wheelDelta ) > 0.001f )
	{
		const float currentSize = m_orthographicCamera->GetOrthographicSize();
		const float zoomAmount = input.mouse.wheelDelta * currentSize * 0.1f * m_zoomSensitivity;

		const float newSize = std::clamp(
			currentSize - zoomAmount,
			m_minOrthographicSize,
			m_maxOrthographicSize );

		m_orthographicCamera->SetOrthographicSize( newSize );
	}
}

void OrthographicCameraController::FrameBounds( const math::Vec3<> &center, const math::Vec3<> &size ) noexcept
{
	m_orthographicCamera->FrameBounds( center, size );
}

void OrthographicCameraController::SetZoomLimits( float minSize, float maxSize ) noexcept
{
	m_minOrthographicSize = std::max( 0.001f, minSize );
	m_maxOrthographicSize = std::max( m_minOrthographicSize, maxSize );
}

//=============================================================================
// Factory Functions Implementation
//=============================================================================

namespace ControllerFactory
{

std::unique_ptr<CameraController> CreateController( Camera *camera )
{
	if ( !camera )
		return nullptr;

	switch ( camera->GetType() )
	{
	case CameraType::Perspective:
		if ( auto *perspCamera = dynamic_cast<PerspectiveCamera *>( camera ) )
		{
			return std::make_unique<PerspectiveCameraController>( perspCamera );
		}
		break;

	case CameraType::Orthographic:
		if ( auto *orthoCamera = dynamic_cast<OrthographicCamera *>( camera ) )
		{
			return std::make_unique<OrthographicCameraController>( orthoCamera );
		}
		break;
	}

	return nullptr;
}

std::unique_ptr<PerspectiveCameraController> CreatePerspectiveController( PerspectiveCamera *camera )
{
	if ( !camera )
		return nullptr;

	return std::make_unique<PerspectiveCameraController>( camera );
}

std::unique_ptr<OrthographicCameraController> CreateOrthographicController( OrthographicCamera *camera )
{
	if ( !camera )
		return nullptr;

	return std::make_unique<OrthographicCameraController>( camera );
}

} // namespace ControllerFactory

//=============================================================================
// Input Utility Functions Implementation
//=============================================================================

namespace InputUtils
{

math::Vec2<> ScreenToNDC( const math::Vec2<> &screenPos, const math::Vec2<> &screenSize ) noexcept
{
	return math::Vec2<>{
		( 2.0f * screenPos.x / screenSize.x ) - 1.0f,
		1.0f - ( 2.0f * screenPos.y / screenSize.y )
	};
}

float CalculateDistanceBasedSensitivity( float baseSpeed, float distance, float minSpeed ) noexcept
{
	return std::max( minSpeed, baseSpeed * std::log10( std::max( 1.0f, distance * 0.1f ) ) );
}

float SmoothInput( float current, float target, float smoothing, float deltaTime ) noexcept
{
	const float t = 1.0f - std::exp( -smoothing * deltaTime );
	return math::lerp( current, target, t );
}

math::Vec2<> SmoothInput( const math::Vec2<> &current, const math::Vec2<> &target, float smoothing, float deltaTime ) noexcept
{
	const float t = 1.0f - std::exp( -smoothing * deltaTime );
	return math::lerp( current, target, t );
}

float ApplyDeadzone( float input, float deadzone ) noexcept
{
	const float absInput = std::abs( input );
	if ( absInput < deadzone )
		return 0.0f;

	const float sign = input > 0.0f ? 1.0f : -1.0f;
	return sign * ( ( absInput - deadzone ) / ( 1.0f - deadzone ) );
}

math::Vec2<> ApplyDeadzone( const math::Vec2<> &input, float deadzone ) noexcept
{
	const float magnitude = math::length( input );
	if ( magnitude < deadzone )
		return math::Vec2<>{ 0.0f, 0.0f };

	const auto direction = input / magnitude;
	const float adjustedMagnitude = ( magnitude - deadzone ) / ( 1.0f - deadzone );
	return direction * adjustedMagnitude;
}

} // namespace InputUtils

} // namespace camera
