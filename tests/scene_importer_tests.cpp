#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <memory>

#include "runtime/ecs.h"
#include "runtime/entity.h"
#include "runtime/components.h"
#include "runtime/scene_importer.h"
#include "engine/assets/assets.h"
#include "graphics/gpu/gpu_resource_manager.h"
#include "platform/dx12/dx12_device.h"

using Catch::Approx;
using runtime::SceneImporter;

// Note: Not using "using namespace components" or "using namespace assets" to avoid Transform type ambiguity
using Catch::Approx;

TEST_CASE( "SceneImporter imports scene with single mesh node", "[scene_importer][basic]" )
{
	// Create a scene with a single node containing a mesh
	auto scene = std::make_shared<assets::Scene>();
	scene->setPath( "test_scene.gltf" );
	scene->setLoaded( true );

	// Create root node with mesh
	auto rootNode = std::make_unique<assets::SceneNode>( "TestNode" );

	// Create a simple mesh
	auto mesh = std::make_shared<assets::Mesh>();
	const auto meshHandle = scene->addMesh( mesh );
	rootNode->addMeshHandle( meshHandle );

	// Set transform
	assets::Transform transform;
	transform.position = { 1.0f, 2.0f, 3.0f };
	transform.rotation = { 0.1f, 0.2f, 0.3f };
	transform.scale = { 2.0f, 2.0f, 2.0f };
	rootNode->setTransform( transform );

	scene->addRootNode( std::move( rootNode ) );

	// Import scene using runtime::SceneImporter
	ecs::Scene ecsScene;
	const bool result = runtime::SceneImporter::importScene( scene, ecsScene );

	REQUIRE( result );

	// Verify entity was created
	const auto entities = ecsScene.getAllEntities();
	REQUIRE( entities.size() == 1 );

	const ecs::Entity entity = entities[0];
	REQUIRE( ecsScene.isValid( entity ) );

	// Check Name component
	const auto *nameComp = ecsScene.getComponent<components::Name>( entity );
	REQUIRE( nameComp != nullptr );
	REQUIRE( nameComp->name == "TestNode" );

	// Check Transform component
	const auto *transformComp = ecsScene.getComponent<components::Transform>( entity );
	REQUIRE( transformComp != nullptr );
	REQUIRE( transformComp->position.x == Catch::Approx( 1.0f ) );
	REQUIRE( transformComp->position.y == Catch::Approx( 2.0f ) );
	REQUIRE( transformComp->position.z == Catch::Approx( 3.0f ) );
	REQUIRE( transformComp->rotation.x == Catch::Approx( 0.1f ) );
	REQUIRE( transformComp->rotation.y == Catch::Approx( 0.2f ) );
	REQUIRE( transformComp->rotation.z == Catch::Approx( 0.3f ) );
	REQUIRE( transformComp->scale.x == Catch::Approx( 2.0f ) );
	REQUIRE( transformComp->scale.y == Catch::Approx( 2.0f ) );
	REQUIRE( transformComp->scale.z == Catch::Approx( 2.0f ) );

	// Check MeshRenderer component
	const auto *rendererComp = ecsScene.getComponent<components::MeshRenderer>( entity );
	REQUIRE( rendererComp != nullptr );
}

