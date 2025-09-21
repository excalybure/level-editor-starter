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

TEST_CASE( "picking::PickingSystem - picking::Ray-AABB intersection", "[picking][aabb]" )
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

	SECTION( "picking::Ray hits center of AABB" )
	{
		math::Vec3<> rayOrigin{ 0.0f, 0.0f, -5.0f };
		math::Vec3<> rayDirection{ 0.0f, 0.0f, 1.0f };

		auto result = picker.raycast( scene, rayOrigin, rayDirection );

		REQUIRE( result.hit == true );
		REQUIRE( result.entity == entity );
		REQUIRE( result.distance == Catch::Approx( 4.0f ) ); // Hits front face at z=-1
	}

	SECTION( "picking::Ray misses AABB" )
	{
		math::Vec3<> rayOrigin{ 5.0f, 0.0f, -5.0f }; // Way off to the side
		math::Vec3<> rayDirection{ 0.0f, 0.0f, 1.0f };

		auto result = picker.raycast( scene, rayOrigin, rayDirection );

		REQUIRE( result.hit == false );
		REQUIRE( result.entity == ecs::Entity{} );
	}
}

TEST_CASE( "picking::PickingSystem - Multiple entities distance sorting", "[picking][multiple]" )
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