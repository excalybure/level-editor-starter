#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "runtime/ecs.h"
#include "runtime/systems.h"

TEST_CASE( "Reparenting - Child retains world position when moved to new parent", "[reparenting][transform][hierarchy][preserve]" )
{
	// This test verifies that when reparenting an entity to a new parent,
	// the child RETAINS its world position by adjusting its local transform
	// rather than moving with the new parent.
	
	// Arrange: Create scene with transform system
	ecs::Scene scene;
	systems::SystemManager systemManager;
	auto *transformSystem = systemManager.addSystem<systems::TransformSystem>();
	systemManager.initialize( scene );

	// Create two potential parents and a child
	const ecs::Entity parent1 = scene.createEntity( "Parent1" );
	const ecs::Entity parent2 = scene.createEntity( "Parent2" );
	const ecs::Entity child = scene.createEntity( "Child" );

	// Parent1 at origin, Parent2 at (100, 0, 0)
	components::Transform parent1Transform;
	parent1Transform.position = { 0.0f, 0.0f, 0.0f };
	scene.addComponent( parent1, parent1Transform );

	components::Transform parent2Transform;
	parent2Transform.position = { 100.0f, 0.0f, 0.0f };
	scene.addComponent( parent2, parent2Transform );

	// Child at (10, 0, 0) local offset (world position is also (10, 0, 0) since parent1 is at origin)
	components::Transform childTransform;
	childTransform.position = { 10.0f, 0.0f, 0.0f };
	scene.addComponent( child, childTransform );

	// Initially parent child to parent1
	scene.setParent( child, parent1 );
	systemManager.update( scene, 0.016f );

	// Verify child's world position is parent1(0) + child(10) = (10, 0, 0)
	const auto childWorldWithParent1 = transformSystem->getWorldTransform( scene, child );
	REQUIRE( childWorldWithParent1.m03() == Catch::Approx( 10.0f ).epsilon( 0.001f ) );

	// Act: Reparent child from parent1 to parent2
	// Desired behavior: child should RETAIN its world position (10,0,0)
	// This means its local transform should be adjusted to (-90, 0, 0) relative to parent2 at (100,0,0)
	scene.setParent( child, parent2 );
	systemManager.update( scene, 0.016f );

	// Assert: Child should remain at world position (10, 0, 0)
	const auto childWorldWithParent2 = transformSystem->getWorldTransform( scene, child );
	REQUIRE( childWorldWithParent2.m03() == Catch::Approx( 10.0f ).epsilon( 0.001f ) );
	
	// Verify local transform was adjusted: should be (10 - 100) = (-90, 0, 0)
	const auto *childTransformPtr = scene.getComponent<components::Transform>( child );
	REQUIRE( childTransformPtr->position.x == Catch::Approx( -90.0f ).epsilon( 0.001f ) );

	systemManager.shutdown( scene );
}

TEST_CASE( "Reparenting - Moving parent updates child world position", "[reparenting][transform][hierarchy]" )
{
	// Arrange: Create scene with transform system
	ecs::Scene scene;
	systems::SystemManager systemManager;
	auto *transformSystem = systemManager.addSystem<systems::TransformSystem>();
	systemManager.initialize( scene );

	// Create parent and child entities
	const ecs::Entity parent = scene.createEntity( "Parent" );
	const ecs::Entity child = scene.createEntity( "Child" );

	// Add transforms
	components::Transform parentTransform;
	parentTransform.position = { 0.0f, 0.0f, 0.0f };
	scene.addComponent( parent, parentTransform );

	components::Transform childTransform;
	childTransform.position = { 1.0f, 0.0f, 0.0f }; // Local offset
	scene.addComponent( child, childTransform );

	// Set up hierarchy
	scene.setParent( child, parent );

	// Initial system update
	systemManager.update( scene, 0.016f );

	// Verify initial child world position
	const auto childWorldInitial = transformSystem->getWorldTransform( scene, child );
	REQUIRE( childWorldInitial.m03() == Catch::Approx( 1.0f ).epsilon( 0.001f ) ); // Parent(0) + Child(1)

	// Act: Move parent to new position
	auto *parentTransformPtr = scene.getComponent<components::Transform>( parent );
	parentTransformPtr->position.x = 10.0f;
	parentTransformPtr->markDirty();
	transformSystem->markDirty( parent );

	// Update system to recalculate world matrices
	systemManager.update( scene, 0.016f );

	// Assert: Child world position should now reflect parent's new position
	const auto childWorldAfterMove = transformSystem->getWorldTransform( scene, child );
	REQUIRE( childWorldAfterMove.m03() == Catch::Approx( 11.0f ).epsilon( 0.001f ) ); // Parent(10) + Child(1)
}

