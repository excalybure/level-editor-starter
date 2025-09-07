#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>

import engine.assets;
import engine.bounding_box_3d;

TEST_CASE( "Asset System Tests", "[assets]" )
{

	SECTION( "AssetType enum values" )
	{
		REQUIRE( static_cast<int>( assets::AssetType::Unknown ) == 0 );
		REQUIRE( static_cast<int>( assets::AssetType::Mesh ) == 1 );
		REQUIRE( static_cast<int>( assets::AssetType::Material ) == 2 );
		REQUIRE( static_cast<int>( assets::AssetType::Texture ) == 3 );
		REQUIRE( static_cast<int>( assets::AssetType::Scene ) == 4 );
	}

	SECTION( "Vertex default values" )
	{
		const assets::Vertex vertex{};

		// Check position default (0, 0, 0)
		REQUIRE( vertex.position.x == 0.0f );
		REQUIRE( vertex.position.y == 0.0f );
		REQUIRE( vertex.position.z == 0.0f );

		// Check normal default (0, 1, 0)
		REQUIRE( vertex.normal.x == 0.0f );
		REQUIRE( vertex.normal.y == 1.0f );
		REQUIRE( vertex.normal.z == 0.0f );

		// Check texture coordinate default (0, 0)
		REQUIRE( vertex.texCoord.x == 0.0f );
		REQUIRE( vertex.texCoord.y == 0.0f );

		// Check tangent default (1, 0, 0, 1)
		REQUIRE( vertex.tangent.x == 1.0f );
		REQUIRE( vertex.tangent.y == 0.0f );
		REQUIRE( vertex.tangent.z == 0.0f );
		REQUIRE( vertex.tangent.w == 1.0f );
	}

	SECTION( "Vertex custom values" )
	{
		assets::Vertex vertex{};
		vertex.position.x = 1.0f;
		vertex.position.y = 2.0f;
		vertex.position.z = 3.0f;

		vertex.normal.x = 0.5f;
		vertex.normal.y = 0.5f;
		vertex.normal.z = 0.707f;

		vertex.texCoord.x = 0.25f;
		vertex.texCoord.y = 0.75f;

		REQUIRE( vertex.position.x == 1.0f );
		REQUIRE( vertex.position.y == 2.0f );
		REQUIRE( vertex.position.z == 3.0f );

		REQUIRE( vertex.normal.x == 0.5f );
		REQUIRE( vertex.normal.y == 0.5f );
		REQUIRE( vertex.normal.z == 0.707f );

		REQUIRE( vertex.texCoord.x == 0.25f );
		REQUIRE( vertex.texCoord.y == 0.75f );
	}
}

TEST_CASE( "Mesh Tests", "[assets][mesh]" )
{
	auto mesh = std::make_unique<assets::Mesh>();

	SECTION( "Mesh type and initial state" )
	{
		REQUIRE( mesh->getType() == assets::AssetType::Mesh );
		REQUIRE( mesh->getVertexCount() == 0 );
		REQUIRE( mesh->getIndexCount() == 0 );
		REQUIRE( mesh->getVertices().empty() );
		REQUIRE( mesh->getIndices().empty() );
		REQUIRE_FALSE( mesh->isLoaded() );
	}

	SECTION( "Mesh accessors are const correct" )
	{
		const auto &constMesh = *mesh;

		// These should compile without issues - testing const correctness
		const auto &vertices = constMesh.getVertices();
		const auto &indices = constMesh.getIndices();
		const auto vertexCount = constMesh.getVertexCount();
		const auto indexCount = constMesh.getIndexCount();
		const auto type = constMesh.getType();
		const auto loaded = constMesh.isLoaded();

		REQUIRE( vertices.empty() );
		REQUIRE( indices.empty() );
		REQUIRE( vertexCount == 0 );
		REQUIRE( indexCount == 0 );
		REQUIRE( type == assets::AssetType::Mesh );
		REQUIRE_FALSE( loaded );
	}
}

