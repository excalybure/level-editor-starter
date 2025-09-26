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

bool GizmoSystem::hasValidSelection() const noexcept
{
	if ( !m_selectionManager || !m_scene )
	{
		return false;
	}

	const auto &selectedEntities = m_selectionManager->getSelectedEntities();
	if ( selectedEntities.empty() )
	{
		return false;
	}

	// Check if any selected entity has a transform component
	for ( const auto entity : selectedEntities )
	{
		if ( m_scene->hasComponent<components::Transform>( entity ) )
		{
			return true;
		}
	}

	return false;
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

	// ImGuizmo expects column-major matrices, but our math library uses row-major
	// We need to transpose before passing to ImGuizmo
	const auto viewMatrixTransposed = m_viewMatrix.transpose();
	const auto projMatrixTransposed = m_projectionMatrix.transpose();
	auto gizmoMatrixTransposed = gizmoMatrix.transpose();

	// Call ImGuizmo
	result.isManipulating = ImGuizmo::IsUsing();
	result.wasManipulated = ImGuizmo::Manipulate(
		viewMatrixTransposed.data(),
		projMatrixTransposed.data(),
		static_cast<ImGuizmo::OPERATION>( operation ),
		static_cast<ImGuizmo::MODE>( mode ),
		gizmoMatrixTransposed.data(),
		nullptr,
		snapPtr );

	// If manipulation occurred, transpose back and calculate delta
	if ( result.wasManipulated )
	{
		// Transpose the result back to our row-major format
		gizmoMatrix = gizmoMatrixTransposed.transpose();

		// Calculate transform delta (basic implementation)
		// This is a simplified version - proper decomposition will be implemented in AF3.6
		result.translationDelta = math::Vec3<>{
			gizmoMatrix.row0.w - originalMatrix.row0.w,
			gizmoMatrix.row1.w - originalMatrix.row1.w,
			gizmoMatrix.row2.w - originalMatrix.row2.w
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

GizmoResult GizmoSystem::renderGizmo( const math::Mat4<> &viewMatrix, const math::Mat4<> &projectionMatrix, const math::Vec4<> &viewport ) noexcept
{
	// Setup ImGuizmo with provided matrices and viewport
	if ( !setupImGuizmo( viewMatrix, projectionMatrix, viewport ) )
	{
		return GizmoResult{}; // Return default result if setup fails
	}

	// Call the existing renderGizmo method
	return renderGizmo();
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

void GizmoUI::renderToolbar( bool *isOpen )
{
	if ( !isOpen || !*isOpen )
	{
		return; // Window should not be shown
	}

	// Only create window if ImGui context exists and window should be open
	if ( ImGui::GetCurrentContext() != nullptr )
	{
		// Create a floating window for gizmo toolbar with close button support
		if ( !ImGui::Begin( "Gizmo Tools", isOpen ) )
		{
			ImGui::End();
			return; // Window is collapsed, don't render content
		}
	}

	// Get current operation for button state
	const auto currentOp = m_gizmoSystem.getCurrentOperation();
	const auto currentMode = m_gizmoSystem.getCurrentMode();

	// Helper lambda to check if this button was clicked (real ImGui or mock mode)
	const auto isButtonClicked = [this]( const std::string &name, const std::string &label ) -> bool {
		// Check mock mode first for testing
		if ( !m_mockClickedButton.empty() && m_mockClickedButton == name )
		{
			m_mockClickedButton.clear(); // Clear after use
			return true;
		}
		// Real ImGui button (only if ImGui context exists)
		if ( ImGui::GetCurrentContext() != nullptr )
		{
			return ImGui::Button( label.c_str() );
		}
		return false;
	};

	// Helper lambda for selectable buttons (shows selected state)
	const auto isSelectableButtonClicked = [this]( const std::string &name, const std::string &label, bool isSelected ) -> bool {
		// Check mock mode first for testing
		if ( !m_mockClickedButton.empty() && m_mockClickedButton == name )
		{
			m_mockClickedButton.clear(); // Clear after use
			return true;
		}

		// Real ImGui selectable button (different style when selected)
		if ( ImGui::GetCurrentContext() != nullptr )
		{
			if ( isSelected )
			{
				ImGui::PushStyleColor( ImGuiCol_Button, ImGui::GetStyleColorVec4( ImGuiCol_ButtonActive ) );
			}
			const bool clicked = ImGui::Button( label.c_str() );
			if ( isSelected )
			{
				ImGui::PopStyleColor();
			}
			return clicked;
		}
		return false;
	};

	// Operation mode section
	if ( ImGui::GetCurrentContext() != nullptr )
	{
		ImGui::Text( "Operation Mode:" );
	}

	// Translate button (W key)
	if ( isSelectableButtonClicked( "Translate (W)", "Translate (W)", currentOp == GizmoOperation::Translate ) )
	{
		m_gizmoSystem.setOperation( GizmoOperation::Translate );
	}
	if ( ImGui::GetCurrentContext() != nullptr )
	{
		ImGui::SameLine();
	}

	// Rotate button (E key)
	if ( isSelectableButtonClicked( "Rotate (E)", "Rotate (E)", currentOp == GizmoOperation::Rotate ) )
	{
		m_gizmoSystem.setOperation( GizmoOperation::Rotate );
	}
	if ( ImGui::GetCurrentContext() != nullptr )
	{
		ImGui::SameLine();
	}

	// Scale button (R key)
	if ( isSelectableButtonClicked( "Scale (R)", "Scale (R)", currentOp == GizmoOperation::Scale ) )
	{
		m_gizmoSystem.setOperation( GizmoOperation::Scale );
	}
	if ( ImGui::GetCurrentContext() != nullptr )
	{
		ImGui::SameLine();
	}

	// Universal button
	if ( isSelectableButtonClicked( "Universal", "Universal", currentOp == GizmoOperation::Universal ) )
	{
		m_gizmoSystem.setOperation( GizmoOperation::Universal );
	}

	// Separator between operation and other controls
	if ( ImGui::GetCurrentContext() != nullptr )
	{
		ImGui::Separator();
		ImGui::Text( "Coordinate Space:" );
	}

	// Coordinate space toggle (X key)
	const std::string modeLabel = ( currentMode == GizmoMode::Local ) ? "Local (X)" : "World (X)";
	if ( isButtonClicked( "Local/World (X)", modeLabel ) )
	{
		if ( currentMode == GizmoMode::Local )
		{
			m_gizmoSystem.setMode( GizmoMode::World );
		}
		else
		{
			m_gizmoSystem.setMode( GizmoMode::Local );
		}
	}

	if ( ImGui::GetCurrentContext() != nullptr )
	{
		ImGui::Separator();
		ImGui::Text( "Visibility:" );
	}

	// Visibility toggle (G key)
	const std::string visibilityLabel = m_gizmoSystem.isVisible() ? "Hide Gizmo (G)" : "Show Gizmo (G)";
	if ( isButtonClicked( "Toggle Gizmo (G)", visibilityLabel ) )
	{
		m_gizmoSystem.setVisible( !m_gizmoSystem.isVisible() );
	}

	// End the ImGui window
	if ( ImGui::GetCurrentContext() != nullptr )
	{
		ImGui::End();
	}
}

void GizmoUI::renderSettings( bool *isOpen )
{
	if ( !isOpen || !*isOpen )
	{
		return; // Window should not be shown
	}

	// Only create window if ImGui context exists and window should be open
	if ( ImGui::GetCurrentContext() != nullptr )
	{
		// Create a floating window for gizmo settings with close button support
		if ( !ImGui::Begin( "Gizmo Settings", isOpen ) )
		{
			ImGui::End();
			return; // Window is collapsed, don't render content
		}
	}

	// Helper lambda to handle slider changes (real ImGui or mock mode)
	const auto handleSlider = [this]( const std::string &name, const std::string &label, float *value, float min, float max ) -> bool {
		// Check mock mode first for testing
		if ( !m_mockSliderName.empty() && m_mockSliderName == name )
		{
			*value = m_mockSliderValue;
			m_mockSliderName.clear(); // Clear after use
			return true;
		}
		// Real ImGui slider (only if ImGui context exists)
		if ( ImGui::GetCurrentContext() != nullptr )
		{
			return ImGui::SliderFloat( label.c_str(), value, min, max );
		}
		return false;
	};

	// Helper lambda to handle button clicks (real ImGui or mock mode)
	const auto isButtonClicked = [this]( const std::string &name, const std::string &label ) -> bool {
		// Check mock mode first for testing
		if ( !m_mockClickedButton.empty() && m_mockClickedButton == name )
		{
			m_mockClickedButton.clear(); // Clear after use
			return true;
		}
		// Real ImGui button (only if ImGui context exists)
		if ( ImGui::GetCurrentContext() != nullptr )
		{
			return ImGui::Button( label.c_str() );
		}
		return false;
	};

	// Helper lambda for checkbox
	const auto handleCheckbox = [this]( const std::string &name, const std::string &label, bool *value ) -> bool {
		// Check mock mode first for testing (using button click for checkbox toggle)
		if ( !m_mockClickedButton.empty() && m_mockClickedButton == name )
		{
			*value = !*value;
			m_mockClickedButton.clear(); // Clear after use
			return true;
		}
		// Real ImGui checkbox (only if ImGui context exists)
		if ( ImGui::GetCurrentContext() != nullptr )
		{
			return ImGui::Checkbox( label.c_str(), value );
		}
		return false;
	};

	// Snap enable/disable section
	if ( ImGui::GetCurrentContext() != nullptr )
	{
		ImGui::Text( "Snap-to-Grid:" );
	}

	// Snap enable/disable toggle
	bool snapEnabled = m_gizmoSystem.isSnapEnabled();
	if ( handleCheckbox( "Enable Snap", "Enable Snap", &snapEnabled ) )
	{
		m_gizmoSystem.setSnapEnabled( snapEnabled );
	}

	// Only show snap value sliders if snap is enabled
	if ( m_gizmoSystem.isSnapEnabled() )
	{
		if ( ImGui::GetCurrentContext() != nullptr )
		{
			ImGui::Separator();
			ImGui::Text( "Snap Values:" );
		}

		// Translation snap slider
		float translationSnap = m_gizmoSystem.getTranslationSnap();
		if ( handleSlider( "Translation Snap", "Translation##trans", &translationSnap, 0.1f, 10.0f ) )
		{
			m_gizmoSystem.setTranslationSnap( translationSnap );
		}

		// Rotation snap slider (in degrees)
		float rotationSnap = m_gizmoSystem.getRotationSnap();
		if ( handleSlider( "Rotation Snap", "Rotation (deg)##rot", &rotationSnap, 1.0f, 90.0f ) )
		{
			m_gizmoSystem.setRotationSnap( rotationSnap );
		}

		// Scale snap slider
		float scaleSnap = m_gizmoSystem.getScaleSnap();
		if ( handleSlider( "Scale Snap", "Scale##scale", &scaleSnap, 0.01f, 1.0f ) )
		{
			m_gizmoSystem.setScaleSnap( scaleSnap );
		}
	}

	// End the ImGui window
	if ( ImGui::GetCurrentContext() != nullptr )
	{
		ImGui::End();
	}
}

void GizmoUI::handleKeyboardShortcuts()
{
	// Helper lambda to check if key was pressed (real ImGui or mock mode)
	auto isKeyPressed = [this]( const std::string &key, ImGuiKey imguiKey ) -> bool {
		// Check mock mode first for testing
		if ( !m_mockPressedKey.empty() && m_mockPressedKey == key )
		{
			m_mockPressedKey.clear(); // Clear after use
			return true;
		}
		// Real ImGui key detection (only if ImGui context exists)
		if ( ImGui::GetCurrentContext() != nullptr )
		{
			return ImGui::IsKeyPressed( imguiKey );
		}
		return false;
	};

	// Operation shortcuts
	if ( isKeyPressed( "W", ImGuiKey_W ) )
	{
		m_gizmoSystem.setOperation( GizmoOperation::Translate );
	}
	else if ( isKeyPressed( "E", ImGuiKey_E ) )
	{
		m_gizmoSystem.setOperation( GizmoOperation::Rotate );
	}
	else if ( isKeyPressed( "R", ImGuiKey_R ) )
	{
		m_gizmoSystem.setOperation( GizmoOperation::Scale );
	}

	// Coordinate space toggle
	if ( isKeyPressed( "X", ImGuiKey_X ) )
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
	if ( isKeyPressed( "G", ImGuiKey_G ) )
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