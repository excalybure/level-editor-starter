#include "viewport.h"

// Viewport Management implementation for multi-viewport 3D editor
// Implements viewport instances with cameras, render targets, and input handling
#include <d3d12.h>
#include <wrl.h>
#include <cstring>
#include <memory>
#include <vector>
#include <array>

#include "engine/math/vec.h"
#include "engine/math/matrix.h"
#include "engine/camera/camera.h"
#include "engine/camera/camera_controller.h"
#include "platform/dx12/dx12_device.h"
#include "platform/pix/pix.h"
#include "engine/grid/grid.h"
#include "runtime/console.h"
#include "runtime/systems.h"
#include "runtime/mesh_rendering_system.h"
#include "editor/viewport_input.h"
#include "editor/selection_renderer.h"

namespace editor
{

// Frame constants structure matching unlit.hlsl shader expectations
struct FrameConstants
{
	math::Mat4<> viewMatrix;
	math::Mat4<> projMatrix;
	math::Mat4<> viewProjMatrix;
	math::Vec3f cameraPosition;
	float padding0 = 0.0f;

	FrameConstants() = default;
};

//=============================================================================
// Viewport Implementation
//=============================================================================

Viewport::Viewport( ViewportType type )
	: m_type( type )
{
	initializeCamera();

	// Initialize input state
	m_currentInput = {};
	m_currentInput.deltaTime = 0.0f;
}

Viewport::~Viewport() = default;

float Viewport::getAspectRatio() const noexcept
{
	// Use pending size if a resize is pending, otherwise use current size
	const auto &size = m_resizePending ? m_pendingSize : m_size;
	if ( size.y <= 0 )
		return 1.0f;
	return static_cast<float>( size.x ) / static_cast<float>( size.y );
}

void Viewport::setRenderTargetSize( int width, int height )
{
	// Store the pending size instead of applying immediately
	// This avoids deleting resources while command lists are being built
	m_pendingSize = { width, height };
	m_resizePending = true;
}

void Viewport::applyPendingResize( dx12::Device *device )
{
	if ( !m_resizePending )
		return;

	m_size = m_pendingSize;
	m_resizePending = false;

	// Resize existing render target if it exists
	if ( m_renderTarget && device )
	{
		// Note: resize may recreate the texture
		m_renderTarget->resize( device, m_pendingSize.x, m_pendingSize.y );
	}

	// Camera aspect ratio is handled in GetProjectionMatrix calls
}

math::Vec2<int> Viewport::getSize() const noexcept
{
	// Return pending size if a resize is pending, otherwise return current size
	return m_resizePending ? m_pendingSize : m_size;
}

bool Viewport::createRenderTarget( dx12::Device *device, int width, int height )
{
	if ( !device )
		return false;

	// Create render target texture through device texture manager
	m_renderTarget = device->getTextureManager()->createViewportRenderTarget( width, height );
	if ( !m_renderTarget )
		return false;

	// Update size and handle
	m_size = { static_cast<int>( width ), static_cast<int>( height ) };
	m_renderTargetHandle = m_renderTarget->getImGuiTextureId();

	return true;
}

bool Viewport::clearRenderTarget( dx12::Device *device, const float clearColor[4] )
{
	if ( !m_renderTarget || !device )
		return false;

	return m_renderTarget->clearRenderTarget( device, clearColor );
}

void *Viewport::getImGuiTextureId() const noexcept
{
	return m_renderTarget ? m_renderTarget->getImGuiTextureId() : nullptr;
}

bool Viewport::createFrameConstantBuffer( dx12::Device *device )
{
	if ( !device || !device->get() )
	{
		return false;
	}

	// Create an upload heap for the frame constant buffer
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = 0;
	desc.Width = ( sizeof( FrameConstants ) + 255 ) & ~255; // Align to 256 bytes for constant buffer
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	HRESULT hr = device->get()->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS( &m_frameConstantBuffer ) );

	if ( FAILED( hr ) )
	{
		m_frameConstantBuffer = nullptr;
		m_frameConstantBufferData = nullptr;
		return false;
	}

	// Map the constant buffer so we can update it
	D3D12_RANGE readRange = { 0, 0 }; // We don't intend to read from this resource
	hr = m_frameConstantBuffer->Map( 0, &readRange, reinterpret_cast<void **>( &m_frameConstantBufferData ) );

	if ( FAILED( hr ) )
	{
		m_frameConstantBuffer = nullptr;
		m_frameConstantBufferData = nullptr;
		return false;
	}

	return true;
}

