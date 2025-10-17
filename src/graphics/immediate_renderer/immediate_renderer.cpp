#include "immediate_renderer.h"

#include <d3d12.h>
#include <wrl.h>

#include "core/console.h"
#include "graphics/shader_manager/shader_manager.h"
#include "graphics/shader_manager/shader_compiler.h"

namespace graphics
{

// RenderState implementation
RenderState::RenderState() = default;

void RenderState::apply( [[maybe_unused]] ID3D12GraphicsCommandList *cmdList ) const
{
	// Note: In D3D12, render state is baked into pipeline state objects
	// This would typically be used to set dynamic state like viewport, scissors, etc.
	// For now, this is a placeholder - actual state changes happen during PSO creation
}

D3D12_DEPTH_STENCIL_DESC RenderState::getDepthStencilDesc() const
{
	D3D12_DEPTH_STENCIL_DESC desc = {};
	desc.DepthEnable = m_depthTestEnabled;
	desc.DepthWriteMask = m_depthWriteEnabled ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	desc.StencilEnable = FALSE;
	return desc;
}

D3D12_RASTERIZER_DESC RenderState::getRasterizerDesc() const
{
	D3D12_RASTERIZER_DESC desc = {};
	desc.FillMode = m_wireframeEnabled ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
	desc.CullMode = m_cullMode;
	desc.FrontCounterClockwise = FALSE;
	desc.DepthBias = 0;
	desc.DepthBiasClamp = 0.0f;
	desc.SlopeScaledDepthBias = 0.0f;
	desc.DepthClipEnable = TRUE;
	desc.MultisampleEnable = FALSE;
	desc.AntialiasedLineEnable = FALSE;
	desc.ForcedSampleCount = 0;
	desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	return desc;
}

D3D12_BLEND_DESC RenderState::getBlendDesc() const
{
	D3D12_BLEND_DESC desc = {};
	desc.AlphaToCoverageEnable = FALSE;
	desc.IndependentBlendEnable = FALSE;

	D3D12_RENDER_TARGET_BLEND_DESC &rtBlend = desc.RenderTarget[0];
	rtBlend.BlendEnable = m_blendEnabled;
	rtBlend.LogicOpEnable = FALSE;
	rtBlend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	rtBlend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	rtBlend.BlendOp = D3D12_BLEND_OP_ADD;
	rtBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
	rtBlend.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	rtBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	rtBlend.LogicOp = D3D12_LOGIC_OP_NOOP;
	rtBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return desc;
}

// VertexBuffer implementation
VertexBuffer::VertexBuffer( dx12::Device &device, const std::vector<Vertex> &vertices )
	: m_device( device ), m_vertexCount( static_cast<UINT>( vertices.size() ) )
{
	createBuffer( vertices );
}

void VertexBuffer::createBuffer( const std::vector<Vertex> &vertices )
{
	if ( vertices.empty() )
	{
		console::errorAndThrow( "VertexBuffer: empty vertex array" );
	}
	const UINT bufferSize = static_cast<UINT>( vertices.size() * sizeof( Vertex ) );

	// Create upload heap
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

	dx12::throwIfFailed( m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS( &m_vertexBuffer ) ) );

	// Copy vertex data
	void *mappedData;
	const D3D12_RANGE readRange = { 0, 0 };
	dx12::throwIfFailed( m_vertexBuffer->Map( 0, &readRange, &mappedData ) );
	memcpy( mappedData, vertices.data(), bufferSize );
	m_vertexBuffer->Unmap( 0, nullptr );

	// Create vertex buffer view
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.SizeInBytes = bufferSize;
	m_vertexBufferView.StrideInBytes = sizeof( Vertex );
}

void VertexBuffer::update( const std::vector<Vertex> &vertices )
{
	if ( vertices.size() != m_vertexCount )
	{
		// Recreate buffer with new size
		m_vertexCount = static_cast<UINT>( vertices.size() );
		createBuffer( vertices );
	}
	else
	{
		// Update existing buffer
		const UINT bufferSize = static_cast<UINT>( vertices.size() * sizeof( Vertex ) );
		void *mappedData;
		D3D12_RANGE readRange = { 0, 0 };
		dx12::throwIfFailed( m_vertexBuffer->Map( 0, &readRange, &mappedData ) );
		memcpy( mappedData, vertices.data(), bufferSize );
		m_vertexBuffer->Unmap( 0, nullptr );
	}
}

