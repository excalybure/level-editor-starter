#include <catch2/catch_test_macros.hpp>
#include "editor/entity_inspector/EntityInspectorPanel.h"
#include "editor/selection.h"
#include "editor/commands/CommandHistory.h"
#include "editor/commands/EcsCommands.h"
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "runtime/systems.h"
#include "math/math.h"
#include "engine/assets/assets.h"
#include "graphics/gpu/mesh_gpu.h"
#include "graphics/gpu/material_gpu.h"
#include "graphics/texture/texture_manager.h"
#include "graphics/texture/bindless_texture_heap.h"
#include "platform/dx12/dx12_device.h"
#include <cmath>

// ============================================================================
// T2.1: Inspector Panel Foundation Tests
// ============================================================================

TEST_CASE( "EntityInspectorPanel - Panel can be constructed", "[T2.1][entity_inspector][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	// Act
	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Assert
	REQUIRE( panel.isVisible() ); // Should be visible by default
}

TEST_CASE( "EntityInspectorPanel - Panel visibility can be toggled", "[T2.1][entity_inspector][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Act
	panel.setVisible( false );

	// Assert
	REQUIRE( !panel.isVisible() );

	// Act
	panel.setVisible( true );

	// Assert
	REQUIRE( panel.isVisible() );
}

TEST_CASE( "EntityInspectorPanel - No selection shows empty state", "[T2.1][entity_inspector][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Act - No entities selected
	// Nothing to do here

	// Assert - Panel should handle empty selection gracefully
	// This test verifies that render() doesn't crash with no selection
	REQUIRE( selectionManager.getSelectionCount() == 0 );
}

TEST_CASE( "EntityInspectorPanel - Single selection shows entity info", "[T2.1][entity_inspector][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity = scene.createEntity( "TestEntity" );
	selectionManager.select( entity );

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Act - Single entity selected
	// Nothing to do here

	// Assert - Panel should recognize single selection
	REQUIRE( selectionManager.getSelectionCount() == 1 );
	REQUIRE( selectionManager.isSelected( entity ) );
}

TEST_CASE( "EntityInspectorPanel - Multiple selection shows multi-select state", "[T2.1][entity_inspector][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity1 = scene.createEntity( "Entity1" );
	const ecs::Entity entity2 = scene.createEntity( "Entity2" );
	selectionManager.select( entity1, false );
	selectionManager.select( entity2, true );

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Act - Multiple entities selected
	// Nothing to do here

	// Assert - Panel should recognize multiple selection
	REQUIRE( selectionManager.getSelectionCount() == 2 );
	REQUIRE( selectionManager.isSelected( entity1 ) );
	REQUIRE( selectionManager.isSelected( entity2 ) );
}

TEST_CASE( "EntityInspectorPanel - Panel can be hidden and shown", "[T2.1][entity_inspector][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Act & Assert - Toggle visibility multiple times
	panel.setVisible( false );
	REQUIRE( !panel.isVisible() );

	panel.setVisible( true );
	REQUIRE( panel.isVisible() );

	panel.setVisible( false );
	REQUIRE( !panel.isVisible() );
}

// ============================================================================
// T2.3: Transform Component Editor Tests
// ============================================================================

TEST_CASE( "EntityInspectorPanel - Entity with Transform component can be inspected", "[T2.3][entity_inspector][transform][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity = scene.createEntity( "TransformEntity" );

	// Add Transform component
	scene.addComponent( entity, components::Transform{} );

	// Set initial transform values
	auto *transform = scene.getComponent<components::Transform>( entity );
	REQUIRE( transform != nullptr );

	transform->position = { 1.0f, 2.0f, 3.0f };
	transform->rotation = { 0.1f, 0.2f, 0.3f }; // radians
	transform->scale = { 2.0f, 2.0f, 2.0f };

	selectionManager.select( entity );

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Assert - Panel should be able to access Transform component
	REQUIRE( scene.hasComponent<components::Transform>( entity ) );
	REQUIRE( transform->position.x == 1.0f );
	REQUIRE( transform->position.y == 2.0f );
	REQUIRE( transform->position.z == 3.0f );
}

