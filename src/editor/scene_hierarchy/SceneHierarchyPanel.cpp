#include "SceneHierarchyPanel.h"
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "editor/selection.h"
#include "editor/commands/CommandHistory.h"
#include "editor/commands/EcsCommands.h"
#include <imgui.h>
#include <format>
#include <memory>
#include <cstring>
#include <algorithm>
#include <cctype>

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

	// Render search bar at the top
	char searchBuffer[256];
	std::strncpy( searchBuffer, m_searchFilter.c_str(), sizeof( searchBuffer ) - 1 );
	searchBuffer[sizeof( searchBuffer ) - 1] = '\0';

	ImGui::SetNextItemWidth( -1.0f ); // Full width
	if ( ImGui::InputTextWithHint( "##search", "Search...", searchBuffer, sizeof( searchBuffer ) ) )
	{
		m_searchFilter = searchBuffer;
	}

	ImGui::Separator();

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

		// Skip entities that don't match the search filter
		if ( !matchesSearchFilter( entity ) )
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

	// Check if this entity is being renamed
	const bool isRenaming = m_renameEntity.isValid() && m_renameEntity.id == entity.id;

	// Setup tree node flags
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
	if ( hasChildren )
	{
		flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
	}
	else
	{
		flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}

	if ( isSelected )
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	// Render tree node or rename input
	bool nodeOpen = false;
	if ( isRenaming )
	{
		// Show tree node with input field for name
		nodeOpen = ImGui::TreeNodeEx( std::format( "##rename{}", entity.id ).c_str(), flags );
		ImGui::SameLine();

		// Focus input on first frame
		ImGui::SetKeyboardFocusHere();

		// Render input field
		char buffer[256];
		std::strncpy( buffer, m_renameBuffer.c_str(), sizeof( buffer ) - 1 );
		buffer[sizeof( buffer ) - 1] = '\0';

		if ( ImGui::InputText( std::format( "##input{}", entity.id ).c_str(), buffer, sizeof( buffer ), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll ) )
		{
			// Enter pressed - commit rename
			m_renameBuffer = buffer;
			commitRename();
		}
		else
		{
			m_renameBuffer = buffer;
		}

		// Check for Escape to cancel
		if ( ImGui::IsKeyPressed( ImGuiKey_Escape ) )
		{
			cancelRename();
		}
	}
	else
	{
		// Normal tree node rendering
		nodeOpen = ImGui::TreeNodeEx( label.c_str(), flags );

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

		// Handle double-click to start rename
		if ( ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
		{
			startRename( entity );
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

	// Render children recursively if node is open
	if ( nodeOpen && hasChildren )
	{
		for ( const auto &child : children )
		{
			if ( m_scene.isValid( child ) && matchesSearchFilter( child ) )
			{
				renderEntityNode( child );
			}
		}

		ImGui::TreePop();
	} // Render context menu (once per frame, not per entity)
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
			// Start inline rename mode
			startRename( entity );
		}

		ImGui::EndPopup();
	}
}

void SceneHierarchyPanel::startRename( ecs::Entity entity )
{
	if ( !m_scene.isValid( entity ) )
		return;

	m_renameEntity = entity;
	m_renameBuffer = getEntityDisplayName( entity );
}

void SceneHierarchyPanel::commitRename()
{
	if ( !m_renameEntity.isValid() )
		return;

	// Validate non-empty name
	if ( m_renameBuffer.empty() )
	{
		cancelRename();
		return;
	}

	// Execute rename command
	auto command = std::make_unique<RenameEntityCommand>( m_scene, m_renameEntity, m_renameBuffer );
	m_commandHistory.executeCommand( std::move( command ) );

	// Clear rename state
	m_renameEntity = ecs::Entity{};
	m_renameBuffer.clear();
}

void SceneHierarchyPanel::cancelRename()
{
	m_renameEntity = ecs::Entity{};
	m_renameBuffer.clear();
}

void SceneHierarchyPanel::setRenameBuffer( const std::string &name )
{
	m_renameBuffer = name;
}

void SceneHierarchyPanel::setSearchFilter( const std::string &filter )
{
	m_searchFilter = filter;
}

bool SceneHierarchyPanel::matchesSearchFilter( ecs::Entity entity ) const
{
	// Empty filter matches everything
	if ( m_searchFilter.empty() )
		return true;

	// Get entity name
	const std::string entityName = getEntityDisplayName( entity );

	// Convert both to lowercase for case-insensitive comparison
	std::string lowerEntityName = entityName;
	std::string lowerFilter = m_searchFilter;

	std::transform( lowerEntityName.begin(), lowerEntityName.end(), lowerEntityName.begin(), []( unsigned char c ) { return std::tolower( c ); } );

	std::transform( lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), []( unsigned char c ) { return std::tolower( c ); } );

	// Check if entity name contains the filter string
	return lowerEntityName.find( lowerFilter ) != std::string::npos;
}

} // namespace editor
