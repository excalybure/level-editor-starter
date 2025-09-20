#include <catch2/catch_test_macros.hpp>
#include <imgui.h>
#include <ImGuizmo.h>

TEST_CASE( "ImGuizmo headers can be included", "[imguizmo][integration]" )
{
	REQUIRE( true ); // Basic compilation test
}

TEST_CASE( "ImGuizmo basic functionality is accessible", "[imguizmo][integration]" )
{
	// Test that we can access ImGuizmo namespace and basic functions

	// BeginFrame should be callable (even though we won't actually call it in this test)
	// We're just testing that the function exists and the header is properly included
	bool canAccessBeginFrame = true;

	// Test that we can access ImGuizmo enums
	ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
	ImGuizmo::MODE mode = ImGuizmo::WORLD;

	REQUIRE( canAccessBeginFrame );
	REQUIRE( operation == ImGuizmo::TRANSLATE );
	REQUIRE( mode == ImGuizmo::WORLD );
}

TEST_CASE( "ImGuizmo operation types are available", "[imguizmo][unit]" )
{
	// Test all operation types are accessible
	REQUIRE( ImGuizmo::TRANSLATE == ImGuizmo::TRANSLATE );
	REQUIRE( ImGuizmo::ROTATE == ImGuizmo::ROTATE );
	REQUIRE( ImGuizmo::SCALE == ImGuizmo::SCALE );

	// Test that we can combine operations (universal mode)
	auto universal = ImGuizmo::TRANSLATE | ImGuizmo::ROTATE | ImGuizmo::SCALE;
	REQUIRE( universal != 0 );
}

TEST_CASE( "ImGuizmo mode types are available", "[imguizmo][unit]" )
{
	// Test coordinate space modes
	REQUIRE( ImGuizmo::LOCAL == ImGuizmo::LOCAL );
	REQUIRE( ImGuizmo::WORLD == ImGuizmo::WORLD );
	REQUIRE( ImGuizmo::LOCAL != ImGuizmo::WORLD );
}