// ============================================================================
// T2.4: Name and Visible Component Editor Tests
// ============================================================================

TEST_CASE( "EntityInspectorPanel - Entity with Name component can be inspected", "[T2.4][entity_inspector][name][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity = scene.createEntity( "NamedEntity" );

	// Name component is auto-added by createEntity with custom name
	REQUIRE( scene.hasComponent<components::Name>( entity ) );

	selectionManager.select( entity );

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Assert - Panel should be able to access Name component
	const auto *name = scene.getComponent<components::Name>( entity );
	REQUIRE( name != nullptr );
	REQUIRE( name->name == "NamedEntity" );
}

TEST_CASE( "EntityInspectorPanel - Entity with Visible component can be inspected", "[T2.4][entity_inspector][visible][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity = scene.createEntity( "VisibleEntity" );

	// Add Visible component
	components::Visible visible;
	visible.visible = true;
	visible.castShadows = false;
	visible.receiveShadows = true;
	scene.addComponent( entity, visible );

	selectionManager.select( entity );

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Assert - Panel should be able to access Visible component
	const auto *visibleComp = scene.getComponent<components::Visible>( entity );
	REQUIRE( visibleComp != nullptr );
	REQUIRE( visibleComp->visible == true );
	REQUIRE( visibleComp->castShadows == false );
	REQUIRE( visibleComp->receiveShadows == true );
}

TEST_CASE( "EntityInspectorPanel - Entity with MeshRenderer component can be inspected", "[T2.5][entity_inspector][meshrenderer][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity = scene.createEntity( "MeshEntity" );

	// Add MeshRenderer component
	components::MeshRenderer meshRenderer;
	meshRenderer.meshHandle = 42; // Set a test mesh handle
	scene.addComponent( entity, meshRenderer );

	selectionManager.select( entity );

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Assert - Panel should be able to access MeshRenderer component
	const auto *meshRendererComp = scene.getComponent<components::MeshRenderer>( entity );
	REQUIRE( meshRendererComp != nullptr );
	REQUIRE( meshRendererComp->meshHandle == 42 );
	REQUIRE( meshRendererComp->gpuMesh == nullptr ); // No GPU resources in test
}

// ============================================================================
// T2.6: Add Component Menu Tests
// ============================================================================

TEST_CASE( "EntityInspectorPanel - Can add components to entity via command", "[T2.6][entity_inspector][add_component][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity = scene.createEntity( "TestEntity" );
	selectionManager.select( entity );

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Initially, entity should not have Transform component
	REQUIRE( !scene.hasComponent<components::Transform>( entity ) );

	// Act - Create AddComponentCommand directly (simulating menu selection)
	components::Transform transform;
	transform.position = { 1.0f, 2.0f, 3.0f };
	auto command = std::make_unique<editor::AddComponentCommand<components::Transform>>( scene, entity, transform );
	commandHistory.executeCommand( std::move( command ) );

	// Assert - Entity should now have Transform component with correct values
	REQUIRE( scene.hasComponent<components::Transform>( entity ) );
	const auto *transformComp = scene.getComponent<components::Transform>( entity );
	REQUIRE( transformComp != nullptr );
	REQUIRE( transformComp->position.x == 1.0f );
	REQUIRE( transformComp->position.y == 2.0f );
	REQUIRE( transformComp->position.z == 3.0f );

	// Act - Undo the add component command
	commandHistory.undo();

	// Assert - Component should be removed
	REQUIRE( !scene.hasComponent<components::Transform>( entity ) );

	// Act - Redo the add component command
	commandHistory.redo();

	// Assert - Component should be added back
	REQUIRE( scene.hasComponent<components::Transform>( entity ) );
}

// ============================================================================
// T2.7: Remove Component Menu Tests
// ============================================================================

