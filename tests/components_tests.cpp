#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "runtime/ecs.h"
#include "runtime/components.h"
#include "math/math.h"
#include "math/matrix.h"
#include "math/vec.h"

using Catch::Approx;

TEST_CASE( "Transform component basic functionality", "[components][transform]" )
{
	components::Transform transform;

	// Test default values
	REQUIRE( transform.position.x == Catch::Approx( 0.0f ) );
	REQUIRE( transform.position.y == Catch::Approx( 0.0f ) );
	REQUIRE( transform.position.z == Catch::Approx( 0.0f ) );
	REQUIRE( transform.scale.x == Catch::Approx( 1.0f ) );
	REQUIRE( transform.scale.y == Catch::Approx( 1.0f ) );
	REQUIRE( transform.scale.z == Catch::Approx( 1.0f ) );
	REQUIRE( transform.rotation.x == Catch::Approx( 0.0f ) );
	REQUIRE( transform.rotation.y == Catch::Approx( 0.0f ) );
	REQUIRE( transform.rotation.z == Catch::Approx( 0.0f ) );

	// Test that matrix is marked as dirty initially
	REQUIRE( transform.localMatrixDirty );
}

TEST_CASE( "Transform component local matrix calculation", "[components][transform][matrix]" )
{
	components::Transform transform;
	transform.position = math::Vec3<float>( 5.0f, 10.0f, 15.0f );
	transform.scale = math::Vec3<float>( 2.0f, 3.0f, 4.0f );

	const auto localMatrix = transform.getLocalMatrix();

	// Check translation components (last column)
	REQUIRE( localMatrix.m03() == Catch::Approx( 5.0f ) );
	REQUIRE( localMatrix.m13() == Catch::Approx( 10.0f ) );
	REQUIRE( localMatrix.m23() == Catch::Approx( 15.0f ) );

	// Check that matrix is no longer dirty after calculation
	REQUIRE_FALSE( transform.localMatrixDirty );
}

TEST_CASE( "Transform component rotation matrix", "[components][transform][rotation]" )
{
	components::Transform transform;
	transform.rotation = math::Vec3<float>( math::pi<float> / 2.0f, 0.0f, 0.0f ); // 90 degrees around X-axis

	const auto localMatrix = transform.getLocalMatrix();

	// After 90-degree rotation around X, Y becomes -Z and Z becomes Y
	// Check the rotation part of the matrix (approximately)
	REQUIRE( localMatrix.m11() == Catch::Approx( 0.0f ).margin( 0.0001f ) );
	REQUIRE( localMatrix.m12() == Catch::Approx( -1.0f ).margin( 0.0001f ) );
	REQUIRE( localMatrix.m21() == Catch::Approx( 1.0f ).margin( 0.0001f ) );
	REQUIRE( localMatrix.m22() == Catch::Approx( 0.0f ).margin( 0.0001f ) );
}

TEST_CASE( "Transform component scale matrix", "[components][transform][scale]" )
{
	components::Transform transform;
	transform.scale = math::Vec3<float>( 2.0f, 3.0f, 4.0f );

	const auto localMatrix = transform.getLocalMatrix();

	// Check scale components (diagonal elements)
	REQUIRE( localMatrix.m00() == Catch::Approx( 2.0f ) );
	REQUIRE( localMatrix.m11() == Catch::Approx( 3.0f ) );
	REQUIRE( localMatrix.m22() == Catch::Approx( 4.0f ) );
}

TEST_CASE( "Transform component markDirty", "[components][transform][dirty]" )
{
	components::Transform transform;

	// Get matrix to clear dirty flag
	transform.getLocalMatrix();
	REQUIRE_FALSE( transform.localMatrixDirty );

	// Mark as dirty
	transform.markDirty();
	REQUIRE( transform.localMatrixDirty );
}

TEST_CASE( "Name component functionality", "[components][name]" )
{
	SECTION( "Default constructor" )
	{
		components::Name name;
		REQUIRE( name.name == "Unnamed" );
	}

	SECTION( "String constructor" )
	{
		components::Name name( "TestEntity" );
		REQUIRE( name.name == "TestEntity" );
	}

	SECTION( "Assignment" )
	{
		components::Name name;
		name.name = "AssignedName";
		REQUIRE( name.name == "AssignedName" );
	}
}

TEST_CASE( "Visible component functionality", "[components][visible]" )
{
	components::Visible visible;

	// Test default values
	REQUIRE( visible.visible );
	REQUIRE( visible.castShadows );
	REQUIRE( visible.receiveShadows );

	// Test modifications
	visible.visible = false;
	visible.castShadows = false;
	visible.receiveShadows = false;

	REQUIRE_FALSE( visible.visible );
	REQUIRE_FALSE( visible.castShadows );
	REQUIRE_FALSE( visible.receiveShadows );
}

