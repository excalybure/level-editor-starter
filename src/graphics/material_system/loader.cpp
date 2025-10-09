#include "graphics/material_system/loader.h"
#include "core/console.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace material_system
{

bool JsonLoader::load( const std::string &rootPath )
{
	m_mergedDocument = nlohmann::json::object();
	m_errors.clear();
	m_loadedFiles.clear();

	std::vector<std::filesystem::path> includeStack;
	const std::filesystem::path rootFilePath( rootPath );

	if ( !loadRecursive( rootFilePath, includeStack ) )
	{
		console::error( "Failed to load material system JSON from: " + rootPath );
		for ( const auto &error : m_errors )
		{
			console::error( "  " + error );
		}
		return false;
	}

	return true;
}

bool JsonLoader::loadRecursive( const std::filesystem::path &filePath,
	std::vector<std::filesystem::path> &includeStack )
{
	// Normalize path for comparison
	const auto canonicalPath = std::filesystem::weakly_canonical( filePath );
	const std::string canonicalStr = canonicalPath.string();

	// Check for cycle: is this file already in the include stack?
	for ( const auto &stackPath : includeStack )
	{
		const auto stackCanonical = std::filesystem::weakly_canonical( stackPath );
		if ( stackCanonical == canonicalPath )
		{
			// Cycle detected - build error message with chain
			std::ostringstream chain;
			chain << "Circular include detected: ";
			for ( size_t i = 0; i < includeStack.size(); ++i )
			{
				chain << includeStack[i].filename().string();
				if ( i < includeStack.size() - 1 )
				{
					chain << " -> ";
				}
			}
			chain << " -> " << filePath.filename().string();

			m_errors.push_back( chain.str() );
			return false;
		}
	}

	// Check if file exists
	if ( !std::filesystem::exists( canonicalPath ) )
	{
		m_errors.push_back( "File not found: " + canonicalStr );
		return false;
	}

	// Read file
	std::ifstream file( canonicalPath );
	if ( !file.is_open() )
	{
		m_errors.push_back( "Failed to open file: " + canonicalStr );
		return false;
	}

	// Parse JSON
	nlohmann::json doc;
	try
	{
		file >> doc;
	}
	catch ( const nlohmann::json::exception &e )
	{
		m_errors.push_back( "JSON parse error in " + canonicalStr + ": " + e.what() );
		return false;
	}

	// Push current file onto stack
	includeStack.push_back( canonicalPath );

	// Process includes if present
	if ( doc.contains( "includes" ) && doc["includes"].is_array() )
	{
		const auto &includes = doc["includes"];
		const auto parentDir = canonicalPath.parent_path();

		for ( const auto &include : includes )
		{
			if ( !include.is_string() )
			{
				m_errors.push_back( "Include entry is not a string in " + canonicalStr );
				includeStack.pop_back();
				return false;
			}

			const std::filesystem::path includePath = parentDir / include.get<std::string>();

			if ( !loadRecursive( includePath, includeStack ) )
			{
				includeStack.pop_back();
				return false;
			}
		}
	}

	// Pop from stack after processing includes
	includeStack.pop_back();

	// Merge this document into the result
	// Mark as loaded to avoid duplicate processing
	if ( m_loadedFiles.find( canonicalStr ) == m_loadedFiles.end() )
	{
		mergeDocument( doc );
		m_loadedFiles.insert( canonicalStr );
	}

	return true;
}

void JsonLoader::mergeDocument( const nlohmann::json &source )
{
	// Merge strategy: combine arrays, shallow merge objects
	// For initial implementation, simple merge by key

	for ( auto it = source.begin(); it != source.end(); ++it )
	{
		const auto &key = it.key();

		// Skip "includes" as it's processed separately
		if ( key == "includes" )
		{
			continue;
		}

		if ( !m_mergedDocument.contains( key ) )
		{
			// Key doesn't exist yet, copy it
			m_mergedDocument[key] = it.value();
		}
		else
		{
			// Key exists - merge depending on type
			if ( it.value().is_array() && m_mergedDocument[key].is_array() )
			{
				// Append array elements
				for ( const auto &elem : it.value() )
				{
					m_mergedDocument[key].push_back( elem );
				}
			}
			else if ( it.value().is_object() && m_mergedDocument[key].is_object() )
			{
				// Shallow merge object keys
				for ( auto objIt = it.value().begin(); objIt != it.value().end(); ++objIt )
				{
					m_mergedDocument[key][objIt.key()] = objIt.value();
				}
			}
			else
			{
				// Type mismatch or scalar - replace (last wins)
				m_mergedDocument[key] = it.value();
			}
		}
	}
}

} // namespace material_system
