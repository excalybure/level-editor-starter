// Viewport implementation
module editor.viewport;

import std;
import engine.vec;
import engine.matrix;
import engine.camera;
import engine.camera.controller;

using namespace editor;
using namespace math;

// Viewport constructor
Viewport::Viewport( ViewportType type )
	: m_type( type )
{
	initializeCamera();
	initializeCameraController();
}

// Size management
void Viewport::resize( int width, int height )
{
	m_info.width = width;
	m_info.height = height;
}

float Viewport::getAspectRatio() const noexcept
{
	if ( m_info.height == 0 )
		return 1.0f;
	return static_cast<float>( m_info.width ) / static_cast<float>( m_info.height );
}

// Rendering placeholders
void Viewport::beginRender()
{
	// TODO: Set up D3D12 render targets, clear buffers, set viewport/scissor
	// This will be implemented when we integrate with the D3D12 renderer
}

void Viewport::endRender()
{
	// TODO: Present render target, generate ImGui texture
	// This will be implemented when we integrate with the D3D12 renderer
}

// Input handling
void Viewport::handleInput( const camera::InputState &input )
{
	if ( m_controller && m_info.hasFocus )
	{
		m_controller->update( input );
	}
}

void Viewport::setFocus( bool hasFocus )
{
	m_info.hasFocus = hasFocus;
}

// 3D picking
Ray Viewport::getPickingRay( float screenX, float screenY ) const
{
	if ( !m_camera )
	{
		// Return default ray if no camera
		return Ray( Vec3<>( 0, 0, 0 ), Vec3<>( 0, 0, -1 ) );
	}

	// Convert screen coordinates to normalized device coordinates
	// Screen space: (0,0) top-left to (width,height) bottom-right
	// NDC space: (-1,-1) bottom-left to (1,1) top-right
	float ndcX = ( screenX / static_cast<float>( m_info.width ) ) * 2.0f - 1.0f;
	float ndcY = 1.0f - ( screenY / static_cast<float>( m_info.height ) ) * 2.0f;

	// Get camera matrices
	Mat4<> viewMatrix = m_camera->getViewMatrix();
	Mat4<> projMatrix = m_camera->getProjectionMatrix( getAspectRatio() );

	// Compute inverse view-projection matrix
	Mat4<> viewProjMatrix = projMatrix * viewMatrix;
	Mat4<> invViewProjMatrix = inverse( viewProjMatrix );

	// Create points in clip space (near and far plane)
	Vec4<> nearPoint( ndcX, ndcY, -1.0f, 1.0f ); // Near plane
	Vec4<> farPoint( ndcX, ndcY, 1.0f, 1.0f );	 // Far plane

	// Transform to world space
	Vec4<> worldNear = invViewProjMatrix * nearPoint;
	Vec4<> worldFar = invViewProjMatrix * farPoint;

	// Perform perspective divide
	if ( worldNear.w != 0.0f )
	{
		worldNear.x /= worldNear.w;
		worldNear.y /= worldNear.w;
		worldNear.z /= worldNear.w;
	}

	if ( worldFar.w != 0.0f )
	{
		worldFar.x /= worldFar.w;
		worldFar.y /= worldFar.w;
		worldFar.z /= worldFar.w;
	}

	// Create ray
	Vec3<> origin( worldNear.x, worldNear.y, worldNear.z );
	Vec3<> direction = normalize( Vec3<>( worldFar.x - worldNear.x, worldFar.y - worldNear.y, worldFar.z - worldNear.z ) );

	return Ray( origin, direction );
}

// Display name
const char *Viewport::getDisplayName() const noexcept
{
	return ViewportUtils::getViewportTypeName( m_type );
}

// Camera view type conversion
camera::ViewType Viewport::getCameraViewType() const noexcept
{
	return ViewportUtils::toCameraViewType( m_type );
}