TEST_CASE( "Material Tests", "[assets][material]" )
{
	auto material = std::make_unique<assets::Material>();

	SECTION( "Material type and initial state" )
	{
		REQUIRE( material->getType() == assets::AssetType::Material );
		REQUIRE_FALSE( material->isLoaded() );
	}

	SECTION( "PBR Material default values" )
	{
		const auto &pbr = material->getPBRMaterial();

		// Check base color factor default (1, 1, 1, 1)
		REQUIRE( pbr.baseColorFactor[0] == 1.0f );
		REQUIRE( pbr.baseColorFactor[1] == 1.0f );
		REQUIRE( pbr.baseColorFactor[2] == 1.0f );
		REQUIRE( pbr.baseColorFactor[3] == 1.0f );

		// Check metallic and roughness defaults
		REQUIRE( pbr.metallicFactor == 0.0f );
		REQUIRE( pbr.roughnessFactor == 1.0f );

		// Check emissive factor default (0, 0, 0)
		REQUIRE( pbr.emissiveFactor[0] == 0.0f );
		REQUIRE( pbr.emissiveFactor[1] == 0.0f );
		REQUIRE( pbr.emissiveFactor[2] == 0.0f );

		// Check texture paths are empty
		REQUIRE( pbr.baseColorTexture.empty() );
		REQUIRE( pbr.metallicRoughnessTexture.empty() );
		REQUIRE( pbr.normalTexture.empty() );
		REQUIRE( pbr.emissiveTexture.empty() );
	}

	SECTION( "PBR Material modifications" )
	{
		auto &pbr = material->getPBRMaterial();

		// Modify values
		pbr.baseColorFactor[0] = 0.8f;
		pbr.baseColorFactor[1] = 0.6f;
		pbr.baseColorFactor[2] = 0.4f;
		pbr.baseColorFactor[3] = 0.9f;

		pbr.metallicFactor = 0.7f;
		pbr.roughnessFactor = 0.3f;

		pbr.emissiveFactor[0] = 0.1f;
		pbr.emissiveFactor[1] = 0.2f;
		pbr.emissiveFactor[2] = 0.05f;

		pbr.baseColorTexture = "textures/base_color.png";
		pbr.normalTexture = "textures/normal.png";

		// Verify modifications
		const auto &constPbr = material->getPBRMaterial();
		REQUIRE( constPbr.baseColorFactor[0] == 0.8f );
		REQUIRE( constPbr.baseColorFactor[1] == 0.6f );
		REQUIRE( constPbr.baseColorFactor[2] == 0.4f );
		REQUIRE( constPbr.baseColorFactor[3] == 0.9f );

		REQUIRE( constPbr.metallicFactor == 0.7f );
		REQUIRE( constPbr.roughnessFactor == 0.3f );

		REQUIRE( constPbr.emissiveFactor[0] == 0.1f );
		REQUIRE( constPbr.emissiveFactor[1] == 0.2f );
		REQUIRE( constPbr.emissiveFactor[2] == 0.05f );

		REQUIRE( constPbr.baseColorTexture == "textures/base_color.png" );
		REQUIRE( constPbr.normalTexture == "textures/normal.png" );
	}

	SECTION( "Material accessors are const correct" )
	{
		const auto &constMaterial = *material;

		// These should compile without issues - testing const correctness
		const auto &constPbr = constMaterial.getPBRMaterial();
		const auto type = constMaterial.getType();
		const auto loaded = constMaterial.isLoaded();

		REQUIRE( type == assets::AssetType::Material );
		REQUIRE_FALSE( loaded );
		REQUIRE( constPbr.metallicFactor == 0.0f );
	}
}

