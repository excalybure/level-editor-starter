#include "shader_compiler.h"

#include <d3d12.h>
#include <dxcapi.h>
#include <wrl.h>
#include <fstream>
#include <iterator>

#include "core/console.h"
#include "core/strings.h"

namespace shader_manager
{

// Static members for DXC singleton
Microsoft::WRL::ComPtr<IDxcUtils> ShaderCompiler::s_dxcUtils;
Microsoft::WRL::ComPtr<IDxcCompiler3> ShaderCompiler::s_dxcCompiler;

// DXC include handler (replaces ID3DInclude)
class DxcIncludeHandler : public IDxcIncludeHandler
{
private:
	std::filesystem::path m_shaderDirectory;
	std::vector<std::filesystem::path> m_includedFiles;
	Microsoft::WRL::ComPtr<IDxcUtils> m_utils;
	ULONG m_refCount = 1;

public:
	DxcIncludeHandler( const std::filesystem::path &dir, IDxcUtils *utils )
		: m_shaderDirectory( dir ), m_utils( utils )
	{
	}

	const std::vector<std::filesystem::path> &getIncludedFiles() const noexcept { return m_includedFiles; }

	HRESULT STDMETHODCALLTYPE LoadSource( LPCWSTR pFilename, IDxcBlob **ppIncludeSource ) override
	{
		if ( !pFilename || !ppIncludeSource )
		{
			return E_INVALIDARG;
		}

		// Convert wide string to narrow for filesystem
		const std::wstring wFilename( pFilename );
		const std::filesystem::path includePath = m_shaderDirectory / wFilename;

		if ( !std::filesystem::exists( includePath ) )
		{
			return E_FAIL;
		}

		// Track this included file
		m_includedFiles.push_back( std::filesystem::canonical( includePath ) );

		// Load file content
		std::ifstream file( includePath, std::ios::binary | std::ios::ate );
		if ( !file.is_open() )
		{
			return E_FAIL;
		}

		const auto size = file.tellg();
		file.seekg( 0, std::ios::beg );

		std::vector<char> content( static_cast<size_t>( size ) );
		if ( !file.read( content.data(), size ) )
		{
			return E_FAIL;
		}

		// Create DXC blob from content
		Microsoft::WRL::ComPtr<IDxcBlobEncoding> sourceBlob;
		const HRESULT hr = m_utils->CreateBlob( content.data(), static_cast<UINT32>( content.size() ), CP_UTF8, &sourceBlob );
		if ( FAILED( hr ) )
		{
			return hr;
		}

		*ppIncludeSource = sourceBlob.Detach();
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void **ppvObject ) override
	{
		if ( !ppvObject )
		{
			return E_POINTER;
		}

		if ( riid == __uuidof( IUnknown ) || riid == __uuidof( IDxcIncludeHandler ) )
		{
			*ppvObject = static_cast<IDxcIncludeHandler *>( this );
			AddRef();
			return S_OK;
		}

		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef() override { return ++m_refCount; }

	ULONG STDMETHODCALLTYPE Release() override
	{
		const ULONG count = --m_refCount;
		if ( count == 0 )
		{
			delete this;
		}
		return count;
	}
};

// Initialize DXC compiler (call once at application startup)
void ShaderCompiler::InitializeDxc()
{
	HRESULT hr = DxcCreateInstance( CLSID_DxcUtils, IID_PPV_ARGS( &s_dxcUtils ) );
	if ( FAILED( hr ) )
	{
		console::errorAndThrow( "Failed to create DXC utils instance (HRESULT: 0x{:08X}). Ensure dxcompiler.dll is available.", static_cast<unsigned int>( hr ) );
	}

	hr = DxcCreateInstance( CLSID_DxcCompiler, IID_PPV_ARGS( &s_dxcCompiler ) );
	if ( FAILED( hr ) )
	{
		console::errorAndThrow( "Failed to create DXC compiler instance (HRESULT: 0x{:08X}). Ensure dxcompiler.dll is available.", static_cast<unsigned int>( hr ) );
	}
}

// Build DXC compilation arguments
std::vector<std::wstring> ShaderCompiler::BuildCompilationArguments(
	const std::string &entryPoint,
	const std::string &profile,
	const std::vector<std::string> &defines )
{
	std::vector<std::wstring> args;

	// Entry point
	args.push_back( L"-E" );
	args.push_back( strings::toWideString( entryPoint ) );

	// Target profile
	args.push_back( L"-T" );
	args.push_back( strings::toWideString( profile ) );

	// HLSL version 2021 (for modern features)
	args.push_back( L"-HV" );
	args.push_back( L"2021" );

	// Enable 16-bit types only for SM 6.2+ (requires HLSL 2018+)
	// Parse shader model version from profile (e.g., "vs_6_6" -> 6.6)
	const size_t underscorePos = profile.find( '_' );
	if ( underscorePos != std::string::npos && underscorePos + 3 < profile.length() )
	{
		const int majorVersion = profile[underscorePos + 1] - '0';
		const int minorVersion = profile[underscorePos + 3] - '0';
		const int shaderModel = majorVersion * 10 + minorVersion;

		if ( shaderModel >= 62 ) // SM 6.2 or higher
		{
			args.push_back( L"-enable-16bit-types" );
		}
	}

#ifdef _DEBUG
	args.push_back( L"-Zi" );			// Debug info
	args.push_back( L"-Od" );			// Disable optimizations
	args.push_back( L"-Qembed_debug" ); // Embed debug info in shader
#else
	args.push_back( L"-O3" ); // Maximum optimization
#endif

	// Add defines
	for ( const auto &define : defines )
	{
		args.push_back( L"-D" );
		args.push_back( strings::toWideString( define ) );
	}

	return args;
}

// Get compiler version
std::string ShaderCompiler::GetCompilerVersion()
{
	Microsoft::WRL::ComPtr<IDxcVersionInfo> versionInfo;
	if ( SUCCEEDED( s_dxcCompiler.As( &versionInfo ) ) )
	{
		UINT32 major = 0, minor = 0;
		if ( SUCCEEDED( versionInfo->GetVersion( &major, &minor ) ) )
		{
			return std::format( "DXC {}.{}", major, minor );
		}
	}

	return "DXC (version unknown)";
}

// Main compilation function
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

