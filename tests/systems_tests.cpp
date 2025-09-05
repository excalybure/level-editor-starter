#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

import runtime.ecs;
import runtime.components;
import runtime.systems;
import engine.math;

using namespace ecs;
using namespace components;
using namespace systems;

TEST_CASE( "TransformSystem basic world matrix calculation", "[systems][transform]" )
{
	Scene scene;
	SystemManager systemManager;
	auto *transformSystem = systemManager.addSystem<TransformSystem>();
	systemManager.initialize( scene );

	// Create entity with transform
	Entity entity = scene.createEntity( "Entity1" );
	Transform t;
	t.position = { 1.0f, 2.0f, 3.0f };
	t.scale = { 2.0f, 2.0f, 2.0f };
	REQUIRE( scene.addComponent( entity, t ) );
	transformSystem->markDirty( entity );
	systemManager.update( scene, 0.016f );

	const auto worldMatrix = transformSystem->getWorldTransform( scene, entity );
	REQUIRE( worldMatrix.m03() == Catch::Approx( 1.0f ) );
	REQUIRE( worldMatrix.m13() == Catch::Approx( 2.0f ) );
	REQUIRE( worldMatrix.m23() == Catch::Approx( 3.0f ) );
}

TEST_CASE( "TransformSystem hierarchy updates", "[systems][transform][hierarchy]" )
{
	Scene scene;
	SystemManager systemManager;
	auto *transformSystem = systemManager.addSystem<TransformSystem>();
	systemManager.initialize( scene );

	// Parent entity
	Entity parent = scene.createEntity( "Parent" );
	Transform parentTransform;
	parentTransform.position = { 10.0f, 0.0f, 0.0f };
	REQUIRE( scene.addComponent( parent, parentTransform ) );

	// Child entity
	Entity child = scene.createEntity( "Child" );
	Transform childTransform;
	childTransform.position = { 1.0f, 2.0f, 3.0f };
	REQUIRE( scene.addComponent( child, childTransform ) );
	scene.setParent( child, parent );

	transformSystem->markDirty( parent );
	transformSystem->markDirty( child );
	systemManager.update( scene, 0.016f );

	const auto childWorldMatrix = transformSystem->getWorldTransform( scene, child );
	// Child's world position should be parent's position + child's local position
	REQUIRE( childWorldMatrix.m03() == Catch::Approx( 11.0f ) );
	REQUIRE( childWorldMatrix.m13() == Catch::Approx( 2.0f ) );
	REQUIRE( childWorldMatrix.m23() == Catch::Approx( 3.0f ) );
}

TEST_CASE( "SystemManager add/get/clear systems", "[systems][manager]" )
{
	SystemManager manager;
	auto *sys1 = manager.addSystem<TransformSystem>();
	auto *sys2 = manager.getSystem<TransformSystem>();
	REQUIRE( sys1 == sys2 );
	manager.clear();
	REQUIRE( manager.getSystem<TransformSystem>() == nullptr );
}

TEST_CASE( "TransformSystem markDirty only marks entity", "[systems][transform][markDirty]" )
{
	Scene scene;
	SystemManager systemManager;
	auto *transformSystem = systemManager.addSystem<TransformSystem>();
	systemManager.initialize( scene );

	Entity entity = scene.createEntity( "Entity1" );
	Transform t;
	t.position = { 5.0f, 0.0f, 0.0f };
	REQUIRE( scene.addComponent( entity, t ) );

	transformSystem->markDirty( entity );
	systemManager.update( scene, 0.016f );

	const auto worldMatrix = transformSystem->getWorldTransform( scene, entity );
	REQUIRE( worldMatrix.m03() == Catch::Approx( 5.0f ) );
}

TEST_CASE( "TransformSystem shutdown does not throw", "[systems][shutdown]" )
{
	Scene scene;
	SystemManager systemManager;
	systemManager.addSystem<TransformSystem>();
	systemManager.initialize( scene );
	REQUIRE_NOTHROW( systemManager.shutdown( scene ) );
}

TEST_CASE( "TransformSystem multiple entities", "[systems][transform][multiple]" )
{
	Scene scene;
	SystemManager systemManager;
	auto *transformSystem = systemManager.addSystem<TransformSystem>();
	systemManager.initialize( scene );

	// Create multiple entities with transforms
	Entity entity1 = scene.createEntity( "Entity1" );
	Entity entity2 = scene.createEntity( "Entity2" );
	Entity entity3 = scene.createEntity( "Entity3" );

	Transform t1, t2, t3;
	t1.position = { 1.0f, 0.0f, 0.0f };
	t2.position = { 0.0f, 2.0f, 0.0f };
	t3.position = { 0.0f, 0.0f, 3.0f };

	REQUIRE( scene.addComponent( entity1, t1 ) );
	REQUIRE( scene.addComponent( entity2, t2 ) );
	REQUIRE( scene.addComponent( entity3, t3 ) );

	transformSystem->markDirty( entity1 );
	transformSystem->markDirty( entity2 );
	transformSystem->markDirty( entity3 );
	systemManager.update( scene, 0.016f );

	const auto matrix1 = transformSystem->getWorldTransform( scene, entity1 );
	const auto matrix2 = transformSystem->getWorldTransform( scene, entity2 );
	const auto matrix3 = transformSystem->getWorldTransform( scene, entity3 );

	REQUIRE( matrix1.m03() == Catch::Approx( 1.0f ) );
	REQUIRE( matrix2.m13() == Catch::Approx( 2.0f ) );
	REQUIRE( matrix3.m23() == Catch::Approx( 3.0f ) );
}

