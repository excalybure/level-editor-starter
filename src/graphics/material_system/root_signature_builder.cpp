#include "graphics/material_system/root_signature_builder.h"
#include "graphics/shader_manager/shader_manager.h"
#include "core/console.h"
#include <algorithm>
#include <unordered_set>

namespace graphics::material_system
{

// New reflection-based Build() implementation
RootSignatureSpec RootSignatureBuilder::Build(
	const MaterialPass &pass,
	shader_manager::ShaderManager *shaderManager,
	ShaderReflectionCache *reflectionCache )
{
	RootSignatureSpec spec;

	// Validate inputs
	if ( !shaderManager )
	{
		console::error( "RootSignatureBuilder::Build: shaderManager is null" );
		return spec;
	}

	if ( !reflectionCache )
	{
		console::error( "RootSignatureBuilder::Build: reflectionCache is null" );
		return spec;
	}

	// Iterate all shaders in the pass and reflect each one
	for ( const auto &shaderRef : pass.shaders )
	{
		// Convert ShaderStage to ShaderType for ShaderManager
		shader_manager::ShaderType shaderType;
		switch ( shaderRef.stage )
		{
		case ShaderStage::Vertex:
			shaderType = shader_manager::ShaderType::Vertex;
			break;
		case ShaderStage::Pixel:
			shaderType = shader_manager::ShaderType::Pixel;
			break;
		case ShaderStage::Compute:
			shaderType = shader_manager::ShaderType::Compute;
			break;
		case ShaderStage::Geometry:
			shaderType = shader_manager::ShaderType::Geometry;
			break;
		case ShaderStage::Hull:
			shaderType = shader_manager::ShaderType::Hull;
			break;
		case ShaderStage::Domain:
			shaderType = shader_manager::ShaderType::Domain;
			break;
		default:
			console::error( "RootSignatureBuilder::Build: unknown shader stage" );
			continue;
		}

		// Register shader with ShaderManager to get handle
		const auto shaderHandle = shaderManager->registerShader(
			shaderRef.file,
			shaderRef.entryPoint,
			shaderRef.profile,
			shaderType );

		if ( shaderHandle == shader_manager::INVALID_SHADER_HANDLE )
		{
			console::error( "RootSignatureBuilder::Build: failed to register shader: {}", shaderRef.file );
			continue;
		}

		// Get compiled shader blob
		const shader_manager::ShaderBlob *blob = shaderManager->getShaderBlob( shaderHandle );
		if ( !blob || !blob->blob )
		{
			console::error( "RootSignatureBuilder::Build: failed to get shader blob for: {}", shaderRef.file );
			continue;
		}

		// Reflect shader to extract resource bindings
		const auto reflectionResult = reflectionCache->GetOrReflect( blob, shaderHandle );
		if ( !reflectionResult.success )
		{
			console::error( "RootSignatureBuilder::Build: shader reflection failed for: {}", shaderRef.file );
			continue;
		}

		// Add bindings from this shader to the spec
		for ( const auto &binding : reflectionResult.bindings )
		{
			spec.resourceBindings.push_back( binding );
		}
	}

	// TODO: Merge and validate bindings (AF3)
	// TODO: Group bindings for root signature (AF4)

	// For now, just return the raw collected bindings
	ValidateBindings( spec.resourceBindings );
	SortBindings( spec.resourceBindings );

	return spec;
}

// Legacy Build() implementation - DEPRECATED
RootSignatureSpec RootSignatureBuilder::Build( const MaterialDefinition &material, bool includeFrameConstants, bool includeObjectConstants, bool includeMaterialConstants )
{
	RootSignatureSpec spec;

	// Optionally add default frame/view constant buffer binding (b0)
	// Most shaders need this for view-projection matrices, etc.
	if ( includeFrameConstants )
	{
		ResourceBinding frameConstants;
		frameConstants.name = "FrameConstants";
		frameConstants.type = ResourceBindingType::CBV;
		frameConstants.slot = 0; // Always at b0
		spec.resourceBindings.push_back( frameConstants );
	}

	// Optionally add object transform constant buffer binding (b1)
	// Contains world matrix and normal matrix
	if ( includeObjectConstants )
	{
		ResourceBinding objectConstants;
		objectConstants.name = "ObjectConstants";
		objectConstants.type = ResourceBindingType::CBV;
		objectConstants.slot = 1; // Always at b1
		spec.resourceBindings.push_back( objectConstants );
	}

	// Optionally add material properties constant buffer binding (b2)
	// Contains base color, metallic, roughness, etc.
	if ( includeMaterialConstants )
	{
		ResourceBinding materialConstants;
		materialConstants.name = "MaterialConstants";
		materialConstants.type = ResourceBindingType::CBV;
		materialConstants.slot = 2; // Always at b2
		spec.resourceBindings.push_back( materialConstants );
	}

	// Add bindings from material parameters
	AddParameterBindings( material, spec.resourceBindings );

	// Validate no duplicates (fatal if found)
	ValidateBindings( spec.resourceBindings );

	// Sort for deterministic ordering (important for hashing)
	SortBindings( spec.resourceBindings );

	// Assign slots after sorting (starting from b1 since b0 is reserved)
	AssignSlots( spec.resourceBindings );

	return spec;
}

void RootSignatureBuilder::AddParameterBindings(
	const MaterialDefinition &material,
	std::vector<ResourceBinding> &bindings )
{
	// TODO: Parameters are now in MaterialPass, not MaterialDefinition
	// This needs to be refactored to iterate over all passes and collect their parameters
	// For now, collecting from first pass if it exists
	if ( !material.passes.empty() && !material.passes[0].parameters.empty() )
	{
		for ( const auto &param : material.passes[0].parameters )
		{
			ResourceBinding binding;
			binding.name = param.name;
			binding.type = ResourceBindingType::CBV;
			binding.slot = -1; // Will be assigned later

			bindings.push_back( binding );
		}
	}
}

void RootSignatureBuilder::ValidateBindings( const std::vector<ResourceBinding> &bindings )
{
	std::unordered_set<std::string> seenNames;

	for ( const auto &binding : bindings )
	{
		if ( seenNames.contains( binding.name ) )
		{
			console::errorAndThrow( "Duplicate resource binding name '{}' in root signature", binding.name );
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
	// Assign sequential slots for bindings that don't have explicit slots
	// Start from the first available slot (b1 if FrameConstants at b0 exists, otherwise b0)
	int nextSlot = 0;

	// Find the first available slot after any explicitly assigned slots
	for ( const auto &binding : bindings )
	{
		if ( binding.slot >= nextSlot )
		{
			nextSlot = binding.slot + 1;
		}
	}

	for ( auto &binding : bindings )
	{
		// Skip bindings that already have explicit slots (like FrameConstants at b0)
		if ( binding.slot >= 0 )
		{
			continue;
		}

		binding.slot = nextSlot++;
	}
}

} // namespace graphics::material_system
