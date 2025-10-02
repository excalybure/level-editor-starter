#include "SceneHierarchyPanel.h"
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "editor/selection.h"
#include "editor/commands/CommandHistory.h"
#include "editor/commands/EcsCommands.h"
#include <imgui.h>
#include <format>
#include <memory>

namespace editor
{

SceneHierarchyPanel::SceneHierarchyPanel( ecs::Scene &scene,
	SelectionManager &selectionManager,
	CommandHistory &commandHistory )
	: m_scene( scene ), m_selectionManager( selectionManager ), m_commandHistory( commandHistory ), m_visible( true )
{
}

void SceneHierarchyPanel::render()
{
	if ( !m_visible )
		return;

	ImGui::Begin( "Scene Hierarchy", &m_visible );

	renderEntityTree();

	ImGui::End();
}

void SceneHierarchyPanel::setVisible( bool visible )
{
	m_visible = visible;
}

void SceneHierarchyPanel::renderEntityTree()
{
	// Iterate through all entities in the scene
	const auto entities = m_scene.getAllEntities();

	// First pass: render only root entities (those without parents)
	for ( const auto &entity : entities )
	{
		// Skip invalid entities
		if ( !m_scene.isValid( entity ) )
			continue;

		// Only render entities that don't have a parent (root entities)
		const ecs::Entity parent = m_scene.getParent( entity );
		if ( m_scene.isValid( parent ) )
			continue; // Has a parent, will be rendered as child

		// Render this root entity and its children recursively
		renderEntityNode( entity );
	}
}

void SceneHierarchyPanel::renderEntityNode( ecs::Entity entity )
{
	// Get display name
	const std::string displayName = getEntityDisplayName( entity );

	// Using ## separator to have unique IDs even with same names
	const std::string label = std::format( "{}##{}", displayName, entity.id );

	// Get children to determine if this node should have a tree node or just be a leaf
	const std::vector<ecs::Entity> children = m_scene.getChildren( entity );
	const bool hasChildren = !children.empty();

	// Check if this entity is selected
	const bool isSelected = m_selectionManager.isSelected( entity );

	if ( hasChildren )
	{
		// Use ImGui tree node for entities with children
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
			ImGuiTreeNodeFlags_SpanAvailWidth;

		// Add Selected flag if entity is selected
		if ( isSelected )
		{
			flags |= ImGuiTreeNodeFlags_Selected;
		}

		const bool nodeOpen = ImGui::TreeNodeEx( label.c_str(), flags );

		// Handle selection on click
		if ( ImGui::IsItemClicked() )
		{
			// Check for Ctrl modifier for additive selection
			const bool additive = ImGui::GetIO().KeyCtrl;

			if ( additive && isSelected )
			{
				// Ctrl+Click on selected entity: toggle off
				m_selectionManager.toggleSelection( entity );
			}
			else
			{
				// Normal click or Ctrl+Click on unselected: select (additive if Ctrl held)
				m_selectionManager.select( entity, additive );
			}
		}

		// Setup drag source
		if ( ImGui::BeginDragDropSource( ImGuiDragDropFlags_None ) )
		{
			// Set payload containing the entity ID
			ImGui::SetDragDropPayload( "ENTITY_HIERARCHY", &entity, sizeof( ecs::Entity ) );
			ImGui::Text( "%s", displayName.c_str() );
			ImGui::EndDragDropSource();
		}

		// Setup drop target
		if ( ImGui::BeginDragDropTarget() )
		{
			if ( const ImGuiPayload *payload = ImGui::AcceptDragDropPayload( "ENTITY_HIERARCHY" ) )
			{
				const ecs::Entity draggedEntity = *static_cast<const ecs::Entity *>( payload->Data );

				// Only execute if dragged entity is different from target
				if ( draggedEntity.id != entity.id )
				{
					// Create and execute SetParentCommand
					auto command = std::make_unique<SetParentCommand>( m_scene, draggedEntity, entity );
					m_commandHistory.executeCommand( std::move( command ) );
				}
			}
			ImGui::EndDragDropTarget();
		}

		// Handle right-click for context menu
		if ( ImGui::IsItemHovered() && ImGui::IsMouseClicked( ImGuiMouseButton_Right ) )
		{
			m_contextMenuEntity = entity;
			ImGui::OpenPopup( "EntityContextMenu" );
		}

		if ( nodeOpen )
		{
			// Render children recursively
			for ( const auto &child : children )
			{
				if ( m_scene.isValid( child ) )
				{
					renderEntityNode( child );
				}
			}

			ImGui::TreePop();
		}
	}
	else
	{
		// Leaf node - use TreeNodeEx with leaf flag
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
			ImGuiTreeNodeFlags_SpanAvailWidth;

		// Add Selected flag if entity is selected
		if ( isSelected )
		{
			flags |= ImGuiTreeNodeFlags_Selected;
		}

		ImGui::TreeNodeEx( label.c_str(), flags );

		// Handle selection on click
		if ( ImGui::IsItemClicked() )
		{
			// Check for Ctrl modifier for additive selection
			const bool additive = ImGui::GetIO().KeyCtrl;

			if ( additive && isSelected )
			{
				// Ctrl+Click on selected entity: toggle off
				m_selectionManager.toggleSelection( entity );
			}
			else
			{
				// Normal click or Ctrl+Click on unselected: select (additive if Ctrl held)
				m_selectionManager.select( entity, additive );
			}
		}

		// Setup drag source
		if ( ImGui::BeginDragDropSource( ImGuiDragDropFlags_None ) )
		{
			// Set payload containing the entity ID
			ImGui::SetDragDropPayload( "ENTITY_HIERARCHY", &entity, sizeof( ecs::Entity ) );
			ImGui::Text( "%s", displayName.c_str() );
			ImGui::EndDragDropSource();
		}

		// Setup drop target
		if ( ImGui::BeginDragDropTarget() )
		{
			if ( const ImGuiPayload *payload = ImGui::AcceptDragDropPayload( "ENTITY_HIERARCHY" ) )
			{
				const ecs::Entity draggedEntity = *static_cast<const ecs::Entity *>( payload->Data );

				// Only execute if dragged entity is different from target
				if ( draggedEntity.id != entity.id )
				{
					// Create and execute SetParentCommand
					auto command = std::make_unique<SetParentCommand>( m_scene, draggedEntity, entity );
					m_commandHistory.executeCommand( std::move( command ) );
				}
			}
			ImGui::EndDragDropTarget();
		}

		// Handle right-click for context menu
		if ( ImGui::IsItemHovered() && ImGui::IsMouseClicked( ImGuiMouseButton_Right ) )
		{
			m_contextMenuEntity = entity;
			ImGui::OpenPopup( "EntityContextMenu" );
		}
	}

	// Render context menu (once per frame, not per entity)
	renderContextMenu( entity );
}

std::string SceneHierarchyPanel::getEntityDisplayName( ecs::Entity entity ) const
{
	// Try to get Name component
	if ( const auto *name = m_scene.getComponent<components::Name>( entity ) )
	{
		return name->name;
	}

	// Fallback to Entity [ID]
	return std::format( "Entity [{}]", entity.id );
}

void SceneHierarchyPanel::renderContextMenu( ecs::Entity entity )
{
	// Only render context menu for the entity that was right-clicked
	if ( entity.id != m_contextMenuEntity.id )
		return;

	if ( ImGui::BeginPopup( "EntityContextMenu" ) )
	{
		const std::string displayName = getEntityDisplayName( entity );
		ImGui::Text( "Entity: %s", displayName.c_str() );
		ImGui::Separator();

		// Create Child
		if ( ImGui::MenuItem( "Create Child" ) )
		{
			// Create new entity
			auto createCommand = std::make_unique<CreateEntityCommand>( m_scene, "New Entity" );
			m_commandHistory.executeCommand( std::move( createCommand ) );

			// Get the created entity (last entity in scene)
			const auto entities = m_scene.getAllEntities();
			const ecs::Entity newEntity = entities.back();

			// Set parent relationship
			auto parentCommand = std::make_unique<SetParentCommand>( m_scene, newEntity, entity );
			m_commandHistory.executeCommand( std::move( parentCommand ) );
		}

		// Duplicate (placeholder - requires DuplicateEntityCommand)
		if ( ImGui::MenuItem( "Duplicate" ) )
		{
			// TODO: Implement DuplicateEntityCommand
			// For now, just create a new entity with same name
			const auto *name = m_scene.getComponent<components::Name>( entity );
			const std::string newName = name ? name->name + " Copy" : "Entity Copy";

			auto command = std::make_unique<CreateEntityCommand>( m_scene, newName );
			m_commandHistory.executeCommand( std::move( command ) );
		}

		// Delete
		if ( ImGui::MenuItem( "Delete" ) )
		{
			auto command = std::make_unique<DeleteEntityCommand>( m_scene, entity );
			m_commandHistory.executeCommand( std::move( command ) );
		}

		ImGui::Separator();

		// Rename
		if ( ImGui::MenuItem( "Rename" ) )
		{
			// TODO: Implement inline rename (T1.6)
			// For now, just use a simple hardcoded rename
			auto command = std::make_unique<RenameEntityCommand>( m_scene, entity, "Renamed Entity" );
			m_commandHistory.executeCommand( std::move( command ) );
		}

		ImGui::EndPopup();
	}
}

} // namespace editor
