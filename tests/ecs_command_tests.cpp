#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>

#include "editor/commands/Command.h"
#include "editor/commands/EcsCommands.h"
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "engine/gpu/gpu_resource_manager.h"
#include "platform/dx12/dx12_device.h"

// Mock GPUResourceManager for testing (CPU-only tests don't need actual GPU resources)
class MockGPUResourceManager : public engine::GPUResourceManager
{
public:
	MockGPUResourceManager() : engine::GPUResourceManager( getMockDevice() ) {}

private:
	static dx12::Device &getMockDevice()
	{
		static dx12::Device device;
		return device;
	}
};

TEST_CASE( "CreateEntityCommand basic functionality", "[ecs-commands][unit][AF2.1]" )
{
	SECTION( "CreateEntityCommand can be constructed with scene and name" )
	{
		ecs::Scene scene;
		editor::CreateEntityCommand cmd( scene, "TestEntity" );

		REQUIRE( cmd.getDescription() == "Create Entity: TestEntity" );
		// Entity should not exist until execute is called
		REQUIRE( !cmd.getCreatedEntity().isValid() );
	}

	SECTION( "CreateEntityCommand execute creates entity in scene" )
	{
		ecs::Scene scene;
		editor::CreateEntityCommand cmd( scene, "NewEntity" );

		REQUIRE( cmd.execute() );

		const ecs::Entity entity = cmd.getCreatedEntity();
		REQUIRE( entity.isValid() );
		REQUIRE( scene.isValid( entity ) );

		// Check if entity has Name component when custom name provided
		REQUIRE( scene.hasComponent<components::Name>( entity ) );
		const auto *nameComp = scene.getComponent<components::Name>( entity );
		REQUIRE( nameComp->name == "NewEntity" );
	}

	SECTION( "CreateEntityCommand undo removes entity from scene" )
	{
		ecs::Scene scene;
		editor::CreateEntityCommand cmd( scene, "ToDeleteEntity" );

		cmd.execute();
		const ecs::Entity entity = cmd.getCreatedEntity();
		REQUIRE( scene.isValid( entity ) );

		REQUIRE( cmd.undo() );
		REQUIRE( !scene.isValid( entity ) );
		REQUIRE( !cmd.getCreatedEntity().isValid() );
	}

	SECTION( "CreateEntityCommand cannot execute twice" )
	{
		ecs::Scene scene;
		editor::CreateEntityCommand cmd( scene, "OnceOnly" );

		REQUIRE( cmd.execute() );
		REQUIRE( !cmd.execute() ); // Second execution should fail
	}

	SECTION( "CreateEntityCommand cannot undo before execute" )
	{
		ecs::Scene scene;
		editor::CreateEntityCommand cmd( scene, "NotExecuted" );

		REQUIRE( !cmd.undo() ); // Cannot undo without execute
	}

	SECTION( "CreateEntityCommand provides memory usage" )
	{
		ecs::Scene scene;
		editor::CreateEntityCommand cmd( scene, "MemoryTest" );

		const size_t memUsage = cmd.getMemoryUsage();
		REQUIRE( memUsage > sizeof( editor::CreateEntityCommand ) );
	}

	SECTION( "CreateEntityCommand cannot merge with other commands" )
	{
		ecs::Scene scene;
		editor::CreateEntityCommand cmd1( scene, "Entity1" );
		editor::CreateEntityCommand cmd2( scene, "Entity2" );

		REQUIRE( !cmd1.canMergeWith( &cmd2 ) );
		REQUIRE( !cmd1.mergeWith( std::make_unique<editor::CreateEntityCommand>( scene, "Entity3" ) ) );
	}
}

