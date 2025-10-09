#pragma once

#include <d3d12.h>
#include <filesystem>
#include <string>
#include <vector>
#include <wrl.h>

namespace shader_manager
{

// Shader compilation result
struct ShaderBlob
{
	Microsoft::WRL::ComPtr<ID3DBlob> blob;
	std::string entryPoint;
	std::string profile;
	std::vector<std::filesystem::path> includedFiles; // Track all included files for dependency checking

	bool isValid() const noexcept { return blob != nullptr; }
};

// Basic shader compiler
class ShaderCompiler
{
public:
	static ShaderBlob CompileFromFile(
		const std::filesystem::path &filePath,
		const std::string &entryPoint,
		const std::string &profile,
		const std::vector<std::string> &defines = {} );

private:
	static std::string BuildDefineString( const std::vector<std::string> &defines );
};

} // namespace shader_manager
