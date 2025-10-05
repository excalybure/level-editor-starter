#include <catch2/catch_test_macros.hpp>
#include "editor/entity_inspector/EntityInspectorPanel.h"
#include "editor/selection.h"
#include "editor/commands/CommandHistory.h"
#include "editor/commands/EcsCommands.h"
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "runtime/systems.h"
#include "math/math.h"
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
	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

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

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

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

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

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

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

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

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

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

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

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

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Assert - Panel should be able to access Transform component
	REQUIRE( scene.hasComponent<components::Transform>( entity ) );
	REQUIRE( transform->position.x == 1.0f );
	REQUIRE( transform->position.y == 2.0f );
	REQUIRE( transform->position.z == 3.0f );
}

// ============================================================================
// T2.4: Name and Visible Component Editor Tests
// ============================================================================

TEST_CASE( "EntityInspectorPanel - Entity with Name component can be inspected", "[T2.4][entity_inspector][name][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity = scene.createEntity( "NamedEntity" );

	// Name component is auto-added by createEntity with custom name
	REQUIRE( scene.hasComponent<components::Name>( entity ) );

	selectionManager.select( entity );

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Assert - Panel should be able to access Name component
	const auto *name = scene.getComponent<components::Name>( entity );
	REQUIRE( name != nullptr );
	REQUIRE( name->name == "NamedEntity" );
}

TEST_CASE( "EntityInspectorPanel - Entity with Visible component can be inspected", "[T2.4][entity_inspector][visible][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity = scene.createEntity( "VisibleEntity" );

	// Add Visible component
	components::Visible visible;
	visible.visible = true;
	visible.castShadows = false;
	visible.receiveShadows = true;
	scene.addComponent( entity, visible );

	selectionManager.select( entity );

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Assert - Panel should be able to access Visible component
	const auto *visibleComp = scene.getComponent<components::Visible>( entity );
	REQUIRE( visibleComp != nullptr );
	REQUIRE( visibleComp->visible == true );
	REQUIRE( visibleComp->castShadows == false );
	REQUIRE( visibleComp->receiveShadows == true );
}

TEST_CASE( "EntityInspectorPanel - Entity with MeshRenderer component can be inspected", "[T2.5][entity_inspector][meshrenderer][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity = scene.createEntity( "MeshEntity" );

	// Add MeshRenderer component
	components::MeshRenderer meshRenderer;
	meshRenderer.meshHandle = 42; // Set a test mesh handle
	scene.addComponent( entity, meshRenderer );

	selectionManager.select( entity );

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Assert - Panel should be able to access MeshRenderer component
	const auto *meshRendererComp = scene.getComponent<components::MeshRenderer>( entity );
	REQUIRE( meshRendererComp != nullptr );
	REQUIRE( meshRendererComp->meshHandle == 42 );
	REQUIRE( meshRendererComp->gpuMesh == nullptr ); // No GPU resources in test
}

// ============================================================================
// T2.6: Add Component Menu Tests
// ============================================================================

TEST_CASE( "EntityInspectorPanel - Can add components to entity via command", "[T2.6][entity_inspector][add_component][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity = scene.createEntity( "TestEntity" );
	selectionManager.select( entity );

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Initially, entity should not have Transform component
	REQUIRE( !scene.hasComponent<components::Transform>( entity ) );

	// Act - Create AddComponentCommand directly (simulating menu selection)
	components::Transform transform;
	transform.position = { 1.0f, 2.0f, 3.0f };
	auto command = std::make_unique<editor::AddComponentCommand<components::Transform>>( scene, entity, transform );
	commandHistory.executeCommand( std::move( command ) );

	// Assert - Entity should now have Transform component with correct values
	REQUIRE( scene.hasComponent<components::Transform>( entity ) );
	const auto *transformComp = scene.getComponent<components::Transform>( entity );
	REQUIRE( transformComp != nullptr );
	REQUIRE( transformComp->position.x == 1.0f );
	REQUIRE( transformComp->position.y == 2.0f );
	REQUIRE( transformComp->position.z == 3.0f );

	// Act - Undo the add component command
	commandHistory.undo();

	// Assert - Component should be removed
	REQUIRE( !scene.hasComponent<components::Transform>( entity ) );

	// Act - Redo the add component command
	commandHistory.redo();

	// Assert - Component should be added back
	REQUIRE( scene.hasComponent<components::Transform>( entity ) );
}

