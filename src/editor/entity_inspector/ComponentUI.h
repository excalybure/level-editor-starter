#pragma once

#include "math/vec.h"

namespace editor
{

/**
 * @brief Utility class for rendering component UI controls
 * 
 * Provides reusable ImGui widgets for editing component properties with
 * consistent styling, color coding, and interaction patterns.
 * All methods return true if the value was modified by the user.
 */
class ComponentUI
{
public:
	/**
	 * @brief Render a Vec3 control with drag interaction
	 * 
	 * Displays three drag floats for X, Y, Z components with color-coded labels
	 * (red, green, blue). Provides a reset button to restore default values.
	 * 
	 * @param label Display label for the control
	 * @param value Vec3 value to edit (modified in place)
	 * @param resetValue Default value for reset button
	 * @param speed Drag speed multiplier (default 0.1f)
	 * @return true if the value was changed by user interaction
	 */
	static bool renderVec3Control( const char *label,
		math::Vec3f &value,
		const math::Vec3f &resetValue = { 0.0f, 0.0f, 0.0f },
		float speed = 0.1f );

	/**
	 * @brief Render a float control with drag interaction
	 * 
	 * Displays a single drag float with optional min/max bounds.
	 * 
	 * @param label Display label for the control
	 * @param value Float value to edit (modified in place)
	 * @param min Minimum allowed value
	 * @param max Maximum allowed value
	 * @return true if the value was changed by user interaction
	 */
	static bool renderFloatControl( const char *label,
		float &value,
		float min = 0.0f,
		float max = 0.0f );
};

} // namespace editor
