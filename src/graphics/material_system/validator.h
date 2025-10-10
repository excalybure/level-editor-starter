#pragma once

#include <nlohmann/json_fwd.hpp>
#include <string>
#include <vector>

// Forward declarations
namespace graphics::material_system
{
struct MaterialDefinition;
}

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

	// T008: Duplicate ID Detection
	// Validates that all IDs in the merged document are unique across all scopes:
	// - materials, renderPasses, states (all categories), shaders (all types)
	// Returns true if no duplicates found, false if duplicates detected (errors logged via console::error)
	bool validateDuplicateIds( const nlohmann::json &document );
};

} // namespace material_system

namespace graphics::material_system
{

// T010: Reference Validation
class ReferenceValidator
{
public:
	// Validates that a material's references (pass, states, shaders) exist in the document
	// - pass: must be in knownPasses list
	// - states: referenced state IDs must exist in document.states.<category>
	// - shaders: referenced shader IDs must exist in document.shaders.<stage>
	// Returns true if all references valid, false if any undefined (errors logged via console::error)
	bool validateReferences(
		const MaterialDefinition &material,
		const std::vector<std::string> &knownPasses,
		const nlohmann::json &document );
};

} // namespace graphics::material_system
