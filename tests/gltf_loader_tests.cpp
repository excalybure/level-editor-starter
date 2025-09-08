#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>
#include <fstream>
#include <cstdio>
#include <cgltf.h>

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
		const auto scene = loader.loadFromString( gltfContent );

		REQUIRE( scene != nullptr );
		REQUIRE( scene->getType() == assets::AssetType::Scene );
		REQUIRE( scene->getTotalNodeCount() > 0 );

		// Should have at least one root node
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );

		// First node should have a mesh
		REQUIRE( rootNodes[0]->hasMesh() );
		REQUIRE( !rootNodes[0]->meshObjects.empty() );
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

		const auto scene = loader.loadFromString( gltfContent );

		REQUIRE( scene != nullptr );
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );
		REQUIRE( rootNodes[0]->hasMesh() );

		// NEW: Test that we extract actual mesh data rather than placeholder strings
		// Access the actual mesh object (this will fail until we implement extractMesh)
		const auto meshPtr = rootNodes[0]->getFirstMesh();
		REQUIRE( meshPtr != nullptr );

		// NEW: Test primitive-based structure
		// Each mesh should contain one primitive for this simple case
		REQUIRE( meshPtr->getPrimitiveCount() == 1 );

		const auto &primitive = meshPtr->getPrimitive( 0 );
		REQUIRE( primitive.getVertexCount() == 3 );
		REQUIRE( primitive.getIndexCount() == 3 );

		// Verify actual vertex positions (triangle vertices: (0,0,0), (1,0,0), (0.5,1,0))
		const auto &vertices = primitive.getVertices();
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
		const auto &indices = primitive.getIndices();
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
		const auto center = meshPtr->getBoundsCenter();
		const auto size = meshPtr->getBoundsSize();

		REQUIRE( center.x == 0.5f );
		REQUIRE( center.y == 0.5f );
		REQUIRE( center.z == 0.0f );

		REQUIRE( size.x == 1.0f );
		REQUIRE( size.y == 1.0f );
		REQUIRE( size.z == 0.0f );
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
		REQUIRE( meshPtr->getPrimitive( 0 ).getVertexCount() == 3 );

		// Verify default normal values are used when normals are missing
		const auto &primitive = meshPtr->getPrimitive( 0 );
		const auto &vertices = primitive.getVertices();
		REQUIRE( vertices.size() == 3 );

		// All vertices should have default normal (0, 0, 1)
		for ( const auto &vertex : vertices )
		{
			REQUIRE( vertex.normal.x == 0.0f );
			REQUIRE( vertex.normal.y == 0.0f );
			REQUIRE( vertex.normal.z == 1.0f );
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
		REQUIRE( meshPtr->getPrimitive( 0 ).getVertexCount() == 3 );
		REQUIRE( meshPtr->getPrimitive( 0 ).getIndexCount() == 3 );

		// Verify indices are correctly converted from uint8 to uint32
		const auto &primitive = meshPtr->getPrimitive( 0 );
		const auto &indices = primitive.getIndices();
		REQUIRE( indices.size() == 3 );
		REQUIRE( indices[0] == 0 );
		REQUIRE( indices[1] == 1 );
		REQUIRE( indices[2] == 2 );
	}

	SECTION( "Extract mesh with UV coordinates (TEXCOORD_0)" )
	{
		const gltf_loader::GLTFLoader loader;

		// Test data: triangle with positions and UV coordinates
		const std::string gltfContent = R"({
			"asset": { "version": "2.0" },
			"scene": 0,
			"scenes": [{ "nodes": [0] }],
			"nodes": [{ "mesh": 0 }],
			"meshes": [{
				"primitives": [{
					"attributes": { 
						"POSITION": 0,
						"TEXCOORD_0": 1
					}
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
					"componentType": 5126,
					"count": 3,
					"type": "VEC2"
				}
			],
			"bufferViews": [
				{ "buffer": 0, "byteOffset": 0, "byteLength": 36 },
				{ "buffer": 0, "byteOffset": 36, "byteLength": 24 }
			],
			"buffers": [{
				"byteLength": 60,
				"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAIA/AAAAAAAAAD8AAIA/"
			}]
		})";

		const auto scene = loader.loadFromString( gltfContent );

		REQUIRE( scene != nullptr );
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );
		REQUIRE( rootNodes[0]->hasMesh() );

		const auto meshPtr = rootNodes[0]->getFirstMesh();
		REQUIRE( meshPtr != nullptr );
		REQUIRE( meshPtr->getPrimitive( 0 ).getVertexCount() == 3 );

		// Verify UV coordinates are correctly extracted
		const auto &primitive = meshPtr->getPrimitive( 0 );
		const auto &vertices = primitive.getVertices();
		REQUIRE( vertices.size() == 3 );

		// Check UV coordinates: (0,0), (1,0), (0.5,1)
		REQUIRE( vertices[0].texCoord.x == 0.0f );
		REQUIRE( vertices[0].texCoord.y == 0.0f );

		REQUIRE( vertices[1].texCoord.x == 1.0f );
		REQUIRE( vertices[1].texCoord.y == 0.0f );

		REQUIRE( vertices[2].texCoord.x == 0.5f );
		REQUIRE( vertices[2].texCoord.y == 1.0f );
	}

	SECTION( "Extract mesh with UV coordinates but different buffer layout" )
	{
		const gltf_loader::GLTFLoader loader;

		// Test data: interleaved positions and UVs
		const std::string gltfContent = R"({
			"asset": { "version": "2.0" },
			"scene": 0,
			"scenes": [{ "nodes": [0] }],
			"nodes": [{ "mesh": 0 }],
			"meshes": [{
				"primitives": [{
					"attributes": { 
						"POSITION": 0,
						"TEXCOORD_0": 1
					}
				}]
			}],
			"accessors": [
				{
					"bufferView": 0,
					"componentType": 5126,
					"count": 2,
					"type": "VEC3"
				},
				{
					"bufferView": 0,
					"byteOffset": 12,
					"componentType": 5126,
					"count": 2,
					"type": "VEC2"
				}
			],
			"bufferViews": [
				{ "buffer": 0, "byteOffset": 0, "byteLength": 40, "byteStride": 20 }
			],
			"buffers": [{
				"byteLength": 40,
				"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AACAPw=="
			}]
		})";

		const auto scene = loader.loadFromString( gltfContent );

		REQUIRE( scene != nullptr );
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );
		REQUIRE( rootNodes[0]->hasMesh() );

		const auto meshPtr = rootNodes[0]->getFirstMesh();
		REQUIRE( meshPtr != nullptr );
		REQUIRE( meshPtr->getPrimitive( 0 ).getVertexCount() == 2 );

		// Verify UV coordinates are correctly extracted from interleaved data
		const auto &primitive = meshPtr->getPrimitive( 0 );
		const auto &vertices = primitive.getVertices();
		REQUIRE( vertices.size() == 2 );

		// Check UV coordinates from interleaved buffer
		REQUIRE( vertices[0].texCoord.x == 0.0f );
		REQUIRE( vertices[0].texCoord.y == 0.0f );

		REQUIRE( vertices[1].texCoord.x == 1.0f );
		REQUIRE( vertices[1].texCoord.y == 1.0f );
	}

	SECTION( "Extract mesh without UV coordinates uses default values" )
	{
		const gltf_loader::GLTFLoader loader;

		// Test data: triangle with only positions (no UVs)
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
				"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAIA/AAAA"
			}]
		})";

		const auto scene = loader.loadFromString( gltfContent );

		REQUIRE( scene != nullptr );
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );
		REQUIRE( rootNodes[0]->hasMesh() );

		const auto meshPtr = rootNodes[0]->getFirstMesh();
		REQUIRE( meshPtr != nullptr );
		REQUIRE( meshPtr->getPrimitive( 0 ).getVertexCount() == 3 );

		// Verify default UV coordinates are used when UVs are missing
		const auto &primitive = meshPtr->getPrimitive( 0 );
		const auto &vertices = primitive.getVertices();
		REQUIRE( vertices.size() == 3 );

		// All vertices should have default UV (0.0, 0.0)
		for ( const auto &vertex : vertices )
		{
			REQUIRE( vertex.texCoord.x == 0.0f );
			REQUIRE( vertex.texCoord.y == 0.0f );
		}
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

		const auto scene = loader.loadFromString( gltfWithMaterial );

		REQUIRE( scene != nullptr );
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );
		REQUIRE( rootNodes[0]->hasMaterial() );
	}

	SECTION( "Extract mesh with tangent vectors (TANGENT)" )
	{
		const gltf_loader::GLTFLoader loader;

		// Test data: triangle with positions, normals, and tangent vectors
		const std::string gltfContent = R"({
			"asset": { "version": "2.0" },
			"scene": 0,
			"scenes": [{ "nodes": [0] }],
			"nodes": [{ "mesh": 0 }],
			"meshes": [{
				"primitives": [{
					"attributes": { 
						"POSITION": 0,
						"NORMAL": 1,
						"TANGENT": 2
					}
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
					"componentType": 5126,
					"count": 3,
					"type": "VEC3"
				},
				{
					"bufferView": 2,
					"componentType": 5126,
					"count": 3,
					"type": "VEC4"
				}
			],
			"bufferViews": [
				{ "buffer": 0, "byteOffset": 0, "byteLength": 36 },
				{ "buffer": 0, "byteOffset": 36, "byteLength": 36 },
				{ "buffer": 0, "byteOffset": 72, "byteLength": 48 }
			],
			"buffers": [{
				"byteLength": 120,
				"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AACAPwAAAAAAAAAAAACAPwAAgD8AAAAAAAAAAAAAgD8AAIA/AAAAAAAAAAAAAIA/"
			}]
		})";

		const auto scene = loader.loadFromString( gltfContent );

		REQUIRE( scene != nullptr );
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );
		REQUIRE( rootNodes[0]->hasMesh() );

		const auto meshPtr = rootNodes[0]->getFirstMesh();
		REQUIRE( meshPtr != nullptr );
		REQUIRE( meshPtr->getPrimitive( 0 ).getVertexCount() == 3 );

		// Verify tangent vectors are correctly extracted
		const auto &primitive = meshPtr->getPrimitive( 0 );
		const auto &vertices = primitive.getVertices();
		REQUIRE( vertices.size() == 3 );

		// Check tangent vectors: all should be (1,0,0,1) for this test
		for ( const auto &vertex : vertices )
		{
			REQUIRE( vertex.tangent.x == 1.0f );
			REQUIRE( vertex.tangent.y == 0.0f );
			REQUIRE( vertex.tangent.z == 0.0f );
			REQUIRE( vertex.tangent.w == 1.0f ); // Handedness
		}
	}

	SECTION( "Extract mesh with tangents having different handedness" )
	{
		const gltf_loader::GLTFLoader loader;

		// Test data: tangents with mixed handedness (w = 1.0 and w = -1.0)
		const std::string gltfContent = R"({
			"asset": { "version": "2.0" },
			"scene": 0,
			"scenes": [{ "nodes": [0] }],
			"nodes": [{ "mesh": 0 }],
			"meshes": [{
				"primitives": [{
					"attributes": { 
						"POSITION": 0,
						"TANGENT": 1
					}
				}]
			}],
			"accessors": [
				{
					"bufferView": 0,
					"componentType": 5126,
					"count": 2,
					"type": "VEC3"
				},
				{
					"bufferView": 1,
					"componentType": 5126,
					"count": 2,
					"type": "VEC4"
				}
			],
			"bufferViews": [
				{ "buffer": 0, "byteOffset": 0, "byteLength": 24 },
				{ "buffer": 0, "byteOffset": 24, "byteLength": 32 }
			],
			"buffers": [{
				"byteLength": 56,
				"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAACAPwAAAAAAAAAAAACAPwAAgD8AAAAAAAAAAAAAgL8="
			}]
		})";

		const auto scene = loader.loadFromString( gltfContent );

		REQUIRE( scene != nullptr );
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );
		REQUIRE( rootNodes[0]->hasMesh() );

		const auto meshPtr = rootNodes[0]->getFirstMesh();
		REQUIRE( meshPtr != nullptr );
		REQUIRE( meshPtr->getPrimitive( 0 ).getVertexCount() == 2 );

		// Verify tangent vectors with different handedness
		const auto &primitive = meshPtr->getPrimitive( 0 );
		const auto &vertices = primitive.getVertices();
		REQUIRE( vertices.size() == 2 );

		// First tangent: (1, 0, 0, 1) - right-handed
		REQUIRE( vertices[0].tangent.x == 1.0f );
		REQUIRE( vertices[0].tangent.y == 0.0f );
		REQUIRE( vertices[0].tangent.z == 0.0f );
		REQUIRE( vertices[0].tangent.w == 1.0f );

		// Second tangent: (1, 0, 0, -1) - left-handed
		REQUIRE( vertices[1].tangent.x == 1.0f );
		REQUIRE( vertices[1].tangent.y == 0.0f );
		REQUIRE( vertices[1].tangent.z == 0.0f );
		REQUIRE( vertices[1].tangent.w == -1.0f );
	}

	SECTION( "Extract mesh without tangents uses default values" )
	{
		const gltf_loader::GLTFLoader loader;

		// Test data: triangle with positions and normals but no tangents
		const std::string gltfContent = R"({
			"asset": { "version": "2.0" },
			"scene": 0,
			"scenes": [{ "nodes": [0] }],
			"nodes": [{ "mesh": 0 }],
			"meshes": [{
				"primitives": [{
					"attributes": { 
						"POSITION": 0,
						"NORMAL": 1
					}
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
					"componentType": 5126,
					"count": 3,
					"type": "VEC3"
				}
			],
			"bufferViews": [
				{ "buffer": 0, "byteOffset": 0, "byteLength": 36 },
				{ "buffer": 0, "byteOffset": 36, "byteLength": 36 }
			],
			"buffers": [{
				"byteLength": 72,
				"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAIA/AAAAAAAAAAAAAAAAAACAP0YOzL8AAAAAAAAAAIA/QczMPgAAAAAAAACAPw=="
			}]
		})";

		const auto scene = loader.loadFromString( gltfContent );

		REQUIRE( scene != nullptr );
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );
		REQUIRE( rootNodes[0]->hasMesh() );

		const auto meshPtr = rootNodes[0]->getFirstMesh();
		REQUIRE( meshPtr != nullptr );
		REQUIRE( meshPtr->getPrimitive( 0 ).getVertexCount() == 3 );

		// Verify default tangent vectors are used when tangents are missing
		const auto &primitive = meshPtr->getPrimitive( 0 );
		const auto &vertices = primitive.getVertices();
		REQUIRE( vertices.size() == 3 );

		// All vertices should have default tangent (1.0, 0.0, 0.0, 1.0)
		for ( const auto &vertex : vertices )
		{
			REQUIRE( vertex.tangent.x == 1.0f );
			REQUIRE( vertex.tangent.y == 0.0f );
			REQUIRE( vertex.tangent.z == 0.0f );
			REQUIRE( vertex.tangent.w == 1.0f );
		}
	}
}