void Viewport::updateFrameConstants()
{
	if ( !m_frameConstantBufferData || !m_camera )
	{
		return;
	}

	FrameConstants frameConstants;
	frameConstants.viewMatrix = m_camera->getViewMatrix().transpose(); // HLSL expects column-major matrices, so transpose
	frameConstants.projMatrix = m_camera->getProjectionMatrix( getAspectRatio() ).transpose();
	// Because matrices are transposed, we multiply left to right: (A * B)ᵀ = Bᵀ * Aᵀ
	frameConstants.viewProjMatrix = frameConstants.viewMatrix * frameConstants.projMatrix;
	frameConstants.cameraPosition = m_camera->getPosition();

	memcpy( m_frameConstantBufferData, &frameConstants, sizeof( FrameConstants ) );
}

void Viewport::bindFrameConstants( ID3D12GraphicsCommandList *commandList ) const
{
	if ( m_frameConstantBuffer && commandList )
	{
		commandList->SetGraphicsRootConstantBufferView( 0, m_frameConstantBuffer->GetGPUVirtualAddress() );
	}
}

void Viewport::update( float deltaTime )
{
	if ( !m_controller || !m_camera )
		return;

	// Update input state with timing
	m_currentInput.deltaTime = deltaTime;

	// Update camera through controller
	m_controller->setCamera( m_camera.get() );
	m_controller->update( m_currentInput );

	// Clear one-shot input values after processing
	m_currentInput.mouse.wheelDelta = 0.0f;
}

void Viewport::render( dx12::Device *device )
{
	if ( !m_camera || !m_renderTarget || !device )
		return;

	// PIX marker for the entire viewport render
	ID3D12GraphicsCommandList *commandList = device->getCommandList();
	pix::ScopedEvent pixViewportRender( commandList, pix::MarkerColor::Cyan, std::format( "Viewport Render {}x{}", m_size.x, m_size.y ) );

	// Clear the render target with a nice dark gray color
	const float clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
	{
		pix::ScopedEvent pixClear( commandList, pix::MarkerColor::Red, "Clear Render Target" );
		if ( !clearRenderTarget( device, clearColor ) )
		{
			pix::SetMarker( commandList, pix::MarkerColor::Yellow, "Clear Failed" );
			return;
		}
	}

	// Render grid if enabled and available
	if ( m_showGrid && m_gridRenderer )
	{
		pix::ScopedEvent pixGrid( commandList, pix::MarkerColor::Green, "Grid Rendering" );

		// Get camera matrices with proper aspect ratio
		const auto viewMatrix = m_camera->getViewMatrix();
		const auto projMatrix = m_camera->getProjectionMatrix( getAspectRatio() );

		// Render the grid for this viewport
		const float viewportWidth = static_cast<float>( m_size.x );
		const float viewportHeight = static_cast<float>( m_size.y );

		if ( !m_gridRenderer->render( *m_camera, viewMatrix, projMatrix, viewportWidth, viewportHeight ) )
		{
			console::warning( "Grid rendering failed for viewport" );
			pix::SetMarker( commandList, pix::MarkerColor::Yellow, "Grid Render Failed" );
		}
	}
	else
	{
		pix::SetMarker( commandList, pix::MarkerColor::Orange, m_showGrid ? "Grid Renderer Missing" : "Grid Disabled" );
	}

	// Render selection outlines and highlights if available
	if ( m_selectionRenderer && m_scene )
	{
		pix::ScopedEvent pixSelection( commandList, pix::MarkerColor::Purple, "Selection Rendering" );

		// Get camera matrices
		const auto viewMatrix = m_camera->getViewMatrix();
		const auto projMatrix = m_camera->getProjectionMatrix( getAspectRatio() );
		const math::Vec2<> viewportSize{ static_cast<float>( m_size.x ), static_cast<float>( m_size.y ) };

		// Render selection visual feedback
		m_selectionRenderer->render( *m_scene, commandList, viewMatrix, projMatrix, viewportSize );
	}
}

void Viewport::setupInputHandler( editor::SelectionManager *selectionManager, picking::PickingSystem *pickingSystem, systems::SystemManager *systemManager )
{
	if ( selectionManager && pickingSystem && systemManager )
	{
		m_inputHandler = std::make_unique<editor::ViewportInputHandler>( *selectionManager, *pickingSystem, *systemManager );
	}
}

void Viewport::setupSelectionRenderer( dx12::Device *device, std::shared_ptr<shader_manager::ShaderManager> shaderManager )
{
	if ( device && shaderManager )
	{
		m_selectionRenderer = std::make_unique<editor::SelectionRenderer>( *device, *shaderManager );
		console::info( "SelectionRenderer created for viewport visual feedback" );
	}
	else
	{
		console::warning( "Cannot setup selection renderer: missing device or shader manager" );
	}
}

