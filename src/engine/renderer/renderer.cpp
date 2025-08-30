// Global module fragment - headers go here
module;

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <windows.h>

module engine.renderer;

import std;

namespace renderer
{

// Default vertex and pixel shaders
const char *DefaultShaders::VertexShader = R"(
cbuffer ConstantBuffer : register(b0)
{
    float4x4 viewProjectionMatrix;
};

struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput main(VSInput input)
{
    PSInput result;
    result.position = mul(float4(input.position, 1.0f), viewProjectionMatrix);
    result.color = input.color;
    return result;
}
)";

const char *DefaultShaders::PixelShader = R"(
struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

float4 main(PSInput input) : SV_TARGET
{
    return input.color;
}
)";

// ShaderCompiler implementation
ShaderBlob ShaderCompiler::CompileFromSource(
	const std::string &source,
	const std::string &entryPoint,
	const std::string &profile,
	const std::vector<std::string> &defines )
{
	ShaderBlob result;
	result.entryPoint = entryPoint;
	result.profile = profile;

	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
	UINT compileFlags = 0;

#ifdef _DEBUG
	compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	const std::string defineString = BuildDefineString( defines );

	HRESULT hr = D3DCompile(
		source.c_str(),
		source.length(),
		nullptr,
		nullptr,
		nullptr,
		entryPoint.c_str(),
		profile.c_str(),
		compileFlags,
		0,
		&result.blob,
		&errorBlob );

	if ( FAILED( hr ) )
	{
		if ( errorBlob )
		{
			const std::string error = static_cast<const char *>( errorBlob->GetBufferPointer() );
			throw std::runtime_error( "Shader compilation failed: " + error );
		}
		else
		{
			throw std::runtime_error( "Shader compilation failed with unknown error" );
		}
	}

	return result;
}

ShaderBlob ShaderCompiler::CompileFromFile(
	const std::filesystem::path &filePath,
	const std::string &entryPoint,
	const std::string &profile,
	const std::vector<std::string> &defines )
{
	if ( !std::filesystem::exists( filePath ) )
	{
		throw std::runtime_error( "Shader file not found: " + filePath.string() );
	}

	std::ifstream file( filePath );
	if ( !file.is_open() )
	{
		throw std::runtime_error( "Failed to open shader file: " + filePath.string() );
	}

	const std::string source( ( std::istreambuf_iterator<char>( file ) ), std::istreambuf_iterator<char>() );
	return CompileFromSource( source, entryPoint, profile, defines );
}

std::string ShaderCompiler::BuildDefineString( const std::vector<std::string> &defines )
{
	std::string result;
	for ( const auto &define : defines )
	{
		result += "#define " + define + "\n";
	}
	return result;
}

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

	dx12::ThrowIfFailed( m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS( &m_vertexBuffer ) ) );

	// Copy vertex data
	void *mappedData;
	const D3D12_RANGE readRange = { 0, 0 };
	dx12::ThrowIfFailed( m_vertexBuffer->Map( 0, &readRange, &mappedData ) );
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
		dx12::ThrowIfFailed( m_vertexBuffer->Map( 0, &readRange, &mappedData ) );
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

	dx12::ThrowIfFailed( m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS( &m_indexBuffer ) ) );

	// Copy index data
	void *mappedData;
	D3D12_RANGE readRange = { 0, 0 };
	dx12::ThrowIfFailed( m_indexBuffer->Map( 0, &readRange, &mappedData ) );
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
		dx12::ThrowIfFailed( m_indexBuffer->Map( 0, &readRange, &mappedData ) );
		memcpy( mappedData, indices.data(), bufferSize );
		m_indexBuffer->Unmap( 0, nullptr );
	}
}

// Renderer implementation
Renderer::Renderer( dx12::Device &device )
	: m_device( device )
{
	createRootSignature();
	createPipelineState();
	createConstantBuffer();
}

Renderer::~Renderer()
{
	waitForGPU();

	if ( m_constantBuffer && m_constantBufferData )
	{
		m_constantBuffer->Unmap( 0, nullptr );
	}
}

void Renderer::createRootSignature()
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
	dx12::ThrowIfFailed( D3D12SerializeVersionedRootSignature(
		&rootSigDesc, &signature, &error ) );

	dx12::ThrowIfFailed( m_device->CreateRootSignature(
		0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS( &m_rootSignature ) ) );
}