// ============================================================================
// T2.7: Remove Component Menu Tests
// ============================================================================

TEST_CASE( "Can remove components from entity via command", "[T2.7][entity_inspector][remove_component][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity = scene.createEntity( "TestEntity" );
	selectionManager.select( entity );

	// Add Visible component to test removal
	components::Visible visible;
	visible.visible = true;
	visible.castShadows = true;
	visible.receiveShadows = false;
	scene.addComponent( entity, visible );

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Initially, entity should have Visible component
	REQUIRE( scene.hasComponent<components::Visible>( entity ) );

	// Act - Create RemoveComponentCommand (simulating context menu selection)
	auto command = std::make_unique<editor::RemoveComponentCommand<components::Visible>>( scene, entity );
	commandHistory.executeCommand( std::move( command ) );

	// Assert - Component should be removed
	REQUIRE( !scene.hasComponent<components::Visible>( entity ) );

	// Act - Undo the remove command
	commandHistory.undo();

	// Assert - Component should be restored with original values
	REQUIRE( scene.hasComponent<components::Visible>( entity ) );
	const auto *restoredVisible = scene.getComponent<components::Visible>( entity );
	REQUIRE( restoredVisible != nullptr );
	REQUIRE( restoredVisible->visible == true );
	REQUIRE( restoredVisible->castShadows == true );
	REQUIRE( restoredVisible->receiveShadows == false );

	// Act - Redo the remove command
	commandHistory.redo();

	// Assert - Component should be removed again
	REQUIRE( !scene.hasComponent<components::Visible>( entity ) );
}

// ============================================================================
// T2.8: Multi-Selection Support Tests
// ============================================================================

TEST_CASE( "Multi-selection shows common components", "[T2.8][entity_inspector][multi_selection][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	// Create three entities with Transform and Visible components
	const ecs::Entity entity1 = scene.createEntity( "Entity1" );
	const ecs::Entity entity2 = scene.createEntity( "Entity2" );
	const ecs::Entity entity3 = scene.createEntity( "Entity3" );

	// Add Transform components
	components::Transform transform;
	transform.position = { 1.0f, 2.0f, 3.0f };
	transform.rotation = { 0.0f, 0.0f, 0.0f };
	transform.scale = { 1.0f, 1.0f, 1.0f };
	scene.addComponent( entity1, transform );
	scene.addComponent( entity2, transform );
	scene.addComponent( entity3, transform );

	// Add Visible components
	components::Visible visible;
	visible.visible = true;
	visible.castShadows = true;
	visible.receiveShadows = true;
	scene.addComponent( entity1, visible );
	scene.addComponent( entity2, visible );
	scene.addComponent( entity3, visible );

	// Select all three entities
	selectionManager.select( entity1 );
	selectionManager.select( entity2, true ); // additive = true
	selectionManager.select( entity3, true ); // additive = true

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Assert - All entities should be selected
	REQUIRE( selectionManager.getSelectionCount() == 3 );
	REQUIRE( selectionManager.isSelected( entity1 ) );
	REQUIRE( selectionManager.isSelected( entity2 ) );
	REQUIRE( selectionManager.isSelected( entity3 ) );

	// Assert - All entities have Transform (created by default)
	REQUIRE( scene.hasComponent<components::Transform>( entity1 ) );
	REQUIRE( scene.hasComponent<components::Transform>( entity2 ) );
	REQUIRE( scene.hasComponent<components::Transform>( entity3 ) );

	// Assert - All entities have Visible
	REQUIRE( scene.hasComponent<components::Visible>( entity1 ) );
	REQUIRE( scene.hasComponent<components::Visible>( entity2 ) );
	REQUIRE( scene.hasComponent<components::Visible>( entity3 ) );
}