void Viewport::handleInput( const ViewportInputEvent &event )
{
	if ( !m_isFocused || !m_controller )
		return;

	updateInputState( event );

	// Try selection input first (for left mouse button and selection operations)
	bool selectionHandled = handleSelectionInput( event );

	// If selection didn't handle it (e.g., right click, keyboard), handle camera controls
	if ( !selectionHandled )
	{
		handleCameraInput( event );
	}
}

bool Viewport::handleSelectionInput( const ViewportInputEvent &event )
{
	if ( !m_inputHandler || !m_scene )
		return false;

	switch ( event.type )
	{
	case ViewportInputEvent::Type::MouseButton:
		if ( event.mouse.button == 0 ) // Left mouse button for selection
		{
			if ( event.mouse.pressed )
			{
				// Store mouse position for potential drag operation
				m_lastMousePos = { event.mouse.x, event.mouse.y };
				m_mouseTracking = true;

				// Handle mouse click for object selection
				const math::Vec2f screenPos{ event.mouse.x, event.mouse.y };
				m_inputHandler->handleMouseClick( *m_scene, *this, screenPos, true, false, // leftButton, rightButton
					m_currentInput.keyboard.ctrl,
					m_currentInput.keyboard.shift );
			}
			else
			{
				// Handle mouse release
				if ( m_mouseTracking )
				{
					math::Vec2f releasePos{ event.mouse.x, event.mouse.y };
					m_inputHandler->handleMouseRelease( *m_scene, *this, releasePos );
					m_mouseTracking = false;
				}
			}
			return true; // Left click events are always handled by selection
		}
		break;

	case ViewportInputEvent::Type::MouseMove: {
		const math::Vec2f currentPos{ event.mouse.x, event.mouse.y };

		// Handle mouse drag if we're tracking
		if ( m_mouseTracking )
		{
			m_inputHandler->handleMouseDrag( *m_scene, *this, m_lastMousePos, currentPos, m_currentInput.keyboard.ctrl, m_currentInput.keyboard.shift );
		}
		else
		{
			// Handle hover detection
			m_inputHandler->handleMouseMove( *m_scene, *this, currentPos );
		}
		return false; // Allow camera to also handle mouse move for camera controls
	}
	break;

	default:
		break;
	}

	return false; // Event not handled by selection
}

void Viewport::handleCameraInput( const ViewportInputEvent &event )
{
	// Handle view-specific operations and camera controls
	switch ( event.type )
	{
	case ViewportInputEvent::Type::MouseButton:
		if ( event.mouse.button == 1 && event.mouse.pressed )
		{	// Right click
			// Context menu or specific view operations could go here
		}
		break;

	case ViewportInputEvent::Type::KeyPress:
		// Handle view-specific keyboard shortcuts
		switch ( event.keyboard.keyCode )
		{
		case 'F': // Frame all
			if ( !event.keyboard.shift )
			{
				frameAll();
			}
			break;
		case 'R': // Reset view
			if ( event.keyboard.ctrl )
			{
				resetView();
			}
			break;
		}
		break;

	default:
		break;
	}
}

ViewportRay Viewport::getPickingRay( const math::Vec2<> &screenPos ) const noexcept
{
	if ( !m_camera )
	{
		return { { 0, 0, 0 }, { 0, 0, -1 }, 1000.0f };
	}

	// Convert screen coordinates to normalized device coordinates
	const math::Vec2<> ndc = ViewportUtils::pixelToNormalized( screenPos, m_size );

	// Get camera matrices
	const auto viewMatrix = m_camera->getViewMatrix();
	const auto projMatrix = m_camera->getProjectionMatrix( getAspectRatio() );

	// Convert NDC to camera ray
	math::Vec3<> rayOrigin, rayDirection;

	if ( m_camera->getViewType() == camera::ViewType::Perspective )
	{
		// Perspective projection - ray starts from camera position
		rayOrigin = m_camera->getPosition();

		// Calculate ray direction through screen point
		// Inverse transform from NDC back to world space
		const auto invViewProj = ( projMatrix * viewMatrix ).inverse();

		// Point on near plane in NDC
		const math::Vec4<> nearPoint{ ndc.x, ndc.y, -1.0f, 1.0f };
		// Point on far plane in NDC
		const math::Vec4<> farPoint{ ndc.x, ndc.y, 1.0f, 1.0f };

		// Transform to world space
		auto worldNear = invViewProj * nearPoint;
		auto worldFar = invViewProj * farPoint;

		// Perspective divide
		if ( worldNear.w != 0.0f )
			worldNear = worldNear * ( 1.0f / worldNear.w );
		if ( worldFar.w != 0.0f )
			worldFar = worldFar * ( 1.0f / worldFar.w );

		rayDirection = math::normalize( worldFar.xyz() - worldNear.xyz() );
	}
	else
	{
		// Orthographic projection - ray is parallel to view direction
		rayDirection = -m_camera->getForwardVector(); // Camera looks down -Z, so forward is camera's -Z

		// Ray origin is on the near plane at the screen position
		const auto invViewProj = ( projMatrix * viewMatrix ).inverse();

		const math::Vec4<> nearPoint{ ndc.x, ndc.y, -1.0f, 1.0f };
		auto worldPoint = invViewProj * nearPoint;
		if ( worldPoint.w != 0.0f )
			worldPoint = worldPoint * ( 1.0f / worldPoint.w );

		rayOrigin = worldPoint.xyz();
	}

	return { rayOrigin, rayDirection, 1000.0f };
}

