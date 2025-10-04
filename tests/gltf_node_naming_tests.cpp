#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>

#include "engine/assets/assets.h"
#include "engine/gltf_loader/gltf_loader.h"

// Tests for GLTF node naming priority
// Priority: 1) node name, 2) filename (root nodes), 3) mesh name, 4) "UnnamedNode"

TEST_CASE( "GLTF Node Naming Priority", "[gltf][loader][name][unit]" )
{
	gltf_loader::GLTFLoader loader;

	SECTION( "Node with explicit name uses node name (highest priority)" )
	{
		const std::string gltfContent = R"({
			"asset": { "version": "2.0" },
			"scene": 0,
			"scenes": [{ "nodes": [0] }],
			"nodes": [{
				"name": "ExplicitNodeName",
				"mesh": 0
			}],
			"meshes": [{
				"name": "MeshName",
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

		const auto scene = loader.loadFromString( gltfContent );
		REQUIRE( scene != nullptr );

		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( rootNodes.size() == 1 );

		// Should use explicit node name, not mesh name
		REQUIRE( rootNodes[0]->getName() == "ExplicitNodeName" );
	}

	SECTION( "Root node without name but with mesh name uses mesh name" )
	{
		const std::string gltfContent = R"({
			"asset": { "version": "2.0" },
			"scene": 0,
			"scenes": [{ "nodes": [0] }],
			"nodes": [{
				"mesh": 0
			}],
			"meshes": [{
				"name": "MyMeshName",
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

		const auto scene = loader.loadFromString( gltfContent );
		REQUIRE( scene != nullptr );

		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( rootNodes.size() == 1 );

		// loadFromString doesn't have filename, so should use mesh name
		REQUIRE( rootNodes[0]->getName() == "MyMeshName" );
	}

	SECTION( "Node without name or mesh defaults to UnnamedNode" )
	{
		const std::string gltfContent = R"({
			"asset": { "version": "2.0" },
			"scene": 0,
			"scenes": [{ "nodes": [0] }],
			"nodes": [{}]
		})";

		const auto scene = loader.loadFromString( gltfContent );
		REQUIRE( scene != nullptr );

		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( rootNodes.size() == 1 );

		// No name, no mesh -> UnnamedNode
		REQUIRE( rootNodes[0]->getName() == "UnnamedNode" );
	}

	SECTION( "Child node without name uses mesh name, not filename" )
	{
		const std::string gltfContent = R"({
			"asset": { "version": "2.0" },
			"scene": 0,
			"scenes": [{ "nodes": [0] }],
			"nodes": [
				{
					"name": "ParentNode",
					"children": [1]
				},
				{
					"mesh": 0
				}
			],
			"meshes": [{
				"name": "ChildMeshName",
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

		const auto scene = loader.loadFromString( gltfContent );
		REQUIRE( scene != nullptr );

		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( rootNodes.size() == 1 );
		REQUIRE( rootNodes[0]->getName() == "ParentNode" );

		// Child should use mesh name (not filename, which only applies to root nodes)
		REQUIRE( rootNodes[0]->getChildCount() == 1 );
		REQUIRE( rootNodes[0]->getChild( 0 ).getName() == "ChildMeshName" );
	}

	SECTION( "Multiple root nodes with different naming sources" )
	{
		const std::string gltfContent = R"({
			"asset": { "version": "2.0" },
			"scene": 0,
			"scenes": [{ "nodes": [0, 1, 2] }],
			"nodes": [
				{
					"name": "NamedNode"
				},
				{
					"mesh": 0
				},
				{}
			],
			"meshes": [{
				"name": "MeshWithName",
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

		const auto scene = loader.loadFromString( gltfContent );
		REQUIRE( scene != nullptr );

		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( rootNodes.size() == 3 );

		// First node has explicit name
		REQUIRE( rootNodes[0]->getName() == "NamedNode" );

		// Second node uses mesh name
		REQUIRE( rootNodes[1]->getName() == "MeshWithName" );

		// Third node has no name or mesh
		REQUIRE( rootNodes[2]->getName() == "UnnamedNode" );
	}

	SECTION( "Unnamed mesh still triggers UnnamedNode fallback" )
	{
		const std::string gltfContent = R"({
			"asset": { "version": "2.0" },
			"scene": 0,
			"scenes": [{ "nodes": [0] }],
			"nodes": [{
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

		const auto scene = loader.loadFromString( gltfContent );
		REQUIRE( scene != nullptr );

		const auto &rootNodes = scene->getRootNodes();
		REQUIRE( rootNodes.size() == 1 );

		// Mesh has no name, so should fall back to UnnamedNode
		REQUIRE( rootNodes[0]->getName() == "UnnamedNode" );
	}
}

// NOTE: File-based integration tests for filename extraction would require
// full GLTF loader infrastructure and are validated through manual testing
// and existing integration test suites. The unit tests above verify the
// node naming priority logic in isolation without file I/O dependencies.
