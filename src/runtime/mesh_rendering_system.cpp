module runtime.mesh_rendering_system;

namespace runtime::systems
{

MeshRenderingSystem::MeshRenderingSystem( renderer::Renderer &renderer )
	: m_renderer( renderer )
{
}

void MeshRenderingSystem::update( ecs::Scene &scene, float deltaTime )
{
	// The mesh rendering system doesn't need to update per frame
	// Rendering happens in the render() method
}

void MeshRenderingSystem::render( ecs::Scene &scene, const camera::Camera &camera )
{
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

	// Calculate MVP matrix for this entity
	const auto mvpMatrix = calculateMVPMatrix( transform, camera );

	// TODO: Implement actual GPU rendering pipeline
	// For now, we validate that we have the necessary resources
	// Future implementation will:
	// 1. Use PrimitiveGPU.bindForRendering() for each primitive
	// 2. Call renderer draw commands directly on command list
	// 3. Handle materials via MaterialGPU.bindToCommandList()

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

		// TODO: Bind primitive for rendering and issue draw call
		// primitive.bindForRendering(commandList);
		// commandList->DrawIndexedInstanced(primitive.getIndexCount(), 1, 0, 0, 0);
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

} // namespace runtime::systems