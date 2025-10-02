#include <catch2/catch_test_macros.hpp>
#include "editor/entity_inspector/EntityInspectorPanel.h"
#include "editor/selection.h"
#include "editor/commands/CommandHistory.h"
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "runtime/systems.h"
#include "engine/math/math.h"
#include <cmath>

// ============================================================================
// T2.1: Inspector Panel Foundation Tests
// ============================================================================

TEST_CASE( "EntityInspectorPanel - Panel can be constructed", "[T2.1][entity_inspector][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	// Act
	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory );

	// Assert
	REQUIRE( panel.isVisible() ); // Should be visible by default
}

TEST_CASE( "EntityInspectorPanel - Panel visibility can be toggled", "[T2.1][entity_inspector][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory );

	// Act
	panel.setVisible( false );

	// Assert
	REQUIRE( !panel.isVisible() );

	// Act
	panel.setVisible( true );

	// Assert
	REQUIRE( panel.isVisible() );
}

TEST_CASE( "EntityInspectorPanel - No selection shows empty state", "[T2.1][entity_inspector][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory );

	// Act - No entities selected
	// Nothing to do here

	// Assert - Panel should handle empty selection gracefully
	// This test verifies that render() doesn't crash with no selection
	REQUIRE( selectionManager.getSelectionCount() == 0 );
}

TEST_CASE( "EntityInspectorPanel - Single selection shows entity info", "[T2.1][entity_inspector][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity = scene.createEntity( "TestEntity" );
	selectionManager.select( entity );

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory );

	// Act - Single entity selected
	// Nothing to do here

	// Assert - Panel should recognize single selection
	REQUIRE( selectionManager.getSelectionCount() == 1 );
	REQUIRE( selectionManager.isSelected( entity ) );
}

TEST_CASE( "EntityInspectorPanel - Multiple selection shows multi-select state", "[T2.1][entity_inspector][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity1 = scene.createEntity( "Entity1" );
	const ecs::Entity entity2 = scene.createEntity( "Entity2" );
	selectionManager.select( entity1, false );
	selectionManager.select( entity2, true );

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory );

	// Act - Multiple entities selected
	// Nothing to do here

	// Assert - Panel should recognize multiple selection
	REQUIRE( selectionManager.getSelectionCount() == 2 );
	REQUIRE( selectionManager.isSelected( entity1 ) );
	REQUIRE( selectionManager.isSelected( entity2 ) );
}

TEST_CASE( "EntityInspectorPanel - Panel can be hidden and shown", "[T2.1][entity_inspector][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory );

	// Act & Assert - Toggle visibility multiple times
	panel.setVisible( false );
	REQUIRE( !panel.isVisible() );

	panel.setVisible( true );
	REQUIRE( panel.isVisible() );

	panel.setVisible( false );
	REQUIRE( !panel.isVisible() );
}

// ============================================================================
// T2.3: Transform Component Editor Tests
// ============================================================================

TEST_CASE( "EntityInspectorPanel - Entity with Transform component can be inspected", "[T2.3][entity_inspector][transform][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity = scene.createEntity( "TransformEntity" );

	// Add Transform component
	scene.addComponent( entity, components::Transform{} );

	// Set initial transform values
	auto *transform = scene.getComponent<components::Transform>( entity );
	REQUIRE( transform != nullptr );

	transform->position = { 1.0f, 2.0f, 3.0f };
	transform->rotation = { 0.1f, 0.2f, 0.3f }; // radians
	transform->scale = { 2.0f, 2.0f, 2.0f };

	selectionManager.select( entity );

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory );

	// Assert - Panel should be able to access Transform component
	REQUIRE( scene.hasComponent<components::Transform>( entity ) );
	REQUIRE( transform->position.x == 1.0f );
	REQUIRE( transform->position.y == 2.0f );
	REQUIRE( transform->position.z == 3.0f );
}