// Camera initialization
void Viewport::initializeCamera()
{
	camera::ViewType cameraViewType = getCameraViewType();
	Vec3<> position = ViewportUtils::getDefaultCameraPosition( m_type );
	Vec3<> target = ViewportUtils::getDefaultCameraTarget( m_type );
	Vec3<> up = ViewportUtils::getDefaultCameraUp( m_type );

	if ( cameraViewType == camera::ViewType::Perspective )
	{
		// Create perspective camera
		auto perspCamera = std::make_unique<camera::PerspectiveCamera>();
		perspCamera->setPosition( position );
		perspCamera->setTarget( target );
		perspCamera->setUp( up );
		perspCamera->setFieldOfView( 45.0f ); // 45-degree FOV
		perspCamera->setNearFar( 0.1f, 1000.0f );

		m_camera = std::move( perspCamera );
	}
	else
	{
		// Create orthographic camera
		auto orthoCamera = std::make_unique<camera::OrthographicCamera>();
		orthoCamera->setPosition( position );
		orthoCamera->setTarget( target );
		orthoCamera->setUp( up );
		orthoCamera->setViewType( cameraViewType );
		orthoCamera->setSize( 10.0f ); // 10-unit orthographic size
		orthoCamera->setNearFar( -100.0f, 100.0f );

		m_camera = std::move( orthoCamera );
	}
}

void Viewport::initializeCameraController()
{
	if ( !m_camera )
		return;

	camera::ViewType cameraViewType = getCameraViewType();

	if ( cameraViewType == camera::ViewType::Perspective )
	{
		// Create perspective camera controller
		m_controller = std::make_unique<camera::PerspectiveCameraController>( m_camera.get() );
	}
	else
	{
		// Create orthographic camera controller
		m_controller = std::make_unique<camera::OrthographicCameraController>( m_camera.get() );
	}
}

// Utility function implementations
namespace editor::ViewportUtils
{

const char *getViewportTypeName( ViewportType type )
{
	switch ( type )
	{
	case ViewportType::Perspective:
		return "Perspective";
	case ViewportType::Top:
		return "Top (XY)";
	case ViewportType::Front:
		return "Front (XZ)";
	case ViewportType::Side:
		return "Side (YZ)";
	default:
		return "Unknown";
	}
}

camera::ViewType toCameraViewType( ViewportType type )
{
	switch ( type )
	{
	case ViewportType::Perspective:
		return camera::ViewType::Perspective;
	case ViewportType::Top:
		return camera::ViewType::Top;
	case ViewportType::Front:
		return camera::ViewType::Front;
	case ViewportType::Side:
		return camera::ViewType::Side;
	default:
		return camera::ViewType::Perspective;
	}
}

Vec3<> getDefaultCameraPosition( ViewportType type )
{
	switch ( type )
	{
	case ViewportType::Perspective:
		return Vec3<>( 5.0f, -5.0f, 5.0f ); // Isometric-like view
	case ViewportType::Top:
		return Vec3<>( 0.0f, 0.0f, 10.0f ); // Looking down Z-axis
	case ViewportType::Front:
		return Vec3<>( 0.0f, -10.0f, 0.0f ); // Looking down Y-axis
	case ViewportType::Side:
		return Vec3<>( 10.0f, 0.0f, 0.0f ); // Looking down X-axis
	default:
		return Vec3<>( 0.0f, 0.0f, 5.0f );
	}
}

Vec3<> getDefaultCameraTarget( ViewportType type )
{
	// All cameras look at origin by default
	return Vec3<>( 0.0f, 0.0f, 0.0f );
}

Vec3<> getDefaultCameraUp( ViewportType type )
{
	switch ( type )
	{
	case ViewportType::Perspective:
		return Vec3<>( 0.0f, 0.0f, 1.0f ); // Z-up
	case ViewportType::Top:
		return Vec3<>( 0.0f, 1.0f, 0.0f ); // Y-forward when looking down
	case ViewportType::Front:
		return Vec3<>( 0.0f, 0.0f, 1.0f ); // Z-up when looking from front
	case ViewportType::Side:
		return Vec3<>( 0.0f, 0.0f, 1.0f ); // Z-up when looking from side
	default:
		return Vec3<>( 0.0f, 0.0f, 1.0f );
	}
}

} // namespace editor::ViewportUtils