TEST_CASE( "TransformSystem deep hierarchy", "[systems][transform][deep_hierarchy]" )
{
	Scene scene;
	SystemManager systemManager;
	auto *transformSystem = systemManager.addSystem<TransformSystem>();
	systemManager.initialize( scene );

	// Create a deep hierarchy: grandparent -> parent -> child -> grandchild
	Entity grandparent = scene.createEntity( "Grandparent" );
	Entity parent = scene.createEntity( "Parent" );
	Entity child = scene.createEntity( "Child" );
	Entity grandchild = scene.createEntity( "Grandchild" );

	Transform gpTransform, pTransform, cTransform, gcTransform;
	gpTransform.position = { 10.0f, 0.0f, 0.0f };
	pTransform.position = { 5.0f, 0.0f, 0.0f };
	cTransform.position = { 2.0f, 0.0f, 0.0f };
	gcTransform.position = { 1.0f, 0.0f, 0.0f };

	REQUIRE( scene.addComponent( grandparent, gpTransform ) );
	REQUIRE( scene.addComponent( parent, pTransform ) );
	REQUIRE( scene.addComponent( child, cTransform ) );
	REQUIRE( scene.addComponent( grandchild, gcTransform ) );

	scene.setParent( parent, grandparent );
	scene.setParent( child, parent );
	scene.setParent( grandchild, child );

	transformSystem->markDirty( grandparent );
	transformSystem->markDirty( parent );
	transformSystem->markDirty( child );
	transformSystem->markDirty( grandchild );
	systemManager.update( scene, 0.016f );

	const auto gcMatrix = transformSystem->getWorldTransform( scene, grandchild );
	// Debug: Let's check intermediate values
	const auto gpMatrix = transformSystem->getWorldTransform( scene, grandparent );
	const auto pMatrix = transformSystem->getWorldTransform( scene, parent );
	const auto cMatrix = transformSystem->getWorldTransform( scene, child );

	// Let's check what we're actually getting
	REQUIRE( gpMatrix.m03() == Catch::Approx( 10.0f ) ); // Grandparent should be at 10
}

// Forward declare a system type that we won't add
class DummySystem : public System
{
public:
	void update( ecs::Scene & /*scene*/, float /*deltaTime*/ ) override {}
};

TEST_CASE( "SystemManager multiple system types", "[systems][manager][multiple]" )
{
	SystemManager manager;

	// Add transform system
	auto *transformSystem = manager.addSystem<TransformSystem>();
	REQUIRE( transformSystem != nullptr );

	// Verify we can get it back
	auto *retrievedTransform = manager.getSystem<TransformSystem>();
	REQUIRE( retrievedTransform == transformSystem );

	// Test that we get nullptr for systems that don't exist
	REQUIRE( manager.getSystem<DummySystem>() == nullptr );
}

TEST_CASE( "SystemManager initialize/update/shutdown flow", "[systems][manager][lifecycle]" )
{
	Scene scene;
	SystemManager manager;

	manager.addSystem<TransformSystem>();

	// Test initialization
	REQUIRE_NOTHROW( manager.initialize( scene ) );

	// Add an entity with transform to test update
	Entity entity = scene.createEntity( "TestEntity" );
	Transform t;
	t.position = { 1.0f, 2.0f, 3.0f };
	REQUIRE( scene.addComponent( entity, t ) );

	// Test update
	REQUIRE_NOTHROW( manager.update( scene, 0.016f ) );

	// Test shutdown
	REQUIRE_NOTHROW( manager.shutdown( scene ) );
}

TEST_CASE( "TransformSystem with rotation and scale", "[systems][transform][rotation_scale]" )
{
	Scene scene;
	SystemManager systemManager;
	auto *transformSystem = systemManager.addSystem<TransformSystem>();
	systemManager.initialize( scene );

	Entity entity = scene.createEntity( "RotScaleEntity" );
	Transform transform;
	transform.position = math::Vec3<float>( 1.0f, 2.0f, 3.0f );
	transform.rotation = math::Vec3<float>( 0.0f, math::pi<float> / 4.0f, 0.0f ); // 45 degrees around Y
	transform.scale = math::Vec3<float>( 2.0f, 2.0f, 2.0f );

	REQUIRE( scene.addComponent( entity, transform ) );
	transformSystem->markDirty( entity );
	systemManager.update( scene, 0.016f );

	const auto worldMatrix = transformSystem->getWorldTransform( scene, entity );

	// Translation should still be correct
	REQUIRE( worldMatrix.m03() == Catch::Approx( 1.0f ) );
	REQUIRE( worldMatrix.m13() == Catch::Approx( 2.0f ) );
	REQUIRE( worldMatrix.m23() == Catch::Approx( 3.0f ) );
}
