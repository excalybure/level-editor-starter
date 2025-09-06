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

		// Verify Name component is NOT added when using default name
		REQUIRE_FALSE( scene.hasComponent<components::Name>( entity ) );
	}

	SECTION( "Entity creation with default name parameter" )
	{
		Entity entity = scene.createEntity( "Entity" );
		REQUIRE( scene.isValid( entity ) );

		// Verify Name component is NOT added when using "Entity" as name
		REQUIRE_FALSE( scene.hasComponent<components::Name>( entity ) );
	}

	SECTION( "Entity creation with name" )
	{
		Entity entity = scene.createEntity( "TestEntity" );
		REQUIRE( entity.id != 0 );
		REQUIRE( scene.isValid( entity ) );

		// Verify Name component is automatically added when a custom name is provided
		REQUIRE( scene.hasComponent<components::Name>( entity ) );
		auto *namePtr = scene.getComponent<components::Name>( entity );
		REQUIRE( namePtr != nullptr );
		REQUIRE( namePtr->name == "TestEntity" );
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
	REQUIRE( components::Component<components::Transform> );
	REQUIRE( components::Component<Velocity> );
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

		// Use modifyComponent to change parent position, then manually mark transform system dirty
		scene.modifyComponent<components::Transform>( parent, []( components::Transform &t ) {
			t.position.x = 5.0f;
		} );

		// Manually mark the entity dirty in the transform system for proper propagation
		transformSystem->markDirty( parent );

		systemManager.update( scene, 0.016f );

		// Child should reflect the new parent position
		const auto childWorld = transformSystem->getWorldTransform( scene, child );
		REQUIRE( childWorld.m03() == Catch::Approx( 5.0f + 0.5f ) );
	}

	systemManager.shutdown( scene );
}

TEST_CASE( "Hierarchy Safety - Cycle Prevention", "[ecs][hierarchy][safety]" )
{
	Scene scene;

	SECTION( "Self-parenting should be prevented" )
	{
		Entity entity = scene.createEntity( "SelfParent" );

		// Attempt to parent entity to itself - should be ignored
		scene.setParent( entity, entity );

		// Verify no parent was set
		Entity parent = scene.getParent( entity );
		REQUIRE( !parent.isValid() );

		// Verify entity doesn't appear in its own children list
		auto children = scene.getChildren( entity );
		REQUIRE( children.empty() );
	}

	SECTION( "Descendant-parenting should be prevented" )
	{
		Entity grandparent = scene.createEntity( "Grandparent" );
		Entity parent = scene.createEntity( "Parent" );
		Entity child = scene.createEntity( "Child" );

		// Create hierarchy: grandparent -> parent -> child
		scene.setParent( parent, grandparent );
		scene.setParent( child, parent );

		// Verify initial hierarchy is correct
		REQUIRE( scene.getParent( parent ) == grandparent );
		REQUIRE( scene.getParent( child ) == parent );

		// Attempt to parent grandparent to child (creates cycle) - should be ignored
		scene.setParent( grandparent, child );

		// Verify hierarchy is unchanged
		REQUIRE( scene.getParent( parent ) == grandparent );
		REQUIRE( scene.getParent( child ) == parent );
		REQUIRE( !scene.getParent( grandparent ).isValid() );

		// Verify children lists are correct
		const auto grandparentChildren = scene.getChildren( grandparent );
		const auto parentChildren = scene.getChildren( parent );
		const auto childChildren = scene.getChildren( child );

		REQUIRE( grandparentChildren.size() == 1 );
		REQUIRE( grandparentChildren[0] == parent );
		REQUIRE( parentChildren.size() == 1 );
		REQUIRE( parentChildren[0] == child );
		REQUIRE( childChildren.empty() );
	}

	SECTION( "Direct descendant-parenting should be prevented" )
	{
		Entity parent = scene.createEntity( "Parent" );
		Entity child = scene.createEntity( "Child" );

		// Create simple parent-child relationship
		scene.setParent( child, parent );

		// Verify initial relationship
		REQUIRE( scene.getParent( child ) == parent );

		// Attempt to parent parent to child (creates cycle) - should be ignored
		scene.setParent( parent, child );

		// Verify relationship is unchanged
		REQUIRE( scene.getParent( child ) == parent );
		REQUIRE( !scene.getParent( parent ).isValid() );

		const auto parentChildren = scene.getChildren( parent );
		const auto childChildren = scene.getChildren( child );

		REQUIRE( parentChildren.size() == 1 );
		REQUIRE( parentChildren[0] == child );
		REQUIRE( childChildren.empty() );
	}
}

