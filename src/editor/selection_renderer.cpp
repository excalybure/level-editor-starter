module; // global module fragment

#include <d3d12.h>
#include <wrl.h>

module editor.selection_renderer;

import runtime.console;
import runtime.time;
import engine.shader_manager;
import std;

namespace editor
{

SelectionRenderer::SelectionRenderer( dx12::Device &device, shader_manager::ShaderManager &shaderManager )
	: m_device( device ), m_shaderManager( shaderManager )
{
	setupRenderingResources();
}

void SelectionRenderer::render( ecs::Scene &scene,
	ID3D12GraphicsCommandList *commandList,
	const math::Mat4<> &viewMatrix,
	const math::Mat4<> &projMatrix,
	const math::Vec2<> &viewportSize )
{
	// Main render method that calls both outline and hover rendering
	renderSelectionOutlines( scene, commandList, viewMatrix, projMatrix, viewportSize );
	// Note: Hover highlighting would typically be called separately by the viewport system
}

void SelectionRenderer::renderSelectionOutlines( ecs::Scene &scene,
	ID3D12GraphicsCommandList *commandList,
	const math::Mat4<> &viewMatrix,
	const math::Mat4<> &projMatrix,
	const math::Vec2<> &viewportSize )
{
	if ( !commandList )
	{
		return;
	}

	// Set pipeline state for outline rendering if available
	if ( m_outlinePipelineState )
	{
		commandList->SetPipelineState( m_outlinePipelineState.Get() );
	}

	if ( m_rootSignature )
	{
		commandList->SetGraphicsRootSignature( m_rootSignature.Get() );
	}

	// Iterate through all entities with Selected component
	scene.forEach<components::Selected>( [&]( ecs::Entity entity, const components::Selected &selected ) {
		// Check if entity also has Transform component
		if ( !scene.hasComponent<components::Transform>( entity ) )
		{
			return;
		}

		// Skip entities without renderable meshes
		if ( !entityHasRenderableMesh( entity, scene ) )
		{
			return;
		}

		// Choose color based on selection state
		math::Vec4<> outlineColor = selected.isPrimary ? m_style.primaryColor : m_style.selectedColor;

		// Apply animation if enabled
		if ( m_style.animateSelection )
		{
			outlineColor = animateColor( outlineColor, getAnimationTime() );
		}

		renderEntityOutline( entity, scene, outlineColor, commandList, viewMatrix, projMatrix, viewportSize );
	} );
}

void SelectionRenderer::renderHoverHighlight( ecs::Entity hoveredEntity,
	ecs::Scene &scene,
	ID3D12GraphicsCommandList *commandList,
	const math::Mat4<> &viewMatrix,
	const math::Mat4<> &projMatrix,
	const math::Vec2<> &viewportSize )
{
	if ( !commandList || hoveredEntity == ecs::Entity{} )
	{
		return;
	}

	// Skip if entity is already selected (avoid double highlighting)
	if ( scene.hasComponent<components::Selected>( hoveredEntity ) )
	{
		return;
	}

	if ( !entityHasRenderableMesh( hoveredEntity, scene ) )
	{
		return;
	}

	renderEntityOutline( hoveredEntity, scene, m_style.hoveredColor, commandList, viewMatrix, projMatrix, viewportSize );
}

void SelectionRenderer::renderRectSelection( const math::Vec2<> &startPos,
	const math::Vec2<> &endPos,
	ID3D12GraphicsCommandList *commandList,
	const math::Vec2<> &viewportSize )
{
	if ( !commandList )
	{
		return;
	}

	// Check if shaders are ready
	const auto *vsBlob = m_shaderManager.getShaderBlob( m_rectVertexShader );
	const auto *psBlob = m_shaderManager.getShaderBlob( m_rectPixelShader );

	if ( !vsBlob || !psBlob || !vsBlob->isValid() || !psBlob->isValid() )
	{
		console::warning( "Selection rectangle shaders not ready, skipping render" );
		return;
	}

	// Set pipeline state for rectangle rendering
	if ( m_rectPipelineState )
	{
		commandList->SetPipelineState( m_rectPipelineState.Get() );
	}

	if ( m_rootSignature )
	{
		commandList->SetGraphicsRootSignature( m_rootSignature.Get() );
	}

	// Update constant buffer with rectangle bounds
	if ( m_constantBuffer && m_constantBufferData )
	{
		// Calculate normalized device coordinates for rectangle
		// Assuming screen coordinates are passed in, we need to convert to NDC
		struct RectConstants
		{
			math::Vec4<> rectBounds;   // startX, startY, endX, endY in NDC
			math::Vec4<> rectColor;	   // Selection color
			math::Vec4<> screenParams; // screenWidth, screenHeight, unused, unused
			math::Vec4<> padding;	   // Padding for 256-byte alignment
		};

		RectConstants constants;
		constants.rectBounds = math::Vec4<>{ startPos.x, startPos.y, endPos.x, endPos.y };
		constants.rectColor = m_style.rectSelectColor;
		constants.screenParams = math::Vec4<>{ viewportSize.x, viewportSize.y, 0.0f, 0.0f };
		constants.padding = math::Vec4<>{ 0.0f, 0.0f, 0.0f, 0.0f };

		// Copy to constant buffer
		memcpy( m_constantBufferData, &constants, sizeof( constants ) );

		// Set constant buffer for rendering
		commandList->SetGraphicsRootConstantBufferView( 0, m_constantBuffer->GetGPUVirtualAddress() );
	}

	// Set vertex and index buffers
	if ( m_rectVertexBuffer && m_rectIndexBuffer )
	{
		commandList->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		commandList->IASetVertexBuffers( 0, 1, &m_rectVertexBufferView );
		commandList->IASetIndexBuffer( &m_rectIndexBufferView );

		// Draw the rectangle (2 triangles = 6 indices)
		commandList->DrawIndexedInstanced( 6, 1, 0, 0, 0 );
	}
}

void SelectionRenderer::setupRenderingResources()
{
	try
	{
		// Register shaders with ShaderManager
		m_outlineVertexShader = m_shaderManager.registerShader(
			"shaders/selection_outline.hlsl",
			"VSMain",
			"vs_5_1",
			shader_manager::ShaderType::Vertex );

		m_outlinePixelShader = m_shaderManager.registerShader(
			"shaders/selection_outline.hlsl",
			"PSMain",
			"ps_5_1",
			shader_manager::ShaderType::Pixel );

		m_rectVertexShader = m_shaderManager.registerShader(
			"shaders/selection_rect.hlsl",
			"VSMain",
			"vs_5_1",
			shader_manager::ShaderType::Vertex );

		m_rectPixelShader = m_shaderManager.registerShader(
			"shaders/selection_rect.hlsl",
			"PSMain",
			"ps_5_1",
			shader_manager::ShaderType::Pixel );

		// Create D3D12 resources
		createRootSignature();
		createConstantBuffer();
		createRectVertexBuffer();
		createRectPipelineState();
		createOutlinePipelineState();
	}
	catch ( const std::exception &e )
	{
		console::error( "Failed to setup selection renderer resources: {}", e.what() );
	}
}

void SelectionRenderer::renderEntityOutline( ecs::Entity entity,
	ecs::Scene &scene,
	const math::Vec4<> &color,
	ID3D12GraphicsCommandList *commandList,
	const math::Mat4<> &viewMatrix,
	const math::Mat4<> &projMatrix,
	const math::Vec2<> &viewportSize )
{
	if ( !commandList )
	{
		return;
	}

	// Get entity mesh renderer for geometry
	auto *meshRenderer = scene.getComponent<components::MeshRenderer>( entity );
	if ( !meshRenderer || !meshRenderer->gpuMesh )
	{
		return;
	}

	// Get entity world matrix
	const math::Mat4<> worldMatrix = getEntityWorldMatrix( entity, scene );
	const math::Mat4<> worldViewProj = projMatrix * viewMatrix * worldMatrix;

	// Check if outline shaders are ready
	const auto *vsBlob = m_shaderManager.getShaderBlob( m_outlineVertexShader );
	const auto *psBlob = m_shaderManager.getShaderBlob( m_outlinePixelShader );

	if ( !vsBlob || !psBlob || !vsBlob->isValid() || !psBlob->isValid() )
	{
		console::warning( "Selection outline shaders not ready, skipping outline render" );
		return;
	}

	// Set pipeline state for outline rendering
	if ( m_outlinePipelineState )
	{
		commandList->SetPipelineState( m_outlinePipelineState.Get() );
	}

	// Update constant buffer with outline constants
	if ( m_constantBuffer && m_constantBufferData )
	{
		struct OutlineConstants
		{
			math::Mat4<> worldViewProj;
			math::Vec4<> outlineColor;
			math::Vec4<> screenParams; // width, height, outlineWidth, time
			math::Vec4<> padding;	   // Padding for 256-byte alignment
		};

		OutlineConstants constants;
		constants.worldViewProj = worldViewProj;
		constants.outlineColor = color;
		constants.screenParams = math::Vec4<>{ viewportSize.x, viewportSize.y, m_style.outlineWidth, getAnimationTime() };
		constants.padding = math::Vec4<>{ 0.0f, 0.0f, 0.0f, 0.0f };

		// Copy to constant buffer
		memcpy( m_constantBufferData, &constants, sizeof( constants ) );

		// Set constant buffer for rendering
		commandList->SetGraphicsRootConstantBufferView( 0, m_constantBuffer->GetGPUVirtualAddress() );
	}

	// Render the mesh with outline effect
	if ( meshRenderer->gpuMesh )
	{
		// Render all primitives in the mesh
		const std::uint32_t primitiveCount = meshRenderer->gpuMesh->getPrimitiveCount();
		for ( std::uint32_t i = 0; i < primitiveCount; ++i )
		{
			const auto &primitive = meshRenderer->gpuMesh->getPrimitive( i );

			// Set vertex and index buffers from the primitive
			const auto vertexBufferView = primitive.getVertexBufferView();
			const auto indexBufferView = primitive.getIndexBufferView();

			commandList->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
			commandList->IASetVertexBuffers( 0, 1, &vertexBufferView );
			commandList->IASetIndexBuffer( &indexBufferView );

			// Draw the primitive geometry
			const uint32_t indexCount = primitive.getIndexCount();
			commandList->DrawIndexedInstanced( indexCount, 1, 0, 0, 0 );
		}
	}
}

math::Mat4<> SelectionRenderer::getEntityWorldMatrix( ecs::Entity entity, ecs::Scene &scene ) const
{
	if ( auto *transform = scene.getComponent<components::Transform>( entity ) )
	{
		return transform->getLocalMatrix();
	}
	return math::Mat4<>::identity();
}

bool SelectionRenderer::entityHasRenderableMesh( ecs::Entity entity, ecs::Scene &scene ) const
{
	return scene.hasComponent<components::MeshRenderer>( entity );
}

float SelectionRenderer::getAnimationTime() const
{
	return static_cast<float>( runtime::time::getCurrentTime() ) * m_style.animationSpeed;
}

math::Vec4<> SelectionRenderer::animateColor( const math::Vec4<> &baseColor, float time ) const
{
	const float pulse = std::sin( time ) * 0.2f + 0.8f;
	return math::Vec4<>{
		baseColor.x * pulse,
		baseColor.y * pulse,
		baseColor.z * pulse,
		baseColor.w
	};
}

void SelectionRenderer::createRootSignature()
{
	if ( m_device.isValid() )
	{
		// Create root signature with one constant buffer parameter for rectangle constants
		D3D12_ROOT_PARAMETER rootParam = {};
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParam.Descriptor.ShaderRegister = 0;
		rootParam.Descriptor.RegisterSpace = 0;
		rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
		rootSigDesc.NumParameters = 1;
		rootSigDesc.pParameters = &rootParam;
		rootSigDesc.NumStaticSamplers = 0;
		rootSigDesc.pStaticSamplers = nullptr;
		rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		Microsoft::WRL::ComPtr<ID3DBlob> signature;
		Microsoft::WRL::ComPtr<ID3DBlob> error;

		HRESULT hr = D3D12SerializeRootSignature( &rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error );
		if ( FAILED( hr ) )
		{
			if ( error )
			{
				console::error( "Failed to serialize root signature: {}", reinterpret_cast<const char *>( error->GetBufferPointer() ) );
			}
			else
			{
				console::error( "Failed to create selection renderer root signature. hr is {}", hr );
			}
		}

		hr = m_device->CreateRootSignature( 0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS( &m_rootSignature ) );
		if ( FAILED( hr ) )
		{
			console::error( "Failed to create selection renderer root signature. hr is {}", hr );
		}
	}
}

void SelectionRenderer::createRectPipelineState()
{
	if ( m_device.isValid() )
	{
		// Check if shaders are compiled
		const auto *vsBlob = m_shaderManager.getShaderBlob( m_rectVertexShader );
		const auto *psBlob = m_shaderManager.getShaderBlob( m_rectPixelShader );

		if ( !vsBlob || !psBlob || !vsBlob->isValid() || !psBlob->isValid() )
		{
			console::warning( "Rectangle shaders not ready, will create pipeline state later" );
			return;
		}

		// Define input layout for rectangle vertices (just position)
		D3D12_INPUT_ELEMENT_DESC inputElements[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Create blend state for transparent rectangle overlay
		D3D12_BLEND_DESC blendDesc = {};
		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE;
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].LogicOpEnable = FALSE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		// Create rasterizer state
		D3D12_RASTERIZER_DESC rasterizerDesc = {};
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
		rasterizerDesc.FrontCounterClockwise = FALSE;
		rasterizerDesc.DepthBias = 0;
		rasterizerDesc.DepthBiasClamp = 0.0f;
		rasterizerDesc.SlopeScaledDepthBias = 0.0f;
		rasterizerDesc.DepthClipEnable = TRUE;
		rasterizerDesc.MultisampleEnable = FALSE;
		rasterizerDesc.AntialiasedLineEnable = FALSE;
		rasterizerDesc.ForcedSampleCount = 0;
		rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		// Disable depth testing for UI overlay
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
		depthStencilDesc.DepthEnable = FALSE;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		depthStencilDesc.StencilEnable = FALSE;

		// Create pipeline state
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS = { vsBlob->blob->GetBufferPointer(), vsBlob->blob->GetBufferSize() };
		psoDesc.PS = { psBlob->blob->GetBufferPointer(), psBlob->blob->GetBufferSize() };
		psoDesc.BlendState = blendDesc;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.RasterizerState = rasterizerDesc;
		psoDesc.DepthStencilState = depthStencilDesc;
		psoDesc.InputLayout = { inputElements, _countof( inputElements ) };
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN; // No depth buffer
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;

		HRESULT hr = m_device->CreateGraphicsPipelineState( &psoDesc, IID_PPV_ARGS( &m_rectPipelineState ) );
		if ( FAILED( hr ) )
		{
			console::error( "Failed to create rectangle pipeline state. hr was {}", hr );
		}
	}
}

void SelectionRenderer::createConstantBuffer()
{
	if ( m_device.isValid() )
	{
		// Create constant buffer for rectangle constants (256-byte aligned)
		const UINT constantBufferSize = ( sizeof( math::Vec4<> ) * 4 + 255 ) & ~255; // 4 Vec4s + alignment

		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask = 0;
		heapProps.VisibleNodeMask = 0;

		D3D12_RESOURCE_DESC bufferDesc = {};
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Width = constantBufferSize;
		bufferDesc.Height = 1;
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.MipLevels = 1;
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.SampleDesc.Quality = 0;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		HRESULT hr = m_device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS( &m_constantBuffer ) );

		if ( FAILED( hr ) )
		{
			console::error( "Failed to create selection renderer constant buffer. hr was {}", hr );
		}

		// Map the constant buffer for CPU access
		D3D12_RANGE readRange = { 0, 0 }; // We won't read from this resource on the CPU
		hr = m_constantBuffer->Map( 0, &readRange, reinterpret_cast<void **>( &m_constantBufferData ) );
		if ( FAILED( hr ) )
		{
			console::error( "Failed to map selection renderer constant buffer. hr was {}", hr );
		}
	}
}

