export module runtime.scene_importer;

import std;
import runtime.ecs;
import runtime.entity;
import runtime.components;
import engine.assets;

export namespace runtime
{

class SceneImporter
{
public:
	/// @brief Import a scene using the non-GPU path for tests/headless scenarios
	/// @param assetScene The asset scene to import
	/// @param targetScene The ECS scene to populate
	/// @return true if import was successful
	static bool importScene( std::shared_ptr<assets::Scene> assetScene, ecs::Scene &targetScene );

	/// @brief Import a scene using the GPU-enabled path
	/// @param assetScene The asset scene to import
	/// @param targetScene The ECS scene to populate
	/// @return true if import was successful
	static bool importSceneWithGPU( std::shared_ptr<assets::Scene> assetScene, ecs::Scene &targetScene );

private:
	/// @brief Recursively import a scene node and its children
	/// @param node The scene node to import
	/// @param targetScene The ECS scene to populate
	/// @param parent The parent entity (invalid for root nodes)
	/// @return The created entity
	static ecs::Entity importNode( const assets::SceneNode &node, ecs::Scene &targetScene, ecs::Entity parent = {} );

	/// @brief Setup Transform component from scene node transform
	/// @param node The scene node with transform data
	/// @param entity The entity to attach the component to
	/// @param targetScene The ECS scene
	static void setupTransformComponent( const assets::SceneNode &node, ecs::Entity entity, ecs::Scene &targetScene );

	/// @brief Setup MeshRenderer component using string paths (current approach)
	/// @param node The scene node with mesh data
	/// @param entity The entity to attach the component to
	/// @param targetScene The ECS scene
	/// @param assetScene The source asset scene for path resolution
	static void setupMeshRendererPaths( const assets::SceneNode &node, ecs::Entity entity, ecs::Scene &targetScene, std::shared_ptr<assets::Scene> assetScene );

	/// @brief Setup MeshRenderer component using GPU resources (placeholder)
	/// @param node The scene node with mesh data
	/// @param entity The entity to attach the component to
	/// @param targetScene The ECS scene
	/// @param assetScene The source asset scene for resource creation
	static void setupMeshRendererWithGPU( const assets::SceneNode &node, ecs::Entity entity, ecs::Scene &targetScene, std::shared_ptr<assets::Scene> assetScene );
};

} // namespace runtime