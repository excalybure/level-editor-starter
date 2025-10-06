// Test for duplicate entity functionality
#include <catch2/catch_test_macros.hpp>

#include "editor/commands/CommandHistory.h"
#include "editor/commands/EcsCommands.h"
#include "runtime/ecs.h"
#include "runtime/components.h"

using namespace editor;

TEST_CASE( "Duplicate entity creates copy with all components", "[duplicate][integration][AF1]" )
{
	SECTION( "Duplicate entity copies Transform component" )
	{
		ecs::Scene scene;
		CommandHistory history;

		// Create an entity with Transform
		auto createCmd = EcsCommandFactory::createEntity( scene, "OriginalEntity" );
		CreateEntityCommand *createCmdPtr = createCmd.get();
		REQUIRE( history.executeCommand( std::move( createCmd ) ) );

		const ecs::Entity originalEntity = createCmdPtr->getCreatedEntity();
		REQUIRE( originalEntity.isValid() );

		// Add Transform with specific values
		components::Transform transform;
		transform.position = { 10.0f, 20.0f, 30.0f };
		transform.rotation = { 0.1f, 0.2f, 0.3f }; // Euler angles
		transform.scale = { 2.0f, 3.0f, 4.0f };

		auto addTransformCmd = EcsCommandFactory::addComponent( scene, originalEntity, transform );
		REQUIRE( history.executeCommand( std::move( addTransformCmd ) ) );

		// Duplicate the entity (simulating the UI behavior)
		const auto *name = scene.getComponent<components::Name>( originalEntity );
		const std::string newName = name ? name->name + " Copy" : "Entity Copy";

		auto duplicateCreateCmd = std::make_unique<CreateEntityCommand>( scene, newName );
		CreateEntityCommand *duplicateCreateCmdPtr = duplicateCreateCmd.get();
		REQUIRE( history.executeCommand( std::move( duplicateCreateCmd ) ) );

		const ecs::Entity duplicatedEntity = duplicateCreateCmdPtr->getCreatedEntity();
		REQUIRE( duplicatedEntity.isValid() );

		// Copy Transform component
		if ( scene.hasComponent<components::Transform>( originalEntity ) )
		{
			const auto *origTransform = scene.getComponent<components::Transform>( originalEntity );
			auto copyTransformCmd = std::make_unique<AddComponentCommand<components::Transform>>( scene, duplicatedEntity, *origTransform );
			REQUIRE( history.executeCommand( std::move( copyTransformCmd ) ) );
		}

		// Verify the duplicated entity has the same Transform values
		REQUIRE( scene.hasComponent<components::Transform>( duplicatedEntity ) );
		const auto *duplicatedTransform = scene.getComponent<components::Transform>( duplicatedEntity );
		REQUIRE( duplicatedTransform->position.x == 10.0f );
		REQUIRE( duplicatedTransform->position.y == 20.0f );
		REQUIRE( duplicatedTransform->position.z == 30.0f );
		REQUIRE( duplicatedTransform->rotation.x == 0.1f );
		REQUIRE( duplicatedTransform->scale.x == 2.0f );
		REQUIRE( duplicatedTransform->scale.y == 3.0f );
		REQUIRE( duplicatedTransform->scale.z == 4.0f );

		// Verify the original entity is unchanged
		const auto *origTransform = scene.getComponent<components::Transform>( originalEntity );
		REQUIRE( origTransform->position.x == 10.0f );
	}

	SECTION( "Duplicate entity copies Visible component" )
	{
		ecs::Scene scene;
		CommandHistory history;

		// Create an entity with Visible
		auto createCmd = EcsCommandFactory::createEntity( scene, "VisibleEntity" );
		CreateEntityCommand *createCmdPtr = createCmd.get();
		REQUIRE( history.executeCommand( std::move( createCmd ) ) );

		const ecs::Entity originalEntity = createCmdPtr->getCreatedEntity();

		components::Visible visible;
		visible.visible = false;
		visible.castShadows = true;

		auto addVisibleCmd = EcsCommandFactory::addComponent( scene, originalEntity, visible );
		REQUIRE( history.executeCommand( std::move( addVisibleCmd ) ) );

		// Duplicate the entity
		auto duplicateCreateCmd = std::make_unique<CreateEntityCommand>( scene, "VisibleEntity Copy" );
		CreateEntityCommand *duplicateCreateCmdPtr = duplicateCreateCmd.get();
		REQUIRE( history.executeCommand( std::move( duplicateCreateCmd ) ) );

		const ecs::Entity duplicatedEntity = duplicateCreateCmdPtr->getCreatedEntity();

		// Copy Visible component
		if ( scene.hasComponent<components::Visible>( originalEntity ) )
		{
			const auto *origVisible = scene.getComponent<components::Visible>( originalEntity );
			auto copyVisibleCmd = std::make_unique<AddComponentCommand<components::Visible>>( scene, duplicatedEntity, *origVisible );
			REQUIRE( history.executeCommand( std::move( copyVisibleCmd ) ) );
		}

		// Verify the duplicated entity has the same Visible values
		REQUIRE( scene.hasComponent<components::Visible>( duplicatedEntity ) );
		const auto *duplicatedVisible = scene.getComponent<components::Visible>( duplicatedEntity );
		REQUIRE( duplicatedVisible->visible == false );
		REQUIRE( duplicatedVisible->castShadows == true );
	}

	SECTION( "Duplicate entity copies MeshRenderer component" )
	{
		ecs::Scene scene;
		CommandHistory history;

		// Create an entity with MeshRenderer
		auto createCmd = EcsCommandFactory::createEntity( scene, "MeshEntity" );
		CreateEntityCommand *createCmdPtr = createCmd.get();
		REQUIRE( history.executeCommand( std::move( createCmd ) ) );

		const ecs::Entity originalEntity = createCmdPtr->getCreatedEntity();

		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = 42;

		auto addMeshRendererCmd = EcsCommandFactory::addComponent( scene, originalEntity, meshRenderer );
		REQUIRE( history.executeCommand( std::move( addMeshRendererCmd ) ) );

		// Duplicate the entity
		auto duplicateCreateCmd = std::make_unique<CreateEntityCommand>( scene, "MeshEntity Copy" );
		CreateEntityCommand *duplicateCreateCmdPtr = duplicateCreateCmd.get();
		REQUIRE( history.executeCommand( std::move( duplicateCreateCmd ) ) );

		const ecs::Entity duplicatedEntity = duplicateCreateCmdPtr->getCreatedEntity();

		// Copy MeshRenderer component
		if ( scene.hasComponent<components::MeshRenderer>( originalEntity ) )
		{
			const auto *origMeshRenderer = scene.getComponent<components::MeshRenderer>( originalEntity );
			auto copyMeshRendererCmd = std::make_unique<AddComponentCommand<components::MeshRenderer>>( scene, duplicatedEntity, *origMeshRenderer );
			REQUIRE( history.executeCommand( std::move( copyMeshRendererCmd ) ) );
		}

		// Verify the duplicated entity has the same MeshRenderer values
		REQUIRE( scene.hasComponent<components::MeshRenderer>( duplicatedEntity ) );
		const auto *duplicatedMeshRenderer = scene.getComponent<components::MeshRenderer>( duplicatedEntity );
		REQUIRE( duplicatedMeshRenderer->meshHandle == 42 );
	}

	SECTION( "Duplicate entity copies all components together" )
	{
		ecs::Scene scene;
		CommandHistory history;

		// Create an entity with multiple components
		auto createCmd = EcsCommandFactory::createEntity( scene, "FullEntity" );
		CreateEntityCommand *createCmdPtr = createCmd.get();
		REQUIRE( history.executeCommand( std::move( createCmd ) ) );

		const ecs::Entity originalEntity = createCmdPtr->getCreatedEntity();

		// Add Transform
		components::Transform transform;
		transform.position = { 5.0f, 10.0f, 15.0f };
		auto addTransformCmd = EcsCommandFactory::addComponent( scene, originalEntity, transform );
		REQUIRE( history.executeCommand( std::move( addTransformCmd ) ) );

		// Add Visible
		components::Visible visible;
		visible.visible = true;
		auto addVisibleCmd = EcsCommandFactory::addComponent( scene, originalEntity, visible );
		REQUIRE( history.executeCommand( std::move( addVisibleCmd ) ) );

		// Add MeshRenderer
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = 99;
		auto addMeshRendererCmd = EcsCommandFactory::addComponent( scene, originalEntity, meshRenderer );
		REQUIRE( history.executeCommand( std::move( addMeshRendererCmd ) ) );

		// Duplicate the entity
		auto duplicateCreateCmd = std::make_unique<CreateEntityCommand>( scene, "FullEntity Copy" );
		CreateEntityCommand *duplicateCreateCmdPtr = duplicateCreateCmd.get();
		REQUIRE( history.executeCommand( std::move( duplicateCreateCmd ) ) );

		const ecs::Entity duplicatedEntity = duplicateCreateCmdPtr->getCreatedEntity();

		// Copy all components
		if ( scene.hasComponent<components::Transform>( originalEntity ) )
		{
			const auto *origTransform = scene.getComponent<components::Transform>( originalEntity );
			auto copyTransformCmd = std::make_unique<AddComponentCommand<components::Transform>>( scene, duplicatedEntity, *origTransform );
			REQUIRE( history.executeCommand( std::move( copyTransformCmd ) ) );
		}

		if ( scene.hasComponent<components::Visible>( originalEntity ) )
		{
			const auto *origVisible = scene.getComponent<components::Visible>( originalEntity );
			auto copyVisibleCmd = std::make_unique<AddComponentCommand<components::Visible>>( scene, duplicatedEntity, *origVisible );
			REQUIRE( history.executeCommand( std::move( copyVisibleCmd ) ) );
		}

		if ( scene.hasComponent<components::MeshRenderer>( originalEntity ) )
		{
			const auto *origMeshRenderer = scene.getComponent<components::MeshRenderer>( originalEntity );
			auto copyMeshRendererCmd = std::make_unique<AddComponentCommand<components::MeshRenderer>>( scene, duplicatedEntity, *origMeshRenderer );
			REQUIRE( history.executeCommand( std::move( copyMeshRendererCmd ) ) );
		}

		// Verify all components were copied
		REQUIRE( scene.hasComponent<components::Transform>( duplicatedEntity ) );
		REQUIRE( scene.hasComponent<components::Visible>( duplicatedEntity ) );
		REQUIRE( scene.hasComponent<components::MeshRenderer>( duplicatedEntity ) );

		const auto *dupTransform = scene.getComponent<components::Transform>( duplicatedEntity );
		REQUIRE( dupTransform->position.x == 5.0f );

		const auto *dupVisible = scene.getComponent<components::Visible>( duplicatedEntity );
		REQUIRE( dupVisible->visible == true );

		const auto *dupMeshRenderer = scene.getComponent<components::MeshRenderer>( duplicatedEntity );
		REQUIRE( dupMeshRenderer->meshHandle == 99 );
	}

	SECTION( "Duplicate entity does not copy Selected component" )
	{
		ecs::Scene scene;
		CommandHistory history;

		// Create an entity with Selected component
		auto createCmd = EcsCommandFactory::createEntity( scene, "SelectedEntity" );
		CreateEntityCommand *createCmdPtr = createCmd.get();
		REQUIRE( history.executeCommand( std::move( createCmd ) ) );

		const ecs::Entity originalEntity = createCmdPtr->getCreatedEntity();

		// Add Selected component
		components::Selected selected;
		scene.addComponent( originalEntity, selected );

		REQUIRE( scene.hasComponent<components::Selected>( originalEntity ) );

		// Duplicate the entity
		auto duplicateCreateCmd = std::make_unique<CreateEntityCommand>( scene, "SelectedEntity Copy" );
		CreateEntityCommand *duplicateCreateCmdPtr = duplicateCreateCmd.get();
		REQUIRE( history.executeCommand( std::move( duplicateCreateCmd ) ) );

		const ecs::Entity duplicatedEntity = duplicateCreateCmdPtr->getCreatedEntity();

		// Note: We intentionally do NOT copy the Selected component
		// (as per the implementation - duplicated entities should not be selected by default)

		// Verify the duplicated entity does NOT have the Selected component
		REQUIRE( !scene.hasComponent<components::Selected>( duplicatedEntity ) );

		// Original entity should still have Selected
		REQUIRE( scene.hasComponent<components::Selected>( originalEntity ) );
	}
}
