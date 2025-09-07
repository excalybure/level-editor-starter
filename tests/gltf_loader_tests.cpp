#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>
#include <fstream>
#include <cstdio>

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

	SECTION( "GLTFLoader loadScene with non-existent file should throw" )
	{
		const gltf_loader::GLTFLoader loader;
		const std::string testPath = "test_scene.gltf";

		// Should now return null/empty scene for non-existent files
		REQUIRE( !loader.loadScene( testPath ) );
	}

	SECTION( "GLTFLoader loadScene with empty path should throw" )
	{
		const gltf_loader::GLTFLoader loader;
		const std::string emptyPath = "";

		// Should return null/empty scene
		REQUIRE( !loader.loadScene( emptyPath ) );
	}

	SECTION( "GLTFLoader multiple scene loads with non-existent files should throw" )
	{
		const gltf_loader::GLTFLoader loader;

		// Both should return null/empty scene
		REQUIRE( !loader.loadScene( "scene1.gltf" ) );
		REQUIRE( !loader.loadScene( "scene2.gltf" ) );
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

	SECTION( "Extract real triangle mesh data from glTF" )
	{
		const gltf_loader::GLTFLoader loader;

		// Test data: simple triangle with known vertex data
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
				"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAPwAAgD8AAAAAAAABAAIA"
			}]
		})";

		auto scene = loader.loadFromString( gltfContent );

		REQUIRE( scene != nullptr );
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );
		REQUIRE( rootNodes[0]->hasMesh() );

		// NEW: Test that we extract actual mesh data rather than placeholder strings
		// Access the actual mesh object (this will fail until we implement extractMesh)
		const auto meshPtr = rootNodes[0]->getFirstMesh();
		REQUIRE( meshPtr != nullptr );

		// Verify vertex count matches the glTF data (3 vertices)
		REQUIRE( meshPtr->getVertexCount() == 3 );

		// Verify index count matches the glTF data (3 indices)
		REQUIRE( meshPtr->getIndexCount() == 3 );

		// Verify actual vertex positions (triangle vertices: (0,0,0), (1,0,0), (0.5,1,0))
		const auto &vertices = meshPtr->getVertices();
		REQUIRE( vertices.size() == 3 );

		// Check first vertex position (0,0,0)
		REQUIRE( vertices[0].position.x == 0.0f );
		REQUIRE( vertices[0].position.y == 0.0f );
		REQUIRE( vertices[0].position.z == 0.0f );

		// Check second vertex position (1,0,0)
		REQUIRE( vertices[1].position.x == 1.0f );
		REQUIRE( vertices[1].position.y == 0.0f );
		REQUIRE( vertices[1].position.z == 0.0f );

		// Check third vertex position (0.5,1,0)
		REQUIRE( vertices[2].position.x == 0.5f );
		REQUIRE( vertices[2].position.y == 1.0f );
		REQUIRE( vertices[2].position.z == 0.0f );

		// Verify indices are correct (0, 1, 2)
		const auto &indices = meshPtr->getIndices();
		REQUIRE( indices.size() == 3 );
		REQUIRE( indices[0] == 0 );
		REQUIRE( indices[1] == 1 );
		REQUIRE( indices[2] == 2 );

		// NEW: Verify bounding box computation
		REQUIRE( meshPtr->hasBounds() );

		// Expected bounds for triangle vertices (0,0,0), (1,0,0), (0.5,1,0):
		// Min: (0, 0, 0), Max: (1, 1, 0)
		const auto &bounds = meshPtr->getBounds();

		REQUIRE( bounds.min.x == 0.0f );
		REQUIRE( bounds.min.y == 0.0f );
		REQUIRE( bounds.min.z == 0.0f );

		REQUIRE( bounds.max.x == 1.0f );
		REQUIRE( bounds.max.y == 1.0f );
		REQUIRE( bounds.max.z == 0.0f );

		// Verify computed center and size
		float center[3], size[3];
		meshPtr->getBoundsCenter( center );
		meshPtr->getBoundsSize( size );

		REQUIRE( center[0] == 0.5f );
		REQUIRE( center[1] == 0.5f );
		REQUIRE( center[2] == 0.0f );

		REQUIRE( size[0] == 1.0f );
		REQUIRE( size[1] == 1.0f );
		REQUIRE( size[2] == 0.0f );
	}

	SECTION( "Load invalid glTF should throw or return null" )
	{
		const gltf_loader::GLTFLoader loader;
		const std::string invalidGltf = "{ invalid json }";

		// Should return null/empty scene
		REQUIRE( !loader.loadFromString( invalidGltf ) );
	}

	SECTION( "Extract mesh with only positions (no normals, texcoords)" )
	{
		const gltf_loader::GLTFLoader loader;

		// Test data: triangle with only POSITION attribute
		const std::string gltfContent = R"({
			"asset": { "version": "2.0" },
			"scene": 0,
			"scenes": [{ "nodes": [0] }],
			"nodes": [{ "mesh": 0 }],
			"meshes": [{
				"primitives": [{
					"attributes": { "POSITION": 0 }
				}]
			}],
			"accessors": [{
				"bufferView": 0,
				"componentType": 5126,
				"count": 3,
				"type": "VEC3"
			}],
			"bufferViews": [
				{ "buffer": 0, "byteOffset": 0, "byteLength": 36 }
			],
			"buffers": [{
				"byteLength": 36,
				"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAPwAAgD8AAAAAAAAA"
			}]
		})";

		const auto scene = loader.loadFromString( gltfContent );

		REQUIRE( scene != nullptr );
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );
		REQUIRE( rootNodes[0]->hasMesh() );

		const auto meshPtr = rootNodes[0]->getFirstMesh();
		REQUIRE( meshPtr != nullptr );
		REQUIRE( meshPtr->getVertexCount() == 3 );

		// Verify default normal values are used when normals are missing
		const auto &vertices = meshPtr->getVertices();
		REQUIRE( vertices.size() == 3 );

		// All vertices should have default normal (0, 1, 0)
		for ( const auto &vertex : vertices )
		{
			REQUIRE( vertex.normal.x == 0.0f );
			REQUIRE( vertex.normal.y == 1.0f );
			REQUIRE( vertex.normal.z == 0.0f );
		}
	}

	SECTION( "Extract mesh with different index component types" )
	{
		const gltf_loader::GLTFLoader loader;

		// Test data: triangle with uint8 indices
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
					"componentType": 5121,
					"count": 3,
					"type": "SCALAR"
				}
			],
			"bufferViews": [
				{ "buffer": 0, "byteOffset": 0, "byteLength": 36 },
				{ "buffer": 0, "byteOffset": 36, "byteLength": 3 }
			],
			"buffers": [{
				"byteLength": 39,
				"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAIA/AAEC"
			}]
		})";

		const auto scene = loader.loadFromString( gltfContent );

		REQUIRE( scene != nullptr );
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );
		REQUIRE( rootNodes[0]->hasMesh() );

		const auto meshPtr = rootNodes[0]->getFirstMesh();
		REQUIRE( meshPtr != nullptr );
		REQUIRE( meshPtr->getVertexCount() == 3 );
		REQUIRE( meshPtr->getIndexCount() == 3 );

		// Verify indices are correctly converted from uint8 to uint32
		const auto &indices = meshPtr->getIndices();
		REQUIRE( indices.size() == 3 );
		REQUIRE( indices[0] == 0 );
		REQUIRE( indices[1] == 1 );
		REQUIRE( indices[2] == 2 );
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