TEST_CASE( "DeleteEntityCommand with complete component restoration", "[ecs-commands][unit][AF2.2]" )
{
	SECTION( "DeleteEntityCommand can be constructed with scene and entity" )
	{
		ecs::Scene scene;
		const ecs::Entity entity = scene.createEntity( "ToDelete" );

		editor::DeleteEntityCommand cmd( scene, entity );

		REQUIRE( cmd.getDescription() == "Delete Entity: ToDelete" );
	}

	SECTION( "DeleteEntityCommand execute removes entity from scene" )
	{
		ecs::Scene scene;
		const ecs::Entity entity = scene.createEntity( "TestEntity" );
		editor::DeleteEntityCommand cmd( scene, entity );

		REQUIRE( scene.isValid( entity ) );
		REQUIRE( cmd.execute() );
		REQUIRE( !scene.isValid( entity ) );
	}

	SECTION( "DeleteEntityCommand undo recreates entity with same components" )
	{
		ecs::Scene scene;
		const ecs::Entity entity = scene.createEntity( "ComplexEntity" );

		// Add multiple components to the entity
		components::Transform transform;
		transform.position = { 1.0f, 2.0f, 3.0f };
		transform.scale = { 0.5f, 0.5f, 0.5f };
		scene.addComponent( entity, transform );

		components::Visible visible;
		visible.visible = false;
		visible.castShadows = false;
		scene.addComponent( entity, visible );

		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = 42;
		scene.addComponent( entity, meshRenderer );

		components::Selected selected( true ); // Primary selection
		scene.addComponent( entity, selected );

		// Create and execute delete command
		editor::DeleteEntityCommand cmd( scene, entity );
		cmd.execute();
		REQUIRE( !scene.isValid( entity ) );

		// Undo should recreate entity with all components
		REQUIRE( cmd.undo() );

		// Note: entity ID will be different after recreation, but components should be restored
		// We can't directly compare entity IDs, so we check that the scene has entities with the right components
		bool foundEntityWithComponents = false;
		scene.forEach<components::Name>( [&]( ecs::Entity e, const components::Name &name ) {
			if ( name.name == "ComplexEntity" )
			{
				// Verify all components are restored
				REQUIRE( scene.hasComponent<components::Transform>( e ) );
				REQUIRE( scene.hasComponent<components::Visible>( e ) );
				REQUIRE( scene.hasComponent<components::MeshRenderer>( e ) );
				REQUIRE( scene.hasComponent<components::Selected>( e ) );

				// Verify component values
				const auto *restoredTransform = scene.getComponent<components::Transform>( e );
				REQUIRE( restoredTransform->position.x == 1.0f );
				REQUIRE( restoredTransform->position.y == 2.0f );
				REQUIRE( restoredTransform->position.z == 3.0f );
				REQUIRE( restoredTransform->scale.x == 0.5f );

				const auto *restoredVisible = scene.getComponent<components::Visible>( e );
				REQUIRE( !restoredVisible->visible );
				REQUIRE( !restoredVisible->castShadows );

				const auto *restoredMeshRenderer = scene.getComponent<components::MeshRenderer>( e );
				REQUIRE( restoredMeshRenderer->meshHandle == 42 );

				const auto *restoredSelected = scene.getComponent<components::Selected>( e );
				REQUIRE( restoredSelected->isPrimary );

				foundEntityWithComponents = true;
			}
		} );

		REQUIRE( foundEntityWithComponents );
	}

	SECTION( "DeleteEntityCommand cannot execute twice" )
	{
		ecs::Scene scene;
		const ecs::Entity entity = scene.createEntity( "OnceOnly" );
		editor::DeleteEntityCommand cmd( scene, entity );

		REQUIRE( cmd.execute() );
		REQUIRE( !cmd.execute() ); // Second execution should fail
	}

	SECTION( "DeleteEntityCommand cannot undo before execute" )
	{
		ecs::Scene scene;
		const ecs::Entity entity = scene.createEntity( "NotExecuted" );
		editor::DeleteEntityCommand cmd( scene, entity );

		REQUIRE( !cmd.undo() ); // Cannot undo without execute
	}

	SECTION( "DeleteEntityCommand handles invalid entity gracefully" )
	{
		ecs::Scene scene;
		const ecs::Entity invalidEntity{}; // Invalid entity
		editor::DeleteEntityCommand cmd( scene, invalidEntity );

		REQUIRE( !cmd.execute() ); // Should fail gracefully
	}

	SECTION( "DeleteEntityCommand cannot merge with other commands" )
	{
		ecs::Scene scene;
		ecs::Entity entity1 = scene.createEntity( "Entity1" );
		ecs::Entity entity2 = scene.createEntity( "Entity2" );

		editor::DeleteEntityCommand cmd1( scene, entity1 );
		editor::DeleteEntityCommand cmd2( scene, entity2 );

		REQUIRE( !cmd1.canMergeWith( &cmd2 ) );
		REQUIRE( !cmd1.mergeWith( std::make_unique<editor::DeleteEntityCommand>( scene, entity2 ) ) );
	}
}

