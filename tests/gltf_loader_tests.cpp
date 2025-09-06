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

// RED Phase: Tests for actual glTF file loading that should fail initially
TEST_CASE( "GLTFLoader File Loading", "[gltf][loader][file]" )
{
	SECTION( "Load simple triangle glTF" )
	{
		const gltf_loader::GLTFLoader loader;

		// Test data: simple triangle in glTF format
		const std::string gltfContent = R"({
			"asset": { "version": "2.0" },
			"scene": 0,
			"scenes": [{ "nodes": [0] }],
			"nodes": [{ "mesh": 0 }],
			"meshes": [{
				"primitives": [{
					"attributes": { "POSITION": 0 },
					"indices": 1
				}]
			}],
			"accessors": [
				{
					"bufferView": 0,
					"componentType": 5126,
					"count": 3,
					"type": "VEC3"
				},
				{
					"bufferView": 1,
					"componentType": 5123,
					"count": 3,
					"type": "SCALAR"
				}
			],
			"bufferViews": [
				{ "buffer": 0, "byteOffset": 0, "byteLength": 36 },
				{ "buffer": 0, "byteOffset": 36, "byteLength": 6 }
			],
			"buffers": [{
				"byteLength": 42,
				"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAAAAPwAAAAA/AAAAPwAAAAAAAAAAAAABAAIAAA=="
			}]
		})";

		// Create a temporary glTF file
		auto scene = loader.loadFromString( gltfContent );

		REQUIRE( scene != nullptr );
		REQUIRE( scene->getType() == assets::AssetType::Scene );
		REQUIRE( scene->getTotalNodeCount() > 0 );

		// Should have at least one root node
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );

		// First node should have a mesh
		REQUIRE( rootNodes[0]->hasMesh() );
		REQUIRE( !rootNodes[0]->meshes.empty() );
	}

	SECTION( "Load invalid glTF should throw or return null" )
	{
		const gltf_loader::GLTFLoader loader;
		const std::string invalidGltf = "{ invalid json }";

		// Should either throw exception or return null/empty scene
		REQUIRE_THROWS_AS( loader.loadFromString( invalidGltf ), std::runtime_error );
	}

	SECTION( "Load glTF with materials" )
	{
		const gltf_loader::GLTFLoader loader;

		const std::string gltfWithMaterial = R"({
			"asset": { "version": "2.0" },
			"scene": 0,
			"scenes": [{ "nodes": [0] }],
			"nodes": [{ "mesh": 0 }],
			"meshes": [{
				"primitives": [{
					"attributes": { "POSITION": 0 },
					"material": 0
				}]
			}],
			"materials": [{
				"name": "TestMaterial",
				"pbrMetallicRoughness": {
					"baseColorFactor": [1.0, 0.5, 0.0, 1.0],
					"metallicFactor": 0.8,
					"roughnessFactor": 0.2
				}
			}],
			"accessors": [{
				"bufferView": 0,
				"componentType": 5126,
				"count": 3,
				"type": "VEC3"
			}],
			"bufferViews": [{ "buffer": 0, "byteOffset": 0, "byteLength": 36 }],
			"buffers": [{
				"byteLength": 36,
				"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAAAAAA=="
			}]
		})";

		auto scene = loader.loadFromString( gltfWithMaterial );

		REQUIRE( scene != nullptr );
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );
		REQUIRE( rootNodes[0]->hasMaterial() );
	}
}