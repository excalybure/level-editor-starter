#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>


import runtime.ecs;
import runtime.components;
import runtime.systems;

using namespace ecs;
using namespace components;

TEST_CASE( "ECS Storage basic create/has/get", "[ecs]" )
{
	Storage<components::Transform> storage;

	// Create default entity
	Entity e0 = storage.create();
	REQUIRE( e0.id == 1u ); // New system starts IDs from 1
	REQUIRE( storage.has( e0 ) );

	// Create with value
	components::Transform t;
	t.position = { 3.5f, -2.0f, 1.0f };
	Entity e1 = storage.create( t );
	REQUIRE( e1.id == 2u ); // Second entity gets ID 2
	REQUIRE( storage.has( e1 ) );
	REQUIRE( storage.get( e1 ).position.x == Catch::Approx( 3.5f ) );
	REQUIRE( storage.get( e1 ).position.y == Catch::Approx( -2.0f ) );
	REQUIRE( storage.get( e1 ).position.z == Catch::Approx( 1.0f ) );

	// Modify component via reference
	auto &posRef = storage.get( e0 );
	posRef.position.x = 10.f;
	posRef.position.y = 5.f;
	posRef.position.z = 2.f;
	REQUIRE( storage.get( e0 ).position.x == Catch::Approx( 10.f ) );
	REQUIRE( storage.get( e0 ).position.y == Catch::Approx( 5.f ) );
	REQUIRE( storage.get( e0 ).position.z == Catch::Approx( 2.f ) );

	// has() should be false for non-existent entity id
	Entity invalid{ 100, 0 };
	REQUIRE_FALSE( storage.has( invalid ) );
}

struct Velocity
{
	float dx = 0.0f, dy = 0.0f, dz = 0.0f;

	Velocity() = default;
	Velocity( float dx, float dy, float dz ) : dx( dx ), dy( dy ), dz( dz ) {}
};

TEST_CASE( "Entity creation and management", "[ecs]" )
{
	EntityManager entityManager;

	SECTION( "Create entity" )
	{
		Entity entity = entityManager.create();
		REQUIRE( entity.id != 0 );
		REQUIRE( entity.generation == 0 );
		REQUIRE( entityManager.isValid( entity ) );
	}

	SECTION( "Create multiple entities" )
	{
		Entity entity1 = entityManager.create();
		Entity entity2 = entityManager.create();
		Entity entity3 = entityManager.create();

		REQUIRE( entity1.id != entity2.id );
		REQUIRE( entity2.id != entity3.id );
		REQUIRE( entity1.id != entity3.id );

		REQUIRE( entityManager.isValid( entity1 ) );
		REQUIRE( entityManager.isValid( entity2 ) );
		REQUIRE( entityManager.isValid( entity3 ) );
	}

	SECTION( "Destroy entity" )
	{
		Entity entity = entityManager.create();
		REQUIRE( entityManager.isValid( entity ) );

		entityManager.destroy( entity );
		REQUIRE_FALSE( entityManager.isValid( entity ) );
	}

	SECTION( "Entity recycling and generation" )
	{
		Entity entity1 = entityManager.create();
		uint32_t firstId = entity1.id;

		entityManager.destroy( entity1 );
		REQUIRE_FALSE( entityManager.isValid( entity1 ) );

		Entity entity2 = entityManager.create();
		REQUIRE( entity2.id == firstId );	// ID should be recycled
		REQUIRE( entity2.generation == 2 ); // Generation should increment by 2 (1 for destroy, 1 for reuse)
		REQUIRE( entityManager.isValid( entity2 ) );

		// Old entity with same ID but old generation should be invalid
		REQUIRE_FALSE( entityManager.isValid( entity1 ) );
	}
}