TEST_CASE( "AddComponentCommand template functionality", "[ecs-commands][unit][AF2.3]" )
{
	SECTION( "AddComponentCommand can be constructed with entity and component" )
	{
		ecs::Scene scene;
		const ecs::Entity entity = scene.createEntity( "TestEntity" );

		components::Transform transform;
		transform.position = { 1.0f, 2.0f, 3.0f };

		editor::AddComponentCommand<components::Transform> cmd( scene, entity, transform );

		REQUIRE( cmd.getDescription() == "Add Transform Component" );
	}

	SECTION( "AddComponentCommand execute adds component to entity" )
	{
		ecs::Scene scene;
		const ecs::Entity entity = scene.createEntity( "TestEntity" );

		components::Visible visible;
		visible.visible = false;
		visible.castShadows = true;

		editor::AddComponentCommand<components::Visible> cmd( scene, entity, visible );

		REQUIRE( !scene.hasComponent<components::Visible>( entity ) );
		REQUIRE( cmd.execute() );
		REQUIRE( scene.hasComponent<components::Visible>( entity ) );

		// Verify component values
		const auto *visibleComp = scene.getComponent<components::Visible>( entity );
		REQUIRE( !visibleComp->visible );
		REQUIRE( visibleComp->castShadows );
	}

	SECTION( "AddComponentCommand undo removes component from entity" )
	{
		ecs::Scene scene;
		const ecs::Entity entity = scene.createEntity( "TestEntity" );

		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = 123;

		editor::AddComponentCommand<components::MeshRenderer> cmd( scene, entity, meshRenderer );

		cmd.execute();
		REQUIRE( scene.hasComponent<components::MeshRenderer>( entity ) );

		REQUIRE( cmd.undo() );
		REQUIRE( !scene.hasComponent<components::MeshRenderer>( entity ) );
	}

	SECTION( "AddComponentCommand works with different component types" )
	{
		ecs::Scene scene;
		ecs::Entity entity = scene.createEntity( "TestEntity" );

		// Test with Selected component
		components::Selected selected( true );
		editor::AddComponentCommand<components::Selected> selectedCmd( scene, entity, selected );

		REQUIRE( selectedCmd.execute() );
		REQUIRE( scene.hasComponent<components::Selected>( entity ) );
		const auto *selectedComp = scene.getComponent<components::Selected>( entity );
		REQUIRE( selectedComp->isPrimary );

		// Test with Transform component
		components::Transform transform;
		transform.position = { 10.0f, 20.0f, 30.0f };
		transform.scale = { 2.0f, 2.0f, 2.0f };
		editor::AddComponentCommand<components::Transform> transformCmd( scene, entity, transform );

		REQUIRE( transformCmd.execute() );
		REQUIRE( scene.hasComponent<components::Transform>( entity ) );
		const auto *transformComp = scene.getComponent<components::Transform>( entity );
		REQUIRE( transformComp->position.x == 10.0f );
		REQUIRE( transformComp->scale.x == 2.0f );
	}

	SECTION( "AddComponentCommand cannot execute twice" )
	{
		ecs::Scene scene;
		ecs::Entity entity = scene.createEntity( "TestEntity" );

		components::Visible visible;
		editor::AddComponentCommand<components::Visible> cmd( scene, entity, visible );

		REQUIRE( cmd.execute() );
		REQUIRE( !cmd.execute() ); // Second execution should fail
	}

	SECTION( "AddComponentCommand cannot undo before execute" )
	{
		ecs::Scene scene;
		ecs::Entity entity = scene.createEntity( "TestEntity" );

		components::Transform transform;
		editor::AddComponentCommand<components::Transform> cmd( scene, entity, transform );

		REQUIRE( !cmd.undo() ); // Cannot undo without execute
	}

	SECTION( "AddComponentCommand handles invalid entity gracefully" )
	{
		ecs::Scene scene;
		const ecs::Entity invalidEntity{}; // Invalid entity

		components::Transform transform;
		editor::AddComponentCommand<components::Transform> cmd( scene, invalidEntity, transform );

		REQUIRE( !cmd.execute() ); // Should fail gracefully
	}

	SECTION( "AddComponentCommand cannot merge with other commands" )
	{
		ecs::Scene scene;
		const ecs::Entity entity = scene.createEntity( "TestEntity" );

		components::Transform transform;
		components::Visible visible;

		editor::AddComponentCommand<components::Transform> cmd1( scene, entity, transform );
		editor::AddComponentCommand<components::Visible> cmd2( scene, entity, visible );

		REQUIRE( !cmd1.canMergeWith( &cmd2 ) );
		REQUIRE( !cmd1.mergeWith( std::make_unique<editor::AddComponentCommand<components::Visible>>( scene, entity, visible ) ) );
	}
}

