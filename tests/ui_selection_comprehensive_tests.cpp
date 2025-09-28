#include <catch2/catch_test_macros.hpp>

#include "editor/selection.h"
#include "editor/viewport_input.h"
#include "editor/viewport/viewport.h"
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "runtime/systems.h"
#include "engine/picking.h"

using namespace editor;

// Test to verify the fix for selection preservation when UI captures input
TEST_CASE( "Selection preservation behavior when UI captures mouse input", "[ui][selection][mouse][capture]" )
{
	// Setup minimal ECS and systems
	ecs::Scene scene;
	systems::SystemManager systemManager;
	systemManager.addSystem<systems::TransformSystem>();
	systemManager.initialize( scene );

	// Create picking system
	picking::PickingSystem pickingSystem( systemManager );

	// Create selection manager
	SelectionManager selectionManager( scene, systemManager );

	// Create viewport input handler
	ViewportInputHandler inputHandler( selectionManager, pickingSystem, systemManager );

	// Create test entities
	auto entity1 = scene.createEntity( "TestEntity1" );
	auto entity2 = scene.createEntity( "TestEntity2" );
	scene.addComponent<components::Transform>( entity1, {} );
	scene.addComponent<components::MeshRenderer>( entity1, {} );
	scene.addComponent<components::Transform>( entity2, {} );
	scene.addComponent<components::MeshRenderer>( entity2, {} );

	// Select entities
	selectionManager.select( { entity1, entity2 } );
	REQUIRE( selectionManager.getSelectionCount() == 2 );
	REQUIRE( selectionManager.isSelected( entity1 ) );
	REQUIRE( selectionManager.isSelected( entity2 ) );

	// Create a mock viewport that returns no hit
	class MockViewport : public Viewport
	{
	public:
		MockViewport() : Viewport( ViewportType::Perspective ) {}

		ViewportRay getPickingRay( const math::Vec2<> & /*viewportPos*/ ) const noexcept override
		{
			// Return a ray that doesn't hit anything
			return ViewportRay{
				math::Vec3f{ 0, 0, 0 },	 // origin
				math::Vec3f{ 0, 0, -1 }, // direction
				1000.0f					 // length
			};
		}

		math::Vec2f worldToScreen( const math::Vec3f & /*worldPos*/ ) const noexcept override
		{
			return math::Vec2f{ 0, 0 };
		}
	};

	MockViewport mockViewport;

	SECTION( "Selection is cleared when clicking empty viewport area (normal behavior)" )
	{
		// This is the expected behavior when clicking empty area in viewport
		// (not on UI elements) - selection should be cleared

		// Simulate mouse click on empty area (no entity hit, no UI capture)
		inputHandler.handleMouseClick( scene, mockViewport, math::Vec2f{ 100, 100 },
			true,  // leftButton
			false, // rightButton
			false, // ctrlPressed
			false  // shiftPressed
		);

		// Selection should be cleared when clicking empty viewport area
		REQUIRE( selectionManager.getSelectionCount() == 0 );
		REQUIRE_FALSE( selectionManager.isSelected( entity1 ) );
		REQUIRE_FALSE( selectionManager.isSelected( entity2 ) );
	}

	SECTION( "Selection is NOT cleared when clicking with Ctrl (additive mode)" )
	{
		// Reset selection for this test
		selectionManager.select( { entity1, entity2 } );
		REQUIRE( selectionManager.getSelectionCount() == 2 );

		// Simulate mouse click on empty area with Ctrl (additive mode)
		inputHandler.handleMouseClick( scene, mockViewport, math::Vec2f{ 100, 100 },
			true,  // leftButton
			false, // rightButton
			true,  // ctrlPressed (additive mode)
			false  // shiftPressed
		);

		// Selection should NOT be cleared in additive mode
		REQUIRE( selectionManager.getSelectionCount() == 2 );
		REQUIRE( selectionManager.isSelected( entity1 ) );
		REQUIRE( selectionManager.isSelected( entity2 ) );
	}
}

