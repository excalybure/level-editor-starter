#include <catch2/catch_test_macros.hpp>
#include "editor/config/EditorConfig.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

// Helper to create a temporary test config file
static void createTestConfigFile( const std::string &path, const std::string &content )
{
	std::ofstream file( path );
	file << content;
	file.close();
}

// Helper to delete test config file
static void deleteTestConfigFile( const std::string &path )
{
	if ( fs::exists( path ) )
	{
		fs::remove( path );
	}
}

TEST_CASE( "EditorConfig can be constructed", "[editor-config][unit]" )
{
	SECTION( "Default constructor uses editor_config.json" )
	{
		editor::EditorConfig config;
		REQUIRE( config.getFilePath() == "editor_config.json" );
	}

	SECTION( "Constructor with custom path" )
	{
		editor::EditorConfig config( "custom_config.json" );
		REQUIRE( config.getFilePath() == "custom_config.json" );
	}
}

TEST_CASE( "EditorConfig load handles missing file gracefully", "[editor-config][unit]" )
{
	const std::string testPath = "test_missing_config.json";
	deleteTestConfigFile( testPath );

	editor::EditorConfig config( testPath );
	const bool loaded = config.load();

	// Should return false but not crash
	REQUIRE_FALSE( loaded );
}

TEST_CASE( "EditorConfig load reads valid JSON file", "[editor-config][unit]" )
{
	const std::string testPath = "test_valid_config.json";
	const std::string jsonContent = R"({
		"version": 1,
		"test": "value"
	})";

	createTestConfigFile( testPath, jsonContent );

	editor::EditorConfig config( testPath );
	const bool loaded = config.load();

	REQUIRE( loaded );

	deleteTestConfigFile( testPath );
}

TEST_CASE( "EditorConfig load handles malformed JSON", "[editor-config][unit]" )
{
	const std::string testPath = "test_malformed_config.json";
	const std::string jsonContent = "{ this is not valid json }";

	createTestConfigFile( testPath, jsonContent );

	editor::EditorConfig config( testPath );
	const bool loaded = config.load();

	// Should return false and log error, but not crash
	REQUIRE_FALSE( loaded );

	deleteTestConfigFile( testPath );
}
