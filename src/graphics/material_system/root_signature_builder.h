#pragma once

#include "graphics/material_system/parser.h"
#include "graphics/material_system/shader_reflection.h"
#include <string>
#include <vector>
#include <d3d12.h>

namespace shader_manager
{
class ShaderManager;
}

namespace graphics::material_system
{

// Static sampler definition for root signature
struct StaticSamplerBinding
{
	std::string name;
	int slot;
	D3D12_FILTER filter;
	D3D12_TEXTURE_ADDRESS_MODE addressMode;
};

// Root signature specification
struct RootSignatureSpec
{
	// CBVs use root descriptors (2 DWORDs per CBV)
	std::vector<ResourceBinding> cbvRootDescriptors;

	// SRVs, UAVs, and Samplers use descriptor tables (1 DWORD per table)
	// Will be organized into tables in a future phase
	std::vector<ResourceBinding> descriptorTableResources;

	// Static samplers that are defined in the root signature itself
	// (no descriptor heap binding needed)
	std::vector<StaticSamplerBinding> staticSamplers;
};

// Builds root signatures from material definitions
class RootSignatureBuilder
{
public:
	// Build root signature spec from material pass using shader reflection
	// Uses D3D12 shader reflection to automatically detect CBV/SRV/UAV/Sampler requirements
	// @param pass - Material pass containing shaders to reflect
	// @param shaderManager - ShaderManager to retrieve compiled shader blobs
	// @param reflectionCache - Cache for shader reflection results (avoids redundant D3DReflect calls)
	// @return RootSignatureSpec with bindings extracted from shaders
	static RootSignatureSpec Build(
		const MaterialPass &pass,
		shader_manager::ShaderManager *shaderManager,
		ShaderReflectionCache *reflectionCache );

	// Check if a sampler name is known to be a static sampler
	// Static samplers are defined in the root signature and don't need descriptor heap bindings
	// @param name - Sampler name (e.g., "linearSampler")
	// @return true if this sampler should be a static sampler
	static bool IsStaticSamplerName( const std::string &name );

	// Get D3D12 filter type for a known static sampler name
	// @param name - Sampler name
	// @return D3D12_FILTER type for this sampler
	static D3D12_FILTER GetFilterForSamplerName( const std::string &name );

private:
	// Merge bindings from multiple shaders, removing duplicates
	// Validates that duplicate names have matching type and slot
	// @param bindings - Vector of bindings to merge (may contain duplicates)
	// @return Merged bindings with duplicates removed
	static std::vector<ResourceBinding> MergeAndValidateBindings(
		const std::vector<ResourceBinding> &bindings );

	// Group bindings into CBVs (root descriptors) vs other resources (descriptor tables)
	// CBVs are placed in root signature as root descriptors (2 DWORDs each)
	// SRVs, UAVs, Samplers are placed in descriptor tables (1 DWORD per table)
	// Static samplers are identified and placed in root signature itself
	// @param merged - Merged bindings from all shaders
	// @param outSpec - RootSignatureSpec to populate with grouped bindings
	static void GroupBindingsForRootSignature(
		const std::vector<ResourceBinding> &merged,
		RootSignatureSpec &outSpec );
};

} // namespace graphics::material_system