math::Vec3<> Viewport::screenToWorld( const math::Vec2<> &screenPos, float depth ) const noexcept
{
	if ( !m_camera )
		return { 0, 0, 0 };

	// Get picking ray and extend to specified depth
	const auto ray = getPickingRay( screenPos );
	return ray.origin + ray.direction * depth;
}

math::Vec2<> Viewport::worldToScreen( const math::Vec3<> &worldPos ) const noexcept
{
	if ( !m_camera )
		return { 0, 0 };

	// Transform world position to screen coordinates
	const auto viewMatrix = m_camera->getViewMatrix();
	const auto projMatrix = m_camera->getProjectionMatrix( getAspectRatio() );
	const auto viewProj = projMatrix * viewMatrix;

	math::Vec4<> clipPos = viewProj * math::Vec4<>{ worldPos.x, worldPos.y, worldPos.z, 1.0f };

	// Perspective divide
	if ( clipPos.w != 0.0f )
	{
		clipPos = clipPos * ( 1.0f / clipPos.w );
	}

	// Convert NDC to screen coordinates
	return ViewportUtils::normalizedToPixel( clipPos.xy(), m_size );
}

void Viewport::frameAll() noexcept
{
	if ( !m_controller )
		return;

	// For now, reset to a default framing
	// In a full editor, this would frame all objects in the scene
	const math::Vec3<> center{ 0, 0, 0 };
	const math::Vec3<> size{ 20, 20, 20 }; // Default scene bounds

	frameSelection( center, size );
}

void Viewport::frameSelection( const math::Vec3<> &center, const math::Vec3<> &size ) noexcept
{
	if ( !m_controller || !m_camera )
		return;

	if ( m_camera->getViewType() == camera::ViewType::Perspective )
	{
		// For perspective, position camera at appropriate distance
		auto *perspController = dynamic_cast<camera::PerspectiveCameraController *>( m_controller.get() );
		if ( perspController )
		{
			perspController->focusOnBounds( center, size );
		}
	}
	else
	{
		// For orthographic, set position and zoom level
		auto *orthoController = dynamic_cast<camera::OrthographicCameraController *>( m_controller.get() );
		if ( orthoController )
		{
			orthoController->frameBounds( center, size );
		}
	}
}

void Viewport::resetView() noexcept
{
	if ( !m_camera )
		return;

	// Reset to default view based on viewport type
	switch ( m_type )
	{
	case ViewportType::Perspective:
		m_camera->setPosition( { 5, 5, 5 } );
		m_camera->setTarget( { 0, 0, 0 } );
		break;

	case ViewportType::Top: // Looking down Z-axis
		m_camera->setPosition( { 0, 0, 10 } );
		m_camera->setTarget( { 0, 0, 0 } );
		break;

	case ViewportType::Front: // Looking down Y-axis
		m_camera->setPosition( { 0, 10, 0 } );
		m_camera->setTarget( { 0, 0, 0 } );
		break;

	case ViewportType::Side: // Looking down X-axis
		m_camera->setPosition( { 10, 0, 0 } );
		m_camera->setTarget( { 0, 0, 0 } );
		break;
	}

	// Reset controller state
	if ( m_controller )
	{
		// Controllers will pick up the new camera state on next update
	}
}