TEST_CASE( "SceneImporter preserves hierarchy correctly", "[scene_importer][hierarchy]" )
{
	// Create a scene with parent-child hierarchy
	auto scene = std::make_shared<assets::Scene>();
	scene->setPath( "hierarchy_scene.gltf" );
	scene->setLoaded( true );

	// Create parent node
	auto parentNode = std::make_unique<assets::SceneNode>( "ParentNode" );
	assets::Transform parentTransform;
	parentTransform.position = { 0.0f, 0.0f, 0.0f };
	parentNode->setTransform( parentTransform );

	// Create child node
	auto childNode = std::make_unique<assets::SceneNode>( "ChildNode" );
	assets::Transform childTransform;
	childTransform.position = { 1.0f, 0.0f, 0.0f };
	childNode->setTransform( childTransform );

	// Add child to parent
	parentNode->addChild( std::move( childNode ) );
	scene->addRootNode( std::move( parentNode ) );

	// Import scene
	ecs::Scene ecsScene;
	const bool result = runtime::SceneImporter::importScene( scene, ecsScene );

	REQUIRE( result );

	// Verify hierarchy was created correctly
	const auto entities = ecsScene.getAllEntities();
	REQUIRE( entities.size() == 2 ); // Parent + child

	// Find parent and child entities
	ecs::Entity parentEntity{}, childEntity{};
	for ( const auto entity : entities )
	{
		if ( !entity.isValid() )
			continue;

		const auto *name = ecsScene.getComponent<components::Name>( entity );
		if ( name && name->name == "ParentNode" )
		{
			parentEntity = entity;
		}
		else if ( name && name->name == "ChildNode" )
		{
			childEntity = entity;
		}
	}

	REQUIRE( parentEntity.isValid() );
	REQUIRE( childEntity.isValid() );

	// Verify parent-child relationship
	const ecs::Entity actualParent = ecsScene.getParent( childEntity );
	REQUIRE( actualParent == parentEntity );

	const auto children = ecsScene.getChildren( parentEntity );
	REQUIRE( children.size() == 1 );
	REQUIRE( children[0] == childEntity );
}

TEST_CASE( "SceneImporter handles nodes without meshes", "[scene_importer][empty_nodes]" )
{
	// Create a scene with a node that has no mesh data
	auto scene = std::make_shared<assets::Scene>();
	scene->setPath( "empty_node_scene.gltf" );
	scene->setLoaded( true );

	auto emptyNode = std::make_unique<assets::SceneNode>( "EmptyNode" );
	assets::Transform transform;
	transform.position = { 5.0f, 6.0f, 7.0f };
	emptyNode->setTransform( transform );
	// Note: no mesh added to this node

	scene->addRootNode( std::move( emptyNode ) );

	// Import scene
	ecs::Scene ecsScene;
	const bool result = runtime::SceneImporter::importScene( scene, ecsScene );

	REQUIRE( result );

	// Verify entity was created but without MeshRenderer
	const auto entities = ecsScene.getAllEntities();
	REQUIRE( entities.size() == 1 );

	const ecs::Entity entity = entities[0];
	REQUIRE( ecsScene.isValid( entity ) );

	// Should have Name and Transform but not MeshRenderer
	REQUIRE( ecsScene.hasComponent<components::Name>( entity ) );
	REQUIRE( ecsScene.hasComponent<components::Transform>( entity ) );
	REQUIRE_FALSE( ecsScene.hasComponent<components::MeshRenderer>( entity ) );
}

