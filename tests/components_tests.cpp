#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

import runtime.ecs;
import runtime.components;
import engine.math;
import engine.matrix;
import engine.vec;

using namespace ecs;
using namespace components;

TEST_CASE( "Transform component basic functionality", "[components][transform]" )
{
	Transform transform;

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

	// Test that matrices are marked as dirty initially
	REQUIRE( transform.localMatrixDirty );
	REQUIRE( transform.worldMatrixDirty );
}

TEST_CASE( "Transform component local matrix calculation", "[components][transform][matrix]" )
{
	Transform transform;
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
	Transform transform;
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
	Transform transform;
	transform.scale = math::Vec3<float>( 2.0f, 3.0f, 4.0f );

	const auto localMatrix = transform.getLocalMatrix();

	// Check scale components (diagonal elements)
	REQUIRE( localMatrix.m00() == Catch::Approx( 2.0f ) );
	REQUIRE( localMatrix.m11() == Catch::Approx( 3.0f ) );
	REQUIRE( localMatrix.m22() == Catch::Approx( 4.0f ) );
}

TEST_CASE( "Transform component markDirty", "[components][transform][dirty]" )
{
	Transform transform;

	// Get matrix to clear dirty flags
	transform.getLocalMatrix();
	REQUIRE_FALSE( transform.localMatrixDirty );
	REQUIRE( transform.worldMatrixDirty ); // This should still be true initially

	// Mark as dirty
	transform.markDirty();
	REQUIRE( transform.localMatrixDirty );
	REQUIRE( transform.worldMatrixDirty );
}

TEST_CASE( "Name component functionality", "[components][name]" )
{
	SECTION( "Default constructor" )
	{
		Name name;
		REQUIRE( name.name == "Unnamed" );
	}

	SECTION( "String constructor" )
	{
		Name name( "TestEntity" );
		REQUIRE( name.name == "TestEntity" );
	}

	SECTION( "Assignment" )
	{
		Name name;
		name.name = "AssignedName";
		REQUIRE( name.name == "AssignedName" );
	}
}

TEST_CASE( "Visible component functionality", "[components][visible]" )
{
	Visible visible;

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
		MeshRenderer renderer;
		REQUIRE( renderer.gpuMesh == nullptr );
		REQUIRE( renderer.lodBias == 0.0f );
	}

	SECTION( "Default GPU mesh state" )
	{
		// Test that default constructor creates MeshRenderer with no GPU mesh
		MeshRenderer renderer;
		REQUIRE( renderer.gpuMesh == nullptr );
		REQUIRE( renderer.lodBias == 0.0f );
	}

	SECTION( "LOD bias assignment" )
	{
		MeshRenderer renderer;
		renderer.lodBias = 2.5f;

		REQUIRE( renderer.lodBias == Catch::Approx( 2.5f ) );
	}

	SECTION( "Bounds assignment" )
	{
		MeshRenderer renderer;
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
		// Verify that the new MeshRenderer structure is more memory-efficient
		// Old structure had: std::string + std::vector<std::string> + bool + bounds
		// New structure has: shared_ptr + float + bounds

		// The new structure should be significantly smaller due to:
		// - shared_ptr (8 bytes) vs string + vector of strings (potentially 100+ bytes)
		// - float (4 bytes) vs bool (1 byte, but with padding considerations)
		const std::size_t rendererSize = sizeof( MeshRenderer );

		// Reasonable upper bound: shared_ptr(8) + float(4) + bounds(24) + padding ≈ 40 bytes
		// Old structure with strings could easily be 100+ bytes
		REQUIRE( rendererSize <= 64 ); // Conservative upper limit

		// Verify that size is at least the minimum expected components
		const std::size_t minimumSize = sizeof( std::shared_ptr<void> ) + sizeof( float ) + sizeof( math::BoundingBox3Df );
		REQUIRE( rendererSize >= minimumSize );
	}
}

