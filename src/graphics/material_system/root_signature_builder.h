#pragma once

#include "graphics/material_system/parser.h"
#include <string>
#include <vector>

namespace graphics::material_system
{

// Resource binding types for root signature
enum class ResourceBindingType
{
	CBV, // Constant Buffer View
	SRV, // Shader Resource View
	UAV, // Unordered Access View
	Sampler
};

// Individual resource binding in root signature
struct ResourceBinding
{
	std::string name;
	ResourceBindingType type;
	int slot;

	bool operator<( const ResourceBinding &other ) const
	{
		// Sort by name for deterministic ordering
		return name < other.name;
	}
};

// Root signature specification
struct RootSignatureSpec
{
	std::vector<ResourceBinding> resourceBindings;
};

// Builds root signatures from material definitions
class RootSignatureBuilder
{
public:
	// Build root signature spec from material definition
	// Note: This simplified version infers bindings from parameters only.
	// Full implementation would use shader reflection (DXIL analysis).
	static RootSignatureSpec Build( const MaterialDefinition &material );

private:
	// Convert material parameters to resource bindings
	static void AddParameterBindings(
		const MaterialDefinition &material,
		std::vector<ResourceBinding> &bindings );

	// Validate no duplicate binding names (fatal if found)
	static void ValidateBindings( const std::vector<ResourceBinding> &bindings );

	// Sort bindings for deterministic hashing
	static void SortBindings( std::vector<ResourceBinding> &bindings );

	// Assign slots to bindings
	static void AssignSlots( std::vector<ResourceBinding> &bindings );
};

} // namespace graphics::material_system