// Integration tests for complete mesh extraction with all vertex attributes
TEST_CASE( "GLTFLoader Complete Mesh Extraction", "[gltf][loader][integration]" )
{
	SECTION( "Extract mesh with all vertex attributes (positions, normals, UVs, tangents)" )
	{
		const gltf_loader::GLTFLoader loader;

		// Test data: complete triangle with all attributes
		const std::string gltfContent = R"({
			"asset": { "version": "2.0" },
			"scene": 0,
			"scenes": [{ "nodes": [0] }],
			"nodes": [{ "mesh": 0 }],
			"meshes": [{
				"primitives": [{
					"attributes": { 
						"POSITION": 0,
						"NORMAL": 1,
						"TEXCOORD_0": 2,
						"TANGENT": 3
					},
					"indices": 4
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
					"componentType": 5126,
					"count": 3,
					"type": "VEC3"
				},
				{
					"bufferView": 2,
					"componentType": 5126,
					"count": 3,
					"type": "VEC2"
				},
				{
					"bufferView": 3,
					"componentType": 5126,
					"count": 3,
					"type": "VEC4"
				},
				{
					"bufferView": 4,
					"componentType": 5123,
					"count": 3,
					"type": "SCALAR"
				}
			],
			"bufferViews": [
				{ "buffer": 0, "byteOffset": 0, "byteLength": 36 },
				{ "buffer": 0, "byteOffset": 36, "byteLength": 36 },
				{ "buffer": 0, "byteOffset": 72, "byteLength": 24 },
				{ "buffer": 0, "byteOffset": 96, "byteLength": 48 },
				{ "buffer": 0, "byteOffset": 144, "byteLength": 6 }
			],
			"buffers": [{
				"byteLength": 150,
				"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AACAPwAAAAAAAAAAAACAPwAAgD8AAAAAAAAAAAAAgD8AAIA/AAAAAAAAAAAAAIA/AAABAAIA"
			}]
		})";

		const auto scene = loader.loadFromString( gltfContent );

		REQUIRE( scene != nullptr );
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );
		REQUIRE( rootNodes[0]->hasMesh() );

		const auto meshPtr = rootNodes[0]->getFirstMesh();
		REQUIRE( meshPtr != nullptr );
		REQUIRE( meshPtr->getPrimitive( 0 ).getVertexCount() == 3 );
		REQUIRE( meshPtr->getPrimitive( 0 ).getIndexCount() == 3 );

		// Verify all vertex attributes are correctly extracted
		const auto &primitive = meshPtr->getPrimitive( 0 );
		const auto &vertices = primitive.getVertices();
		REQUIRE( vertices.size() == 3 );

		// Verify first vertex has all attributes
		const auto &v0 = vertices[0];
		REQUIRE( v0.position.x == 0.0f );
		REQUIRE( v0.position.y == 0.0f );
		REQUIRE( v0.position.z == 0.0f );

		REQUIRE( v0.normal.x == 0.0f );
		REQUIRE( v0.normal.y == 0.0f );
		REQUIRE( v0.normal.z == 1.0f );

		REQUIRE( v0.texCoord.x == 0.0f );
		REQUIRE( v0.texCoord.y == 0.0f );

		REQUIRE( v0.tangent.x == 1.0f );
		REQUIRE( v0.tangent.y == 0.0f );
		REQUIRE( v0.tangent.z == 0.0f );
		REQUIRE( v0.tangent.w == 1.0f );

		// Verify indices are correctly extracted
		const auto &indices = primitive.getIndices();
		REQUIRE( indices.size() == 3 );
		REQUIRE( indices[0] == 0 );
		REQUIRE( indices[1] == 1 );
		REQUIRE( indices[2] == 2 );
	}

	SECTION( "Extract mesh with interleaved vertex data (all attributes in single buffer)" )
	{
		const gltf_loader::GLTFLoader loader;

		// Test data: interleaved vertex data (pos + normal + uv + tangent per vertex)
		const std::string gltfContent = R"({
			"asset": { "version": "2.0" },
			"scene": 0,
			"scenes": [{ "nodes": [0] }],
			"nodes": [{ "mesh": 0 }],
			"meshes": [{
				"primitives": [{
					"attributes": { 
						"POSITION": 0,
						"NORMAL": 1,
						"TEXCOORD_0": 2,
						"TANGENT": 3
					}
				}]
			}],
			"accessors": [
				{
					"bufferView": 0,
					"byteOffset": 0,
					"componentType": 5126,
					"count": 2,
					"type": "VEC3"
				},
				{
					"bufferView": 0,
					"byteOffset": 12,
					"componentType": 5126,
					"count": 2,
					"type": "VEC3"
				},
				{
					"bufferView": 0,
					"byteOffset": 24,
					"componentType": 5126,
					"count": 2,
					"type": "VEC2"
				},
				{
					"bufferView": 0,
					"byteOffset": 32,
					"componentType": 5126,
					"count": 2,
					"type": "VEC4"
				}
			],
			"bufferViews": [
				{ "buffer": 0, "byteOffset": 0, "byteLength": 96, "byteStride": 48 }
			],
			"buffers": [{
				"byteLength": 96,
				"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AACAPwAAAAAAAAAAAAAAAAAAAAAAAIA/AACAPwAAAAAAAIA/AAAAAAAAAAAAAIA/"
			}]
		})";

		const auto scene = loader.loadFromString( gltfContent );

		REQUIRE( scene != nullptr );
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );
		REQUIRE( rootNodes[0]->hasMesh() );

		const auto meshPtr = rootNodes[0]->getFirstMesh();
		REQUIRE( meshPtr != nullptr );
		REQUIRE( meshPtr->getPrimitive( 0 ).getVertexCount() == 2 );

		// Verify interleaved data extraction
		const auto &primitive = meshPtr->getPrimitive( 0 );
		const auto &vertices = primitive.getVertices();
		REQUIRE( vertices.size() == 2 );

		// Check that stride correctly extracts each vertex's attributes
		for ( const auto &vertex : vertices )
		{
			// Normal should be (0, 0, 1) for all vertices in this test
			REQUIRE( vertex.normal.x == 0.0f );
			REQUIRE( vertex.normal.y == 0.0f );
			REQUIRE( vertex.normal.z == 1.0f );

			// Tangent should be (1, 0, 0, 1) for all vertices in this test
			REQUIRE( vertex.tangent.x == 1.0f );
			REQUIRE( vertex.tangent.y == 0.0f );
			REQUIRE( vertex.tangent.z == 0.0f );
			REQUIRE( vertex.tangent.w == 1.0f );
		}
	}

	SECTION( "Extract mesh with some missing optional attributes" )
	{
		const gltf_loader::GLTFLoader loader;

		// Test data: mesh with positions, UVs, but no normals or tangents
		const std::string gltfContent = R"({
			"asset": { "version": "2.0" },
			"scene": 0,
			"scenes": [{ "nodes": [0] }],
			"nodes": [{ "mesh": 0 }],
			"meshes": [{
				"primitives": [{
					"attributes": { 
						"POSITION": 0,
						"TEXCOORD_0": 1
					}
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
					"componentType": 5126,
					"count": 3,
					"type": "VEC2"
				}
			],
			"bufferViews": [
				{ "buffer": 0, "byteOffset": 0, "byteLength": 36 },
				{ "buffer": 0, "byteOffset": 36, "byteLength": 24 }
			],
			"buffers": [{
				"byteLength": 60,
				"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAIA/AAAAAAAAAD8AAIA/"
			}]
		})";

		const auto scene = loader.loadFromString( gltfContent );

		REQUIRE( scene != nullptr );
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );
		REQUIRE( rootNodes[0]->hasMesh() );

		const auto meshPtr = rootNodes[0]->getFirstMesh();
		REQUIRE( meshPtr != nullptr );
		REQUIRE( meshPtr->getPrimitive( 0 ).getVertexCount() == 3 );

		// Verify UVs are extracted and defaults are used for missing attributes
		const auto &primitive = meshPtr->getPrimitive( 0 );
		const auto &vertices = primitive.getVertices();
		REQUIRE( vertices.size() == 3 );

		for ( std::size_t i = 0; i < vertices.size(); ++i )
		{
			const auto &vertex = vertices[i];

			// UVs should be extracted correctly
			if ( i == 0 )
			{
				REQUIRE( vertex.texCoord.x == 0.0f );
				REQUIRE( vertex.texCoord.y == 0.0f );
			}
			else if ( i == 1 )
			{
				REQUIRE( vertex.texCoord.x == 1.0f );
				REQUIRE( vertex.texCoord.y == 0.0f );
			}
			else if ( i == 2 )
			{
				REQUIRE( vertex.texCoord.x == 0.5f );
				REQUIRE( vertex.texCoord.y == 1.0f );
			}

			// Default values for missing attributes
			REQUIRE( vertex.normal.x == 0.0f );
			REQUIRE( vertex.normal.y == 0.0f );
			REQUIRE( vertex.normal.z == 1.0f );

			REQUIRE( vertex.tangent.x == 1.0f );
			REQUIRE( vertex.tangent.y == 0.0f );
			REQUIRE( vertex.tangent.z == 0.0f );
			REQUIRE( vertex.tangent.w == 1.0f );
		}
	}

	SECTION( "Extract large mesh with all attributes and validate performance" )
	{
		const gltf_loader::GLTFLoader loader;

		// Test data: quad mesh (4 vertices, 6 indices) with all attributes
		const std::string gltfContent = R"({
			"asset": { "version": "2.0" },
			"scene": 0,
			"scenes": [{ "nodes": [0] }],
			"nodes": [{ "mesh": 0 }],
			"meshes": [{
				"primitives": [{
					"attributes": { 
						"POSITION": 0,
						"NORMAL": 1,
						"TEXCOORD_0": 2,
						"TANGENT": 3
					},
					"indices": 4
				}]
			}],
			"accessors": [
				{
					"bufferView": 0,
					"componentType": 5126,
					"count": 4,
					"type": "VEC3"
				},
				{
					"bufferView": 1,
					"componentType": 5126,
					"count": 4,
					"type": "VEC3"
				},
				{
					"bufferView": 2,
					"componentType": 5126,
					"count": 4,
					"type": "VEC2"
				},
				{
					"bufferView": 3,
					"componentType": 5126,
					"count": 4,
					"type": "VEC4"
				},
				{
					"bufferView": 4,
					"componentType": 5123,
					"count": 6,
					"type": "SCALAR"
				}
			],
			"bufferViews": [
				{ "buffer": 0, "byteOffset": 0, "byteLength": 48 },
				{ "buffer": 0, "byteOffset": 48, "byteLength": 48 },
				{ "buffer": 0, "byteOffset": 96, "byteLength": 32 },
				{ "buffer": 0, "byteOffset": 128, "byteLength": 64 },
				{ "buffer": 0, "byteOffset": 192, "byteLength": 12 }
			],
			"buffers": [{
				"byteLength": 204,
				"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAACAPwAAgD8AAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAgD8AAIA/AAAAAAAAgD8AAIA/AAAAAAAAAAAAAIA/AACAPwAAAAAAAAAAAACAPwAAgD8AAAAAAAAAAAAAgD8AAIA/AAAAAAAAAAAAAIA/AAABAAIAAAACAAMA"
			}]
		})";

		const auto scene = loader.loadFromString( gltfContent );

		REQUIRE( scene != nullptr );
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );
		REQUIRE( rootNodes[0]->hasMesh() );

		const auto meshPtr = rootNodes[0]->getFirstMesh();
		REQUIRE( meshPtr != nullptr );
		REQUIRE( meshPtr->getPrimitive( 0 ).getVertexCount() == 4 );
		REQUIRE( meshPtr->getPrimitive( 0 ).getIndexCount() == 6 );

		// Verify quad mesh extraction
		const auto &primitive = meshPtr->getPrimitive( 0 );
		const auto &vertices = primitive.getVertices();
		REQUIRE( vertices.size() == 4 );

		// Verify all vertices have valid attributes
		for ( const auto &vertex : vertices )
		{
			// Normal vectors should be unit length and pointing up
			REQUIRE( vertex.normal.x == 0.0f );
			REQUIRE( vertex.normal.y == 0.0f );
			REQUIRE( vertex.normal.z == 1.0f );

			// Tangent vectors should be valid
			REQUIRE( vertex.tangent.x == 1.0f );
			REQUIRE( vertex.tangent.y == 0.0f );
			REQUIRE( vertex.tangent.z == 0.0f );
			REQUIRE( ( vertex.tangent.w == 1.0f || vertex.tangent.w == -1.0f ) ); // Valid handedness
		}

		// Verify indices form proper triangles (0,1,2) and (1,3,2) for quad
		const auto &indices = primitive.getIndices();
		REQUIRE( indices.size() == 6 );
		// Check that indices are within valid range
		for ( auto index : indices )
		{
			REQUIRE( index < 4 );
		}
	}

	SECTION( "Extract mesh with multiple primitives" )
	{
		const gltf_loader::GLTFLoader loader;

		// Test data: mesh with two primitives (both triangles with positions and normals)
		const std::string gltfContent = R"({
			"asset": { "version": "2.0" },
			"scene": 0,
			"scenes": [{ "nodes": [0] }],
			"nodes": [{ "mesh": 0 }],
			"meshes": [{
				"primitives": [
					{
						"attributes": { 
							"POSITION": 0,
							"NORMAL": 1
						},
						"indices": 2
					},
					{
						"attributes": { 
							"POSITION": 3,
							"NORMAL": 4
						},
						"indices": 5
					}
				]
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
					"componentType": 5126,
					"count": 3,
					"type": "VEC3"
				},
				{
					"bufferView": 2,
					"componentType": 5123,
					"count": 3,
					"type": "SCALAR"
				},
				{
					"bufferView": 3,
					"componentType": 5126,
					"count": 3,
					"type": "VEC3"
				},
				{
					"bufferView": 4,
					"componentType": 5126,
					"count": 3,
					"type": "VEC3"
				},
				{
					"bufferView": 5,
					"componentType": 5123,
					"count": 3,
					"type": "SCALAR"
				}
			],
			"bufferViews": [
				{ "buffer": 0, "byteOffset": 0, "byteLength": 36 },
				{ "buffer": 0, "byteOffset": 36, "byteLength": 36 },
				{ "buffer": 0, "byteOffset": 72, "byteLength": 6 },
				{ "buffer": 0, "byteOffset": 78, "byteLength": 36 },
				{ "buffer": 0, "byteOffset": 114, "byteLength": 36 },
				{ "buffer": 0, "byteOffset": 150, "byteLength": 6 }
			],
			"buffers": [{
				"byteLength": 156,
				"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAPwAAgD8AAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAABAAIAAAAAQAAAAAAAAAAAAABAQAAAAAAAAAAAAAAgQAAAgD8AAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAABAAIA"
			}]
		})";

		const auto scene = loader.loadFromString( gltfContent );

		REQUIRE( scene != nullptr );
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );
		REQUIRE( rootNodes[0]->hasMesh() );

		const auto meshPtr = rootNodes[0]->getFirstMesh();
		REQUIRE( meshPtr != nullptr );

		// Verify the mesh has exactly 2 primitives
		REQUIRE( meshPtr->getPrimitiveCount() == 2 );

		// Verify first primitive (triangle 1)
		const auto &primitive1 = meshPtr->getPrimitive( 0 );
		REQUIRE( primitive1.getVertexCount() == 3 );
		REQUIRE( primitive1.getIndexCount() == 3 );

		const auto &vertices1 = primitive1.getVertices();
		REQUIRE( vertices1.size() == 3 );

		// Verify triangle vertices have positions and normals
		for ( const auto &vertex : vertices1 )
		{
			REQUIRE( vertex.normal.x == 0.0f );
			REQUIRE( vertex.normal.y == 0.0f );
			REQUIRE( vertex.normal.z == 1.0f );

			// Default UV coordinates since not provided
			REQUIRE( vertex.texCoord.x == 0.0f );
			REQUIRE( vertex.texCoord.y == 0.0f );
		}

		const auto &indices1 = primitive1.getIndices();
		REQUIRE( indices1.size() == 3 );
		REQUIRE( indices1[0] == 0 );
		REQUIRE( indices1[1] == 1 );
		REQUIRE( indices1[2] == 2 );

		// Verify second primitive (triangle 2)
		const auto &primitive2 = meshPtr->getPrimitive( 1 );
		REQUIRE( primitive2.getVertexCount() == 3 );
		REQUIRE( primitive2.getIndexCount() == 3 );

		const auto &vertices2 = primitive2.getVertices();
		REQUIRE( vertices2.size() == 3 );

		// Verify second triangle has positions and normals
		for ( const auto &vertex : vertices2 )
		{
			REQUIRE( vertex.normal.x == 0.0f );
			REQUIRE( vertex.normal.y == 0.0f );
			REQUIRE( vertex.normal.z == 1.0f );

			// Default UV coordinates since not provided
			REQUIRE( vertex.texCoord.x == 0.0f );
			REQUIRE( vertex.texCoord.y == 0.0f );
		}

		const auto &indices2 = primitive2.getIndices();
		REQUIRE( indices2.size() == 3 );
		REQUIRE( indices2[0] == 0 );
		REQUIRE( indices2[1] == 1 );
		REQUIRE( indices2[2] == 2 );

		// Verify that mesh bounds encompass both primitives
		REQUIRE( meshPtr->hasBounds() );
		const auto &bounds = meshPtr->getBounds();

		// Bounds should cover both triangles (x: 0-3, y: 0-1, z: 0)
		REQUIRE( bounds.min.x == 0.0f );
		REQUIRE( bounds.min.y == 0.0f );
		REQUIRE( bounds.min.z == 0.0f );
		REQUIRE( bounds.max.x == 3.0f );
		REQUIRE( bounds.max.y == 1.0f );
		REQUIRE( bounds.max.z == 0.0f );
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
		REQUIRE( !rootNodes[0]->meshObjects.empty() );
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

