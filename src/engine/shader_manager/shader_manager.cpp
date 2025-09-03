// Shader Manager implementation
// Copyright (c) 2025 Level Editor Project

module; // global module fragment

#include <windows.h>
#include <shared_mutex>

module engine.shader_manager;

import std;
import engine.renderer;
import runtime.console;

namespace shader_manager
{

ShaderHandle ShaderManager::registerShader(
	const std::filesystem::path &filePath,
	const std::string &entryPoint,
	const std::string &target,
	ShaderType type )
{
	// Check if this exact shader is already registered (uses read lock)
	{
		std::shared_lock<std::shared_mutex> lock( m_shaderMutex );
		ShaderHandle existingHandle = findExistingShader( filePath, entryPoint, target, type );
		if ( existingHandle != INVALID_SHADER_HANDLE )
		{
			return existingHandle;
		}
	}

	// Acquire write lock for registration
	std::unique_lock<std::shared_mutex> lock( m_shaderMutex );

	// Double-check after acquiring write lock (another thread might have registered it)
	ShaderHandle existingHandle = findExistingShader( filePath, entryPoint, target, type );
	if ( existingHandle != INVALID_SHADER_HANDLE )
	{
		return existingHandle;
	}

	ShaderHandle handle = m_nextHandle++;

	ShaderInfo shaderInfo;
	shaderInfo.handle = handle;
	shaderInfo.filePath = filePath;
	shaderInfo.entryPoint = entryPoint;
	shaderInfo.target = target;
	shaderInfo.type = type;
	shaderInfo.lastModified = getFileModificationTime( filePath );
	shaderInfo.isValid = false;

	// Try to compile the shader initially
	if ( compileShader( shaderInfo ) )
	{
		console::info( "Shader Manager: Successfully compiled shader {} ({})", filePath.string(), shaderTypeToString( type ) );
	}
	else
	{
		console::error( "Shader Manager: Failed to compile shader {} ({})", filePath.string(), shaderTypeToString( type ) );
	}

	// Add to hash map for fast lookup
	size_t shaderHash = computeShaderHash( filePath, entryPoint, target, type );
	m_shaderHashMap[shaderHash] = handle;

	m_shaders[handle] = std::move( shaderInfo );
	return handle;
}

void ShaderManager::unregisterShader( ShaderHandle handle )
{
	std::unique_lock<std::shared_mutex> lock( m_shaderMutex );
	auto it = m_shaders.find( handle );
	if ( it != m_shaders.end() )
	{
		console::info( "Shader Manager: Unregistering shader {}", it->second.filePath.string() );

		// Remove from hash map
		size_t shaderHash = computeShaderHash( it->second.filePath, it->second.entryPoint, it->second.target, it->second.type );
		m_shaderHashMap.erase( shaderHash );

		// Remove from main shader map
		m_shaders.erase( it );
	}
}

CallbackHandle ShaderManager::registerReloadCallback( ShaderReloadCallback callback )
{
	std::unique_lock<std::shared_mutex> lock( m_callbackMutex );
	CallbackHandle handle = m_nextCallbackHandle++;
	m_reloadCallbacks[handle] = std::move( callback );
	return handle;
}

void ShaderManager::unregisterReloadCallback( CallbackHandle callbackHandle )
{
	std::unique_lock<std::shared_mutex> lock( m_callbackMutex );
	auto it = m_reloadCallbacks.find( callbackHandle );
	if ( it != m_reloadCallbacks.end() )
	{
		m_reloadCallbacks.erase( it );
	}
}

const renderer::ShaderBlob *ShaderManager::getShaderBlob( ShaderHandle handle ) const
{
	std::shared_lock<std::shared_mutex> lock( m_shaderMutex );
	auto it = m_shaders.find( handle );
	if ( it != m_shaders.end() && it->second.isValid )
	{
		return &it->second.compiledBlob;
	}
	return nullptr;
}

void ShaderManager::update()
{
	std::unique_lock<std::shared_mutex> lock( m_shaderMutex );
	for ( auto &[handle, shaderInfo] : m_shaders )
	{
		bool needsRecompile = false;
		std::string changeReason;

		// Check if main shader file has been modified
		const auto currentModTime = getFileModificationTime( shaderInfo.filePath );
		if ( currentModTime != shaderInfo.lastModified )
		{
			needsRecompile = true;
			changeReason = "main shader file modified";
		}

		// Check if any included files have been modified
		if ( !needsRecompile )
		{
			for ( size_t i = 0; i < shaderInfo.includedFiles.size(); ++i )
			{
				const auto includedModTime = getFileModificationTime( shaderInfo.includedFiles[i] );
				if ( i < shaderInfo.includedFilesModTimes.size() &&
					includedModTime != shaderInfo.includedFilesModTimes[i] )
				{
					needsRecompile = true;
					changeReason = "included file modified: " + shaderInfo.includedFiles[i].string();
					break;
				}
			}
		}

		if ( needsRecompile )
		{
			console::info( "Shader Manager: Detected change in {} ({} - {}) ({}), recompiling...",
				shaderInfo.filePath.string(),
				shaderTypeToString( shaderInfo.type ),
				shaderInfo.entryPoint,
				changeReason );

			shaderInfo.lastModified = currentModTime;

			if ( compileShader( shaderInfo ) )
			{
				console::info( "Shader Manager: Successfully recompiled {} ({} - {})",
					shaderInfo.filePath.string(),
					shaderTypeToString( shaderInfo.type ),
					shaderInfo.entryPoint );

				// Notify all registered callbacks (copy callbacks to avoid deadlock)
				std::unordered_map<CallbackHandle, ShaderReloadCallback> callbacksCopy;
				{
					std::shared_lock<std::shared_mutex> lock( m_callbackMutex );
					callbacksCopy = m_reloadCallbacks;
				}

				for ( const auto &[callbackHandle, callback] : callbacksCopy )
				{
					if ( callback )
					{
						callback( handle, shaderInfo.compiledBlob );
					}
				}
			}
			else
			{
				console::error( "Shader Manager: Failed to recompile {} ({} - {})",
					shaderInfo.filePath.string(),
					shaderTypeToString( shaderInfo.type ),
					shaderInfo.entryPoint );
			}
		}
	}
}

bool ShaderManager::forceRecompile( ShaderHandle handle )
{
	std::unique_lock<std::shared_mutex> lock( m_shaderMutex );
	auto it = m_shaders.find( handle );
	if ( it != m_shaders.end() )
	{
		console::info( "Shader Manager: Force recompiling {} ({} - {})",
			it->second.filePath.string(),
			shaderTypeToString( it->second.type ),
			it->second.entryPoint );

		if ( compileShader( it->second ) )
		{
			console::info( "Shader Manager: Successfully recompiled {} ({} - {})",
				it->second.filePath.string(),
				shaderTypeToString( it->second.type ),
				it->second.entryPoint );

			// Notify all registered callbacks (copy callbacks to avoid deadlock)
			std::unordered_map<CallbackHandle, ShaderReloadCallback> callbacksCopy;
			{
				std::shared_lock<std::shared_mutex> lock( m_callbackMutex );
				callbacksCopy = m_reloadCallbacks;
			}

			for ( const auto &[callbackHandle, callback] : callbacksCopy )
			{
				if ( callback )
				{
					callback( handle, it->second.compiledBlob );
				}
			}
			return true;
		}
		else
		{
			console::error( "Shader Manager: Failed to recompile {} ({} - {})",
				it->second.filePath.string(),
				shaderTypeToString( it->second.type ),
				it->second.entryPoint );
			return false;
		}
	}
	return false;
}

void ShaderManager::forceRecompileAll()
{
	// Get shader count and handles under read lock
	std::vector<ShaderHandle> handles;
	{
		std::shared_lock<std::shared_mutex> lock( m_shaderMutex );
		console::info( "Shader Manager: Force recompiling all {} shaders", m_shaders.size() );
		handles.reserve( m_shaders.size() );
		for ( const auto &[handle, shaderInfo] : m_shaders )
		{
			handles.push_back( handle );
		}
	}

	// Recompile each shader (forceRecompile will acquire its own lock)
	for ( ShaderHandle handle : handles )
	{
		forceRecompile( handle );
	}
}

const ShaderInfo *ShaderManager::getShaderInfo( ShaderHandle handle ) const
{
	std::shared_lock<std::shared_mutex> lock( m_shaderMutex );
	auto it = m_shaders.find( handle );
	if ( it != m_shaders.end() )
	{
		return &it->second;
	}
	return nullptr;
}

std::vector<ShaderHandle> ShaderManager::getAllShaderHandles() const
{
	std::shared_lock<std::shared_mutex> lock( m_shaderMutex );
	std::vector<ShaderHandle> handles;
	handles.reserve( m_shaders.size() );

	for ( const auto &[handle, shaderInfo] : m_shaders )
	{
		handles.push_back( handle );
	}

	return handles;
}

bool ShaderManager::compileShader( ShaderInfo &shaderInfo )
{
	try
	{
		// Use the existing shader compiler
		shaderInfo.compiledBlob = renderer::ShaderCompiler::CompileFromFile(
			shaderInfo.filePath,
			shaderInfo.entryPoint,
			shaderInfo.target );

		shaderInfo.isValid = shaderInfo.compiledBlob.isValid();

		if ( shaderInfo.isValid )
		{
			// Extract included files from the compiled blob
			shaderInfo.includedFiles = shaderInfo.compiledBlob.includedFiles;

			// Get modification times for all included files
			shaderInfo.includedFilesModTimes.clear();
			shaderInfo.includedFilesModTimes.reserve( shaderInfo.includedFiles.size() );

			for ( const auto &includedFile : shaderInfo.includedFiles )
			{
				const auto modTime = getFileModificationTime( includedFile );
				shaderInfo.includedFilesModTimes.push_back( modTime );
			}
		}

		return shaderInfo.isValid;
	}
	catch ( const std::exception &e )
	{
		console::error( "Shader Manager: Exception during compilation of {}: {}",
			shaderInfo.filePath.string(),
			e.what() );
		shaderInfo.isValid = false;
		return false;
	}
}

std::filesystem::file_time_type ShaderManager::getFileModificationTime( const std::filesystem::path &path )
{
	std::error_code ec;
	auto modTime = std::filesystem::last_write_time( path, ec );

	if ( ec )
	{
		console::error( "Shader Manager: Failed to get modification time for {}: {}",
			path.string(),
			ec.message() );
		return {};
	}

	return modTime;
}

std::string ShaderManager::shaderTypeToString( ShaderType type ) const
{
	switch ( type )
	{
	case ShaderType::Vertex:
		return "Vertex";
	case ShaderType::Pixel:
		return "Pixel";
	case ShaderType::Compute:
		return "Compute";
	case ShaderType::Geometry:
		return "Geometry";
	case ShaderType::Hull:
		return "Hull";
	case ShaderType::Domain:
		return "Domain";
	default:
		return "Unknown";
	}
}

ShaderHandle ShaderManager::findExistingShader( const std::filesystem::path &filePath,
	const std::string &entryPoint,
	const std::string &target,
	ShaderType type ) const
{
	// Compute hash for fast lookup
	size_t shaderHash = computeShaderHash( filePath, entryPoint, target, type );

	auto it = m_shaderHashMap.find( shaderHash );
	if ( it != m_shaderHashMap.end() )
	{
		// Found a shader with matching hash - verify it's actually the same shader
		// (hash collisions are possible, so we need to double-check)
		ShaderHandle handle = it->second;
		auto shaderIt = m_shaders.find( handle );
		if ( shaderIt != m_shaders.end() )
		{
			const ShaderInfo &shaderInfo = shaderIt->second;
			if ( shaderInfo.filePath == filePath &&
				shaderInfo.entryPoint == entryPoint &&
				shaderInfo.target == target &&
				shaderInfo.type == type )
			{
				return handle;
			}
		}
	}

	return INVALID_SHADER_HANDLE;
}

size_t ShaderManager::computeShaderHash( const std::filesystem::path &filePath,
	const std::string &entryPoint,
	const std::string &target,
	ShaderType type ) const
{
	// Combine hashes of all shader parameters
	std::hash<std::string> stringHasher;
	std::hash<int> intHasher;

	size_t hash = 0;

	// Hash the file path (as string for consistency)
	hash ^= stringHasher( filePath.string() ) + 0x9e3779b9 + ( hash << 6 ) + ( hash >> 2 );

	// Hash the entry point
	hash ^= stringHasher( entryPoint ) + 0x9e3779b9 + ( hash << 6 ) + ( hash >> 2 );

	// Hash the target
	hash ^= stringHasher( target ) + 0x9e3779b9 + ( hash << 6 ) + ( hash >> 2 );

	// Hash the shader type
	hash ^= intHasher( static_cast<int>( type ) ) + 0x9e3779b9 + ( hash << 6 ) + ( hash >> 2 );

	return hash;
}

} // namespace shader_manager
