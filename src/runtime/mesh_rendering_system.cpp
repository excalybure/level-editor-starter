module;

#include <d3d12.h>
#include <wrl.h>
#include <cstring>

module runtime.mesh_rendering_system;

import engine.gpu.material_gpu;
import runtime.console;

namespace runtime::systems
{

MeshRenderingSystem::MeshRenderingSystem( renderer::Renderer &renderer, std::shared_ptr<shader_manager::ShaderManager> shaderManager )
	: m_renderer( renderer ), m_shaderManager( shaderManager )
{
	createRootSignature();

	if ( !registerShaders() )
	{
		console::error( "MeshRenderingSystem: Failed to register shaders" );
	}
}

MeshRenderingSystem::MeshRenderingSystem( renderer::Renderer &renderer )
	: m_renderer( renderer ), m_shaderManager( nullptr )
{
	createRootSignature();
	console::warning( "MeshRenderingSystem: Created without ShaderManager - shader compilation will use legacy D3DCompileFromFile method" );
}

void MeshRenderingSystem::update( ecs::Scene &scene, float deltaTime )
{
	// The mesh rendering system doesn't need to update per frame
	// Rendering happens in the render() method
}

bool MeshRenderingSystem::registerShaders()
{
	if ( !m_shaderManager )
	{
		console::error( "MeshRenderingSystem: ShaderManager not available for shader registration" );
		return false;
	}

	// Register vertex shader
	m_vertexShaderHandle = m_shaderManager->registerShader(
		"shaders/unlit.hlsl",
		"VSMain",
		"vs_5_0",
		shader_manager::ShaderType::Vertex );

	if ( m_vertexShaderHandle == shader_manager::INVALID_SHADER_HANDLE )
	{
		console::error( "MeshRenderingSystem: Failed to register vertex shader" );
		return false;
	}

	// Register pixel shader
	m_pixelShaderHandle = m_shaderManager->registerShader(
		"shaders/unlit.hlsl",
		"PSMain",
		"ps_5_0",
		shader_manager::ShaderType::Pixel );

	if ( m_pixelShaderHandle == shader_manager::INVALID_SHADER_HANDLE )
	{
		console::error( "MeshRenderingSystem: Failed to register pixel shader" );
		return false;
	}

	// Set up reload callback for shader hot reloading
	m_callbackHandle = m_shaderManager->registerReloadCallback(
		[this]( shader_manager::ShaderHandle handle, const renderer::ShaderBlob &newShader ) {
			// When shaders are reloaded, invalidate pipeline state cache
			if ( handle == m_vertexShaderHandle || handle == m_pixelShaderHandle )
			{
				console::info( "MeshRenderingSystem: Shader reloaded, clearing pipeline state cache" );
				m_pipelineStateCache.clear();
			}
		} );

	if ( m_callbackHandle == shader_manager::INVALID_CALLBACK_HANDLE )
	{
		console::warning( "MeshRenderingSystem: Failed to register shader reload callback" );
	}

	return true;
}

void MeshRenderingSystem::render( ecs::Scene &scene, const camera::Camera &camera )
{
	// Get command context for binding
	auto *commandContext = m_renderer.getCommandContext();
	if ( !commandContext )
	{
		return;
	}

	auto *commandList = commandContext->get();
	if ( !commandList )
	{
		return;
	}

	// Iterate through all entities to find those with both MeshRenderer and Transform components
	const auto allEntities = scene.getAllEntities();
	for ( const auto entity : allEntities )
	{
		if ( !entity.isValid() )
		{
			continue;
		}

		// Check if entity has both required components
		const auto *transform = scene.getComponent<components::Transform>( entity );
		const auto *meshRenderer = scene.getComponent<components::MeshRenderer>( entity );

		if ( transform && meshRenderer )
		{
			renderEntity( *transform, *meshRenderer, camera );
		}
	}
}

void MeshRenderingSystem::renderEntity( const components::Transform &transform,
	const components::MeshRenderer &meshRenderer,
	const camera::Camera &camera )
{
	// Early return if no GPU mesh available
	if ( !meshRenderer.gpuMesh )
	{
		// This is expected for entities that haven't been processed by GPUResourceManager yet
		return;
	}

	// Get command context for direct GPU commands
	auto *commandContext = m_renderer.getCommandContext();
	if ( !commandContext )
	{
		// No active command context (e.g., during headless tests)
		return;
	}

	auto *commandList = commandContext->get();
	if ( !commandList )
	{
		return;
	}

	// Calculate object constants for this entity
	ObjectConstants objectConstants;
	objectConstants.worldMatrix = transform.getLocalMatrix();
	// For normal matrix, we need the inverse transpose of the world matrix
	// For uniform scaling, we can use the world matrix directly
	// TODO: Implement proper inverse transpose calculation for non-uniform scaling
	objectConstants.normalMatrix = objectConstants.worldMatrix;

	// Bind object constants to register b1 using root constants
	commandList->SetGraphicsRoot32BitConstants( 1, sizeof( ObjectConstants ) / 4, &objectConstants, 0 );

	const auto &gpuMesh = *meshRenderer.gpuMesh;
	if ( !gpuMesh.isValid() )
	{
		return;
	}

	// Iterate through all primitives in the mesh
	for ( std::uint32_t i = 0; i < gpuMesh.getPrimitiveCount(); ++i )
	{
		const auto &primitive = gpuMesh.getPrimitive( i );
		if ( !primitive.isValid() )
		{
			continue;
		}

		// Bind primitive GPU buffers and material (material binding is handled by primitive)
		primitive.bindForRendering( commandList );

		// Get and set the appropriate pipeline state for the material
		if ( primitive.hasMaterial() )
		{
			auto *pipelineState = getMaterialPipelineState( *primitive.getMaterial() );
			if ( pipelineState )
			{
				commandList->SetPipelineState( pipelineState );
				// Issue the draw call
				if ( primitive.hasIndexBuffer() )
				{
					// Indexed drawing
					commandList->DrawIndexedInstanced( primitive.getIndexCount(), 1, 0, 0, 0 );
				}
				else
				{
					// Non-indexed drawing
					commandList->DrawInstanced( primitive.getVertexCount(), 1, 0, 0 );
				}
			}
		}
	}
}

math::Mat4<> MeshRenderingSystem::calculateMVPMatrix(
	const components::Transform &transform,
	const camera::Camera &camera )
{
	// Get model matrix from transform
	const auto modelMatrix = transform.getLocalMatrix();

	// Get view matrix from camera
	const auto viewMatrix = camera.getViewMatrix();

	// Get projection matrix from camera (assuming 16:9 aspect ratio for now)
	const auto projectionMatrix = camera.getProjectionMatrix( 16.0f / 9.0f );

	// Calculate MVP matrix: Projection * View * Model
	return projectionMatrix * viewMatrix * modelMatrix;
}

void MeshRenderingSystem::createRootSignature()
{
	// Get device from renderer for root signature creation
	auto &device = m_renderer.getDevice();

	// Create root signature matching unlit.hlsl shader expectations
	// b0 - Frame constants (view/projection matrices) - using CBV (too large for root constants)
	// b1 - Object constants (world matrix) - using root constants for better performance
	// b2 - Material constants (base color, etc.) - using CBV
	D3D12_ROOT_PARAMETER rootParams[3] = {};

	// Frame constants (b0) - using CBV since it's too large for root constants (68 DWORDs > 64 limit)
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[0].Descriptor.ShaderRegister = 0; // b0
	rootParams[0].Descriptor.RegisterSpace = 0;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// Object constants (b1) - using 32-bit constants for better performance (32 DWORDs fits in limit)
	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParams[1].Constants.ShaderRegister = 1; // b1
	rootParams[1].Constants.RegisterSpace = 0;
	rootParams[1].Constants.Num32BitValues = sizeof( ObjectConstants ) / 4; // Convert bytes to 32-bit values
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// Material constants (b2) - keep as CBV for traditional constant buffer
	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[2].Descriptor.ShaderRegister = 2; // b2
	rootParams[2].Descriptor.RegisterSpace = 0;
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.NumParameters = 3;
	rootSigDesc.pParameters = rootParams;
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
	HRESULT hr = D3D12SerializeRootSignature( &rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, &errorBlob );
	if ( SUCCEEDED( hr ) )
	{
		hr = device->CreateRootSignature( 0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS( &m_rootSignature ) );
	}
}

ID3D12PipelineState *MeshRenderingSystem::getMaterialPipelineState( const engine::gpu::MaterialGPU &material )
{
	// Generate cache key based on material properties
	auto *sourceMaterial = material.getSourceMaterial().get();
	if ( !sourceMaterial )
	{
		return nullptr;
	}

	const std::string cacheKey = sourceMaterial->getPath();

	// Check if pipeline state is already cached
	auto it = m_pipelineStateCache.find( cacheKey );
	if ( it != m_pipelineStateCache.end() )
	{
		return it->second.Get();
	}

	// Create new pipeline state for this material
	auto pipelineState = createMaterialPipelineState( material );
	if ( pipelineState )
	{
		m_pipelineStateCache[cacheKey] = pipelineState;
		return pipelineState.Get();
	}

	return nullptr;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> MeshRenderingSystem::createMaterialPipelineState( const engine::gpu::MaterialGPU &material )
{
	// Get device from renderer
	auto &device = m_renderer.getDevice();

	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> psBlob;

	// Use ShaderManager if available, otherwise fall back to D3DCompileFromFile
	if ( m_shaderManager )
	{
		// Get current shader blobs from shader manager
		const renderer::ShaderBlob *vertexShader = m_shaderManager->getShaderBlob( m_vertexShaderHandle );
		const renderer::ShaderBlob *pixelShader = m_shaderManager->getShaderBlob( m_pixelShaderHandle );

		if ( !vertexShader || !pixelShader || !vertexShader->isValid() || !pixelShader->isValid() )
		{
			console::warning( "MeshRenderingSystem: Shaders not ready for pipeline state creation" );
			return nullptr;
		}

		vsBlob = vertexShader->blob;
		psBlob = pixelShader->blob;
	}
	else
	{
		console::error( "MeshRenderingSystem: Shader manager is not set. Skipping pipeline state creation" );
		return nullptr;
	}

	// Create pipeline state
	const D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputLayout, _countof( inputLayout ) };
	psoDesc.pRootSignature = m_rootSignature.Get(); // Use our managed root signature
	psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
	psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	// Depth stencil state (disable depth testing for viewport rendering)
	psoDesc.DepthStencilState.DepthEnable = FALSE; // Disable depth testing
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	psoDesc.DepthStencilState.StencilEnable = FALSE;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
	const HRESULT hr = device->CreateGraphicsPipelineState( &psoDesc, IID_PPV_ARGS( &pipelineState ) );
	if ( FAILED( hr ) )
	{
		console::error( "MeshRenderingSystem: Failed to create pipeline state for material" );
		return nullptr;
	}
	return pipelineState;
}

void MeshRenderingSystem::setRootSignature( ID3D12GraphicsCommandList *commandList )
{
	if ( m_rootSignature && commandList )
	{
		commandList->SetGraphicsRootSignature( m_rootSignature.Get() );
	}
}

} // namespace runtime::systems