#include <catch2/catch_test_macros.hpp>

#include "editor/gizmos.h"
#include "editor/selection.h"
#include "runtime/ecs.h"
#include "runtime/systems.h"
#include "runtime/components.h"

TEST_CASE( "Gizmo operation switching preserves selection", "[gizmos][selection][integration-fix]" )
{
	SECTION( "Selection remains active when switching gizmo operations" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );
		editor::GizmoUI gizmoUI( gizmoSystem );

		// Create and select an entity
		const auto entity = scene.createEntity();
		scene.addComponent<components::Transform>( entity,
			components::Transform{ math::Vec3f{ 1, 2, 3 }, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );

		selectionManager.select( entity );
		REQUIRE( selectionManager.isSelected( entity ) );
		REQUIRE( gizmoSystem.hasValidSelection() );

		// Switch from translate to rotate
		REQUIRE( gizmoSystem.getCurrentOperation() == editor::GizmoOperation::Translate );

		gizmoUI.setMockKeyPressed( "E" );
		gizmoUI.handleKeyboardShortcuts();

		REQUIRE( gizmoSystem.getCurrentOperation() == editor::GizmoOperation::Rotate );
		REQUIRE( selectionManager.isSelected( entity ) );
		REQUIRE( gizmoSystem.hasValidSelection() );
	}

	SECTION( "Multiple operation switches preserve selection" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );
		editor::GizmoUI gizmoUI( gizmoSystem );

		const auto entity = scene.createEntity();
		scene.addComponent<components::Transform>( entity,
			components::Transform{ math::Vec3f{ 5, 10, 15 }, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );

		selectionManager.select( entity );
		REQUIRE( selectionManager.isSelected( entity ) );

		// Switch through operations: Translate -> Rotate -> Scale -> Translate
		gizmoUI.setMockKeyPressed( "E" );
		gizmoUI.handleKeyboardShortcuts();
		REQUIRE( gizmoSystem.getCurrentOperation() == editor::GizmoOperation::Rotate );
		REQUIRE( selectionManager.isSelected( entity ) );

		gizmoUI.setMockKeyPressed( "R" );
		gizmoUI.handleKeyboardShortcuts();
		REQUIRE( gizmoSystem.getCurrentOperation() == editor::GizmoOperation::Scale );
		REQUIRE( selectionManager.isSelected( entity ) );

		gizmoUI.setMockKeyPressed( "W" );
		gizmoUI.handleKeyboardShortcuts();
		REQUIRE( gizmoSystem.getCurrentOperation() == editor::GizmoOperation::Translate );
		REQUIRE( selectionManager.isSelected( entity ) );
	}

	SECTION( "Coordinate space and visibility toggles preserve selection" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );
		editor::GizmoUI gizmoUI( gizmoSystem );

		const auto entity = scene.createEntity();
		scene.addComponent<components::Transform>( entity,
			components::Transform{ math::Vec3f{ 7, 8, 9 }, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );

		selectionManager.select( entity );
		REQUIRE( selectionManager.isSelected( entity ) );

		// Switch coordinate space (X key)
		gizmoUI.setMockKeyPressed( "X" );
		gizmoUI.handleKeyboardShortcuts();
		REQUIRE( selectionManager.isSelected( entity ) );

		// Toggle visibility (G key) - twice to test both states
		gizmoUI.setMockKeyPressed( "G" );
		gizmoUI.handleKeyboardShortcuts();
		REQUIRE( selectionManager.isSelected( entity ) );

		gizmoUI.setMockKeyPressed( "G" );
		gizmoUI.handleKeyboardShortcuts();
		REQUIRE( selectionManager.isSelected( entity ) );
	}
}