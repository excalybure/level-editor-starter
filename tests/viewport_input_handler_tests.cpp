#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "editor/viewport_input.h"
#include "editor/selection.h"
#include "engine/picking.h"
#include "editor/viewport/viewport.h"
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "runtime/entity.h"
#include "runtime/systems.h"
#include "engine/camera/camera.h"
#include "engine/math/vec.h"
#include "engine/math/matrix.h"
#include "engine/math/bounding_box_3d.h"

using Catch::Approx;

// Mock viewport helper for testing
class MockViewport : public editor::Viewport
{
public:
	MockViewport() : editor::Viewport( editor::ViewportType::Perspective )
	{
		// Setup basic viewport for testing
	}

	// Override picking ray to return predictable results for testing
	editor::ViewportRay getPickingRay( const math::Vec2<> &screenPos ) const noexcept override
	{
		editor::ViewportRay ray;
		ray.origin = math::Vec3<>{ screenPos.x / 100.0f, screenPos.y / 100.0f, -5.0f };
		ray.direction = math::Vec3<>{ 0.0f, 0.0f, 1.0f };
		ray.length = 1000.0f;
		return ray;
	}

	// Override world to screen for rectangle selection testing
	math::Vec2<> worldToScreen( const math::Vec3<> &worldPos ) const noexcept override
	{
		// Simple projection for testing
		return math::Vec2<>{ worldPos.x * 100.0f, worldPos.y * 100.0f };
	}
};

TEST_CASE( "ViewportInputHandler - Basic clicking", "[viewport][input][click]" )
{
	ecs::Scene scene;
	systems::SystemManager systemManager;
	systemManager.addSystem<systems::TransformSystem>();
	systemManager.initialize( scene );
	picking::PickingSystem pickingSystem( systemManager );
	editor::SelectionManager selectionManager( scene, systemManager );
	editor::ViewportInputHandler inputHandler( selectionManager, pickingSystem, systemManager );

	MockViewport viewport;
	const auto entity = scene.createEntity( "TestCube" );
	scene.addComponent( entity, components::Transform{} );

	// Mock mesh renderer with bounds for picking
	components::MeshRenderer meshRenderer;
	meshRenderer.bounds = math::BoundingBox3D<float>{
		math::Vec3<>{ -1.0f, -1.0f, -1.0f },
		math::Vec3<>{ 1.0f, 1.0f, 1.0f }
	};
	scene.addComponent( entity, meshRenderer );

	SECTION( "Left click selects object" )
	{
		const math::Vec2<> screenPos{ 0.0f, 0.0f }; // Center of entity in mock viewport

		inputHandler.handleMouseClick( scene, viewport, screenPos, true, false, // Left button only
			false,
			false ); // No modifiers

		REQUIRE( selectionManager.isSelected( entity ) );
		REQUIRE( selectionManager.getSelectionCount() == 1 );
	}

	SECTION( "Right click does not select" )
	{
		const math::Vec2<> screenPos{ 0.0f, 0.0f };

		inputHandler.handleMouseClick( scene, viewport, screenPos, false, true, // Right button only
			false,
			false ); // No modifiers

		REQUIRE_FALSE( selectionManager.isSelected( entity ) );
		REQUIRE( selectionManager.getSelectionCount() == 0 );
	}

	SECTION( "Click on empty space clears selection" )
	{
		// First select the entity
		selectionManager.select( entity );
		REQUIRE( selectionManager.isSelected( entity ) );

		// Click far away from entity
		const math::Vec2<> emptyPos{ 500.0f, 500.0f };

		inputHandler.handleMouseClick( scene, viewport, emptyPos, true, false, // Left button
			false,
			false ); // No modifiers

		REQUIRE_FALSE( selectionManager.isSelected( entity ) );
		REQUIRE( selectionManager.getSelectionCount() == 0 );
	}
}