void Renderer::createPipelineState()
{
	// Compile shaders
	const auto vs = ShaderCompiler::CompileFromSource( DefaultShaders::VertexShader, "main", "vs_5_0" );
	const auto ps = ShaderCompiler::CompileFromSource( DefaultShaders::PixelShader, "main", "ps_5_0" );

	// Input layout
	const D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Pipeline state description
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputLayout, _countof( inputLayout ) };
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = { vs.blob->GetBufferPointer(), vs.blob->GetBufferSize() };
	psoDesc.PS = { ps.blob->GetBufferPointer(), ps.blob->GetBufferSize() };
	psoDesc.RasterizerState = m_currentRenderState.getRasterizerDesc();
	psoDesc.BlendState = m_currentRenderState.getBlendDesc();
	psoDesc.DepthStencilState = m_currentRenderState.getDepthStencilDesc();
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;

	dx12::ThrowIfFailed( m_device->CreateGraphicsPipelineState( &psoDesc, IID_PPV_ARGS( &m_pipelineState ) ) );
}

void Renderer::createConstantBuffer()
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

	dx12::ThrowIfFailed( m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS( &m_constantBuffer ) ) );

	// Map the constant buffer
	D3D12_RANGE readRange = { 0, 0 };
	dx12::ThrowIfFailed( m_constantBuffer->Map( 0, &readRange, &m_constantBufferData ) );
}

void Renderer::createRenderTargets( UINT width, UINT height )
{
	// Create RTV descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = 2; // Back buffer count
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dx12::ThrowIfFailed( m_device->CreateDescriptorHeap( &rtvHeapDesc, IID_PPV_ARGS( &m_rtvHeap ) ) );

	// Create DSV descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dx12::ThrowIfFailed( m_device->CreateDescriptorHeap( &dsvHeapDesc, IID_PPV_ARGS( &m_dsvHeap ) ) );

	// Create depth buffer
	D3D12_HEAP_PROPERTIES depthHeapProps = {};
	depthHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC depthDesc = {};
	depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthDesc.Width = width;
	depthDesc.Height = height;
	depthDesc.DepthOrArraySize = 1;
	depthDesc.MipLevels = 1;
	depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthClearValue.DepthStencil.Depth = 1.0f;

	dx12::ThrowIfFailed( m_device->CreateCommittedResource(
		&depthHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS( &m_depthBuffer ) ) );

	// Create depth stencil view
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	m_device->CreateDepthStencilView( m_depthBuffer.Get(), &dsvDesc, getDSV() );
}

void Renderer::beginFrame( dx12::CommandContext &context, dx12::SwapChain &swapChain )
{
	m_currentContext = &context;
	m_currentSwapChain = &swapChain;

	// Create render targets if needed (first time or resize)
	if ( !m_rtvHeap )
	{
		createRenderTargets( 1920, 1080 ); // Default size, should be configurable
	}

	// Create RTVs for current swap chain back buffers
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	UINT rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );

	for ( UINT i = 0; i < dx12::SwapChain::BufferCount; i++ )
	{
		ID3D12Resource *backBuffer = swapChain.GetCurrentBackBuffer();
		m_device->CreateRenderTargetView( backBuffer, nullptr, rtvHandle );
		rtvHandle.ptr += rtvDescriptorSize;
	}

	// Transition back buffer to render target
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = swapChain.GetCurrentBackBuffer();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	context->ResourceBarrier( 1, &barrier );

	// Set render targets
	D3D12_CPU_DESCRIPTOR_HANDLE currentRTV = getCurrentRTV();
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = getDSV();
	context->OMSetRenderTargets( 1, &currentRTV, FALSE, &dsv );

	// Set viewport
	D3D12_VIEWPORT viewport = {};
	viewport.Width = 1920.0f; // Should match render target size
	viewport.Height = 1080.0f;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	context->RSSetViewports( 1, &viewport );

	D3D12_RECT scissorRect = {};
	scissorRect.right = 1920;
	scissorRect.bottom = 1080;
	context->RSSetScissorRects( 1, &scissorRect );
}

void Renderer::endFrame()
{
	if ( !m_currentContext || !m_currentSwapChain )
		return;

	// Transition back buffer to present
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = m_currentSwapChain->GetCurrentBackBuffer();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	( *m_currentContext )->ResourceBarrier( 1, &barrier );

	m_currentContext = nullptr;
	m_currentSwapChain = nullptr;
}

void Renderer::clear( const Color &clearColor ) noexcept
{
	if ( !m_currentContext )
		return;

	float color[4] = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
	( *m_currentContext )->ClearRenderTargetView( getCurrentRTV(), color, 0, nullptr );
}

void Renderer::clearDepth( float depth ) noexcept
{
	if ( !m_currentContext )
		return;

	( *m_currentContext )->ClearDepthStencilView( getDSV(), D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr );
}

void Renderer::setViewProjectionMatrix( const math::Mat4<> &viewProj ) noexcept
{
	m_viewProjectionMatrix = viewProj;
	updateConstantBuffer();
}

