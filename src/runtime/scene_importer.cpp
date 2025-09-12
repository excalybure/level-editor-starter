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

	// Import all root nodes recursively
	for ( const auto &rootNode : assetScene->getRootNodes() )
	{
		importNode( *assetScene, *rootNode, targetScene, Entity{} );
	}

	return true;
}

bool SceneImporter::importSceneWithGPU( std::shared_ptr<assets::Scene> assetScene, ecs::Scene &targetScene, engine::GPUResourceManager &gpuResourceManager )
{
	// AF2: Import scene using GPU resource creation path
	if ( !assetScene || !assetScene->isLoaded() )
	{
		return false;
	}

	// Import all root nodes recursively using GPU path
	for ( const auto &rootNode : assetScene->getRootNodes() )
	{
		importNodeWithGPU( *assetScene, *rootNode, targetScene, Entity{}, assetScene, gpuResourceManager );
	}

	return true;
}

Entity SceneImporter::importNode( const assets::Scene &assetScene, const assets::SceneNode &node, ecs::Scene &targetScene, Entity parent )
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

	// Setup MeshRenderer component if node has meshes
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

			// Create MeshRenderer and set bounds from mesh if available
			MeshRenderer renderer;

			// Get mesh from asset scene and set bounds if available
			const auto mesh = assetScene.getMesh( meshHandle );
			if ( mesh && mesh->hasBounds() )
			{
				renderer.bounds = mesh->getBounds();
			}

			targetScene.addComponent( meshEntity, renderer );
		} );
	}

	// Recursively import children
	node.foreachChild( [&]( const SceneNode &child ) {
		importNode( assetScene, child, targetScene, entity );
	} );

	return entity;
}

Entity SceneImporter::importNodeWithGPU( const assets::Scene &assetScene, const assets::SceneNode &node, ecs::Scene &targetScene, Entity parent, std::shared_ptr<assets::Scene> sharedAssetScene, engine::GPUResourceManager &gpuResourceManager )
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

	// Setup MeshRenderer component if node has meshes using GPU resources
	if ( node.hasMeshHandles() )
	{
		// For this implementation, we'll create one MeshRenderer per mesh handle
		// This matches the current test expectations but now uses GPU resources
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

			// Setup MeshRenderer with GPU resources
			setupMeshRendererWithGPU( node, meshEntity, targetScene, sharedAssetScene, gpuResourceManager );
		} );
	}

	// Recursively import children
	node.foreachChild( [&]( const SceneNode &child ) {
		importNodeWithGPU( assetScene, child, targetScene, entity, sharedAssetScene, gpuResourceManager );
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

void SceneImporter::setupMeshRendererPaths( const assets::SceneNode &node, Entity entity, ecs::Scene &targetScene, std::shared_ptr<assets::Scene> assetScene )
{
	// This method is currently not used in the basic import path
	// It would be used for string-path based mesh renderer setup in more complex scenarios
	// For now, we keep the basic approach in importNode
	(void)node;
	(void)entity;
	(void)targetScene;
	(void)assetScene;
}

void SceneImporter::setupMeshRendererWithGPU( const assets::SceneNode &node, Entity entity, ecs::Scene &targetScene, std::shared_ptr<assets::Scene> assetScene, engine::GPUResourceManager &gpuResourceManager )
{
	// AF2: Setup MeshRenderer with actual GPU resources via GPUResourceManager

	// Create MeshRenderer component
	MeshRenderer renderer;

	// Get the mesh handle from the node (assume first handle for now)
	if ( node.hasMeshHandles() )
	{
		// Process the first mesh handle (nodes with multiple meshes would create multiple entities)
		const auto meshHandles = node.getMeshHandles();
		if ( !meshHandles.empty() )
		{
			const MeshHandle meshHandle = meshHandles[0];

			// Get the mesh from the asset scene
			const auto mesh = assetScene->getMesh( meshHandle );
			if ( mesh )
			{
				// Set bounds from mesh if available
				if ( mesh->hasBounds() )
				{
					renderer.bounds = mesh->getBounds();
				}

				// Create GPU mesh using GPUResourceManager
				auto gpuMesh = gpuResourceManager.getMeshGPU( mesh );
				renderer.gpuMesh = std::move( gpuMesh );

				// Note: MeshRenderer currently only supports gpuMesh, not materials
				// Material handling would need to be added to MeshRenderer or handled separately
			}
		}
	}

	// Add the MeshRenderer component to the entity
	targetScene.addComponent( entity, renderer );
}