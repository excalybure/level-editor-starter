#include "graphics/material_system/root_signature_builder.h"
#include "graphics/shader_manager/shader_manager.h"
#include "core/console.h"
#include <algorithm>
#include <unordered_set>
#include <cctype>
#include <stdexcept>

namespace graphics::material_system
{

// New reflection-based Build() implementation
RootSignatureSpec RootSignatureBuilder::Build(
	const MaterialPass &pass,
	shader_manager::ShaderManager *shaderManager,
	ShaderReflectionCache *reflectionCache )
{
	RootSignatureSpec spec;
	std::vector<ResourceBinding> allBindings;

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

		// Add bindings from this shader to temporary collection
		for ( const auto &binding : reflectionResult.bindings )
		{
			allBindings.push_back( binding );
		}
	}

	// Merge and validate bindings (removes duplicates, validates conflicts)
	const auto merged = MergeAndValidateBindings( allBindings );

	// Group bindings into CBVs vs descriptor table resources
	GroupBindingsForRootSignature( merged, spec );

	return spec;
}

std::vector<ResourceBinding> RootSignatureBuilder::MergeAndValidateBindings(
	const std::vector<ResourceBinding> &bindings )
{
	std::vector<ResourceBinding> merged;
	std::unordered_map<std::string, ResourceBinding> bindingMap;

	for ( const auto &binding : bindings )
	{
		const auto it = bindingMap.find( binding.name );
		if ( it != bindingMap.end() )
		{
			// Found duplicate - validate it matches
			const auto &existing = it->second;

			if ( existing.type != binding.type )
			{
				console::errorAndThrow(
					"Binding '{}' has conflicting types across shaders: {} vs {}",
					binding.name,
					static_cast<int>( existing.type ),
					static_cast<int>( binding.type ) );
			}

			if ( existing.slot != binding.slot )
			{
				console::errorAndThrow(
					"Binding '{}' has conflicting register slots across shaders: {} vs {}",
					binding.name,
					existing.slot,
					binding.slot );
			}

			// Duplicate is valid - skip it (already in map)
		}
		else
		{
			// New binding - add to map
			bindingMap[binding.name] = binding;
		}
	}

	// Convert map to vector
	merged.reserve( bindingMap.size() );
	for ( const auto &pair : bindingMap )
	{
		merged.push_back( pair.second );
	}

	return merged;
}

bool RootSignatureBuilder::IsStaticSamplerName( const std::string &name )
{
	// Common sampler names that should be static (defined in root signature)
	// These samplers don't change per draw call, so static definition is efficient
	static const std::unordered_set<std::string> staticSamplerNames = {
		"linearSampler",
		"pointSampler",
		"anisotropicSampler",
		"comparisonSampler"
	};

	// Check legacy names first
	if ( staticSamplerNames.find( name ) != staticSamplerNames.end() )
	{
		return true;
	}

	// Check for pattern-based names: (linear|point|aniso)(Wrap|Clamp)(\d+X)?Sampler
	// Examples: linearWrapSampler, linearClampSampler, anisoWrap4XSampler, anisoClamp16XSampler

	// Must end with "Sampler"
	if ( name.size() <= 7 || name.substr( name.size() - 7 ) != "Sampler" )
	{
		return false;
	}

	// Check for filter type prefix: linear, point, or aniso
	if ( name.find( "linear" ) == 0 || name.find( "point" ) == 0 || name.find( "aniso" ) == 0 )
	{
		// Must contain either Wrap or Clamp
		if ( name.find( "Wrap" ) != std::string::npos || name.find( "Clamp" ) != std::string::npos )
		{
			return true;
		}
	}

	return false;
}

D3D12_TEXTURE_ADDRESS_MODE RootSignatureBuilder::GetAddressModeForSamplerName( const std::string &name )
{
	// Parse address mode from sampler name
	// Look for "Wrap" or "Clamp" in the name

	if ( name.find( "Clamp" ) != std::string::npos )
	{
		return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	}

	// Default to WRAP if not explicitly clamped
	return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
}

