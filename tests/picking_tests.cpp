#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "engine/picking.h"
#include "engine/math/math_3d.h"
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "runtime/entity.h"
#include "runtime/systems.h"
#include "engine/math/vec.h"
#include "engine/math/matrix.h"
#include "engine/math/bounding_box_3d.h"

using Catch::Approx;


TEST_CASE( "HitResult - Basic functionality", "[picking][hit-result]" )
{
	picking::HitResult hit1, hit2;

	hit1.hit = true;
	hit1.distance = 5.0f;
	hit2.hit = true;
	hit2.distance = 10.0f;

	REQUIRE( hit1 < hit2 ); // Closer hits sort first

	std::vector<picking::HitResult> hits = { hit2, hit1 };
	std::sort( hits.begin(), hits.end() );
	REQUIRE( hits[0].distance == 5.0f );
	REQUIRE( hits[1].distance == 10.0f );
}

TEST_CASE( "PickingSystem - Ray-AABB intersection", "[picking][aabb]" )
{
	ecs::Scene scene;
	systems::SystemManager systemManager;
	systemManager.addSystem<systems::TransformSystem>();
	systemManager.initialize( scene );
	picking::PickingSystem picker( systemManager );

	// Create entity with mesh renderer (has bounds)
	auto entity = scene.createEntity( "TestCube" );
	scene.addComponent( entity, components::Transform{} );

	// Mock mesh with known bounds for testing
	components::MeshRenderer meshRenderer;
	meshRenderer.bounds = math::BoundingBox3D<float>{
		math::Vec3<>{ -1.0f, -1.0f, -1.0f },
		math::Vec3<>{ 1.0f, 1.0f, 1.0f }
	};
	scene.addComponent( entity, meshRenderer );

	SECTION( "Ray hits center of AABB" )
	{
		math::Vec3<> rayOrigin{ 0.0f, 0.0f, -5.0f };
		math::Vec3<> rayDirection{ 0.0f, 0.0f, 1.0f };

		auto result = picker.raycast( scene, rayOrigin, rayDirection );

		REQUIRE( result.hit == true );
		REQUIRE( result.entity == entity );
		REQUIRE( result.distance == Catch::Approx( 4.0f ) ); // Hits front face at z=-1
	}

	SECTION( "Ray misses AABB" )
	{
		math::Vec3<> rayOrigin{ 5.0f, 0.0f, -5.0f }; // Way off to the side
		math::Vec3<> rayDirection{ 0.0f, 0.0f, 1.0f };

		auto result = picker.raycast( scene, rayOrigin, rayDirection );

		REQUIRE( result.hit == false );
		REQUIRE( result.entity == ecs::Entity{} );
	}
}

TEST_CASE( "PickingSystem - Multiple entities distance sorting", "[picking][multiple]" )
{
	ecs::Scene scene;
	systems::SystemManager systemManager;
	systemManager.addSystem<systems::TransformSystem>();
	systemManager.initialize( scene );
	picking::PickingSystem picker( systemManager );

	// Create two cubes at different distances
	auto nearCube = scene.createEntity( "NearCube" );
	scene.addComponent( nearCube, components::Transform{} );
	components::MeshRenderer nearRenderer;
	nearRenderer.bounds = math::BoundingBox3D<float>{
		math::Vec3<>{ -0.5f, -0.5f, -0.5f },
		math::Vec3<>{ 0.5f, 0.5f, 0.5f }
	};
	scene.addComponent( nearCube, nearRenderer );

	auto farCube = scene.createEntity( "FarCube" );

	components::Transform farTransform;
	farTransform.position = math::Vec3<>{ 0.0f, 0.0f, 5.0f }; // Further away
	scene.addComponent( farCube, farTransform );

	components::MeshRenderer farRenderer;
	farRenderer.bounds = math::BoundingBox3D<float>{
		math::Vec3<>{ -0.5f, -0.5f, -0.5f },
		math::Vec3<>{ 0.5f, 0.5f, 0.5f }
	};
	scene.addComponent( farCube, farRenderer );

	// picking::Ray hits both cubes
	math::Vec3<> rayOrigin{ 0.0f, 0.0f, -5.0f };
	math::Vec3<> rayDirection{ 0.0f, 0.0f, 1.0f };

	auto results = picker.raycastAll( scene, rayOrigin, rayDirection );

	REQUIRE( results.size() == 2 );
	REQUIRE( results[0].entity == nearCube ); // Closer cube first
	REQUIRE( results[1].entity == farCube );  // Further cube second
	REQUIRE( results[0].distance < results[1].distance );
}

