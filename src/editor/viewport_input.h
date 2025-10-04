#pragma once

#include <vector>
#include "math/vec.h"
#include "runtime/entity.h"

// Forward declarations
namespace editor
{
class SelectionManager;
class Viewport;
} // namespace editor
namespace picking
{
class PickingSystem;
}
namespace ecs
{
class Scene;
}
namespace systems
{
class SystemManager;
}

namespace editor
{

// Mouse selection modes
enum class SelectionMode
{
	Replace,  // Default: replace current selection
	Add,	  // Ctrl+Click: add to selection
	Subtract, // Ctrl+Shift+Click: remove from selection
	Toggle	  // Shift+Click: toggle selection state
};

// Rectangle selection state
struct RectSelection
{
	bool active = false;
	math::Vec2f startPos{};
	math::Vec2f endPos{};
	SelectionMode mode = SelectionMode::Replace;
};

// Mouse picking handler
class ViewportInputHandler
{
public:
	ViewportInputHandler( SelectionManager &selectionManager,
		picking::PickingSystem &pickingSystem,
		systems::SystemManager &systemManager );

	// Mouse input handling
	void handleMouseClick( ecs::Scene &scene,
		const Viewport &viewport,
		const math::Vec2f &viewportPos,
		bool leftButton,
		bool rightButton,
		bool ctrlPressed,
		bool shiftPressed );

	void handleMouseDrag( ecs::Scene &scene,
		const Viewport &viewport,
		const math::Vec2f &startPos,
		const math::Vec2f &currentPos,
		bool ctrlPressed,
		bool shiftPressed );

	void handleMouseRelease( ecs::Scene &scene,
		const Viewport &viewport,
		const math::Vec2f &releasePos );

	// Hover detection for visual feedback
	void handleMouseMove( ecs::Scene &scene,
		const Viewport &viewport,
		const math::Vec2f &viewportPos );

	// Rectangle selection
	const RectSelection &getRectSelection() const { return m_rectSelection; }
	bool isRectSelectionActive() const { return m_rectSelection.active; }

	// Hover state
	ecs::Entity getHoveredEntity() const { return m_hoveredEntity; }

private:
	SelectionManager &m_selectionManager;
	picking::PickingSystem &m_pickingSystem;
	systems::SystemManager &m_systemManager;

	RectSelection m_rectSelection;
	ecs::Entity m_hoveredEntity{};
	math::Vec2f m_lastMousePos{};

	// Helper methods
	SelectionMode getSelectionMode( bool ctrlPressed, bool shiftPressed ) const;
	std::vector<ecs::Entity> getEntitiesInRect( ecs::Scene &scene,
		const Viewport &viewport,
		const math::Vec2f &minPos,
		const math::Vec2f &maxPos ) const;

	void applyRectSelection( ecs::Scene &scene, const Viewport &viewport );
	void updateHoverState( ecs::Scene &scene, const Viewport &viewport, const math::Vec2f &viewportPos );
};

} // namespace editor