TEST_CASE( "RemoveComponentCommand template with state capture", "[ecs-commands][unit][AF2.4]" )
{
	SECTION( "RemoveComponentCommand can be constructed with entity" )
	{
		ecs::Scene scene;
		const ecs::Entity entity = scene.createEntity( "TestEntity" );

		// Add a component first
		components::Transform transform;
		transform.position = { 1.0f, 2.0f, 3.0f };
		scene.addComponent( entity, transform );

		editor::RemoveComponentCommand<components::Transform> cmd( scene, entity );

		REQUIRE( cmd.getDescription() == "Remove Transform Component" );
	}

	SECTION( "RemoveComponentCommand execute removes component from entity" )
	{
		ecs::Scene scene;
		const ecs::Entity entity = scene.createEntity( "TestEntity" );

		// Add a component first
		components::Visible visible;
		visible.visible = false;
		visible.castShadows = true;
		scene.addComponent( entity, visible );

		editor::RemoveComponentCommand<components::Visible> cmd( scene, entity );

		REQUIRE( scene.hasComponent<components::Visible>( entity ) );
		REQUIRE( cmd.execute() );
		REQUIRE( !scene.hasComponent<components::Visible>( entity ) );
	}

	SECTION( "RemoveComponentCommand undo restores component with original values" )
	{
		ecs::Scene scene;
		ecs::Entity entity = scene.createEntity( "TestEntity" );

		// Add a component with specific values
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = 456;
		meshRenderer.lodBias = 1.5f;
		scene.addComponent( entity, meshRenderer );

		editor::RemoveComponentCommand<components::MeshRenderer> cmd( scene, entity );

		cmd.execute();
		REQUIRE( !scene.hasComponent<components::MeshRenderer>( entity ) );

		REQUIRE( cmd.undo() );
		REQUIRE( scene.hasComponent<components::MeshRenderer>( entity ) );

		// Verify component values are restored
		const auto *restoredMeshRenderer = scene.getComponent<components::MeshRenderer>( entity );
		REQUIRE( restoredMeshRenderer->meshHandle == 456 );
		REQUIRE( restoredMeshRenderer->lodBias == 1.5f );
	}

	SECTION( "RemoveComponentCommand works with different component types" )
	{
		ecs::Scene scene;
		ecs::Entity entity = scene.createEntity( "TestEntity" );

		// Test with Transform component
		components::Transform transform;
		transform.position = { 5.0f, 10.0f, 15.0f };
		transform.rotation = { 0.1f, 0.2f, 0.3f };
		transform.scale = { 1.5f, 2.0f, 2.5f };
		scene.addComponent( entity, transform );

		editor::RemoveComponentCommand<components::Transform> transformCmd( scene, entity );

		REQUIRE( transformCmd.execute() );
		REQUIRE( !scene.hasComponent<components::Transform>( entity ) );

		REQUIRE( transformCmd.undo() );
		REQUIRE( scene.hasComponent<components::Transform>( entity ) );

		// Verify all transform values are restored
		const auto *restoredTransform = scene.getComponent<components::Transform>( entity );
		REQUIRE( restoredTransform->position.x == 5.0f );
		REQUIRE( restoredTransform->position.y == 10.0f );
		REQUIRE( restoredTransform->position.z == 15.0f );
		REQUIRE( restoredTransform->rotation.x == 0.1f );
		REQUIRE( restoredTransform->scale.x == 1.5f );
		REQUIRE( restoredTransform->scale.y == 2.0f );
		REQUIRE( restoredTransform->scale.z == 2.5f );
	}

	SECTION( "RemoveComponentCommand cannot execute twice" )
	{
		ecs::Scene scene;
		ecs::Entity entity = scene.createEntity( "TestEntity" );

		components::Visible visible;
		scene.addComponent( entity, visible );

		editor::RemoveComponentCommand<components::Visible> cmd( scene, entity );

		REQUIRE( cmd.execute() );
		REQUIRE( !cmd.execute() ); // Second execution should fail
	}

	SECTION( "RemoveComponentCommand cannot undo before execute" )
	{
		ecs::Scene scene;
		ecs::Entity entity = scene.createEntity( "TestEntity" );

		components::Transform transform;
		scene.addComponent( entity, transform );

		editor::RemoveComponentCommand<components::Transform> cmd( scene, entity );

		REQUIRE( !cmd.undo() ); // Cannot undo without execute
	}

	SECTION( "RemoveComponentCommand handles entity without component gracefully" )
	{
		ecs::Scene scene;
		ecs::Entity entity = scene.createEntity( "TestEntity" );

		// Don't add the component - test removal of non-existent component
		editor::RemoveComponentCommand<components::Transform> cmd( scene, entity );

		REQUIRE( !cmd.execute() ); // Should fail gracefully
	}

	SECTION( "RemoveComponentCommand handles invalid entity gracefully" )
	{
		ecs::Scene scene;
		ecs::Entity invalidEntity{}; // Invalid entity

		editor::RemoveComponentCommand<components::Transform> cmd( scene, invalidEntity );

		REQUIRE( !cmd.execute() ); // Should fail gracefully
	}

	SECTION( "RemoveComponentCommand cannot merge with other commands" )
	{
		ecs::Scene scene;
		ecs::Entity entity = scene.createEntity( "TestEntity" );

		components::Transform transform;
		components::Visible visible;
		scene.addComponent( entity, transform );
		scene.addComponent( entity, visible );

		editor::RemoveComponentCommand<components::Transform> cmd1( scene, entity );
		editor::RemoveComponentCommand<components::Visible> cmd2( scene, entity );

		REQUIRE( !cmd1.canMergeWith( &cmd2 ) );
		REQUIRE( !cmd1.mergeWith( std::make_unique<editor::RemoveComponentCommand<components::Visible>>( scene, entity ) ) );
	}
}

