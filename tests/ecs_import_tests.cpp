#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <memory>

import std;
import runtime.ecs;
import runtime.components;
import engine.assets;
import engine.asset_manager;

using namespace components;
using namespace assets;
using namespace ecs;
using Catch::Approx;

TEST_CASE( "ECS import creates entities for each scene node", "[ecs][import][primitive]" )
{
	// Create a scene with primitive-based mesh
	auto scene = std::make_shared<assets::Scene>();
	scene->setPath( "test_scene.gltf" );
	scene->setLoaded( true );

	// Create root node with mesh containing two primitives
	auto rootNode = std::make_unique<SceneNode>( "RootNode" );

	// Create a mesh with two primitives
	auto mesh = std::make_shared<Mesh>();

	// First primitive
	Primitive primitive1;
	primitive1.addVertex( { .position = { 0.0f, 0.0f, 0.0f } } );
	primitive1.addVertex( { .position = { 1.0f, 0.0f, 0.0f } } );
	primitive1.addVertex( { .position = { 0.0f, 1.0f, 0.0f } } );
	primitive1.setMaterialPath( "material1.mat" );

	// Second primitive
	Primitive primitive2;
	primitive2.addVertex( { .position = { 2.0f, 0.0f, 0.0f } } );
	primitive2.addVertex( { .position = { 3.0f, 0.0f, 0.0f } } );
	primitive2.addVertex( { .position = { 2.0f, 1.0f, 0.0f } } );
	primitive2.setMaterialPath( "material2.mat" );

	mesh->addPrimitive( std::move( primitive1 ) );
	mesh->addPrimitive( std::move( primitive2 ) );
	const auto meshHandle = scene->addMesh( mesh );
	rootNode->addMeshHandle( meshHandle );

	// Set transform
	assets::Transform transform;
	transform.position = { 1.0f, 2.0f, 3.0f };
	transform.rotation = { 0.1f, 0.2f, 0.3f };
	transform.scale = { 2.0f, 2.0f, 2.0f };
	rootNode->setTransform( transform );

	scene->addRootNode( std::move( rootNode ) );

	// Set up ECS scene
	ecs::Scene ecsScene;

	// Register a mock callback that should convert the asset scene to ECS
	bool callbackCalled = false;
	AssetManager::setImportSceneCallback( [&callbackCalled]( std::shared_ptr<assets::Scene> assetScene, ecs::Scene &targetScene ) {
		callbackCalled = true;

		// Import each root node
		for ( const auto &rootNode : assetScene->getRootNodes() )
		{
			Entity entity = targetScene.createEntity( rootNode->getName() );

			// Add Transform component
			if ( rootNode->hasTransform() )
			{
				const auto &nodeTransform = rootNode->getTransform();
				components::Transform ecsTransform;
				ecsTransform.position = { nodeTransform.position.x, nodeTransform.position.y, nodeTransform.position.z };
				ecsTransform.rotation = { nodeTransform.rotation.x, nodeTransform.rotation.y, nodeTransform.rotation.z };
				ecsTransform.scale = { nodeTransform.scale.x, nodeTransform.scale.y, nodeTransform.scale.z };
				targetScene.addComponent( entity, ecsTransform );
			}

			// Add MeshRenderer for each mesh handle
			rootNode->foreachMeshHandle( [&]( assets::MeshHandle meshHandle ) {
				MeshRenderer renderer;
				// TODO: Update for new GPU buffer-based MeshRenderer architecture
				// renderer.meshPath = assetScene->getPath(); // Reference the scene asset

				// Extract material paths from mesh primitives using the new API
				const auto mesh = assetScene->getMesh( meshHandle );
				if ( mesh )
				{
					// TODO: Update material handling for GPU-based approach
					// for ( std::size_t i = 0; i < mesh->getPrimitiveCount(); ++i )
					// {
					// 	const auto &primitive = mesh->getPrimitive( i );
					// 	if ( primitive.hasMaterial() )
					// 	{
					// 		renderer.materialPaths.push_back( primitive.getMaterialPath() );
					// 	}
					// }
				}

				// Set bounds from mesh
				if ( mesh->hasBounds() )
				{
					const auto center = mesh->getBoundsCenter();
					const auto size = mesh->getBoundsSize();
					renderer.bounds.min = center - size * 0.5f;
					renderer.bounds.max = center + size * 0.5f;
				}

				targetScene.addComponent( entity, renderer );
			} );
		}
	} );

	// Use AssetManager to import scene
	AssetManager manager;
	manager.store( "test_scene.gltf", scene );

	const bool result = manager.importScene( "test_scene.gltf", ecsScene );

	REQUIRE( result );
	REQUIRE( callbackCalled );

	// Verify entity was created with correct components
	const auto entities = ecsScene.getAllEntities();
	REQUIRE( entities.size() == 1 );

	const Entity entity = entities[0];
	REQUIRE( ecsScene.isValid( entity ) );

	// Check Name component
	const auto *nameComp = ecsScene.getComponent<Name>( entity );
	REQUIRE( nameComp != nullptr );
	REQUIRE( nameComp->name == "RootNode" );

	// Check Transform component
	const auto *transformComp = ecsScene.getComponent<Transform>( entity );
	REQUIRE( transformComp != nullptr );
	REQUIRE( transformComp->position.x == Approx( 1.0f ) );
	REQUIRE( transformComp->position.y == Approx( 2.0f ) );
	REQUIRE( transformComp->position.z == Approx( 3.0f ) );

	// Check MeshRenderer component
	const auto *rendererComp = ecsScene.getComponent<MeshRenderer>( entity );
	REQUIRE( rendererComp != nullptr );

	// Cleanup
	AssetManager::clearImportSceneCallback();
}