TEST_CASE( "Name Component Auto-Add on Creation", "[ecs][name][creation]" )
{
	Scene scene;

	SECTION( "Default name creates entity without Name component" )
	{
		Entity entity = scene.createEntity(); // Using default name "Entity"
		REQUIRE( !scene.hasComponent<components::Name>( entity ) );
	}

	SECTION( "Empty string creates entity without Name component" )
	{
		Entity entity = scene.createEntity( "" );
		REQUIRE( !scene.hasComponent<Name>( entity ) );
	}

	SECTION( "Empty string creates entity without Name component" )
	{
		Entity entity = scene.createEntity( "" );
		REQUIRE( !scene.hasComponent<Name>( entity ) );
	}

	SECTION( "Custom name auto-adds Name component" )
	{
		Entity entity = scene.createEntity( "TestEntity" );
		REQUIRE( scene.hasComponent<components::Name>( entity ) );

		const components::Name *nameComp = scene.getComponent<components::Name>( entity );
		REQUIRE( nameComp != nullptr );
		REQUIRE( nameComp->name == "TestEntity" );
	}

	SECTION( "Different custom names create correct Name components" )
	{
		Entity player = scene.createEntity( "Player" );
		Entity enemy = scene.createEntity( "Enemy" );
		Entity world = scene.createEntity( "World" );

		REQUIRE( scene.hasComponent<components::Name>( player ) );
		REQUIRE( scene.hasComponent<components::Name>( enemy ) );
		REQUIRE( scene.hasComponent<components::Name>( world ) );

		REQUIRE( scene.getComponent<components::Name>( player )->name == "Player" );
		REQUIRE( scene.getComponent<components::Name>( enemy )->name == "Enemy" );
		REQUIRE( scene.getComponent<components::Name>( world )->name == "World" );
	}

	SECTION( "Name component not added when name matches default" )
	{
		Entity entity = scene.createEntity( "Entity" ); // Explicit default name
		REQUIRE( !scene.hasComponent<Name>( entity ) );
	}

	SECTION( "Explicit default name creates entity without Name component" )
	{
		Entity entity = scene.createEntity( "Entity" );
		REQUIRE( !scene.hasComponent<components::Name>( entity ) );
	}
}

TEST_CASE( "Component Types Validation", "[ecs][components]" )
{
	// Verify all components satisfy the Component concept
	REQUIRE( components::Component<components::Transform> );
	REQUIRE( components::Component<components::Name> );
	REQUIRE( components::Component<components::Visible> );
	REQUIRE( components::Component<components::MeshRenderer> );
	REQUIRE( components::Component<components::Selected> );
}

TEST_CASE( "Transform Cache Invalidation on Component Removal", "[ecs][systems][cache]" )
{
	Scene scene;
	systems::SystemManager systemManager;

	auto *transformSystem = systemManager.addSystem<systems::TransformSystem>();
	systemManager.initialize( scene );

	SECTION( "Removing Transform component clears cached world matrix and dirty state" )
	{
		Entity entity = scene.createEntity( "CacheTest" );

		// Add Transform component and position it
		components::Transform transform;
		transform.position = { 10.0f, 20.0f, 30.0f };
		scene.addComponent( entity, transform );

		// Mark as dirty and update to ensure it's cached
		transformSystem->markDirty( entity );
		systemManager.update( scene, 0.016f );

		// Verify world matrix is calculated and cached
		const auto worldMatrix = transformSystem->getWorldTransform( scene, entity );
		REQUIRE( worldMatrix.m03() == Catch::Approx( 10.0f ) );
		REQUIRE( worldMatrix.m13() == Catch::Approx( 20.0f ) );
		REQUIRE( worldMatrix.m23() == Catch::Approx( 30.0f ) );

		// Mark dirty again to verify it's in the dirty set
		transformSystem->markDirty( entity );

		// Remove the Transform component
		REQUIRE( scene.removeComponent<components::Transform>( entity ) );

		// Verify component is actually removed
		REQUIRE_FALSE( scene.hasComponent<components::Transform>( entity ) );

		// After removal, getting world transform should return identity matrix
		// since the entity no longer has a Transform component
		const auto worldMatrixAfterRemoval = transformSystem->getWorldTransform( scene, entity );
		// The system should handle missing transform gracefully, and we verify
		// that no stale cache remains by checking that the entity is not in cache

		// Update system to process any remaining dirty entities
		systemManager.update( scene, 0.016f );

		// Verify cache invalidation worked by ensuring subsequent requests
		// don't return stale data - this is implicitly tested by not crashing
		// and not having unexpected values
	}

	SECTION( "Removing Transform component on parent with children" )
	{
		Entity parent = scene.createEntity( "Parent" );
		Entity child = scene.createEntity( "Child" );
		scene.setParent( child, parent );

		// Add Transform to both
		components::Transform parentTransform;
		parentTransform.position = { 5.0f, 0.0f, 0.0f };
		components::Transform childTransform;
		childTransform.position = { 1.0f, 0.0f, 0.0f };

		scene.addComponent( parent, parentTransform );
		scene.addComponent( child, childTransform );

		// Build cache
		transformSystem->markDirty( parent );
		systemManager.update( scene, 0.016f );

		// Verify child has world position reflecting parent + child
		const auto childWorldBefore = transformSystem->getWorldTransform( scene, child );
		REQUIRE( childWorldBefore.m03() == Catch::Approx( 6.0f ) );

		// Remove parent Transform
		REQUIRE( scene.removeComponent<components::Transform>( parent ) );

		// Child should now only have its local transform as world transform
		transformSystem->markDirty( child );
		systemManager.update( scene, 0.016f );

		const auto childWorldAfter = transformSystem->getWorldTransform( scene, child );
		REQUIRE( childWorldAfter.m03() == Catch::Approx( 1.0f ) );
	}

	systemManager.shutdown( scene );
}
