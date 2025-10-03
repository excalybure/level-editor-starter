#include "EntityInspectorPanel.h"
#include "ComponentUI.h"
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "editor/selection.h"
#include "editor/commands/CommandHistory.h"
#include "editor/commands/EcsCommands.h"
#include "editor/transform_commands.h"
#include "engine/math/math.h"
#include "engine/gpu/mesh_gpu.h"
#include <imgui.h>
#include <format>
#include <cstring>

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

	// Render Name component if present
	if ( m_scene.hasComponent<components::Name>( entity ) )
	{
		renderNameComponent( entity );
	}

	// Render Visible component if present
	if ( m_scene.hasComponent<components::Visible>( entity ) )
	{
		renderVisibleComponent( entity );
	}

	// Render Transform component if present
	if ( m_scene.hasComponent<components::Transform>( entity ) )
	{
		renderTransformComponent( entity );
	}

	// Render MeshRenderer component if present
	if ( m_scene.hasComponent<components::MeshRenderer>( entity ) )
	{
		renderMeshRendererComponent( entity );
	}

	// Add Component menu
	ImGui::Separator();
	renderAddComponentMenu( entity );
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

void EntityInspectorPanel::renderNameComponent( ecs::Entity entity )
{
	auto *nameComp = m_scene.getComponent<components::Name>( entity );
	if ( !nameComp )
		return;

	// Component header with collapsing header
	if ( ImGui::CollapsingHeader( "Name", ImGuiTreeNodeFlags_DefaultOpen ) )
	{
		ImGui::PushID( "Name" );

		// Initialize buffer if not editing yet
		if ( !m_nameEditState.isEditing )
		{
			std::strncpy( m_nameEditState.nameBuffer, nameComp->name.c_str(), sizeof( m_nameEditState.nameBuffer ) - 1 );
			m_nameEditState.nameBuffer[sizeof( m_nameEditState.nameBuffer ) - 1] = '\0';
		}

		ImGui::Text( "Name" );
		ImGui::SameLine();

		// Input text for name editing
		const bool valueChanged = ImGui::InputText( "##NameInput", m_nameEditState.nameBuffer, sizeof( m_nameEditState.nameBuffer ), ImGuiInputTextFlags_EnterReturnsTrue );

		// Detect when input becomes active (user clicks in field)
		if ( ImGui::IsItemActivated() && !m_nameEditState.isEditing )
		{
			// Capture before state
			m_nameEditState.isEditing = true;
			m_nameEditState.beforeName = nameComp->name;
		}

		// Detect when input becomes inactive (user finishes editing)
		const bool editingEnded = m_nameEditState.isEditing && ( valueChanged || ( ImGui::IsItemDeactivated() && !ImGui::IsItemActive() ) );

		if ( editingEnded )
		{
			// Get new name from buffer
			const std::string newName = m_nameEditState.nameBuffer;

			// Only create command if name actually changed
			if ( newName != m_nameEditState.beforeName )
			{
				auto command = std::make_unique<editor::RenameEntityCommand>( m_scene, entity, newName );
				m_commandHistory.executeCommand( std::move( command ) );
			}

			// Reset edit state
			m_nameEditState.isEditing = false;
		}

		ImGui::PopID();
	}
}

void EntityInspectorPanel::renderVisibleComponent( ecs::Entity entity )
{
	auto *visible = m_scene.getComponent<components::Visible>( entity );
	if ( !visible )
		return;

	// Component header with collapsing header
	if ( ImGui::CollapsingHeader( "Visible", ImGuiTreeNodeFlags_DefaultOpen ) )
	{
		ImGui::PushID( "Visible" );

		bool valueChanged = false;

		// Visible checkbox
		bool isVisible = visible->visible;
		if ( ImGui::Checkbox( "Visible", &isVisible ) )
		{
			if ( !m_visibleEditState.isEditing )
			{
				m_visibleEditState.isEditing = true;
				m_visibleEditState.beforeVisible = *visible;
			}
			visible->visible = isVisible;
			valueChanged = true;
		}

		// Cast Shadows checkbox
		bool castShadows = visible->castShadows;
		if ( ImGui::Checkbox( "Cast Shadows", &castShadows ) )
		{
			if ( !m_visibleEditState.isEditing )
			{
				m_visibleEditState.isEditing = true;
				m_visibleEditState.beforeVisible = *visible;
			}
			visible->castShadows = castShadows;
			valueChanged = true;
		}

		// Receive Shadows checkbox
		bool receiveShadows = visible->receiveShadows;
		if ( ImGui::Checkbox( "Receive Shadows", &receiveShadows ) )
		{
			if ( !m_visibleEditState.isEditing )
			{
				m_visibleEditState.isEditing = true;
				m_visibleEditState.beforeVisible = *visible;
			}
			visible->receiveShadows = receiveShadows;
			valueChanged = true;
		}

		// Create command when editing ends (no items active anymore)
		if ( m_visibleEditState.isEditing && !ImGui::IsAnyItemActive() )
		{
			// Create and execute visible command
			const components::Visible afterVisible = *visible;
			auto command = std::make_unique<editor::ModifyVisibleCommand>( m_scene, entity, afterVisible );

			m_commandHistory.executeCommand( std::move( command ) );

			// Reset edit state
			m_visibleEditState.isEditing = false;
		}

		ImGui::PopID();
	}
}

