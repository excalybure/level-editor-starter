#pragma once

#include <d3d12.h>
#include <dxcapi.h>
#include <filesystem>
#include <string>
#include <vector>
#include <wrl.h>

namespace shader_manager
{

// Shader compilation result (API unchanged for compatibility)
struct ShaderBlob
{
	Microsoft::WRL::ComPtr<ID3DBlob> blob;
	std::string entryPoint;
	std::string profile;
	std::vector<std::filesystem::path> includedFiles; // Track all included files for dependency checking

	bool isValid() const noexcept { return blob != nullptr; }
};

// DXC-based shader compiler with Shader Model 6.x support
class ShaderCompiler
{
public:
	// Initialize DXC utils and compiler (call once at application startup)
	static void InitializeDxc();

	// Public API unchanged - compiles shader using DXC (IDxcCompiler3)
	static ShaderBlob CompileFromFile(
		const std::filesystem::path &filePath,
		const std::string &entryPoint,
		const std::string &profile,
		const std::vector<std::string> &defines = {} );

	// Get DXC compiler version for diagnostics
	static std::string GetCompilerVersion();

	// Get DXC utils instance for shader reflection and other operations
	// (Internal use - call InitializeDxc first)
	static IDxcUtils *GetDxcUtils();

private:
	// Build DXC compilation arguments
	static std::vector<std::wstring> BuildCompilationArguments(
		const std::string &entryPoint,
		const std::string &profile,
		const std::vector<std::string> &defines );

	// DXC instances (initialized on first use)
	static Microsoft::WRL::ComPtr<IDxcUtils> s_dxcUtils;
	static Microsoft::WRL::ComPtr<IDxcCompiler3> s_dxcCompiler;
};

} // namespace shader_manager