void SelectionRenderer::createRectVertexBuffer()
{
	if ( m_device.isValid() )
	{
		// Rectangle vertices in normalized device coordinates (full screen quad)
		struct RectVertex
		{
			float position[2];
		};

		const RectVertex vertices[] = {
			{ { -1.0f, -1.0f } }, // Bottom left
			{ { 1.0f, -1.0f } },  // Bottom right
			{ { 1.0f, 1.0f } },	  // Top right
			{ { -1.0f, 1.0f } }	  // Top left
		};

		const uint16_t indices[] = {
			0, 1, 2, // First triangle
			0,
			2,
			3 // Second triangle
		};

		// Create vertex buffer
		const UINT vertexBufferSize = sizeof( vertices );

		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask = 0;
		heapProps.VisibleNodeMask = 0;

		D3D12_RESOURCE_DESC bufferDesc = {};
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Width = vertexBufferSize;
		bufferDesc.Height = 1;
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.MipLevels = 1;
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.SampleDesc.Quality = 0;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		HRESULT hr = m_device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS( &m_rectVertexBuffer ) );

		if ( FAILED( hr ) )
		{
			console::error( "Failed to create rectangle vertex buffer. hr was {}", hr );
		}

		// Copy vertex data
		void *pVertexDataBegin;
		D3D12_RANGE readRange = { 0, 0 };
		hr = m_rectVertexBuffer->Map( 0, &readRange, &pVertexDataBegin );
		if ( FAILED( hr ) )
		{
			console::error( "Failed to map rectangle vertex buffer. hr was {}", hr );
		}
		memcpy( pVertexDataBegin, vertices, vertexBufferSize );
		m_rectVertexBuffer->Unmap( 0, nullptr );

		// Initialize vertex buffer view
		m_rectVertexBufferView.BufferLocation = m_rectVertexBuffer->GetGPUVirtualAddress();
		m_rectVertexBufferView.StrideInBytes = sizeof( RectVertex );
		m_rectVertexBufferView.SizeInBytes = vertexBufferSize;

		// Create index buffer
		const UINT indexBufferSize = sizeof( indices );
		bufferDesc.Width = indexBufferSize;

		hr = m_device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS( &m_rectIndexBuffer ) );

		if ( FAILED( hr ) )
		{
			console::error( "Failed to create rectangle index buffer. hr was {}", hr );
		}

		// Copy index data
		void *pIndexDataBegin;
		hr = m_rectIndexBuffer->Map( 0, &readRange, &pIndexDataBegin );
		if ( FAILED( hr ) )
		{
			console::error( "Failed to map rectangle index buffer. hr was {}", hr );
		}
		memcpy( pIndexDataBegin, indices, indexBufferSize );
		m_rectIndexBuffer->Unmap( 0, nullptr );

		// Initialize index buffer view
		m_rectIndexBufferView.BufferLocation = m_rectIndexBuffer->GetGPUVirtualAddress();
		m_rectIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
		m_rectIndexBufferView.SizeInBytes = indexBufferSize;
	}
}