TEST_CASE( "SceneImporter GPU and non-GPU paths produce identical results", "[scene_importer][gpu_comparison]" )
{
	// Create a test scene
	auto scene = std::make_shared<assets::Scene>();
	scene->setPath( "comparison_scene.gltf" );
	scene->setLoaded( true );

	// Create a node with transform and mesh
	auto node = std::make_unique<assets::SceneNode>( "ComparisonNode" );
	assets::Transform transform;
	transform.position = { 10.0f, 20.0f, 30.0f };
	transform.rotation = { 0.5f, 1.0f, 1.5f };
	transform.scale = { 0.5f, 1.5f, 2.0f };
	node->setTransform( transform );

	auto mesh = std::make_shared<assets::Mesh>();
	const auto meshHandle = scene->addMesh( mesh );
	node->addMeshHandle( meshHandle );

	scene->addRootNode( std::move( node ) );

	// Import using non-GPU path
	ecs::Scene nonGpuScene;
	const bool nonGpuResult = runtime::SceneImporter::importScene( scene, nonGpuScene );

	// Import using CPU-only path
	ecs::Scene gpuScene;
	// Create a dummy device and manager for GPU path test
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );
	graphics::GPUResourceManager resourceManager( device );
	const bool gpuResult = runtime::SceneImporter::importScene( scene, gpuScene );
	REQUIRE( gpuResult );

	// Create GPU resources separately
	const bool gpuResourceResult = runtime::SceneImporter::createGPUResources( scene, gpuScene, resourceManager );
	REQUIRE( gpuResourceResult );

	// Both should succeed
	REQUIRE( nonGpuResult );
	REQUIRE( gpuResult );

	// Both scenes should have the same number of entities
	const auto nonGpuEntities = nonGpuScene.getAllEntities();
	const auto gpuEntities = gpuScene.getAllEntities();
	REQUIRE( nonGpuEntities.size() == gpuEntities.size() );
	REQUIRE( nonGpuEntities.size() == 1 );

	// Compare components (they should be identical for now since GPU path calls non-GPU path)
	const ecs::Entity nonGpuEntity = nonGpuEntities[0];
	const ecs::Entity gpuEntity = gpuEntities[0];

	// Compare Name components
	const auto *nonGpuName = nonGpuScene.getComponent<components::Name>( nonGpuEntity );
	const auto *gpuName = gpuScene.getComponent<components::Name>( gpuEntity );
	REQUIRE( nonGpuName != nullptr );
	REQUIRE( gpuName != nullptr );
	REQUIRE( nonGpuName->name == gpuName->name );

	// Compare Transform components
	const auto *nonGpuTransform = nonGpuScene.getComponent<components::Transform>( nonGpuEntity );
	const auto *gpuTransform = gpuScene.getComponent<components::Transform>( gpuEntity );
	REQUIRE( nonGpuTransform != nullptr );
	REQUIRE( gpuTransform != nullptr );
	REQUIRE( nonGpuTransform->position.x == Catch::Approx( gpuTransform->position.x ) );
	REQUIRE( nonGpuTransform->position.y == Catch::Approx( gpuTransform->position.y ) );
	REQUIRE( nonGpuTransform->position.z == Catch::Approx( gpuTransform->position.z ) );

	// Compare MeshRenderer components
	const auto *nonGpuRenderer = nonGpuScene.getComponent<components::MeshRenderer>( nonGpuEntity );
	const auto *gpuRenderer = gpuScene.getComponent<components::MeshRenderer>( gpuEntity );
	REQUIRE( nonGpuRenderer != nullptr );
	REQUIRE( gpuRenderer != nullptr );
}

TEST_CASE( "SceneImporter handles invalid scene gracefully", "[scene_importer][error_handling]" )
{
	ecs::Scene ecsScene;

	// Test with null scene
	REQUIRE_FALSE( runtime::SceneImporter::importScene( nullptr, ecsScene ) );

	// Test with unloaded scene
	auto unloadedScene = std::make_shared<assets::Scene>();
	unloadedScene->setPath( "unloaded.gltf" );
	unloadedScene->setLoaded( false );
	REQUIRE_FALSE( runtime::SceneImporter::importScene( unloadedScene, ecsScene ) );

	// ecs::Scene should remain empty after failed imports
	const auto entities = ecsScene.getAllEntities();
	REQUIRE( entities.empty() );
}

