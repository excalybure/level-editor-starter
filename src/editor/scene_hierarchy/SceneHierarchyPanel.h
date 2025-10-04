#pragma once

#include <string>
#include <functional>
#include "runtime/entity.h"

namespace ecs
{
class Scene;
}

namespace assets
{
class AssetManager;
}

namespace engine
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
 * @brief Scene hierarchy panel displaying entity tree structure
 * 
 * Provides a tree view of all entities in the scene with support for:
 * - Hierarchical parent-child relationships
 * - Entity selection and multi-selection
 * - Drag-and-drop reparenting
 * - Context menu operations
 * - Inline renaming
 */
class SceneHierarchyPanel
{
public:
	/**
	 * @brief Construct a scene hierarchy panel
	 * @param scene The scene to display
	 * @param selectionManager Selection manager for entity selection
	 * @param commandHistory Command history for undo/redo support
	 * @param assetManager Asset manager for asset loading (optional, for drag-drop support)
	 * @param gpuManager GPU resource manager for mesh uploading (optional, for drag-drop support)
	 */
	SceneHierarchyPanel( ecs::Scene &scene,
		SelectionManager &selectionManager,
		CommandHistory &commandHistory,
		assets::AssetManager *assetManager = nullptr,
		engine::GPUResourceManager *gpuManager = nullptr );

	/**
	 * @brief Render the hierarchy panel UI
	 * 
	 * Displays all entities in a tree structure with names and hierarchy.
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

	// Inline rename API (for testing)
	void startRename( ecs::Entity entity );
	void commitRename();
	void cancelRename();
	bool isRenaming() const { return m_renameEntity.isValid(); }
	ecs::Entity getRenamingEntity() const { return m_renameEntity; }
	void setRenameBuffer( const std::string &name );

	// Search filter API (for testing)
	void setSearchFilter( const std::string &filter );
	std::string getSearchFilter() const { return m_searchFilter; }
	bool matchesSearchFilter( ecs::Entity entity ) const;

	// Focus entity API (for testing)
	using FocusCallback = std::function<void( ecs::Entity )>;
	void setFocusCallback( FocusCallback callback );
	void requestFocus( ecs::Entity entity );

private:
	ecs::Scene &m_scene;
	SelectionManager &m_selectionManager;
	CommandHistory &m_commandHistory;
	assets::AssetManager *m_assetManager = nullptr;		// Optional: for asset drag-drop
	engine::GPUResourceManager *m_gpuManager = nullptr; // Optional: for mesh uploading
	bool m_visible = true;
	ecs::Entity m_contextMenuEntity{}; // Entity for which context menu is open

	// Inline rename state
	ecs::Entity m_renameEntity{}; // Entity being renamed (invalid = not renaming)
	std::string m_renameBuffer;	  // Current text in rename input field

	// Search filter state
	std::string m_searchFilter; // Current search filter text

	// Focus callback
	FocusCallback m_focusCallback; // Callback to focus camera on entity

	/**
	 * @brief Render the entity tree (all entities in hierarchical structure)
	 */
	void renderEntityTree();

	/**
	 * @brief Render an entity node recursively
	 * @param entity The entity to render
	 */
	void renderEntityNode( ecs::Entity entity );

	/**
	 * @brief Get display name for an entity
	 * @param entity The entity to get name for
	 * @return Entity name or "Entity [ID]" fallback
	 */
	std::string getEntityDisplayName( ecs::Entity entity ) const;

	/**
	 * @brief Render context menu for an entity
	 * @param entity The entity to show context menu for
	 */
	void renderContextMenu( ecs::Entity entity );
};

} // namespace editor