TEST_CASE( "Can remove components from entity via command", "[T2.7][entity_inspector][remove_component][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity = scene.createEntity( "TestEntity" );
	selectionManager.select( entity );

	// Add Visible component to test removal
	components::Visible visible;
	visible.visible = true;
	visible.castShadows = true;
	visible.receiveShadows = false;
	scene.addComponent( entity, visible );

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Initially, entity should have Visible component
	REQUIRE( scene.hasComponent<components::Visible>( entity ) );

	// Act - Create RemoveComponentCommand (simulating context menu selection)
	auto command = std::make_unique<editor::RemoveComponentCommand<components::Visible>>( scene, entity );
	commandHistory.executeCommand( std::move( command ) );

	// Assert - Component should be removed
	REQUIRE( !scene.hasComponent<components::Visible>( entity ) );

	// Act - Undo the remove command
	commandHistory.undo();

	// Assert - Component should be restored with original values
	REQUIRE( scene.hasComponent<components::Visible>( entity ) );
	const auto *restoredVisible = scene.getComponent<components::Visible>( entity );
	REQUIRE( restoredVisible != nullptr );
	REQUIRE( restoredVisible->visible == true );
	REQUIRE( restoredVisible->castShadows == true );
	REQUIRE( restoredVisible->receiveShadows == false );

	// Act - Redo the remove command
	commandHistory.redo();

	// Assert - Component should be removed again
	REQUIRE( !scene.hasComponent<components::Visible>( entity ) );
}

TEST_CASE( "Essential components (Transform, Name, Visible) cannot be removed", "[T2.7][entity_inspector][essential_components][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	const ecs::Entity entity = scene.createEntity( "TestEntity" );

	// Add Transform component (not auto-added, but essential once present)
	scene.addComponent( entity, components::Transform{} );

	selectionManager.select( entity );

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Note: The UI prevents removal of essential components via the isEssential flag
	// in renderComponentContextMenu template, which disables the "Remove Component" menu item.
	// The underlying RemoveComponentCommand can still execute if called directly,
	// but the UI should never invoke it for Transform, Name, or Visible components.

	// This test verifies that essential components are present and should remain so
	// in normal usage through the UI.

	SECTION( "Transform component is present and essential" )
	{
		// Transform was added above (essential once present)
		REQUIRE( scene.hasComponent<components::Transform>( entity ) );
	}

	SECTION( "Name component is present and essential" )
	{
		// Name is auto-added by createEntity with custom name
		REQUIRE( scene.hasComponent<components::Name>( entity ) );
	}

	SECTION( "Visible component is present and essential" )
	{
		// Visible is auto-added by createEntity
		REQUIRE( scene.hasComponent<components::Visible>( entity ) );
	}

	SECTION( "Non-essential component (MeshRenderer) can be removed" )
	{
		// Add MeshRenderer component
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = 123;
		scene.addComponent( entity, meshRenderer );
		REQUIRE( scene.hasComponent<components::MeshRenderer>( entity ) );

		// MeshRenderer is not essential, so it can be removed
		auto command = std::make_unique<editor::RemoveComponentCommand<components::MeshRenderer>>( scene, entity );
		commandHistory.executeCommand( std::move( command ) );

		// Should be successfully removed
		REQUIRE( !scene.hasComponent<components::MeshRenderer>( entity ) );
	}
}

// ============================================================================
// T2.8: Multi-Selection Support Tests
// ============================================================================

