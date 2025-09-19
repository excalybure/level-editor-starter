// Grid rendering system implementation
// Copyright (c) 2025 Level Editor Project

module; // global module fragment

#include <cstring>
#include <d3d12.h>
#include <wrl.h>

module engine.grid;

import std;
import platform.dx12;
import engine.renderer;
import engine.vec;
import engine.matrix;
import engine.camera;
import engine.color;
import runtime.console;

using namespace grid;
using namespace Microsoft::WRL;

namespace
{

// Constant buffer structure for grid shader
struct GridConstants
{
	math::Mat4<> viewMatrix;
	math::Mat4<> projMatrix;
	math::Mat4<> invViewProjMatrix;

	math::Vec3f cameraPosition;
	float gridScale;

	math::Vec3f majorGridColor;
	float majorGridAlpha;

	math::Vec3f minorGridColor;
	float minorGridAlpha;

	math::Vec3f axisXColor;
	float axisXAlpha;

	math::Vec3f axisYColor;
	float axisYAlpha;

	math::Vec3f axisZColor;
	float axisZAlpha;

	float fadeDistance;
	float gridSpacing;
	float majorGridInterval;
	float nearPlane;

	float farPlane;
	int showGrid;
	int showAxes;
	float axisThickness;
	int viewType;	  // 0=Perspective, 1=Top, 2=Front, 3=Side
	float padding[3]; // Ensure 16-byte alignment
};
} // namespace

// GridRenderer implementation
GridRenderer::GridRenderer()
{
	// Initialize default settings
	m_settings = GridSettings{};
}

GridRenderer::~GridRenderer()
{
	// Unregister the callback if we have one
	if ( m_shaderManager && m_callbackHandle != shader_manager::INVALID_CALLBACK_HANDLE )
	{
		m_shaderManager->unregisterReloadCallback( m_callbackHandle );
	}
}

bool GridRenderer::initialize( dx12::Device *device, std::shared_ptr<shader_manager::ShaderManager> shaderManager )
{
	if ( !device || !shaderManager )
	{
		return false;
	}

	m_device = device;
	m_shaderManager = shaderManager;

	// Register shaders with the shader manager
	if ( !registerShaders() )
	{
		return false;
	}

	// Create root signature
	if ( !createRootSignature() )
	{
		return false;
	}

	// Try to create pipeline state (may fail if shaders aren't ready yet)
	// The dirty flag will ensure it gets created later when shaders are available
	if ( !createPipelineState() )
	{
		console::warning( "Initial pipeline state creation failed, will retry when shaders are ready" );
		m_pipelineStateDirty = true;
	}
	else
	{
		m_pipelineStateDirty = false;
	}

	// Create constant buffer
	if ( !createConstantBuffer() )
	{
		return false;
	}

	return true;
}

void GridRenderer::shutdown()
{
	// Unregister the callback if we have one
	if ( m_shaderManager && m_callbackHandle != shader_manager::INVALID_CALLBACK_HANDLE )
	{
		m_shaderManager->unregisterReloadCallback( m_callbackHandle );
		m_callbackHandle = shader_manager::INVALID_CALLBACK_HANDLE;
	}

	if ( m_constantBufferData )
	{
		m_constantBuffer->Unmap( 0, nullptr );
		m_constantBufferData = nullptr;
	}

	m_constantBuffer.Reset();
	m_pipelineState.Reset();
	m_rootSignature.Reset();
	m_device = nullptr;
	m_shaderManager.reset(); // Properly release the shared_ptr
}