void SelectionRenderer::createOutlinePipelineState()
{
	if ( m_device.isValid() )
	{
		// Check if shaders are compiled
		const auto *vsBlob = m_shaderManager.getShaderBlob( m_outlineVertexShader );
		const auto *psBlob = m_shaderManager.getShaderBlob( m_outlinePixelShader );

		if ( !vsBlob || !psBlob || !vsBlob->isValid() || !psBlob->isValid() )
		{
			console::warning( "Outline shaders not ready, will create pipeline state later" );
			return;
		}

		// Define input layout for outline vertices (position, normal, UV)
		D3D12_INPUT_ELEMENT_DESC inputElements[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Create blend state for outline rendering (no blending, replace)
		D3D12_BLEND_DESC blendDesc = {};
		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE;
		blendDesc.RenderTarget[0].BlendEnable = FALSE;
		blendDesc.RenderTarget[0].LogicOpEnable = FALSE;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		// Create rasterizer state for outline (wireframe or inflated geometry)
		D3D12_RASTERIZER_DESC rasterizerDesc = {};
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
		rasterizerDesc.FrontCounterClockwise = FALSE;
		rasterizerDesc.DepthBias = 0;
		rasterizerDesc.DepthBiasClamp = 0.0f;
		rasterizerDesc.SlopeScaledDepthBias = 0.0f;
		rasterizerDesc.DepthClipEnable = TRUE;
		rasterizerDesc.MultisampleEnable = FALSE;
		rasterizerDesc.AntialiasedLineEnable = FALSE;
		rasterizerDesc.ForcedSampleCount = 0;
		rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		// Enable depth testing for 3D outline rendering
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
		depthStencilDesc.DepthEnable = TRUE;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		depthStencilDesc.StencilEnable = FALSE;

		// Create pipeline state
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS = { vsBlob->blob->GetBufferPointer(), vsBlob->blob->GetBufferSize() };
		psoDesc.PS = { psBlob->blob->GetBufferPointer(), psBlob->blob->GetBufferSize() };
		psoDesc.BlendState = blendDesc;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.RasterizerState = rasterizerDesc;
		psoDesc.DepthStencilState = depthStencilDesc;
		psoDesc.InputLayout = { inputElements, _countof( inputElements ) };
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT; // Depth buffer
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;

		HRESULT hr = m_device->CreateGraphicsPipelineState( &psoDesc, IID_PPV_ARGS( &m_outlinePipelineState ) );
		if ( FAILED( hr ) )
		{
			console::error( "Failed to create outline pipeline state. hr was {}", hr );
		}
	}
}

} // namespace editor