camera::InputState Viewport::convertToInputState( const ViewportInputEvent &event ) const
{
	camera::InputState state = m_currentInput; // Copy current state

	// Update based on event type
	switch ( event.type )
	{
	case ViewportInputEvent::Type::MouseMove:
		state.mouse.x = event.mouse.x;
		state.mouse.y = event.mouse.y;
		state.mouse.deltaX = event.mouse.deltaX;
		state.mouse.deltaY = event.mouse.deltaY;
		break;

	case ViewportInputEvent::Type::MouseButton:
		state.mouse.x = event.mouse.x;
		state.mouse.y = event.mouse.y;
		state.mouse.leftButton = ( event.mouse.button == 0 ) ? event.mouse.pressed : state.mouse.leftButton;
		state.mouse.rightButton = ( event.mouse.button == 1 ) ? event.mouse.pressed : state.mouse.rightButton;
		state.mouse.middleButton = ( event.mouse.button == 2 ) ? event.mouse.pressed : state.mouse.middleButton;
		break;

	case ViewportInputEvent::Type::MouseWheel:
		state.mouse.x = event.mouse.x;
		state.mouse.y = event.mouse.y;
		state.mouse.wheelDelta = event.mouse.wheelDelta;
		break;

	case ViewportInputEvent::Type::KeyPress:
	case ViewportInputEvent::Type::KeyRelease: {
		const bool pressed = ( event.type == ViewportInputEvent::Type::KeyPress );
		state.keyboard.shift = event.keyboard.shift;
		state.keyboard.ctrl = event.keyboard.ctrl;
		state.keyboard.alt = event.keyboard.alt;

		// Map specific keys
		switch ( event.keyboard.keyCode )
		{
		case 'W':
			state.keyboard.w = pressed;
			break;
		case 'A':
			state.keyboard.a = pressed;
			break;
		case 'S':
			state.keyboard.s = pressed;
			break;
		case 'D':
			state.keyboard.d = pressed;
			break;
		case 'Q':
			state.keyboard.q = pressed;
			break;
		case 'E':
			state.keyboard.e = pressed;
			break;
		case 'F':
			state.keyboard.f = pressed;
			break;
		}
	}
	break;

	default:
		break;
	}

	return state;
}

void Viewport::updateInputState( const ViewportInputEvent &event )
{
	m_currentInput = convertToInputState( event );
}

void Viewport::initializeCamera()
{
	// Create appropriate camera and controller based on viewport type
	switch ( m_type )
	{
	case ViewportType::Perspective: {
		m_camera = std::make_unique<camera::PerspectiveCamera>();
		m_controller = std::make_unique<camera::PerspectiveCameraController>( static_cast<camera::PerspectiveCamera *>( m_camera.get() ) );
		m_camera->setPosition( { 5, 5, 5 } );
		m_camera->setTarget( { 0, 0, 0 } );
		m_camera->setUp( { 0, 0, 1 } ); // Z-up coordinate system for perspective
	}
	break;

	case ViewportType::Top:
	case ViewportType::Front:
	case ViewportType::Side: {
		m_camera = std::make_unique<camera::OrthographicCamera>( ViewportUtils::getCameraViewType( m_type ) );
		m_controller = std::make_unique<camera::OrthographicCameraController>( static_cast<camera::OrthographicCamera *>( m_camera.get() ) );
		setupOrthographicView(); // Sets up per-view up vectors
	}
	break;
	}
}

void Viewport::setupOrthographicView()
{
	if ( !m_camera )
		return;

	// Set up camera position and orientation for orthographic views
	switch ( m_type )
	{
	case ViewportType::Top: // Looking down Z-axis (XY plane)
		m_camera->setPosition( { 0, 0, 10 } );
		m_camera->setTarget( { 0, 0, 0 } );
		m_camera->setUp( { 0, 1, 0 } ); // Y is up in top view
		break;

	case ViewportType::Front: // Looking down Y-axis (XZ plane)
		m_camera->setPosition( { 0, 10, 0 } );
		m_camera->setTarget( { 0, 0, 0 } );
		m_camera->setUp( { 0, 0, 1 } ); // Z is up in front view
		break;

	case ViewportType::Side: // Looking down X-axis (YZ plane)
		m_camera->setPosition( { 10, 0, 0 } );
		m_camera->setTarget( { 0, 0, 0 } );
		m_camera->setUp( { 0, 0, 1 } ); // Z is up in side view
		break;

	default:
		break;
	}
}

//=============================================================================
// ViewportManager Implementation
//=============================================================================
// ViewportManager Implementation
//=============================================================================

bool ViewportManager::initialize( dx12::Device *device, std::shared_ptr<shader_manager::ShaderManager> shaderManager )
{
	if ( !device || !shaderManager )
		return false;

	m_device = device;
	m_shaderManager = shaderManager;
	return true;
}

void ViewportManager::shutdown()
{
	// Destroy all viewports first (this will trigger proper cleanup)
	destroyAllViewports();
	m_device = nullptr;
	m_shaderManager.reset(); // Properly release the shared_ptr
}