TEST_CASE( "SceneImporter sets MeshRenderer bounds from mesh with single primitive", "[scene_importer][bounds]" )
{
	// Create a scene with a mesh that has bounds data
	auto scene = std::make_shared<assets::Scene>();
	scene->setPath( "bounds_test.gltf" );
	scene->setLoaded( true );

	// Create a mesh with a primitive containing vertices
	auto mesh = std::make_shared<assets::Mesh>();

	// Create primitive with vertices that define a specific bounding box
	assets::Primitive primitive;
	primitive.addVertex( { { -2.0f, -3.0f, -4.0f }, {}, {} } ); // Min corner
	primitive.addVertex( { { 5.0f, 7.0f, 9.0f }, {}, {} } );	// Max corner
	primitive.addVertex( { { 1.0f, 2.0f, 3.0f }, {}, {} } );	// Interior point

	// Add primitive to mesh (this should trigger bounds calculation)
	mesh->addPrimitive( std::move( primitive ) );

	// Verify mesh has bounds
	REQUIRE( mesh->hasBounds() );

	// Add mesh to scene
	const auto meshHandle = scene->addMesh( mesh );

	// Create a node with this mesh
	auto rootNode = std::make_unique<assets::SceneNode>( "BoundsTestNode" );
	rootNode->addMeshHandle( meshHandle );
	scene->addRootNode( std::move( rootNode ) );

	// Import the scene
	ecs::Scene ecsScene;
	const bool result = runtime::SceneImporter::importScene( scene, ecsScene );
	REQUIRE( result );

	// Verify that an entity was created with bounds
	const auto entities = ecsScene.getAllEntities();
	REQUIRE( entities.size() == 1 );

	const ecs::Entity entity = entities[0];
	REQUIRE( ecsScene.hasComponent<components::MeshRenderer>( entity ) );

	const auto *rendererComp = ecsScene.getComponent<components::MeshRenderer>( entity );
	REQUIRE( rendererComp != nullptr );

	// Verify bounds were set and are valid
	REQUIRE( rendererComp->bounds.isValid() );

	// Verify bounds encompass our test vertices
	// Min should be <= (-2, -3, -4) and Max should be >= (5, 7, 9)
	REQUIRE( rendererComp->bounds.min.x <= -2.0f );
	REQUIRE( rendererComp->bounds.min.y <= -3.0f );
	REQUIRE( rendererComp->bounds.min.z <= -4.0f );
	REQUIRE( rendererComp->bounds.max.x >= 5.0f );
	REQUIRE( rendererComp->bounds.max.y >= 7.0f );
	REQUIRE( rendererComp->bounds.max.z >= 9.0f );

	// Verify the bounds match the mesh bounds
	const auto meshBounds = mesh->getBounds();
	REQUIRE( rendererComp->bounds.min.x == Catch::Approx( meshBounds.min.x ) );
	REQUIRE( rendererComp->bounds.min.y == Catch::Approx( meshBounds.min.y ) );
	REQUIRE( rendererComp->bounds.min.z == Catch::Approx( meshBounds.min.z ) );
	REQUIRE( rendererComp->bounds.max.x == Catch::Approx( meshBounds.max.x ) );
	REQUIRE( rendererComp->bounds.max.y == Catch::Approx( meshBounds.max.y ) );
	REQUIRE( rendererComp->bounds.max.z == Catch::Approx( meshBounds.max.z ) );
}

TEST_CASE( "SceneImporter sets MeshRenderer bounds from mesh with multiple primitives", "[scene_importer][bounds]" )
{
	// Create a scene with a mesh containing multiple primitives
	auto scene = std::make_shared<assets::Scene>();
	scene->setPath( "multi_primitive_bounds_test.gltf" );
	scene->setLoaded( true );

	auto mesh = std::make_shared<assets::Mesh>();

	// First primitive - extends from (-10, -5, -1) to (0, 0, 0)
	assets::Primitive primitive1;
	primitive1.addVertex( { { -10.0f, -5.0f, -1.0f }, {}, {} } );
	primitive1.addVertex( { { 0.0f, 0.0f, 0.0f }, {}, {} } );
	mesh->addPrimitive( std::move( primitive1 ) );

	// Second primitive - extends from (0, 0, 0) to (8, 12, 6)
	assets::Primitive primitive2;
	primitive2.addVertex( { { 0.0f, 0.0f, 0.0f }, {}, {} } );
	primitive2.addVertex( { { 8.0f, 12.0f, 6.0f }, {}, {} } );
	mesh->addPrimitive( std::move( primitive2 ) );

	// Combined bounds should be (-10, -5, -1) to (8, 12, 6)
	REQUIRE( mesh->hasBounds() );

	const auto meshHandle = scene->addMesh( mesh );
	auto rootNode = std::make_unique<assets::SceneNode>( "MultiPrimitiveNode" );
	rootNode->addMeshHandle( meshHandle );
	scene->addRootNode( std::move( rootNode ) );

	// Import scene
	ecs::Scene ecsScene;
	const bool result = SceneImporter::importScene( scene, ecsScene );
	REQUIRE( result );

	const auto entities = ecsScene.getAllEntities();
	REQUIRE( entities.size() == 1 );

	const ecs::Entity entity = entities[0];
	REQUIRE( ecsScene.hasComponent<components::MeshRenderer>( entity ) );

	const auto *rendererComp = ecsScene.getComponent<components::MeshRenderer>( entity );
	REQUIRE( rendererComp != nullptr );

	// Verify bounds encompass all vertices
	REQUIRE( rendererComp->bounds.isValid() );

	// Check all expected vertices are within bounds
	REQUIRE( rendererComp->bounds.min.x <= -10.0f );
	REQUIRE( rendererComp->bounds.min.y <= -5.0f );
	REQUIRE( rendererComp->bounds.min.z <= -1.0f );
	REQUIRE( rendererComp->bounds.max.x >= 8.0f );
	REQUIRE( rendererComp->bounds.max.y >= 12.0f );
	REQUIRE( rendererComp->bounds.max.z >= 6.0f );
}