TEST_CASE( "Component Storage", "[ecs]" )
{
	ComponentStorage<components::Transform> transformStorage;
	ComponentStorage<Velocity> velocityStorage;
	Entity entity{ 1, 0 };

	SECTION( "Add and get component" )
	{
		components::Transform t;
		t.position = { 10.0f, 20.0f, 30.0f };
		transformStorage.add( entity, t );

		REQUIRE( transformStorage.has( entity ) );

		const auto *storedT = transformStorage.get( entity );
		REQUIRE( storedT != nullptr );
		REQUIRE( storedT->position.x == Catch::Approx( 10.0f ) );
		REQUIRE( storedT->position.y == Catch::Approx( 20.0f ) );
	}

	SECTION( "Remove component" )
	{
		components::Transform t;
		t.position = { 5.0f, 15.0f, 25.0f };
		transformStorage.add( entity, t );
		REQUIRE( transformStorage.has( entity ) );

		REQUIRE( transformStorage.remove( entity ) );
		REQUIRE_FALSE( transformStorage.has( entity ) );
		REQUIRE( transformStorage.get( entity ) == nullptr );
	}

	SECTION( "Multiple components" )
	{
		Entity entity2{ 2, 0 };
		Entity entity3{ 3, 0 };

		transformStorage.add( entity, components::Transform{ { 1.0f, 2.0f, 3.0f } } );
		transformStorage.add( entity2, components::Transform{ { 4.0f, 5.0f, 6.0f } } );
		transformStorage.add( entity3, components::Transform{ { 7.0f, 8.0f, 9.0f } } );

		REQUIRE( transformStorage.has( entity ) );
		REQUIRE( transformStorage.has( entity2 ) );
		REQUIRE( transformStorage.has( entity3 ) );

		auto *t1 = transformStorage.get( entity );
		auto *t2 = transformStorage.get( entity2 );
		auto *t3 = transformStorage.get( entity3 );

		REQUIRE( t1->position.x == Catch::Approx( 1.0f ) );
		REQUIRE( t2->position.x == Catch::Approx( 4.0f ) );
		REQUIRE( t3->position.x == Catch::Approx( 7.0f ) );
	}
}

TEST_CASE( "Enhanced ECS Scene", "[ecs][scene]" )
{
	Scene scene;

	SECTION( "Entity creation" )
	{
		Entity entity = scene.createEntity();
		REQUIRE( scene.isValid( entity ) );
	}

	SECTION( "Entity creation with name" )
	{
		Entity entity = scene.createEntity( "TestEntity" );
		REQUIRE( entity.id != 0 );
		REQUIRE( scene.isValid( entity ) );
	}

	SECTION( "Entity destruction" )
	{
		Entity entity = scene.createEntity();
		REQUIRE( scene.isValid( entity ) );

		scene.destroyEntity( entity );
		REQUIRE_FALSE( scene.isValid( entity ) );
	}

	SECTION( "Basic component management" )
	{
		Entity entity = scene.createEntity();

		// Add component
		components::Transform t;
		t.position = { 10.0f, 20.0f, 30.0f };
		REQUIRE( scene.addComponent( entity, t ) );
		REQUIRE( scene.hasComponent<components::Transform>( entity ) );
		// ...existing code...
		// Get component
		auto *storedT = scene.getComponent<components::Transform>( entity );
		REQUIRE( storedT != nullptr );
		REQUIRE( storedT->position.x == Catch::Approx( 10.0f ) );

		// ...existing code...
		REQUIRE( scene.removeComponent<components::Transform>( entity ) );
		REQUIRE_FALSE( scene.hasComponent<components::Transform>( entity ) );
		REQUIRE( scene.getComponent<components::Transform>( entity ) == nullptr );
	}
}

// Component concept validation tests
TEST_CASE( "Component concept validation", "[ecs][concepts]" )
{
	REQUIRE( ecs::Component<components::Transform> );
	REQUIRE( ecs::Component<Velocity> );
}

