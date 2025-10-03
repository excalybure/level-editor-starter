#pragma once

#include <string>
#include "runtime/entity.h"
#include "runtime/components.h"

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
	 */
	EntityInspectorPanel( ecs::Scene &scene,
		SelectionManager &selectionManager,
		CommandHistory &commandHistory );

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

private:
	ecs::Scene &m_scene;
	SelectionManager &m_selectionManager;
	CommandHistory &m_commandHistory;
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

	// Component management
	void renderAddComponentMenu( ecs::Entity entity );

	// Transform editing state for command creation
	struct TransformEditState
	{
		bool isEditing = false;
		components::Transform beforeTransform;
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
	};
	VisibleEditState m_visibleEditState;
};

} // namespace editor