TEST_CASE( "MeshRenderer component functionality", "[components][meshrenderer]" )
{
	SECTION( "Default constructor" )
	{
		components::MeshRenderer renderer;
		REQUIRE( renderer.gpuMesh == nullptr );
		REQUIRE( renderer.lodBias == 0.0f );
	}

	SECTION( "Default GPU mesh state" )
	{
		// Test that default constructor creates components::MeshRenderer with no GPU mesh
		components::MeshRenderer renderer;
		REQUIRE( renderer.gpuMesh == nullptr );
		REQUIRE( renderer.lodBias == 0.0f );
	}

	SECTION( "LOD bias assignment" )
	{
		components::MeshRenderer renderer;
		renderer.lodBias = 2.5f;

		REQUIRE( renderer.lodBias == Catch::Approx( 2.5f ) );
	}

	SECTION( "Bounds assignment" )
	{
		components::MeshRenderer renderer;
		const math::Vec3f minPoint{ -1.0f, -2.0f, -3.0f };
		const math::Vec3f maxPoint{ 1.0f, 2.0f, 3.0f };
		renderer.bounds = math::BoundingBox3Df{ minPoint, maxPoint };

		REQUIRE( renderer.bounds.min.x == Catch::Approx( -1.0f ) );
		REQUIRE( renderer.bounds.min.y == Catch::Approx( -2.0f ) );
		REQUIRE( renderer.bounds.min.z == Catch::Approx( -3.0f ) );
		REQUIRE( renderer.bounds.max.x == Catch::Approx( 1.0f ) );
		REQUIRE( renderer.bounds.max.y == Catch::Approx( 2.0f ) );
		REQUIRE( renderer.bounds.max.z == Catch::Approx( 3.0f ) );
	}

	SECTION( "Component size optimization verification" )
	{
		// Verify that the new components::MeshRenderer structure is more memory-efficient
		// Old structure had: std::string + std::vector<std::string> + bool + bounds
		// New structure has: shared_ptr + float + bounds

		// The new structure should be significantly smaller due to:
		// - shared_ptr (8 bytes) vs string + vector of strings (potentially 100+ bytes)
		// - float (4 bytes) vs bool (1 byte, but with padding considerations)
		const std::size_t rendererSize = sizeof( components::MeshRenderer );

		// Reasonable upper bound: shared_ptr(8) + float(4) + bounds(24) + padding ≈ 40 bytes
		// Old structure with strings could easily be 100+ bytes
		REQUIRE( rendererSize <= 64 ); // Conservative upper limit

		// Verify that size is at least the minimum expected components
		const std::size_t minimumSize = sizeof( std::shared_ptr<void> ) + sizeof( float ) + sizeof( math::BoundingBox3Df );
		REQUIRE( rendererSize >= minimumSize );
	}
}

TEST_CASE( "Selected Component - Basic functionality", "[components][selection]" )
{
	SECTION( "Default construction sets timestamp" )
	{
		components::Selected selected;
		REQUIRE( selected.isPrimary == false );
		REQUIRE( selected.selectionTime > 0.0f );
		REQUIRE( selected.highlightColor.x == 1.0f ); // Orange highlight
		REQUIRE( selected.highlightColor.y == Catch::Approx( 0.6f ) );
		REQUIRE( selected.highlightColor.z == Catch::Approx( 0.0f ) );
		REQUIRE( selected.highlightColor.w == 1.0f );
	}

	SECTION( "Primary selection constructor" )
	{
		components::Selected primary( true );
		REQUIRE( primary.isPrimary == true );
		REQUIRE( primary.selectionTime > 0.0f );
	}
}

TEST_CASE( "Component concept validation", "[components][concepts]" )
{
	// These should all compile and pass with our relaxed Component concept
	REQUIRE( components::Component<components::Transform> );
	REQUIRE( components::Component<components::Name> );
	REQUIRE( components::Component<components::Visible> );
	REQUIRE( components::Component<components::MeshRenderer> );
	REQUIRE( components::Component<components::Selected> );
}

TEST_CASE( "Transform component with Scene integration", "[components][transform][integration]" )
{
	ecs::Scene scene;
	ecs::Entity entity = scene.createEntity( "TransformTest" );

	// Add transform component
	components::Transform transform;
	transform.position = math::Vec3<float>( 1.0f, 2.0f, 3.0f );
	transform.rotation = math::Vec3<float>( 0.0f, math::pi<float> / 4.0f, 0.0f ); // 45 degrees around Y
	transform.scale = math::Vec3<float>( 1.5f, 1.5f, 1.5f );

	REQUIRE( scene.addComponent( entity, transform ) );
	REQUIRE( scene.hasComponent<components::Transform>( entity ) );

	// Get the component back and verify
	const components::Transform *storedTransform = scene.getComponent<components::Transform>( entity );
	REQUIRE( storedTransform != nullptr );
	REQUIRE( storedTransform->position.x == Catch::Approx( 1.0f ) );
	REQUIRE( storedTransform->position.y == Catch::Approx( 2.0f ) );
	REQUIRE( storedTransform->position.z == Catch::Approx( 3.0f ) );
	REQUIRE( storedTransform->scale.x == Catch::Approx( 1.5f ) );
}

