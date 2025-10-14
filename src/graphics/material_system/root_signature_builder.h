#pragma once

#include "graphics/material_system/parser.h"
#include "graphics/material_system/shader_reflection.h"
#include <string>
#include <vector>

namespace shader_manager
{
class ShaderManager;
}

namespace graphics::material_system
{

// Root signature specification
struct RootSignatureSpec
{
	std::vector<ResourceBinding> resourceBindings;
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

	// Legacy Build() for backward compatibility - DEPRECATED
	// Will be removed after all call sites are migrated to reflection-based Build()
	static RootSignatureSpec Build( const MaterialDefinition &material, bool includeFrameConstants = false, bool includeObjectConstants = false, bool includeMaterialConstants = false );

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

	// Merge bindings from multiple shaders, removing duplicates
	// Validates that duplicate names have matching type and slot
	// @param bindings - Vector of bindings to merge (may contain duplicates)
	// @return Merged bindings with duplicates removed
	static std::vector<ResourceBinding> MergeAndValidateBindings(
		const std::vector<ResourceBinding> &bindings );
};

} // namespace graphics::material_system
