export module editor.gizmos;

import engine.vec;

export namespace editor
{

using namespace math;

// Gizmo operation types for transformation modes
export enum class GizmoOperation {
	Translate = 0,
	Rotate = 1,
	Scale = 2,
	Universal = 3
};

// Gizmo coordinate space modes
export enum class GizmoMode {
	Local = 0,
	World = 1
};

// Result structure containing manipulation deltas and state flags
export struct GizmoResult
{
	// Manipulation state flags
	bool wasManipulated = false; // True if manipulation occurred this frame
	bool isManipulating = false; // True if currently manipulating

	// Transform deltas from manipulation
	Vec3<> translationDelta{ 0.0f, 0.0f, 0.0f }; // Translation change
	Vec3<> rotationDelta{ 0.0f, 0.0f, 0.0f };	 // Rotation change (Euler angles in radians)
	Vec3<> scaleDelta{ 1.0f, 1.0f, 1.0f };		 // Scale multipliers (1.0 = no change)
};

// Main gizmo system for rendering and manipulating transforms
export class GizmoSystem
{
public:
	// Constructor with default operation and mode
	GizmoSystem() noexcept = default;

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

private:
	GizmoOperation m_currentOperation = GizmoOperation::Translate;
	GizmoMode m_currentMode = GizmoMode::World;
	bool m_isManipulating = false;
	bool m_wasManipulated = false;
};

} // namespace editor