void ViewportManager::setSceneAndSystems( ecs::Scene *scene, systems::SystemManager *systemManager, editor::SelectionManager *selectionManager, picking::PickingSystem *pickingSystem )
{
	m_scene = scene;
	m_systemManager = systemManager;
	m_selectionManager = selectionManager;
	m_pickingSystem = pickingSystem;
}

Viewport *ViewportManager::createViewport( ViewportType type )
{
	if ( !m_device )
		return nullptr;

	auto viewport = std::make_unique<Viewport>( type );
	auto *ptr = viewport.get();

	// Create D3D12 render target with default size
	if ( !ptr->createRenderTarget( m_device, 800, 600 ) )
	{
		return nullptr; // Failed to create render target
	}

	// Create frame constant buffer for this viewport
	if ( !ptr->createFrameConstantBuffer( m_device ) )
	{
		console::warning( "Failed to create frame constant buffer for viewport" );
		return nullptr;
	}

	// Initialize grid renderer for this viewport
	if ( !ptr->initializeGrid( m_device, m_shaderManager ) )
	{
		console::warning( "Failed to initialize grid for viewport, grid rendering will not be available" );
	}

	// Setup input handler for object selection if dependencies are available
	if ( m_selectionManager && m_pickingSystem && m_systemManager )
	{
		ptr->setupInputHandler( m_selectionManager, m_pickingSystem, m_systemManager );
	}

	// Setup selection renderer for visual feedback
	ptr->setupSelectionRenderer( m_device, m_shaderManager );

	// Set scene reference for object selection
	if ( m_scene )
	{
		ptr->setScene( m_scene );
	}

	m_viewports.push_back( std::move( viewport ) );

	// If this is the first viewport, make it active
	if ( m_viewports.size() == 1 )
	{
		setActiveViewport( ptr );
		setFocusedViewport( ptr );
	}

	return ptr;
}

void ViewportManager::destroyViewport( Viewport *viewport )
{
	auto it = findViewport( viewport );
	if ( it != m_viewports.end() )
	{
		// Clear references if this viewport is active/focused
		if ( m_activeViewport == viewport )
		{
			m_activeViewport = nullptr;
		}
		if ( m_focusedViewport == viewport )
		{
			m_focusedViewport = nullptr;
		}

		m_viewports.erase( it );

		// Set new active viewport if we have any left
		if ( !m_viewports.empty() && !m_activeViewport )
		{
			setActiveViewport( m_viewports[0].get() );
			setFocusedViewport( m_viewports[0].get() );
		}
	}
}

void ViewportManager::destroyAllViewports()
{
	m_activeViewport = nullptr;
	m_focusedViewport = nullptr;
	m_viewports.clear(); // shared_ptr will ensure proper cleanup order
}

void ViewportManager::setActiveViewport( Viewport *viewport ) noexcept
{
	if ( m_activeViewport )
	{
		m_activeViewport->setActive( false );
	}

	m_activeViewport = viewport;

	if ( m_activeViewport )
	{
		m_activeViewport->setActive( true );
	}
}

void ViewportManager::setFocusedViewport( Viewport *viewport ) noexcept
{
	if ( m_focusedViewport )
	{
		m_focusedViewport->setFocused( false );
	}

	m_focusedViewport = viewport;

	if ( m_focusedViewport )
	{
		m_focusedViewport->setFocused( true );
	}
}

void ViewportManager::update( float deltaTime )
{
	for ( auto &viewport : m_viewports )
	{
		viewport->update( deltaTime );
	}
}

