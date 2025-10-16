#include <catch2/catch_test_macros.hpp>
#include <string>
#include "core/strings.h"

TEST_CASE( "getBaseFilename extracts base filename from path", "[strings][unit]" )
{
	SECTION( "Normal path with directory and extension" )
	{
		const std::string path = "assets/scenes/test_scene.gltf";
		const std::string result = strings::getBaseFilename( path );
		REQUIRE( result == "test_scene" );
	}

	SECTION( "Path with multiple directory levels" )
	{
		const std::string path = "D:/cod_users/setienne/level-editor/assets/models/cube.gltf";
		const std::string result = strings::getBaseFilename( path );
		REQUIRE( result == "cube" );
	}

	SECTION( "Path with backslashes" )
	{
		const std::string path = "C:\\Users\\test\\Documents\\model.gltf";
		const std::string result = strings::getBaseFilename( path );
		REQUIRE( result == "model" );
	}

	SECTION( "Path with mixed slashes" )
	{
		const std::string path = "assets\\scenes/mixed\\path/file.gltf";
		const std::string result = strings::getBaseFilename( path );
		REQUIRE( result == "file" );
	}

	SECTION( "Simple filename without directory" )
	{
		const std::string path = "simple.gltf";
		const std::string result = strings::getBaseFilename( path );
		REQUIRE( result == "simple" );
	}

	SECTION( "Filename without extension" )
	{
		const std::string path = "assets/scenes/no_extension";
		const std::string result = strings::getBaseFilename( path );
		REQUIRE( result == "no_extension" );
	}

	SECTION( "Filename with multiple dots" )
	{
		const std::string path = "assets/test.scene.v2.gltf";
		const std::string result = strings::getBaseFilename( path );
		REQUIRE( result == "test.scene.v2" );
	}

	SECTION( "Empty string" )
	{
		const std::string path = "";
		const std::string result = strings::getBaseFilename( path );
		REQUIRE( result == "" );
	}

	SECTION( "Path with only directory separator at end" )
	{
		const std::string path = "assets/scenes/";
		const std::string result = strings::getBaseFilename( path );
		REQUIRE( result == "" );
	}

	SECTION( "Root directory file" )
	{
		const std::string path = "/model.gltf";
		const std::string result = strings::getBaseFilename( path );
		REQUIRE( result == "model" );
	}

	SECTION( "Filename with dot but no extension" )
	{
		const std::string path = "assets/file.";
		const std::string result = strings::getBaseFilename( path );
		REQUIRE( result == "file" );
	}

	SECTION( "Hidden file (starts with dot)" )
	{
		const std::string path = "assets/.gitignore";
		const std::string result = strings::getBaseFilename( path );
		REQUIRE( result == ".gitignore" );
	}
}

TEST_CASE( "getDirectoryPath extracts directory path from file path", "[strings][unit]" )
{
	SECTION( "Normal path with directory and filename" )
	{
		const std::string path = "assets/scenes/test_scene.gltf";
		const std::string result = strings::getDirectoryPath( path );
		REQUIRE( result == "assets/scenes" );
	}

	SECTION( "Path with multiple directory levels" )
	{
		const std::string path = "D:/cod_users/setienne/level-editor/assets/models/cube.gltf";
		const std::string result = strings::getDirectoryPath( path );
		REQUIRE( result == "D:/cod_users/setienne/level-editor/assets/models" );
	}

	SECTION( "Path with backslashes" )
	{
		const std::string path = "C:\\Users\\test\\Documents\\model.gltf";
		const std::string result = strings::getDirectoryPath( path );
		REQUIRE( result == "C:\\Users\\test\\Documents" );
	}

	SECTION( "Path with mixed slashes" )
	{
		const std::string path = "assets\\scenes/mixed\\path/file.gltf";
		const std::string result = strings::getDirectoryPath( path );
		REQUIRE( result == "assets\\scenes/mixed\\path" );
	}

	SECTION( "Simple filename without directory" )
	{
		const std::string path = "simple.gltf";
		const std::string result = strings::getDirectoryPath( path );
		REQUIRE( result == "" );
	}

	SECTION( "Path with trailing slash" )
	{
		const std::string path = "assets/scenes/";
		const std::string result = strings::getDirectoryPath( path );
		REQUIRE( result == "assets/scenes" );
	}

	SECTION( "Empty string" )
	{
		const std::string path = "";
		const std::string result = strings::getDirectoryPath( path );
		REQUIRE( result == "" );
	}

	SECTION( "Root directory file" )
	{
		const std::string path = "/model.gltf";
		const std::string result = strings::getDirectoryPath( path );
		REQUIRE( result == "" );
	}

	SECTION( "Single directory level" )
	{
		const std::string path = "assets/model.gltf";
		const std::string result = strings::getDirectoryPath( path );
		REQUIRE( result == "assets" );
	}

	SECTION( "Filename without extension" )
	{
		const std::string path = "assets/scenes/no_extension";
		const std::string result = strings::getDirectoryPath( path );
		REQUIRE( result == "assets/scenes" );
	}
}