// Test unaligned byte offset handling in extractFloat functions
TEST_CASE( "GLTFLoader Unaligned Byte Offset Handling", "[gltf][loader][unaligned]" )
{
	SECTION( "extractFloat3Positions handles unaligned offsets correctly" )
	{
		// Create a buffer with float data starting at unaligned offset (offset 2)
		// Layout: [2 padding bytes][float1][float2][float3]
		std::vector<std::uint8_t> buffer( 16, 0 );

		// Place float values at offset 2 (unaligned)
		const float expectedValues[] = { 1.0f, 2.0f, 3.0f };
		std::memcpy( buffer.data() + 2, expectedValues, sizeof( expectedValues ) );

		const float *floatBuffer = reinterpret_cast<const float *>( buffer.data() );
		const auto positions = gltf_loader::extractFloat3Positions( floatBuffer, 1, 2, 0 );

		REQUIRE( positions.size() == 1 );
		REQUIRE( positions[0].x == 1.0f );
		REQUIRE( positions[0].y == 2.0f );
		REQUIRE( positions[0].z == 3.0f );
	}

	SECTION( "extractFloat3Normals handles unaligned offsets correctly" )
	{
		// Create a buffer with normal data starting at unaligned offset (offset 6)
		std::vector<std::uint8_t> buffer( 20, 0 );

		// Place float values at offset 6 (unaligned)
		const float expectedNormals[] = { 0.0f, 1.0f, 0.0f };
		std::memcpy( buffer.data() + 6, expectedNormals, sizeof( expectedNormals ) );

		const float *floatBuffer = reinterpret_cast<const float *>( buffer.data() );
		const auto normals = gltf_loader::extractFloat3Normals( floatBuffer, 1, 6, 0 );

		REQUIRE( normals.size() == 1 );
		REQUIRE( normals[0].x == 0.0f );
		REQUIRE( normals[0].y == 1.0f );
		REQUIRE( normals[0].z == 0.0f );
	}

	SECTION( "extractFloat2UVs handles unaligned offsets correctly" )
	{
		// Create a buffer with UV data starting at unaligned offset (offset 10)
		std::vector<std::uint8_t> buffer( 20, 0 );

		// Place float values at offset 10 (unaligned)
		const float expectedUVs[] = { 0.5f, 0.75f };
		std::memcpy( buffer.data() + 10, expectedUVs, sizeof( expectedUVs ) );

		const float *floatBuffer = reinterpret_cast<const float *>( buffer.data() );
		const auto uvs = gltf_loader::extractFloat2UVs( floatBuffer, 1, 10, 0 );

		REQUIRE( uvs.size() == 1 );
		REQUIRE( uvs[0].x == 0.5f );
		REQUIRE( uvs[0].y == 0.75f );
	}
}