TEST_CASE( "Reparenting - Moving parent with gizmo updates child world position", "[reparenting][transform][gizmo][hierarchy]" )
{
	// Arrange: Create scene with transform system
	ecs::Scene scene;
	systems::SystemManager systemManager;
	auto *transformSystem = systemManager.addSystem<systems::TransformSystem>();
	systemManager.initialize( scene );

	// Create parent with two children
	const ecs::Entity parent = scene.createEntity( "Parent" );
	const ecs::Entity child1 = scene.createEntity( "Child1" );
	const ecs::Entity child2 = scene.createEntity( "Child2" );

	// Add transforms
	components::Transform parentTransform;
	parentTransform.position = { 0.0f, 0.0f, 0.0f };
	scene.addComponent( parent, parentTransform );

	components::Transform child1Transform;
	child1Transform.position = { 1.0f, 0.0f, 0.0f }; // Local offset
	scene.addComponent( child1, child1Transform );

	components::Transform child2Transform;
	child2Transform.position = { 0.0f, 2.0f, 0.0f }; // Local offset
	scene.addComponent( child2, child2Transform );

	// Set up hierarchy
	scene.setParent( child1, parent );
	scene.setParent( child2, parent );

	// Initial system update
	systemManager.update( scene, 0.016f );

	// Verify initial world positions
	const auto child1WorldInitial = transformSystem->getWorldTransform( scene, child1 );
	const auto child2WorldInitial = transformSystem->getWorldTransform( scene, child2 );
	REQUIRE( child1WorldInitial.m03() == Catch::Approx( 1.0f ).epsilon( 0.001f ) );
	REQUIRE( child2WorldInitial.m13() == Catch::Approx( 2.0f ).epsilon( 0.001f ) );

	// Act: Simulate gizmo move - update parent transform directly
	auto *parentTransformPtr = scene.getComponent<components::Transform>( parent );
	parentTransformPtr->position.x = 5.0f;
	parentTransformPtr->position.y = 3.0f;
	parentTransformPtr->markDirty();

	// This is what gizmo system does
	transformSystem->markDirty( parent );

	// Update system
	systemManager.update( scene, 0.016f );

	// Assert: Both children should reflect parent's new position
	const auto child1WorldAfter = transformSystem->getWorldTransform( scene, child1 );
	const auto child2WorldAfter = transformSystem->getWorldTransform( scene, child2 );

	REQUIRE( child1WorldAfter.m03() == Catch::Approx( 6.0f ).epsilon( 0.001f ) ); // Parent(5) + Child1(1)
	REQUIRE( child1WorldAfter.m13() == Catch::Approx( 3.0f ).epsilon( 0.001f ) ); // Parent(3) + Child1(0)
	REQUIRE( child2WorldAfter.m03() == Catch::Approx( 5.0f ).epsilon( 0.001f ) ); // Parent(5) + Child2(0)
	REQUIRE( child2WorldAfter.m13() == Catch::Approx( 5.0f ).epsilon( 0.001f ) ); // Parent(3) + Child2(2)

	systemManager.shutdown( scene );
}

TEST_CASE( "Reparenting - Deep hierarchy updates when grandparent moves", "[reparenting][transform][hierarchy][deep]" )
{
	// Arrange: Create scene with transform system
	ecs::Scene scene;
	systems::SystemManager systemManager;
	auto *transformSystem = systemManager.addSystem<systems::TransformSystem>();
	systemManager.initialize( scene );

	// Create grandparent -> parent -> child hierarchy
	const ecs::Entity grandparent = scene.createEntity( "Grandparent" );
	const ecs::Entity parent = scene.createEntity( "Parent" );
	const ecs::Entity child = scene.createEntity( "Child" );

	// Add transforms
	components::Transform grandparentTransform;
	grandparentTransform.position = { 0.0f, 0.0f, 0.0f };
	scene.addComponent( grandparent, grandparentTransform );

	components::Transform parentTransform;
	parentTransform.position = { 1.0f, 0.0f, 0.0f }; // World position
	scene.addComponent( parent, parentTransform );

	components::Transform childTransform;
	childTransform.position = { 2.0f, 0.0f, 0.0f }; // World position (will become local after reparenting)
	scene.addComponent( child, childTransform );

	// Set up hierarchy
	// NEW BEHAVIOR: setParent preserves world positions
	// Parent at world (1,0,0) under grandparent(0,0,0) → local stays (1,0,0)
	scene.setParent( parent, grandparent );
	// Child at world (2,0,0) under parent(1,0,0) → local becomes (1,0,0)
	scene.setParent( child, parent );

	// Initial system update
	systemManager.update( scene, 0.016f );

	// Verify initial positions - child world should be preserved at (2,0,0)
	const auto childWorldInitial = transformSystem->getWorldTransform( scene, child );
	REQUIRE( childWorldInitial.m03() == Catch::Approx( 2.0f ).epsilon( 0.001f ) ); // GP(0) + P(1) + C_local(1)

	// Act: Move grandparent
	auto *grandparentTransformPtr = scene.getComponent<components::Transform>( grandparent );
	grandparentTransformPtr->position.x = 10.0f;
	grandparentTransformPtr->markDirty();
	transformSystem->markDirty( grandparent );

	// Update system
	systemManager.update( scene, 0.016f );

	// Assert: Child should reflect grandparent's move through parent
	const auto childWorldAfter = transformSystem->getWorldTransform( scene, child );
	REQUIRE( childWorldAfter.m03() == Catch::Approx( 12.0f ).epsilon( 0.001f ) ); // GP(10) + P(1) + C(1)

	systemManager.shutdown( scene );
}
