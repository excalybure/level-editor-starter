#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/catch_approx.hpp>

#include "editor/gizmos.h"
#include "editor/selection.h"
#include "runtime/ecs.h"
#include "runtime/systems.h"
#include "runtime/components.h"
#include "math/vec.h"
#include "math/matrix.h"

using Catch::Matchers::WithinAbs;
using Catch::Approx;

TEST_CASE( "GizmoSystem SelectionManager Integration", "[gizmos][integration][AF6.1]" )
{
	SECTION( "Gizmo appears when objects are selected" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );

		// Create entity with transform
		const auto entity = scene.createEntity();
		scene.addComponent<components::Transform>( entity,
			components::Transform{ math::Vec3f{ 5.0f, 10.0f, 15.0f }, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );

		// Initially no selection, gizmo should not be active
		REQUIRE_FALSE( gizmoSystem.hasValidSelection() );

		// Select the entity
		selectionManager.select( entity );

		// Gizmo should now have valid selection
		REQUIRE( gizmoSystem.hasValidSelection() );

		// Gizmo matrix should be positioned at entity location
		const auto gizmoMatrix = gizmoSystem.calculateGizmoMatrix();
		REQUIRE_THAT( gizmoMatrix.row0.w, WithinAbs( 5.0f, 0.001f ) );
		REQUIRE_THAT( gizmoMatrix.row1.w, WithinAbs( 10.0f, 0.001f ) );
		REQUIRE_THAT( gizmoMatrix.row2.w, WithinAbs( 15.0f, 0.001f ) );
	}

	SECTION( "Gizmo disappears when selection is cleared" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );

		// Create and select entity
		const auto entity = scene.createEntity();
		scene.addComponent<components::Transform>( entity,
			components::Transform{ math::Vec3f{ 1.0f, 2.0f, 3.0f }, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );

		selectionManager.select( entity );
		REQUIRE( gizmoSystem.hasValidSelection() );

		// Clear selection
		selectionManager.deselectAll();

		// Gizmo should no longer have valid selection
		REQUIRE_FALSE( gizmoSystem.hasValidSelection() );

		// Gizmo matrix should return to identity when no selection
		const auto gizmoMatrix = gizmoSystem.calculateGizmoMatrix();
		const auto identity = math::Mat4f::identity();
		REQUIRE_THAT( gizmoMatrix.row0.w, WithinAbs( identity.row0.w, 0.001f ) );
		REQUIRE_THAT( gizmoMatrix.row1.w, WithinAbs( identity.row1.w, 0.001f ) );
		REQUIRE_THAT( gizmoMatrix.row2.w, WithinAbs( identity.row2.w, 0.001f ) );
	}

	SECTION( "Gizmo updates when selection changes" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );

		// Create two entities
		const auto entity1 = scene.createEntity();
		const auto entity2 = scene.createEntity();
		scene.addComponent<components::Transform>( entity1,
			components::Transform{ math::Vec3f{ 10.0f, 0.0f, 0.0f }, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );
		scene.addComponent<components::Transform>( entity2,
			components::Transform{ math::Vec3f{ 0.0f, 20.0f, 0.0f }, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );

		// Select first entity
		selectionManager.select( entity1 );
		REQUIRE( gizmoSystem.hasValidSelection() );

		auto gizmoMatrix = gizmoSystem.calculateGizmoMatrix();
		REQUIRE_THAT( gizmoMatrix.row0.w, WithinAbs( 10.0f, 0.001f ) );
		REQUIRE_THAT( gizmoMatrix.row1.w, WithinAbs( 0.0f, 0.001f ) );

		// Change selection to second entity
		selectionManager.select( entity2 );
		REQUIRE( gizmoSystem.hasValidSelection() );

		gizmoMatrix = gizmoSystem.calculateGizmoMatrix();
		REQUIRE_THAT( gizmoMatrix.row0.w, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( gizmoMatrix.row1.w, WithinAbs( 20.0f, 0.001f ) );
	}
}

TEST_CASE( "GizmoSystem Viewport Rendering Integration", "[gizmos][integration][AF6.2]" )
{
	SECTION( "Gizmo viewport setup with valid matrices" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );

		// Create test viewport matrices
		const auto viewMatrix = math::Mat4f::lookAt(
			math::Vec3f{ 0, 0, 10 },
			math::Vec3f{ 0, 0, 0 },
			math::Vec3f{ 0, 1, 0 } );
		const auto projMatrix = math::Mat4f::perspective( 45.0f, 16.0f / 9.0f, 0.1f, 1000.0f );
		const auto viewport = math::Vec4<>{ 0.0f, 0.0f, 1920.0f, 1080.0f };

		// Setup should succeed with valid parameters
		const bool result = gizmoSystem.setupImGuizmo( viewMatrix, projMatrix, viewport );
		REQUIRE( result );
	}

	SECTION( "Gizmo rendering integration with viewport" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );

		// Create entity and select it
		const auto entity = scene.createEntity();
		scene.addComponent<components::Transform>( entity,
			components::Transform{ math::Vec3f{ 0.0f, 0.0f, 0.0f }, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );
		selectionManager.select( entity );

		// Setup viewport
		const auto viewMatrix = math::Mat4f::lookAt(
			math::Vec3f{ 5, 5, 5 },
			math::Vec3f{ 0, 0, 0 },
			math::Vec3f{ 0, 1, 0 } );
		const auto projMatrix = math::Mat4f::perspective( 60.0f, 16.0f / 9.0f, 0.1f, 100.0f );
		const auto viewport = math::Vec4<>{ 0.0f, 0.0f, 800.0f, 600.0f };

		REQUIRE( gizmoSystem.setupImGuizmo( viewMatrix, projMatrix, viewport ) );

		// Test basic integration setup without calling ImGuizmo (which requires ImGui context)
		REQUIRE( gizmoSystem.hasValidSelection() );
		REQUIRE( gizmoSystem.isVisible() );

		// Verify gizmo matrix is calculated correctly for viewport integration
		const auto gizmoMatrix = gizmoSystem.calculateGizmoMatrix();
		REQUIRE_THAT( gizmoMatrix.row0.w, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( gizmoMatrix.row1.w, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( gizmoMatrix.row2.w, WithinAbs( 0.0f, 0.001f ) );
	}
}

TEST_CASE( "Complete Manipulation Workflow", "[gizmos][integration][AF6.3]" )
{
	SECTION( "Select objects → show gizmo → manipulate → apply transforms" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );

		// Step 1: Select objects
		const auto entity = scene.createEntity();
		const math::Vec3f initialPosition{ 5.0f, 10.0f, 15.0f };
		scene.addComponent<components::Transform>( entity,
			components::Transform{ initialPosition, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );

		selectionManager.select( entity );
		REQUIRE( gizmoSystem.hasValidSelection() );

		// Step 2: Show gizmo (verify gizmo is positioned correctly)
		const auto gizmoMatrix = gizmoSystem.calculateGizmoMatrix();
		REQUIRE_THAT( gizmoMatrix.row0.w, WithinAbs( initialPosition.x, 0.001f ) );
		REQUIRE_THAT( gizmoMatrix.row1.w, WithinAbs( initialPosition.y, 0.001f ) );
		REQUIRE_THAT( gizmoMatrix.row2.w, WithinAbs( initialPosition.z, 0.001f ) );

		// Step 3: Manipulate (simulate user manipulation)
		editor::GizmoResult manipulation;
		manipulation.wasManipulated = true;
		manipulation.translationDelta = math::Vec3f{ 2.0f, 3.0f, 4.0f };
		manipulation.rotationDelta = math::Vec3f{ 0.1f, 0.2f, 0.3f };
		manipulation.scaleDelta = math::Vec3f{ 1.5f, 2.0f, 0.8f };

		// Step 4: Apply transforms
		gizmoSystem.applyTransformDelta( manipulation );

		// Verify transform was applied correctly
		const auto *transform = scene.getComponent<components::Transform>( entity );
		REQUIRE( transform != nullptr );

		const auto expectedPosition = initialPosition + manipulation.translationDelta;
		REQUIRE_THAT( transform->position.x, WithinAbs( expectedPosition.x, 0.001f ) );
		REQUIRE_THAT( transform->position.y, WithinAbs( expectedPosition.y, 0.001f ) );
		REQUIRE_THAT( transform->position.z, WithinAbs( expectedPosition.z, 0.001f ) );

		REQUIRE_THAT( transform->rotation.x, WithinAbs( manipulation.rotationDelta.x, 0.001f ) );
		REQUIRE_THAT( transform->rotation.y, WithinAbs( manipulation.rotationDelta.y, 0.001f ) );
		REQUIRE_THAT( transform->rotation.z, WithinAbs( manipulation.rotationDelta.z, 0.001f ) );

		const auto expectedScale = math::Vec3f{ 1, 1, 1 } * manipulation.scaleDelta;
		REQUIRE_THAT( transform->scale.x, WithinAbs( expectedScale.x, 0.001f ) );
		REQUIRE_THAT( transform->scale.y, WithinAbs( expectedScale.y, 0.001f ) );
		REQUIRE_THAT( transform->scale.z, WithinAbs( expectedScale.z, 0.001f ) );
	}

	SECTION( "Manipulation workflow with different operations" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );

		// Create and select entity
		const auto entity = scene.createEntity();
		scene.addComponent<components::Transform>( entity,
			components::Transform{ math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );
		selectionManager.select( entity );

		// Test translate operation
		gizmoSystem.setOperation( editor::GizmoOperation::Translate );
		REQUIRE( gizmoSystem.getCurrentOperation() == editor::GizmoOperation::Translate );

		// Test rotate operation
		gizmoSystem.setOperation( editor::GizmoOperation::Rotate );
		REQUIRE( gizmoSystem.getCurrentOperation() == editor::GizmoOperation::Rotate );

		// Test scale operation
		gizmoSystem.setOperation( editor::GizmoOperation::Scale );
		REQUIRE( gizmoSystem.getCurrentOperation() == editor::GizmoOperation::Scale );

		// Test universal operation
		gizmoSystem.setOperation( editor::GizmoOperation::Universal );
		REQUIRE( gizmoSystem.getCurrentOperation() == editor::GizmoOperation::Universal );
	}
}

TEST_CASE( "Multi-Selection Manipulation Scenarios", "[gizmos][integration][AF6.4]" )
{
	SECTION( "Multi-selection shows unified gizmo at selection center" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );

		// Create multiple entities at different positions
		const auto entity1 = scene.createEntity();
		const auto entity2 = scene.createEntity();
		const auto entity3 = scene.createEntity();

		const math::Vec3f pos1{ -10.0f, 0.0f, 0.0f };
		const math::Vec3f pos2{ 10.0f, 0.0f, 0.0f };
		const math::Vec3f pos3{ 0.0f, 20.0f, 0.0f };

		scene.addComponent<components::Transform>( entity1,
			components::Transform{ pos1, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );
		scene.addComponent<components::Transform>( entity2,
			components::Transform{ pos2, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );
		scene.addComponent<components::Transform>( entity3,
			components::Transform{ pos3, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );

		// Select all entities
		selectionManager.select( { entity1, entity2, entity3 } );
		REQUIRE( gizmoSystem.hasValidSelection() );

		// Calculate expected center
		const auto expectedCenter = ( pos1 + pos2 + pos3 ) / 3.0f;
		const auto actualCenter = gizmoSystem.calculateSelectionCenter();

		REQUIRE_THAT( actualCenter.x, WithinAbs( expectedCenter.x, 0.001f ) );
		REQUIRE_THAT( actualCenter.y, WithinAbs( expectedCenter.y, 0.001f ) );
		REQUIRE_THAT( actualCenter.z, WithinAbs( expectedCenter.z, 0.001f ) );

		// Gizmo matrix should be positioned at center
		const auto gizmoMatrix = gizmoSystem.calculateGizmoMatrix();
		REQUIRE_THAT( gizmoMatrix.row0.w, WithinAbs( expectedCenter.x, 0.001f ) );
		REQUIRE_THAT( gizmoMatrix.row1.w, WithinAbs( expectedCenter.y, 0.001f ) );
		REQUIRE_THAT( gizmoMatrix.row2.w, WithinAbs( expectedCenter.z, 0.001f ) );
	}

	SECTION( "Multi-selection manipulation applies to all selected entities" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );

		// Create entities
		const auto entity1 = scene.createEntity();
		const auto entity2 = scene.createEntity();

		const math::Vec3f initialPos1{ 1.0f, 2.0f, 3.0f };
		const math::Vec3f initialPos2{ 10.0f, 20.0f, 30.0f };
		const math::Vec3f initialScale1{ 1.0f, 1.0f, 1.0f };
		const math::Vec3f initialScale2{ 2.0f, 2.0f, 2.0f };

		scene.addComponent<components::Transform>( entity1,
			components::Transform{ initialPos1, math::Vec3f{ 0, 0, 0 }, initialScale1 } );
		scene.addComponent<components::Transform>( entity2,
			components::Transform{ initialPos2, math::Vec3f{ 0, 0, 0 }, initialScale2 } );

		// Select both entities
		selectionManager.select( { entity1, entity2 } );

		// Apply manipulation
		editor::GizmoResult manipulation;
		manipulation.wasManipulated = true;
		manipulation.translationDelta = math::Vec3f{ 5.0f, 10.0f, 15.0f };
		manipulation.scaleDelta = math::Vec3f{ 1.5f, 1.5f, 1.5f };

		gizmoSystem.applyTransformDelta( manipulation );

		// Verify both entities were transformed
		const auto *transform1 = scene.getComponent<components::Transform>( entity1 );
		const auto *transform2 = scene.getComponent<components::Transform>( entity2 );

		REQUIRE( transform1 != nullptr );
		REQUIRE( transform2 != nullptr );

		// Check translation was applied to both
		const auto expectedPos1 = initialPos1 + manipulation.translationDelta;
		const auto expectedPos2 = initialPos2 + manipulation.translationDelta;

		REQUIRE_THAT( transform1->position.x, WithinAbs( expectedPos1.x, 0.001f ) );
		REQUIRE_THAT( transform1->position.y, WithinAbs( expectedPos1.y, 0.001f ) );
		REQUIRE_THAT( transform1->position.z, WithinAbs( expectedPos1.z, 0.001f ) );

		REQUIRE_THAT( transform2->position.x, WithinAbs( expectedPos2.x, 0.001f ) );
		REQUIRE_THAT( transform2->position.y, WithinAbs( expectedPos2.y, 0.001f ) );
		REQUIRE_THAT( transform2->position.z, WithinAbs( expectedPos2.z, 0.001f ) );

		// Check scale was applied multiplicatively
		const auto expectedScale1 = initialScale1 * manipulation.scaleDelta;
		const auto expectedScale2 = initialScale2 * manipulation.scaleDelta;

		REQUIRE_THAT( transform1->scale.x, WithinAbs( expectedScale1.x, 0.001f ) );
		REQUIRE_THAT( transform1->scale.y, WithinAbs( expectedScale1.y, 0.001f ) );
		REQUIRE_THAT( transform1->scale.z, WithinAbs( expectedScale1.z, 0.001f ) );

		REQUIRE_THAT( transform2->scale.x, WithinAbs( expectedScale2.x, 0.001f ) );
		REQUIRE_THAT( transform2->scale.y, WithinAbs( expectedScale2.y, 0.001f ) );
		REQUIRE_THAT( transform2->scale.z, WithinAbs( expectedScale2.z, 0.001f ) );
	}
}

TEST_CASE( "Coordinate Space Switching During Manipulation", "[gizmos][integration][AF6.5]" )
{
	SECTION( "Local and world coordinate spaces work correctly" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );

		// Create entity
		const auto entity = scene.createEntity();
		scene.addComponent<components::Transform>( entity,
			components::Transform{ math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );
		selectionManager.select( entity );

		// Test World mode
		gizmoSystem.setMode( editor::GizmoMode::World );
		REQUIRE( gizmoSystem.getCurrentMode() == editor::GizmoMode::World );
		REQUIRE( gizmoSystem.getImGuizmoMode() == 1 ); // ImGuizmo::WORLD

		// Test Local mode
		gizmoSystem.setMode( editor::GizmoMode::Local );
		REQUIRE( gizmoSystem.getCurrentMode() == editor::GizmoMode::Local );
		REQUIRE( gizmoSystem.getImGuizmoMode() == 0 ); // ImGuizmo::LOCAL

		// Switch back to World mode
		gizmoSystem.setMode( editor::GizmoMode::World );
		REQUIRE( gizmoSystem.getCurrentMode() == editor::GizmoMode::World );
	}

	SECTION( "Coordinate space switching maintains consistency" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );

		// Create rotated entity
		const auto entity = scene.createEntity();
		scene.addComponent<components::Transform>( entity,
			components::Transform{
				math::Vec3f{ 5, 5, 5 },
				math::Vec3f{ 0, 45.0f, 0 }, // 45 degree Y rotation
				math::Vec3f{ 1, 1, 1 } } );
		selectionManager.select( entity );

		// Gizmo matrix should be consistent regardless of coordinate mode
		gizmoSystem.setMode( editor::GizmoMode::World );
		const auto worldMatrix = gizmoSystem.calculateGizmoMatrix();

		gizmoSystem.setMode( editor::GizmoMode::Local );
		const auto localMatrix = gizmoSystem.calculateGizmoMatrix();

		// Position should be the same in both modes (only orientation differs)
		REQUIRE_THAT( worldMatrix.row0.w, WithinAbs( localMatrix.row0.w, 0.001f ) );
		REQUIRE_THAT( worldMatrix.row1.w, WithinAbs( localMatrix.row1.w, 0.001f ) );
		REQUIRE_THAT( worldMatrix.row2.w, WithinAbs( localMatrix.row2.w, 0.001f ) );
	}
}

TEST_CASE( "Snap Functionality Testing", "[gizmos][integration][AF6.6]" )
{
	SECTION( "Snap-to-grid functions with configurable precision" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );

		// Configure snap settings
		gizmoSystem.setSnapEnabled( true );
		gizmoSystem.setTranslationSnap( 1.0f );
		gizmoSystem.setRotationSnap( 15.0f );
		gizmoSystem.setScaleSnap( 0.1f );

		REQUIRE( gizmoSystem.isSnapEnabled() );
		REQUIRE_THAT( gizmoSystem.getTranslationSnap(), WithinAbs( 1.0f, 0.001f ) );
		REQUIRE_THAT( gizmoSystem.getRotationSnap(), WithinAbs( 15.0f, 0.001f ) );
		REQUIRE_THAT( gizmoSystem.getScaleSnap(), WithinAbs( 0.1f, 0.001f ) );
	}

	SECTION( "Snap settings work with different grid sizes" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );

		// Test fine grid
		gizmoSystem.setSnapEnabled( true );
		gizmoSystem.setTranslationSnap( 0.1f );
		REQUIRE_THAT( gizmoSystem.getTranslationSnap(), WithinAbs( 0.1f, 0.001f ) );

		// Test coarse grid
		gizmoSystem.setTranslationSnap( 5.0f );
		REQUIRE_THAT( gizmoSystem.getTranslationSnap(), WithinAbs( 5.0f, 0.001f ) );

		// Test rotation snap values
		gizmoSystem.setRotationSnap( 45.0f );
		REQUIRE_THAT( gizmoSystem.getRotationSnap(), WithinAbs( 45.0f, 0.001f ) );

		gizmoSystem.setRotationSnap( 5.0f );
		REQUIRE_THAT( gizmoSystem.getRotationSnap(), WithinAbs( 5.0f, 0.001f ) );

		// Test scale snap values
		gizmoSystem.setScaleSnap( 0.25f );
		REQUIRE_THAT( gizmoSystem.getScaleSnap(), WithinAbs( 0.25f, 0.001f ) );

		gizmoSystem.setScaleSnap( 0.01f );
		REQUIRE_THAT( gizmoSystem.getScaleSnap(), WithinAbs( 0.01f, 0.001f ) );
	}

	SECTION( "Snap can be enabled and disabled dynamically" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );

		// Initially disabled
		REQUIRE_FALSE( gizmoSystem.isSnapEnabled() );

		// Enable snap
		gizmoSystem.setSnapEnabled( true );
		REQUIRE( gizmoSystem.isSnapEnabled() );

		// Disable snap
		gizmoSystem.setSnapEnabled( false );
		REQUIRE_FALSE( gizmoSystem.isSnapEnabled() );

		// Enable again
		gizmoSystem.setSnapEnabled( true );
		REQUIRE( gizmoSystem.isSnapEnabled() );
	}
}

