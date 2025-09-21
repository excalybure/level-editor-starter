#include <catch2/catch_test_macros.hpp>

#include "editor/gizmos.h"
#include "editor/selection.h"
#include "engine/math/vec.h"
#include "engine/math/matrix.h"
#include "runtime/ecs.h"
#include "runtime/systems.h"
#include "runtime/components.h"
#include <imgui.h>
#include <ImGuizmo.h>


TEST_CASE( "GizmoOperation enum values", "[gizmos][unit][operation]" )
{
	SECTION( "GizmoOperation has correct enum values" )
	{
		REQUIRE( static_cast<int>( editor::GizmoOperation::Translate ) == 0 );
		REQUIRE( static_cast<int>( editor::GizmoOperation::Rotate ) == 1 );
		REQUIRE( static_cast<int>( editor::GizmoOperation::Scale ) == 2 );
		REQUIRE( static_cast<int>( editor::GizmoOperation::Universal ) == 3 );
	}

	SECTION( "GizmoOperation enum can be compared" )
	{
		const auto op1 = editor::GizmoOperation::Translate;
		const auto op2 = editor::GizmoOperation::Translate;
		const auto op3 = editor::GizmoOperation::Rotate;

		REQUIRE( op1 == op2 );
		REQUIRE( op1 != op3 );
	}
}

TEST_CASE( "GizmoMode enum values", "[gizmos][unit][mode]" )
{
	SECTION( "GizmoMode has correct enum values" )
	{
		REQUIRE( static_cast<int>( editor::GizmoMode::Local ) == 0 );
		REQUIRE( static_cast<int>( editor::GizmoMode::World ) == 1 );
	}

	SECTION( "GizmoMode enum can be compared" )
	{
		const auto mode1 = editor::GizmoMode::Local;
		const auto mode2 = editor::GizmoMode::Local;
		const auto mode3 = editor::GizmoMode::World;

		REQUIRE( mode1 == mode2 );
		REQUIRE( mode1 != mode3 );
	}
}

TEST_CASE( "GizmoResult struct default values and manipulation flags", "[gizmos][unit][result]" )
{
	SECTION( "GizmoResult has correct default values" )
	{
		const editor::GizmoResult result;

		REQUIRE_FALSE( result.wasManipulated );
		REQUIRE_FALSE( result.isManipulating );

		// Check individual components instead of using math::Vec3 equality
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
		editor::GizmoResult result;
		result.wasManipulated = true;
		result.isManipulating = true;

		REQUIRE( result.wasManipulated );
		REQUIRE( result.isManipulating );
	}

	SECTION( "GizmoResult delta values can be set" )
	{
		editor::GizmoResult result;
		result.translationDelta = math::Vec3<>{ 1.0f, 2.0f, 3.0f };
		result.rotationDelta = math::Vec3<>{ 0.1f, 0.2f, 0.3f };
		result.scaleDelta = math::Vec3<>{ 1.5f, 2.0f, 0.5f };

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
		const editor::GizmoSystem system;

		// Should have default values for operation and mode
		REQUIRE( system.getCurrentOperation() == editor::GizmoOperation::Translate );
		REQUIRE( system.getCurrentMode() == editor::GizmoMode::World );
	}

	SECTION( "GizmoSystem operation and mode can be set" )
	{
		editor::GizmoSystem system;

		system.setOperation( editor::GizmoOperation::Rotate );
		system.setMode( editor::GizmoMode::Local );

		REQUIRE( system.getCurrentOperation() == editor::GizmoOperation::Rotate );
		REQUIRE( system.getCurrentMode() == editor::GizmoMode::Local );
	}
}

TEST_CASE( "GizmoSystem with SelectionManager", "[gizmos][unit][selection]" )
{
	SECTION( "GizmoSystem can be constructed with SelectionManager and Scene" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );

		const editor::GizmoSystem system( selectionManager, scene );

		// Should have default values for operation and mode
		REQUIRE( system.getCurrentOperation() == editor::GizmoOperation::Translate );
		REQUIRE( system.getCurrentMode() == editor::GizmoMode::World );
	}
}