// Tests for Material Parsing
TEST_CASE( "GLTFLoader Material Parsing", "[gltf][loader][material]" )
{
	SECTION( "Parse material with PBR factors" )
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
				"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAIA/"
			}]
		})";

		const auto scene = loader.loadFromString( gltfWithMaterial );

		REQUIRE( scene != nullptr );
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );
		REQUIRE( rootNodes[0]->hasMesh() );

		// Get the mesh and verify it has a primitive with material
		const auto meshPtr = rootNodes[0]->getFirstMesh();
		REQUIRE( meshPtr != nullptr );
		REQUIRE( meshPtr->getPrimitiveCount() == 1 );

		const auto &primitive = meshPtr->getPrimitive( 0 );
		REQUIRE( primitive.hasMaterial() );

		// Verify material path was captured (for now just check it's not empty)
		REQUIRE( !primitive.getMaterialPath().empty() );
	}

	SECTION( "Parse material with emissive factor" )
	{
		const gltf_loader::GLTFLoader loader;

		const std::string gltfWithEmissive = R"({
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
				"name": "EmissiveMaterial",
				"pbrMetallicRoughness": {
					"baseColorFactor": [1.0, 1.0, 1.0, 1.0]
				},
				"emissiveFactor": [0.2, 0.4, 0.6]
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
				"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAIA/"
			}]
		})";

		const auto scene = loader.loadFromString( gltfWithEmissive );

		REQUIRE( scene != nullptr );
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );
		REQUIRE( rootNodes[0]->hasMesh() );

		const auto meshPtr = rootNodes[0]->getFirstMesh();
		REQUIRE( meshPtr != nullptr );
		const auto &primitive = meshPtr->getPrimitive( 0 );
		REQUIRE( primitive.hasMaterial() );
	}

	SECTION( "Parse material with texture references" )
	{
		const gltf_loader::GLTFLoader loader;

		const std::string gltfWithTextures = R"({
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
				"name": "TexturedMaterial",
				"pbrMetallicRoughness": {
					"baseColorTexture": { "index": 0 },
					"metallicRoughnessTexture": { "index": 1 }
				},
				"normalTexture": { "index": 2 },
				"emissiveTexture": { "index": 3 }
			}],
			"textures": [
				{ "source": 0 },
				{ "source": 1 },
				{ "source": 2 },
				{ "source": 3 }
			],
			"images": [
				{ "uri": "basecolor.png" },
				{ "uri": "metallic_roughness.png" },
				{ "uri": "normal.png" },
				{ "uri": "emissive.png" }
			],
			"accessors": [{
				"bufferView": 0,
				"componentType": 5126,
				"count": 3,
				"type": "VEC3"
			}],
			"bufferViews": [{ "buffer": 0, "byteOffset": 0, "byteLength": 36 }],
			"buffers": [{
				"byteLength": 36,
				"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAIA/"
			}]
		})";

		const auto scene = loader.loadFromString( gltfWithTextures );

		REQUIRE( scene != nullptr );
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );
		REQUIRE( rootNodes[0]->hasMesh() );

		const auto meshPtr = rootNodes[0]->getFirstMesh();
		REQUIRE( meshPtr != nullptr );
		const auto &primitive = meshPtr->getPrimitive( 0 );
		REQUIRE( primitive.hasMaterial() );
	}

	SECTION( "Parse material with default values when factors are missing" )
	{
		const gltf_loader::GLTFLoader loader;

		const std::string gltfMinimalMaterial = R"({
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
				"name": "MinimalMaterial"
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
				"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAIA/"
			}]
		})";

		const auto scene = loader.loadFromString( gltfMinimalMaterial );

		REQUIRE( scene != nullptr );
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );
		REQUIRE( rootNodes[0]->hasMesh() );

		const auto meshPtr = rootNodes[0]->getFirstMesh();
		REQUIRE( meshPtr != nullptr );
		const auto &primitive = meshPtr->getPrimitive( 0 );
		REQUIRE( primitive.hasMaterial() );
	}

	SECTION( "Extract and validate PBR factor values" )
	{
		const gltf_loader::GLTFLoader loader;

		// Test material extraction directly
		const std::string gltfWithDetailedMaterial = R"({
			"materials": [{
				"name": "DetailedMaterial",
				"pbrMetallicRoughness": {
					"baseColorFactor": [0.8, 0.2, 0.1, 0.9],
					"metallicFactor": 0.7,
					"roughnessFactor": 0.3
				},
				"emissiveFactor": [0.1, 0.05, 0.02]
			}]
		})";

		// Since we cannot directly test extractMaterial function from outside,
		// we need to test through a complete glTF scene that uses the material
		const std::string completeGltf = R"({
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
				"name": "DetailedMaterial",
				"pbrMetallicRoughness": {
					"baseColorFactor": [0.8, 0.2, 0.1, 0.9],
					"metallicFactor": 0.7,
					"roughnessFactor": 0.3
				},
				"emissiveFactor": [0.1, 0.05, 0.02]
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
				"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAIA/"
			}]
		})";

		const auto scene = loader.loadFromString( completeGltf );

		REQUIRE( scene != nullptr );
		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );
		REQUIRE( rootNodes[0]->hasMesh() );

		const auto meshPtr = rootNodes[0]->getFirstMesh();
		REQUIRE( meshPtr != nullptr );
		const auto &primitive = meshPtr->getPrimitive( 0 );
		REQUIRE( primitive.hasMaterial() );

		// For now, just verify the material path was set
		// TODO: When AssetManager is implemented, we should be able to retrieve and validate the actual material data
		REQUIRE( !primitive.getMaterialPath().empty() );
	}
}