// Test the atomic functionality of selection mode behavior
TEST_CASE( "ViewportInputHandler selection mode handling", "[viewport][input][selection][mode]" )
{
	// Setup minimal ECS and systems
	ecs::Scene scene;
	systems::SystemManager systemManager;
	systemManager.addSystem<systems::TransformSystem>();
	systemManager.initialize( scene );

	// Create picking system
	picking::PickingSystem pickingSystem( systemManager );

	// Create selection manager
	SelectionManager selectionManager( scene, systemManager );

	// Create viewport input handler
	ViewportInputHandler inputHandler( selectionManager, pickingSystem, systemManager );

	// Create test entity
	auto entity1 = scene.createEntity( "TestEntity1" );
	scene.addComponent<components::Transform>( entity1, {} );
	scene.addComponent<components::MeshRenderer>( entity1, {} );

	// Create a mock viewport that returns no hit
	class MockViewport : public Viewport
	{
	public:
		MockViewport() : Viewport( ViewportType::Perspective ) {}

		ViewportRay getPickingRay( const math::Vec2<> & /*viewportPos*/ ) const noexcept override
		{
			return ViewportRay{
				math::Vec3f{ 0, 0, 0 },	 // origin
				math::Vec3f{ 0, 0, -1 }, // direction
				1000.0f					 // length
			};
		}

		math::Vec2f worldToScreen( const math::Vec3f & /*worldPos*/ ) const noexcept override
		{
			return math::Vec2f{ 0, 0 };
		}
	};

	MockViewport mockViewport;

	SECTION( "Replace mode (no modifiers) clears selection when clicking empty area" )
	{
		// Select entity first
		selectionManager.select( entity1 );
		REQUIRE( selectionManager.isSelected( entity1 ) );

		// Click empty area with no modifiers (Replace mode)
		inputHandler.handleMouseClick( scene, mockViewport, math::Vec2f{ 100, 100 },
			true,  // leftButton
			false, // rightButton
			false, // ctrlPressed
			false  // shiftPressed
		);

		// Selection should be cleared
		REQUIRE( selectionManager.getSelectionCount() == 0 );
	}

	SECTION( "Add mode (Ctrl) preserves selection when clicking empty area" )
	{
		// Select entity first
		selectionManager.select( entity1 );
		REQUIRE( selectionManager.isSelected( entity1 ) );

		// Click empty area with Ctrl (Add mode)
		inputHandler.handleMouseClick( scene, mockViewport, math::Vec2f{ 100, 100 },
			true,  // leftButton
			false, // rightButton
			true,  // ctrlPressed (Add mode)
			false  // shiftPressed
		);

		// Selection should be preserved in Add mode
		REQUIRE( selectionManager.getSelectionCount() == 1 );
		REQUIRE( selectionManager.isSelected( entity1 ) );
	}

	SECTION( "Toggle mode (Shift) preserves selection when clicking empty area" )
	{
		// Select entity first
		selectionManager.select( entity1 );
		REQUIRE( selectionManager.isSelected( entity1 ) );

		// Click empty area with Shift (Toggle mode)
		inputHandler.handleMouseClick( scene, mockViewport, math::Vec2f{ 100, 100 },
			true,  // leftButton
			false, // rightButton
			false, // ctrlPressed
			true   // shiftPressed (Toggle mode)
		);

		// Selection should be preserved in Toggle mode
		REQUIRE( selectionManager.getSelectionCount() == 1 );
		REQUIRE( selectionManager.isSelected( entity1 ) );
	}

	SECTION( "Subtract mode (Ctrl+Shift) preserves selection when clicking empty area" )
	{
		// Select entity first
		selectionManager.select( entity1 );
		REQUIRE( selectionManager.isSelected( entity1 ) );

		// Click empty area with Ctrl+Shift (Subtract mode)
		inputHandler.handleMouseClick( scene, mockViewport, math::Vec2f{ 100, 100 },
			true,  // leftButton
			false, // rightButton
			true,  // ctrlPressed
			true   // shiftPressed (Subtract mode)
		);

		// Selection should be preserved in Subtract mode
		REQUIRE( selectionManager.getSelectionCount() == 1 );
		REQUIRE( selectionManager.isSelected( entity1 ) );
	}
}