TEST_CASE( "SetParentCommand for hierarchy manipulation", "[ecs-commands][unit][AF2.5]" )
{
	SECTION( "SetParentCommand can be constructed with child and parent entities" )
	{
		ecs::Scene scene;
		const ecs::Entity parent = scene.createEntity( "Parent" );
		const ecs::Entity child = scene.createEntity( "Child" );

		editor::SetParentCommand cmd( scene, child, parent );

		REQUIRE( cmd.getDescription() == "Set Parent: Child -> Parent" );
	}
}

TEST_CASE( "RenameEntityCommand for name changes", "[ecs-commands][unit][AF2.6]" )
{
	SECTION( "RenameEntityCommand can be constructed with entity and new name" )
	{
		ecs::Scene scene;
		const ecs::Entity entity = scene.createEntity( "OldName" );

		editor::RenameEntityCommand cmd( scene, entity, "NewName" );

		REQUIRE( cmd.getDescription() == "Rename Entity: OldName -> NewName" );
	}
}

TEST_CASE( "ModifyVisibleCommand for visibility changes", "[ecs-commands][unit][T2.4][visible]" )
{
	SECTION( "ModifyVisibleCommand can be constructed with entity and new visible state" )
	{
		ecs::Scene scene;
		const ecs::Entity entity = scene.createEntity( "TestEntity" );

		components::Visible oldVisible;
		oldVisible.visible = true;
		oldVisible.castShadows = true;
		oldVisible.receiveShadows = true;
		scene.addComponent( entity, oldVisible );

		components::Visible newVisible;
		newVisible.visible = false;
		newVisible.castShadows = false;
		newVisible.receiveShadows = false;

		editor::ModifyVisibleCommand cmd( scene, entity, newVisible );

		REQUIRE( cmd.getDescription() == "Modify Visibility" );
	}

	SECTION( "ModifyVisibleCommand execute modifies visible component" )
	{
		ecs::Scene scene;
		const ecs::Entity entity = scene.createEntity( "TestEntity" );

		components::Visible oldVisible;
		oldVisible.visible = true;
		oldVisible.castShadows = true;
		oldVisible.receiveShadows = true;
		scene.addComponent( entity, oldVisible );

		components::Visible newVisible;
		newVisible.visible = false;
		newVisible.castShadows = false;
		newVisible.receiveShadows = false;

		editor::ModifyVisibleCommand cmd( scene, entity, newVisible );

		REQUIRE( cmd.execute() );

		const auto *visible = scene.getComponent<components::Visible>( entity );
		REQUIRE( visible != nullptr );
		REQUIRE( visible->visible == false );
		REQUIRE( visible->castShadows == false );
		REQUIRE( visible->receiveShadows == false );
	}

	SECTION( "ModifyVisibleCommand undo restores original visible state" )
	{
		ecs::Scene scene;
		const ecs::Entity entity = scene.createEntity( "TestEntity" );

		components::Visible oldVisible;
		oldVisible.visible = true;
		oldVisible.castShadows = true;
		oldVisible.receiveShadows = false;
		scene.addComponent( entity, oldVisible );

		components::Visible newVisible;
		newVisible.visible = false;
		newVisible.castShadows = false;
		newVisible.receiveShadows = true;

		editor::ModifyVisibleCommand cmd( scene, entity, newVisible );

		REQUIRE( cmd.execute() );
		REQUIRE( cmd.undo() );

		const auto *visible = scene.getComponent<components::Visible>( entity );
		REQUIRE( visible != nullptr );
		REQUIRE( visible->visible == true );
		REQUIRE( visible->castShadows == true );
		REQUIRE( visible->receiveShadows == false );
	}
}