TEST_CASE( "ECS import preserves scene hierarchy", "[ecs][import][hierarchy]" )
{
	// Create a scene with parent-child hierarchy
	auto scene = std::make_shared<assets::Scene>();
	scene->setPath( "hierarchy_scene.gltf" );
	scene->setLoaded( true );

	// Create parent node
	auto parentNode = std::make_unique<SceneNode>( "ParentNode" );
	assets::Transform parentTransform;
	parentTransform.position = { 0.0f, 0.0f, 0.0f };
	parentNode->setTransform( parentTransform );

	// Create child node
	auto childNode = std::make_unique<SceneNode>( "ChildNode" );
	assets::Transform childTransform;
	childTransform.position = { 1.0f, 0.0f, 0.0f };
	childNode->setTransform( childTransform );

	// Add child to parent
	parentNode->addChild( std::move( childNode ) );
	scene->addRootNode( std::move( parentNode ) );

	// Set up ECS scene
	ecs::Scene ecsScene;

	// Register callback that imports hierarchy
	AssetManager::setImportSceneCallback( []( std::shared_ptr<assets::Scene> assetScene, ecs::Scene &targetScene ) {
		std::function<Entity( const SceneNode &, Entity )> importNode = [&]( const SceneNode &node, Entity parent ) -> Entity {
			Entity entity = targetScene.createEntity( node.getName() );

			// Add Transform component
			if ( node.hasTransform() )
			{
				const auto &nodeTransform = node.getTransform();
				components::Transform ecsTransform;
				ecsTransform.position = { nodeTransform.position.x, nodeTransform.position.y, nodeTransform.position.z };
				ecsTransform.rotation = { nodeTransform.rotation.x, nodeTransform.rotation.y, nodeTransform.rotation.z };
				ecsTransform.scale = { nodeTransform.scale.x, nodeTransform.scale.y, nodeTransform.scale.z };
				targetScene.addComponent( entity, ecsTransform );
			}

			// Set parent relationship
			if ( parent.isValid() )
			{
				targetScene.setParent( entity, parent );
			}

			// Import children recursively
			node.foreachChild( [&]( const SceneNode &child ) {
				importNode( child, entity );
			} );

			return entity;
		};

		// Import all root nodes
		for ( const auto &rootNode : assetScene->getRootNodes() )
		{
			importNode( *rootNode, Entity{} );
		}
	} );

	// Import scene
	AssetManager manager;
	manager.store( "hierarchy_scene.gltf", scene );

	const bool result = manager.importScene( "hierarchy_scene.gltf", ecsScene );

	REQUIRE( result );

	// Verify hierarchy was created correctly
	const auto entities = ecsScene.getAllEntities();
	REQUIRE( entities.size() == 2 ); // Parent + child

	// Find parent and child entities
	Entity parentEntity{}, childEntity{};
	for ( const auto entity : entities )
	{
		if ( !entity.isValid() )
			continue;

		const auto *name = ecsScene.getComponent<Name>( entity );
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
	const Entity actualParent = ecsScene.getParent( childEntity );
	REQUIRE( actualParent == parentEntity );

	const auto children = ecsScene.getChildren( parentEntity );
	REQUIRE( children.size() == 1 );
	REQUIRE( children[0] == childEntity );

	// Cleanup
	AssetManager::clearImportSceneCallback();
}

TEST_CASE( "ECS import handles nodes without meshes", "[ecs][import][empty-nodes]" )
{
	// Create a scene with a node that has no mesh data
	auto scene = std::make_shared<assets::Scene>();
	scene->setPath( "empty_node_scene.gltf" );
	scene->setLoaded( true );

	auto emptyNode = std::make_unique<SceneNode>( "EmptyNode" );
	assets::Transform transform;
	transform.position = { 5.0f, 6.0f, 7.0f };
	emptyNode->setTransform( transform );
	// Note: no mesh added to this node

	scene->addRootNode( std::move( emptyNode ) );

	// Set up ECS scene
	ecs::Scene ecsScene;

	// Register callback
	AssetManager::setImportSceneCallback( []( std::shared_ptr<assets::Scene> assetScene, ecs::Scene &targetScene ) {
		for ( const auto &rootNode : assetScene->getRootNodes() )
		{
			Entity entity = targetScene.createEntity( rootNode->getName() );

			// Add Transform component
			if ( rootNode->hasTransform() )
			{
				const auto &nodeTransform = rootNode->getTransform();
				components::Transform ecsTransform;
				ecsTransform.position = { nodeTransform.position.x, nodeTransform.position.y, nodeTransform.position.z };
				ecsTransform.rotation = { nodeTransform.rotation.x, nodeTransform.rotation.y, nodeTransform.rotation.z };
				ecsTransform.scale = { nodeTransform.scale.x, nodeTransform.scale.y, nodeTransform.scale.z };
				targetScene.addComponent( entity, ecsTransform );
			}

			// Only add MeshRenderer if node has meshes
			if ( rootNode->hasMeshHandles() )
			{
				rootNode->foreachMeshHandle( [&]( MeshHandle meshHandle ) {
					MeshRenderer renderer;
					// TODO: Will need to set the MeshRenderer with a MeshGPUBuffers
					// For now, just mark meshHandle as used to avoid warning
					(void)meshHandle;
					targetScene.addComponent( entity, renderer );
				} );
			}
		}
	} );

	// Import scene
	AssetManager manager;
	manager.store( "empty_node_scene.gltf", scene );

	const bool result = manager.importScene( "empty_node_scene.gltf", ecsScene );

	REQUIRE( result );

	// Verify entity was created but without MeshRenderer
	const auto entities = ecsScene.getAllEntities();
	REQUIRE( entities.size() == 1 );

	const Entity entity = entities[0];
	REQUIRE( ecsScene.isValid( entity ) );

	// Should have Name and Transform but not MeshRenderer
	REQUIRE( ecsScene.hasComponent<Name>( entity ) );
	REQUIRE( ecsScene.hasComponent<Transform>( entity ) );
	REQUIRE_FALSE( ecsScene.hasComponent<MeshRenderer>( entity ) );

	// Cleanup
	AssetManager::clearImportSceneCallback();
}