TEST_CASE( "ViewportInputHandler - Modifier keys", "[viewport][input][modifiers]" )
{
	ecs::Scene scene;
	systems::SystemManager systemManager;
	systemManager.addSystem<systems::TransformSystem>();
	systemManager.initialize( scene );
	picking::PickingSystem pickingSystem( systemManager );
	editor::SelectionManager selectionManager( scene, systemManager );
	editor::ViewportInputHandler inputHandler( selectionManager, pickingSystem, systemManager );

	MockViewport viewport;
	const auto entity1 = scene.createEntity( "Cube1" );
	const auto entity2 = scene.createEntity( "Cube2" );

	// Setup entities with bounds at different positions
	scene.addComponent( entity1, components::Transform{} );
	components::MeshRenderer meshRenderer1;
	meshRenderer1.bounds = math::BoundingBox3D<float>{
		math::Vec3<>{ -1.0f, -1.0f, -1.0f },
		math::Vec3<>{ 1.0f, 1.0f, 1.0f }
	};
	scene.addComponent( entity1, meshRenderer1 );

	scene.addComponent( entity2, components::Transform{} );
	auto *transform2 = scene.getComponent<components::Transform>( entity2 );
	REQUIRE( transform2 != nullptr );
	transform2->position = math::Vec3<>{ 2.0f, 0.0f, 0.0f }; // Offset position
	components::MeshRenderer meshRenderer2;
	meshRenderer2.bounds = math::BoundingBox3D<float>{
		math::Vec3<>{ -1.0f, -1.0f, -1.0f },
		math::Vec3<>{ 1.0f, 1.0f, 1.0f }
	};
	scene.addComponent( entity2, meshRenderer2 );

	SECTION( "Ctrl+Click adds to selection" )
	{
		// First select entity1
		selectionManager.select( entity1 );

		// Ctrl+Click entity2 to add (entity2 is at screen pos 200, 0 in mock viewport)
		const math::Vec2<> screenPos{ 200.0f, 0.0f };
		inputHandler.handleMouseClick( scene, viewport, screenPos, true, false, // Left button
			true,
			false ); // Ctrl pressed

		REQUIRE( selectionManager.isSelected( entity1 ) );
		REQUIRE( selectionManager.isSelected( entity2 ) );
		REQUIRE( selectionManager.getSelectionCount() == 2 );
	}

	SECTION( "Ctrl+Shift+Click removes from selection" )
	{
		// First select both entities
		selectionManager.select( { entity1, entity2 } );
		REQUIRE( selectionManager.getSelectionCount() == 2 );

		// Ctrl+Shift+Click entity1 to remove
		const math::Vec2<> screenPos{ 0.0f, 0.0f };
		inputHandler.handleMouseClick( scene, viewport, screenPos, true, false, // Left button
			true,
			true ); // Ctrl+Shift pressed

		REQUIRE_FALSE( selectionManager.isSelected( entity1 ) );
		REQUIRE( selectionManager.isSelected( entity2 ) );
		REQUIRE( selectionManager.getSelectionCount() == 1 );
	}

	SECTION( "Shift+Click toggles selection" )
	{
		selectionManager.select( entity1 );

		// Shift+Click entity2 to add
		const math::Vec2<> screenPos{ 200.0f, 0.0f };
		inputHandler.handleMouseClick( scene, viewport, screenPos, true, false, // Left button
			false,
			true ); // Shift pressed

		REQUIRE( selectionManager.isSelected( entity1 ) );
		REQUIRE( selectionManager.isSelected( entity2 ) );
		REQUIRE( selectionManager.getSelectionCount() == 2 );

		// Shift+Click entity1 to remove
		const math::Vec2<> screenPos2{ 0.0f, 0.0f };
		inputHandler.handleMouseClick( scene, viewport, screenPos2, true, false, // Left button
			false,
			true ); // Shift pressed

		REQUIRE_FALSE( selectionManager.isSelected( entity1 ) );
		REQUIRE( selectionManager.isSelected( entity2 ) );
		REQUIRE( selectionManager.getSelectionCount() == 1 );
	}
}