TEST_CASE( "EcsCommandFactory convenient command creation", "[ecs-commands][unit][AF2.8]" )
{
	SECTION( "EcsCommandFactory can create all command types" )
	{
		ecs::Scene scene;
		const ecs::Entity entity = scene.createEntity( "TestEntity" );
		const ecs::Entity parent = scene.createEntity( "Parent" );

		// Test entity commands
		const auto createCmd = editor::EcsCommandFactory::createEntity( scene, "NewEntity" );
		REQUIRE( createCmd != nullptr );
		REQUIRE( createCmd->getDescription() == "Create Entity: NewEntity" );

		const auto deleteCmd = editor::EcsCommandFactory::deleteEntity( scene, entity );
		REQUIRE( deleteCmd != nullptr );
		REQUIRE( deleteCmd->getDescription() == "Delete Entity: TestEntity" );

		// Test component commands
		components::Transform transform;
		transform.position = { 1.0f, 2.0f, 3.0f };
		const auto addCompCmd = editor::EcsCommandFactory::addComponent( scene, entity, transform );
		REQUIRE( addCompCmd != nullptr );
		REQUIRE( addCompCmd->getDescription() == "Add Transform Component" );

		scene.addComponent( entity, transform ); // Add it first for removal
		const auto removeCompCmd = editor::EcsCommandFactory::removeComponent<components::Transform>( scene, entity );
		REQUIRE( removeCompCmd != nullptr );
		REQUIRE( removeCompCmd->getDescription() == "Remove Transform Component" );

		// Test hierarchy commands
		const auto setParentCmd = editor::EcsCommandFactory::setParent( scene, entity, parent );
		REQUIRE( setParentCmd != nullptr );
		REQUIRE( setParentCmd->getDescription() == "Set Parent: TestEntity -> Parent" );

		// Test rename commands
		const auto renameCmd = editor::EcsCommandFactory::renameEntity( scene, entity, "RenamedEntity" );
		REQUIRE( renameCmd != nullptr );
		REQUIRE( renameCmd->getDescription() == "Rename Entity: TestEntity -> RenamedEntity" );
	}
}