void EntityInspectorPanel::renderMeshRendererComponent( ecs::Entity entity )
{
	const auto *meshRenderer = m_scene.getComponent<components::MeshRenderer>( entity );
	if ( !meshRenderer )
		return;

	// Component header with collapsing header
	if ( ImGui::CollapsingHeader( "MeshRenderer", ImGuiTreeNodeFlags_DefaultOpen ) )
	{
		ImGui::PushID( "MeshRenderer" );

		// Display mesh handle (read-only)
		ImGui::Text( "Mesh Handle" );
		ImGui::SameLine();
		ImGui::TextDisabled( "%u", meshRenderer->meshHandle );

		// Display GPU status (read-only)
		ImGui::Text( "GPU Status" );
		ImGui::SameLine();
		if ( meshRenderer->gpuMesh != nullptr )
		{
			ImGui::TextColored( ImVec4( 0.0f, 1.0f, 0.0f, 1.0f ), "Uploaded" );

			// Display primitive count if GPU mesh is available
			ImGui::Text( "Primitives" );
			ImGui::SameLine();
			ImGui::TextDisabled( "%u", meshRenderer->gpuMesh->getPrimitiveCount() );
		}
		else
		{
			ImGui::TextColored( ImVec4( 1.0f, 0.5f, 0.0f, 1.0f ), "Not Uploaded" );
		}

		// Display LOD bias (read-only for now)
		ImGui::Text( "LOD Bias" );
		ImGui::SameLine();
		ImGui::TextDisabled( "%.2f", meshRenderer->lodBias );

		// Future: Asset selector button will be added here
		ImGui::Separator();
		ImGui::TextDisabled( "(Asset selector coming soon)" );

		ImGui::PopID();
	}
}

void EntityInspectorPanel::renderAddComponentMenu( ecs::Entity entity )
{
	// Add Component button
	if ( ImGui::Button( "Add Component", ImVec2( -1, 0 ) ) )
	{
		ImGui::OpenPopup( "AddComponentPopup" );
	}

	// Popup menu with component options
	if ( ImGui::BeginPopup( "AddComponentPopup" ) )
	{
		ImGui::TextDisabled( "Select Component Type" );
		ImGui::Separator();

		// Transform component
		const bool hasTransform = m_scene.hasComponent<components::Transform>( entity );
		if ( ImGui::MenuItem( "Transform", nullptr, false, !hasTransform ) )
		{
			components::Transform transform;
			transform.position = { 0.0f, 0.0f, 0.0f };
			transform.rotation = { 0.0f, 0.0f, 0.0f };
			transform.scale = { 1.0f, 1.0f, 1.0f };
			auto command = std::make_unique<editor::AddComponentCommand<components::Transform>>( m_scene, entity, transform );
			m_commandHistory.executeCommand( std::move( command ) );
		}
		if ( hasTransform && ImGui::IsItemHovered() )
		{
			ImGui::SetTooltip( "Component already present" );
		}

		// Name component
		const bool hasName = m_scene.hasComponent<components::Name>( entity );
		if ( ImGui::MenuItem( "Name", nullptr, false, !hasName ) )
		{
			components::Name name;
			name.name = "Entity";
			auto command = std::make_unique<editor::AddComponentCommand<components::Name>>( m_scene, entity, name );
			m_commandHistory.executeCommand( std::move( command ) );
		}
		if ( hasName && ImGui::IsItemHovered() )
		{
			ImGui::SetTooltip( "Component already present" );
		}

		// Visible component
		const bool hasVisible = m_scene.hasComponent<components::Visible>( entity );
		if ( ImGui::MenuItem( "Visible", nullptr, false, !hasVisible ) )
		{
			components::Visible visible;
			visible.visible = true;
			visible.castShadows = true;
			visible.receiveShadows = true;
			auto command = std::make_unique<editor::AddComponentCommand<components::Visible>>( m_scene, entity, visible );
			m_commandHistory.executeCommand( std::move( command ) );
		}
		if ( hasVisible && ImGui::IsItemHovered() )
		{
			ImGui::SetTooltip( "Component already present" );
		}

		// MeshRenderer component
		const bool hasMeshRenderer = m_scene.hasComponent<components::MeshRenderer>( entity );
		if ( ImGui::MenuItem( "MeshRenderer", nullptr, false, !hasMeshRenderer ) )
		{
			components::MeshRenderer meshRenderer;
			meshRenderer.meshHandle = 0;
			auto command = std::make_unique<editor::AddComponentCommand<components::MeshRenderer>>( m_scene, entity, meshRenderer );
			m_commandHistory.executeCommand( std::move( command ) );
		}
		if ( hasMeshRenderer && ImGui::IsItemHovered() )
		{
			ImGui::SetTooltip( "Component already present" );
		}

		// Selected component
		const bool hasSelected = m_scene.hasComponent<components::Selected>( entity );
		if ( ImGui::MenuItem( "Selected", nullptr, false, !hasSelected ) )
		{
			components::Selected selected;
			selected.isPrimary = false;
			auto command = std::make_unique<editor::AddComponentCommand<components::Selected>>( m_scene, entity, selected );
			m_commandHistory.executeCommand( std::move( command ) );
		}
		if ( hasSelected && ImGui::IsItemHovered() )
		{
			ImGui::SetTooltip( "Component already present" );
		}

		ImGui::EndPopup();
	}
}

} // namespace editor