bool GridRenderer::render( const camera::Camera &camera,
	const math::Mat4<> &viewMatrix,
	const math::Mat4<> &projMatrix,
	float viewportWidth,
	float viewportHeight )
{
	if ( !m_device || !m_constantBuffer )
	{
		return false;
	}

	// Check if pipeline state needs recreation
	if ( m_pipelineStateDirty )
	{
		console::info( "Grid pipeline state is dirty, recreating..." );
		if ( !createPipelineState() )
		{
			console::error( "Failed to recreate grid pipeline state" );
			return false;
		}
		m_pipelineStateDirty = false;
	}

	if ( !m_pipelineState )
	{
		console::warning( "Grid pipeline state not available for rendering" );
		return false;
	}

	// Update adaptive spacing based on camera
	updateAdaptiveSpacing( camera );

	// Update constant buffer with current frame data
	updateConstantBuffer( camera, viewMatrix, projMatrix, viewportWidth, viewportHeight );

	// Get command list
	const auto commandList = m_device->getCommandList();
	if ( !commandList )
	{
		return false;
	}

	// Set D3D12 viewport to match render target dimensions
	D3D12_VIEWPORT viewport = {};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = viewportWidth;
	viewport.Height = viewportHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	commandList->RSSetViewports( 1, &viewport );

	// Set scissor rectangle to match viewport
	D3D12_RECT scissorRect = {};
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = static_cast<LONG>( viewportWidth );
	scissorRect.bottom = static_cast<LONG>( viewportHeight );
	commandList->RSSetScissorRects( 1, &scissorRect );

	// Set pipeline state
	commandList->SetPipelineState( m_pipelineState.Get() );
	commandList->SetGraphicsRootSignature( m_rootSignature.Get() );

	// Bind constant buffer
	commandList->SetGraphicsRootConstantBufferView( 0, m_constantBuffer->GetGPUVirtualAddress() );

	// Set primitive topology
	commandList->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	// Draw fullscreen triangle (3 vertices, no vertex buffer needed)
	commandList->DrawInstanced( 3, 1, 0, 0 );

	return true;
}

void GridRenderer::updateAdaptiveSpacing( const camera::Camera &camera )
{
	// Calculate optimal grid spacing based on camera distance
	const math::Vec3<> cameraPos = camera.getPosition();
	const float distanceToOrigin = math::length( cameraPos );

	// Use a more sophisticated calculation for adaptive spacing
	const float baseSpacing = m_settings.gridSpacing;
	float optimalSpacing = calculateOptimalSpacing( distanceToOrigin, baseSpacing );

	// Update settings if spacing has changed significantly
	if ( std::abs( m_settings.gridSpacing - optimalSpacing ) > m_settings.gridSpacing * 0.1f )
	{
		m_settings.gridSpacing = optimalSpacing;
		m_settings.majorGridInterval = static_cast<float>( calculateMajorInterval( optimalSpacing ) );
	}
}

float GridRenderer::calculateOptimalSpacing( const float cameraDistance, const float baseSpacing )
{
	// Handle edge cases
	if ( cameraDistance <= 0.0f )
	{
		return baseSpacing * 0.1f; // Fallback for invalid distance
	}

	const float logDistance = std::log10f( cameraDistance );
	const float magnitudeExponent = std::floorf( logDistance );
	const float magnitude = std::powf( 10.0f, magnitudeExponent );
	const float optimalSpacing = magnitude * 0.1f;

	return optimalSpacing;
}

int GridRenderer::calculateMajorInterval( const float spacing )
{
	// Major grid lines appear every 5 or 10 minor lines depending on spacing
	if ( spacing <= 0.1f )
	{
		return 10; // Every 10 lines for fine grids
	}
	else if ( spacing <= 1.0f )
	{
		return 5; // Every 5 lines for normal grids
	}
	else
	{
		return 10; // Every 10 lines for coarse grids
	}
}