TEST_CASE( "CreateEntityFromAssetCommand basic functionality", "[ecs-commands][unit][T8.1][AF1]" )
{
	SECTION( "CreateEntityFromAssetCommand can be constructed with required parameters" )
	{
		ecs::Scene scene;
		assets::AssetManager assetManager;
		MockGPUResourceManager gpuManager;
		const std::string assetPath = "test.gltf";
		const math::Vec3f worldPosition{ 1.0f, 2.0f, 3.0f };

		editor::CreateEntityFromAssetCommand cmd( scene, assetManager, gpuManager, assetPath, worldPosition );

		REQUIRE( cmd.getDescription() == "Create entity from test.gltf" );
		REQUIRE( !cmd.getCreatedEntity().isValid() ); // Entity should not exist until execute
	}

	SECTION( "CreateEntityFromAssetCommand execute loads asset and creates entity" )
	{
		// Given a scene and asset manager with a valid glTF file
		ecs::Scene scene;
		assets::AssetManager assetManager;
		MockGPUResourceManager gpuManager;
		const std::string assetPath = "assets/test/triangle.gltf";
		const math::Vec3f worldPosition{ 5.0f, 10.0f, 15.0f };

		// Setup glTF loader callback (from integration code)
		assets::AssetManager::setSceneLoaderCallback( []( const std::string &path ) -> std::shared_ptr<assets::Scene> {
			// Create a minimal test scene
			auto testScene = std::make_shared<assets::Scene>();
			testScene->setPath( path );
			testScene->setLoaded( true );

			// Create a simple node hierarchy
			auto rootNode = std::make_unique<assets::SceneNode>( "RootNode" );
			rootNode->setTransform( assets::Transform{} );
			testScene->addRootNode( std::move( rootNode ) );

			return testScene;
		} );

		editor::CreateEntityFromAssetCommand cmd( scene, assetManager, gpuManager, assetPath, worldPosition );

		// When execute is called
		const bool result = cmd.execute();

		// Then entity should be created successfully
		REQUIRE( result == true );

		const ecs::Entity entity = cmd.getCreatedEntity();
		REQUIRE( entity.isValid() );
		REQUIRE( scene.isValid( entity ) );

		// Entity should have Name component from asset
		REQUIRE( scene.hasComponent<components::Name>( entity ) );
		const auto *name = scene.getComponent<components::Name>( entity );
		REQUIRE( name->name == "RootNode" );

		// Entity should have Transform at world position
		REQUIRE( scene.hasComponent<components::Transform>( entity ) );
		const auto *transform = scene.getComponent<components::Transform>( entity );
		REQUIRE( transform->position.x == 5.0f );
		REQUIRE( transform->position.y == 10.0f );
		REQUIRE( transform->position.z == 15.0f );

		// Cleanup
		assets::AssetManager::clearSceneLoaderCallback();
	}

	SECTION( "CreateEntityFromAssetCommand undo destroys created entity" )
	{
		// Given a scene with an entity created from an asset
		ecs::Scene scene;
		assets::AssetManager assetManager;
		MockGPUResourceManager gpuManager;
		const std::string assetPath = "assets/test/triangle.gltf";
		const math::Vec3f worldPosition{ 0.0f, 0.0f, 0.0f };

		// Setup glTF loader callback
		assets::AssetManager::setSceneLoaderCallback( []( const std::string &path ) -> std::shared_ptr<assets::Scene> {
			auto testScene = std::make_shared<assets::Scene>();
			testScene->setPath( path );
			testScene->setLoaded( true );

			auto rootNode = std::make_unique<assets::SceneNode>( "TestNode" );
			rootNode->setTransform( assets::Transform{} );
			testScene->addRootNode( std::move( rootNode ) );

			return testScene;
		} );

		editor::CreateEntityFromAssetCommand cmd( scene, assetManager, gpuManager, assetPath, worldPosition );
		cmd.execute();

		const ecs::Entity entity = cmd.getCreatedEntity();
		REQUIRE( scene.isValid( entity ) );

		// When undo is called
		const bool undoResult = cmd.undo();

		// Then entity should be destroyed
		REQUIRE( undoResult == true );
		REQUIRE( !scene.isValid( entity ) );
		REQUIRE( scene.getEntityCount() == 0 );

		// Cleanup
		assets::AssetManager::clearSceneLoaderCallback();
	}

	SECTION( "CreateEntityFromAssetCommand cannot execute twice" )
	{
		ecs::Scene scene;
		assets::AssetManager assetManager;
		MockGPUResourceManager gpuManager;
		const std::string assetPath = "assets/test/triangle.gltf";
		const math::Vec3f worldPosition{ 0.0f, 0.0f, 0.0f };

		// Setup glTF loader callback
		assets::AssetManager::setSceneLoaderCallback( []( const std::string &path ) -> std::shared_ptr<assets::Scene> {
			auto testScene = std::make_shared<assets::Scene>();
			testScene->setPath( path );
			testScene->setLoaded( true );

			auto rootNode = std::make_unique<assets::SceneNode>( "TestNode" );
			testScene->addRootNode( std::move( rootNode ) );

			return testScene;
		} );

		editor::CreateEntityFromAssetCommand cmd( scene, assetManager, gpuManager, assetPath, worldPosition );

		REQUIRE( cmd.execute() == true );
		REQUIRE( cmd.execute() == false ); // Second execution should fail

		// Cleanup
		assets::AssetManager::clearSceneLoaderCallback();
	}

	SECTION( "CreateEntityFromAssetCommand handles invalid asset path" )
	{
		ecs::Scene scene;
		assets::AssetManager assetManager;
		MockGPUResourceManager gpuManager;
		const std::string assetPath = "non_existent_file.gltf";
		const math::Vec3f worldPosition{ 0.0f, 0.0f, 0.0f };

		editor::CreateEntityFromAssetCommand cmd( scene, assetManager, gpuManager, assetPath, worldPosition );

		// When execute is called with invalid asset
		const bool result = cmd.execute();

		// Then it should fail gracefully
		REQUIRE( result == false );
		REQUIRE( !cmd.getCreatedEntity().isValid() );
		REQUIRE( scene.getEntityCount() == 0 );
	}
}
