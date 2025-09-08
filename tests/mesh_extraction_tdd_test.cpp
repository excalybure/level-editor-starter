#include <catch2/catch_test_macros.hpp>
#include <string>

import engine.gltf_loader;

TEST_CASE( "Mesh Extraction Base Triangle", "[mesh-extraction][tdd]" )
{
	// RED: Create a failing test for basic triangle mesh extraction
	const gltf_loader::GLTFLoader loader;

	// Simple triangle with only position data - manual base64 for vertices (0,0,0), (1,0,0), (0.5,1,0)
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
            "uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAPwAAgD8AAAAA"
        }]
    })";

	auto scene = loader.loadFromString( gltfContent );

	// REQUIRE: Scene is loaded successfully
	REQUIRE( scene != nullptr );
	REQUIRE( scene->getTotalNodeCount() > 0 );

	const auto &rootNodes = scene->getRootNodes();
	REQUIRE( !rootNodes.empty() );
	REQUIRE( rootNodes[0]->hasMesh() );

	const auto meshPtr = rootNodes[0]->getFirstMesh();
	REQUIRE( meshPtr != nullptr );

	// REQUIRE: Mesh has the expected triangle data
	REQUIRE( meshPtr->getPrimitiveCount() == 1 );

	const auto &primitive = meshPtr->getPrimitive( 0 );
	REQUIRE( primitive.getVertexCount() == 3 );

	const auto &vertices = primitive.getVertices();
	REQUIRE( vertices.size() == 3 );

	// Check vertex positions: (0,0,0), (1,0,0), (0.5,1,0)
	REQUIRE( vertices[0].position.x == 0.0f );
	REQUIRE( vertices[0].position.y == 0.0f );
	REQUIRE( vertices[0].position.z == 0.0f );

	REQUIRE( vertices[1].position.x == 1.0f );
	REQUIRE( vertices[1].position.y == 0.0f );
	REQUIRE( vertices[1].position.z == 0.0f );

	REQUIRE( vertices[2].position.x == 0.5f );
	REQUIRE( vertices[2].position.y == 1.0f );
	REQUIRE( vertices[2].position.z == 0.0f );
}

TEST_CASE( "Mesh Extraction With Normals", "[mesh-extraction][tdd][normals]" )
{
	// RED: Test for extracting normals in addition to positions
	const gltf_loader::GLTFLoader loader;

	// Triangle with position and normal data - this should fail until we implement normal extraction
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
            "uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAPwAAgD8AAAAAAAAAAAAAAAAAAIA/AAAAAAAAgD8AAAAAAACAPwAAAAAAAAAA"
        }]
    })";

	auto scene = loader.loadFromString( gltfContent );

	REQUIRE( scene != nullptr );
	const auto &rootNodes = scene->getRootNodes();
	REQUIRE( !rootNodes.empty() );
	REQUIRE( rootNodes[0]->hasMesh() );

	const auto meshPtr = rootNodes[0]->getFirstMesh();
	REQUIRE( meshPtr != nullptr );
	REQUIRE( meshPtr->getPrimitiveCount() == 1 );

	const auto &primitive = meshPtr->getPrimitive( 0 );
	REQUIRE( primitive.getVertexCount() == 3 );

	const auto &vertices = primitive.getVertices();
	REQUIRE( vertices.size() == 3 );

	REQUIRE( vertices[0].normal.x == 0.0f );
	REQUIRE( vertices[0].normal.y == 0.0f );
	REQUIRE( vertices[0].normal.z == 1.0f );

	REQUIRE( vertices[1].normal.x == 0.0f );
	REQUIRE( vertices[1].normal.y == 1.0f );
	REQUIRE( vertices[1].normal.z == 0.0f );

	REQUIRE( vertices[2].normal.x == 1.0f );
	REQUIRE( vertices[2].normal.y == 0.0f );
	REQUIRE( vertices[2].normal.z == 0.0f );
}