// IndexBuffer implementation
IndexBuffer::IndexBuffer( dx12::Device &device, const std::vector<uint16_t> &indices )
	: m_device( device ), m_indexCount( static_cast<UINT>( indices.size() ) )
{
	createBuffer( indices );
}

void IndexBuffer::createBuffer( const std::vector<uint16_t> &indices )
{
	if ( indices.empty() )
	{
		console::errorAndThrow( "IndexBuffer: empty index array" );
	}
	const UINT bufferSize = static_cast<UINT>( indices.size() * sizeof( uint16_t ) );

	// Create upload heap
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = bufferSize;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	dx12::throwIfFailed( m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS( &m_indexBuffer ) ) );

	// Copy index data
	void *mappedData;
	D3D12_RANGE readRange = { 0, 0 };
	dx12::throwIfFailed( m_indexBuffer->Map( 0, &readRange, &mappedData ) );
	memcpy( mappedData, indices.data(), bufferSize );
	m_indexBuffer->Unmap( 0, nullptr );

	// Create index buffer view
	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.SizeInBytes = bufferSize;
	m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
}

void IndexBuffer::update( const std::vector<uint16_t> &indices )
{
	if ( indices.size() != m_indexCount )
	{
		m_indexCount = static_cast<UINT>( indices.size() );
		createBuffer( indices );
	}
	else
	{
		const UINT bufferSize = static_cast<UINT>( indices.size() * sizeof( uint16_t ) );
		void *mappedData;
		D3D12_RANGE readRange = { 0, 0 };
		dx12::throwIfFailed( m_indexBuffer->Map( 0, &readRange, &mappedData ) );
		memcpy( mappedData, indices.data(), bufferSize );
		m_indexBuffer->Unmap( 0, nullptr );
	}
}

// ImmediateRenderer implementation
ImmediateRenderer::ImmediateRenderer( dx12::Device &device, shader_manager::ShaderManager &shaderManager )
	: m_device( device ), m_shaderManager( shaderManager )
{
	createRootSignature();
	compileDefaultShaders();
	createConstantBuffer();
}

ImmediateRenderer::~ImmediateRenderer()
{
	waitForGPU();

	if ( m_constantBuffer && m_constantBufferData )
	{
		m_constantBuffer->Unmap( 0, nullptr );
	}
}

void ImmediateRenderer::createRootSignature()
{
	// Root parameter for constant buffer
	D3D12_ROOT_PARAMETER1 rootParameter = {};
	rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameter.Descriptor.ShaderRegister = 0;
	rootParameter.Descriptor.RegisterSpace = 0;
	rootParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
	rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	rootSigDesc.Desc_1_1.NumParameters = 1;
	rootSigDesc.Desc_1_1.pParameters = &rootParameter;
	rootSigDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	Microsoft::WRL::ComPtr<ID3DBlob> signature;
	Microsoft::WRL::ComPtr<ID3DBlob> error;
	dx12::throwIfFailed( D3D12SerializeVersionedRootSignature(
		&rootSigDesc, &signature, &error ) );

	dx12::throwIfFailed( m_device->CreateRootSignature(
		0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS( &m_rootSignature ) ) );
}

void ImmediateRenderer::compileDefaultShaders()
{
	if ( !m_vsBlob || !m_psBlob )
	{
		// Register shaders with ShaderManager
		m_vertexShaderHandle = m_shaderManager.registerShader(
			"shaders/simple.hlsl",
			"VSMain",
			"vs_5_0",
			shader_manager::ShaderType::Vertex );

		m_pixelShaderHandle = m_shaderManager.registerShader(
			"shaders/simple.hlsl",
			"PSMain",
			"ps_5_0",
			shader_manager::ShaderType::Pixel );

		// Get shader blobs from ShaderManager
		const shader_manager::ShaderBlob *vsBlob = m_shaderManager.getShaderBlob( m_vertexShaderHandle );
		const shader_manager::ShaderBlob *psBlob = m_shaderManager.getShaderBlob( m_pixelShaderHandle );

		if ( vsBlob && vsBlob->isValid() )
		{
			m_vsBlob = vsBlob->blob;
		}
		else
		{
			console::error( "Renderer: Failed to compile vertex shader from shaders/simple.hlsl" );
		}

		if ( psBlob && psBlob->isValid() )
		{
			m_psBlob = psBlob->blob;
		}
		else
		{
			console::error( "Renderer: Failed to compile pixel shader from shaders/simple.hlsl" );
		}
	}
}

