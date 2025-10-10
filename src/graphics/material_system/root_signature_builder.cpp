#include "graphics/material_system/root_signature_builder.h"
#include "core/console.h"
#include <algorithm>
#include <unordered_set>

namespace graphics::material_system
{

RootSignatureSpec RootSignatureBuilder::Build( const MaterialDefinition &material )
{
	RootSignatureSpec spec;

	// Add bindings from material parameters
	AddParameterBindings( material, spec.resourceBindings );

	// Validate no duplicates (fatal if found)
	ValidateBindings( spec.resourceBindings );

	// Sort for deterministic ordering (important for hashing)
	SortBindings( spec.resourceBindings );

	// Assign slots after sorting
	AssignSlots( spec.resourceBindings );

	return spec;
}

void RootSignatureBuilder::AddParameterBindings(
	const MaterialDefinition &material,
	std::vector<ResourceBinding> &bindings )
{
	// For now, treat all parameters as CBVs (Constant Buffer Views)
	// Full implementation would use shader reflection to determine actual binding types
	for ( const auto &param : material.parameters )
	{
		ResourceBinding binding;
		binding.name = param.name;
		binding.type = ResourceBindingType::CBV;
		binding.slot = -1; // Will be assigned later

		bindings.push_back( binding );
	}
}

void RootSignatureBuilder::ValidateBindings( const std::vector<ResourceBinding> &bindings )
{
	std::unordered_set<std::string> seenNames;

	for ( const auto &binding : bindings )
	{
		if ( seenNames.contains( binding.name ) )
		{
			console::fatal( "Duplicate resource binding name '{}' in root signature", binding.name );
		}
		seenNames.insert( binding.name );
	}
}

void RootSignatureBuilder::SortBindings( std::vector<ResourceBinding> &bindings )
{
	// Sort by name for deterministic ordering
	std::sort( bindings.begin(), bindings.end() );
}

void RootSignatureBuilder::AssignSlots( std::vector<ResourceBinding> &bindings )
{
	// Assign sequential slots after sorting
	int nextSlot = 0;
	for ( auto &binding : bindings )
	{
		binding.slot = nextSlot++;
	}
}

} // namespace graphics::material_system