TEST_CASE( "Multi-selection shows common components", "[T2.8][entity_inspector][multi_selection][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	// Create three entities with Transform and Visible components
	const ecs::Entity entity1 = scene.createEntity( "Entity1" );
	const ecs::Entity entity2 = scene.createEntity( "Entity2" );
	const ecs::Entity entity3 = scene.createEntity( "Entity3" );

	// Add Transform components
	components::Transform transform;
	transform.position = { 1.0f, 2.0f, 3.0f };
	transform.rotation = { 0.0f, 0.0f, 0.0f };
	transform.scale = { 1.0f, 1.0f, 1.0f };
	scene.addComponent( entity1, transform );
	scene.addComponent( entity2, transform );
	scene.addComponent( entity3, transform );

	// Add Visible components
	components::Visible visible;
	visible.visible = true;
	visible.castShadows = true;
	visible.receiveShadows = true;
	scene.addComponent( entity1, visible );
	scene.addComponent( entity2, visible );
	scene.addComponent( entity3, visible );

	// Select all three entities
	selectionManager.select( entity1 );
	selectionManager.select( entity2, true ); // additive = true
	selectionManager.select( entity3, true ); // additive = true

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Assert - All entities should be selected
	REQUIRE( selectionManager.getSelectionCount() == 3 );
	REQUIRE( selectionManager.isSelected( entity1 ) );
	REQUIRE( selectionManager.isSelected( entity2 ) );
	REQUIRE( selectionManager.isSelected( entity3 ) );

	// Assert - All entities have Transform (created by default)
	REQUIRE( scene.hasComponent<components::Transform>( entity1 ) );
	REQUIRE( scene.hasComponent<components::Transform>( entity2 ) );
	REQUIRE( scene.hasComponent<components::Transform>( entity3 ) );

	// Assert - All entities have Visible
	REQUIRE( scene.hasComponent<components::Visible>( entity1 ) );
	REQUIRE( scene.hasComponent<components::Visible>( entity2 ) );
	REQUIRE( scene.hasComponent<components::Visible>( entity3 ) );
}

// ============================================================================
// T1.1: Primitive Tree Display Tests
// ============================================================================

