#include "EntityInspectorPanel.h"
#include "ComponentUI.h"
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "editor/selection.h"
#include "editor/commands/CommandHistory.h"
#include "editor/transform_commands.h"
#include "engine/math/math.h"
#include <imgui.h>
#include <format>

namespace editor
{

EntityInspectorPanel::EntityInspectorPanel( ecs::Scene &scene,
	SelectionManager &selectionManager,
	CommandHistory &commandHistory )
	: m_scene( scene ), m_selectionManager( selectionManager ), m_commandHistory( commandHistory ), m_visible( true )
{
}

void EntityInspectorPanel::render()
{
	if ( !m_visible )
		return;

	ImGui::Begin( "Entity Inspector", &m_visible );

	const auto selectedEntities = m_selectionManager.getSelectedEntities();
	const size_t selectionCount = selectedEntities.size();

	if ( selectionCount == 0 )
	{
		// No selection
		renderNoSelection();
	}
	else if ( selectionCount == 1 )
	{
		// Single entity selected
		const ecs::Entity entity = selectedEntities[0];
		if ( m_scene.isValid( entity ) )
		{
			renderSingleEntity( entity );
		}
		else
		{
			renderNoSelection(); // Invalid entity, show no selection
		}
	}
	else
	{
		// Multiple entities selected
		renderMultiSelection();
	}

	ImGui::End();
}

void EntityInspectorPanel::setVisible( bool visible )
{
	m_visible = visible;
}

void EntityInspectorPanel::renderNoSelection()
{
	ImGui::TextDisabled( "No Selection" );
	ImGui::Separator();
	ImGui::TextWrapped( "Select an entity in the scene hierarchy or viewport to view and edit its properties." );
}

void EntityInspectorPanel::renderSingleEntity( ecs::Entity entity )
{
	renderEntityHeader( entity );
	ImGui::Separator();

	// Render Transform component if present
	if ( m_scene.hasComponent<components::Transform>( entity ) )
	{
		renderTransformComponent( entity );
	}

	// Additional component editors will be added in subsequent tasks
}

void EntityInspectorPanel::renderMultiSelection()
{
	const size_t selectionCount = m_selectionManager.getSelectionCount();
	ImGui::Text( "Multiple Selected (%zu entities)", selectionCount );
	ImGui::Separator();
	ImGui::TextWrapped( "Multi-selection editing will be available soon." );
}

void EntityInspectorPanel::renderEntityHeader( ecs::Entity entity )
{
	// Display entity ID and name
	const auto *nameComponent = m_scene.getComponent<components::Name>( entity );
	const std::string entityName = nameComponent ? nameComponent->name : std::format( "Entity [{}]", entity.id );

	ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.0f, 1.0f, 0.5f, 1.0f ) ); // Yellow text for header
	ImGui::Text( "%s", entityName.c_str() );
	ImGui::PopStyleColor();

	ImGui::SameLine();
	ImGui::TextDisabled( "(ID: %u)", entity.id );
}

void EntityInspectorPanel::renderTransformComponent( ecs::Entity entity )
{
	auto *transform = m_scene.getComponent<components::Transform>( entity );
	if ( !transform )
		return;

	// Component header with collapsing header
	if ( ImGui::CollapsingHeader( "Transform", ImGuiTreeNodeFlags_DefaultOpen ) )
	{
		ImGui::PushID( "Transform" );

		bool valueChanged = false;
		bool editingStarted = false;
		bool editingEnded = false;

		// Check if any drag control is being activated this frame
		if ( ImGui::IsItemActivated() )
		{
			editingStarted = true;
		}

		// Position control
		math::Vec3f position = transform->position;
		if ( ComponentUI::renderVec3Control( "Position", position, { 0.0f, 0.0f, 0.0f }, 0.1f ) )
		{
			if ( !m_transformEditState.isEditing )
			{
				editingStarted = true;
			}
			transform->position = position;
			transform->markDirty();
			valueChanged = true;
		}

		// Rotation control (convert radians to degrees for display)
		math::Vec3f rotationDegrees = math::degrees( transform->rotation );
		if ( ComponentUI::renderVec3Control( "Rotation", rotationDegrees, { 0.0f, 0.0f, 0.0f }, 1.0f ) )
		{
			if ( !m_transformEditState.isEditing )
			{
				editingStarted = true;
			}
			// Convert back to radians for storage
			transform->rotation = math::radians( rotationDegrees );
			transform->markDirty();
			valueChanged = true;
		}

		// Scale control
		math::Vec3f scale = transform->scale;
		if ( ComponentUI::renderVec3Control( "Scale", scale, { 1.0f, 1.0f, 1.0f }, 0.1f ) )
		{
			if ( !m_transformEditState.isEditing )
			{
				editingStarted = true;
			}
			transform->scale = scale;
			transform->markDirty();
			valueChanged = true;
		}

		// Check if editing has ended (mouse released after drag)
		if ( m_transformEditState.isEditing && !ImGui::IsAnyItemActive() )
		{
			editingEnded = true;
		}

		// Handle edit state transitions
		if ( editingStarted && !m_transformEditState.isEditing )
		{
			// Capture before state
			m_transformEditState.isEditing = true;
			m_transformEditState.beforeTransform = *transform;
		}

		if ( editingEnded && m_transformEditState.isEditing )
		{
			// Create and execute transform command
			const components::Transform afterTransform = *transform;
			auto command = std::make_unique<editor::TransformEntityCommand>(
				entity, m_scene, m_transformEditState.beforeTransform, afterTransform );

			m_commandHistory.executeCommand( std::move( command ) );

			// Reset edit state
			m_transformEditState.isEditing = false;
		}

		ImGui::PopID();
	}
}

} // namespace editor
