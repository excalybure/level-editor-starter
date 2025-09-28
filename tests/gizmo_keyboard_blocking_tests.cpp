#include <catch2/catch_test_macros.hpp>

#include "editor/gizmos.h"
#include "editor/selection.h"
#include "runtime/ecs.h"
#include "runtime/systems.h"
#include "runtime/components.h"

TEST_CASE( "Gizmo keyboard blocking conditional behavior", "[gizmos][input][keyboard-blocking]" )
{
	SECTION( "Keys should NOT be blocked when gizmos are invisible" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );

		// Create and select an entity
		const auto entity = scene.createEntity();
		scene.addComponent<components::Transform>( entity,
			components::Transform{ math::Vec3f{ 1, 2, 3 }, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );

		selectionManager.select( entity );
		REQUIRE( selectionManager.isSelected( entity ) );
		REQUIRE( gizmoSystem.hasValidSelection() );

		// Make gizmos invisible
		gizmoSystem.setVisible( false );
		REQUIRE_FALSE( gizmoSystem.isVisible() );

		// Even with selection, keyboard should NOT be blocked when gizmos are invisible
		// This simulates the condition in ui.cpp:
		const bool gizmosAreActive = gizmoSystem.isVisible() && gizmoSystem.hasValidSelection();
		REQUIRE_FALSE( gizmosAreActive ); // Should be false because invisible
	}

	SECTION( "Keys should NOT be blocked when no selection exists" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );

		// No selection
		REQUIRE_FALSE( gizmoSystem.hasValidSelection() );
		REQUIRE( gizmoSystem.isVisible() ); // Visible by default

		// Without selection, keyboard should NOT be blocked even if gizmos are visible
		const bool gizmosAreActive = gizmoSystem.isVisible() && gizmoSystem.hasValidSelection();
		REQUIRE_FALSE( gizmosAreActive ); // Should be false because no selection
	}

	SECTION( "Keys should be blocked ONLY when gizmos are visible AND have selection" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );

		// Create and select an entity
		const auto entity = scene.createEntity();
		scene.addComponent<components::Transform>( entity,
			components::Transform{ math::Vec3f{ 1, 2, 3 }, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );

		selectionManager.select( entity );
		REQUIRE( selectionManager.isSelected( entity ) );
		REQUIRE( gizmoSystem.hasValidSelection() );
		REQUIRE( gizmoSystem.isVisible() );

		// Both conditions met: keyboard SHOULD be blocked
		const bool gizmosAreActive = gizmoSystem.isVisible() && gizmoSystem.hasValidSelection();
		REQUIRE( gizmosAreActive ); // Should be true - both conditions met
	}

	SECTION( "Keys should NOT be blocked when invisible even with selection" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );

		// Create and select an entity
		const auto entity = scene.createEntity();
		scene.addComponent<components::Transform>( entity,
			components::Transform{ math::Vec3f{ 1, 2, 3 }, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );

		selectionManager.select( entity );
		REQUIRE( selectionManager.isSelected( entity ) );
		REQUIRE( gizmoSystem.hasValidSelection() );

		// Hide gizmos
		gizmoSystem.setVisible( false );
		REQUIRE_FALSE( gizmoSystem.isVisible() );

		// Even with selection, should NOT block when invisible
		const bool gizmosAreActive = gizmoSystem.isVisible() && gizmoSystem.hasValidSelection();
		REQUIRE_FALSE( gizmosAreActive ); // Should be false because invisible
	}
}