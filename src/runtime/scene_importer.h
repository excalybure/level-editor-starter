#pragma once

#include "engine/assets/assets.h"
#include "runtime/entity.h"

namespace ecs
{
class Scene;
}

namespace engine
{
class GPUResourceManager;
}

#include <memory>

namespace runtime
{

class SceneImporter
{
public:
	/// @brief Import a scene from assets into ECS scene
	/// @param assetScene The asset scene to import
	/// @param targetScene The ECS scene to populate
	/// @return true if import was successful
	static bool importScene( std::shared_ptr<assets::Scene> assetScene, ecs::Scene &targetScene );

	/// @brief Create GPU resources for an already-imported scene
	/// @param assetScene The original asset scene used for reference
	/// @param targetScene The ECS scene with imported entities
	/// @param gpuResourceManager GPU resource manager for creating GPU resources
	/// @return true if GPU resource creation was successful
	static bool createGPUResources( std::shared_ptr<assets::Scene> assetScene, ecs::Scene &targetScene, engine::GPUResourceManager &gpuResourceManager );

private:
	/// @brief Recursively import a scene node and its children
	/// @param assetScene The asset scene containing meshes and other data
	/// @param node The scene node to import
	/// @param targetScene The ECS scene to populate
	/// @param parent The parent entity (invalid for root nodes)
	/// @return The created entity
	static ecs::Entity importNode( std::shared_ptr<assets::Scene> assetScene, const assets::SceneNode &node, ecs::Scene &targetScene, ecs::Entity parent = {} );

	/// @brief Setup Transform component from scene node transform
	/// @param node The scene node with transform data
	/// @param entity The entity to attach the component to
	/// @param targetScene The ECS scene
	static void setupTransformComponent( const assets::SceneNode &node, ecs::Entity entity, ecs::Scene &targetScene );

	/// @brief Setup MeshRenderer component (CPU-only)
	/// @param meshHandle The mesh handle to create renderer for
	/// @param entity The entity to attach the component to
	/// @param targetScene The ECS scene
	/// @param assetScene The source asset scene for resource creation
	static void setupMeshRenderer( assets::MeshHandle meshHandle, ecs::Entity entity, ecs::Scene &targetScene, std::shared_ptr<assets::Scene> assetScene );
};

} // namespace runtime
