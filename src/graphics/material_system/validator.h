#pragma once

#include <nlohmann/json_fwd.hpp>

namespace material_system
{

class Validator
{
public:
	// T005: JSON Schema Validation
	// Validates that the JSON document has the required top-level structure:
	// - "materials" (array) - required
	// - "renderPasses" (array) - required
	// - "defines" (object) - optional
	// - "includes" (array) - optional
	// Returns true if valid, false if invalid (errors logged via console::error)
	bool validateSchema( const nlohmann::json &document );
};

} // namespace material_system
