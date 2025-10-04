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
	: m_filePath( "editor_config.json" )
{
}

EditorConfig::EditorConfig( const std::string &filePath )
	: m_filePath( filePath )
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

		const json data = json::parse( file );
		file.close();

		// Store the parsed JSON (we'll add member variable in next step)
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

} // namespace editor