bool GridRenderer::registerShaders()
{
	// Register vertex shader
	m_vertexShaderHandle = m_shaderManager->registerShader(
		"shaders/grid.hlsl",
		"VSMain",
		"vs_5_0",
		shader_manager::ShaderType::Vertex );

	if ( m_vertexShaderHandle == shader_manager::INVALID_SHADER_HANDLE )
	{
		console::error( "Failed to register vertex shader for grid" );
		return false;
	}

	// Register pixel shader
	m_pixelShaderHandle = m_shaderManager->registerShader(
		"shaders/grid.hlsl",
		"PSMain",
		"ps_5_0",
		shader_manager::ShaderType::Pixel );

	if ( m_pixelShaderHandle == shader_manager::INVALID_SHADER_HANDLE )
	{
		console::error( "Failed to register pixel shader for grid" );
		return false;
	}

	// Set up reload callback for shader hot reloading
	m_callbackHandle = m_shaderManager->registerReloadCallback(
		[this]( shader_manager::ShaderHandle handle, const renderer::ShaderBlob &newShader ) {
			this->onShaderReloaded( handle, newShader );
		} );

	if ( m_pixelShaderHandle == shader_manager::INVALID_SHADER_HANDLE )
	{
		console::error( "Failed to register pixel shader for grid" );
		return false;
	}

	return true;
}

void GridRenderer::onShaderReloaded( shader_manager::ShaderHandle handle, const renderer::ShaderBlob & /*newShader*/ )
{
	// Check if the reloaded shader is one of ours
	if ( handle == m_vertexShaderHandle || handle == m_pixelShaderHandle )
	{
		m_pipelineStateDirty = true;
	}
}

bool GridRenderer::createRootSignature()
{
	// Create root signature with one constant buffer parameter
	D3D12_ROOT_PARAMETER rootParameter = {};
	rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameter.Descriptor.ShaderRegister = 0;
	rootParameter.Descriptor.RegisterSpace = 0;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.NumParameters = 1;
	rootSignatureDesc.pParameters = &rootParameter;
	rootSignatureDesc.NumStaticSamplers = 0;
	rootSignatureDesc.pStaticSamplers = nullptr;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	HRESULT hr = D3D12SerializeRootSignature( &rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error );

	if ( FAILED( hr ) )
	{
		if ( error )
		{
			console::error( static_cast<const char *>( error->GetBufferPointer() ) );
		}
		return false;
	}

	hr = m_device->get()->CreateRootSignature(
		0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS( &m_rootSignature ) );

	return SUCCEEDED( hr );
}

bool GridRenderer::createPipelineState()
{
	// Get current shader blobs from shader manager
	const renderer::ShaderBlob *vertexShader = m_shaderManager->getShaderBlob( m_vertexShaderHandle );
	const renderer::ShaderBlob *pixelShader = m_shaderManager->getShaderBlob( m_pixelShaderHandle );

	if ( !vertexShader || !pixelShader || !vertexShader->isValid() || !pixelShader->isValid() )
	{
		console::warning( "Grid shaders not ready for pipeline state creation" );
		return false;
	}

	// Create pipeline state for grid rendering
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = { vertexShader->blob->GetBufferPointer(), vertexShader->blob->GetBufferSize() };
	psoDesc.PS = { pixelShader->blob->GetBufferPointer(), pixelShader->blob->GetBufferSize() };

	// Blend state for alpha blending
	psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
	psoDesc.BlendState.IndependentBlendEnable = FALSE;
	psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// Rasterizer state
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // No culling for fullscreen quad
	psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
	psoDesc.RasterizerState.DepthBias = 0;
	psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
	psoDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
	psoDesc.RasterizerState.DepthClipEnable = TRUE;
	psoDesc.RasterizerState.MultisampleEnable = FALSE;
	psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
	psoDesc.RasterizerState.ForcedSampleCount = 0;
	psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// Depth stencil state (disable depth testing for viewport rendering)
	psoDesc.DepthStencilState.DepthEnable = FALSE; // Disable depth testing
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	psoDesc.DepthStencilState.StencilEnable = FALSE;

	// Input layout (none needed for fullscreen triangle)
	psoDesc.InputLayout = { nullptr, 0 };

	// Primitive topology
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// Render target format
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN; // No depth buffer

	// Sample desc
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;

	psoDesc.SampleMask = UINT_MAX; // 0xffffffff - enable all samples

	HRESULT hr = m_device->get()->CreateGraphicsPipelineState( &psoDesc, IID_PPV_ARGS( &m_pipelineState ) );

	if ( SUCCEEDED( hr ) )
	{
		return true;
	}
	else
	{
		console::error( "Failed to create grid pipeline state" );
		return false;
	}
}