ImmediateRenderer::PipelineStateKey ImmediateRenderer::makeKeyFromState( const RenderState &state, D3D12_PRIMITIVE_TOPOLOGY_TYPE topology ) const noexcept
{
	return PipelineStateKey{ state.isDepthTestEnabled(), state.isDepthWriteEnabled(), state.isWireframeEnabled(), state.isBlendEnabled(), state.getCullMode(), topology };
}

void ImmediateRenderer::createPipelineStateForKey( const PipelineStateKey &key )
{
	// Build a temporary RenderState reflecting the key
	RenderState temp;
	temp.setDepthTest( key.depthTest );
	temp.setDepthWrite( key.depthWrite );
	temp.setWireframe( key.wireframe );
	temp.setBlendEnabled( key.blend );
	temp.setCullMode( key.cullMode );

	const D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputLayout, _countof( inputLayout ) };
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = { m_vsBlob->GetBufferPointer(), m_vsBlob->GetBufferSize() };
	psoDesc.PS = { m_psBlob->GetBufferPointer(), m_psBlob->GetBufferSize() };
	psoDesc.RasterizerState = temp.getRasterizerDesc();
	psoDesc.BlendState = temp.getBlendDesc();
	psoDesc.DepthStencilState = temp.getDepthStencilDesc();
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = key.topologyType;

	// In headless mode (no swap chain), we don't have render targets
	if ( m_currentSwapChain )
	{
		// Windowed mode: use actual render target and depth formats
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	}
	else
	{
		// Headless mode: no render targets
		psoDesc.NumRenderTargets = 0;
		psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	}

	psoDesc.SampleDesc.Count = 1;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
	dx12::throwIfFailed( m_device->CreateGraphicsPipelineState( &psoDesc, IID_PPV_ARGS( &pso ) ) );
	m_psoCache.emplace( key, pso );
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE ImmediateRenderer::topologyToTopologyType( D3D_PRIMITIVE_TOPOLOGY topology ) noexcept
{
	switch ( topology )
	{
	case D3D_PRIMITIVE_TOPOLOGY_POINTLIST:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
	case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
	case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	default:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // Safe fallback
	}
}

void ImmediateRenderer::ensurePipelineForCurrentState( D3D12_PRIMITIVE_TOPOLOGY_TYPE topology )
{
	compileDefaultShaders();
	const auto key = makeKeyFromState( m_currentRenderState, topology );
	auto it = m_psoCache.find( key );
	if ( it == m_psoCache.end() )
	{
		createPipelineStateForKey( key );
		it = m_psoCache.find( key );
	}
	m_activePipelineState = it->second;
}

void ImmediateRenderer::createConstantBuffer()
{
	const UINT constantBufferSize = ( sizeof( math::Mat4<> ) + 255 ) & ~255; // Align to 256 bytes

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = constantBufferSize;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	dx12::throwIfFailed( m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS( &m_constantBuffer ) ) );

	// Map the constant buffer
	D3D12_RANGE readRange = { 0, 0 };
	dx12::throwIfFailed( m_constantBuffer->Map( 0, &readRange, &m_constantBufferData ) );
}


void ImmediateRenderer::beginFrame()
{
	// Validate that beginFrame hasn't already been called
	if ( m_inFrame )
	{
		console::error( "ImmediateRenderer::beginFrame called when already in frame. Call endFrame() first." );
		return;
	}

	// Validate that Device is in frame (Device::beginFrame should be called first)
	if ( !m_device.isInFrame() )
	{
		console::error( "ImmediateRenderer::beginFrame called but Device is not in frame. Call Device::beginFrame() first." );
		return;
	}

	// Get CommandContext and SwapChain from the Device (Device::beginFrame should be called by caller)
	m_currentContext = m_device.getCommandContext();
	m_currentSwapChain = m_device.getSwapChain();

	if ( !m_currentContext )
	{
		console::error( "ImmediateRenderer::beginFrame failed - no command context. Ensure Device::beginFrame() was called first." );
		return;
	}

	// Mark that we're now in a frame
	m_inFrame = true;

	// Now it's safe to delete any pending buffers since the command list has been executed
	m_pendingVertexBufferDeletions.clear();
	m_pendingIndexBufferDeletions.clear();

	// Use actual SwapChain dimensions instead of hardcoded values
	D3D12_VIEWPORT viewport = {};
	D3D12_RECT scissorRect = {};

	if ( m_currentSwapChain )
	{
		// Use actual swap chain dimensions
		viewport.Width = static_cast<float>( m_currentSwapChain->getWidth() );
		viewport.Height = static_cast<float>( m_currentSwapChain->getHeight() );
		scissorRect.right = m_currentSwapChain->getWidth();
		scissorRect.bottom = m_currentSwapChain->getHeight();
	}
	else
	{
		// Fallback for headless mode - use reasonable defaults
		viewport.Width = 1920.0f;
		viewport.Height = 1080.0f;
		scissorRect.right = 1920;
		scissorRect.bottom = 1080;
	}

	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	m_currentContext->get()->RSSetViewports( 1, &viewport );
	m_currentContext->get()->RSSetScissorRects( 1, &scissorRect );
}

