#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "editor/gizmos.h"
#include "editor/selection.h"
#include "engine/picking.h"
#include "editor/viewport/viewport.h"
#include "runtime/ecs.h"
#include "runtime/systems.h"
#include "runtime/components.h"
#include "math/vec.h"
#include "math/matrix.h"
#include "math/bounding_box_3d.h"

using Catch::Approx;

TEST_CASE( "Gizmo Selection Integration - Transform Update Propagation", "[gizmos][selection][picking][integration]" )
{
	SECTION( "Selection works correctly after gizmo transform application" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;

		// Add transform system for proper world matrix handling
		auto *transformSystem = systemManager.addSystem<systems::TransformSystem>();
		systemManager.initialize( scene );

		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );
		picking::PickingSystem pickingSystem( systemManager );

		// Create entity at origin for simple testing
		const auto entity = scene.createEntity();
		const math::Vec3f initialPos{ 0.0f, 0.0f, 0.0f };
		scene.addComponent<components::Transform>( entity,
			components::Transform{ initialPos, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );

		// Add mesh renderer with bounds for picking
		components::MeshRenderer meshRenderer;
		meshRenderer.bounds = math::BoundingBox3D<float>{
			math::Vec3<>{ -1.0f, -1.0f, -1.0f },
			math::Vec3<>{ 1.0f, 1.0f, 1.0f }
		};
		scene.addComponent( entity, meshRenderer );

		// Initialize transform system state
		systemManager.update( scene, 0.016f );

		// Select the entity
		selectionManager.select( entity );
		REQUIRE( selectionManager.isSelected( entity ) );

		// Verify initial world transform is correct
		const auto initialWorldMatrix = transformSystem->getWorldTransform( scene, entity );
		REQUIRE( initialWorldMatrix.m03() == Approx( initialPos.x ) );
		REQUIRE( initialWorldMatrix.m13() == Approx( initialPos.y ) );
		REQUIRE( initialWorldMatrix.m23() == Approx( initialPos.z ) );

		// Test picking at initial position works - ray from -Z pointing towards origin
		const math::Vec3f rayOrigin{ 0.0f, 0.0f, -10.0f };
		const math::Vec3f rayDirection{ 0.0f, 0.0f, 1.0f }; // Pointing directly forward towards origin
		const float rayLength = 20.0f;

		const auto initialHit = pickingSystem.raycast( scene, rayOrigin, rayDirection, rayLength );
		REQUIRE( initialHit.hit );
		REQUIRE( initialHit.entity == entity );

		// Move the entity with gizmo system
		const math::Vec3f moveDelta{ 5.0f, 0.0f, 0.0f };
		const math::Vec3f expectedNewPos = initialPos + moveDelta;

		editor::GizmoResult delta;
		delta.translationDelta = moveDelta;
		delta.rotationDelta = math::Vec3f{ 0, 0, 0 };
		delta.scaleDelta = math::Vec3f{ 1, 1, 1 };

		// Apply the transform delta
		gizmoSystem.applyTransformDelta( delta );

		// Update systems to process any changes
		systemManager.update( scene, 0.016f );

		// Verify the transform component was updated
		const auto *transform = scene.getComponent<components::Transform>( entity );
		REQUIRE( transform->position.x == Approx( expectedNewPos.x ) );
		REQUIRE( transform->position.y == Approx( expectedNewPos.y ) );
		REQUIRE( transform->position.z == Approx( expectedNewPos.z ) );

		// CRITICAL TEST: Verify world matrix is updated correctly
		const auto newWorldMatrix = transformSystem->getWorldTransform( scene, entity );
		REQUIRE( newWorldMatrix.m03() == Approx( expectedNewPos.x ) );
		REQUIRE( newWorldMatrix.m13() == Approx( expectedNewPos.y ) );
		REQUIRE( newWorldMatrix.m23() == Approx( expectedNewPos.z ) );

		// Deselect the entity
		selectionManager.deselectAll();
		REQUIRE_FALSE( selectionManager.isSelected( entity ) );

		// Test picking at the OLD position should fail
		const auto oldPosHit = pickingSystem.raycast( scene, rayOrigin, rayDirection, rayLength );
		REQUIRE_FALSE( oldPosHit.hit ); // Should not hit at old position

		// Test picking at the NEW position should succeed
		const math::Vec3f newRayOrigin{ 5.0f, 0.0f, -10.0f }; // Moved to align with new entity position
		const auto newPosHit = pickingSystem.raycast( scene, newRayOrigin, rayDirection, rayLength );
		REQUIRE( newPosHit.hit );
		REQUIRE( newPosHit.entity == entity );

		// Test re-selection at new position
		// This simulates clicking at the new position to select the entity
		selectionManager.select( entity ); // In real scenario, this would be triggered by viewport input handler
		REQUIRE( selectionManager.isSelected( entity ) );
	}

	SECTION( "Demonstrates the fix with proper SystemManager integration" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;

		// Add transform system for proper world matrix handling
		auto *transformSystem = systemManager.addSystem<systems::TransformSystem>();
		systemManager.initialize( scene );

		editor::SelectionManager selectionManager( scene, systemManager );
		// Since the old constructor was removed, use the proper constructor
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );
		picking::PickingSystem pickingSystem( systemManager );

		// Create entity at origin for simple testing
		const auto entity = scene.createEntity();
		const math::Vec3f initialPos{ 0.0f, 0.0f, 0.0f };
		scene.addComponent<components::Transform>( entity,
			components::Transform{ initialPos, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );

		// Add mesh renderer with bounds for picking
		components::MeshRenderer meshRenderer;
		meshRenderer.bounds = math::BoundingBox3D<float>{
			math::Vec3<>{ -1.0f, -1.0f, -1.0f },
			math::Vec3<>{ 1.0f, 1.0f, 1.0f }
		};
		scene.addComponent( entity, meshRenderer );

		// Initialize transform system state
		systemManager.update( scene, 0.016f );

		// Select the entity
		selectionManager.select( entity );
		REQUIRE( selectionManager.isSelected( entity ) );

		// Move the entity with gizmo system (proper constructor)
		const math::Vec3f moveDelta{ 5.0f, 0.0f, 0.0f };
		const math::Vec3f expectedNewPos = initialPos + moveDelta;

		editor::GizmoResult delta;
		delta.translationDelta = moveDelta;
		delta.rotationDelta = math::Vec3f{ 0, 0, 0 };
		delta.scaleDelta = math::Vec3f{ 1, 1, 1 };

		// Apply the transform delta using the proper constructor
		gizmoSystem.applyTransformDelta( delta );

		// Update systems to process any changes
		systemManager.update( scene, 0.016f );

		// Verify the transform component was updated
		const auto *transform = scene.getComponent<components::Transform>( entity );
		REQUIRE( transform->position.x == Approx( expectedNewPos.x ) );
		REQUIRE( transform->position.y == Approx( expectedNewPos.y ) );
		REQUIRE( transform->position.z == Approx( expectedNewPos.z ) );

		// VERIFICATION: With the fix, world matrix should be updated correctly
		const auto fixedWorldMatrix = transformSystem->getWorldTransform( scene, entity );
		// This should pass because the proper constructor marks entities as dirty in TransformSystem
		REQUIRE( fixedWorldMatrix.m03() == Approx( expectedNewPos.x ) ); // Now at new position!
		REQUIRE( fixedWorldMatrix.m13() == Approx( expectedNewPos.y ) ); // Now at new position!
		REQUIRE( fixedWorldMatrix.m23() == Approx( expectedNewPos.z ) ); // Now at new position!

		// This demonstrates that world matrix is properly updated, fixing the selection bug
	}
}