bool GridRenderer::createConstantBuffer()
{
	// Create constant buffer for grid parameters
	UINT bufferSize = sizeof( GridConstants );
	bufferSize = ( bufferSize + 255 ) & ~255; // Align to 256 bytes

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = bufferSize;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	HRESULT hr = m_device->get()->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS( &m_constantBuffer ) );

	if ( FAILED( hr ) )
	{
		return false;
	}

	// Map the constant buffer for writing
	const D3D12_RANGE readRange = { 0, 0 }; // We don't read from this buffer on CPU
	hr = m_constantBuffer->Map( 0, &readRange, &m_constantBufferData );

	return SUCCEEDED( hr );
}

void GridRenderer::updateConstantBuffer( const camera::Camera &camera,
	const math::Mat4<> &viewMatrix,
	const math::Mat4<> &projMatrix,
	float viewportWidth,
	float viewportHeight )
{
	if ( !m_constantBufferData )
	{
		return;
	}

	GridConstants constants = {};

	// Create a camera-relative view matrix by removing the translation component
	math::Mat4<> cameraRelativeViewMatrix{
		math::Vec4<>{ viewMatrix.m00(), viewMatrix.m01(), viewMatrix.m02(), 0.0f },
		math::Vec4<>{ viewMatrix.m10(), viewMatrix.m11(), viewMatrix.m12(), 0.0f },
		math::Vec4<>{ viewMatrix.m20(), viewMatrix.m21(), viewMatrix.m22(), 0.0f },
		math::Vec4<>{ viewMatrix.m30(), viewMatrix.m31(), viewMatrix.m32(), viewMatrix.m33() } // Clear translation, keep homogeneous scaling
	};

	// For forward transformation: HLSL needs transpose(M)
	// For inverse transformation: HLSL gets inverse(M) but interprets it
	// as transpose(inverse(M)) = inverse(transpose(M))
	// Since transpose(M) is what HLSL needs for the forward case,
	// inverse(transpose(M)) is the correct inverse for the HLSL coordinate system.
	// This explains why viewMatrix & projMatrix are transposed, but not invViewProjMatrix.

	// Transform matrices
	constants.viewMatrix = cameraRelativeViewMatrix.transpose(); // HLSL expects column-major matrices, so transpose
	constants.projMatrix = projMatrix.transpose();
	constants.invViewProjMatrix = calculateInverseViewProjMatrix( cameraRelativeViewMatrix, projMatrix );

	// Camera data
	constants.cameraPosition = camera.getPosition();
	constants.gridScale = 1.0f;
	constants.nearPlane = camera.getNearPlane();
	constants.farPlane = camera.getFarPlane();

	// Grid colors
	constants.majorGridColor = m_settings.majorGridColor;
	constants.majorGridAlpha = m_settings.majorGridAlpha;
	constants.minorGridColor = m_settings.minorGridColor;
	constants.minorGridAlpha = m_settings.minorGridAlpha;

	// Axis colors
	constants.axisXColor = m_settings.axisXColor;
	constants.axisXAlpha = m_settings.axisXAlpha;
	constants.axisYColor = m_settings.axisYColor;
	constants.axisYAlpha = m_settings.axisYAlpha;
	constants.axisZColor = m_settings.axisZColor;
	constants.axisZAlpha = m_settings.axisZAlpha;

	// Grid properties - calculate fade distance dynamically based on camera distance
	const float cameraDistance = math::length( camera.getPosition() );
	constants.fadeDistance = cameraDistance * m_settings.fadeDistanceMultiplier;
	constants.gridSpacing = m_settings.gridSpacing;
	constants.majorGridInterval = m_settings.majorGridInterval;
	constants.axisThickness = m_settings.axisThickness;

	// Visibility flags
	constants.showGrid = m_settings.showGrid ? 1 : 0;
	constants.showAxes = m_settings.showAxes ? 1 : 0;

	// Set view type for shader to distinguish between camera modes
	switch ( camera.getViewType() )
	{
	case camera::ViewType::Perspective:
		constants.viewType = 0; // Perspective view
		break;
	case camera::ViewType::Top:
		constants.viewType = 1; // Top orthographic view (XY plane)
		break;
	case camera::ViewType::Front:
		constants.viewType = 2; // Front orthographic view (XZ plane)
		break;
	case camera::ViewType::Side:
		constants.viewType = 3; // Side orthographic view (YZ plane)
		break;
	default:
		constants.viewType = 0; // Default to perspective
		break;
	}

	// Copy to GPU buffer
	std::memcpy( m_constantBufferData, &constants, sizeof( GridConstants ) );
}