TEST_CASE( "SceneNode Tests", "[assets][scene][node]" )
{

	SECTION( "SceneNode default construction" )
	{
		const assets::SceneNode node{};

		REQUIRE( node.name.empty() );
		REQUIRE( node.meshes.empty() );
		REQUIRE( node.materials.empty() );
		REQUIRE( node.children.empty() );

		REQUIRE_FALSE( node.hasMesh() );
		REQUIRE_FALSE( node.hasMaterial() );
		REQUIRE_FALSE( node.hasChildren() );
	}

	SECTION( "SceneNode named construction" )
	{
		const std::string nodeName = "TestNode";
		const assets::SceneNode node( nodeName );

		REQUIRE( node.name == nodeName );
		REQUIRE( node.meshes.empty() );
		REQUIRE( node.materials.empty() );
		REQUIRE( node.children.empty() );

		REQUIRE_FALSE( node.hasMesh() );
		REQUIRE_FALSE( node.hasMaterial() );
		REQUIRE_FALSE( node.hasChildren() );
	}

	SECTION( "SceneNode with content" )
	{
		assets::SceneNode node( "RootNode" );

		// Add meshes and materials
		node.meshes.push_back( "meshes/cube.mesh" );
		node.meshes.push_back( "meshes/sphere.mesh" );
		node.materials.push_back( "materials/metal.mat" );

		// Add a child node
		auto child = std::make_unique<assets::SceneNode>( "ChildNode" );
		child->meshes.push_back( "meshes/child_mesh.mesh" );
		node.children.push_back( std::move( child ) );

		REQUIRE( node.hasMesh() );
		REQUIRE( node.hasMaterial() );
		REQUIRE( node.hasChildren() );

		REQUIRE( node.meshes.size() == 2 );
		REQUIRE( node.materials.size() == 1 );
		REQUIRE( node.children.size() == 1 );

		REQUIRE( node.meshes[0] == "meshes/cube.mesh" );
		REQUIRE( node.meshes[1] == "meshes/sphere.mesh" );
		REQUIRE( node.materials[0] == "materials/metal.mat" );

		REQUIRE( node.children[0]->name == "ChildNode" );
		REQUIRE( node.children[0]->hasMesh() );
		REQUIRE_FALSE( node.children[0]->hasMaterial() );
		REQUIRE_FALSE( node.children[0]->hasChildren() );
	}
}

TEST_CASE( "Scene Tests", "[assets][scene]" )
{
	auto scene = std::make_unique<assets::Scene>();

	SECTION( "Scene type and initial state" )
	{
		REQUIRE( scene->getType() == assets::AssetType::Scene );
		REQUIRE_FALSE( scene->isLoaded() );
		REQUIRE( scene->getRootNodes().empty() );
		REQUIRE( scene->getTotalNodeCount() == 0 );
	}

	SECTION( "Scene with root nodes" )
	{
		// Add root nodes
		auto rootNode1 = std::make_unique<assets::SceneNode>( "Root1" );
		auto rootNode2 = std::make_unique<assets::SceneNode>( "Root2" );

		rootNode1->meshes.push_back( "meshes/root1.mesh" );
		rootNode2->materials.push_back( "materials/root2.mat" );

		scene->addRootNode( std::move( rootNode1 ) );
		scene->addRootNode( std::move( rootNode2 ) );

		REQUIRE( scene->getRootNodes().size() == 2 );
		REQUIRE( scene->getTotalNodeCount() == 2 );

		const auto &nodes = scene->getRootNodes();
		REQUIRE( nodes[0]->name == "Root1" );
		REQUIRE( nodes[1]->name == "Root2" );

		REQUIRE( nodes[0]->hasMesh() );
		REQUIRE_FALSE( nodes[0]->hasMaterial() );

		REQUIRE_FALSE( nodes[1]->hasMesh() );
		REQUIRE( nodes[1]->hasMaterial() );
	}

	SECTION( "Scene rejects null nodes" )
	{
		const auto initialCount = scene->getTotalNodeCount();

		scene->addRootNode( nullptr );

		REQUIRE( scene->getTotalNodeCount() == initialCount );
		REQUIRE( scene->getRootNodes().size() == initialCount );
	}

	SECTION( "Scene accessors are const correct" )
	{
		const auto &constScene = *scene;

		// These should compile without issues - testing const correctness
		const auto &constNodes = constScene.getRootNodes();
		const auto type = constScene.getType();
		const auto loaded = constScene.isLoaded();
		const auto nodeCount = constScene.getTotalNodeCount();

		REQUIRE( type == assets::AssetType::Scene );
		REQUIRE_FALSE( loaded );
		REQUIRE( constNodes.empty() );
		REQUIRE( nodeCount == 0 );
	}
}

