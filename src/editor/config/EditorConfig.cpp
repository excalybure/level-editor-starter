#include "editor/config/EditorConfig.h"
#include "runtime/console.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace editor
{

EditorConfig::EditorConfig()
	: m_filePath( "editor_config.json" ), m_data( std::make_unique<json>() )
{
}

EditorConfig::EditorConfig( const std::string &filePath )
	: m_filePath( filePath ), m_data( std::make_unique<json>() )
{
}

EditorConfig::~EditorConfig() = default;

const std::string &EditorConfig::getFilePath() const noexcept
{
	return m_filePath;
}

bool EditorConfig::load()
{
	// Check if file exists
	if ( !fs::exists( m_filePath ) )
	{
		console::info( "Config file not found: {}", m_filePath );
		return false;
	}

	// Try to open and parse the file
	try
	{
		std::ifstream file( m_filePath );
		if ( !file.is_open() )
		{
			console::error( "Failed to open config file: {}", m_filePath );
			return false;
		}

		*m_data = json::parse( file );
		file.close();

		console::info( "Loaded config from: {}", m_filePath );
		return true;
	}
	catch ( const json::parse_error &e )
	{
		console::error( "Failed to parse config file {}: {}", m_filePath, e.what() );
		return false;
	}
	catch ( const std::exception &e )
	{
		console::error( "Error loading config file {}: {}", m_filePath, e.what() );
		return false;
	}
}

bool EditorConfig::save()
{
	try
	{
		// Create parent directories if they don't exist
		const fs::path filePath( m_filePath );
		if ( filePath.has_parent_path() )
		{
			const fs::path parentPath = filePath.parent_path();
			if ( !fs::exists( parentPath ) )
			{
				fs::create_directories( parentPath );
			}
		}

		// Open file for writing
		std::ofstream file( m_filePath );
		if ( !file.is_open() )
		{
			console::error( "Failed to open config file for writing: {}", m_filePath );
			return false;
		}

		// Write JSON with pretty-print (indent = 2 spaces)
		file << m_data->dump( 2 );
		file.close();

		console::info( "Saved config to: {}", m_filePath );
		return true;
	}
	catch ( const std::exception &e )
	{
		console::error( "Error saving config file {}: {}", m_filePath, e.what() );
		return false;
	}
}

std::vector<std::string> EditorConfig::splitKey( const std::string &key ) const
{
	std::vector<std::string> segments;
	std::string current;

	for ( const char c : key )
	{
		if ( c == '.' )
		{
			if ( !current.empty() )
			{
				segments.push_back( current );
				current.clear();
			}
		}
		else
		{
			current += c;
		}
	}

	if ( !current.empty() )
	{
		segments.push_back( current );
	}

	return segments;
}

bool EditorConfig::getBool( const std::string &key, const bool defaultValue ) const
{
	const std::vector<std::string> segments = splitKey( key );
	if ( segments.empty() )
	{
		return defaultValue;
	}

	try
	{
		json *current = m_data.get();

		// Navigate through the path
		for ( size_t i = 0; i < segments.size(); ++i )
		{
			const std::string &segment = segments[i];

			if ( !current->contains( segment ) )
			{
				return defaultValue;
			}

			if ( i == segments.size() - 1 )
			{
				// Last segment - get the value
				const auto &value = ( *current )[segment];
				if ( value.is_boolean() )
				{
					return value.get<bool>();
				}
				return defaultValue;
			}

			// Not last segment - navigate deeper
			current = &( ( *current )[segment] );
		}

		return defaultValue;
	}
	catch ( const std::exception & )
	{
		return defaultValue;
	}
}

void EditorConfig::setBool( const std::string &key, const bool value )
{
	const std::vector<std::string> segments = splitKey( key );
	if ( segments.empty() )
	{
		return;
	}

	try
	{
		json *current = m_data.get();

		// Navigate/create the path
		for ( size_t i = 0; i < segments.size(); ++i )
		{
			const std::string &segment = segments[i];

			if ( i == segments.size() - 1 )
			{
				// Last segment - set the value
				( *current )[segment] = value;
				return;
			}

			// Not last segment - ensure it's an object and navigate
			if ( !current->contains( segment ) || !( *current )[segment].is_object() )
			{
				( *current )[segment] = json::object();
			}

			current = &( ( *current )[segment] );
		}
	}
	catch ( const std::exception &e )
	{
		console::error( "Error setting bool value for key {}: {}", key, e.what() );
	}
}

} // namespace editor