uint32_t RootSignatureBuilder::GetMaxAnisotropyForSamplerName( const std::string &name )
{
	// Parse anisotropy level from sampler name
	// Look for patterns like "4X", "8X", "16X" etc in the name
	// Examples: anisoWrap4XSampler -> 4, anisoClamp16XSampler -> 16

	// Find "X" followed by "Sampler"
	const size_t xPos = name.find( 'X' );
	if ( xPos != std::string::npos && xPos > 0 && xPos + 8 == name.size() ) // "XSampler" is 8 chars
	{
		// Extract digit(s) before the 'X'
		// Go backwards from xPos to find the start of the number
		size_t digitStart = xPos - 1;
		while ( digitStart > 0 && std::isdigit( name[digitStart - 1] ) )
		{
			--digitStart;
		}

		try
		{
			const auto anisotropyStr = name.substr( digitStart, xPos - digitStart );
			const int anisotropy = std::stoi( anisotropyStr );

			// Clamp to valid range [1, 16]
			if ( anisotropy >= 1 && anisotropy <= 16 )
			{
				return static_cast<uint32_t>( anisotropy );
			}
		}
		catch ( const std::exception & )
		{
			// If parsing fails, fall through to default
		}
	}

	// Default to 1 (no anisotropy)
	return 1;
}

D3D12_FILTER RootSignatureBuilder::GetFilterForSamplerName( const std::string &name )
{
	// Map sampler names to D3D12 filter types
	// Support both legacy names and pattern-based names

	// Check for legacy names first
	if ( name == "pointSampler" )
	{
		return D3D12_FILTER_MIN_MAG_MIP_POINT;
	}
	else if ( name == "anisotropicSampler" )
	{
		return D3D12_FILTER_ANISOTROPIC;
	}
	else if ( name == "comparisonSampler" )
	{
		return D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	}

	// Check for pattern-based names
	// Extract filter type from prefix: (linear|point|aniso)

	if ( name.find( "point" ) == 0 )
	{
		return D3D12_FILTER_MIN_MAG_MIP_POINT;
	}
	else if ( name.find( "aniso" ) == 0 )
	{
		return D3D12_FILTER_ANISOTROPIC;
	}

	// Default to linear for linearSampler, linearWrapSampler, linearClampSampler, and unknown names
	return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
}

void RootSignatureBuilder::GroupBindingsForRootSignature(
	const std::vector<ResourceBinding> &merged,
	RootSignatureSpec &outSpec )
{
	// Clear output vectors
	outSpec.cbvRootDescriptors.clear();
	outSpec.descriptorTableResources.clear();
	outSpec.staticSamplers.clear();

	// Separate bindings by type
	for ( const auto &binding : merged )
	{
		if ( binding.type == ResourceBindingType::CBV )
		{
			// CBVs use root descriptors (2 DWORDs per CBV)
			outSpec.cbvRootDescriptors.push_back( binding );
		}
		else if ( binding.type == ResourceBindingType::Sampler && IsStaticSamplerName( binding.name ) )
		{
			// Known static samplers - add to static sampler list
			StaticSamplerBinding staticSampler;
			staticSampler.name = binding.name;
			staticSampler.slot = binding.slot;
			staticSampler.filter = GetFilterForSamplerName( binding.name );
			staticSampler.addressMode = GetAddressModeForSamplerName( binding.name );
			staticSampler.maxAnisotropy = GetMaxAnisotropyForSamplerName( binding.name );
			outSpec.staticSamplers.push_back( staticSampler );
		}
		else
		{
			// SRVs, UAVs, and unknown Samplers use descriptor tables (1 DWORD per table)
			outSpec.descriptorTableResources.push_back( binding );
		}
	}

	// Sort all groups by slot for deterministic output matching shader register order
	std::sort( outSpec.cbvRootDescriptors.begin(), outSpec.cbvRootDescriptors.end(), []( const ResourceBinding &a, const ResourceBinding &b ) { return a.slot < b.slot; } );
	std::sort( outSpec.descriptorTableResources.begin(), outSpec.descriptorTableResources.end(), []( const ResourceBinding &a, const ResourceBinding &b ) { return a.slot < b.slot; } );
	std::sort( outSpec.staticSamplers.begin(), outSpec.staticSamplers.end(), []( const StaticSamplerBinding &a, const StaticSamplerBinding &b ) { return a.slot < b.slot; } );
}

} // namespace graphics::material_system