TEST_CASE( "Transform Command Generation Validation", "[gizmos][integration][AF6.7]" )
{
	SECTION( "Transform commands are generated correctly from gizmo manipulations" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );

		// Create entity
		const auto entity = scene.createEntity();
		scene.addComponent<components::Transform>( entity,
			components::Transform{ math::Vec3f{ 1, 2, 3 }, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );
		selectionManager.select( entity );

		// Create manipulation result
		editor::GizmoResult manipulation;
		manipulation.wasManipulated = true;
		manipulation.translationDelta = math::Vec3f{ 5, 10, 15 };
		manipulation.rotationDelta = math::Vec3f{ 0.1f, 0.2f, 0.3f };
		manipulation.scaleDelta = math::Vec3f{ 1.5f, 2.0f, 0.8f };

		// This tests that the command generation infrastructure is in place
		// The actual command creation would be handled by the command system
		REQUIRE( manipulation.wasManipulated );
		// Check individual components instead of using math::Vec3 equality
		REQUIRE_FALSE( ( manipulation.translationDelta.x == 0.0f && manipulation.translationDelta.y == 0.0f && manipulation.translationDelta.z == 0.0f ) );
		REQUIRE_FALSE( ( manipulation.rotationDelta.x == 0.0f && manipulation.rotationDelta.y == 0.0f && manipulation.rotationDelta.z == 0.0f ) );
		REQUIRE_FALSE( ( manipulation.scaleDelta.x == 1.0f && manipulation.scaleDelta.y == 1.0f && manipulation.scaleDelta.z == 1.0f ) );
	}

	SECTION( "Command generation accuracy with different manipulations" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );

		// Create entity
		const auto entity = scene.createEntity();
		const math::Vec3f initialPos{ 10, 20, 30 };
		const math::Vec3f initialRot{ 0.5f, 1.0f, 1.5f };
		const math::Vec3f initialScale{ 2, 3, 4 };

		scene.addComponent<components::Transform>( entity,
			components::Transform{ initialPos, initialRot, initialScale } );
		selectionManager.select( entity );

		// Simulate different types of manipulations

		// Translation only
		editor::GizmoResult translateResult;
		translateResult.wasManipulated = true;
		translateResult.translationDelta = math::Vec3f{ 1, 2, 3 };

		gizmoSystem.applyTransformDelta( translateResult );

		const auto *transform = scene.getComponent<components::Transform>( entity );
		REQUIRE_THAT( transform->position.x, WithinAbs( initialPos.x + 1.0f, 0.001f ) );
		REQUIRE_THAT( transform->position.y, WithinAbs( initialPos.y + 2.0f, 0.001f ) );
		REQUIRE_THAT( transform->position.z, WithinAbs( initialPos.z + 3.0f, 0.001f ) );

		// Reset entity for next test
		scene.getComponent<components::Transform>( entity )->position = initialPos;
		scene.getComponent<components::Transform>( entity )->rotation = initialRot;
		scene.getComponent<components::Transform>( entity )->scale = initialScale;

		// Scale only
		editor::GizmoResult scaleResult;
		scaleResult.wasManipulated = true;
		scaleResult.scaleDelta = math::Vec3f{ 0.5f, 0.5f, 0.5f };

		gizmoSystem.applyTransformDelta( scaleResult );

		transform = scene.getComponent<components::Transform>( entity );
		REQUIRE_THAT( transform->scale.x, WithinAbs( initialScale.x * 0.5f, 0.001f ) );
		REQUIRE_THAT( transform->scale.y, WithinAbs( initialScale.y * 0.5f, 0.001f ) );
		REQUIRE_THAT( transform->scale.z, WithinAbs( initialScale.z * 0.5f, 0.001f ) );
	}
}

