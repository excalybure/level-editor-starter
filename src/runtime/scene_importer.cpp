#include "scene_importer.h"
#include "entity.h"
#include "ecs.h"
#include "components.h"
#include "engine/assets/assets.h"
#include "engine/gpu/mesh_gpu.h"
#include "engine/gpu/gpu_resource_manager.h"

using namespace runtime;
using namespace ecs;

bool SceneImporter::importScene( std::shared_ptr<assets::Scene> assetScene, ecs::Scene &targetScene )
{
	if ( !assetScene || !assetScene->isLoaded() )
	{
		return false;
	}

	// Import all root nodes recursively (CPU-only)
	for ( const auto &rootNode : assetScene->getRootNodes() )
	{
		importNode( assetScene, *rootNode, targetScene, ecs::Entity{} );
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

		auto *meshRenderer = targetScene.getComponent<components::MeshRenderer>( entity );
		if ( !meshRenderer || meshRenderer->gpuMesh != nullptr )
			continue; // Skip if no MeshRenderer or already has GPU resources

		// Use the stored mesh handle to get the correct mesh
		const auto mesh = assetScene->getMesh( meshRenderer->meshHandle );
		if ( mesh )
		{
			auto gpuMesh = gpuResourceManager.getMeshGPU( mesh );
			if ( gpuMesh )
			{
				gpuMesh->configureMaterials( gpuResourceManager, *assetScene, *mesh );
				meshRenderer->gpuMesh = std::move( gpuMesh );
			}
		}
	}

	return true;
}

ecs::Entity SceneImporter::importNode( std::shared_ptr<assets::Scene> assetScene, const assets::SceneNode &node, ecs::Scene &targetScene, ecs::Entity parent )
{
	// Create entity with node name
	ecs::Entity entity = targetScene.createEntity( node.getName() );

	// Set parent relationship if provided
	if ( parent.isValid() )
	{
		targetScene.setParent( entity, parent );
	}

	// Setup Transform component (always add, use node data if available)
	setupTransformComponent( node, entity, targetScene );

	// Setup MeshRenderer component if node has meshes (CPU-only)
	if ( node.hasMeshHandles() )
	{
		// For this implementation, we'll create one MeshRenderer per mesh handle
		// This matches the current test expectations
		node.foreachMeshHandle( [&]( assets::MeshHandle meshHandle ) {
			// Create a separate entity for each mesh if multiple meshes exist
			// or use the same entity for the first mesh
			ecs::Entity meshEntity = ( node.meshCount() == 1 ) ? entity : targetScene.createEntity( node.getName() + "_Mesh" );

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
	node.foreachChild( [&]( const assets::SceneNode &child ) {
		importNode( assetScene, child, targetScene, entity );
	} );

	return entity;
}


void SceneImporter::setupTransformComponent( const assets::SceneNode &node, ecs::Entity entity, ecs::Scene &targetScene )
{
	// Create Transform component with default values
	components::Transform ecsTransform{};

	// Update with node data if available
	if ( node.hasTransform() )
	{
		const auto &nodeTransform = node.getTransform();
		ecsTransform.position = nodeTransform.position;
		ecsTransform.rotation = nodeTransform.rotation;
		ecsTransform.scale = nodeTransform.scale;
	}

	// Add component to entity
	const bool success = targetScene.addComponent( entity, ecsTransform );

	// Fallback: if explicit template failed, try alternative approach
	if ( !success )
	{
		// This shouldn't happen but let's try anyway
		components::Transform fallbackTransform{};
		targetScene.addComponent<components::Transform>( entity, fallbackTransform );
	}
}

void SceneImporter::setupMeshRenderer( assets::MeshHandle meshHandle, ecs::Entity entity, ecs::Scene &targetScene, std::shared_ptr<assets::Scene> assetScene )
{
	const auto mesh = assetScene->getMesh( meshHandle );
	if ( mesh )
	{
		components::MeshRenderer renderer( meshHandle );

		// Populate meshPath for scene serialization portability
		// First try to use the mesh's own path
		const std::string meshPath = mesh->getPath();
		if ( !meshPath.empty() )
		{
			renderer.meshPath = meshPath;
		}
		else
		{
			// Fall back to the asset scene's path if mesh has no path
			renderer.meshPath = assetScene->getPath();
		}

		renderer.bounds = mesh->getBounds();
		targetScene.addComponent( entity, renderer );
	}
}