TEST_CASE( "ViewportInputHandler - Rectangle selection", "[viewport][input][rect]" )
{
	ecs::Scene scene;
	systems::SystemManager systemManager;
	systemManager.addSystem<systems::TransformSystem>();
	systemManager.initialize( scene );
	picking::PickingSystem pickingSystem( systemManager );
	editor::SelectionManager selectionManager( scene, systemManager );
	editor::ViewportInputHandler inputHandler( selectionManager, pickingSystem, systemManager );

	MockViewport viewport;

	SECTION( "Small drag does not start rectangle selection" )
	{
		const math::Vec2<> startPos{ 100.0f, 100.0f };
		const math::Vec2<> endPos{ 102.0f, 103.0f }; // Less than 5 pixel threshold

		inputHandler.handleMouseDrag( scene, viewport, startPos, endPos, false, false );

		REQUIRE_FALSE( inputHandler.isRectSelectionActive() );
	}

	SECTION( "Drag creates rectangle selection" )
	{
		const math::Vec2<> startPos{ 100.0f, 100.0f };
		const math::Vec2<> endPos{ 200.0f, 200.0f }; // More than 5 pixel threshold

		// Start drag
		inputHandler.handleMouseDrag( scene, viewport, startPos, endPos, false, false );

		REQUIRE( inputHandler.isRectSelectionActive() );

		const auto &rectSel = inputHandler.getRectSelection();
		REQUIRE( rectSel.startPos.x == Catch::Approx( 100.0f ) );
		REQUIRE( rectSel.startPos.y == Catch::Approx( 100.0f ) );
		REQUIRE( rectSel.endPos.x == Catch::Approx( 200.0f ) );
		REQUIRE( rectSel.endPos.y == Catch::Approx( 200.0f ) );
		REQUIRE( rectSel.mode == editor::SelectionMode::Replace );

		// End drag
		inputHandler.handleMouseRelease( scene, viewport, endPos );

		REQUIRE_FALSE( inputHandler.isRectSelectionActive() );
	}

	SECTION( "Rectangle selection with modifiers" )
	{
		const math::Vec2<> startPos{ 50.0f, 50.0f };
		const math::Vec2<> endPos{ 150.0f, 150.0f };

		// Ctrl+Drag for additive selection
		inputHandler.handleMouseDrag( scene, viewport, startPos, endPos, true, false );

		REQUIRE( inputHandler.isRectSelectionActive() );
		const auto &rectSel = inputHandler.getRectSelection();
		REQUIRE( rectSel.mode == editor::SelectionMode::Add );

		inputHandler.handleMouseRelease( scene, viewport, endPos );
		REQUIRE_FALSE( inputHandler.isRectSelectionActive() );
	}
}

TEST_CASE( "ViewportInputHandler - Hover detection", "[viewport][input][hover]" )
{
	ecs::Scene scene;
	systems::SystemManager systemManager;
	systemManager.addSystem<systems::TransformSystem>();
	systemManager.initialize( scene );
	picking::PickingSystem pickingSystem( systemManager );
	editor::SelectionManager selectionManager( scene, systemManager );
	editor::ViewportInputHandler inputHandler( selectionManager, pickingSystem, systemManager );

	MockViewport viewport;
	const auto entity = scene.createEntity( "HoverCube" );
	scene.addComponent( entity, components::Transform{} );

	components::MeshRenderer meshRenderer;
	meshRenderer.bounds = math::BoundingBox3D<float>{
		math::Vec3<>{ -1.0f, -1.0f, -1.0f },
		math::Vec3<>{ 1.0f, 1.0f, 1.0f }
	};
	scene.addComponent( entity, meshRenderer );

	SECTION( "Mouse move over entity sets hover" )
	{
		REQUIRE( inputHandler.getHoveredEntity() == ecs::Entity{} );

		const math::Vec2<> screenPos{ 0.0f, 0.0f }; // Center of entity
		inputHandler.handleMouseMove( scene, viewport, screenPos );

		REQUIRE( inputHandler.getHoveredEntity() == entity );
	}

	SECTION( "Mouse move away from entity clears hover" )
	{
		// First hover over entity
		const math::Vec2<> onEntity{ 0.0f, 0.0f };
		inputHandler.handleMouseMove( scene, viewport, onEntity );
		REQUIRE( inputHandler.getHoveredEntity() == entity );

		// Move away from entity
		const math::Vec2<> awayFromEntity{ 500.0f, 500.0f };
		inputHandler.handleMouseMove( scene, viewport, awayFromEntity );

		REQUIRE( inputHandler.getHoveredEntity() == ecs::Entity{} );
	}
}

