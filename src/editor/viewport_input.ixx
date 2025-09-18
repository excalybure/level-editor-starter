export module editor.viewport_input;

import editor.selection;
import engine.picking;
import editor.viewport;
import runtime.ecs;
import runtime.entity;
import engine.vec;
import std;

export namespace editor
{

// Mouse selection modes
export enum class SelectionMode {
	Replace,  // Default: replace current selection
	Add,	  // Ctrl+Click: add to selection
	Subtract, // Ctrl+Shift+Click: remove from selection
	Toggle	  // Shift+Click: toggle selection state
};

// Rectangle selection state
export struct RectSelection
{
	bool active = false;
	math::Vec2<> startPos{};
	math::Vec2<> endPos{};
	SelectionMode mode = SelectionMode::Replace;
};

// Mouse picking handler
export class ViewportInputHandler
{
public:
	ViewportInputHandler( SelectionManager &selectionManager,
		picking::PickingSystem &pickingSystem );

	// Mouse input handling
	void handleMouseClick( ecs::Scene &scene,
		const Viewport &viewport,
		const math::Vec2<> &screenPos,
		bool leftButton,
		bool rightButton,
		bool ctrlPressed,
		bool shiftPressed );

	void handleMouseDrag( ecs::Scene &scene,
		const Viewport &viewport,
		const math::Vec2<> &startPos,
		const math::Vec2<> &currentPos,
		bool ctrlPressed,
		bool shiftPressed );

	void handleMouseRelease( ecs::Scene &scene,
		const Viewport &viewport,
		const math::Vec2<> &releasePos );

	// Hover detection for visual feedback
	void handleMouseMove( ecs::Scene &scene,
		const Viewport &viewport,
		const math::Vec2<> &screenPos );

	// Rectangle selection
	const RectSelection &getRectSelection() const { return m_rectSelection; }
	bool isRectSelectionActive() const { return m_rectSelection.active; }

	// Hover state
	ecs::Entity getHoveredEntity() const { return m_hoveredEntity; }

private:
	SelectionManager &m_selectionManager;
	picking::PickingSystem &m_pickingSystem;

	RectSelection m_rectSelection;
	ecs::Entity m_hoveredEntity{};
	math::Vec2<> m_lastMousePos{};

	// Helper methods
	SelectionMode getSelectionMode( bool ctrlPressed, bool shiftPressed ) const;
	std::vector<ecs::Entity> getEntitiesInRect( ecs::Scene &scene,
		const Viewport &viewport,
		const math::Vec2<> &minPos,
		const math::Vec2<> &maxPos ) const;

	void applyRectSelection( ecs::Scene &scene, const Viewport &viewport );
	void updateHoverState( ecs::Scene &scene, const Viewport &viewport, const math::Vec2<> &screenPos );
};

} // namespace editor