	// Load source file
	std::ifstream file( filePath, std::ios::binary | std::ios::ate );
	if ( !file.is_open() )
	{
		console::errorAndThrow( "Failed to open shader file: {}", filePath.string() );
	}

	const auto fileSize = file.tellg();
	file.seekg( 0, std::ios::beg );

	std::vector<char> source( static_cast<size_t>( fileSize ) );
	if ( !file.read( source.data(), fileSize ) )
	{
		console::errorAndThrow( "Failed to read shader file: {}", filePath.string() );
	}

	// Create source blob
	Microsoft::WRL::ComPtr<IDxcBlobEncoding> sourceBlob;
	HRESULT hr = s_dxcUtils->CreateBlob( source.data(), static_cast<UINT32>( source.size() ), CP_UTF8, &sourceBlob );
	if ( FAILED( hr ) )
	{
		console::errorAndThrow( "Failed to create source blob for: {}", filePath.string() );
	}

	// Build compilation arguments
	const std::vector<std::wstring> argStrings = BuildCompilationArguments( entryPoint, profile, defines );
	std::vector<LPCWSTR> args;
	args.reserve( argStrings.size() );
	for ( const auto &arg : argStrings )
	{
		args.push_back( arg.c_str() );
	}

	// Create include handler
	const std::filesystem::path shaderDirectory = filePath.parent_path();
	DxcIncludeHandler *includeHandler = new DxcIncludeHandler( shaderDirectory, s_dxcUtils.Get() );

	// Prepare source buffer
	DxcBuffer sourceBuffer{};
	sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
	sourceBuffer.Size = sourceBlob->GetBufferSize();
	sourceBuffer.Encoding = CP_UTF8;

	// Compile shader
	Microsoft::WRL::ComPtr<IDxcResult> result;
	const std::wstring wFilePath = strings::toWideString( filePath.string() );
	hr = s_dxcCompiler->Compile(
		&sourceBuffer,
		args.data(),
		static_cast<UINT32>( args.size() ),
		includeHandler,
		IID_PPV_ARGS( &result ) );

	// Check compilation status
	if ( SUCCEEDED( hr ) )
	{
		result->GetStatus( &hr );
	}

	// Handle errors
	if ( FAILED( hr ) )
	{
		Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors;
		if ( SUCCEEDED( result->GetOutput( DXC_OUT_ERRORS, IID_PPV_ARGS( &errors ), nullptr ) ) && errors && errors->GetStringLength() > 0 )
		{
			const std::string errorMsg( errors->GetStringPointer(), errors->GetStringLength() );
			includeHandler->Release();
			console::errorAndThrow( "Shader compilation failed:\n{}", errorMsg );
		}
		else
		{
			includeHandler->Release();
			console::errorAndThrow( "Shader compilation failed with unknown error (HRESULT: 0x{:08X})", static_cast<unsigned int>( hr ) );
		}
	}

	// Get compiled shader object
	Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob;
	hr = result->GetOutput( DXC_OUT_OBJECT, IID_PPV_ARGS( &shaderBlob ), nullptr );
	if ( FAILED( hr ) || !shaderBlob )
	{
		includeHandler->Release();
		console::errorAndThrow( "Failed to retrieve compiled shader object" );
	}

	// Convert IDxcBlob to ID3DBlob for compatibility
	ShaderBlob resultBlob;
	resultBlob.entryPoint = entryPoint;
	resultBlob.profile = profile;
	resultBlob.includedFiles = includeHandler->getIncludedFiles();

	hr = shaderBlob.As( &resultBlob.blob );
	if ( FAILED( hr ) )
	{
		includeHandler->Release();
		console::errorAndThrow( "Failed to convert IDxcBlob to ID3DBlob" );
	}

	includeHandler->Release();
	return resultBlob;
}

} // namespace shader_manager
