module runtime.scene_importer;

import runtime.ecs;
import runtime.entity;
import runtime.components;
import engine.assets;
import engine.gpu.mesh_gpu;

using namespace runtime;
using namespace ecs;
using namespace components;
using namespace assets;

bool SceneImporter::importScene( std::shared_ptr<assets::Scene> assetScene, ecs::Scene &targetScene )
{
	if ( !assetScene || !assetScene->isLoaded() )
	{
		return false;
	}

	// Import all root nodes recursively (CPU-only)
	for ( const auto &rootNode : assetScene->getRootNodes() )
	{
		importNode( assetScene, *rootNode, targetScene, Entity{} );
	}

	return true;
}

bool SceneImporter::createGPUResources( std::shared_ptr<assets::Scene> assetScene, ecs::Scene &targetScene, engine::GPUResourceManager &gpuResourceManager )
{
	if ( !assetScene || !assetScene->isLoaded() )
	{
		return false;
	}

	// Traverse all entities and populate GPU resources for MeshRenderer components that don't have them
	const auto entities = targetScene.getAllEntities();
	for ( const Entity entity : entities )
	{
		if ( !entity.isValid() )
			continue;

		auto *meshRenderer = targetScene.getComponent<MeshRenderer>( entity );
		if ( !meshRenderer || meshRenderer->gpuMesh != nullptr )
			continue; // Skip if no MeshRenderer or already has GPU resources

		// Use the stored mesh handle to get the correct mesh
		const auto mesh = assetScene->getMesh( meshRenderer->meshHandle );
		if ( mesh )
		{
			auto gpuMesh = gpuResourceManager.getMeshGPU( mesh );
			meshRenderer->gpuMesh = std::move( gpuMesh );
		}
	}

	return true;
}

Entity SceneImporter::importNode( std::shared_ptr<assets::Scene> assetScene, const assets::SceneNode &node, ecs::Scene &targetScene, Entity parent )
{
	// Create entity with node name
	Entity entity = targetScene.createEntity( node.getName() );

	// Set parent relationship if provided
	if ( parent.isValid() )
	{
		targetScene.setParent( entity, parent );
	}

	// Setup Transform component if node has transform data
	if ( node.hasTransform() )
	{
		setupTransformComponent( node, entity, targetScene );
	}

	// Setup MeshRenderer component if node has meshes (CPU-only)
	if ( node.hasMeshHandles() )
	{
		// For this implementation, we'll create one MeshRenderer per mesh handle
		// This matches the current test expectations
		node.foreachMeshHandle( [&]( MeshHandle meshHandle ) {
			// Create a separate entity for each mesh if multiple meshes exist
			// or use the same entity for the first mesh
			Entity meshEntity = ( node.meshCount() == 1 ) ? entity : targetScene.createEntity( node.getName() + "_Mesh" );

			if ( meshEntity != entity && parent.isValid() )
			{
				targetScene.setParent( meshEntity, parent );
			}
			else if ( meshEntity != entity )
			{
				targetScene.setParent( meshEntity, entity );
			}

			// Setup MeshRenderer (CPU-only)
			setupMeshRenderer( meshHandle, meshEntity, targetScene, assetScene );
		} );
	}

	// Recursively import children
	node.foreachChild( [&]( const SceneNode &child ) {
		importNode( assetScene, child, targetScene, entity );
	} );

	return entity;
}


void SceneImporter::setupTransformComponent( const assets::SceneNode &node, Entity entity, ecs::Scene &targetScene )
{
	const auto &nodeTransform = node.getTransform();

	components::Transform ecsTransform;
	ecsTransform.position = { nodeTransform.position.x, nodeTransform.position.y, nodeTransform.position.z };
	ecsTransform.rotation = { nodeTransform.rotation.x, nodeTransform.rotation.y, nodeTransform.rotation.z };
	ecsTransform.scale = { nodeTransform.scale.x, nodeTransform.scale.y, nodeTransform.scale.z };

	targetScene.addComponent( entity, ecsTransform );
}

void SceneImporter::setupMeshRenderer( assets::MeshHandle meshHandle, Entity entity, ecs::Scene &targetScene, std::shared_ptr<assets::Scene> assetScene )
{
	const auto mesh = assetScene->getMesh( meshHandle );
	if ( mesh )
	{
		MeshRenderer renderer( meshHandle );

		renderer.bounds = mesh->getBounds();
		targetScene.addComponent( entity, renderer );
	}
}
