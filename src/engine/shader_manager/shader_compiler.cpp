#include "shader_compiler.h"

#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl.h>
#include <fstream>

#include "core/console.h"

namespace shader_manager
{

// Custom include handler for shader compilation
class ShaderIncludeHandler : public ID3DInclude
{
private:
	std::filesystem::path m_shaderDirectory;
	std::vector<std::filesystem::path> m_includedFiles; // Track all included files

public:
	ShaderIncludeHandler( const std::filesystem::path &shaderDir ) : m_shaderDirectory( shaderDir ) {}

	// Get the list of all included files
	const std::vector<std::filesystem::path> &getIncludedFiles() const noexcept { return m_includedFiles; }

	HRESULT __stdcall Open( D3D_INCLUDE_TYPE /*IncludeType*/, LPCSTR pFileName, LPCVOID /*pParentData*/, LPCVOID *ppData, UINT *pBytes ) override
	{
		const std::filesystem::path includePath = m_shaderDirectory / pFileName;

		if ( !std::filesystem::exists( includePath ) )
		{
			return E_FAIL;
		}

		// Track this included file
		m_includedFiles.push_back( std::filesystem::canonical( includePath ) );

		std::ifstream file( includePath, std::ios::binary | std::ios::ate );
		if ( !file.is_open() )
		{
			return E_FAIL;
		}

		file.seekg( 0, std::ios::end );
		auto size = static_cast<size_t>( file.tellg() );
		file.seekg( 0, std::ios::beg );

		char *buffer = new char[size + 1];
		if ( !file.read( buffer, static_cast<std::streamsize>( size ) ) )
		{
			delete[] buffer;
			return E_FAIL;
		}

		buffer[size] = '\0';
		*ppData = buffer;
		*pBytes = static_cast<UINT>( size );

		return S_OK;
	}

	HRESULT __stdcall Close( LPCVOID pData ) override
	{
		delete[] static_cast<const char *>( pData );
		return S_OK;
	}
};

// ShaderCompiler implementation
ShaderBlob ShaderCompiler::CompileFromSource(
	const std::string &source,
	const std::string &entryPoint,
	const std::string &profile,
	const std::vector<std::string> &defines,
	const std::filesystem::path *shaderDirectory )
{
	ShaderBlob result;
	result.entryPoint = entryPoint;
	result.profile = profile;

	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
	UINT compileFlags = 0;

#ifdef _DEBUG
	compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	// Prepend generated #defines to source if any
	std::string fullSource;
	if ( !defines.empty() )
	{
		fullSource = BuildDefineString( defines );
		fullSource += source;
	}
	const char *finalSource = defines.empty() ? source.c_str() : fullSource.c_str();
	const size_t finalSourceSize = defines.empty() ? source.length() : fullSource.length();

	// Create include handler for shader directory
	const std::filesystem::path includeDir = shaderDirectory ? *shaderDirectory : ( std::filesystem::current_path() / "shaders" );
	ShaderIncludeHandler includeHandler( includeDir );

	HRESULT hr = D3DCompile(
		finalSource,
		finalSourceSize,
		nullptr,
		nullptr,
		&includeHandler,
		entryPoint.c_str(),
		profile.c_str(),
		compileFlags,
		0,
		&result.blob,
		&errorBlob );

	if ( FAILED( hr ) )
	{
		if ( errorBlob )
		{
			const std::string error = static_cast<const char *>( errorBlob->GetBufferPointer() );
			console::errorAndThrow( "Shader compilation failed: {}", error );
		}
		else
		{
			console::errorAndThrow( "Shader compilation failed with unknown error" );
		}
	}

	// Copy the list of included files from the include handler
	result.includedFiles = includeHandler.getIncludedFiles();

	return result;
}

ShaderBlob ShaderCompiler::CompileFromFile(
	const std::filesystem::path &filePath,
	const std::string &entryPoint,
	const std::string &profile,
	const std::vector<std::string> &defines )
{
	if ( !std::filesystem::exists( filePath ) )
	{
		console::errorAndThrow( "Shader file not found: {}", filePath.string() );
	}

	std::ifstream file( filePath );
	if ( !file.is_open() )
	{
		console::errorAndThrow( "Failed to open shader file: {}", filePath.string() );
	}

	const std::string source( ( std::istreambuf_iterator<char>( file ) ), std::istreambuf_iterator<char>() );

	// Extract directory from file path for include resolution
	const std::filesystem::path shaderDirectory = filePath.parent_path();
	return CompileFromSource( source, entryPoint, profile, defines, &shaderDirectory );
}

std::string ShaderCompiler::BuildDefineString( const std::vector<std::string> &defines )
{
	std::string result;
	for ( const auto &define : defines )
	{
		result += "#define " + define + "\n";
	}
	return result;
}

} // namespace shader_manager
