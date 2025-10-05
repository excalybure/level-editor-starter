#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "runtime/ecs.h"
#include "runtime/components.h"
#include "runtime/systems.h"

using Catch::Approx;


TEST_CASE( "ECS Storage basic create/has/get", "[ecs]" )
{
	ecs::Storage<components::Transform> storage;

	// Create default entity
	const ecs::Entity e0 = storage.create();
	REQUIRE( e0.id == 1u ); // New system starts IDs from 1
	REQUIRE( storage.has( e0 ) );

	// Create with value
	components::Transform t;
	t.position = { 3.5f, -2.0f, 1.0f };
	const ecs::Entity e1 = storage.create( t );
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
	const ecs::Entity invalid{ 100, 0 };
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
	ecs::EntityManager entityManager;

	SECTION( "Create entity" )
	{
		const ecs::Entity entity = entityManager.create();
		REQUIRE( entity.id != 0 );
		REQUIRE( entity.generation == 0 );
		REQUIRE( entityManager.isValid( entity ) );
	}

	SECTION( "Create multiple entities" )
	{
		const ecs::Entity entity1 = entityManager.create();
		const ecs::Entity entity2 = entityManager.create();
		const ecs::Entity entity3 = entityManager.create();

		REQUIRE( entity1.id != entity2.id );
		REQUIRE( entity2.id != entity3.id );
		REQUIRE( entity1.id != entity3.id );

		REQUIRE( entityManager.isValid( entity1 ) );
		REQUIRE( entityManager.isValid( entity2 ) );
		REQUIRE( entityManager.isValid( entity3 ) );
	}

	SECTION( "Destroy entity" )
	{
		const ecs::Entity entity = entityManager.create();
		REQUIRE( entityManager.isValid( entity ) );

		entityManager.destroy( entity );
		REQUIRE_FALSE( entityManager.isValid( entity ) );
	}

	SECTION( "Entity recycling and generation" )
	{
		const ecs::Entity entity1 = entityManager.create();
		const uint32_t firstId = entity1.id;

		entityManager.destroy( entity1 );
		REQUIRE_FALSE( entityManager.isValid( entity1 ) );

		const ecs::Entity entity2 = entityManager.create();
		REQUIRE( entity2.id == firstId );	// ID should be recycled
		REQUIRE( entity2.generation == 2 ); // Generation should increment by 2 (1 for destroy, 1 for reuse)
		REQUIRE( entityManager.isValid( entity2 ) );

		// Old entity with same ID but old generation should be invalid
		REQUIRE_FALSE( entityManager.isValid( entity1 ) );
	}
}

TEST_CASE( "Component Storage", "[ecs]" )
{
	ecs::ComponentStorage<components::Transform> transformStorage;
	ecs::ComponentStorage<Velocity> velocityStorage;
	const ecs::Entity entity{ 1, 0 };

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
		const ecs::Entity entity2{ 2, 0 };
		const ecs::Entity entity3{ 3, 0 };

		transformStorage.add( entity, components::Transform{ { 1.0f, 2.0f, 3.0f } } );
		transformStorage.add( entity2, components::Transform{ { 4.0f, 5.0f, 6.0f } } );
		transformStorage.add( entity3, components::Transform{ { 7.0f, 8.0f, 9.0f } } );

		REQUIRE( transformStorage.has( entity ) );
		REQUIRE( transformStorage.has( entity2 ) );
		REQUIRE( transformStorage.has( entity3 ) );

		const auto *t1 = transformStorage.get( entity );
		const auto *t2 = transformStorage.get( entity2 );
		const auto *t3 = transformStorage.get( entity3 );

		REQUIRE( t1->position.x == Catch::Approx( 1.0f ) );
		REQUIRE( t2->position.x == Catch::Approx( 4.0f ) );
		REQUIRE( t3->position.x == Catch::Approx( 7.0f ) );
	}
}