TEST_CASE( "GizmoSystem settings with snap values", "[gizmos][unit][settings]" )
{
	SECTION( "GizmoSystem has default snap values" )
	{
		const editor::GizmoSystem system;

		REQUIRE( system.getTranslationSnap() == 1.0f );
		REQUIRE( system.getRotationSnap() == 15.0f ); // 15 degrees
		REQUIRE( system.getScaleSnap() == 0.1f );
		REQUIRE_FALSE( system.isSnapEnabled() );
	}

	SECTION( "GizmoSystem snap values can be set" )
	{
		editor::GizmoSystem system;

		system.setTranslationSnap( 0.5f );
		system.setRotationSnap( 30.0f );
		system.setScaleSnap( 0.25f );
		system.setSnapEnabled( true );

		REQUIRE( system.getTranslationSnap() == 0.5f );
		REQUIRE( system.getRotationSnap() == 30.0f );
		REQUIRE( system.getScaleSnap() == 0.25f );
		REQUIRE( system.isSnapEnabled() );
	}
}

TEST_CASE( "GizmoSystem visibility control", "[gizmos][unit][visibility]" )
{
	SECTION( "GizmoSystem starts visible by default" )
	{
		const editor::GizmoSystem system;

		REQUIRE( system.isVisible() );
	}

	SECTION( "GizmoSystem visibility can be toggled" )
	{
		editor::GizmoSystem system;

		system.setVisible( false );
		REQUIRE_FALSE( system.isVisible() );

		system.setVisible( true );
		REQUIRE( system.isVisible() );
	}
}

