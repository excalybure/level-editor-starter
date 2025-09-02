// Shader Manager implementation
// Copyright (c) 2025 Level Editor Project

module; // global module fragment

#include <windows.h>

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

	m_shaders[handle] = std::move( shaderInfo );
	return handle;
}

void ShaderManager::unregisterShader( ShaderHandle handle )
{
	auto it = m_shaders.find( handle );
	if ( it != m_shaders.end() )
	{
		console::info( "Shader Manager: Unregistering shader {}", it->second.filePath.string() );
		m_shaders.erase( it );
	}
}

void ShaderManager::setReloadCallback( ShaderReloadCallback callback )
{
	m_reloadCallback = std::move( callback );
}

const renderer::ShaderBlob *ShaderManager::getShaderBlob( ShaderHandle handle ) const
{
	auto it = m_shaders.find( handle );
	if ( it != m_shaders.end() && it->second.isValid )
	{
		return &it->second.compiledBlob;
	}
	return nullptr;
}

void ShaderManager::update()
{
	for ( auto &[handle, shaderInfo] : m_shaders )
	{
		// Check if file has been modified
		auto currentModTime = getFileModificationTime( shaderInfo.filePath );

		if ( currentModTime != shaderInfo.lastModified )
		{
			console::info( "Shader Manager: Detected change in {}, recompiling...", shaderInfo.filePath.string() );

			shaderInfo.lastModified = currentModTime;

			if ( compileShader( shaderInfo ) )
			{
				console::info( "Shader Manager: Successfully recompiled {}", shaderInfo.filePath.string() );

				// Notify callback if set
				if ( m_reloadCallback )
				{
					m_reloadCallback( handle, shaderInfo.compiledBlob );
				}
			}
			else
			{
				console::error( "Shader Manager: Failed to recompile {}", shaderInfo.filePath.string() );
			}
		}
	}
}

bool ShaderManager::forceRecompile( ShaderHandle handle )
{
	auto it = m_shaders.find( handle );
	if ( it != m_shaders.end() )
	{
		console::info( "Shader Manager: Force recompiling {}", it->second.filePath.string() );

		if ( compileShader( it->second ) )
		{
			console::info( "Shader Manager: Successfully recompiled {}", it->second.filePath.string() );

			// Notify callback if set
			if ( m_reloadCallback )
			{
				m_reloadCallback( handle, it->second.compiledBlob );
			}
			return true;
		}
		else
		{
			console::error( "Shader Manager: Failed to recompile {}", it->second.filePath.string() );
			return false;
		}
	}
	return false;
}

void ShaderManager::forceRecompileAll()
{
	console::info( "Shader Manager: Force recompiling all {} shaders", m_shaders.size() );

	for ( auto &[handle, shaderInfo] : m_shaders )
	{
		forceRecompile( handle );
	}
}

const ShaderInfo *ShaderManager::getShaderInfo( ShaderHandle handle ) const
{
	auto it = m_shaders.find( handle );
	if ( it != m_shaders.end() )
	{
		return &it->second;
	}
	return nullptr;
}

std::vector<ShaderHandle> ShaderManager::getAllShaderHandles() const
{
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

} // namespace shader_manager