void ViewportManager::render()
{
	if ( !m_device )
		return;

	// PIX marker for the entire viewport manager render
	ID3D12GraphicsCommandList *commandList = m_device->getCommandList();
	pix::ScopedEvent pixManagerRender( commandList, pix::MarkerColor::Purple, "ViewportManager Render" );

	// Apply any pending resizes before rendering to avoid resource conflicts
	{
		pix::ScopedEvent pixResize( commandList, pix::MarkerColor::Yellow, "Apply Pending Resizes" );
		for ( auto &viewport : m_viewports )
		{
			viewport->applyPendingResize( m_device );
		}
	}

	// Render all active viewports
	int activeViewports = 0;
	for ( auto &viewport : m_viewports )
	{
		if ( viewport->isActive() )
		{
			activeViewports++;
			const char *viewportName = ViewportUtils::getViewportTypeName( viewport->getType() );
			pix::ScopedEvent pixIndividualViewport( commandList, pix::MarkerColor::LightBlue, std::format( "Viewport {} Render", viewportName ) );

			// Render the viewport (clears render target and renders grid)
			viewport->render( m_device );

			// Render 3D scene content if we have scene and systems
			if ( m_scene && m_systemManager && viewport->getCamera() )
			{
				// Get the MeshRenderingSystem and call its render method
				if ( auto *meshRenderingSystem = m_systemManager->getSystem<systems::MeshRenderingSystem>() )
				{
					pix::ScopedEvent pixSceneContent( commandList, pix::MarkerColor::Orange, "Scene Content Rendering" );

					// Set root signature FIRST before binding any parameters
					{
						pix::ScopedEvent pixRootSig( commandList, pix::MarkerColor::Red, "Root Signature Setup" );
						meshRenderingSystem->setRootSignature( commandList );
					}

					// Update and bind frame constants for this viewport
					{
						pix::ScopedEvent pixFrameConstants( commandList, pix::MarkerColor::Blue, "Frame Constants" );
						viewport->updateFrameConstants();
						viewport->bindFrameConstants( commandList );
					}

					meshRenderingSystem->render( *m_scene, *viewport->getCamera() );
				}
			}
		}
	}

	pix::SetMarker( commandList, pix::MarkerColor::White, std::format( "ViewportManager Complete - {} active viewports", activeViewports ) );
}

void ViewportManager::handleGlobalInput( const ViewportInputEvent &event )
{
	// Route input to focused viewport
	if ( m_focusedViewport )
	{
		m_focusedViewport->handleInput( event );
	}
}

void ViewportManager::synchronizeViews( Viewport *sourceViewport )
{
	if ( !sourceViewport || !sourceViewport->isViewSyncEnabled() )
		return;

	auto *sourceCamera = sourceViewport->getCamera();
	if ( !sourceCamera )
		return;

	// Synchronize other viewports with sync enabled
	for ( auto &viewport : m_viewports )
	{
		if ( viewport.get() != sourceViewport && viewport->isViewSyncEnabled() )
		{
			auto *camera = viewport->getCamera();
			if ( camera )
			{
				// For view sync, typically we'd sync the target position
				// but keep each view's specific orientation
				camera->setTarget( sourceCamera->getTarget() );
			}
		}
	}
}

void ViewportManager::frameAllInAllViewports() noexcept
{
	for ( auto &viewport : m_viewports )
	{
		viewport->frameAll();
	}
}

void ViewportManager::resetAllViews() noexcept
{
	for ( auto &viewport : m_viewports )
	{
		viewport->resetView();
	}
}

void ViewportManager::setGlobalGridVisible( bool visible ) noexcept
{
	for ( auto &viewport : m_viewports )
	{
		viewport->setGridVisible( visible );
	}
}

void ViewportManager::setGlobalGizmosVisible( bool visible ) noexcept
{
	for ( auto &viewport : m_viewports )
	{
		viewport->setGizmosVisible( visible );
	}
}

auto ViewportManager::findViewport( Viewport *viewport ) -> decltype( m_viewports.begin() )
{
	return std::find_if( m_viewports.begin(), m_viewports.end(), [viewport]( const auto &vp ) { return vp.get() == viewport; } );
}

//=============================================================================
// ViewportFactory Implementation
//=============================================================================

ViewportFactory::StandardLayout ViewportFactory::createStandardLayout( ViewportManager &manager )
{
	StandardLayout layout;

	layout.perspective = manager.createViewport( ViewportType::Perspective );
	layout.top = manager.createViewport( ViewportType::Top );
	layout.front = manager.createViewport( ViewportType::Front );
	layout.side = manager.createViewport( ViewportType::Side );

	// Set perspective as active by default
	manager.setActiveViewport( layout.perspective );
	manager.setFocusedViewport( layout.perspective );

	return layout;
}

Viewport *ViewportFactory::createSingleViewport( ViewportManager &manager, ViewportType type )
{
	return manager.createViewport( type );
}

//=============================================================================
// ViewportUtils Implementation
//=============================================================================

math::Vec2<> ViewportUtils::normalizedToPixel( const math::Vec2<> &normalized, const math::Vec2<int> &size ) noexcept
{
	return {
		( normalized.x * 0.5f + 0.5f ) * static_cast<float>( size.x ),
		( 1.0f - ( normalized.y * 0.5f + 0.5f ) ) * static_cast<float>( size.y ) // Flip Y
	};
}

math::Vec2<> ViewportUtils::pixelToNormalized( const math::Vec2<> &pixel, const math::Vec2<int> &size ) noexcept
{
	if ( size.x <= 0 || size.y <= 0 )
		return { 0, 0 };

	return {
		( pixel.x / static_cast<float>( size.x ) ) * 2.0f - 1.0f,
		-( ( pixel.y / static_cast<float>( size.y ) ) * 2.0f - 1.0f ) // Flip Y
	};
}

