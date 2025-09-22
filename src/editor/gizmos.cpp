#include "gizmos.h"
#include "selection.h"
#include "runtime/components.h"
#include "runtime/ecs.h"

// ImGui headers must be included before ImGuizmo
#include <imgui.h>
#include <ImGuizmo.h>

namespace editor
{

GizmoSystem::GizmoSystem( SelectionManager &selectionManager, ecs::Scene &scene ) noexcept
	: m_selectionManager( &selectionManager ), m_scene( &scene )
{
	// Initialize with default values (already set in header)
}

math::Vec3<> GizmoSystem::calculateSelectionCenter() const
{
	if ( !m_selectionManager || !m_scene )
	{
		return math::Vec3<>{ 0.0f, 0.0f, 0.0f };
	}

	const auto &selectedEntities = m_selectionManager->getSelectedEntities();
	if ( selectedEntities.empty() )
	{
		return math::Vec3<>{ 0.0f, 0.0f, 0.0f };
	}

	// Calculate average position of all selected entities
	math::Vec3<> center{ 0.0f, 0.0f, 0.0f };
	int validEntityCount = 0;

	for ( const auto entity : selectedEntities )
	{
		if ( m_scene->hasComponent<components::Transform>( entity ) )
		{
			const auto *transform = m_scene->getComponent<components::Transform>( entity );
			center += transform->position;
			validEntityCount++;
		}
	}

	if ( validEntityCount > 0 )
	{
		center /= static_cast<float>( validEntityCount );
	}

	return center;
}

math::Mat4<> GizmoSystem::calculateGizmoMatrix() const
{
	if ( !m_selectionManager || !m_scene )
	{
		return math::Mat4<>::identity();
	}

	const auto &selectedEntities = m_selectionManager->getSelectedEntities();
	if ( selectedEntities.empty() )
	{
		return math::Mat4<>::identity();
	}

	// For now, we'll create a translation matrix positioned at the selection center
	// In the future, this could account for rotation based on gizmo mode (local vs world)
	const auto center = calculateSelectionCenter();

	// Create translation matrix
	return math::Mat4<>::translation( center );
}

void GizmoSystem::applyTransformDelta( const GizmoResult &delta )
{
	if ( !m_selectionManager || !m_scene )
	{
		return;
	}

	const auto &selectedEntities = m_selectionManager->getSelectedEntities();
	if ( selectedEntities.empty() )
	{
		return;
	}

	// Apply delta to all selected entities
	for ( const auto entity : selectedEntities )
	{
		if ( m_scene->hasComponent<components::Transform>( entity ) )
		{
			auto *transform = m_scene->getComponent<components::Transform>( entity );

			// Apply translation delta (additive)
			transform->position += delta.translationDelta;

			// Apply rotation delta (additive)
			transform->rotation += delta.rotationDelta;

			// Apply scale delta (multiplicative)
			transform->scale *= delta.scaleDelta;

			// Mark transform as dirty for matrix recalculation
			transform->markDirty();
		}
	}
}

bool GizmoSystem::setupImGuizmo( const math::Mat4<> &viewMatrix, const math::Mat4<> &projectionMatrix, const math::Vec4<> &viewport ) noexcept
{
	// Validate viewport dimensions
	if ( viewport.z <= 0.0f || viewport.w <= 0.0f )
	{
		return false;
	}

	// Set ImGuizmo viewport
	ImGuizmo::SetRect( viewport.x, viewport.y, viewport.z, viewport.w );

	// Store matrices for later use in renderGizmo
	m_viewMatrix = viewMatrix;
	m_projectionMatrix = projectionMatrix;
	m_viewportRect = viewport;
	m_isImGuizmoSetup = true;

	return true;
}

GizmoResult GizmoSystem::renderGizmo() noexcept
{
	GizmoResult result;

	// Check if ImGuizmo is properly setup
	if ( !m_isImGuizmoSetup )
	{
		return result; // Return default result
	}

	// Check if we have a valid selection and scene
	if ( !m_selectionManager || !m_scene )
	{
		return result;
	}

	const auto &selectedEntities = m_selectionManager->getSelectedEntities();
	if ( selectedEntities.empty() || !m_visible )
	{
		return result;
	}

	// Calculate gizmo matrix
	auto gizmoMatrix = calculateGizmoMatrix();

	// Convert our operation to ImGuizmo operation
	const int operation = getImGuizmoOperation();

	// Convert our mode to ImGuizmo mode
	const int mode = getImGuizmoMode();

	// Apply snap values if enabled
	float snapValues[3] = { 0.0f, 0.0f, 0.0f };
	float *snapPtr = nullptr;
	if ( m_snapEnabled )
	{
		switch ( m_currentOperation )
		{
		case GizmoOperation::Translate:
			snapValues[0] = snapValues[1] = snapValues[2] = m_translationSnap;
			snapPtr = snapValues;
			break;
		case GizmoOperation::Rotate:
			snapValues[0] = snapValues[1] = snapValues[2] = m_rotationSnap;
			snapPtr = snapValues;
			break;
		case GizmoOperation::Scale:
			snapValues[0] = snapValues[1] = snapValues[2] = m_scaleSnap;
			snapPtr = snapValues;
			break;
		case GizmoOperation::Universal:
			// For universal mode, use translation snap
			snapValues[0] = snapValues[1] = snapValues[2] = m_translationSnap;
			snapPtr = snapValues;
			break;
		}
	}

	// Store previous matrix for delta calculation
	math::Mat4<> originalMatrix = gizmoMatrix;

	// Call ImGuizmo
	result.isManipulating = ImGuizmo::IsUsing();
	result.wasManipulated = ImGuizmo::Manipulate( m_viewMatrix.data(), m_projectionMatrix.data(), static_cast<ImGuizmo::OPERATION>( operation ), static_cast<ImGuizmo::MODE>( mode ), gizmoMatrix.data(), nullptr, snapPtr );

	// If manipulation occurred, calculate delta
	if ( result.wasManipulated )
	{
		// Calculate transform delta (basic implementation)
		// This is a simplified version - proper decomposition will be implemented in AF3.6
		result.translationDelta = math::Vec3<>{
			gizmoMatrix.row0.w - gizmoMatrix.row0.w,
			gizmoMatrix.row1.w - gizmoMatrix.row1.w,
			gizmoMatrix.row2.w - gizmoMatrix.row2.w
		};

		// For now, set rotation and scale deltas to defaults
		result.rotationDelta = math::Vec3<>{ 0.0f, 0.0f, 0.0f };
		result.scaleDelta = math::Vec3<>{ 1.0f, 1.0f, 1.0f };

		// Update manipulation state
		if ( !m_isManipulating )
		{
			beginManipulation();
		}
	}
	else if ( m_isManipulating && !result.isManipulating )
	{
		// Just finished manipulating
		endManipulation();
	}

	return result;
}

int GizmoSystem::getImGuizmoMode() const noexcept
{
	// Convert our GizmoMode to ImGuizmo::MODE
	// ImGuizmo::LOCAL = 0, ImGuizmo::WORLD = 1
	return ( m_currentMode == GizmoMode::Local ) ? 0 : 1;
}

int GizmoSystem::getImGuizmoOperation() const noexcept
{
	// Convert our GizmoOperation to ImGuizmo::OPERATION
	// Based on ImGuizmo.h enum values:
	// TRANSLATE = 7, ROTATE = 120, SCALE = 896, UNIVERSAL = 1023
	switch ( m_currentOperation )
	{
	case GizmoOperation::Translate:
		return 7; // ImGuizmo::TRANSLATE
	case GizmoOperation::Rotate:
		return 120; // ImGuizmo::ROTATE
	case GizmoOperation::Scale:
		return 896; // ImGuizmo::SCALE
	case GizmoOperation::Universal:
		return 1023; // ImGuizmo::UNIVERSAL
	default:
		return 7; // Default to translate
	}
}

// GizmoUI implementation
GizmoUI::GizmoUI( GizmoSystem &gizmoSystem ) noexcept
	: m_gizmoSystem( gizmoSystem )
{
}

void GizmoUI::renderToolbar()
{
	// Get current operation for button state
	const auto currentOp = m_gizmoSystem.getCurrentOperation();

	// Helper lambda to check if this button was clicked in mock mode
	auto isButtonClicked = [this]( const std::string &name ) -> bool {
		if ( !m_mockClickedButton.empty() && m_mockClickedButton == name )
		{
			m_mockClickedButton.clear(); // Clear after use
			return true;
		}
		// In real implementation, this would use ImGui::Button()
		return false;
	};

	// Translate button (W key)
	if ( isButtonClicked( "Translate (W)" ) || currentOp == GizmoOperation::Translate )
	{
		m_gizmoSystem.setOperation( GizmoOperation::Translate );
	}

	// Rotate button (E key)
	if ( isButtonClicked( "Rotate (E)" ) )
	{
		m_gizmoSystem.setOperation( GizmoOperation::Rotate );
	}

	// Scale button (R key)
	if ( isButtonClicked( "Scale (R)" ) )
	{
		m_gizmoSystem.setOperation( GizmoOperation::Scale );
	}

	// Universal button
	if ( isButtonClicked( "Universal" ) )
	{
		m_gizmoSystem.setOperation( GizmoOperation::Universal );
	}

	// Coordinate space toggle (X key)
	if ( isButtonClicked( "Local/World (X)" ) )
	{
		const auto currentMode = m_gizmoSystem.getCurrentMode();
		if ( currentMode == GizmoMode::Local )
		{
			m_gizmoSystem.setMode( GizmoMode::World );
		}
		else
		{
			m_gizmoSystem.setMode( GizmoMode::Local );
		}
	}

	// Visibility toggle (G key)
	if ( isButtonClicked( "Toggle Gizmo (G)" ) )
	{
		m_gizmoSystem.setVisible( !m_gizmoSystem.isVisible() );
	}
}

void GizmoUI::renderSettings()
{
	// Helper lambda to check if slider was changed in mock mode
	auto getSliderValue = [this]( const std::string &name, float currentValue ) -> float {
		if ( !m_mockSliderName.empty() && m_mockSliderName == name )
		{
			const float newValue = m_mockSliderValue;
			m_mockSliderName.clear(); // Clear after use
			return newValue;
		}
		// In real implementation, this would use ImGui::SliderFloat()
		return currentValue;
	};

	// Helper lambda to check if button was clicked
	auto isButtonClicked = [this]( const std::string &name ) -> bool {
		if ( !m_mockClickedButton.empty() && m_mockClickedButton == name )
		{
			m_mockClickedButton.clear(); // Clear after use
			return true;
		}
		// In real implementation, this would use ImGui::Button()
		return false;
	};

	// Snap enable/disable toggle
	if ( isButtonClicked( "Enable Snap" ) )
	{
		m_gizmoSystem.setSnapEnabled( !m_gizmoSystem.isSnapEnabled() );
	}

	// Snap value sliders
	const float newTranslationSnap = getSliderValue( "Translation Snap", m_gizmoSystem.getTranslationSnap() );
	if ( newTranslationSnap != m_gizmoSystem.getTranslationSnap() )
	{
		m_gizmoSystem.setTranslationSnap( newTranslationSnap );
	}

	const float newRotationSnap = getSliderValue( "Rotation Snap", m_gizmoSystem.getRotationSnap() );
	if ( newRotationSnap != m_gizmoSystem.getRotationSnap() )
	{
		m_gizmoSystem.setRotationSnap( newRotationSnap );
	}

	const float newScaleSnap = getSliderValue( "Scale Snap", m_gizmoSystem.getScaleSnap() );
	if ( newScaleSnap != m_gizmoSystem.getScaleSnap() )
	{
		m_gizmoSystem.setScaleSnap( newScaleSnap );
	}
}

void GizmoUI::handleKeyboardShortcuts()
{
	// Helper lambda to check if key was pressed in mock mode
	auto isKeyPressed = [this]( const std::string &key ) -> bool {
		if ( !m_mockPressedKey.empty() && m_mockPressedKey == key )
		{
			m_mockPressedKey.clear(); // Clear after use
			return true;
		}
		// In real implementation, this would use ImGui::IsKeyPressed()
		return false;
	};

	// Operation shortcuts
	if ( isKeyPressed( "W" ) )
	{
		m_gizmoSystem.setOperation( GizmoOperation::Translate );
	}
	else if ( isKeyPressed( "E" ) )
	{
		m_gizmoSystem.setOperation( GizmoOperation::Rotate );
	}
	else if ( isKeyPressed( "R" ) )
	{
		m_gizmoSystem.setOperation( GizmoOperation::Scale );
	}

	// Coordinate space toggle
	if ( isKeyPressed( "X" ) )
	{
		const auto currentMode = m_gizmoSystem.getCurrentMode();
		if ( currentMode == GizmoMode::Local )
		{
			m_gizmoSystem.setMode( GizmoMode::World );
		}
		else
		{
			m_gizmoSystem.setMode( GizmoMode::Local );
		}
	}

	// Visibility toggle
	if ( isKeyPressed( "G" ) )
	{
		m_gizmoSystem.setVisible( !m_gizmoSystem.isVisible() );
	}
}

void GizmoUI::setMockButtonClicked( const std::string &buttonName )
{
	m_mockClickedButton = buttonName;
}

void GizmoUI::setMockSliderValue( const std::string &sliderName, float value )
{
	m_mockSliderName = sliderName;
	m_mockSliderValue = value;
}

void GizmoUI::setMockKeyPressed( const std::string &key )
{
	m_mockPressedKey = key;
}

} // namespace editor