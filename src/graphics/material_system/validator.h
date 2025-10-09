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

	// T006: Parameter Type Validation
	// Validates that a parameter declaration has:
	// - Valid type: one of {float, int, bool, float4}
	// - Matching default value type
	// - For float4: default must be array of exactly 4 numbers
	// Returns true if valid, false if invalid (errors logged via console::error)
	bool validateParameterType( const nlohmann::json &parameter );
};

} // namespace material_system
