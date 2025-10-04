#include <catch2/catch_test_macros.hpp>
#include <string>
#include "core/strings.h"

TEST_CASE( "getBaseFilename extracts base filename from path", "[core][strings][unit]" )
{
	SECTION( "Normal path with directory and extension" )
	{
		const std::string path = "assets/scenes/test_scene.gltf";
		const std::string result = core::getBaseFilename( path );
		REQUIRE( result == "test_scene" );
	}

	SECTION( "Path with multiple directory levels" )
	{
		const std::string path = "D:/cod_users/setienne/level-editor/assets/models/cube.gltf";
		const std::string result = core::getBaseFilename( path );
		REQUIRE( result == "cube" );
	}

	SECTION( "Path with backslashes" )
	{
		const std::string path = "C:\\Users\\test\\Documents\\model.gltf";
		const std::string result = core::getBaseFilename( path );
		REQUIRE( result == "model" );
	}

	SECTION( "Path with mixed slashes" )
	{
		const std::string path = "assets\\scenes/mixed\\path/file.gltf";
		const std::string result = core::getBaseFilename( path );
		REQUIRE( result == "file" );
	}

	SECTION( "Simple filename without directory" )
	{
		const std::string path = "simple.gltf";
		const std::string result = core::getBaseFilename( path );
		REQUIRE( result == "simple" );
	}

	SECTION( "Filename without extension" )
	{
		const std::string path = "assets/scenes/no_extension";
		const std::string result = core::getBaseFilename( path );
		REQUIRE( result == "no_extension" );
	}

	SECTION( "Filename with multiple dots" )
	{
		const std::string path = "assets/test.scene.v2.gltf";
		const std::string result = core::getBaseFilename( path );
		REQUIRE( result == "test.scene.v2" );
	}

	SECTION( "Empty string" )
	{
		const std::string path = "";
		const std::string result = core::getBaseFilename( path );
		REQUIRE( result == "" );
	}

	SECTION( "Path with only directory separator at end" )
	{
		const std::string path = "assets/scenes/";
		const std::string result = core::getBaseFilename( path );
		REQUIRE( result == "" );
	}

	SECTION( "Root directory file" )
	{
		const std::string path = "/model.gltf";
		const std::string result = core::getBaseFilename( path );
		REQUIRE( result == "model" );
	}

	SECTION( "Filename with dot but no extension" )
	{
		const std::string path = "assets/file.";
		const std::string result = core::getBaseFilename( path );
		REQUIRE( result == "file" );
	}

	SECTION( "Hidden file (starts with dot)" )
	{
		const std::string path = "assets/.gitignore";
		const std::string result = core::getBaseFilename( path );
		REQUIRE( result == ".gitignore" );
	}
}
