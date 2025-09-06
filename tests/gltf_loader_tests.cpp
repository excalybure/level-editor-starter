#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>

import engine.gltf_loader;
import engine.assets;

TEST_CASE( "GLTFLoader Tests", "[gltf][loader]" )
{

	SECTION( "GLTFLoader construction" )
	{
		const gltf_loader::GLTFLoader loader;

		// Constructor should not throw
		REQUIRE( true ); // Basic construction test
	}

	SECTION( "GLTFLoader loadScene returns valid scene" )
	{
		const gltf_loader::GLTFLoader loader;
		const std::string testPath = "test_scene.gltf";

		auto scene = loader.loadScene( testPath );

		REQUIRE( scene != nullptr );
		REQUIRE( scene->getType() == assets::AssetType::Scene );

		// For now, the placeholder should return an empty scene
		REQUIRE( scene->getRootNodes().empty() );
		REQUIRE( scene->getTotalNodeCount() == 0 );
	}

	SECTION( "GLTFLoader loadScene with empty path" )
	{
		const gltf_loader::GLTFLoader loader;
		const std::string emptyPath = "";

		auto scene = loader.loadScene( emptyPath );

		REQUIRE( scene != nullptr );
		REQUIRE( scene->getType() == assets::AssetType::Scene );
	}

	SECTION( "GLTFLoader multiple scene loads" )
	{
		const gltf_loader::GLTFLoader loader;

		auto scene1 = loader.loadScene( "scene1.gltf" );
		auto scene2 = loader.loadScene( "scene2.gltf" );

		REQUIRE( scene1 != nullptr );
		REQUIRE( scene2 != nullptr );
		REQUIRE( scene1.get() != scene2.get() ); // Different instances

		REQUIRE( scene1->getType() == assets::AssetType::Scene );
		REQUIRE( scene2->getType() == assets::AssetType::Scene );
	}
}