const char *ViewportUtils::getViewportTypeName( ViewportType type ) noexcept
{
	switch ( type )
	{
	case ViewportType::Perspective:
		return "Perspective";
	case ViewportType::Top:
		return "Top";
	case ViewportType::Front:
		return "Front";
	case ViewportType::Side:
		return "Side";
	default:
		return "Unknown";
	}
}

bool ViewportUtils::isOrthographicType( ViewportType type ) noexcept
{
	return type != ViewportType::Perspective;
}

camera::ViewType ViewportUtils::getCameraViewType( ViewportType viewportType ) noexcept
{
	switch ( viewportType )
	{
	case ViewportType::Top:
		return camera::ViewType::Top;
	case ViewportType::Front:
		return camera::ViewType::Front;
	case ViewportType::Side:
		return camera::ViewType::Side;
	case ViewportType::Perspective:
	default:
		return camera::ViewType::Perspective;
	}
}

ViewportUtils::ViewportLayout ViewportUtils::calculateStandardLayout( const math::Vec2<int> &totalSize ) noexcept
{
	ViewportLayout layout;

	// Split into 2x2 grid
	const int halfWidth = totalSize.x / 2;
	const int halfHeight = totalSize.y / 2;

	// Top-left: Perspective
	layout.perspectivePos = { 0, 0 };
	layout.perspectiveSize = { halfWidth, halfHeight };

	// Top-right: Top view
	layout.topPos = { halfWidth, 0 };
	layout.topSize = { halfWidth, halfHeight };

	// Bottom-left: Front view
	layout.frontPos = { 0, halfHeight };
	layout.frontSize = { halfWidth, halfHeight };

	// Bottom-right: Side view
	layout.sidePos = { halfWidth, halfHeight };
	layout.sideSize = { halfWidth, halfHeight };

	return layout;
}

ViewportInputEvent ViewportUtils::createMouseMoveEvent( float x, float y, float deltaX, float deltaY ) noexcept
{
	ViewportInputEvent event;
	event.type = ViewportInputEvent::Type::MouseMove;
	event.mouse.x = x;
	event.mouse.y = y;
	event.mouse.deltaX = deltaX;
	event.mouse.deltaY = deltaY;
	return event;
}

ViewportInputEvent ViewportUtils::createMouseButtonEvent( int button, bool pressed, float x, float y ) noexcept
{
	ViewportInputEvent event;
	event.type = ViewportInputEvent::Type::MouseButton;
	event.mouse.button = button;
	event.mouse.pressed = pressed;
	event.mouse.x = x;
	event.mouse.y = y;
	return event;
}

ViewportInputEvent ViewportUtils::createMouseWheelEvent( float delta, float x, float y ) noexcept
{
	ViewportInputEvent event;
	event.type = ViewportInputEvent::Type::MouseWheel;
	event.mouse.wheelDelta = delta;
	event.mouse.x = x;
	event.mouse.y = y;
	return event;
}

ViewportInputEvent ViewportUtils::createKeyEvent( int keyCode, bool pressed, bool shift, bool ctrl, bool alt ) noexcept
{
	ViewportInputEvent event;
	event.type = pressed ? ViewportInputEvent::Type::KeyPress : ViewportInputEvent::Type::KeyRelease;
	event.keyboard.keyCode = keyCode;
	event.keyboard.shift = shift;
	event.keyboard.ctrl = ctrl;
	event.keyboard.alt = alt;
	return event;
}

// Grid integration methods for Viewport
bool Viewport::initializeGrid( dx12::Device *device, std::shared_ptr<shader_manager::ShaderManager> shaderManager )
{
	if ( !m_gridRenderer )
	{
		m_gridRenderer = std::make_unique<grid::GridRenderer>();
	}

	if ( !m_gridRenderer->initialize( device, shaderManager ) )
	{
		console::error( "Failed to initialize grid renderer for viewport" );
		m_gridRenderer.reset();
		return false;
	}

	return true;
}

void Viewport::setGridSettings( const grid::GridSettings &settings )
{
	m_gridSettings = settings;
	if ( m_gridRenderer )
	{
		m_gridRenderer->setSettings( settings );
	}
}

const grid::GridSettings &Viewport::getGridSettings() const
{
	return m_gridSettings;
}

ViewportInputEvent ViewportUtils::createResizeEvent( int width, int height ) noexcept
{
	ViewportInputEvent event;
	event.type = ViewportInputEvent::Type::Resize;
	event.resize.width = width;
	event.resize.height = height;
	return event;
}

} // namespace editor