void Renderer::updateConstantBuffer()
{
	if ( m_constantBufferData )
	{
		memcpy( m_constantBufferData, &m_viewProjectionMatrix, sizeof( math::Mat4<> ) );
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE Renderer::getCurrentRTV() const
{
	if ( !m_currentSwapChain )
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
		return handle;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	const UINT rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
	rtvHandle.ptr += rtvDescriptorSize * m_currentSwapChain->GetCurrentBackBufferIndex();
	return rtvHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE Renderer::getDSV() const
{
	return m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void Renderer::setRenderState( const RenderState &state ) noexcept
{
	m_currentRenderState = state;
	// Note: In D3D12, render state changes require PSO recreation
	// For now, this stores the state - would need PSO caching for production
}

void Renderer::drawVertices( const std::vector<Vertex> &vertices, D3D_PRIMITIVE_TOPOLOGY topology ) noexcept
{
	if ( !m_currentContext || vertices.empty() )
		return;

	// Update or create dynamic vertex buffer
	if ( !m_dynamicVertexBuffer || m_dynamicVertexBuffer->getVertexCount() < vertices.size() )
	{
		m_dynamicVertexBuffer = std::make_unique<VertexBuffer>( m_device, vertices );
	}
	else
	{
		m_dynamicVertexBuffer->update( vertices );
	}

	// Set pipeline state and root signature
	( *m_currentContext )->SetPipelineState( m_pipelineState.Get() );
	( *m_currentContext )->SetGraphicsRootSignature( m_rootSignature.Get() );
	( *m_currentContext )->SetGraphicsRootConstantBufferView( 0, m_constantBuffer->GetGPUVirtualAddress() );

	// Set primitive topology
	( *m_currentContext )->IASetPrimitiveTopology( topology );

	// Set vertex buffer and draw
	D3D12_VERTEX_BUFFER_VIEW vbv = m_dynamicVertexBuffer->getView();
	( *m_currentContext )->IASetVertexBuffers( 0, 1, &vbv );
	( *m_currentContext )->DrawInstanced( static_cast<UINT>( vertices.size() ), 1, 0, 0 );
}

void Renderer::drawIndexed( const std::vector<Vertex> &vertices, const std::vector<uint16_t> &indices, D3D_PRIMITIVE_TOPOLOGY topology ) noexcept
{
	if ( !m_currentContext || vertices.empty() || indices.empty() )
		return;

	// Update buffers
	if ( !m_dynamicVertexBuffer || m_dynamicVertexBuffer->getVertexCount() < vertices.size() )
	{
		m_dynamicVertexBuffer = std::make_unique<VertexBuffer>( m_device, vertices );
	}
	else
	{
		m_dynamicVertexBuffer->update( vertices );
	}

	if ( !m_dynamicIndexBuffer || m_dynamicIndexBuffer->getIndexCount() < indices.size() )
	{
		m_dynamicIndexBuffer = std::make_unique<IndexBuffer>( m_device, indices );
	}
	else
	{
		m_dynamicIndexBuffer->update( indices );
	}

	// Set pipeline state
	( *m_currentContext )->SetPipelineState( m_pipelineState.Get() );
	( *m_currentContext )->SetGraphicsRootSignature( m_rootSignature.Get() );
	( *m_currentContext )->SetGraphicsRootConstantBufferView( 0, m_constantBuffer->GetGPUVirtualAddress() );
	( *m_currentContext )->IASetPrimitiveTopology( topology );

	// Set buffers and draw
	D3D12_VERTEX_BUFFER_VIEW vbv = m_dynamicVertexBuffer->getView();
	D3D12_INDEX_BUFFER_VIEW ibv = m_dynamicIndexBuffer->getView();
	( *m_currentContext )->IASetVertexBuffers( 0, 1, &vbv );
	( *m_currentContext )->IASetIndexBuffer( &ibv );
	( *m_currentContext )->DrawIndexedInstanced( static_cast<UINT>( indices.size() ), 1, 0, 0, 0 );
}

void Renderer::drawLine( const math::Vec3<> &start, const math::Vec3<> &end, const Color &color ) noexcept
{
	const std::vector<Vertex> vertices = {
		{ start, color },
		{ end, color }
	};
	drawVertices( vertices, D3D_PRIMITIVE_TOPOLOGY_LINELIST );
}


void Renderer::drawWireframeCube( const math::Vec3<> &center, const math::Vec3<> &size, const Color &color ) noexcept
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
    std::vector<uint16_t> indices = {
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


void Renderer::waitForGPU() noexcept
{
	// This would typically use a fence to wait for GPU completion
	// For now, this is a placeholder - actual implementation would need fence synchronization
}

} // namespace renderer
