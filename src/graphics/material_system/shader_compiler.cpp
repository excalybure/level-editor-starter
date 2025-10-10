#include "graphics/material_system/shader_compiler.h"
#include "core/console.h"
#include <algorithm>
#include <vector>

namespace graphics::material_system
{

shader_manager::ShaderBlob MaterialShaderCompiler::CompileWithDefines(
	const std::filesystem::path &shaderPath,
	const std::string &entryPoint,
	const std::string &profile,
	const std::unordered_map<std::string, std::string> &defines )
{
	// Convert material defines to shader compiler format
	const std::vector<std::string> compilerDefines = ConvertDefines( defines );

	// Invoke existing shader compiler
	return shader_manager::ShaderCompiler::CompileFromFile(
		shaderPath, entryPoint, profile, compilerDefines );
}

std::vector<std::string> MaterialShaderCompiler::ConvertDefines(
	const std::unordered_map<std::string, std::string> &defines )
{
	std::vector<std::string> result;
	result.reserve( defines.size() );

	for ( const auto &[key, value] : defines )
	{
		// Format: "KEY VALUE" for shader compiler
		result.push_back( key + " " + value );
	}

	// Sort for deterministic ordering (important for caching)
	std::sort( result.begin(), result.end() );

	return result;
}

} // namespace graphics::material_system
