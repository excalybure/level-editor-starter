#include "SceneHierarchyPanel.h"
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "editor/selection.h"
#include "editor/commands/CommandHistory.h"
#include <imgui.h>
#include <format>

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

	if ( hasChildren )
	{
		// Use ImGui tree node for entities with children
		const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
			ImGuiTreeNodeFlags_SpanAvailWidth;

		const bool nodeOpen = ImGui::TreeNodeEx( label.c_str(), flags );

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
		// Leaf node - use Selectable or TreeNodeEx with leaf flag
		const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
			ImGuiTreeNodeFlags_SpanAvailWidth;

		ImGui::TreeNodeEx( label.c_str(), flags );
	}
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

} // namespace editor
