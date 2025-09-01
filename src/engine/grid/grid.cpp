// Grid rendering system implementation
// Copyright (c) 2025 Level Editor Project

module; // global module fragment

#include <cstring>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <windows.h>

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
// Helper to convert math::Mat4<> to DirectX::XMFLOAT4X4
DirectX::XMFLOAT4X4 toXMFloat4x4( const math::Mat4<> &mat )
{
	DirectX::XMFLOAT4X4 result;
	// Access matrix elements using m## accessors
	result._11 = mat.m00();
	result._12 = mat.m01();
	result._13 = mat.m02();
	result._14 = mat.m03();
	result._21 = mat.m10();
	result._22 = mat.m11();
	result._23 = mat.m12();
	result._24 = mat.m13();
	result._31 = mat.m20();
	result._32 = mat.m21();
	result._33 = mat.m22();
	result._34 = mat.m23();
	result._41 = mat.m30();
	result._42 = mat.m31();
	result._43 = mat.m32();
	result._44 = mat.m33();
	return result;
}

// Helper to convert math::Vec3<> to DirectX::XMFLOAT3
DirectX::XMFLOAT3 toXMFloat3( const math::Vec3<> &vec )
{
	return DirectX::XMFLOAT3( vec.x, vec.y, vec.z );
}

// Constant buffer structure for grid shader
struct GridConstants
{
	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 projMatrix;
	DirectX::XMFLOAT4X4 invViewProjMatrix;

	DirectX::XMFLOAT3 cameraPosition;
	float gridScale;

	DirectX::XMFLOAT3 majorGridColor;
	float majorGridAlpha;

	DirectX::XMFLOAT3 minorGridColor;
	float minorGridAlpha;

	DirectX::XMFLOAT3 axisXColor;
	float axisXAlpha;

	DirectX::XMFLOAT3 axisYColor;
	float axisYAlpha;

	DirectX::XMFLOAT3 axisZColor;
	float axisZAlpha;

	float fadeDistance;
	float gridSpacing;
	float majorGridInterval;
	float nearPlane;

	float farPlane;
	int showGrid;
	int showAxes;
	float axisThickness;
};
} // namespace

// GridRenderer implementation
GridRenderer::GridRenderer()
{
	// Initialize default settings
	m_settings = GridSettings{};
}

bool GridRenderer::initialize( dx12::Device *device )
{
	if ( !device )
	{
		return false;
	}

	m_device = device;

	// Create shaders
	if ( !createShaders() )
	{
		return false;
	}

	// Create root signature
	if ( !createRootSignature() )
	{
		return false;
	}

	// Create pipeline state
	if ( !createPipelineState() )
	{
		return false;
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
	if ( m_constantBufferData )
	{
		m_constantBuffer->Unmap( 0, nullptr );
		m_constantBufferData = nullptr;
	}

	m_constantBuffer.Reset();
	m_pipelineState.Reset();
	m_rootSignature.Reset();
	m_device = nullptr;
}

bool GridRenderer::render( const camera::Camera &camera,
	const math::Mat4<> &viewMatrix,
	const math::Mat4<> &projMatrix,
	float viewportWidth,
	float viewportHeight )
{
	if ( !m_device || !m_pipelineState || !m_constantBuffer )
	{
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

	// Clamp to reasonable bounds
	optimalSpacing = std::clamp( optimalSpacing, m_settings.minGridSpacing, m_settings.maxGridSpacing );

	// Update settings if spacing has changed significantly
	if ( std::abs( m_settings.gridSpacing - optimalSpacing ) > m_settings.gridSpacing * 0.1f )
	{
		m_settings.gridSpacing = optimalSpacing;
		m_settings.majorGridInterval = static_cast<float>( calculateMajorInterval( optimalSpacing ) );
	}
}

float GridRenderer::calculateOptimalSpacing( const float cameraDistance, const float baseSpacing )
{
	// Adaptive spacing algorithm: adjust grid density based on camera distance
	if ( cameraDistance < 1.0f )
	{
		return baseSpacing * 0.1f; // Fine grid for close-up work
	}
	else if ( cameraDistance < 10.0f )
	{
		return baseSpacing; // Normal grid spacing
	}
	else if ( cameraDistance < 100.0f )
	{
		return baseSpacing * 10.0f; // Coarser grid for medium distance
	}
	else
	{
		return baseSpacing * 100.0f; // Very coarse grid for far distances
	}
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

bool GridRenderer::createShaders()
{
	// Load and compile vertex shader from file
	const std::filesystem::path shaderPath = "shaders/grid.hlsl";

	m_vertexShader = renderer::ShaderCompiler::CompileFromFile( shaderPath, "VSMain", "vs_5_0" );

	if ( !m_vertexShader.isValid() )
	{
		// Log error or handle failure
		return false;
	}

	// Load and compile pixel shader from file
	m_pixelShader = renderer::ShaderCompiler::CompileFromFile( shaderPath, "PSMain", "ps_5_0" );

	if ( !m_pixelShader.isValid() )
	{
		// Log error or handle failure
		return false;
	}

	return true;
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
	// Create pipeline state for grid rendering
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = { m_vertexShader.blob->GetBufferPointer(), m_vertexShader.blob->GetBufferSize() };
	psoDesc.PS = { m_pixelShader.blob->GetBufferPointer(), m_pixelShader.blob->GetBufferSize() };

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

	HRESULT hr = m_device->get()->CreateGraphicsPipelineState( &psoDesc, IID_PPV_ARGS( &m_pipelineState ) );
	return SUCCEEDED( hr );
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

	// Transform matrices
	constants.viewMatrix = toXMFloat4x4( viewMatrix );
	constants.projMatrix = toXMFloat4x4( projMatrix );
	constants.invViewProjMatrix = toXMFloat4x4( calculateInverseViewProjMatrix( viewMatrix, projMatrix ) );

	// Camera data
	constants.cameraPosition = toXMFloat3( camera.getPosition() );
	constants.gridScale = 1.0f;
	constants.nearPlane = camera.getNearPlane();
	constants.farPlane = camera.getFarPlane();

	// Grid colors
	constants.majorGridColor = toXMFloat3( m_settings.majorGridColor );
	constants.majorGridAlpha = m_settings.majorGridAlpha;
	constants.minorGridColor = toXMFloat3( m_settings.minorGridColor );
	constants.minorGridAlpha = m_settings.minorGridAlpha;

	// Axis colors
	constants.axisXColor = toXMFloat3( m_settings.axisXColor );
	constants.axisXAlpha = m_settings.axisXAlpha;
	constants.axisYColor = toXMFloat3( m_settings.axisYColor );
	constants.axisYAlpha = m_settings.axisYAlpha;
	constants.axisZColor = toXMFloat3( m_settings.axisZColor );
	constants.axisZAlpha = m_settings.axisZAlpha;

	// Grid properties
	constants.fadeDistance = m_settings.fadeDistance;
	constants.gridSpacing = m_settings.gridSpacing;
	constants.majorGridInterval = m_settings.majorGridInterval;
	constants.axisThickness = m_settings.axisThickness;

	// Visibility flags
	constants.showGrid = m_settings.showGrid ? 1 : 0;
	constants.showAxes = m_settings.showAxes ? 1 : 0;

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