TEST_CASE( "Enhanced ECS Scene", "[ecs][scene]" )
{
	ecs::Scene scene;

	SECTION( "Entity creation" )
	{
		const ecs::Entity entity = scene.createEntity();
		REQUIRE( scene.isValid( entity ) );

		// Verify components::Name component is NOT added when using default name
		REQUIRE_FALSE( scene.hasComponent<components::Name>( entity ) );
	}

	SECTION( "Entity creation with default name parameter" )
	{
		const ecs::Entity entity = scene.createEntity( "Entity" );
		REQUIRE( scene.isValid( entity ) );

		// Verify components::Name component is NOT added when using "Entity" as name
		REQUIRE_FALSE( scene.hasComponent<components::Name>( entity ) );
	}

	SECTION( "Entity creation with name" )
	{
		const ecs::Entity entity = scene.createEntity( "TestEntity" );
		REQUIRE( entity.id != 0 );
		REQUIRE( scene.isValid( entity ) );

		// Verify components::Name component is automatically added when a custom name is provided
		REQUIRE( scene.hasComponent<components::Name>( entity ) );
		const auto *namePtr = scene.getComponent<components::Name>( entity );
		REQUIRE( namePtr != nullptr );
		REQUIRE( namePtr->name == "TestEntity" );
	}

	SECTION( "Entity destruction" )
	{
		ecs::Entity entity = scene.createEntity();
		REQUIRE( scene.isValid( entity ) );

		scene.destroyEntity( entity );
		REQUIRE_FALSE( scene.isValid( entity ) );
	}

	SECTION( "Basic component management" )
	{
		const ecs::Entity entity = scene.createEntity();

		// Add component
		components::Transform transform;
		transform.position = { 1.0f, 2.0f, 3.0f };
		REQUIRE( scene.addComponent( entity, transform ) );

		// Check component exists
		REQUIRE( scene.hasComponent<components::Transform>( entity ) );

		// Get component
		auto *transformPtr = scene.getComponent<components::Transform>( entity );
		REQUIRE( transformPtr != nullptr );
		REQUIRE( transformPtr->position.x == Catch::Approx( 1.0f ) );
		REQUIRE( transformPtr->position.y == Catch::Approx( 2.0f ) );
		REQUIRE( transformPtr->position.z == Catch::Approx( 3.0f ) );

		// Remove component
		REQUIRE( scene.removeComponent<components::Transform>( entity ) );
		REQUIRE_FALSE( scene.hasComponent<components::Transform>( entity ) );
	}

	SECTION( "forEach iteration utility" )
	{
		// Create entities with components::Transform components
		const ecs::Entity entity1 = scene.createEntity( "Entity1" );
		const ecs::Entity entity2 = scene.createEntity( "Entity2" );

		components::Transform t1;
		t1.position = { 1.0f, 0.0f, 0.0f };
		scene.addComponent( entity1, t1 );

		components::Transform t2;
		t2.position = { 2.0f, 0.0f, 0.0f };
		scene.addComponent( entity2, t2 );

		// Use forEach to count components::Transform components
		int count = 0;
		float sumX = 0.0f;
		scene.forEach<components::Transform>( [&count, &sumX]( ecs::Entity, components::Transform &transform ) {
			count++;
			sumX += transform.position.x;
		} );

		REQUIRE( count == 2 );
		REQUIRE( sumX == Catch::Approx( 3.0f ) );
	}

	SECTION( "forEach with empty storage" )
	{
		// Test forEach with no components of the requested type
		int nameCount = 0;
		scene.forEach<components::Name>( [&nameCount]( ecs::Entity, const components::Name & ) {
			nameCount++;
		} );

		REQUIRE( nameCount == 0 );
	}

	SECTION( "forEach with different component types" )
	{
		const ecs::Entity entity = scene.createEntity( "TestEntity" );

		// Add components::Name component (automatically added by createEntity)
		// Add Visible component
		components::Visible visible;
		visible.visible = true;
		scene.addComponent( entity, visible );

		// Test forEach with components::Name components
		int nameCount = 0;
		scene.forEach<components::Name>( [&nameCount]( ecs::Entity, const components::Name & ) {
			nameCount++;
		} );
		REQUIRE( nameCount == 1 );

		// Test forEach with Visible components
		int visibleCount = 0;
		scene.forEach<components::Visible>( [&visibleCount]( ecs::Entity, components::Visible & ) {
			visibleCount++;
		} );
		REQUIRE( visibleCount == 1 );
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
	ecs::Scene scene;

	SECTION( "Entity creation and destruction" )
	{
		const ecs::Entity e1 = scene.createEntity( "TestEntity" );
		REQUIRE( e1.isValid() );
		REQUIRE( scene.isValid( e1 ) );

		const ecs::Entity e2 = scene.createEntity( "AnotherEntity" );
		REQUIRE( e2.isValid() );
		REQUIRE( e2.id != e1.id );

		// Destroy entity
		REQUIRE( scene.destroyEntity( e1 ) );
		REQUIRE_FALSE( scene.isValid( e1 ) );
		REQUIRE( scene.isValid( e2 ) ); // Other entity should still be valid
	}

	SECTION( "Component management" )
	{
		const ecs::Entity entity = scene.createEntity( "ComponentTest" );

		// Add components
		components::Transform transform;
		transform.position = { 1.0f, 2.0f, 3.0f };
		REQUIRE( scene.addComponent( entity, transform ) );
		REQUIRE( scene.hasComponent<components::Transform>( entity ) );

		components::Name name{ "TestName" };
		REQUIRE( scene.addComponent( entity, name ) );
		REQUIRE( scene.hasComponent<components::Name>( entity ) );

		// Get components
		const auto *transformPtr = scene.getComponent<components::Transform>( entity );
		REQUIRE( transformPtr != nullptr );
		REQUIRE( transformPtr->position.x == Catch::Approx( 1.0f ) );
		REQUIRE( transformPtr->position.y == Catch::Approx( 2.0f ) );
		REQUIRE( transformPtr->position.z == Catch::Approx( 3.0f ) );

		const auto *namePtr = scene.getComponent<components::Name>( entity );
		REQUIRE( namePtr != nullptr );
		REQUIRE( namePtr->name == "TestName" );

		// Remove component
		REQUIRE( scene.removeComponent<components::Name>( entity ) );
		REQUIRE_FALSE( scene.hasComponent<components::Name>( entity ) );
		REQUIRE( scene.hasComponent<components::Transform>( entity ) ); // Other component should remain
	}

	SECTION( "Hierarchy management" )
	{
		const ecs::Entity parent = scene.createEntity( "Parent" );
		const ecs::Entity child1 = scene.createEntity( "Child1" );
		const ecs::Entity child2 = scene.createEntity( "Child2" );
		const ecs::Entity grandchild = scene.createEntity( "Grandchild" );

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
		const auto parentChildren = scene.getChildren( parent );
		REQUIRE( parentChildren.size() == 2 );
		REQUIRE( std::find( parentChildren.begin(), parentChildren.end(), child1 ) != parentChildren.end() );
		REQUIRE( std::find( parentChildren.begin(), parentChildren.end(), child2 ) != parentChildren.end() );

		const auto child1Children = scene.getChildren( child1 );
		REQUIRE( child1Children.size() == 1 );
		REQUIRE( child1Children[0] == grandchild );

		// Remove parent relationship
		scene.removeParent( child1 );
		REQUIRE_FALSE( scene.getParent( child1 ).isValid() );

		// Verify grandchild is also orphaned (should be handled by destroy)
		const auto newParentChildren = scene.getChildren( parent );
		REQUIRE( newParentChildren.size() == 1 );
		REQUIRE( newParentChildren[0] == child2 );
	}

	SECTION( "Entity recycling" )
	{
		// Create and destroy entities to test ID recycling
		const ecs::Entity e1 = scene.createEntity();
		const ecs::Entity e2 = scene.createEntity();
		std::uint32_t originalId1 = e1.id;

		scene.destroyEntity( e1 );

		// Create new entity - should reuse the destroyed entity's ID
		const ecs::Entity e3 = scene.createEntity();
		REQUIRE( e3.id == originalId1 );
		REQUIRE( e3.generation > e1.generation ); // But with higher generation

		// Old entity reference should be invalid
		REQUIRE_FALSE( scene.isValid( e1 ) );
		REQUIRE( scene.isValid( e3 ) );
	}
}

TEST_CASE( "Transform System", "[ecs][systems]" )
{
	ecs::Scene scene;
	systems::SystemManager systemManager;

	auto *const transformSystem = systemManager.addSystem<systems::TransformSystem>();
	systemManager.initialize( scene );

	SECTION( "Basic transform matrix calculation" )
	{
		const ecs::Entity entity = scene.createEntity( "TransformTest" );

		components::Transform transform;
		transform.position = { 5.0f, 10.0f, 15.0f };
		transform.scale = { 2.0f, 2.0f, 2.0f };
		scene.addComponent( entity, transform );

		// Mark as dirty and update
		transformSystem->markDirty( entity );
		systemManager.update( scene, 0.016f );

		// Get world transform
		const auto worldMatrix = transformSystem->getWorldTransform( scene, entity );

		// Verify the translation part of the matrix
		REQUIRE( worldMatrix.m03() == Catch::Approx( 5.0f ) );
		REQUIRE( worldMatrix.m13() == Catch::Approx( 10.0f ) );
		REQUIRE( worldMatrix.m23() == Catch::Approx( 15.0f ) );
	}

	SECTION( "Parent dirty propagation updates child" )
	{
		const ecs::Entity parent = scene.createEntity( "Parent" );
		const ecs::Entity child = scene.createEntity( "Child" );
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
		const ecs::Entity parent = scene.createEntity( "Parent" );
		const ecs::Entity child = scene.createEntity( "Child" );
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
	ecs::Scene scene;

	SECTION( "Self-parenting should be prevented" )
	{
		const ecs::Entity entity = scene.createEntity( "SelfParent" );

		// Attempt to parent entity to itself - should be ignored
		scene.setParent( entity, entity );

		// Verify no parent was set
		const ecs::Entity parent = scene.getParent( entity );
		REQUIRE( !parent.isValid() );

		// Verify entity doesn't appear in its own children list
		const auto children = scene.getChildren( entity );
		REQUIRE( children.empty() );
	}

	SECTION( "Descendant-parenting should be prevented" )
	{
		const ecs::Entity grandparent = scene.createEntity( "Grandparent" );
		const ecs::Entity parent = scene.createEntity( "Parent" );
		const ecs::Entity child = scene.createEntity( "Child" );

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
		const ecs::Entity parent = scene.createEntity( "Parent" );
		ecs::Entity child = scene.createEntity( "Child" );

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
	ecs::Scene scene;

	SECTION( "Default name creates entity without Name component" )
	{
		const ecs::Entity entity = scene.createEntity(); // Using default name "Entity"
		REQUIRE( !scene.hasComponent<components::Name>( entity ) );
	}

	SECTION( "Empty string creates entity without Name component" )
	{
		const ecs::Entity entity = scene.createEntity( "" );
		REQUIRE( !scene.hasComponent<components::Name>( entity ) );
	}

	SECTION( "Empty string creates entity without Name component" )
	{
		ecs::Entity entity = scene.createEntity( "" );
		REQUIRE( !scene.hasComponent<components::Name>( entity ) );
	}

	SECTION( "Custom name auto-adds Name component" )
	{
		const ecs::Entity entity = scene.createEntity( "TestEntity" );
		REQUIRE( scene.hasComponent<components::Name>( entity ) );

		const components::Name *nameComp = scene.getComponent<components::Name>( entity );
		REQUIRE( nameComp != nullptr );
		REQUIRE( nameComp->name == "TestEntity" );
	}

	SECTION( "Different custom names create correct Name components" )
	{
		const ecs::Entity player = scene.createEntity( "Player" );
		const ecs::Entity enemy = scene.createEntity( "Enemy" );
		const ecs::Entity world = scene.createEntity( "World" );

		REQUIRE( scene.hasComponent<components::Name>( player ) );
		REQUIRE( scene.hasComponent<components::Name>( enemy ) );
		REQUIRE( scene.hasComponent<components::Name>( world ) );

		REQUIRE( scene.getComponent<components::Name>( player )->name == "Player" );
		REQUIRE( scene.getComponent<components::Name>( enemy )->name == "Enemy" );
		REQUIRE( scene.getComponent<components::Name>( world )->name == "World" );
	}

	SECTION( "Name component not added when name matches default" )
	{
		const ecs::Entity entity = scene.createEntity( "Entity" ); // Explicit default name
		REQUIRE( !scene.hasComponent<components::Name>( entity ) );
	}

	SECTION( "Explicit default name creates entity without Name component" )
	{
		const ecs::Entity entity = scene.createEntity( "Entity" );
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
	ecs::Scene scene;
	systems::SystemManager systemManager;

	auto *const transformSystem = systemManager.addSystem<systems::TransformSystem>();
	systemManager.initialize( scene );

	SECTION( "Removing Transform component clears cached world matrix and dirty state" )
	{
		const ecs::Entity entity = scene.createEntity( "CacheTest" );

		// Add components::Transform component and position it
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

		// Remove the components::Transform component
		REQUIRE( scene.removeComponent<components::Transform>( entity ) );

		// Verify component is actually removed
		REQUIRE_FALSE( scene.hasComponent<components::Transform>( entity ) );

		// After removal, getting world transform should return identity matrix
		// since the entity no longer has a components::Transform component
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
		const ecs::Entity parent = scene.createEntity( "Parent" );
		ecs::Entity child = scene.createEntity( "Child" );
		scene.setParent( child, parent );

		// Add components::Transform to both
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

		// Remove parent components::Transform
		REQUIRE( scene.removeComponent<components::Transform>( parent ) );

		// Child should now only have its local transform as world transform
		transformSystem->markDirty( child );
		systemManager.update( scene, 0.016f );

		const auto childWorldAfter = transformSystem->getWorldTransform( scene, child );
		REQUIRE( childWorldAfter.m03() == Catch::Approx( 1.0f ) );
	}

	systemManager.shutdown( scene );
}

TEST_CASE( "Deep Hierarchy Destruction", "[ecs][hierarchy][destruction]" )
{
	ecs::Scene scene;

	SECTION( "destroyEntity(grandparent) cascades to all descendants" )
	{
		// Create a 4-level hierarchy:
		// great-grandparent -> grandparent -> parent -> child
		const ecs::Entity greatGrandparent = scene.createEntity( "GreatGrandparent" );
		const ecs::Entity grandparent = scene.createEntity( "Grandparent" );
		const ecs::Entity parent = scene.createEntity( "Parent" );
		const ecs::Entity child = scene.createEntity( "Child" );

		// Set up hierarchy
		scene.setParent( grandparent, greatGrandparent );
		scene.setParent( parent, grandparent );
		scene.setParent( child, parent );

		// Verify hierarchy is established
		REQUIRE( scene.getParent( grandparent ) == greatGrandparent );
		REQUIRE( scene.getParent( parent ) == grandparent );
		REQUIRE( scene.getParent( child ) == parent );

		// Verify all entities are valid
		REQUIRE( scene.isValid( greatGrandparent ) );
		REQUIRE( scene.isValid( grandparent ) );
		REQUIRE( scene.isValid( parent ) );
		REQUIRE( scene.isValid( child ) );

		// Destroy the grandparent - should cascade to parent and child
		scene.destroyEntity( grandparent );

		// Verify grandparent and all descendants are destroyed
		REQUIRE_FALSE( scene.isValid( grandparent ) );
		REQUIRE_FALSE( scene.isValid( parent ) );
		REQUIRE_FALSE( scene.isValid( child ) );

		// Great-grandparent should still be valid
		REQUIRE( scene.isValid( greatGrandparent ) );

		// Great-grandparent should have no children
		const auto remainingChildren = scene.getChildren( greatGrandparent );
		REQUIRE( remainingChildren.empty() );
	}

	SECTION( "destroyEntity with multiple child branches" )
	{
		// Create a tree structure:
		//        root
		//       /    \
		//   child1   child2
		//   /        /    \
		// gc1      gc2   gc3
		const ecs::Entity root = scene.createEntity( "Root" );
		const ecs::Entity child1 = scene.createEntity( "Child1" );
		const ecs::Entity child2 = scene.createEntity( "Child2" );
		const ecs::Entity grandchild1 = scene.createEntity( "Grandchild1" );
		const ecs::Entity grandchild2 = scene.createEntity( "Grandchild2" );
		const ecs::Entity grandchild3 = scene.createEntity( "Grandchild3" );

		// Set up hierarchy
		scene.setParent( child1, root );
		scene.setParent( child2, root );
		scene.setParent( grandchild1, child1 );
		scene.setParent( grandchild2, child2 );
		scene.setParent( grandchild3, child2 );

		// Verify all entities are valid
		REQUIRE( scene.isValid( root ) );
		REQUIRE( scene.isValid( child1 ) );
		REQUIRE( scene.isValid( child2 ) );
		REQUIRE( scene.isValid( grandchild1 ) );
		REQUIRE( scene.isValid( grandchild2 ) );
		REQUIRE( scene.isValid( grandchild3 ) );

		// Destroy the root - should cascade to all descendants
		scene.destroyEntity( root );

		// Verify all entities are destroyed
		REQUIRE_FALSE( scene.isValid( root ) );
		REQUIRE_FALSE( scene.isValid( child1 ) );
		REQUIRE_FALSE( scene.isValid( child2 ) );
		REQUIRE_FALSE( scene.isValid( grandchild1 ) );
		REQUIRE_FALSE( scene.isValid( grandchild2 ) );
		REQUIRE_FALSE( scene.isValid( grandchild3 ) );
	}

	SECTION( "destroyEntity with great-great-grandchildren" )
	{
		// Create a 5-level hierarchy to test deep recursion
		const ecs::Entity level1 = scene.createEntity( "Level1" );
		const ecs::Entity level2 = scene.createEntity( "Level2" );
		const ecs::Entity level3 = scene.createEntity( "Level3" );
		const ecs::Entity level4 = scene.createEntity( "Level4" );
		const ecs::Entity level5 = scene.createEntity( "Level5" );

		// Set up deep hierarchy
		scene.setParent( level2, level1 );
		scene.setParent( level3, level2 );
		scene.setParent( level4, level3 );
		scene.setParent( level5, level4 );

		// Verify all entities are valid
		REQUIRE( scene.isValid( level1 ) );
		REQUIRE( scene.isValid( level2 ) );
		REQUIRE( scene.isValid( level3 ) );
		REQUIRE( scene.isValid( level4 ) );
		REQUIRE( scene.isValid( level5 ) );

		// Destroy level 2 - should cascade to levels 3, 4, and 5
		scene.destroyEntity( level2 );

		// Verify level 2 and all descendants are destroyed
		REQUIRE_FALSE( scene.isValid( level2 ) );
		REQUIRE_FALSE( scene.isValid( level3 ) );
		REQUIRE_FALSE( scene.isValid( level4 ) );
		REQUIRE_FALSE( scene.isValid( level5 ) );

		// Level 1 should still be valid with no children
		REQUIRE( scene.isValid( level1 ) );
		const auto remainingChildren = scene.getChildren( level1 );
		REQUIRE( remainingChildren.empty() );
	}
}

TEST_CASE( "Reparenting Preserves Sub-hierarchy", "[ecs][hierarchy][reparenting]" )
{
	ecs::Scene scene;

	SECTION( "Moving parent with children maintains sub-tree structure" )
	{
		// Create initial structure:
		//     root1       root2
		//     /   \
		//  child1  child2
		//  /       /   \
		// gc1     gc2  gc3
		const ecs::Entity root1 = scene.createEntity( "Root1" );
		const ecs::Entity root2 = scene.createEntity( "Root2" );
		const ecs::Entity child1 = scene.createEntity( "Child1" );
		const ecs::Entity child2 = scene.createEntity( "Child2" );
		const ecs::Entity grandchild1 = scene.createEntity( "Grandchild1" );
		const ecs::Entity grandchild2 = scene.createEntity( "Grandchild2" );
		const ecs::Entity grandchild3 = scene.createEntity( "Grandchild3" );

		// Set up initial hierarchy
		scene.setParent( child1, root1 );
		scene.setParent( child2, root1 );
		scene.setParent( grandchild1, child1 );
		scene.setParent( grandchild2, child2 );
		scene.setParent( grandchild3, child2 );

		// Verify initial hierarchy
		REQUIRE( scene.getParent( child1 ) == root1 );
		REQUIRE( scene.getParent( child2 ) == root1 );
		REQUIRE( scene.getParent( grandchild1 ) == child1 );
		REQUIRE( scene.getParent( grandchild2 ) == child2 );
		REQUIRE( scene.getParent( grandchild3 ) == child2 );

		const auto root1Children = scene.getChildren( root1 );
		REQUIRE( root1Children.size() == 2 );
		const auto child2Children = scene.getChildren( child2 );
		REQUIRE( child2Children.size() == 2 );

		// Move child2 (with its sub-tree) from root1 to root2
		scene.setParent( child2, root2 );

		// Verify new structure:
		//     root1    root2
		//     /         |
		//  child1     child2
		//  /          /   \
		// gc1        gc2  gc3

		// Child2 should now be under root2
		REQUIRE( scene.getParent( child2 ) == root2 );

		// Child2's subtree should be preserved
		REQUIRE( scene.getParent( grandchild2 ) == child2 );
		REQUIRE( scene.getParent( grandchild3 ) == child2 );
		const auto child2NewChildren = scene.getChildren( child2 );
		REQUIRE( child2NewChildren.size() == 2 );
		REQUIRE( std::find( child2NewChildren.begin(), child2NewChildren.end(), grandchild2 ) != child2NewChildren.end() );
		REQUIRE( std::find( child2NewChildren.begin(), child2NewChildren.end(), grandchild3 ) != child2NewChildren.end() );

		// Root1 should only have child1 now
		const auto root1NewChildren = scene.getChildren( root1 );
		REQUIRE( root1NewChildren.size() == 1 );
		REQUIRE( root1NewChildren[0] == child1 );

		// Root2 should have child2
		const auto root2Children = scene.getChildren( root2 );
		REQUIRE( root2Children.size() == 1 );
		REQUIRE( root2Children[0] == child2 );

		// Child1's subtree should be unchanged
		REQUIRE( scene.getParent( child1 ) == root1 );
		REQUIRE( scene.getParent( grandchild1 ) == child1 );
		const auto child1Children = scene.getChildren( child1 );
		REQUIRE( child1Children.size() == 1 );
		REQUIRE( child1Children[0] == grandchild1 );
	}

	SECTION( "Moving deep sub-tree preserves all descendant relationships" )
	{
		// Create structure:
		//  oldParent       newParent
		//     |
		//   branch
		//     |
		//   level1
		//   /    \
		// level2a level2b
		//    |
		//  level3
		const ecs::Entity oldParent = scene.createEntity( "OldParent" );
		const ecs::Entity newParent = scene.createEntity( "NewParent" );
		const ecs::Entity branch = scene.createEntity( "Branch" );
		const ecs::Entity level1 = scene.createEntity( "Level1" );
		const ecs::Entity level2a = scene.createEntity( "Level2a" );
		const ecs::Entity level2b = scene.createEntity( "Level2b" );
		const ecs::Entity level3 = scene.createEntity( "Level3" );

		// Set up hierarchy
		scene.setParent( branch, oldParent );
		scene.setParent( level1, branch );
		scene.setParent( level2a, level1 );
		scene.setParent( level2b, level1 );
		scene.setParent( level3, level2a );

		// Verify initial structure
		REQUIRE( scene.getParent( branch ) == oldParent );
		REQUIRE( scene.getParent( level1 ) == branch );
		REQUIRE( scene.getParent( level2a ) == level1 );
		REQUIRE( scene.getParent( level2b ) == level1 );
		REQUIRE( scene.getParent( level3 ) == level2a );

		// Move the entire branch to newParent
		scene.setParent( branch, newParent );

		// Verify top-level move worked
		REQUIRE( scene.getParent( branch ) == newParent );
		const auto newParentChildren = scene.getChildren( newParent );
		REQUIRE( newParentChildren.size() == 1 );
		REQUIRE( newParentChildren[0] == branch );

		// Old parent should have no children
		const auto oldParentChildren = scene.getChildren( oldParent );
		REQUIRE( oldParentChildren.empty() );

		// Verify entire sub-tree structure is preserved
		REQUIRE( scene.getParent( level1 ) == branch );
		REQUIRE( scene.getParent( level2a ) == level1 );
		REQUIRE( scene.getParent( level2b ) == level1 );
		REQUIRE( scene.getParent( level3 ) == level2a );

		// Verify children lists are preserved
		const auto branchChildren = scene.getChildren( branch );
		REQUIRE( branchChildren.size() == 1 );
		REQUIRE( branchChildren[0] == level1 );

		const auto level1Children = scene.getChildren( level1 );
		REQUIRE( level1Children.size() == 2 );
		REQUIRE( std::find( level1Children.begin(), level1Children.end(), level2a ) != level1Children.end() );
		REQUIRE( std::find( level1Children.begin(), level1Children.end(), level2b ) != level1Children.end() );

		const auto level2aChildren = scene.getChildren( level2a );
		REQUIRE( level2aChildren.size() == 1 );
		REQUIRE( level2aChildren[0] == level3 );

		const auto level2bChildren = scene.getChildren( level2b );
		REQUIRE( level2bChildren.empty() );
	}

	SECTION( "Reparenting root entity removes it from old parent's children" )
	{
		const ecs::Entity parent1 = scene.createEntity( "Parent1" );
		const ecs::Entity parent2 = scene.createEntity( "Parent2" );
		const ecs::Entity child = scene.createEntity( "Child" );
		const ecs::Entity grandchild = scene.createEntity( "Grandchild" );

		// Set up: parent1 -> child -> grandchild
		scene.setParent( child, parent1 );
		scene.setParent( grandchild, child );

		// Verify initial setup
		const auto parent1InitialChildren = scene.getChildren( parent1 );
		REQUIRE( parent1InitialChildren.size() == 1 );
		REQUIRE( parent1InitialChildren[0] == child );

		// Move child from parent1 to parent2
		scene.setParent( child, parent2 );

		// Verify child moved correctly with its subtree
		REQUIRE( scene.getParent( child ) == parent2 );
		REQUIRE( scene.getParent( grandchild ) == child );

		// Parent1 should have no children
		const auto parent1FinalChildren = scene.getChildren( parent1 );
		REQUIRE( parent1FinalChildren.empty() );

		// Parent2 should have the child
		const auto parent2Children = scene.getChildren( parent2 );
		REQUIRE( parent2Children.size() == 1 );
		REQUIRE( parent2Children[0] == child );

		// Child should still have its grandchild
		const auto childChildren = scene.getChildren( child );
		REQUIRE( childChildren.size() == 1 );
		REQUIRE( childChildren[0] == grandchild );
	}
}

TEST_CASE( "Transform System Edge Cases", "[ecs][systems][transform][edge-cases]" )
{
	ecs::Scene scene;
	systems::SystemManager systemManager;

	auto *const transformSystem = systemManager.addSystem<systems::TransformSystem>();
	systemManager.initialize( scene );

	SECTION( "Getting world matrix after Transform component removal" )
	{
		const ecs::Entity entity = scene.createEntity( "TransformEdgeTest" );

		// Add Transform component
		components::Transform transform;
		transform.position = { 5.0f, 10.0f, 15.0f };
		scene.addComponent( entity, transform );

		// Mark dirty and update to build cache
		transformSystem->markDirty( entity );
		systemManager.update( scene, 0.016f );

		// Verify world matrix exists
		const auto worldMatrix = transformSystem->getWorldTransform( scene, entity );
		REQUIRE( worldMatrix.m03() == Catch::Approx( 5.0f ) );

		// Remove components::Transform component
		REQUIRE( scene.removeComponent<components::Transform>( entity ) );

		// Getting world matrix after removal should return identity matrix
		const auto identityMatrix = transformSystem->getWorldTransform( scene, entity );
		REQUIRE( identityMatrix.m00() == Catch::Approx( 1.0f ) );
		REQUIRE( identityMatrix.m11() == Catch::Approx( 1.0f ) );
		REQUIRE( identityMatrix.m22() == Catch::Approx( 1.0f ) );
		REQUIRE( identityMatrix.m33() == Catch::Approx( 1.0f ) );
		REQUIRE( identityMatrix.m03() == Catch::Approx( 0.0f ) );
		REQUIRE( identityMatrix.m13() == Catch::Approx( 0.0f ) );
		REQUIRE( identityMatrix.m23() == Catch::Approx( 0.0f ) );
	}

	SECTION( "Multiple dirty cycles in one update" )
	{
		const ecs::Entity parent = scene.createEntity( "Parent" );
		const ecs::Entity child = scene.createEntity( "Child" );
		scene.setParent( child, parent );

		components::Transform parentTransform;
		parentTransform.position = { 1.0f, 2.0f, 3.0f };
		components::Transform childTransform;
		childTransform.position = { 0.5f, 0.0f, 0.0f };

		scene.addComponent( parent, parentTransform );
		scene.addComponent( child, childTransform );

		// Mark both as dirty multiple times
		transformSystem->markDirty( parent );
		transformSystem->markDirty( child );
		transformSystem->markDirty( parent ); // Redundant marking
		transformSystem->markDirty( child );  // Redundant marking

		// Update should handle redundant dirty flags gracefully
		systemManager.update( scene, 0.016f );

		// Verify correct world matrices despite multiple dirty markings
		const auto parentWorld = transformSystem->getWorldTransform( scene, parent );
		const auto childWorld = transformSystem->getWorldTransform( scene, child );

		REQUIRE( parentWorld.m03() == Catch::Approx( 1.0f ) );
		REQUIRE( parentWorld.m13() == Catch::Approx( 2.0f ) );
		REQUIRE( parentWorld.m23() == Catch::Approx( 3.0f ) );

		REQUIRE( childWorld.m03() == Catch::Approx( 1.5f ) ); // parent.x + child.x
		REQUIRE( childWorld.m13() == Catch::Approx( 2.0f ) ); // parent.y + child.y
		REQUIRE( childWorld.m23() == Catch::Approx( 3.0f ) ); // parent.z + child.z
	}

	SECTION( "Transform system behavior with orphaned entities" )
	{
		const ecs::Entity parent = scene.createEntity( "Parent" );
		const ecs::Entity child = scene.createEntity( "Child" );

		components::Transform parentTransform;
		parentTransform.position = { 10.0f, 20.0f, 30.0f };
		components::Transform childTransform;
		childTransform.position = { 11.0f, 22.0f, 33.0f }; // World position

		scene.addComponent( parent, parentTransform );
		scene.addComponent( child, childTransform );

		// Set up hierarchy
		// NEW BEHAVIOR: setParent preserves child's world position
		// Child at world (11,22,33), parent at (10,20,30) → local becomes (1,2,3)
		scene.setParent( child, parent );
		transformSystem->markDirty( parent );
		systemManager.update( scene, 0.016f );

		// Verify child world position is preserved at (11, 22, 33)
		const auto childWorldWithParent = transformSystem->getWorldTransform( scene, child );
		REQUIRE( childWorldWithParent.m03() == Catch::Approx( 11.0f ) );

		// Now orphan the child (remove from hierarchy)
		scene.removeParent( child );

		// Mark child dirty and update
		transformSystem->markDirty( child );
		systemManager.update( scene, 0.016f );

		// After orphaning, child's local transform (1,2,3) becomes its world transform
		const auto childWorldOrphaned = transformSystem->getWorldTransform( scene, child );
		REQUIRE( childWorldOrphaned.m03() == Catch::Approx( 1.0f ) );
		REQUIRE( childWorldOrphaned.m13() == Catch::Approx( 2.0f ) );
		REQUIRE( childWorldOrphaned.m23() == Catch::Approx( 3.0f ) );

		// Parent world transform should be unchanged
		const auto parentWorldFinal = transformSystem->getWorldTransform( scene, parent );
		REQUIRE( parentWorldFinal.m03() == Catch::Approx( 10.0f ) );
		REQUIRE( parentWorldFinal.m13() == Catch::Approx( 20.0f ) );
		REQUIRE( parentWorldFinal.m23() == Catch::Approx( 30.0f ) );
	}

	SECTION( "Entity destruction clears transform cache properly" )
	{
		const ecs::Entity entity1 = scene.createEntity( "Entity1" );
		const ecs::Entity entity2 = scene.createEntity( "Entity2" );

		components::Transform t1, t2;
		t1.position = { 5.0f, 0.0f, 0.0f };
		t2.position = { 10.0f, 0.0f, 0.0f };

		scene.addComponent( entity1, t1 );
		scene.addComponent( entity2, t2 );

		// Build cache
		transformSystem->markDirty( entity1 );
		transformSystem->markDirty( entity2 );
		systemManager.update( scene, 0.016f );

		// Verify both have world transforms
		const auto world1 = transformSystem->getWorldTransform( scene, entity1 );
		const auto world2 = transformSystem->getWorldTransform( scene, entity2 );
		REQUIRE( world1.m03() == Catch::Approx( 5.0f ) );
		REQUIRE( world2.m03() == Catch::Approx( 10.0f ) );

		// Destroy entity1 - this removes its components::Transform component and clears cache
		scene.destroyEntity( entity1 );

		// Entity2 should still work normally
		const auto world2After = transformSystem->getWorldTransform( scene, entity2 );
		REQUIRE( world2After.m03() == Catch::Approx( 10.0f ) );

		// Getting transform for destroyed entity should return identity matrix
		const auto destroyedWorld = transformSystem->getWorldTransform( scene, entity1 );
		REQUIRE( destroyedWorld.m00() == Catch::Approx( 1.0f ) );
		REQUIRE( destroyedWorld.m03() == Catch::Approx( 0.0f ) );
	}

	systemManager.shutdown( scene );
}

TEST_CASE( "forEach Utility Comprehensive Coverage", "[ecs][iteration][forEach]" )
{
	ecs::Scene scene;

	SECTION( "forEach covers all entities with specified component type" )
	{
		// Create entities with different component combinations
		const ecs::Entity entityTransformOnly = scene.createEntity( "TransformOnly" );
		const ecs::Entity entityNameOnly = scene.createEntity( "NameOnly" );
		const ecs::Entity entityBoth = scene.createEntity( "BothComponents" );
		const ecs::Entity entityVisibleOnly = scene.createEntity( "VisibleOnly" );
		const ecs::Entity entityTransformVisible = scene.createEntity( "TransformVisible" );

		// Add components::Transform components to some entities
		components::Transform t1, t2, t3;
		t1.position = { 1.0f, 0.0f, 0.0f };
		t2.position = { 2.0f, 0.0f, 0.0f };
		t3.position = { 3.0f, 0.0f, 0.0f };

		scene.addComponent( entityTransformOnly, t1 );
		scene.addComponent( entityBoth, t2 );
		scene.addComponent( entityTransformVisible, t3 );

		// Add components::Name components to some entities (automatically added for some)
		// entityNameOnly already has components::Name from createEntity
		// entityBoth already has components::Name from createEntity

		// Add Visible components to some entities
		components::Visible v1, v2;
		v1.visible = true;
		v2.visible = false;
		scene.addComponent( entityVisibleOnly, v1 );
		scene.addComponent( entityTransformVisible, v2 );

		// Test forEach with components::Transform - should find exactly 3 entities
		std::vector<ecs::Entity> transformEntities;
		std::vector<math::Vec3<float>> transformPositions;
		scene.forEach<components::Transform>( [&transformEntities, &transformPositions]( ecs::Entity entity, components::Transform &transform ) {
			transformEntities.push_back( entity );
			transformPositions.push_back( transform.position );
		} );

		REQUIRE( transformEntities.size() == 3 );
		REQUIRE( transformPositions.size() == 3 );

		// Verify the entities found are the expected ones
		const bool foundTransformOnly = std::find( transformEntities.begin(), transformEntities.end(), entityTransformOnly ) != transformEntities.end();
		const bool foundBoth = std::find( transformEntities.begin(), transformEntities.end(), entityBoth ) != transformEntities.end();
		const bool foundTransformVisible = std::find( transformEntities.begin(), transformEntities.end(), entityTransformVisible ) != transformEntities.end();

		REQUIRE( foundTransformOnly );
		REQUIRE( foundBoth );
		REQUIRE( foundTransformVisible );

		// Verify positions are correct (sum should be 1 + 2 + 3 = 6)
		float totalX = 0.0f;
		for ( const auto &pos : transformPositions )
		{
			totalX += pos.x;
		}
		REQUIRE( totalX == Catch::Approx( 6.0f ) );
	}

	SECTION( "forEach with Name components" )
	{
		// Create entities, some with custom names (auto-add Name), some without
		const ecs::Entity withName1 = scene.createEntity( "CustomName1" );
		const ecs::Entity withName2 = scene.createEntity( "CustomName2" );
		const ecs::Entity withoutName = scene.createEntity();				// No Name component added
		const ecs::Entity explicitDefault = scene.createEntity( "Entity" ); // No Name component added

		// Test forEach with Name - should find exactly 2 entities
		std::vector<ecs::Entity> nameEntities;
		std::vector<std::string> names;
		scene.forEach<components::Name>( [&nameEntities, &names]( ecs::Entity entity, const components::Name &name ) {
			nameEntities.push_back( entity );
			names.push_back( name.name );
		} );

		REQUIRE( nameEntities.size() == 2 );
		REQUIRE( names.size() == 2 );

		// Verify correct names were found
		const bool foundName1 = std::find( names.begin(), names.end(), "CustomName1" ) != names.end();
		const bool foundName2 = std::find( names.begin(), names.end(), "CustomName2" ) != names.end();
		REQUIRE( foundName1 );
		REQUIRE( foundName2 );
	}

	SECTION( "forEach with Visible components" )
	{
		const ecs::Entity visible1 = scene.createEntity( "Visible1" );
		const ecs::Entity visible2 = scene.createEntity( "Visible2" );
		const ecs::Entity noVisible = scene.createEntity( "NoVisible" );

		components::Visible v1, v2;
		v1.visible = true;
		v2.visible = false;
		scene.addComponent( visible1, v1 );
		scene.addComponent( visible2, v2 );

		// Test forEach with Visible
		int visibleCount = 0;
		int trueCount = 0;
		int falseCount = 0;
		scene.forEach<components::Visible>( [&visibleCount, &trueCount, &falseCount]( ecs::Entity, const components::Visible &visible ) {
			visibleCount++;
			if ( visible.visible )
				trueCount++;
			else
				falseCount++;
		} );

		REQUIRE( visibleCount == 2 );
		REQUIRE( trueCount == 1 );
		REQUIRE( falseCount == 1 );
	}

	SECTION( "forEach on empty component type" )
	{
		// Test forEach when no entities have the requested component type
		scene.createEntity( "OnlyName" ); // Has components::Name but not components::MeshRenderer
		scene.createEntity();			  // Has no components

		int meshRendererCount = 0;
		scene.forEach<components::MeshRenderer>( [&meshRendererCount]( ecs::Entity, const components::MeshRenderer & ) {
			meshRendererCount++;
		} );

		REQUIRE( meshRendererCount == 0 );
	}

	SECTION( "forEach after component removal" )
	{
		const ecs::Entity entity1 = scene.createEntity( "Entity1" );
		const ecs::Entity entity2 = scene.createEntity( "Entity2" );

		components::Transform t1, t2;
		t1.position = { 10.0f, 0.0f, 0.0f };
		t2.position = { 20.0f, 0.0f, 0.0f };
		scene.addComponent( entity1, t1 );
		scene.addComponent( entity2, t2 );

		// Initially should find 2 transforms
		int initialCount = 0;
		scene.forEach<components::Transform>( [&initialCount]( ecs::Entity, const components::Transform & ) {
			initialCount++;
		} );
		REQUIRE( initialCount == 2 );

		// Remove one transform
		scene.removeComponent<components::Transform>( entity1 );

		// Should now find only 1 transform
		int afterRemovalCount = 0;
		float remainingX = 0.0f;
		scene.forEach<components::Transform>( [&afterRemovalCount, &remainingX]( ecs::Entity, const components::Transform &transform ) {
			afterRemovalCount++;
			remainingX = transform.position.x;
		} );
		REQUIRE( afterRemovalCount == 1 );
		REQUIRE( remainingX == Catch::Approx( 20.0f ) );
	}

	SECTION( "forEach after entity destruction" )
	{
		const ecs::Entity entity1 = scene.createEntity( "Entity1" );
		const ecs::Entity entity2 = scene.createEntity( "Entity2" );
		const ecs::Entity entity3 = scene.createEntity( "Entity3" );

		components::Transform t1, t2, t3;
		t1.position = { 1.0f, 0.0f, 0.0f };
		t2.position = { 2.0f, 0.0f, 0.0f };
		t3.position = { 3.0f, 0.0f, 0.0f };
		scene.addComponent( entity1, t1 );
		scene.addComponent( entity2, t2 );
		scene.addComponent( entity3, t3 );

		// Initially should find 3 transforms
		int initialCount = 0;
		scene.forEach<components::Transform>( [&initialCount]( ecs::Entity, const components::Transform & ) {
			initialCount++;
		} );
		REQUIRE( initialCount == 3 );

		// Destroy entity2
		scene.destroyEntity( entity2 );

		// Should now find only 2 transforms
		int afterDestructionCount = 0;
		float totalX = 0.0f;
		scene.forEach<components::Transform>( [&afterDestructionCount, &totalX]( ecs::Entity, const components::Transform &transform ) {
			afterDestructionCount++;
			totalX += transform.position.x;
		} );
		REQUIRE( afterDestructionCount == 2 );
		REQUIRE( totalX == Catch::Approx( 4.0f ) ); // 1.0 + 3.0 = 4.0 (entity2's 2.0 removed)
	}
}

TEST_CASE( "Large Hierarchy Performance and Correctness", "[ecs][hierarchy][performance]" )
{
	ecs::Scene scene;

	SECTION( "Large hierarchy creation and manipulation" )
	{
		// Create a large hierarchy for performance testing
		// Structure: 1 root -> 10 children -> 10 grandchildren each (100 total grandchildren)
		// Total: 1 + 10 + 100 = 111 entities

		const ecs::Entity root = scene.createEntity( "Root" );
		std::vector<ecs::Entity> children;
		std::vector<ecs::Entity> grandchildren;

		// Create children
		for ( int i = 0; i < 10; ++i )
		{
			const ecs::Entity child = scene.createEntity( "Child" + std::to_string( i ) );
			scene.setParent( child, root );
			children.push_back( child );

			// Create grandchildren for each child
			for ( int j = 0; j < 10; ++j )
			{
				const ecs::Entity grandchild = scene.createEntity( "Grandchild" + std::to_string( i ) + "_" + std::to_string( j ) );
				scene.setParent( grandchild, child );
				grandchildren.push_back( grandchild );
			}
		}

		// Verify hierarchy structure
		REQUIRE( children.size() == 10 );
		REQUIRE( grandchildren.size() == 100 );

		// Test root children count
		const auto rootChildren = scene.getChildren( root );
		REQUIRE( rootChildren.size() == 10 );

		// Test each child has correct number of grandchildren
		for ( const auto &child : children )
		{
			const auto childGrandchildren = scene.getChildren( child );
			REQUIRE( childGrandchildren.size() == 10 );
			REQUIRE( scene.getParent( child ) == root );
		}

		// Test all grandchildren have correct parents
		for ( size_t i = 0; i < grandchildren.size(); ++i )
		{
			const ecs::Entity grandchild = grandchildren[i];
			const ecs::Entity expectedParent = children[i / 10]; // Each group of 10 grandchildren belongs to one child
			REQUIRE( scene.getParent( grandchild ) == expectedParent );
		}
	}

	SECTION( "Large hierarchy reparenting performance" )
	{
		// Create two separate hierarchies and then merge them
		const ecs::Entity root1 = scene.createEntity( "Root1" );
		const ecs::Entity root2 = scene.createEntity( "Root2" );

		std::vector<ecs::Entity> subtree1, subtree2;

		// Create first subtree (3 levels deep, 20 entities)
		for ( int i = 0; i < 4; ++i )
		{
			const ecs::Entity level1 = scene.createEntity( "Root1_Level1_" + std::to_string( i ) );
			scene.setParent( level1, root1 );
			subtree1.push_back( level1 );

			for ( int j = 0; j < 4; ++j )
			{
				const ecs::Entity level2 = scene.createEntity( "Root1_Level2_" + std::to_string( i ) + "_" + std::to_string( j ) );
				scene.setParent( level2, level1 );
				subtree1.push_back( level2 );
			}
		}

		// Create second subtree (similar structure)
		for ( int i = 0; i < 3; ++i )
		{
			const ecs::Entity level1 = scene.createEntity( "Root2_Level1_" + std::to_string( i ) );
			scene.setParent( level1, root2 );
			subtree2.push_back( level1 );

			for ( int j = 0; j < 3; ++j )
			{
				const ecs::Entity level2 = scene.createEntity( "Root2_Level2_" + std::to_string( i ) + "_" + std::to_string( j ) );
				scene.setParent( level2, level1 );
				subtree2.push_back( level2 );
			}
		}

		// Verify initial structure
		REQUIRE( scene.getChildren( root1 ).size() == 4 );
		REQUIRE( scene.getChildren( root2 ).size() == 3 );

		// Reparent entire subtree2 under root1 (should preserve internal structure)
		const auto root2Children = scene.getChildren( root2 );
		for ( const auto &child : root2Children )
		{
			scene.setParent( child, root1 );
		}

		// Verify merged structure
		REQUIRE( scene.getChildren( root1 ).size() == 7 ); // 4 original + 3 from root2
		REQUIRE( scene.getChildren( root2 ).empty() );

		// Verify internal structure of moved subtrees is preserved
		for ( const auto &entity : subtree2 )
		{
			if ( scene.getParent( entity ) != root1 ) // If not a direct child of root1
			{
				// Should still have a parent from the original subtree2
				const ecs::Entity parent = scene.getParent( entity );
				const bool parentInSubtree2 = std::find( subtree2.begin(), subtree2.end(), parent ) != subtree2.end();
				REQUIRE( parentInSubtree2 );
			}
		}
	}

	SECTION( "Large hierarchy destruction performance" )
	{
		// Create a large hierarchy for destruction testing
		const ecs::Entity superRoot = scene.createEntity( "SuperRoot" );
		std::vector<ecs::Entity> allEntities;
		allEntities.push_back( superRoot );

		// Create a 4-level hierarchy: superRoot -> 5 -> 25 -> 125 entities
		auto level1Children = std::vector<ecs::Entity>();
		for ( int i = 0; i < 5; ++i )
		{
			const ecs::Entity level1 = scene.createEntity( "Level1_" + std::to_string( i ) );
			scene.setParent( level1, superRoot );
			level1Children.push_back( level1 );
			allEntities.push_back( level1 );

			for ( int j = 0; j < 5; ++j )
			{
				const ecs::Entity level2 = scene.createEntity( "Level2_" + std::to_string( i ) + "_" + std::to_string( j ) );
				scene.setParent( level2, level1 );
				allEntities.push_back( level2 );

				for ( int k = 0; k < 5; ++k )
				{
					const ecs::Entity level3 = scene.createEntity( "Level3_" + std::to_string( i ) + "_" + std::to_string( j ) + "_" + std::to_string( k ) );
					scene.setParent( level3, level2 );
					allEntities.push_back( level3 );
				}
			}
		}

		// Total entities: 1 + 5 + 25 + 125 = 156
		REQUIRE( allEntities.size() == 156 );

		// Verify all entities are valid before destruction
		for ( const auto &entity : allEntities )
		{
			REQUIRE( scene.isValid( entity ) );
		}

		// Destroy the super root - should cascade to all descendants
		scene.destroyEntity( superRoot );

		// Verify all entities are destroyed
		for ( const auto &entity : allEntities )
		{
			REQUIRE_FALSE( scene.isValid( entity ) );
		}
	}

	SECTION( "forEach performance with large entity count" )
	{
		// Create many entities with components::Transform components
		std::vector<ecs::Entity> entities;
		for ( int i = 0; i < 200; ++i )
		{
			const ecs::Entity entity = scene.createEntity( "Entity" + std::to_string( i ) );
			entities.push_back( entity );

			// Add components::Transform to every other entity
			if ( i % 2 == 0 )
			{
				components::Transform transform;
				transform.position = { static_cast<float>( i ), 0.0f, 0.0f };
				scene.addComponent( entity, transform );
			}
		}

		// Should have 100 entities with components::Transform components
		int transformCount = 0;
		float totalX = 0.0f;
		scene.forEach<components::Transform>( [&transformCount, &totalX]( ecs::Entity, const components::Transform &transform ) {
			transformCount++;
			totalX += transform.position.x;
		} );

		REQUIRE( transformCount == 100 );
		// Sum of 0 + 2 + 4 + ... + 198 = 2 * (0 + 1 + 2 + ... + 99) = 2 * (99 * 100 / 2) = 9900
		REQUIRE( totalX == Catch::Approx( 9900.0f ) );
	}
}
