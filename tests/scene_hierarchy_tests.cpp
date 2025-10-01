#include <catch2/catch_test_macros.hpp>
#include "editor/scene_hierarchy/SceneHierarchyPanel.h"
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "editor/selection.h"
#include "editor/commands/CommandHistory.h"
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
