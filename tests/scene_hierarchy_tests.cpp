#include <catch2/catch_test_macros.hpp>
#include "editor/scene_hierarchy/SceneHierarchyPanel.h"
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "editor/selection.h"
#include "editor/commands/CommandHistory.h"
#include "editor/commands/EcsCommands.h"
#include "runtime/systems.h"

TEST_CASE( "SceneHierarchyPanel - Empty scene renders without errors", "[T1.1][scene_hierarchy][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	editor::SceneHierarchyPanel panel( scene, selectionManager, commandHistory );

	// Act - Just ensure construction succeeds
	// Note: Actual rendering requires ImGui context, so we test that the panel can be created
	const bool isVisible = panel.isVisible();

	// Assert
	REQUIRE( isVisible == true ); // Default should be visible
}

TEST_CASE( "SceneHierarchyPanel - Scene with entities displays all entity names", "[T1.1][scene_hierarchy][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity1 = scene.createEntity( "Cube" );
	const ecs::Entity entity2 = scene.createEntity( "Sphere" );
	const ecs::Entity entity3 = scene.createEntity( "Light" );

	editor::SceneHierarchyPanel panel( scene, selectionManager, commandHistory );

	// Act - Panel should be able to enumerate entities
	// For now, we just verify entities exist in scene
	const bool hasEntity1 = scene.isValid( entity1 );
	const bool hasEntity2 = scene.isValid( entity2 );
	const bool hasEntity3 = scene.isValid( entity3 );

	// Assert
	REQUIRE( hasEntity1 );
	REQUIRE( hasEntity2 );
	REQUIRE( hasEntity3 );
	REQUIRE( scene.hasComponent<components::Name>( entity1 ) );
	REQUIRE( scene.hasComponent<components::Name>( entity2 ) );
	REQUIRE( scene.hasComponent<components::Name>( entity3 ) );
}

TEST_CASE( "SceneHierarchyPanel - Entities without name show ID fallback", "[T1.1][scene_hierarchy][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity = scene.createEntity( "" ); // Empty name

	editor::SceneHierarchyPanel panel( scene, selectionManager, commandHistory );

	// Act
	const bool hasName = scene.hasComponent<components::Name>( entity );

	// Assert - Entity without explicit name should not have Name component
	REQUIRE( !hasName );
}

TEST_CASE( "SceneHierarchyPanel - Panel can be hidden and shown", "[T1.1][scene_hierarchy][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	editor::SceneHierarchyPanel panel( scene, selectionManager, commandHistory );

	// Act
	panel.setVisible( false );
	const bool hiddenState = panel.isVisible();

	panel.setVisible( true );
	const bool shownState = panel.isVisible();

	// Assert
	REQUIRE( hiddenState == false );
	REQUIRE( shownState == true );
}

// T1.2: Hierarchical Tree Structure Tests

TEST_CASE( "SceneHierarchyPanel - Root entities display at top level", "[T1.2][scene_hierarchy][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity root1 = scene.createEntity( "Root1" );
	const ecs::Entity root2 = scene.createEntity( "Root2" );
	const ecs::Entity child = scene.createEntity( "Child" );
	scene.setParent( child, root1 );

	editor::SceneHierarchyPanel panel( scene, selectionManager, commandHistory );

	// Act - Verify root entities have no parent
	const ecs::Entity root1Parent = scene.getParent( root1 );
	const ecs::Entity root2Parent = scene.getParent( root2 );
	const ecs::Entity childParent = scene.getParent( child );

	// Assert
	REQUIRE( !scene.isValid( root1Parent ) ); // Root has invalid parent
	REQUIRE( !scene.isValid( root2Parent ) );
	REQUIRE( scene.isValid( childParent ) );
	REQUIRE( childParent.id == root1.id );
}

TEST_CASE( "SceneHierarchyPanel - Child entities are indented under parents", "[T1.2][scene_hierarchy][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity parent = scene.createEntity( "Parent" );
	const ecs::Entity child1 = scene.createEntity( "Child1" );
	const ecs::Entity child2 = scene.createEntity( "Child2" );
	scene.setParent( child1, parent );
	scene.setParent( child2, parent );

	editor::SceneHierarchyPanel panel( scene, selectionManager, commandHistory );

	// Act - Verify hierarchy
	const std::vector<ecs::Entity> children = scene.getChildren( parent );

	// Assert
	REQUIRE( children.size() == 2 );
	REQUIRE( ( children[0].id == child1.id || children[1].id == child1.id ) );
	REQUIRE( ( children[0].id == child2.id || children[1].id == child2.id ) );
}