TEST_CASE( "Selected component functionality", "[components][selected]" )
{
	Selected selected;

	// Test default values
	REQUIRE_FALSE( selected.selected );
	REQUIRE( selected.highlightColor.x == Catch::Approx( 1.0f ) );
	REQUIRE( selected.highlightColor.y == Catch::Approx( 0.8f ) );
	REQUIRE( selected.highlightColor.z == Catch::Approx( 0.2f ) );

	// Test modifications
	selected.selected = true;
	selected.highlightColor = math::Vec3<float>( 0.5f, 1.0f, 0.5f );

	REQUIRE( selected.selected );
	REQUIRE( selected.highlightColor.x == Catch::Approx( 0.5f ) );
	REQUIRE( selected.highlightColor.y == Catch::Approx( 1.0f ) );
	REQUIRE( selected.highlightColor.z == Catch::Approx( 0.5f ) );
}

TEST_CASE( "Component concept validation", "[components][concepts]" )
{
	// These should all compile and pass with our relaxed Component concept
	REQUIRE( components::Component<Transform> );
	REQUIRE( components::Component<Name> );
	REQUIRE( components::Component<Visible> );
	REQUIRE( components::Component<MeshRenderer> );
	REQUIRE( components::Component<Selected> );
}

TEST_CASE( "Transform component with Scene integration", "[components][transform][integration]" )
{
	Scene scene;
	Entity entity = scene.createEntity( "TransformTest" );

	// Add transform component
	Transform transform;
	transform.position = math::Vec3<float>( 1.0f, 2.0f, 3.0f );
	transform.rotation = math::Vec3<float>( 0.0f, math::pi<float> / 4.0f, 0.0f ); // 45 degrees around Y
	transform.scale = math::Vec3<float>( 1.5f, 1.5f, 1.5f );

	REQUIRE( scene.addComponent( entity, transform ) );
	REQUIRE( scene.hasComponent<Transform>( entity ) );

	// Get the component back and verify
	const Transform *storedTransform = scene.getComponent<Transform>( entity );
	REQUIRE( storedTransform != nullptr );
	REQUIRE( storedTransform->position.x == Catch::Approx( 1.0f ) );
	REQUIRE( storedTransform->position.y == Catch::Approx( 2.0f ) );
	REQUIRE( storedTransform->position.z == Catch::Approx( 3.0f ) );
	REQUIRE( storedTransform->scale.x == Catch::Approx( 1.5f ) );
}

TEST_CASE( "Multiple components on single entity", "[components][integration]" )
{
	Scene scene;
	Entity entity = scene.createEntity( "MultiComponentTest" );

	// Add multiple components
	Transform transform;
	transform.position = math::Vec3<float>( 5.0f, 0.0f, 0.0f );

	Name name( "TestEntity" );
	Visible visible;
	visible.castShadows = false;

	MeshRenderer renderer;
	Selected selected;
	selected.selected = true;

	REQUIRE( scene.addComponent( entity, transform ) );
	REQUIRE( scene.addComponent( entity, name ) );
	REQUIRE( scene.addComponent( entity, visible ) );
	REQUIRE( scene.addComponent( entity, renderer ) );
	REQUIRE( scene.addComponent( entity, selected ) );

	// Verify all components are present
	REQUIRE( scene.hasComponent<Transform>( entity ) );
	REQUIRE( scene.hasComponent<Name>( entity ) );
	REQUIRE( scene.hasComponent<Visible>( entity ) );
	REQUIRE( scene.hasComponent<MeshRenderer>( entity ) );
	REQUIRE( scene.hasComponent<Selected>( entity ) );

	// Verify component data
	const auto *storedName = scene.getComponent<Name>( entity );
	const auto *storedVisible = scene.getComponent<Visible>( entity );
	const auto *storedRenderer = scene.getComponent<MeshRenderer>( entity );
	const auto *storedSelected = scene.getComponent<Selected>( entity );

	REQUIRE( storedName->name == "TestEntity" );
	REQUIRE_FALSE( storedVisible->castShadows );
	REQUIRE( storedSelected->selected );
}