TEST_CASE( "ViewportInputHandler - Selection mode detection", "[viewport][input][mode]" )
{
	ecs::Scene scene;
	systems::SystemManager systemManager;
	systemManager.addSystem<systems::TransformSystem>();
	systemManager.initialize( scene );
	picking::PickingSystem pickingSystem( systemManager );
	editor::SelectionManager selectionManager( scene, systemManager );
	editor::ViewportInputHandler inputHandler( selectionManager, pickingSystem, systemManager );

	// We can test selection mode indirectly through the behavior
	MockViewport viewport;
	const auto entity = scene.createEntity( "TestEntity" );
	scene.addComponent( entity, components::Transform{} );

	components::MeshRenderer meshRenderer;
	meshRenderer.bounds = math::BoundingBox3D<float>{
		math::Vec3<>{ -1.0f, -1.0f, -1.0f },
		math::Vec3<>{ 1.0f, 1.0f, 1.0f }
	};
	scene.addComponent( entity, meshRenderer );

	SECTION( "No modifiers = Replace mode" )
	{
		// Pre-select entity
		selectionManager.select( entity );
		REQUIRE( selectionManager.isSelected( entity ) );

		// Click empty space with no modifiers should clear selection
		const math::Vec2<> emptyPos{ 500.0f, 500.0f };
		inputHandler.handleMouseClick( scene, viewport, emptyPos, true, false, // Left button
			false,
			false ); // No modifiers

		REQUIRE_FALSE( selectionManager.isSelected( entity ) );
	}

	SECTION( "Ctrl modifier = Add mode" )
	{
		// Click empty space with Ctrl should not clear selection
		const math::Vec2<> emptyPos{ 500.0f, 500.0f };
		inputHandler.handleMouseClick( scene, viewport, emptyPos, true, false, // Left button
			true,
			false ); // Ctrl pressed

		// components::Selection should remain unchanged
		REQUIRE( selectionManager.getSelectionCount() == 0 );
	}
}

TEST_CASE( "ViewportInputHandler - Edge cases", "[viewport][input][edge]" )
{
	ecs::Scene scene;
	systems::SystemManager systemManager;
	systemManager.addSystem<systems::TransformSystem>();
	systemManager.initialize( scene );
	picking::PickingSystem pickingSystem( systemManager );
	editor::SelectionManager selectionManager( scene, systemManager );
	editor::ViewportInputHandler inputHandler( selectionManager, pickingSystem, systemManager );

	MockViewport viewport;

	SECTION( "Handle input with empty scene" )
	{
		const math::Vec2<> screenPos{ 100.0f, 100.0f };

		// Should not crash with empty scene
		REQUIRE_NOTHROW( inputHandler.handleMouseClick( scene, viewport, screenPos, true, false, false, false ) );

		REQUIRE_NOTHROW( inputHandler.handleMouseMove( scene, viewport, screenPos ) );

		REQUIRE_NOTHROW( inputHandler.handleMouseDrag( scene, viewport, screenPos, math::Vec2<>{ 200.0f, 200.0f }, false, false ) );

		REQUIRE_NOTHROW( inputHandler.handleMouseRelease( scene, viewport, screenPos ) );
	}

	SECTION( "Handle invalid entities gracefully" )
	{
		// Create entity then remove it to create invalid reference
		const auto entity = scene.createEntity( "TempEntity" );
		scene.addComponent( entity, components::Transform{} );
		components::MeshRenderer meshRenderer;
		meshRenderer.bounds = math::BoundingBox3D<float>{
			math::Vec3<>{ -1.0f, -1.0f, -1.0f },
			math::Vec3<>{ 1.0f, 1.0f, 1.0f }
		};
		scene.addComponent( entity, meshRenderer );

		// Select entity
		selectionManager.select( entity );
		REQUIRE( selectionManager.isSelected( entity ) );

		// Destroy entity (making it invalid)
		scene.destroyEntity( entity );

		// Input handling should not crash with invalid entity in selection
		const math::Vec2<> screenPos{ 0.0f, 0.0f };
		REQUIRE_NOTHROW( inputHandler.handleMouseClick( scene, viewport, screenPos, true, false, false, false ) );

		REQUIRE_NOTHROW( inputHandler.handleMouseMove( scene, viewport, screenPos ) );
	}

	SECTION( "Very large screen coordinates" )
	{
		const math::Vec2<> largePos{ 1e6f, -1e6f };

		REQUIRE_NOTHROW( inputHandler.handleMouseClick( scene, viewport, largePos, true, false, false, false ) );

		REQUIRE_NOTHROW( inputHandler.handleMouseMove( scene, viewport, largePos ) );

		REQUIRE_NOTHROW( inputHandler.handleMouseDrag( scene, viewport, largePos, math::Vec2<>{ 2e6f, -2e6f }, false, false ) );
	}
}