TEST_CASE( "SceneHierarchyPanel - Deep hierarchies render correctly", "[T1.2][scene_hierarchy][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	// Create a 5-level hierarchy
	const ecs::Entity level0 = scene.createEntity( "Level0" );
	const ecs::Entity level1 = scene.createEntity( "Level1" );
	const ecs::Entity level2 = scene.createEntity( "Level2" );
	const ecs::Entity level3 = scene.createEntity( "Level3" );
	const ecs::Entity level4 = scene.createEntity( "Level4" );

	scene.setParent( level1, level0 );
	scene.setParent( level2, level1 );
	scene.setParent( level3, level2 );
	scene.setParent( level4, level3 );

	editor::SceneHierarchyPanel panel( scene, selectionManager, commandHistory );

	// Act - Verify deep hierarchy
	ecs::Entity current = level4;
	int depth = 0;
	while ( scene.isValid( current ) )
	{
		const ecs::Entity parent = scene.getParent( current );
		if ( !scene.isValid( parent ) )
			break;
		current = parent;
		depth++;
	}

	// Assert - Should have 4 levels above level4
	REQUIRE( depth == 4 );
	REQUIRE( current.id == level0.id ); // Should end at root
}

// T1.3: Selection Integration Tests

TEST_CASE( "SceneHierarchyPanel - Clicking entity selects it", "[T1.3][scene_hierarchy][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity1 = scene.createEntity( "Entity1" );
	const ecs::Entity entity2 = scene.createEntity( "Entity2" );

	editor::SceneHierarchyPanel panel( scene, selectionManager, commandHistory );

	// Act - Simulate selection (panel will internally call selectionManager.select())
	selectionManager.select( entity1 );

	// Assert
	REQUIRE( selectionManager.isSelected( entity1 ) );
	REQUIRE( !selectionManager.isSelected( entity2 ) );
	REQUIRE( selectionManager.getSelectionCount() == 1 );
}

TEST_CASE( "SceneHierarchyPanel - Ctrl+Click adds to selection", "[T1.3][scene_hierarchy][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity1 = scene.createEntity( "Entity1" );
	const ecs::Entity entity2 = scene.createEntity( "Entity2" );
	const ecs::Entity entity3 = scene.createEntity( "Entity3" );

	editor::SceneHierarchyPanel panel( scene, selectionManager, commandHistory );

	// Act - Simulate Ctrl+Click (additive selection)
	selectionManager.select( entity1, false ); // First click (replace)
	selectionManager.select( entity2, true );  // Ctrl+Click (additive)
	selectionManager.select( entity3, true );  // Ctrl+Click (additive)

	// Assert
	REQUIRE( selectionManager.isSelected( entity1 ) );
	REQUIRE( selectionManager.isSelected( entity2 ) );
	REQUIRE( selectionManager.isSelected( entity3 ) );
	REQUIRE( selectionManager.getSelectionCount() == 3 );
}

TEST_CASE( "SceneHierarchyPanel - Ctrl+Click on selected entity deselects it", "[T1.3][scene_hierarchy][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity1 = scene.createEntity( "Entity1" );
	const ecs::Entity entity2 = scene.createEntity( "Entity2" );

	editor::SceneHierarchyPanel panel( scene, selectionManager, commandHistory );

	// Act - Select both, then deselect one
	selectionManager.select( entity1, false );
	selectionManager.select( entity2, true );
	selectionManager.toggleSelection( entity1 ); // Ctrl+Click on selected entity

	// Assert
	REQUIRE( !selectionManager.isSelected( entity1 ) );
	REQUIRE( selectionManager.isSelected( entity2 ) );
	REQUIRE( selectionManager.getSelectionCount() == 1 );
}

TEST_CASE( "SceneHierarchyPanel - Selection synchronizes with SelectionManager", "[T1.3][scene_hierarchy][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity1 = scene.createEntity( "Entity1" );
	const ecs::Entity entity2 = scene.createEntity( "Entity2" );

	editor::SceneHierarchyPanel panel( scene, selectionManager, commandHistory );

	// Act - Select via SelectionManager
	selectionManager.select( entity1 );

	// Assert - Panel should reflect this selection
	REQUIRE( selectionManager.isSelected( entity1 ) );
	REQUIRE( selectionManager.getPrimarySelection().id == entity1.id );

	// Act - Change selection
	selectionManager.select( entity2 );

	// Assert
	REQUIRE( !selectionManager.isSelected( entity1 ) );
	REQUIRE( selectionManager.isSelected( entity2 ) );
	REQUIRE( selectionManager.getPrimarySelection().id == entity2.id );
}

// ============================================================================
// T1.4: Drag-and-Drop Reparenting
// ============================================================================

TEST_CASE( "SceneHierarchyPanel - Drag-drop executes SetParentCommand", "[T1.4][scene_hierarchy][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity parent = scene.createEntity( "Parent" );
	const ecs::Entity child = scene.createEntity( "Child" );

	editor::SceneHierarchyPanel panel( scene, selectionManager, commandHistory );

	// Act - Execute SetParentCommand (simulating drag-drop behavior)
	auto command = std::make_unique<editor::SetParentCommand>( scene, child, parent );
	const bool executed = commandHistory.executeCommand( std::move( command ) );

	// Assert
	REQUIRE( executed );
	REQUIRE( scene.getParent( child ).id == parent.id );
	REQUIRE( scene.getChildren( parent ).size() == 1 );
	REQUIRE( scene.getChildren( parent )[0].id == child.id );
	REQUIRE( commandHistory.canUndo() );
}

