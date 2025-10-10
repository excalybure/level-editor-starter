#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include "graphics/shader_manager/shader_compiler.h"

namespace graphics::material_system
{

// Integrates material system defines with shader compilation
class MaterialShaderCompiler
{
public:
	// Compile shader with material defines (unordered_map) converted to shader compiler format
	static shader_manager::ShaderBlob CompileWithDefines(
		const std::filesystem::path &shaderPath,
		const std::string &entryPoint,
		const std::string &profile,
		const std::unordered_map<std::string, std::string> &defines );

private:
	// Convert material defines map to shader compiler vector format
	static std::vector<std::string> ConvertDefines(
		const std::unordered_map<std::string, std::string> &defines );
};

} // namespace graphics::material_system