math::Mat4<> GridRenderer::calculateInverseViewProjMatrix( const math::Mat4<> &viewMatrix,
	const math::Mat4<> &projMatrix ) const
{
	const math::Mat4<> viewProjMatrix = projMatrix * viewMatrix;
	return viewProjMatrix.inverse();
}

// GridUtils implementation
namespace grid::GridUtils
{
float calculateAdaptiveSpacing( const float cameraDistance, const float baseSpacing )
{
	return GridRenderer::calculateOptimalSpacing( cameraDistance, baseSpacing );
}

int calculateMajorInterval( const float spacing )
{
	return GridRenderer::calculateMajorInterval( spacing );
}

bool isOnGridLine( const math::Vec2<> &point, const float spacing, const float tolerance )
{
	const float xRemainder = std::fmod( point.x, spacing );
	const float yRemainder = std::fmod( point.y, spacing );

	return ( std::abs( xRemainder ) < tolerance || std::abs( xRemainder - spacing ) < tolerance ) ||
		( std::abs( yRemainder ) < tolerance || std::abs( yRemainder - spacing ) < tolerance );
}

math::Vec2<> snapToGrid( const math::Vec2<> &point, const float spacing )
{
	return math::Vec2<>(
		std::round( point.x / spacing ) * spacing,
		std::round( point.y / spacing ) * spacing );
}

math::Vec3<> snapToGrid( const math::Vec3<> &point, const float spacing )
{
	return math::Vec3<>(
		std::round( point.x / spacing ) * spacing,
		std::round( point.y / spacing ) * spacing,
		std::round( point.z / spacing ) * spacing );
}

GridBounds calculateGridBounds( const camera::Camera &camera,
	const math::Mat4<> &viewMatrix,
	const math::Mat4<> &projMatrix,
	const float viewportWidth,
	const float viewportHeight )
{
	GridBounds bounds;

	// Simple implementation - can be made more sophisticated
	const float cameraDistance = math::length( camera.getPosition() );
	bounds.optimalSpacing = calculateAdaptiveSpacing( cameraDistance );
	bounds.majorInterval = calculateMajorInterval( bounds.optimalSpacing );

	// Calculate rough bounds based on camera position
	const math::Vec3<> cameraPos = camera.getPosition();
	const float extent = cameraDistance * 2.0f;

	bounds.min = math::Vec2<>( cameraPos.x - extent, cameraPos.y - extent );
	bounds.max = math::Vec2<>( cameraPos.x + extent, cameraPos.y + extent );

	return bounds;
}

math::Vec3<> getAxisColor( const int axis )
{
	switch ( axis )
	{
	case 0:
		return math::Vec3<>( 1.0f, 0.2f, 0.2f ); // X = Red
	case 1:
		return math::Vec3<>( 0.2f, 1.0f, 0.2f ); // Y = Green
	case 2:
		return math::Vec3<>( 0.2f, 0.2f, 1.0f ); // Z = Blue
	default:
		return math::Vec3<>( 0.5f, 0.5f, 0.5f ); // Default = Gray
	}
}

float calculateGridFade( const math::Vec3<> &worldPos,
	const math::Vec3<> &cameraPos,
	const float fadeDistance )
{
	const float distance = math::length( worldPos - cameraPos );
	const float fade = 1.0f - distance / fadeDistance;
	return fade > 0.0f ? fade : 0.0f;
}
} // namespace grid::GridUtils