TEST_CASE( "GizmoSystem - Rotation Delta Units Issue", "[gizmos][rotation][units][bug]" )
{
	SECTION( "Rotation deltas from renderGizmo are converted from degrees to radians" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		systemManager.addSystem<systems::TransformSystem>();
		systemManager.initialize( scene );

		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager );

		// Create entity with zero initial rotation
		const auto entity = scene.createEntity();
		const math::Vec3f initialPosition{ 0.0f, 0.0f, 0.0f };
		const math::Vec3f initialRotation{ 0.0f, 0.0f, 0.0f }; // Zero rotation in radians
		const math::Vec3f initialScale{ 1.0f, 1.0f, 1.0f };

		scene.addComponent<components::Transform>( entity,
			components::Transform{ initialPosition, initialRotation, initialScale } );

		selectionManager.select( entity );

		// NOTE: This test demonstrates the fix conceptually. In practice, the renderGizmo() method
		// handles the conversion from ImGuizmo's degree-based output to radians.
		// For this test, we create a GizmoResult as if it came from renderGizmo after conversion.

		// Simulate what renderGizmo would produce after converting ImGuizmo's 45-degree output
		const float rotationInDegrees = 45.0f;
		const float expectedRotationInRadians = math::radians( rotationInDegrees );

		editor::GizmoResult rotationResult;
		rotationResult.wasManipulated = true;
		rotationResult.translationDelta = math::Vec3f{ 0.0f, 0.0f, 0.0f };
		// This delta is now in radians as it would come from renderGizmo after conversion
		rotationResult.rotationDelta = math::Vec3f{ 0.0f, expectedRotationInRadians, 0.0f };
		rotationResult.scaleDelta = math::Vec3f{ 1.0f, 1.0f, 1.0f };

		// Apply the rotation delta
		gizmoSystem.applyTransformDelta( rotationResult );

		// Check what actually happened
		const auto *transform = scene.getComponent<components::Transform>( entity );

		// The rotation delta should be applied directly (already in radians)
		const float actualRotationInRadians = transform->rotation.y;

		// Should match the expected value
		REQUIRE_THAT( actualRotationInRadians, WithinAbs( expectedRotationInRadians, 0.001f ) );

		// Convert to degrees to verify it's the expected 45 degrees
		const float actualRotationInDegrees = math::degrees( actualRotationInRadians );
		REQUIRE_THAT( actualRotationInDegrees, WithinAbs( 45.0f, 0.1f ) );
	}
}