TEST_CASE( "SceneImporter handles mesh without bounds gracefully", "[scene_importer][bounds]" )
{
	// Create a scene with an empty mesh (no primitives/vertices)
	auto scene = std::make_shared<assets::Scene>();
	scene->setPath( "empty_mesh_test.gltf" );
	scene->setLoaded( true );

	// Create an empty mesh
	auto mesh = std::make_shared<assets::Mesh>();
	// Don't add any primitives - mesh should have no bounds
	REQUIRE_FALSE( mesh->hasBounds() );

	const auto meshHandle = scene->addMesh( mesh );
	auto rootNode = std::make_unique<assets::SceneNode>( "EmptyMeshNode" );
	rootNode->addMeshHandle( meshHandle );
	scene->addRootNode( std::move( rootNode ) );

	// Import scene
	ecs::Scene ecsScene;
	const bool result = SceneImporter::importScene( scene, ecsScene );
	REQUIRE( result );

	const auto entities = ecsScene.getAllEntities();
	REQUIRE( entities.size() == 1 );

	const ecs::Entity entity = entities[0];
	REQUIRE( ecsScene.hasComponent<components::MeshRenderer>( entity ) );

	const auto *rendererComp = ecsScene.getComponent<components::MeshRenderer>( entity );
	REQUIRE( rendererComp != nullptr );

	// MeshRenderer should exist but bounds should be invalid (default state)
	REQUIRE_FALSE( rendererComp->bounds.isValid() );
}

TEST_CASE( "SceneImporter bounds calculation matches mesh getBoundsCenter and getBoundsSize", "[scene_importer][bounds]" )
{
	// Test that bounds are correctly calculated using center and size approach
	auto scene = std::make_shared<assets::Scene>();
	scene->setPath( "center_size_bounds_test.gltf" );
	scene->setLoaded( true );

	auto mesh = std::make_shared<assets::Mesh>();

	// Create a primitive with vertices that form a known bounding box
	// Center at (1, 2, 3) with size (4, 6, 8) means:
	// Min = (1, 2, 3) - (2, 3, 4) = (-1, -1, -1)
	// Max = (1, 2, 3) + (2, 3, 4) = (3, 5, 7)
	assets::Primitive primitive;
	primitive.addVertex( { { -1.0f, -1.0f, -1.0f }, {}, {} } ); // Min corner
	primitive.addVertex( { { 3.0f, 5.0f, 7.0f }, {}, {} } );	// Max corner
	primitive.addVertex( { { 1.0f, 2.0f, 3.0f }, {}, {} } );	// Center point
	mesh->addPrimitive( std::move( primitive ) );

	// Verify mesh bounds center and size calculations
	REQUIRE( mesh->hasBounds() );
	const auto boundsCenter = mesh->getBounds().center();
	const auto boundsSize = mesh->getBounds().size();

	// Center should be (1, 2, 3)
	REQUIRE( boundsCenter.x == Catch::Approx( 1.0f ) );
	REQUIRE( boundsCenter.y == Catch::Approx( 2.0f ) );
	REQUIRE( boundsCenter.z == Catch::Approx( 3.0f ) );

	// Size should be (4, 6, 8)
	REQUIRE( boundsSize.x == Catch::Approx( 4.0f ) );
	REQUIRE( boundsSize.y == Catch::Approx( 6.0f ) );
	REQUIRE( boundsSize.z == Catch::Approx( 8.0f ) );

	const auto meshHandle = scene->addMesh( mesh );
	auto rootNode = std::make_unique<assets::SceneNode>( "CenterSizeNode" );
	rootNode->addMeshHandle( meshHandle );
	scene->addRootNode( std::move( rootNode ) );

	// Import scene
	ecs::Scene ecsScene;
	const bool result = SceneImporter::importScene( scene, ecsScene );
	REQUIRE( result );

	const auto entities = ecsScene.getAllEntities();
	REQUIRE( entities.size() == 1 );

	const ecs::Entity entity = entities[0];
	const auto *rendererComp = ecsScene.getComponent<components::MeshRenderer>( entity );
	REQUIRE( rendererComp != nullptr );

	// Verify MeshRenderer bounds match the center/size calculation
	// rendererComp->bounds should be calculated as: center ± size/2
	const auto expectedMin = boundsCenter - boundsSize * 0.5f;
	const auto expectedMax = boundsCenter + boundsSize * 0.5f;

	REQUIRE( rendererComp->bounds.min.x == Catch::Approx( expectedMin.x ) );
	REQUIRE( rendererComp->bounds.min.y == Catch::Approx( expectedMin.y ) );
	REQUIRE( rendererComp->bounds.min.z == Catch::Approx( expectedMin.z ) );
	REQUIRE( rendererComp->bounds.max.x == Catch::Approx( expectedMax.x ) );
	REQUIRE( rendererComp->bounds.max.y == Catch::Approx( expectedMax.y ) );
	REQUIRE( rendererComp->bounds.max.z == Catch::Approx( expectedMax.z ) );
}