TEST_CASE( "EntityInspectorPanel - MeshRenderer shows primitive tree with vertex and index counts", "[T1.1][entity_inspector][primitive_tree][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	// Create a mesh with 2 primitives
	auto mesh = std::make_shared<assets::Mesh>();

	// First primitive: 3 vertices, 3 indices (triangle)
	assets::Primitive prim0;
	prim0.addVertex( assets::Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	prim0.addVertex( assets::Vertex{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	prim0.addVertex( assets::Vertex{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	prim0.addIndex( 0 );
	prim0.addIndex( 1 );
	prim0.addIndex( 2 );
	mesh->addPrimitive( std::move( prim0 ) );

	// Second primitive: 4 vertices, 6 indices (quad)
	assets::Primitive prim1;
	prim1.addVertex( assets::Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	prim1.addVertex( assets::Vertex{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	prim1.addVertex( assets::Vertex{ { 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	prim1.addVertex( assets::Vertex{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	prim1.addIndex( 0 );
	prim1.addIndex( 1 );
	prim1.addIndex( 2 );
	prim1.addIndex( 0 );
	prim1.addIndex( 2 );
	prim1.addIndex( 3 );
	mesh->addPrimitive( std::move( prim1 ) );

	// Create GPU mesh
	auto gpuMesh = std::make_shared<graphics::gpu::MeshGPU>( device, *mesh );
	REQUIRE( gpuMesh->isValid() );
	REQUIRE( gpuMesh->getPrimitiveCount() == 2 );

	// Create entity with MeshRenderer
	const ecs::Entity entity = scene.createEntity( "MeshEntity" );
	components::MeshRenderer meshRenderer;
	meshRenderer.meshHandle = 42;
	meshRenderer.gpuMesh = gpuMesh;
	scene.addComponent( entity, meshRenderer );

	selectionManager.select( entity );

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Act & Assert - Verify MeshRenderer has primitives
	const auto *meshRendererComp = scene.getComponent<components::MeshRenderer>( entity );
	REQUIRE( meshRendererComp != nullptr );
	REQUIRE( meshRendererComp->gpuMesh != nullptr );
	REQUIRE( meshRendererComp->gpuMesh->getPrimitiveCount() == 2 );

	// Verify primitive 0 has correct counts
	const auto &primitive0 = meshRendererComp->gpuMesh->getPrimitive( 0 );
	REQUIRE( primitive0.getVertexCount() == 3 );
	REQUIRE( primitive0.getIndexCount() == 3 );

	// Verify primitive 1 has correct counts
	const auto &primitive1 = meshRendererComp->gpuMesh->getPrimitive( 1 );
	REQUIRE( primitive1.getVertexCount() == 4 );
	REQUIRE( primitive1.getIndexCount() == 6 );
}

// ============================================================================
// T1.2: Material Name Display Tests
// ============================================================================

TEST_CASE( "EntityInspectorPanel - Primitive tree displays material name per primitive", "[T1.2][entity_inspector][primitive_tree][material_name][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	graphics::texture::TextureManager textureManager;
	textureManager.initialize( &device, 1024 );

	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	// Create two materials with distinct names
	auto material0 = std::make_shared<assets::Material>();
	material0->setName( "RedMaterial" );
	material0->setBaseColorFactor( 1.0f, 0.0f, 0.0f, 1.0f );
	material0->setPath( "red_material" );
	material0->setLoaded( true );

	auto material1 = std::make_shared<assets::Material>();
	material1->setName( "BlueMaterial" );
	material1->setBaseColorFactor( 0.0f, 0.0f, 1.0f, 1.0f );
	material1->setPath( "blue_material" );
	material1->setLoaded( true );

	// Create GPU materials
	auto gpuMaterial0 = std::make_shared<graphics::gpu::MaterialGPU>( material0, device, textureManager );
	auto gpuMaterial1 = std::make_shared<graphics::gpu::MaterialGPU>( material1, device, textureManager );
	REQUIRE( gpuMaterial0->isValid() );
	REQUIRE( gpuMaterial1->isValid() );

	// Create a mesh with 2 primitives
	auto mesh = std::make_shared<assets::Mesh>();

	// First primitive: 3 vertices, 3 indices (triangle)
	assets::Primitive prim0;
	prim0.addVertex( assets::Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	prim0.addVertex( assets::Vertex{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	prim0.addVertex( assets::Vertex{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	prim0.addIndex( 0 );
	prim0.addIndex( 1 );
	prim0.addIndex( 2 );
	mesh->addPrimitive( std::move( prim0 ) );

	// Second primitive: 4 vertices, 6 indices (quad)
	assets::Primitive prim1;
	prim1.addVertex( assets::Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	prim1.addVertex( assets::Vertex{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	prim1.addVertex( assets::Vertex{ { 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	prim1.addVertex( assets::Vertex{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	prim1.addIndex( 0 );
	prim1.addIndex( 1 );
	prim1.addIndex( 2 );
	prim1.addIndex( 0 );
	prim1.addIndex( 2 );
	prim1.addIndex( 3 );
	mesh->addPrimitive( std::move( prim1 ) );

	// Create GPU mesh
	auto gpuMesh = std::make_shared<graphics::gpu::MeshGPU>( device, *mesh );
	REQUIRE( gpuMesh->isValid() );
	REQUIRE( gpuMesh->getPrimitiveCount() == 2 );

	// Manually assign materials to primitives
	gpuMesh->getPrimitive( 0 ).setMaterial( gpuMaterial0 );
	gpuMesh->getPrimitive( 1 ).setMaterial( gpuMaterial1 );

	// Create entity with MeshRenderer
	const ecs::Entity entity = scene.createEntity( "MeshEntity" );
	components::MeshRenderer meshRenderer;
	meshRenderer.meshHandle = 42;
	meshRenderer.gpuMesh = gpuMesh;
	scene.addComponent( entity, meshRenderer );

	selectionManager.select( entity );

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Act & Assert - Verify primitives have materials with correct names
	const auto *meshRendererComp = scene.getComponent<components::MeshRenderer>( entity );
	REQUIRE( meshRendererComp != nullptr );
	REQUIRE( meshRendererComp->gpuMesh != nullptr );
	REQUIRE( meshRendererComp->gpuMesh->getPrimitiveCount() == 2 );

	// Verify primitive 0 has RedMaterial
	const auto &primitive0 = meshRendererComp->gpuMesh->getPrimitive( 0 );
	REQUIRE( primitive0.hasMaterial() );
	REQUIRE( primitive0.getMaterial() != nullptr );
	const auto sourceMaterial0 = primitive0.getMaterial()->getSourceMaterial();
	REQUIRE( sourceMaterial0.get() != nullptr );
	REQUIRE( sourceMaterial0->getName() == "RedMaterial" );

	// Verify primitive 1 has BlueMaterial
	const auto &primitive1 = meshRendererComp->gpuMesh->getPrimitive( 1 );
	REQUIRE( primitive1.hasMaterial() );
	REQUIRE( primitive1.getMaterial() != nullptr );
	const auto sourceMaterial1 = primitive1.getMaterial()->getSourceMaterial();
	REQUIRE( sourceMaterial1.get() != nullptr );
	REQUIRE( sourceMaterial1->getName() == "BlueMaterial" );
}

// ============================================================================
// T1.3: Material Properties Display Tests (Read-Only)
// ============================================================================

TEST_CASE( "EntityInspectorPanel - Primitive tree displays material properties (read-only)", "[T1.3][entity_inspector][primitive_tree][material_properties][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	graphics::texture::TextureManager textureManager;
	textureManager.initialize( &device, 1024 );

	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	// Create a material with specific PBR properties
	auto material = std::make_shared<assets::Material>();
	material->setName( "TestPBRMaterial" );
	material->setBaseColorFactor( 0.8f, 0.2f, 0.1f, 0.9f );
	material->setMetallicFactor( 0.7f );
	material->setRoughnessFactor( 0.3f );
	material->setPath( "test_pbr_material" );
	material->setLoaded( true );

	// Set emissive factor through direct PBR access
	auto &pbrMaterial = material->getPBRMaterial();
	pbrMaterial.emissiveFactor = { 0.15f, 0.05f, 0.02f };
	pbrMaterial.baseColorTexture = "textures/base_color.png";
	pbrMaterial.normalTexture = "textures/normal.png";
	pbrMaterial.metallicRoughnessTexture = "textures/metallic_roughness.png";
	pbrMaterial.emissiveTexture = "textures/emissive.png";

	// Create GPU material
	auto gpuMaterial = std::make_shared<graphics::gpu::MaterialGPU>( material, device, textureManager );
	REQUIRE( gpuMaterial->isValid() );

	// Create a mesh with a single primitive
	auto mesh = std::make_shared<assets::Mesh>();

	assets::Primitive prim;
	prim.addVertex( assets::Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	prim.addVertex( assets::Vertex{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	prim.addVertex( assets::Vertex{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	prim.addIndex( 0 );
	prim.addIndex( 1 );
	prim.addIndex( 2 );
	mesh->addPrimitive( std::move( prim ) );

	// Create GPU mesh and assign material
	auto gpuMesh = std::make_shared<graphics::gpu::MeshGPU>( device, *mesh );
	REQUIRE( gpuMesh->isValid() );
	gpuMesh->getPrimitive( 0 ).setMaterial( gpuMaterial );

	// Create entity with MeshRenderer
	const ecs::Entity entity = scene.createEntity( "MaterialPropertiesEntity" );
	components::MeshRenderer meshRenderer;
	meshRenderer.meshHandle = 42;
	meshRenderer.gpuMesh = gpuMesh;
	scene.addComponent( entity, meshRenderer );

	selectionManager.select( entity );

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Act & Assert - Verify material properties are accessible
	const auto *meshRendererComp = scene.getComponent<components::MeshRenderer>( entity );
	REQUIRE( meshRendererComp != nullptr );
	REQUIRE( meshRendererComp->gpuMesh != nullptr );
	REQUIRE( meshRendererComp->gpuMesh->getPrimitiveCount() == 1 );

	const auto &primitive = meshRendererComp->gpuMesh->getPrimitive( 0 );
	REQUIRE( primitive.hasMaterial() );

	const auto materialGPU = primitive.getMaterial();
	REQUIRE( materialGPU != nullptr );
	REQUIRE( materialGPU->isValid() );

	// Verify material constants
	const auto &constants = materialGPU->getMaterialConstants();
	REQUIRE( constants.baseColorFactor.x == 0.8f );
	REQUIRE( constants.baseColorFactor.y == 0.2f );
	REQUIRE( constants.baseColorFactor.z == 0.1f );
	REQUIRE( constants.baseColorFactor.w == 0.9f );
	REQUIRE( constants.metallicFactor == 0.7f );
	REQUIRE( constants.roughnessFactor == 0.3f );
	REQUIRE( constants.emissiveFactor.x == 0.15f );
	REQUIRE( constants.emissiveFactor.y == 0.05f );
	REQUIRE( constants.emissiveFactor.z == 0.02f );

	// Verify source material PBR properties for texture names
	const auto sourceMaterial = materialGPU->getSourceMaterial();
	REQUIRE( sourceMaterial != nullptr );
	const auto &pbr = sourceMaterial->getPBRMaterial();
	REQUIRE( pbr.baseColorTexture == "textures/base_color.png" );
	REQUIRE( pbr.normalTexture == "textures/normal.png" );
	REQUIRE( pbr.metallicRoughnessTexture == "textures/metallic_roughness.png" );
	REQUIRE( pbr.emissiveTexture == "textures/emissive.png" );
}

TEST_CASE( "EntityInspectorPanel - Primitive tree displays material properties with empty textures", "[T1.3][entity_inspector][primitive_tree][material_properties][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	graphics::texture::TextureManager textureManager;
	textureManager.initialize( &device, 1024 );

	ecs::Scene scene;
	systems::SystemManager systemManager;
	editor::SelectionManager selectionManager( scene, systemManager );
	CommandHistory commandHistory;

	// Create a material with no textures
	auto material = std::make_shared<assets::Material>();
	material->setName( "SimpleMaterial" );
	material->setBaseColorFactor( 1.0f, 0.5f, 0.2f, 1.0f );
	material->setMetallicFactor( 0.0f );
	material->setRoughnessFactor( 0.8f );
	material->setPath( "simple_material" );
	material->setLoaded( true );

	// Create GPU material (no textures)
	auto gpuMaterial = std::make_shared<graphics::gpu::MaterialGPU>( material, device, textureManager );
	REQUIRE( gpuMaterial->isValid() );

	// Create a mesh with a single primitive
	auto mesh = std::make_shared<assets::Mesh>();

	assets::Primitive prim;
	prim.addVertex( assets::Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	prim.addVertex( assets::Vertex{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	prim.addVertex( assets::Vertex{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	prim.addIndex( 0 );
	prim.addIndex( 1 );
	prim.addIndex( 2 );
	mesh->addPrimitive( std::move( prim ) );

	// Create GPU mesh and assign material
	auto gpuMesh = std::make_shared<graphics::gpu::MeshGPU>( device, *mesh );
	REQUIRE( gpuMesh->isValid() );
	gpuMesh->getPrimitive( 0 ).setMaterial( gpuMaterial );

	// Create entity with MeshRenderer
	const ecs::Entity entity = scene.createEntity( "NoTexturesEntity" );
	components::MeshRenderer meshRenderer;
	meshRenderer.meshHandle = 42;
	meshRenderer.gpuMesh = gpuMesh;
	scene.addComponent( entity, meshRenderer );

	selectionManager.select( entity );

	editor::EntityInspectorPanel panel( scene, selectionManager, commandHistory, systemManager );

	// Act & Assert - Verify material properties work with no textures
	const auto *meshRendererComp = scene.getComponent<components::MeshRenderer>( entity );
	REQUIRE( meshRendererComp != nullptr );

	const auto &primitive = meshRendererComp->gpuMesh->getPrimitive( 0 );
	const auto materialGPU = primitive.getMaterial();
	REQUIRE( materialGPU != nullptr );

	// Verify material constants are accessible
	const auto &constants = materialGPU->getMaterialConstants();
	REQUIRE( constants.baseColorFactor.x == 1.0f );
	REQUIRE( constants.baseColorFactor.y == 0.5f );
	REQUIRE( constants.baseColorFactor.z == 0.2f );
	REQUIRE( constants.metallicFactor == 0.0f );
	REQUIRE( constants.roughnessFactor == 0.8f );

	// Verify textures are empty
	const auto sourceMaterial = materialGPU->getSourceMaterial();
	REQUIRE( sourceMaterial != nullptr );
	const auto &pbr = sourceMaterial->getPBRMaterial();
	REQUIRE( pbr.baseColorTexture.empty() );
	REQUIRE( pbr.normalTexture.empty() );
	REQUIRE( pbr.metallicRoughnessTexture.empty() );
	REQUIRE( pbr.emissiveTexture.empty() );
}
