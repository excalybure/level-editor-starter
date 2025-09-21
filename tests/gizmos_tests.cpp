#include <catch2/catch_test_macros.hpp>

#include "editor/gizmos.h"
#include "engine/math/vec.h"
#include "engine/math/matrix.h"

using namespace editor;
using namespace math;

TEST_CASE( "GizmoOperation enum values", "[gizmos][unit][operation]" )
{
	SECTION( "GizmoOperation has correct enum values" )
	{
		REQUIRE( static_cast<int>( GizmoOperation::Translate ) == 0 );
		REQUIRE( static_cast<int>( GizmoOperation::Rotate ) == 1 );
		REQUIRE( static_cast<int>( GizmoOperation::Scale ) == 2 );
		REQUIRE( static_cast<int>( GizmoOperation::Universal ) == 3 );
	}

	SECTION( "GizmoOperation enum can be compared" )
	{
		const auto op1 = GizmoOperation::Translate;
		const auto op2 = GizmoOperation::Translate;
		const auto op3 = GizmoOperation::Rotate;

		REQUIRE( op1 == op2 );
		REQUIRE( op1 != op3 );
	}
}

TEST_CASE( "GizmoMode enum values", "[gizmos][unit][mode]" )
{
	SECTION( "GizmoMode has correct enum values" )
	{
		REQUIRE( static_cast<int>( GizmoMode::Local ) == 0 );
		REQUIRE( static_cast<int>( GizmoMode::World ) == 1 );
	}

	SECTION( "GizmoMode enum can be compared" )
	{
		const auto mode1 = GizmoMode::Local;
		const auto mode2 = GizmoMode::Local;
		const auto mode3 = GizmoMode::World;

		REQUIRE( mode1 == mode2 );
		REQUIRE( mode1 != mode3 );
	}
}

TEST_CASE( "GizmoResult struct default values and manipulation flags", "[gizmos][unit][result]" )
{
	SECTION( "GizmoResult has correct default values" )
	{
		const GizmoResult result;

		REQUIRE_FALSE( result.wasManipulated );
		REQUIRE_FALSE( result.isManipulating );

		// Check individual components instead of using Vec3 equality
		REQUIRE( result.translationDelta.x == 0.0f );
		REQUIRE( result.translationDelta.y == 0.0f );
		REQUIRE( result.translationDelta.z == 0.0f );

		REQUIRE( result.rotationDelta.x == 0.0f );
		REQUIRE( result.rotationDelta.y == 0.0f );
		REQUIRE( result.rotationDelta.z == 0.0f );

		REQUIRE( result.scaleDelta.x == 1.0f );
		REQUIRE( result.scaleDelta.y == 1.0f );
		REQUIRE( result.scaleDelta.z == 1.0f );
	}

	SECTION( "GizmoResult manipulation flags can be set" )
	{
		GizmoResult result;
		result.wasManipulated = true;
		result.isManipulating = true;

		REQUIRE( result.wasManipulated );
		REQUIRE( result.isManipulating );
	}

	SECTION( "GizmoResult delta values can be set" )
	{
		GizmoResult result;
		result.translationDelta = Vec3<>{ 1.0f, 2.0f, 3.0f };
		result.rotationDelta = Vec3<>{ 0.1f, 0.2f, 0.3f };
		result.scaleDelta = Vec3<>{ 1.5f, 2.0f, 0.5f };

		REQUIRE( result.translationDelta.x == 1.0f );
		REQUIRE( result.translationDelta.y == 2.0f );
		REQUIRE( result.translationDelta.z == 3.0f );

		REQUIRE( result.rotationDelta.x == 0.1f );
		REQUIRE( result.rotationDelta.y == 0.2f );
		REQUIRE( result.rotationDelta.z == 0.3f );

		REQUIRE( result.scaleDelta.x == 1.5f );
		REQUIRE( result.scaleDelta.y == 2.0f );
		REQUIRE( result.scaleDelta.z == 0.5f );
	}
}

TEST_CASE( "GizmoSystem class interface", "[gizmos][unit][system]" )
{
	SECTION( "GizmoSystem can be instantiated" )
	{
		const GizmoSystem system;

		// Should have default values for operation and mode
		REQUIRE( system.getCurrentOperation() == GizmoOperation::Translate );
		REQUIRE( system.getCurrentMode() == GizmoMode::World );
	}

	SECTION( "GizmoSystem operation and mode can be set" )
	{
		GizmoSystem system;

		system.setOperation( GizmoOperation::Rotate );
		system.setMode( GizmoMode::Local );

		REQUIRE( system.getCurrentOperation() == GizmoOperation::Rotate );
		REQUIRE( system.getCurrentMode() == GizmoMode::Local );
	}
}

TEST_CASE( "GizmoSystem state management", "[gizmos][unit][state]" )
{
	SECTION( "GizmoSystem starts with no active manipulation" )
	{
		const GizmoSystem system;

		REQUIRE_FALSE( system.isManipulating() );
		REQUIRE_FALSE( system.wasManipulated() );
	}

	SECTION( "GizmoSystem can track active manipulation state" )
	{
		GizmoSystem system;

		// Start manipulation
		system.beginManipulation();
		REQUIRE( system.isManipulating() );
		REQUIRE_FALSE( system.wasManipulated() );

		// End manipulation
		system.endManipulation();
		REQUIRE_FALSE( system.isManipulating() );
		REQUIRE( system.wasManipulated() );

		// Reset state
		system.resetManipulationState();
		REQUIRE_FALSE( system.isManipulating() );
		REQUIRE_FALSE( system.wasManipulated() );
	}
}