void ImmediateRenderer::endFrame()
{
	// Validate that we're actually in a frame
	if ( !m_inFrame )
	{
		console::error( "ImmediateRenderer::endFrame called when not in frame. Call beginFrame() first." );
		return;
	}

	if ( !m_currentContext )
	{
		console::error( "ImmediateRenderer::endFrame called but no command context available." );
		m_inFrame = false;
		return;
	}

	// Reset renderer state (Device::endFrame should be called by caller)
	m_currentContext = nullptr;
	m_currentSwapChain = nullptr;
	m_inFrame = false;
}

void ImmediateRenderer::clear( const math::Color &clearColor ) noexcept
{
	m_device.clear( clearColor );
}

void ImmediateRenderer::clearDepth( float depth ) noexcept
{
	m_device.clearDepth( depth );
}

void ImmediateRenderer::setViewProjectionMatrix( const math::Mat4<> &viewProj ) noexcept
{
	m_viewProjectionMatrix = viewProj;
	updateConstantBuffer();
}

void ImmediateRenderer::updateConstantBuffer()
{
	if ( m_constantBufferData )
	{
		memcpy( m_constantBufferData, &m_viewProjectionMatrix, sizeof( math::Mat4<> ) );
	}
}


void ImmediateRenderer::setRenderState( const RenderState &state ) noexcept
{
	m_currentRenderState = state;
	// Invalidate active PSO; will lazily fetch from cache
	m_activePipelineState.Reset();
}

void ImmediateRenderer::drawVertices( const std::vector<Vertex> &vertices, D3D_PRIMITIVE_TOPOLOGY topology ) noexcept
{
	if ( vertices.empty() )
		return;

	// In headless mode (no swap chain), skip actual drawing but still update buffers for testing
	const bool isHeadless = !m_currentSwapChain;

	// Update or create dynamic vertex buffer
	if ( !m_dynamicVertexBuffer || m_dynamicVertexBuffer->getVertexCount() < vertices.size() )
	{
		// If we have an existing buffer, defer its deletion until frame end
		if ( m_dynamicVertexBuffer )
		{
			m_pendingVertexBufferDeletions.push_back( std::move( m_dynamicVertexBuffer ) );
		}
		m_dynamicVertexBuffer = std::make_unique<VertexBuffer>( m_device, vertices );
	}
	else
	{
		m_dynamicVertexBuffer->update( vertices );
	}

	// Skip drawing in headless mode (no render targets available)
	if ( isHeadless )
	{
		return;
	}

	// Set pipeline state and root signature
	ensurePipelineForCurrentState( topologyToTopologyType( topology ) );
	( *m_currentContext )->SetPipelineState( m_activePipelineState.Get() );
	( *m_currentContext )->SetGraphicsRootSignature( m_rootSignature.Get() );
	( *m_currentContext )->SetGraphicsRootConstantBufferView( 0, m_constantBuffer->GetGPUVirtualAddress() );

	// Set primitive topology
	( *m_currentContext )->IASetPrimitiveTopology( topology );

	// Set vertex buffer and draw
	const D3D12_VERTEX_BUFFER_VIEW vbv = m_dynamicVertexBuffer->getView();
	( *m_currentContext )->IASetVertexBuffers( 0, 1, &vbv );
	( *m_currentContext )->DrawInstanced( static_cast<UINT>( vertices.size() ), 1, 0, 0 );
}

