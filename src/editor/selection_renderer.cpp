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
	const math::Mat4<> &projMatrix )
{
	if ( !commandList )
	{
		// Headless mode - just return
		return;
	}

	// Render selection outlines for all selected entities
	renderSelectionOutlines( scene, commandList, viewMatrix, projMatrix );
}

void SelectionRenderer::renderSelectionOutlines( ecs::Scene &scene,
	ID3D12GraphicsCommandList *commandList,
	const math::Mat4<> &viewMatrix,
	const math::Mat4<> &projMatrix )
{
	if ( !commandList )
	{
		return;
	}

	// Check if shaders are ready
	const auto *vsBlob = m_shaderManager.getShaderBlob( m_outlineVertexShader );
	const auto *psBlob = m_shaderManager.getShaderBlob( m_outlinePixelShader );

	if ( !vsBlob || !psBlob || !vsBlob->isValid() || !psBlob->isValid() )
	{
		console::warning( "Selection outline shaders not ready, skipping render" );
		return;
	}

	// Set pipeline state for outline rendering
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

		renderEntityOutline( entity, scene, outlineColor, commandList, viewMatrix, projMatrix );
	} );
}

void SelectionRenderer::renderHoverHighlight( ecs::Entity hoveredEntity,
	ecs::Scene &scene,
	ID3D12GraphicsCommandList *commandList,
	const math::Mat4<> &viewMatrix,
	const math::Mat4<> &projMatrix )
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

	// Skip entities without renderable meshes
	if ( !entityHasRenderableMesh( hoveredEntity, scene ) )
	{
		return;
	}

	// Set pipeline state for hover rendering
	if ( m_outlinePipelineState )
	{
		commandList->SetPipelineState( m_outlinePipelineState.Get() );
	}

	if ( m_rootSignature )
	{
		commandList->SetGraphicsRootSignature( m_rootSignature.Get() );
	}

	renderEntityOutline( hoveredEntity, scene, m_style.hoveredColor, commandList, viewMatrix, projMatrix );
}

void SelectionRenderer::renderRectSelection( const math::Vec2<> &startPos,
	const math::Vec2<> &endPos,
	ID3D12GraphicsCommandList *commandList )
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

	// TODO: Update constant buffer with rectangle bounds and render quad
	// For now, this is a placeholder implementation
	console::info( "Rendering rectangle selection from ({}, {}) to ({}, {})",
		startPos.x,
		startPos.y,
		endPos.x,
		endPos.y );
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

		console::info( "Selection renderer shaders registered with ShaderManager" );
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
	const math::Mat4<> &projMatrix )
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

	console::info( "Rendering outline for entity {}.{} with color ({:.2f}, {:.2f}, {:.2f}, {:.2f}) using ShaderManager",
		entity.id,
		entity.generation,
		color.x,
		color.y,
		color.z,
		color.w );
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

} // namespace editor