// RED Phase: Tests for file-based loading (should fail initially)
TEST_CASE( "GLTFLoader File-based Loading", "[gltf][loader][file-loading]" )
{
	SECTION( "Load glTF file with external binary buffer" )
	{
		const gltf_loader::GLTFLoader loader;
		const std::string testPath = "tests/test_assets/simple_triangle.gltf";

		auto scene = loader.loadScene( testPath );

		REQUIRE( scene != nullptr );
		REQUIRE( scene->getType() == assets::AssetType::Scene );
		REQUIRE( scene->getTotalNodeCount() > 0 );

		// Should have at least one root node
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );

		// First node should have a mesh and be named
		REQUIRE( rootNodes[0]->hasMesh() );
		REQUIRE( !rootNodes[0]->meshes.empty() );
		REQUIRE( rootNodes[0]->name == "TriangleNode" );
	}

	SECTION( "Load non-existent glTF file should return nullptr" )
	{
		const gltf_loader::GLTFLoader loader;
		const std::string nonExistentPath = "tests/test_assets/nonexistent.gltf";

		auto scene = loader.loadScene( nonExistentPath );
		REQUIRE( scene == nullptr );
	}

	SECTION( "Load invalid glTF file should return nullptr" )
	{
		const gltf_loader::GLTFLoader loader;

		// Create a temporary invalid file
		const std::string invalidPath = "tests/test_assets/invalid.gltf";
		std::ofstream invalidFile( invalidPath );
		invalidFile << "{ invalid json content }";
		invalidFile.close();

		auto scene = loader.loadScene( invalidPath );
		REQUIRE( scene == nullptr );

		// Clean up
		std::remove( invalidPath.c_str() );
	}

	SECTION( "Load glTF with external binary buffer" )
	{
		const gltf_loader::GLTFLoader loader;

		// Create a test glTF file with external buffer
		const std::string gltfPath = "tests/test_assets/external_test.gltf";
		const std::string binPath = "tests/test_assets/external_test.bin";

		// Create the glTF file
		std::ofstream gltfFile( gltfPath );
		gltfFile << R"({
			"asset": { "version": "2.0" },
			"scene": 0,
			"scenes": [{ "nodes": [0] }],
			"nodes": [{ "mesh": 0, "name": "ExternalNode" }],
			"meshes": [{
				"name": "ExternalTriangle",
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
				"uri": "external_test.bin"
			}]
		})";
		gltfFile.close();

		// Create the binary file
		std::ofstream binFile( binPath, std::ios::binary );
		const float positions[] = { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f };
		const uint16_t indices[] = { 0, 1, 2 };
		binFile.write( reinterpret_cast<const char *>( positions ), sizeof( positions ) );
		binFile.write( reinterpret_cast<const char *>( indices ), sizeof( indices ) );
		binFile.close();

		// Test loading
		auto scene = loader.loadScene( gltfPath );

		REQUIRE( scene != nullptr );
		REQUIRE( scene->getTotalNodeCount() > 0 );
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );
		REQUIRE( rootNodes[0]->hasMesh() );
		REQUIRE( rootNodes[0]->name == "ExternalNode" );

		// Clean up
		std::remove( gltfPath.c_str() );
		std::remove( binPath.c_str() );
	}
}