void ImmediateRenderer::drawIndexed( const std::vector<Vertex> &vertices, const std::vector<uint16_t> &indices, D3D_PRIMITIVE_TOPOLOGY topology ) noexcept
{
	if ( vertices.empty() || indices.empty() )
		return;

	// In headless mode (no swap chain), skip actual drawing but still update buffers for testing
	const bool isHeadless = !m_currentSwapChain;

	// Update buffers
	if ( !m_dynamicVertexBuffer || m_dynamicVertexBuffer->getVertexCount() < vertices.size() )
	{
		// If we have an existing buffer, defer its deletion until frame end
		if ( m_dynamicVertexBuffer )
		{
			m_pendingVertexBufferDeletions.push_back( std::move( m_dynamicVertexBuffer ) );
		}
		m_dynamicVertexBuffer = std::make_unique<VertexBuffer>( m_device, vertices );
	}
	else
	{
		m_dynamicVertexBuffer->update( vertices );
	}

	if ( !m_dynamicIndexBuffer || m_dynamicIndexBuffer->getIndexCount() < indices.size() )
	{
		// If we have an existing buffer, defer its deletion until frame end
		if ( m_dynamicIndexBuffer )
		{
			m_pendingIndexBufferDeletions.push_back( std::move( m_dynamicIndexBuffer ) );
		}
		m_dynamicIndexBuffer = std::make_unique<IndexBuffer>( m_device, indices );
	}
	else
	{
		m_dynamicIndexBuffer->update( indices );
	}

	// Skip drawing in headless mode (no render targets available)
	if ( isHeadless )
	{
		return;
	}

	// Set pipeline state
	ensurePipelineForCurrentState( topologyToTopologyType( topology ) );
	( *m_currentContext )->SetPipelineState( m_activePipelineState.Get() );
	( *m_currentContext )->SetGraphicsRootSignature( m_rootSignature.Get() );
	( *m_currentContext )->SetGraphicsRootConstantBufferView( 0, m_constantBuffer->GetGPUVirtualAddress() );
	( *m_currentContext )->IASetPrimitiveTopology( topology );

	// Set buffers and draw
	const D3D12_VERTEX_BUFFER_VIEW vbv = m_dynamicVertexBuffer->getView();
	const D3D12_INDEX_BUFFER_VIEW ibv = m_dynamicIndexBuffer->getView();
	( *m_currentContext )->IASetVertexBuffers( 0, 1, &vbv );
	( *m_currentContext )->IASetIndexBuffer( &ibv );
	( *m_currentContext )->DrawIndexedInstanced( static_cast<UINT>( indices.size() ), 1, 0, 0, 0 );
}

void ImmediateRenderer::drawLine( const math::Vec3<> &start, const math::Vec3<> &end, const math::Color &color ) noexcept
{
	const std::vector<Vertex> vertices = {
		{ start, color },
		{ end, color }
	};
	drawVertices( vertices, D3D_PRIMITIVE_TOPOLOGY_LINELIST );
}


void ImmediateRenderer::drawWireframeCube( const math::Vec3<> &center, const math::Vec3<> &size, const math::Color &color ) noexcept
{
	const math::Vec3<> halfSize = size * 0.5f;

	// 8 vertices of the cube
	const std::vector<Vertex> vertices = {
		// Bottom face
		{ center + math::Vec3<>{ -halfSize.x, -halfSize.y, -halfSize.z }, color },
		{ center + math::Vec3<>{ halfSize.x, -halfSize.y, -halfSize.z }, color },
		{ center + math::Vec3<>{ halfSize.x, halfSize.y, -halfSize.z }, color },
		{ center + math::Vec3<>{ -halfSize.x, halfSize.y, -halfSize.z }, color },
		// Top face
		{ center + math::Vec3<>{ -halfSize.x, -halfSize.y, halfSize.z }, color },
		{ center + math::Vec3<>{ halfSize.x, -halfSize.y, halfSize.z }, color },
		{ center + math::Vec3<>{ halfSize.x, halfSize.y, halfSize.z }, color },
		{ center + math::Vec3<>{ -halfSize.x, halfSize.y, halfSize.z }, color }
	};

	// 12 edges of the cube
	// clang-format off
    const std::vector<uint16_t> indices = {
        // Bottom face
        0, 1,  1, 2,  2, 3,  3, 0,
        // Top face  
        4, 5,  5, 6,  6, 7,  7, 4,
        // Vertical edges
        0, 4,  1, 5,  2, 6,  3, 7
    };
	// clang-format on

	drawIndexed( vertices, indices, D3D_PRIMITIVE_TOPOLOGY_LINELIST );
}


void ImmediateRenderer::waitForGPU() noexcept
{
	// This would typically use a fence to wait for GPU completion
	// For now, this is a placeholder - actual implementation would need fence synchronization
}

} // namespace graphics