TEST_CASE( "Enhanced ECS Scene Management", "[ecs]" )
{
	Scene scene;

	SECTION( "Entity creation and destruction" )
	{
		Entity e1 = scene.createEntity( "TestEntity" );
		REQUIRE( e1.isValid() );
		REQUIRE( scene.isValid( e1 ) );

		Entity e2 = scene.createEntity( "AnotherEntity" );
		REQUIRE( e2.isValid() );
		REQUIRE( e2.id != e1.id );

		// Destroy entity
		REQUIRE( scene.destroyEntity( e1 ) );
		REQUIRE_FALSE( scene.isValid( e1 ) );
		REQUIRE( scene.isValid( e2 ) ); // Other entity should still be valid
	}

	SECTION( "Component management" )
	{
		Entity entity = scene.createEntity( "ComponentTest" );

		// Add components
		components::Transform transform;
		transform.position = { 1.0f, 2.0f, 3.0f };
		REQUIRE( scene.addComponent( entity, transform ) );
		REQUIRE( scene.hasComponent<components::Transform>( entity ) );

		components::Name name{ "TestName" };
		REQUIRE( scene.addComponent( entity, name ) );
		REQUIRE( scene.hasComponent<components::Name>( entity ) );

		// Get components
		auto *transformPtr = scene.getComponent<components::Transform>( entity );
		REQUIRE( transformPtr != nullptr );
		REQUIRE( transformPtr->position.x == Catch::Approx( 1.0f ) );
		REQUIRE( transformPtr->position.y == Catch::Approx( 2.0f ) );
		REQUIRE( transformPtr->position.z == Catch::Approx( 3.0f ) );

		auto *namePtr = scene.getComponent<components::Name>( entity );
		REQUIRE( namePtr != nullptr );
		REQUIRE( namePtr->name == "TestName" );

		// Remove component
		REQUIRE( scene.removeComponent<components::Name>( entity ) );
		REQUIRE_FALSE( scene.hasComponent<components::Name>( entity ) );
		REQUIRE( scene.hasComponent<components::Transform>( entity ) ); // Other component should remain
	}

	SECTION( "Hierarchy management" )
	{
		Entity parent = scene.createEntity( "Parent" );
		Entity child1 = scene.createEntity( "Child1" );
		Entity child2 = scene.createEntity( "Child2" );
		Entity grandchild = scene.createEntity( "Grandchild" );

		// Set up hierarchy
		scene.setParent( child1, parent );
		scene.setParent( child2, parent );
		scene.setParent( grandchild, child1 );

		// Verify parent relationships
		REQUIRE( scene.getParent( child1 ) == parent );
		REQUIRE( scene.getParent( child2 ) == parent );
		REQUIRE( scene.getParent( grandchild ) == child1 );
		REQUIRE_FALSE( scene.getParent( parent ).isValid() ); // Root has no parent

		// Verify children relationships
		auto parentChildren = scene.getChildren( parent );
		REQUIRE( parentChildren.size() == 2 );
		REQUIRE( std::find( parentChildren.begin(), parentChildren.end(), child1 ) != parentChildren.end() );
		REQUIRE( std::find( parentChildren.begin(), parentChildren.end(), child2 ) != parentChildren.end() );

		auto child1Children = scene.getChildren( child1 );
		REQUIRE( child1Children.size() == 1 );
		REQUIRE( child1Children[0] == grandchild );

		// Remove parent relationship
		scene.removeParent( child1 );
		REQUIRE_FALSE( scene.getParent( child1 ).isValid() );

		// Verify grandchild is also orphaned (should be handled by destroy)
		auto newParentChildren = scene.getChildren( parent );
		REQUIRE( newParentChildren.size() == 1 );
		REQUIRE( newParentChildren[0] == child2 );
	}

	SECTION( "Entity recycling" )
	{
		// Create and destroy entities to test ID recycling
		Entity e1 = scene.createEntity();
		Entity e2 = scene.createEntity();
		std::uint32_t originalId1 = e1.id;

		scene.destroyEntity( e1 );

		// Create new entity - should reuse the destroyed entity's ID
		Entity e3 = scene.createEntity();
		REQUIRE( e3.id == originalId1 );
		REQUIRE( e3.generation > e1.generation ); // But with higher generation

		// Old entity reference should be invalid
		REQUIRE_FALSE( scene.isValid( e1 ) );
		REQUIRE( scene.isValid( e3 ) );
	}
}