TEST_CASE( "GizmoSystem selection center calculation", "[gizmos][unit][center]" )
{
	SECTION( "GizmoSystem calculates center for single entity" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem system( selectionManager, scene );

		// Create entity with transform
		const auto entity = scene.createEntity();
		scene.addComponent<components::Transform>( entity,
			components::Transform{ math::Vec3f{ 5.0f, 10.0f, 15.0f }, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );

		// Select the entity
		selectionManager.select( entity );

		// Calculate center
		const auto center = system.calculateSelectionCenter();

		REQUIRE( center.x == 5.0f );
		REQUIRE( center.y == 10.0f );
		REQUIRE( center.z == 15.0f );
	}

	SECTION( "GizmoSystem calculates center for multiple entities" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem system( selectionManager, scene );

		// Create entities with transforms
		const auto entity1 = scene.createEntity();
		const auto entity2 = scene.createEntity();
		scene.addComponent<components::Transform>( entity1,
			components::Transform{ math::Vec3f{ 0.0f, 0.0f, 0.0f }, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );
		scene.addComponent<components::Transform>( entity2,
			components::Transform{ math::Vec3f{ 10.0f, 20.0f, 30.0f }, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );

		// Select both entities
		selectionManager.select( { entity1, entity2 } );

		// Calculate center (should be average)
		const auto center = system.calculateSelectionCenter();

		REQUIRE( center.x == 5.0f );  // (0+10)/2
		REQUIRE( center.y == 10.0f ); // (0+20)/2
		REQUIRE( center.z == 15.0f ); // (0+30)/2
	}

	SECTION( "GizmoSystem returns zero for empty selection" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem system( selectionManager, scene );

		// No selection
		const auto center = system.calculateSelectionCenter();

		REQUIRE( center.x == 0.0f );
		REQUIRE( center.y == 0.0f );
		REQUIRE( center.z == 0.0f );
	}
}

TEST_CASE( "GizmoSystem matrix calculation", "[gizmos][unit][matrix]" )
{
	SECTION( "GizmoSystem calculates gizmo matrix for single entity" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem system( selectionManager, scene );

		// Create entity with transform
		const auto entity = scene.createEntity();
		scene.addComponent<components::Transform>( entity,
			components::Transform{ math::Vec3f{ 2.0f, 4.0f, 6.0f }, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );

		// Select the entity
		selectionManager.select( entity );

		// Calculate gizmo matrix
		const auto matrix = system.calculateGizmoMatrix();

		// Matrix should be positioned at entity's position (translation in row0.w, row1.w, row2.w)
		REQUIRE( matrix.row0.w == 2.0f );
		REQUIRE( matrix.row1.w == 4.0f );
		REQUIRE( matrix.row2.w == 6.0f );
	}

	SECTION( "GizmoSystem calculates gizmo matrix for multiple entities" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem system( selectionManager, scene );

		// Create entities with transforms
		const auto entity1 = scene.createEntity();
		const auto entity2 = scene.createEntity();
		scene.addComponent<components::Transform>( entity1,
			components::Transform{ math::Vec3f{ 0.0f, 0.0f, 0.0f }, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );
		scene.addComponent<components::Transform>( entity2,
			components::Transform{ math::Vec3f{ 4.0f, 8.0f, 12.0f }, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );

		// Select both entities
		selectionManager.select( { entity1, entity2 } );

		// Calculate gizmo matrix (should be centered at average position)
		const auto matrix = system.calculateGizmoMatrix();

		// Matrix should be positioned at center of selection (translation in row0.w, row1.w, row2.w)
		REQUIRE( matrix.row0.w == 2.0f ); // (0+4)/2
		REQUIRE( matrix.row1.w == 4.0f ); // (0+8)/2
		REQUIRE( matrix.row2.w == 6.0f ); // (0+12)/2
	}

	SECTION( "GizmoSystem returns identity matrix for empty selection" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem system( selectionManager, scene );

		// No selection
		const auto matrix = system.calculateGizmoMatrix();

		// Should be identity matrix
		const auto identity = math::Mat4f::identity();
		REQUIRE( matrix.row0.x == identity.row0.x );
		REQUIRE( matrix.row0.y == identity.row0.y );
		REQUIRE( matrix.row0.z == identity.row0.z );
		REQUIRE( matrix.row0.w == identity.row0.w );
		REQUIRE( matrix.row1.x == identity.row1.x );
		REQUIRE( matrix.row1.y == identity.row1.y );
		REQUIRE( matrix.row1.z == identity.row1.z );
		REQUIRE( matrix.row1.w == identity.row1.w );
		REQUIRE( matrix.row2.x == identity.row2.x );
		REQUIRE( matrix.row2.y == identity.row2.y );
		REQUIRE( matrix.row2.z == identity.row2.z );
		REQUIRE( matrix.row2.w == identity.row2.w );
		REQUIRE( matrix.row3.x == identity.row3.x );
		REQUIRE( matrix.row3.y == identity.row3.y );
		REQUIRE( matrix.row3.z == identity.row3.z );
		REQUIRE( matrix.row3.w == identity.row3.w );
	}
}

TEST_CASE( "GizmoSystem transform delta application", "[gizmos][unit][delta]" )
{
	SECTION( "GizmoSystem applies transform delta to single entity" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem system( selectionManager, scene );

		// Create entity with initial transform
		const auto entity = scene.createEntity();
		scene.addComponent<components::Transform>( entity,
			components::Transform{ math::Vec3f{ 1.0f, 2.0f, 3.0f }, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );

		// Select the entity
		selectionManager.select( entity );

		// Create transform delta
		editor::GizmoResult delta;
		delta.translationDelta = math::Vec3f{ 5.0f, 10.0f, 15.0f };
		delta.rotationDelta = math::Vec3f{ 0.1f, 0.2f, 0.3f };
		delta.scaleDelta = math::Vec3f{ 2.0f, 3.0f, 4.0f };

		// Apply delta
		system.applyTransformDelta( delta );

		// Check that entity's transform was updated
		const auto *transform = scene.getComponent<components::Transform>( entity );
		REQUIRE( transform->position.x == 6.0f );  // 1 + 5
		REQUIRE( transform->position.y == 12.0f ); // 2 + 10
		REQUIRE( transform->position.z == 18.0f ); // 3 + 15

		REQUIRE( transform->rotation.x == 0.1f );
		REQUIRE( transform->rotation.y == 0.2f );
		REQUIRE( transform->rotation.z == 0.3f );

		REQUIRE( transform->scale.x == 2.0f ); // 1 * 2
		REQUIRE( transform->scale.y == 3.0f ); // 1 * 3
		REQUIRE( transform->scale.z == 4.0f ); // 1 * 4
	}

	SECTION( "GizmoSystem applies transform delta to multiple entities" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem system( selectionManager, scene );

		// Create entities with initial transforms
		const auto entity1 = scene.createEntity();
		const auto entity2 = scene.createEntity();
		scene.addComponent<components::Transform>( entity1,
			components::Transform{ math::Vec3f{ 1.0f, 2.0f, 3.0f }, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 1, 1, 1 } } );
		scene.addComponent<components::Transform>( entity2,
			components::Transform{ math::Vec3f{ 10.0f, 20.0f, 30.0f }, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 2, 2, 2 } } );

		// Select both entities
		selectionManager.select( { entity1, entity2 } );

		// Create transform delta
		editor::GizmoResult delta;
		delta.translationDelta = math::Vec3f{ 5.0f, 10.0f, 15.0f };
		delta.scaleDelta = math::Vec3f{ 0.5f, 0.5f, 0.5f };

		// Apply delta
		system.applyTransformDelta( delta );

		// Check that both entities were updated
		const auto *transform1 = scene.getComponent<components::Transform>( entity1 );
		const auto *transform2 = scene.getComponent<components::Transform>( entity2 );

		// Translation should be added
		REQUIRE( transform1->position.x == 6.0f );	// 1 + 5
		REQUIRE( transform1->position.y == 12.0f ); // 2 + 10
		REQUIRE( transform1->position.z == 18.0f ); // 3 + 15

		REQUIRE( transform2->position.x == 15.0f ); // 10 + 5
		REQUIRE( transform2->position.y == 30.0f ); // 20 + 10
		REQUIRE( transform2->position.z == 45.0f ); // 30 + 15

		// Scale should be multiplied
		REQUIRE( transform1->scale.x == 0.5f ); // 1 * 0.5
		REQUIRE( transform1->scale.y == 0.5f ); // 1 * 0.5
		REQUIRE( transform1->scale.z == 0.5f ); // 1 * 0.5

		REQUIRE( transform2->scale.x == 1.0f ); // 2 * 0.5
		REQUIRE( transform2->scale.y == 1.0f ); // 2 * 0.5
		REQUIRE( transform2->scale.z == 1.0f ); // 2 * 0.5
	}

	SECTION( "GizmoSystem handles empty selection gracefully" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem system( selectionManager, scene );

		// No selection
		editor::GizmoResult delta;
		delta.translationDelta = math::Vec3f{ 5.0f, 10.0f, 15.0f };

		// Should not crash when applying delta to empty selection
		system.applyTransformDelta( delta );

		// Test passes if no crash occurs
		REQUIRE( true );
	}
}

TEST_CASE( "GizmoSystem state management", "[gizmos][unit][state]" )
{
	SECTION( "GizmoSystem starts with no active manipulation" )
	{
		const editor::GizmoSystem system;

		REQUIRE_FALSE( system.isManipulating() );
		REQUIRE_FALSE( system.wasManipulated() );
	}

	SECTION( "GizmoSystem can track active manipulation state" )
	{
		editor::GizmoSystem system;

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

TEST_CASE( "GizmoSystem ImGuizmo setup", "[gizmos][unit][imguizmo][setup]" )
{
	SECTION( "GizmoSystem can setup ImGuizmo context with viewport matrices" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem system( selectionManager, scene );

		// Create test view and projection matrices
		const auto viewMatrix = math::Mat4f::lookAt( math::Vec3f{ 0, 0, 10 }, math::Vec3f{ 0, 0, 0 }, math::Vec3f{ 0, 1, 0 } );
		const auto projMatrix = math::Mat4f::perspective( 45.0f, 16.0f / 9.0f, 0.1f, 1000.0f );
		const auto viewport = math::Vec4<>{ 0.0f, 0.0f, 1920.0f, 1080.0f };

		// Setup should not crash and should return true for success
		const bool result = system.setupImGuizmo( viewMatrix, projMatrix, viewport );
		REQUIRE( result );
	}

	SECTION( "GizmoSystem setup handles invalid viewport gracefully" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem system( selectionManager, scene );

		// Create test matrices with invalid viewport (zero width/height)
		const auto viewMatrix = math::Mat4f::identity();
		const auto projMatrix = math::Mat4f::identity();
		const auto invalidViewport = math::Vec4<>{ 0.0f, 0.0f, 0.0f, 0.0f };

		// Setup should handle invalid viewport gracefully
		const bool result = system.setupImGuizmo( viewMatrix, projMatrix, invalidViewport );
		REQUIRE_FALSE( result );
	}
}

TEST_CASE( "GizmoSystem ImGuizmo coordinate space configuration", "[gizmos][unit][imguizmo][coordinates]" )
{
	SECTION( "GizmoSystem converts Local mode to ImGuizmo LOCAL" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem system( selectionManager, scene );

		// Set to local mode
		system.setMode( editor::GizmoMode::Local );

		// Test coordinate space conversion
		const auto imguizmoMode = system.getImGuizmoMode();
		REQUIRE( imguizmoMode == 0 ); // ImGuizmo::LOCAL is 0
	}

	SECTION( "GizmoSystem converts World mode to ImGuizmo WORLD" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem system( selectionManager, scene );

		// Set to world mode
		system.setMode( editor::GizmoMode::World );

		// Test coordinate space conversion
		const auto imguizmoMode = system.getImGuizmoMode();
		REQUIRE( imguizmoMode == 1 ); // ImGuizmo::WORLD is 1
	}

	SECTION( "GizmoSystem coordinate space can be switched dynamically" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem system( selectionManager, scene );

		// Start with local mode
		system.setMode( editor::GizmoMode::Local );
		REQUIRE( system.getImGuizmoMode() == 0 );

		// Switch to world mode
		system.setMode( editor::GizmoMode::World );
		REQUIRE( system.getImGuizmoMode() == 1 );

		// Switch back to local mode
		system.setMode( editor::GizmoMode::Local );
		REQUIRE( system.getImGuizmoMode() == 0 );
	}
}

TEST_CASE( "GizmoSystem ImGuizmo operation mode binding", "[gizmos][unit][imguizmo][operations]" )
{
	SECTION( "GizmoSystem converts Translate operation to ImGuizmo TRANSLATE" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem system( selectionManager, scene );

		// Set to translate operation
		system.setOperation( editor::GizmoOperation::Translate );

		// Test operation conversion
		const auto imguizmoOp = system.getImGuizmoOperation();
		REQUIRE( imguizmoOp == 7 ); // ImGuizmo::TRANSLATE is 7
	}

	SECTION( "GizmoSystem converts Rotate operation to ImGuizmo ROTATE" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem system( selectionManager, scene );

		// Set to rotate operation
		system.setOperation( editor::GizmoOperation::Rotate );

		// Test operation conversion
		const auto imguizmoOp = system.getImGuizmoOperation();
		REQUIRE( imguizmoOp == 120 ); // ImGuizmo::ROTATE is 120
	}

	SECTION( "GizmoSystem converts Scale operation to ImGuizmo SCALE" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem system( selectionManager, scene );

		// Set to scale operation
		system.setOperation( editor::GizmoOperation::Scale );

		// Test operation conversion
		const auto imguizmoOp = system.getImGuizmoOperation();
		REQUIRE( imguizmoOp == 896 ); // ImGuizmo::SCALE is 896
	}

	SECTION( "GizmoSystem converts Universal operation to ImGuizmo UNIVERSAL" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem system( selectionManager, scene );

		// Set to universal operation
		system.setOperation( editor::GizmoOperation::Universal );

		// Test operation conversion
		const auto imguizmoOp = system.getImGuizmoOperation();
		REQUIRE( imguizmoOp == 1023 ); // ImGuizmo::UNIVERSAL is 1023 (all operations combined)
	}

	SECTION( "GizmoSystem operation can be switched dynamically" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem system( selectionManager, scene );

		// Start with translate
		system.setOperation( editor::GizmoOperation::Translate );
		REQUIRE( system.getImGuizmoOperation() == 7 );

		// Switch to rotate
		system.setOperation( editor::GizmoOperation::Rotate );
		REQUIRE( system.getImGuizmoOperation() == 120 );

		// Switch to scale
		system.setOperation( editor::GizmoOperation::Scale );
		REQUIRE( system.getImGuizmoOperation() == 896 );

		// Switch to universal
		system.setOperation( editor::GizmoOperation::Universal );
		REQUIRE( system.getImGuizmoOperation() == 1023 );
	}
}

TEST_CASE( "GizmoSystem ImGuizmo snap configuration", "[gizmos][unit][imguizmo][snap]" )
{
	SECTION( "GizmoSystem respects snap enabled/disabled state" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem system( selectionManager, scene );

		// Initially disabled
		REQUIRE_FALSE( system.isSnapEnabled() );

		// Enable snap
		system.setSnapEnabled( true );
		REQUIRE( system.isSnapEnabled() );

		// Disable snap
		system.setSnapEnabled( false );
		REQUIRE_FALSE( system.isSnapEnabled() );
	}

	SECTION( "GizmoSystem uses correct snap values for different operations" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem system( selectionManager, scene );

		// Set custom snap values
		system.setTranslationSnap( 2.5f );
		system.setRotationSnap( 45.0f );
		system.setScaleSnap( 0.25f );

		// Verify values are stored correctly
		REQUIRE( system.getTranslationSnap() == 2.5f );
		REQUIRE( system.getRotationSnap() == 45.0f );
		REQUIRE( system.getScaleSnap() == 0.25f );
	}
}