TEST_CASE( "SceneImporter populates meshPath from asset scene path", "[scene_importer][meshpath][AF1]" )
{
	// Arrange: Create an asset scene with a known path
	auto assetScene = std::make_shared<assets::Scene>();
	assetScene->setPath( "assets/models/imported_model.gltf" );
	assetScene->setLoaded( true );

	// Create a mesh and add it to the scene
	auto mesh = std::make_shared<assets::Mesh>();
	mesh->setPath( "assets/models/imported_model.gltf" ); // Mesh uses same path as scene
	const auto meshHandle = assetScene->addMesh( mesh );

	// Create a node with this mesh
	auto rootNode = std::make_unique<assets::SceneNode>( "ImportedNode" );
	rootNode->addMeshHandle( meshHandle );
	assetScene->addRootNode( std::move( rootNode ) );

	// Act: Import the scene
	ecs::Scene ecsScene;
	const bool result = SceneImporter::importScene( assetScene, ecsScene );

	// Assert: Import succeeded
	REQUIRE( result );

	// Verify the imported entity has meshPath set
	const auto entities = ecsScene.getAllEntities();
	REQUIRE( entities.size() == 1 );

	const ecs::Entity entity = entities[0];
	REQUIRE( ecsScene.hasComponent<components::MeshRenderer>( entity ) );

	const auto *meshRenderer = ecsScene.getComponent<components::MeshRenderer>( entity );
	REQUIRE( meshRenderer != nullptr );
	REQUIRE( meshRenderer->meshHandle == meshHandle );

	// Key assertion: meshPath should be populated with the asset path
	REQUIRE( meshRenderer->meshPath == "assets/models/imported_model.gltf" );
}

TEST_CASE( "SceneImporter uses scene path as fallback when mesh has no path", "[scene_importer][meshpath][AF2]" )
{
	// Arrange: Create an asset scene with path
	auto assetScene = std::make_shared<assets::Scene>();
	assetScene->setPath( "assets/models/fallback_scene.gltf" );
	assetScene->setLoaded( true );

	// Create a mesh WITHOUT setting its path
	auto mesh = std::make_shared<assets::Mesh>();
	// mesh->setPath() is NOT called - path will be empty
	const auto meshHandle = assetScene->addMesh( mesh );

	// Create a node with this mesh
	auto rootNode = std::make_unique<assets::SceneNode>( "FallbackNode" );
	rootNode->addMeshHandle( meshHandle );
	assetScene->addRootNode( std::move( rootNode ) );

	// Act: Import the scene
	ecs::Scene ecsScene;
	const bool result = SceneImporter::importScene( assetScene, ecsScene );

	// Assert: Import succeeded
	REQUIRE( result );

	const auto entities = ecsScene.getAllEntities();
	REQUIRE( entities.size() == 1 );

	const ecs::Entity entity = entities[0];
	const auto *meshRenderer = ecsScene.getComponent<components::MeshRenderer>( entity );
	REQUIRE( meshRenderer != nullptr );

	// When mesh has no path, should fall back to scene path
	REQUIRE( meshRenderer->meshPath == "assets/models/fallback_scene.gltf" );
}
