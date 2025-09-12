export module runtime.scene_importer;

import std;
import runtime.ecs;
import runtime.entity;
import runtime.components;
import engine.assets;
import engine.gpu.gpu_resource_manager;

export namespace runtime
{

class SceneImporter
{
public:
	/// @brief Import a scene from assets into ECS scene
	/// @param assetScene The asset scene to import
	/// @param targetScene The ECS scene to populate
	/// @param gpuResourceManager Optional GPU resource manager for creating GPU resources during import
	/// @return true if import was successful
	static bool importScene( std::shared_ptr<assets::Scene> assetScene, ecs::Scene &targetScene, engine::GPUResourceManager *gpuResourceManager = nullptr );

	/// @brief Create GPU resources for an already-imported scene
	/// @param assetScene The original asset scene used for import
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
	/// @param gpuResourceManager Optional GPU resource manager for creating GPU resources during import
	/// @return The created entity
	static ecs::Entity importNode( std::shared_ptr<assets::Scene> assetScene, const assets::SceneNode &node, ecs::Scene &targetScene, ecs::Entity parent = {}, engine::GPUResourceManager *gpuResourceManager = nullptr );

	/// @brief Setup Transform component from scene node transform
	/// @param node The scene node with transform data
	/// @param entity The entity to attach the component to
	/// @param targetScene The ECS scene
	static void setupTransformComponent( const assets::SceneNode &node, ecs::Entity entity, ecs::Scene &targetScene );

	/// @brief Setup MeshRenderer component with optional GPU resources
	/// @param node The scene node with mesh data
	/// @param entity The entity to attach the component to
	/// @param targetScene The ECS scene
	/// @param assetScene The source asset scene for resource creation
	/// @param gpuResourceManager Optional GPU resource manager for creating GPU resources
	static void setupMeshRenderer( const assets::SceneNode &node, ecs::Entity entity, ecs::Scene &targetScene, std::shared_ptr<assets::Scene> assetScene, engine::GPUResourceManager *gpuResourceManager = nullptr );
};

} // namespace runtime