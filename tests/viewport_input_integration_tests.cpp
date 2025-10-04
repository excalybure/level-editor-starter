#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "editor/viewport_input.h"
#include "editor/selection.h"
#include "engine/picking.h"
#include "editor/viewport/viewport.h"
#include "runtime/ecs.h"
#include "runtime/entity.h"
#include "runtime/components.h"
#include "runtime/systems.h"
#include "engine/camera/camera.h"
#include "math/vec.h"
#include "math/matrix.h"

namespace
{

// Minimal mock for integration testing
class MockViewport : public editor::Viewport
{
public:
	MockViewport()
		: editor::Viewport( editor::ViewportType::Perspective )
	{
	}

	editor::ViewportRay getPickingRay( const math::Vec2<> &screenPos ) const noexcept override
	{
		// Simple mock implementation - return a ray pointing into the scene
		editor::ViewportRay ray;
		ray.origin = math::Vec3<>{ screenPos.x, screenPos.y, -10.0f };
		ray.direction = math::Vec3<>{ 0.0f, 0.0f, 1.0f };
		ray.length = 20.0f;
		return ray;
	}

	math::Vec2<> worldToScreen( const math::Vec3<> &worldPos ) const noexcept override
	{
		// Simple orthographic projection for testing
		return math::Vec2<>{ worldPos.x, worldPos.y };
	}
};

} // anonymous namespace

TEST_CASE( "ViewportInputHandler - Basic functionality", "[viewport_input][integration]" )
{
	// Setup test environment
	auto scene = ecs::Scene{};
	auto systemManager = systems::SystemManager{};
	systemManager.addSystem<systems::TransformSystem>();
	systemManager.initialize( scene );
	auto pickingSystem = picking::PickingSystem{ systemManager };
	auto selectionManager = editor::SelectionManager{ scene, systemManager };

	// Create handler
	auto handler = editor::ViewportInputHandler{ selectionManager, pickingSystem, systemManager };

	SECTION( "Handler initializes correctly" )
	{
		REQUIRE_FALSE( handler.isRectSelectionActive() );
		REQUIRE( handler.getHoveredEntity() == ecs::Entity{} );
	}

	SECTION( "Selection mode logic" )
	{
		// This tests the internal getSelectionMode logic via the public interface
		MockViewport viewport;

		// Test Replace mode (default)
		handler.handleMouseClick( scene, viewport, math::Vec2<>{ 100.0f, 100.0f }, true, false, false, false );

		// Should not crash - basic functionality test
		REQUIRE( true ); // If we get here, basic handling works

		// Test rectangle selection
		handler.handleMouseDrag( scene, viewport, math::Vec2<>{ 50.0f, 50.0f }, math::Vec2<>{ 150.0f, 150.0f }, false, false );

		REQUIRE( handler.isRectSelectionActive() );

		// Test release
		handler.handleMouseRelease( scene, viewport, math::Vec2<>{ 150.0f, 150.0f } );

		REQUIRE_FALSE( handler.isRectSelectionActive() );
	}

	SECTION( "Mouse move handling" )
	{
		MockViewport viewport;

		handler.handleMouseMove( scene, viewport, math::Vec2<>{ 75.0f, 75.0f } );

		// Should not crash - basic functionality test
		REQUIRE( true );
	}
}

TEST_CASE( "ViewportInputHandler - Rectangle selection bounds", "[viewport_input][integration]" )
{
	auto scene = ecs::Scene{};
	auto systemManager = systems::SystemManager{};
	systemManager.addSystem<systems::TransformSystem>();
	systemManager.initialize( scene );
	auto pickingSystem = picking::PickingSystem{ systemManager };
	auto selectionManager = editor::SelectionManager{ scene, systemManager };
	auto handler = editor::ViewportInputHandler{ selectionManager, pickingSystem, systemManager };
	MockViewport viewport;

	SECTION( "Rectangle selection activation" )
	{
		const auto startPos = math::Vec2<>{ 10.0f, 10.0f };
		const auto dragPos = math::Vec2<>{ 50.0f, 50.0f }; // Sufficient drag distance

		handler.handleMouseDrag( scene, viewport, startPos, dragPos, false, false );

		REQUIRE( handler.isRectSelectionActive() );

		const auto &rectSelection = handler.getRectSelection();
		REQUIRE( rectSelection.active );
		REQUIRE_THAT( rectSelection.startPos.x, Catch::Matchers::WithinRel( 10.0f ) );
		REQUIRE_THAT( rectSelection.startPos.y, Catch::Matchers::WithinRel( 10.0f ) );
		REQUIRE_THAT( rectSelection.endPos.x, Catch::Matchers::WithinRel( 50.0f ) );
		REQUIRE_THAT( rectSelection.endPos.y, Catch::Matchers::WithinRel( 50.0f ) );
	}

	SECTION( "Small drags don't activate rectangle selection" )
	{
		const auto startPos = math::Vec2<>{ 10.0f, 10.0f };
		const auto dragPos = math::Vec2<>{ 12.0f, 12.0f }; // Small drag distance

		handler.handleMouseDrag( scene, viewport, startPos, dragPos, false, false );

		REQUIRE_FALSE( handler.isRectSelectionActive() );
	}
}

TEST_CASE( "ViewportInputHandler - Integration with SelectionManager", "[viewport_input][integration]" )
{
	auto scene = ecs::Scene{};
	auto systemManager = systems::SystemManager{};
	systemManager.addSystem<systems::TransformSystem>();
	systemManager.initialize( scene );
	auto pickingSystem = picking::PickingSystem{ systemManager };
	auto selectionManager = editor::SelectionManager{ scene, systemManager };
	auto handler = editor::ViewportInputHandler{ selectionManager, pickingSystem, systemManager };

	// Create test entity
	const auto entity = scene.createEntity();
	scene.addComponent<components::Transform>( entity, components::Transform{} );
	scene.addComponent<components::MeshRenderer>( entity, components::MeshRenderer{} );

	SECTION( "Click with no hit clears selection" )
	{
		// First select something
		selectionManager.select( entity, false );
		REQUIRE( selectionManager.isSelected( entity ) );

		// Click where no entity exists
		MockViewport viewport;
		handler.handleMouseClick( scene, viewport, math::Vec2<>{ 1000.0f, 1000.0f }, true, false, false, false );

		// Selection should be cleared (Replace mode with no hit)
		REQUIRE_FALSE( selectionManager.isSelected( entity ) );
		REQUIRE( selectionManager.getSelectedEntities().empty() );
	}
}