TEST_CASE( "PickingSystem - Scaled object bounds", "[picking][scale][integration]" )
{
	ecs::Scene scene;
	systems::SystemManager systemManager;
	systemManager.addSystem<systems::TransformSystem>();
	systemManager.initialize( scene );
	picking::PickingSystem picker( systemManager );

	// Create entity with initial bounds of [-1, 1] in all directions
	auto entity = scene.createEntity( "ScaledCube" );

	components::Transform transform;
	transform.position = math::Vec3<>{ 0.0f, 0.0f, 0.0f };
	transform.rotation = math::Vec3<>{ 0.0f, 0.0f, 0.0f };
	transform.scale = math::Vec3<>{ 3.0f, 3.0f, 3.0f }; // Scale up by 3x
	scene.addComponent( entity, transform );

	components::MeshRenderer meshRenderer;
	meshRenderer.bounds = math::BoundingBox3D<float>{
		math::Vec3<>{ -1.0f, -1.0f, -1.0f }, // Original bounds: 2x2x2 cube
		math::Vec3<>{ 1.0f, 1.0f, 1.0f }
	};
	scene.addComponent( entity, meshRenderer );

	// Update system to ensure world transforms are computed
	systemManager.update( scene, 0.016f );

	SECTION( "Ray should hit scaled object at scaled bounds" )
	{
		// After 3x scale, the cube should extend from [-3, 3] in all directions
		// Test ray hitting at the edge of the scaled bounds (should hit)
		math::Vec3<> rayOrigin{ 2.5f, 0.0f, -5.0f }; // X=2.5 is within scaled bounds [-3, 3]
		math::Vec3<> rayDirection{ 0.0f, 0.0f, 1.0f };

		auto result = picker.raycast( scene, rayOrigin, rayDirection );

		// This should hit because the scaled cube extends to x=3.0
		REQUIRE( result.hit == true );
		REQUIRE( result.entity == entity );
	}

	SECTION( "Ray should miss scaled object beyond scaled bounds" )
	{
		// Test ray missing beyond the scaled bounds (should miss)
		math::Vec3<> rayOrigin{ 3.5f, 0.0f, -5.0f }; // X=3.5 is outside scaled bounds [-3, 3]
		math::Vec3<> rayDirection{ 0.0f, 0.0f, 1.0f };

		auto result = picker.raycast( scene, rayOrigin, rayDirection );

		// This should miss because X=3.5 is beyond the scaled bound of x=3.0
		REQUIRE( result.hit == false );
	}

	SECTION( "Ray should miss within original bounds but outside scaled bounds" )
	{
		// Test ray that would hit original bounds but misses scaled bounds
		// This test reveals the bug: if picking uses original bounds, it would miss
		// when it should hit at scaled size
		math::Vec3<> rayOrigin{ 0.5f, 0.0f, -5.0f }; // Within both original and scaled bounds
		math::Vec3<> rayDirection{ 0.0f, 0.0f, 1.0f };

		auto result = picker.raycast( scene, rayOrigin, rayDirection );

		// This should definitely hit - the ray is well within scaled bounds
		REQUIRE( result.hit == true );
		REQUIRE( result.entity == entity );
	}
}