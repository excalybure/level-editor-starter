#pragma once

#include <string>
#include "runtime/entity.h"
#include "runtime/components.h"
#include "editor/commands/PrimitiveMaterialCommands.h"

namespace graphics::gpu
{
class MeshGPU;
}

namespace ecs
{
class Scene;
}

namespace assets
{
class Scene;
class AssetManager;
} // namespace assets

namespace systems
{
class SystemManager;
}

namespace graphics
{
class GPUResourceManager;
}

namespace editor
{
class SelectionManager;
}

class CommandHistory;

namespace editor
{

/**
 * @brief Entity inspector panel for editing component properties
 * 
 * Provides a detailed view of the selected entity's components with support for:
 * - Displaying entity information and metadata
 * - Editing component properties with undo/redo
 * - Adding and removing components
 * - Multi-selection support with common property editing
 */
class EntityInspectorPanel
{
public:
	/**
	 * @brief Construct an entity inspector panel
	 * @param scene The scene containing entities to inspect
	 * @param selectionManager Selection manager for tracking selected entities
	 * @param commandHistory Command history for undo/redo support
	 * @param systemManager System manager for accessing TransformSystem
	 * @param assetManager Asset manager for loading mesh data and material instances
	 * @param gpuManager GPU resource manager for triggering material updates
	 */
	EntityInspectorPanel( ecs::Scene &scene,
		SelectionManager &selectionManager,
		CommandHistory &commandHistory,
		systems::SystemManager &systemManager,
		assets::AssetManager *assetManager = nullptr,
		graphics::GPUResourceManager *gpuManager = nullptr );

	/**
	 * @brief Render the inspector panel UI
	 * 
	 * Displays component editors for the selected entity or entities.
	 * Shows appropriate messages for no selection or multi-selection.
	 * Should be called every frame to update the UI.
	 */
	void render();

	/**
	 * @brief Set panel visibility
	 * @param visible Whether the panel should be visible
	 */
	void setVisible( bool visible );

	/**
	 * @brief Get panel visibility state
	 * @return true if panel is visible
	 */
	bool isVisible() const { return m_visible; }

	/**
	 * @brief Set the current asset scene for material override detection
	 * @param assetScene Pointer to the loaded asset scene (can be nullptr)
	 */
	void setAssetScene( assets::Scene *assetScene ) { m_cachedAssetScene = assetScene; }

	/**
	 * @brief Set the path of the current asset scene to enable lazy-loading
	 * @param assetScenePath Path to the asset scene file (e.g., "assets/models/scene.glb")
	 */
	void setAssetScenePath( const std::string &assetScenePath ) { m_cachedAssetScenePath = assetScenePath; }

private:
	ecs::Scene &m_scene;
	SelectionManager &m_selectionManager;
	CommandHistory &m_commandHistory;
	systems::SystemManager &m_systemManager;
	assets::AssetManager *m_assetManager;
	graphics::GPUResourceManager *m_gpuManager;
	mutable assets::Scene *m_cachedAssetScene; // Cache of loaded asset scene for override detection
	std::string m_cachedAssetScenePath;		   // Track the path of the cached scene
	bool m_visible;

	// Rendering methods for different states
	void renderNoSelection();
	void renderSingleEntity( ecs::Entity entity );
	void renderMultiSelection();
	void renderEntityHeader( ecs::Entity entity );

	// Component rendering methods
	void renderTransformComponent( ecs::Entity entity );
	void renderNameComponent( ecs::Entity entity );
	void renderVisibleComponent( ecs::Entity entity );
	void renderMeshRendererComponent( ecs::Entity entity );
	void renderPrimitiveTree( ecs::Entity entity, const graphics::gpu::MeshGPU &meshGPU );

	// Component management
	void renderAddComponentMenu( ecs::Entity entity );

	// Helper to render component context menu (right-click)
	template <components::Component T>
	void renderComponentContextMenu( const char *componentName, ecs::Entity entity );

	// Multi-selection helper methods
	template <components::Component T>
	bool allSelectedHaveComponent() const;
	void renderMultiTransformComponent( const std::vector<ecs::Entity> &entities );
	void renderMultiVisibleComponent( const std::vector<ecs::Entity> &entities );

	// Transform editing state for command creation
	struct TransformEditState
	{
		bool isEditing = false;
		components::Transform beforeTransform;
		std::vector<components::Transform> beforeTransforms; // For multi-selection
	};
	TransformEditState m_transformEditState;

	// Name editing state for command creation
	struct NameEditState
	{
		bool isEditing = false;
		std::string beforeName;
		char nameBuffer[256] = {};
	};
	NameEditState m_nameEditState;

	// Visible editing state for command creation
	struct VisibleEditState
	{
		bool isEditing = false;
		components::Visible beforeVisible;
		std::vector<components::Visible> beforeVisibles; // For multi-selection
	};
	VisibleEditState m_visibleEditState;

	// Helper methods for material override detection
	assets::Scene *getOrLoadAssetScene();
	bool isPropertyOverridden( const assets::Primitive &primitive, MaterialPropertyType propertyType ) const;
};

} // namespace editor
