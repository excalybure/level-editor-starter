#pragma once

#include <string>
#include "engine/math/vec.h"
#include "engine/math/matrix.h"

// Forward declarations
namespace ecs
{
class Scene;
}

namespace editor
{

class SelectionManager;

// Gizmo operation types for transformation modes
enum class GizmoOperation
{
	Translate = 0,
	Rotate = 1,
	Scale = 2,
	Universal = 3
};

// Gizmo coordinate space modes
enum class GizmoMode
{
	Local = 0,
	World = 1
};

// Result structure containing manipulation deltas and state flags
struct GizmoResult
{
	// Manipulation state flags
	bool wasManipulated = false; // True if manipulation occurred this frame
	bool isManipulating = false; // True if currently manipulating
	bool isHovered = false;		 // True if gizmo is being hovered over

	// Transform deltas from manipulation
	math::Vec3<> translationDelta{ 0.0f, 0.0f, 0.0f }; // Translation change
	math::Vec3<> rotationDelta{ 0.0f, 0.0f, 0.0f };	   // Rotation change (Euler angles in radians)
	math::Vec3<> scaleDelta{ 1.0f, 1.0f, 1.0f };	   // Scale multipliers (1.0 = no change)
};

// Main gizmo system for rendering and manipulating transforms
class GizmoSystem
{
public:
	// Default constructor
	GizmoSystem() noexcept = default;

	// Constructor with SelectionManager and Scene dependencies
	GizmoSystem( SelectionManager &selectionManager, ecs::Scene &scene ) noexcept;

	// Accessors for current state
	constexpr GizmoOperation getCurrentOperation() const noexcept
	{
		return m_currentOperation;
	}

	constexpr GizmoMode getCurrentMode() const noexcept
	{
		return m_currentMode;
	}

	// Mutators for current state
	constexpr void setOperation( GizmoOperation operation ) noexcept
	{
		m_currentOperation = operation;
	}

	constexpr void setMode( GizmoMode mode ) noexcept
	{
		m_currentMode = mode;
	}

	// Snap-to-grid functionality
	constexpr float getTranslationSnap() const noexcept { return m_translationSnap; }
	constexpr float getRotationSnap() const noexcept { return m_rotationSnap; }
	constexpr float getScaleSnap() const noexcept { return m_scaleSnap; }
	constexpr bool isSnapEnabled() const noexcept { return m_snapEnabled; }

	constexpr void setTranslationSnap( float snap ) noexcept { m_translationSnap = snap; }
	constexpr void setRotationSnap( float snap ) noexcept { m_rotationSnap = snap; }
	constexpr void setScaleSnap( float snap ) noexcept { m_scaleSnap = snap; }
	constexpr void setSnapEnabled( bool enabled ) noexcept { m_snapEnabled = enabled; }

	// Visibility control
	constexpr bool isVisible() const noexcept { return m_visible; }
	constexpr void setVisible( bool visible ) noexcept { m_visible = visible; }

	// Selection center calculation
	math::Vec3<> calculateSelectionCenter() const;

	// Gizmo matrix calculation
	math::Mat4<> calculateGizmoMatrix() const;

	// Transform delta application
	void applyTransformDelta( const GizmoResult &delta );

	// State management for manipulation tracking
	constexpr bool isManipulating() const noexcept
	{
		return m_isManipulating;
	}

	constexpr bool wasManipulated() const noexcept
	{
		return m_wasManipulated;
	}

	constexpr void beginManipulation() noexcept
	{
		m_isManipulating = true;
		m_wasManipulated = false;
	}

	constexpr void endManipulation() noexcept
	{
		m_isManipulating = false;
		m_wasManipulated = true;
	}

	constexpr void resetManipulationState() noexcept
	{
		m_isManipulating = false;
		m_wasManipulated = false;
	}

	// Selection validation
	bool hasValidSelection() const noexcept;

	// ImGuizmo integration methods
	bool setupImGuizmo( const math::Mat4<> &viewMatrix, const math::Mat4<> &projectionMatrix, const math::Vec4<> &viewport ) noexcept;
	GizmoResult renderGizmo() noexcept;

	// ImGuizmo coordinate space conversion
	int getImGuizmoMode() const noexcept;
	int getImGuizmoOperation() const noexcept;

private:
	SelectionManager *m_selectionManager = nullptr;
	ecs::Scene *m_scene = nullptr;
	GizmoOperation m_currentOperation = GizmoOperation::Translate;
	GizmoMode m_currentMode = GizmoMode::World;
	bool m_isManipulating = false;
	bool m_wasManipulated = false;

	// Snap-to-grid settings
	float m_translationSnap = 1.0f; // 1 unit
	float m_rotationSnap = 15.0f;	// 15 degrees
	float m_scaleSnap = 0.1f;		// 0.1 scale increment
	bool m_snapEnabled = false;

	// Visibility state
	bool m_visible = true;

	// ImGuizmo state
	bool m_isImGuizmoSetup = false;
	math::Mat4<> m_viewMatrix = math::Mat4<>::identity();
	math::Mat4<> m_projectionMatrix = math::Mat4<>::identity();
	math::Vec4<> m_viewportRect{ 0.0f, 0.0f, 0.0f, 0.0f };
};

// UI class for gizmo controls and settings
class GizmoUI
{
public:
	// Constructor with GizmoSystem reference
	explicit GizmoUI( GizmoSystem &gizmoSystem ) noexcept;

	// Accessors
	constexpr GizmoSystem &getGizmoSystem() noexcept { return m_gizmoSystem; }
	constexpr const GizmoSystem &getGizmoSystem() const noexcept { return m_gizmoSystem; }

	// UI rendering methods
	void renderToolbar( bool *isOpen );
	void renderSettings( bool *isOpen );

	// Keyboard shortcut handling
	void handleKeyboardShortcuts();

	// Testing/mock functionality
	void setMockButtonClicked( const std::string &buttonName );
	void setMockSliderValue( const std::string &sliderName, float value );
	void setMockKeyPressed( const std::string &key );

private:
	GizmoSystem &m_gizmoSystem;

	// Mock state for testing
	std::string m_mockClickedButton;
	std::string m_mockSliderName;
	float m_mockSliderValue = 0.0f;
	std::string m_mockPressedKey;
};

} // namespace editor
