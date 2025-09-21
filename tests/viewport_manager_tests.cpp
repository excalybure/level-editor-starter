// ViewportManager specific tests
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "test_dx12_helpers.h"

#include "editor/viewport/viewport.h"
#include "platform/dx12/dx12_device.h"

using namespace editor;
using Catch::Matchers::WithinAbs;

TEST_CASE( "ViewportManager Creation and Management", "[viewport][manager]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "ViewportManager Creation and Management" ) )
		return;

	ViewportManager manager;
	REQUIRE( manager.initialize( &device ) );

	SECTION( "Multiple viewport creation" )
	{
		auto *perspective = manager.createViewport( ViewportType::Perspective );
		auto *top = manager.createViewport( ViewportType::Top );
		auto *front = manager.createViewport( ViewportType::Front );
		auto *side = manager.createViewport( ViewportType::Side );

		REQUIRE( perspective != nullptr );
		REQUIRE( top != nullptr );
		REQUIRE( front != nullptr );
		REQUIRE( side != nullptr );

		// All should be different instances
		REQUIRE( perspective != top );
		REQUIRE( perspective != front );
		REQUIRE( perspective != side );
		REQUIRE( top != front );

		// Check viewport collection size
		const auto &viewports = manager.getViewports();
		REQUIRE( viewports.size() == 4 );
	}

	SECTION( "Active viewport management" )
	{
		auto *viewport1 = manager.createViewport( ViewportType::Perspective );
		auto *viewport2 = manager.createViewport( ViewportType::Top );

		// Initially no active viewport
		REQUIRE( manager.getActiveViewport() == nullptr );

		// Set active viewport
		manager.setActiveViewport( viewport1 );
		REQUIRE( manager.getActiveViewport() == viewport1 );

		// Change active viewport
		manager.setActiveViewport( viewport2 );
		REQUIRE( manager.getActiveViewport() == viewport2 );

		// Clear active viewport
		manager.setActiveViewport( nullptr );
		REQUIRE( manager.getActiveViewport() == nullptr );
	}

	SECTION( "Focused viewport management" )
	{
		auto *viewport1 = manager.createViewport( ViewportType::Perspective );
		auto *viewport2 = manager.createViewport( ViewportType::Front );

		// Initially no focused viewport
		REQUIRE( manager.getFocusedViewport() == nullptr );

		// Set focused viewport
		manager.setFocusedViewport( viewport1 );
		REQUIRE( manager.getFocusedViewport() == viewport1 );

		// Change focused viewport
		manager.setFocusedViewport( viewport2 );
		REQUIRE( manager.getFocusedViewport() == viewport2 );

		// Clear focused viewport
		manager.setFocusedViewport( nullptr );
		REQUIRE( manager.getFocusedViewport() == nullptr );
	}

	SECTION( "Viewport destruction" )
	{
		auto *viewport = manager.createViewport( ViewportType::Perspective );
		REQUIRE( manager.getViewports().size() == 1 );

		manager.destroyViewport( viewport );
		REQUIRE( manager.getViewports().size() == 0 );
	}

	SECTION( "Destroy all viewports" )
	{
		manager.createViewport( ViewportType::Perspective );
		manager.createViewport( ViewportType::Top );
		manager.createViewport( ViewportType::Front );

		REQUIRE( manager.getViewports().size() == 3 );

		manager.destroyAllViewports();
		REQUIRE( manager.getViewports().size() == 0 );
		REQUIRE( manager.getActiveViewport() == nullptr );
		REQUIRE( manager.getFocusedViewport() == nullptr );
	}
}

TEST_CASE( "ViewportManager Update and Render", "[viewport][manager]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "ViewportManager Update and Render" ) )
		return;

	ViewportManager manager;
	REQUIRE( manager.initialize( &device ) );

	SECTION( "Update with delta time" )
	{
		auto *viewport = manager.createViewport( ViewportType::Perspective );
		manager.setActiveViewport( viewport );

		// Should not crash with various delta times
		REQUIRE_NOTHROW( manager.update( 0.0f ) );
		REQUIRE_NOTHROW( manager.update( 0.016f ) ); // ~60fps
		REQUIRE_NOTHROW( manager.update( 0.033f ) ); // ~30fps
		REQUIRE_NOTHROW( manager.update( 1.0f ) );	 // Large delta
	}

	SECTION( "Render without crash" )
	{
		auto *viewport = manager.createViewport( ViewportType::Perspective );
		manager.setActiveViewport( viewport );

		// Should not crash during render
		REQUIRE_NOTHROW( manager.render() );
	}

	manager.shutdown();
}
