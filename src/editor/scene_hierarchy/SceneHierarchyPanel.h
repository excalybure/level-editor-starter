#pragma once

#include <string>
#include "runtime/entity.h"

namespace ecs
{
class Scene;
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
	 */
	SceneHierarchyPanel( ecs::Scene &scene,
		SelectionManager &selectionManager,
		CommandHistory &commandHistory );

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

private:
	ecs::Scene &m_scene;
	SelectionManager &m_selectionManager;
	CommandHistory &m_commandHistory;
	bool m_visible = true;

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
};

} // namespace editor