TEST_CASE( "Multiple components on single entity", "[components][integration]" )
{
	ecs::Scene scene;
	ecs::Entity entity = scene.createEntity( "MultiComponentTest" );

	// Add multiple components
	components::Transform transform;
	transform.position = math::Vec3<float>( 5.0f, 0.0f, 0.0f );

	components::Name name( "TestEntity" );
	components::Visible visible;
	visible.castShadows = false;

	components::MeshRenderer renderer;
	components::Selected selected{ true }; // Primary selection

	REQUIRE( scene.addComponent( entity, transform ) );
	REQUIRE( scene.addComponent( entity, name ) );
	REQUIRE( scene.addComponent( entity, visible ) );
	REQUIRE( scene.addComponent( entity, renderer ) );
	REQUIRE( scene.addComponent( entity, selected ) );

	// Verify all components are present
	REQUIRE( scene.hasComponent<components::Transform>( entity ) );
	REQUIRE( scene.hasComponent<components::Name>( entity ) );
	REQUIRE( scene.hasComponent<components::Visible>( entity ) );
	REQUIRE( scene.hasComponent<components::MeshRenderer>( entity ) );
	REQUIRE( scene.hasComponent<components::Selected>( entity ) );

	// Verify component data
	const auto *storedName = scene.getComponent<components::Name>( entity );
	const auto *storedVisible = scene.getComponent<components::Visible>( entity );
	const auto *storedSelected = scene.getComponent<components::Selected>( entity );

	REQUIRE( storedName->name == "TestEntity" );
	REQUIRE_FALSE( storedVisible->castShadows );
	REQUIRE( storedSelected->isPrimary == true );
}

TEST_CASE( "Selected Component - ECS integration", "[components][selection][ecs]" )
{
	ecs::Scene scene;

	SECTION( "Add and remove Selected component" )
	{
		auto entity = scene.createEntity( "TestObject" );

		// Add Selected component
		scene.addComponent( entity, components::Selected{} );
		REQUIRE( scene.hasComponent<components::Selected>( entity ) );

		auto *selected = scene.getComponent<components::Selected>( entity );
		REQUIRE( selected != nullptr );
		REQUIRE( selected->isPrimary == false );

		// Remove Selected component
		scene.removeComponent<components::Selected>( entity );
		REQUIRE_FALSE( scene.hasComponent<components::Selected>( entity ) );
	}

	SECTION( "Primary selection tracking" )
	{
		auto entity1 = scene.createEntity( "Object1" );
		auto entity2 = scene.createEntity( "Object2" );

		scene.addComponent( entity1, components::Selected{ true } );  // Primary
		scene.addComponent( entity2, components::Selected{ false } ); // Secondary

		// Count selected entities
		int selectedCount = 0;
		int primaryCount = 0;
		scene.forEach<components::Selected>( [&]( ecs::Entity, const components::Selected &sel ) {
			selectedCount++;
			if ( sel.isPrimary )
				primaryCount++;
		} );

		REQUIRE( selectedCount == 2 );
		REQUIRE( primaryCount == 1 );
	}
}

TEST_CASE( "Transform matrix layout consistency with HLSL", "[components][transform][matrix][hlsl]" )
{
	// This test verifies that our matrix layout works correctly with HLSL shader expectations
	// HLSL mul(matrix, vector) expects column-major matrices, but our C++ matrices are row-major

	components::Transform transform;
	transform.position = { 10.0f, 20.0f, 30.0f };
	transform.scale = { 1.0f, 1.0f, 1.0f };
	transform.rotation = { 0.0f, 0.0f, 0.0f }; // No rotation for simplicity

	const auto matrix = transform.getLocalMatrix();

	// Verify our matrix has translation in the expected position (last column in row-major)
	REQUIRE( matrix.m03() == Catch::Approx( 10.0f ) ); // Translation X
	REQUIRE( matrix.m13() == Catch::Approx( 20.0f ) ); // Translation Y
	REQUIRE( matrix.m23() == Catch::Approx( 30.0f ) ); // Translation Z

	// Test point transformation using our matrix directly (row-major style)
	math::Vec3f point{ 1.0f, 2.0f, 3.0f };
	math::Vec3f transformedPoint = matrix.transformPoint( point );

	// Expected result: point + translation = (1+10, 2+20, 3+30) = (11, 22, 33)
	REQUIRE( transformedPoint.x == Catch::Approx( 11.0f ) );
	REQUIRE( transformedPoint.y == Catch::Approx( 22.0f ) );
	REQUIRE( transformedPoint.z == Catch::Approx( 33.0f ) );

	// However, if we transpose the matrix (converting to column-major),
	// the translation would be in the bottom row instead of the right column
	const auto transposedMatrix = matrix.transpose();

	// After transpose, translation moves to the bottom row
	REQUIRE( transposedMatrix.m30() == Catch::Approx( 10.0f ) ); // Translation X (now in bottom row)
	REQUIRE( transposedMatrix.m31() == Catch::Approx( 20.0f ) ); // Translation Y (now in bottom row)
	REQUIRE( transposedMatrix.m32() == Catch::Approx( 30.0f ) ); // Translation Z (now in bottom row)
}