TEST_CASE( "Asset Base Class Tests", "[assets][base]" )
{

	SECTION( "Asset path and loading state" )
	{
		auto mesh = std::make_unique<assets::Mesh>();

		// Initially, path should be empty and loaded should be false
		REQUIRE( mesh->getPath().empty() );
		REQUIRE_FALSE( mesh->isLoaded() );

		// Note: Since m_path and m_loaded are protected, we can't test setting them directly
		// In a real implementation, they would be set by the asset loading system
	}

	SECTION( "Polymorphic behavior" )
	{
		auto mesh = std::make_unique<assets::Mesh>();
		auto material = std::make_unique<assets::Material>();
		auto scene = std::make_unique<assets::Scene>();

		// Test polymorphic behavior through base class pointer
		std::vector<std::unique_ptr<assets::Asset>> assets;
		assets.push_back( std::move( mesh ) );
		assets.push_back( std::move( material ) );
		assets.push_back( std::move( scene ) );

		REQUIRE( assets.size() == 3 );
		REQUIRE( assets[0]->getType() == assets::AssetType::Mesh );
		REQUIRE( assets[1]->getType() == assets::AssetType::Material );
		REQUIRE( assets[2]->getType() == assets::AssetType::Scene );

		// All should initially be unloaded
		for ( const auto &asset : assets )
		{
			REQUIRE_FALSE( asset->isLoaded() );
			REQUIRE( asset->getPath().empty() );
		}
	}
}

TEST_CASE( "Mesh Bounds BoundingBox3D Integration", "[assets][mesh][bounds]" )
{
	auto mesh = std::make_unique<assets::Mesh>();

	SECTION( "Empty mesh has invalid bounds" )
	{
		const auto &bounds = mesh->getBounds();
		REQUIRE_FALSE( bounds.isValid() );
		REQUIRE_FALSE( mesh->hasBounds() );
	}

	SECTION( "Single vertex creates valid bounds" )
	{
		assets::Vertex vertex{};
		vertex.position = math::Vec3f{ 1.0f, 2.0f, 3.0f };

		mesh->addVertex( vertex );

		const auto &bounds = mesh->getBounds();
		REQUIRE( bounds.isValid() );
		REQUIRE( mesh->hasBounds() );
		REQUIRE( bounds.min.x == 1.0f );
		REQUIRE( bounds.min.y == 2.0f );
		REQUIRE( bounds.min.z == 3.0f );
		REQUIRE( bounds.max.x == 1.0f );
		REQUIRE( bounds.max.y == 2.0f );
		REQUIRE( bounds.max.z == 3.0f );
	}

	SECTION( "Multiple vertices expand bounds correctly" )
	{
		assets::Vertex v1{}, v2{}, v3{};
		v1.position = math::Vec3f{ -1.0f, -2.0f, -3.0f };
		v2.position = math::Vec3f{ 5.0f, 1.0f, 2.0f };
		v3.position = math::Vec3f{ 0.0f, 4.0f, -1.0f };

		mesh->addVertex( v1 );
		mesh->addVertex( v2 );
		mesh->addVertex( v3 );

		const auto &bounds = mesh->getBounds();
		REQUIRE( bounds.isValid() );
		REQUIRE( mesh->hasBounds() );

		// Min should be component-wise minimum
		REQUIRE( bounds.min.x == -1.0f );
		REQUIRE( bounds.min.y == -2.0f );
		REQUIRE( bounds.min.z == -3.0f );

		// Max should be component-wise maximum
		REQUIRE( bounds.max.x == 5.0f );
		REQUIRE( bounds.max.y == 4.0f );
		REQUIRE( bounds.max.z == 2.0f );
	}

	SECTION( "Clear vertices resets bounds to invalid" )
	{
		assets::Vertex vertex{};
		vertex.position = math::Vec3f{ 1.0f, 1.0f, 1.0f };
		mesh->addVertex( vertex );

		REQUIRE( mesh->hasBounds() );

		mesh->clearVertices();

		const auto &bounds = mesh->getBounds();
		REQUIRE_FALSE( bounds.isValid() );
		REQUIRE_FALSE( mesh->hasBounds() );
	}
}