TEST_CASE( "Transform System", "[ecs][systems]" )
{
	Scene scene;
	systems::SystemManager systemManager;

	auto *transformSystem = systemManager.addSystem<systems::TransformSystem>();
	systemManager.initialize( scene );

	SECTION( "Basic transform matrix calculation" )
	{
		Entity entity = scene.createEntity( "TransformTest" );

		components::Transform transform;
		transform.position = { 5.0f, 10.0f, 15.0f };
		transform.scale = { 2.0f, 2.0f, 2.0f };
		scene.addComponent( entity, transform );

		// Mark as dirty and update
		transformSystem->markDirty( entity );
		systemManager.update( scene, 0.016f );

		// Get world transform
		auto worldMatrix = transformSystem->getWorldTransform( scene, entity );

		// Verify the translation part of the matrix
		REQUIRE( worldMatrix.m03() == Catch::Approx( 5.0f ) );
		REQUIRE( worldMatrix.m13() == Catch::Approx( 10.0f ) );
		REQUIRE( worldMatrix.m23() == Catch::Approx( 15.0f ) );
	}

	SECTION( "Parent dirty propagation updates child" )
	{
		Entity parent = scene.createEntity( "Parent" );
		Entity child = scene.createEntity( "Child" );
		// Establish hierarchy
		scene.setParent( child, parent );

		components::Transform parentT;
		parentT.position = { 2.0f, 3.0f, 4.0f };
		components::Transform childT;
		childT.position = { 1.0f, 0.0f, 0.0f };
		scene.addComponent( parent, parentT );
		scene.addComponent( child, childT );

		// Mark only parent dirty
		transformSystem->markDirty( parent );
		systemManager.update( scene, 0.016f );

		// Child world matrix should reflect parent's translation + its own local
		const auto childWorld = transformSystem->getWorldTransform( scene, child );
		REQUIRE( childWorld.m03() == Catch::Approx( 2.0f + 1.0f ) );
		REQUIRE( childWorld.m13() == Catch::Approx( 3.0f + 0.0f ) );
		REQUIRE( childWorld.m23() == Catch::Approx( 4.0f + 0.0f ) );
	}

	SECTION( "Automatic dirty marking via modifyComponent" )
	{
		Entity parent = scene.createEntity( "Parent" );
		Entity child = scene.createEntity( "Child" );
		scene.setParent( child, parent );

		components::Transform parentT;
		parentT.position = { 1.0f, 1.0f, 1.0f };
		components::Transform childT;
		childT.position = { 0.5f, 0.0f, 0.0f };
		scene.addComponent( parent, parentT );
		scene.addComponent( child, childT );

		// Use modifyComponent to change parent position - should auto-mark dirty
		scene.modifyComponent<components::Transform>( parent, []( components::Transform &t ) {
			t.position.x = 5.0f;
		} );

		systemManager.update( scene, 0.016f );

		// Child should reflect the new parent position
		const auto childWorld = transformSystem->getWorldTransform( scene, child );
		REQUIRE( childWorld.m03() == Catch::Approx( 5.0f + 0.5f ) );
	}

	systemManager.shutdown( scene );
}

TEST_CASE( "Component Types Validation", "[ecs][components]" )
{
	// Verify all components satisfy the Component concept
	REQUIRE( ecs::Component<components::Transform> );
	REQUIRE( ecs::Component<components::Name> );
	REQUIRE( ecs::Component<components::Visible> );
	REQUIRE( ecs::Component<components::MeshRenderer> );
	REQUIRE( ecs::Component<components::Selected> );
	REQUIRE( ecs::Component<components::Hierarchy> );
}