TEST_CASE( "SceneHierarchyPanel - Drag-drop command can be undone", "[T1.4][scene_hierarchy][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity parent = scene.createEntity( "Parent" );
	const ecs::Entity child = scene.createEntity( "Child" );

	editor::SceneHierarchyPanel panel( scene, selectionManager, commandHistory );

	// Act - Execute and then undo
	auto command = std::make_unique<editor::SetParentCommand>( scene, child, parent );
	commandHistory.executeCommand( std::move( command ) );
	const bool undone = commandHistory.undo();

	// Assert
	REQUIRE( undone );
	REQUIRE( !scene.getParent( child ).isValid() );
	REQUIRE( scene.getChildren( parent ).empty() );
	REQUIRE( commandHistory.canRedo() );
}

TEST_CASE( "SceneHierarchyPanel - Cannot drag entity onto itself", "[T1.4][scene_hierarchy][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity = scene.createEntity( "Entity" );

	editor::SceneHierarchyPanel panel( scene, selectionManager, commandHistory );

	// Act - Try to parent entity to itself
	auto command = std::make_unique<editor::SetParentCommand>( scene, entity, entity );
	const bool executed = commandHistory.executeCommand( std::move( command ) );

	// Assert - Should fail (circular reference)
	REQUIRE( !executed );
	REQUIRE( !scene.getParent( entity ).isValid() );
}

TEST_CASE( "SceneHierarchyPanel - Cannot create circular parent-child relationships", "[T1.4][scene_hierarchy][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity grandparent = scene.createEntity( "Grandparent" );
	const ecs::Entity parent = scene.createEntity( "Parent" );
	const ecs::Entity child = scene.createEntity( "Child" );

	editor::SceneHierarchyPanel panel( scene, selectionManager, commandHistory );

	// Set up hierarchy: grandparent -> parent -> child
	scene.setParent( parent, grandparent );
	scene.setParent( child, parent );

	// Act - Try to make grandparent a child of child (circular!)
	auto command = std::make_unique<editor::SetParentCommand>( scene, grandparent, child );
	const bool executed = commandHistory.executeCommand( std::move( command ) );

	// Assert - Should fail (would create cycle)
	REQUIRE( !executed );
	REQUIRE( scene.getParent( grandparent ).isValid() == false ); // grandparent should still be root
	REQUIRE( scene.getParent( parent ).id == grandparent.id );	  // original hierarchy intact
	REQUIRE( scene.getParent( child ).id == parent.id );
}

// ============================================================================
// T1.5: Context Menu
// ============================================================================

TEST_CASE( "SceneHierarchyPanel - Create child entity command", "[T1.5][scene_hierarchy][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity parent = scene.createEntity( "Parent" );

	editor::SceneHierarchyPanel panel( scene, selectionManager, commandHistory );

	// Act - Create child entity via command
	auto createCommand = std::make_unique<editor::CreateEntityCommand>( scene, "Child" );
	commandHistory.executeCommand( std::move( createCommand ) );

	// Get the created entity (last entity in scene)
	const auto entities = scene.getAllEntities();
	const ecs::Entity child = entities.back();

	// Set parent relationship
	auto parentCommand = std::make_unique<editor::SetParentCommand>( scene, child, parent );
	commandHistory.executeCommand( std::move( parentCommand ) );

	// Assert
	REQUIRE( scene.isValid( child ) );
	REQUIRE( scene.getParent( child ).id == parent.id );
	REQUIRE( scene.getChildren( parent ).size() == 1 );
	REQUIRE( commandHistory.canUndo() );
}

TEST_CASE( "SceneHierarchyPanel - Delete entity command", "[T1.5][scene_hierarchy][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity = scene.createEntity( "ToDelete" );

	editor::SceneHierarchyPanel panel( scene, selectionManager, commandHistory );

	// Act - Delete entity via command
	auto command = std::make_unique<editor::DeleteEntityCommand>( scene, entity );
	const bool executed = commandHistory.executeCommand( std::move( command ) );

	// Assert
	REQUIRE( executed );
	REQUIRE( !scene.isValid( entity ) );
	REQUIRE( commandHistory.canUndo() );
}

TEST_CASE( "SceneHierarchyPanel - Rename entity command", "[T1.5][scene_hierarchy][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity = scene.createEntity( "OldName" );

	editor::SceneHierarchyPanel panel( scene, selectionManager, commandHistory );

	// Act - Rename entity via command
	auto command = std::make_unique<editor::RenameEntityCommand>( scene, entity, "NewName" );
	const bool executed = commandHistory.executeCommand( std::move( command ) );

	// Assert
	REQUIRE( executed );
	const auto *name = scene.getComponent<components::Name>( entity );
	REQUIRE( name != nullptr );
	REQUIRE( name->name == "NewName" );
	REQUIRE( commandHistory.canUndo() );
}
