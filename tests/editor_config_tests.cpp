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

TEST_CASE( "EditorConfig save writes JSON to disk", "[editor-config][unit]" )
{
	const std::string testPath = "test_save_config.json";
	deleteTestConfigFile( testPath );

	editor::EditorConfig config( testPath );

	// Save should succeed even with empty data
	const bool saved = config.save();
	REQUIRE( saved );

	// File should exist after save
	REQUIRE( fs::exists( testPath ) );

	// Should be able to load it back
	editor::EditorConfig config2( testPath );
	REQUIRE( config2.load() );

	deleteTestConfigFile( testPath );
}

TEST_CASE( "EditorConfig save creates parent directories", "[editor-config][unit]" )
{
	const std::string testDir = "test_config_dir";
	const std::string testPath = testDir + "/nested/config.json";

	// Clean up from any previous run
	if ( fs::exists( testDir ) )
	{
		fs::remove_all( testDir );
	}

	editor::EditorConfig config( testPath );
	const bool saved = config.save();

	REQUIRE( saved );
	REQUIRE( fs::exists( testPath ) );

	// Cleanup
	fs::remove_all( testDir );
}

TEST_CASE( "EditorConfig save handles write errors gracefully", "[editor-config][unit]" )
{
	// Use an invalid path (contains invalid characters on Windows)
	const std::string testPath = "test_<>invalid|path?.json";

	editor::EditorConfig config( testPath );
	const bool saved = config.save();

	// Should return false but not crash
	REQUIRE_FALSE( saved );
}

TEST_CASE( "EditorConfig getBool returns default for missing key", "[editor-config][unit]" )
{
	editor::EditorConfig config( "test_getbool.json" );

	const bool value = config.getBool( "nonexistent.key", true );
	REQUIRE( value == true );

	const bool value2 = config.getBool( "another.missing.key", false );
	REQUIRE( value2 == false );
}

TEST_CASE( "EditorConfig setBool and getBool with simple keys", "[editor-config][unit]" )
{
	editor::EditorConfig config( "test_setbool.json" );

	config.setBool( "testKey", true );
	REQUIRE( config.getBool( "testKey", false ) == true );

	config.setBool( "testKey", false );
	REQUIRE( config.getBool( "testKey", true ) == false );
}

TEST_CASE( "EditorConfig setBool and getBool with dot-notation paths", "[editor-config][unit]" )
{
	editor::EditorConfig config( "test_dotnotation.json" );

	// Set nested values
	config.setBool( "ui.panels.hierarchy", true );
	config.setBool( "ui.panels.inspector", false );
	config.setBool( "ui.tools.grid", true );

	// Get nested values
	REQUIRE( config.getBool( "ui.panels.hierarchy", false ) == true );
	REQUIRE( config.getBool( "ui.panels.inspector", true ) == false );
	REQUIRE( config.getBool( "ui.tools.grid", false ) == true );

	// Get default for missing nested path
	REQUIRE( config.getBool( "ui.panels.missing", false ) == false );
}

TEST_CASE( "EditorConfig setBool/getBool persists through save/load", "[editor-config][unit]" )
{
	const std::string testPath = "test_persist.json";
	deleteTestConfigFile( testPath );

	{
		editor::EditorConfig config( testPath );
		config.setBool( "ui.panels.hierarchy", true );
		config.setBool( "ui.tools.grid", false );
		config.save();
	}

	{
		editor::EditorConfig config2( testPath );
		config2.load();

		REQUIRE( config2.getBool( "ui.panels.hierarchy", false ) == true );
		REQUIRE( config2.getBool( "ui.tools.grid", true ) == false );
	}

	deleteTestConfigFile( testPath );
}
// ============================================================================
// AF1: Test integer get/set support
// ============================================================================
TEST_CASE( "EditorConfig can store and retrieve integers", "[editor-config][unit][integers]" )
{
	const std::string testPath = "test_int_config.json";
	deleteTestConfigFile( testPath );

	editor::EditorConfig config( testPath );

	// Test setting integers
	config.setInt( "window.width", 1920 );
	config.setInt( "window.height", 1080 );
	config.setInt( "window.x", 100 );
	config.setInt( "window.y", 50 );

	// Save to disk
	REQUIRE( config.save() );

	// Load in new instance and verify
	{
		editor::EditorConfig config2( testPath );
		REQUIRE( config2.load() );

		REQUIRE( config2.getInt( "window.width", 0 ) == 1920 );
		REQUIRE( config2.getInt( "window.height", 0 ) == 1080 );
		REQUIRE( config2.getInt( "window.x", 0 ) == 100 );
		REQUIRE( config2.getInt( "window.y", 0 ) == 50 );
	}

	deleteTestConfigFile( testPath );
}

TEST_CASE( "EditorConfig getInt returns default for missing keys", "[editor-config][unit][integers]" )
{
	const std::string testPath = "test_int_default_config.json";
	deleteTestConfigFile( testPath );

	editor::EditorConfig config( testPath );
	config.load(); // Load empty/missing file

	// Should return defaults for missing keys
	REQUIRE( config.getInt( "missing.key", 42 ) == 42 );
	REQUIRE( config.getInt( "another.missing", -1 ) == -1 );

	deleteTestConfigFile( testPath );
}

TEST_CASE( "EditorConfig can mix booleans and integers", "[editor-config][unit][integers]" )
{
	const std::string testPath = "test_mixed_config.json";
	deleteTestConfigFile( testPath );

	editor::EditorConfig config( testPath );

	// Set mix of types
	config.setBool( "window.fullscreen", true );
	config.setInt( "window.width", 1600 );
	config.setInt( "window.height", 900 );
	config.setBool( "ui.panels.visible", false );

	// Save and reload
	REQUIRE( config.save() );

	{
		editor::EditorConfig config2( testPath );
		REQUIRE( config2.load() );

		// Verify both types work
		REQUIRE( config2.getBool( "window.fullscreen", false ) == true );
		REQUIRE( config2.getInt( "window.width", 0 ) == 1600 );
		REQUIRE( config2.getInt( "window.height", 0 ) == 900 );
		REQUIRE( config2.getBool( "ui.panels.visible", true ) == false );
	}

	deleteTestConfigFile( testPath );
}
