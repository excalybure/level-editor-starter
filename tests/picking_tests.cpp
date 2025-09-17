#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

import engine.picking;
import engine.math_3d;
import runtime.ecs;
import runtime.components;
import runtime.entity;
import engine.vec;
import engine.matrix;
import engine.bounding_box_3d;

using namespace math;
using namespace picking;
using namespace components;
using namespace ecs;

TEST_CASE( "HitResult - Basic functionality", "[picking][hit-result]" )
{
	HitResult hit1, hit2;

	hit1.hit = true;
	hit1.distance = 5.0f;
	hit2.hit = true;
	hit2.distance = 10.0f;

	REQUIRE( hit1 < hit2 ); // Closer hits sort first

	std::vector<HitResult> hits = { hit2, hit1 };
	std::sort( hits.begin(), hits.end() );
	REQUIRE( hits[0].distance == 5.0f );
	REQUIRE( hits[1].distance == 10.0f );
}

TEST_CASE( "PickingSystem - Ray-AABB intersection", "[picking][aabb]" )
{
	PickingSystem picker;
	Scene scene;

	// Create entity with mesh renderer (has bounds)
	auto entity = scene.createEntity( "TestCube" );
	scene.addComponent( entity, Transform{} );

	// Mock mesh with known bounds for testing
	MeshRenderer meshRenderer;
	meshRenderer.bounds = BoundingBox3D<float>{
		Vec3<>{ -1.0f, -1.0f, -1.0f },
		Vec3<>{ 1.0f, 1.0f, 1.0f }
	};
	scene.addComponent( entity, meshRenderer );

	SECTION( "Ray hits center of AABB" )
	{
		Vec3<> rayOrigin{ 0.0f, 0.0f, -5.0f };
		Vec3<> rayDirection{ 0.0f, 0.0f, 1.0f };

		auto result = picker.raycast( scene, rayOrigin, rayDirection );

		REQUIRE( result.hit == true );
		REQUIRE( result.entity == entity );
		REQUIRE( result.distance == Catch::Approx( 4.0f ) ); // Hits front face at z=-1
	}

	SECTION( "Ray misses AABB" )
	{
		Vec3<> rayOrigin{ 5.0f, 0.0f, -5.0f }; // Way off to the side
		Vec3<> rayDirection{ 0.0f, 0.0f, 1.0f };

		auto result = picker.raycast( scene, rayOrigin, rayDirection );

		REQUIRE( result.hit == false );
		REQUIRE( result.entity == Entity{} );
	}
}

TEST_CASE( "PickingSystem - Multiple entities distance sorting", "[picking][multiple]" )
{
	PickingSystem picker;
	Scene scene;

	// Create two cubes at different distances
	auto nearCube = scene.createEntity( "NearCube" );
	scene.addComponent( nearCube, Transform{} );
	MeshRenderer nearRenderer;
	nearRenderer.bounds = BoundingBox3D<float>{
		Vec3<>{ -0.5f, -0.5f, -0.5f },
		Vec3<>{ 0.5f, 0.5f, 0.5f }
	};
	scene.addComponent( nearCube, nearRenderer );

	auto farCube = scene.createEntity( "FarCube" );
	scene.addComponent( farCube, Transform{} );
	auto *farTransform = scene.getComponent<Transform>( farCube );
	REQUIRE( farTransform != nullptr );
	farTransform->position = Vec3<>{ 0.0f, 0.0f, 5.0f }; // Further away
	MeshRenderer farRenderer;
	farRenderer.bounds = BoundingBox3D<float>{
		Vec3<>{ -0.5f, -0.5f, -0.5f },
		Vec3<>{ 0.5f, 0.5f, 0.5f }
	};
	scene.addComponent( farCube, farRenderer );

	// Ray hits both cubes
	Vec3<> rayOrigin{ 0.0f, 0.0f, -5.0f };
	Vec3<> rayDirection{ 0.0f, 0.0f, 1.0f };

	auto results = picker.raycastAll( scene, rayOrigin, rayDirection );

	REQUIRE( results.size() == 2 );
	REQUIRE( results[0].entity == nearCube ); // Closer cube first
	REQUIRE( results[1].entity == farCube );  // Further cube second
	REQUIRE( results[0].distance < results[1].distance );
}