// Tests for transform extraction from glTF nodes
TEST_CASE( "GLTFLoader Transform Extraction", "[gltf][loader][transform]" )
{
	SECTION( "Extract TRS from glTF node with translation, rotation, scale" )
	{
		const gltf_loader::GLTFLoader loader;

		const std::string gltfWithTransforms = R"({
			"asset": { "version": "2.0" },
			"scene": 0,
			"scenes": [{ "nodes": [0] }],
			"nodes": [{
				"name": "TransformedNode",
				"translation": [1.0, 2.0, 3.0],
				"rotation": [0.0, 0.0, 0.7071068, 0.7071068],
				"scale": [2.0, 1.0, 0.5],
				"mesh": 0
			}],
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
			"bufferViews": [{ "buffer": 0, "byteOffset": 0, "byteLength": 36 }],
			"buffers": [{
				"byteLength": 36,
				"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAIA/"
			}]
		})";

		const auto scene = loader.loadFromString( gltfWithTransforms );
		REQUIRE( scene != nullptr );

		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );

		const auto &node = rootNodes[0];
		REQUIRE( node->name == "TransformedNode" );

		// Check that the node now has transform data
		// This test will fail initially since SceneNode doesn't store transform data yet
		REQUIRE( node->hasTransform() );
		const auto &transform = node->getTransform();

		// Verify translation
		REQUIRE( transform.position.x == 1.0f );
		REQUIRE( transform.position.y == 2.0f );
		REQUIRE( transform.position.z == 3.0f );

		// Verify scale
		REQUIRE( transform.scale.x == 2.0f );
		REQUIRE( transform.scale.y == 1.0f );
		REQUIRE( transform.scale.z == 0.5f );

		// Verify rotation (quaternion [0, 0, 0.7071068, 0.7071068] should convert to ~90 degrees around Z)
		// This is approximately 90 degrees (π/2 radians) around Z-axis
		REQUIRE( std::abs( transform.rotation.x ) < 0.001f );
		REQUIRE( std::abs( transform.rotation.y ) < 0.001f );
		REQUIRE( std::abs( transform.rotation.z - 1.5708f ) < 0.01f ); // π/2 ≈ 1.5708
	}

	SECTION( "Extract transform from glTF node with matrix" )
	{
		const gltf_loader::GLTFLoader loader;

		// Matrix representing translation (2, 3, 4) and scale (1.5, 1.5, 1.5)
		const std::string gltfWithMatrix = R"({
			"asset": { "version": "2.0" },
			"scene": 0,
			"scenes": [{ "nodes": [0] }],
			"nodes": [{
				"name": "MatrixNode",
				"matrix": [
					1.5, 0.0, 0.0, 0.0,
					0.0, 1.5, 0.0, 0.0,
					0.0, 0.0, 1.5, 0.0,
					2.0, 3.0, 4.0, 1.0
				],
				"mesh": 0
			}],
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
			"bufferViews": [{ "buffer": 0, "byteOffset": 0, "byteLength": 36 }],
			"buffers": [{
				"byteLength": 36,
				"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAIA/"
			}]
		})";

		const auto scene = loader.loadFromString( gltfWithMatrix );
		REQUIRE( scene != nullptr );

		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );

		const auto &node = rootNodes[0];
		REQUIRE( node->name == "MatrixNode" );

		// Check transform extracted from matrix
		REQUIRE( node->hasTransform() );
		const auto &transform = node->getTransform();

		// Translation should be extracted from the last column
		REQUIRE( std::abs( transform.position.x - 2.0f ) < 0.001f );
		REQUIRE( std::abs( transform.position.y - 3.0f ) < 0.001f );
		REQUIRE( std::abs( transform.position.z - 4.0f ) < 0.001f );

		// Scale should be extracted from diagonal (approximately 1.5)
		REQUIRE( std::abs( transform.scale.x - 1.5f ) < 0.001f );
		REQUIRE( std::abs( transform.scale.y - 1.5f ) < 0.001f );
		REQUIRE( std::abs( transform.scale.z - 1.5f ) < 0.001f );
	}

	SECTION( "Default transform for node without TRS or matrix" )
	{
		const gltf_loader::GLTFLoader loader;

		const std::string gltfWithoutTransforms = R"({
			"asset": { "version": "2.0" },
			"scene": 0,
			"scenes": [{ "nodes": [0] }],
			"nodes": [{
				"name": "DefaultNode",
				"mesh": 0
			}],
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
			"bufferViews": [{ "buffer": 0, "byteOffset": 0, "byteLength": 36 }],
			"buffers": [{
				"byteLength": 36,
				"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAIA/"
			}]
		})";

		const auto scene = loader.loadFromString( gltfWithoutTransforms );
		REQUIRE( scene != nullptr );

		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( !rootNodes.empty() );

		const auto &node = rootNodes[0];
		REQUIRE( node->name == "DefaultNode" );

		// Should have default identity transform
		REQUIRE( node->hasTransform() );
		const auto &transform = node->getTransform();

		// Default values: position (0,0,0), rotation (0,0,0), scale (1,1,1)
		REQUIRE( transform.position.x == 0.0f );
		REQUIRE( transform.position.y == 0.0f );
		REQUIRE( transform.position.z == 0.0f );

		REQUIRE( transform.rotation.x == 0.0f );
		REQUIRE( transform.rotation.y == 0.0f );
		REQUIRE( transform.rotation.z == 0.0f );

		REQUIRE( transform.scale.x == 1.0f );
		REQUIRE( transform.scale.y == 1.0f );
		REQUIRE( transform.scale.z == 1.0f );
	}
}
