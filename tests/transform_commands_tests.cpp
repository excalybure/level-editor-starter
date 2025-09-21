#include <catch2/catch_test_macros.hpp>
#include "editor/transform_commands.h"
#include "runtime/ecs.h"
#include <memory>
#include <string>
#include <vector>

TEST_CASE( "Command interface basic contract", "[transform][command][unit]" )
{
	SECTION( "Command is abstract interface" )
	{
		// This test ensures Command is properly defined as an abstract interface
		// We can't instantiate Command directly, but we can verify polymorphic behavior

		// Create a minimal test implementation
		class TestCommand : public editor::Command
		{
		public:
			bool execute() override { return true; }
			bool undo() override { return true; }
			std::string getDescription() const override { return "Test Command"; }
		};

		auto command = std::make_unique<TestCommand>();

		// Test basic interface contract
		REQUIRE( command->execute() == true );
		REQUIRE( command->undo() == true );
		REQUIRE( command->getDescription() == "Test Command" );

		// Test polymorphic behavior
		std::unique_ptr<editor::Command> basePtr = std::move( command );
		REQUIRE( basePtr->execute() == true );
		REQUIRE( basePtr->undo() == true );
		REQUIRE( basePtr->getDescription() == "Test Command" );
	}
}

TEST_CASE( "TransformEntityCommand construction", "[transform][command][unit]" )
{
	SECTION( "TransformEntityCommand can be constructed with entity and scene" )
	{
		// Create a scene and entity for testing
		ecs::Scene scene;
		const ecs::Entity entity = scene.createEntity( "TestEntity" );

		// Add a transform component
		scene.addComponent( entity, components::Transform{ .position = { 1.0f, 2.0f, 3.0f }, .rotation = { 0.1f, 0.2f, 0.3f }, .scale = { 1.0f, 1.0f, 1.0f } } );

		// This should now work with our implementation
		auto command = std::make_unique<editor::TransformEntityCommand>( entity, scene );

		// Test that the command was created successfully
		REQUIRE( command != nullptr );
		REQUIRE( command->getDescription() == "Transform Entity" );
	}
}

TEST_CASE( "TransformEntityCommand execute and undo cycle", "[transform][command][unit]" )
{
	SECTION( "TransformEntityCommand can execute and undo transform changes" )
	{
		// Create a scene and entity for testing
		ecs::Scene scene;
		const ecs::Entity entity = scene.createEntity( "TestEntity" );

		// Add initial transform component
		const components::Transform initialTransform{
			.position = { 1.0f, 2.0f, 3.0f },
			.rotation = { 0.1f, 0.2f, 0.3f },
			.scale = { 1.0f, 1.0f, 1.0f }
		};
		scene.addComponent( entity, initialTransform );

		// Define a new transform state
		const components::Transform newTransform{
			.position = { 5.0f, 6.0f, 7.0f },
			.rotation = { 0.4f, 0.5f, 0.6f },
			.scale = { 2.0f, 2.0f, 2.0f }
		};

		// Create command with explicit before/after states
		auto command = std::make_unique<editor::TransformEntityCommand>( entity, scene, initialTransform, newTransform );

		// Execute the command
		REQUIRE( command->execute() == true );

		// Verify the transform was changed
		const auto *currentTransform = scene.getComponent<components::Transform>( entity );
		REQUIRE( currentTransform != nullptr );
		REQUIRE( currentTransform->position.x == 5.0f );
		REQUIRE( currentTransform->position.y == 6.0f );
		REQUIRE( currentTransform->position.z == 7.0f );

		// Undo the command
		REQUIRE( command->undo() == true );

		// Verify the transform was reverted
		currentTransform = scene.getComponent<components::Transform>( entity );
		REQUIRE( currentTransform != nullptr );
		REQUIRE( currentTransform->position.x == 1.0f );
		REQUIRE( currentTransform->position.y == 2.0f );
		REQUIRE( currentTransform->position.z == 3.0f );
	}
}

TEST_CASE( "BatchTransformCommand construction", "[transform][command][unit]" )
{
	SECTION( "BatchTransformCommand can be constructed with multiple entities" )
	{
		// Create a scene and multiple entities for testing
		ecs::Scene scene;
		const ecs::Entity entity1 = scene.createEntity( "TestEntity1" );
		const ecs::Entity entity2 = scene.createEntity( "TestEntity2" );

		// Add transform components
		scene.addComponent( entity1, components::Transform{ .position = { 1.0f, 2.0f, 3.0f }, .rotation = { 0.1f, 0.2f, 0.3f }, .scale = { 1.0f, 1.0f, 1.0f } } );
		scene.addComponent( entity2, components::Transform{ .position = { 4.0f, 5.0f, 6.0f }, .rotation = { 0.4f, 0.5f, 0.6f }, .scale = { 1.0f, 1.0f, 1.0f } } );

		// This should now work with our implementation
		std::vector<ecs::Entity> entities = { entity1, entity2 };
		auto command = std::make_unique<editor::BatchTransformCommand>( entities, scene );

		// Test that the command was created successfully
		REQUIRE( command != nullptr );
		REQUIRE( command->getDescription() == "Transform 2 Entities" );
	}
}