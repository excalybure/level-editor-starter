#include <catch2/catch_test_macros.hpp>

#include "editor/transform_commands.h"
#include "editor/commands/Command.h"
#include "runtime/ecs.h"
#include "runtime/components.h"


TEST_CASE( "Command interface exists and is pure virtual", "[AF3.1][unit]" )
{
	SECTION( "Commands can be polymorphically handled" )
	{
		ecs::Scene scene;
		const ecs::Entity entity = scene.createEntity();
		scene.addComponent<components::Transform>( entity, components::Transform{} );

		auto command = std::make_unique<editor::TransformEntityCommand>( entity, scene );
		Command *basePtr = command.get();

		REQUIRE( basePtr != nullptr );
		REQUIRE( command->getDescription() == "Transform Entity" );
	}
}

TEST_CASE( "TransformEntityCommand implements new Command interface", "[AF3.1][unit]" )
{
	SECTION( "TransformEntityCommand has all required methods" )
	{
		ecs::Scene scene;
		const ecs::Entity entity = scene.createEntity( "TestEntity" );
		
		components::Transform beforeTransform;
		beforeTransform.position = { 1.0f, 2.0f, 3.0f };
		
		components::Transform afterTransform;
		afterTransform.position = { 4.0f, 5.0f, 6.0f };
		
		scene.addComponent( entity, beforeTransform );
		
		editor::TransformEntityCommand command( entity, scene, beforeTransform, afterTransform );
		
		// Test all Command interface methods
		REQUIRE( command.getDescription() == "Transform TestEntity" );
		REQUIRE( command.getMemoryUsage() > 0 );
		
		// Test execution and undo
		REQUIRE( command.execute() );
		const auto *transform = scene.getComponent<components::Transform>( entity );
		REQUIRE( transform->position.x == 4.0f );
		
		REQUIRE( command.undo() );
		transform = scene.getComponent<components::Transform>( entity );
		REQUIRE( transform->position.x == 1.0f );
	}
}

TEST_CASE( "TransformEntityCommand command merging", "[AF3.3][unit]" )
{
	SECTION( "Can merge with same entity transform commands" )
	{
		ecs::Scene scene;
		const ecs::Entity entity = scene.createEntity();
		scene.addComponent<components::Transform>( entity, components::Transform{} );
		
		editor::TransformEntityCommand command1( entity, scene );
		editor::TransformEntityCommand command2( entity, scene );
		
		REQUIRE( command1.canMergeWith( &command2 ) );
		
		components::Transform afterTransform;
		afterTransform.position = { 10.0f, 20.0f, 30.0f };
		command2.updateAfterTransform( afterTransform );
		
		auto command2Ptr = std::make_unique<editor::TransformEntityCommand>( entity, scene );
		command2Ptr->updateAfterTransform( afterTransform );
		
		REQUIRE( command1.mergeWith( std::move( command2Ptr ) ) );
	}
	
	SECTION( "Cannot merge with different entity transform commands" )
	{
		ecs::Scene scene;
		const ecs::Entity entity1 = scene.createEntity();
		const ecs::Entity entity2 = scene.createEntity();
		scene.addComponent<components::Transform>( entity1, components::Transform{} );
		scene.addComponent<components::Transform>( entity2, components::Transform{} );
		
		editor::TransformEntityCommand command1( entity1, scene );
		editor::TransformEntityCommand command2( entity2, scene );
		
		REQUIRE_FALSE( command1.canMergeWith( &command2 ) );
		
		auto command2Ptr = std::make_unique<editor::TransformEntityCommand>( entity2, scene );
		REQUIRE_FALSE( command1.mergeWith( std::move( command2Ptr ) ) );
	}
}

TEST_CASE( "BatchTransformCommand implements new Command interface", "[AF3.2][unit]" )
{
	SECTION( "BatchTransformCommand has all required methods" )
	{
		ecs::Scene scene;
		std::vector<ecs::Entity> entities;
		entities.push_back( scene.createEntity( "Entity1" ) );
		entities.push_back( scene.createEntity( "Entity2" ) );
		
		for ( auto entity : entities )
		{
			scene.addComponent<components::Transform>( entity, components::Transform{} );
		}
		
		editor::BatchTransformCommand command( entities, scene );
		
		// Test all Command interface methods
		REQUIRE( command.getDescription() == "Transform 2 Entities" );
		REQUIRE( command.getMemoryUsage() > 0 );
		
		// Test entities getter
		auto retrievedEntities = command.getEntities();
		REQUIRE( retrievedEntities.size() == 2 );
		REQUIRE( retrievedEntities[0] == entities[0] );
		REQUIRE( retrievedEntities[1] == entities[1] );
	}
}

TEST_CASE( "BatchTransformCommand command merging", "[AF3.3][unit]" )
{
	SECTION( "Can merge with same entities batch transform commands" )
	{
		ecs::Scene scene;
		std::vector<ecs::Entity> entities;
		entities.push_back( scene.createEntity() );
		entities.push_back( scene.createEntity() );
		
		for ( auto entity : entities )
		{
			scene.addComponent<components::Transform>( entity, components::Transform{} );
		}
		
		editor::BatchTransformCommand command1( entities, scene );
		editor::BatchTransformCommand command2( entities, scene );
		
		REQUIRE( command1.canMergeWith( &command2 ) );
		
		auto command2Ptr = std::make_unique<editor::BatchTransformCommand>( entities, scene );
		REQUIRE( command1.mergeWith( std::move( command2Ptr ) ) );
	}
	
	SECTION( "Cannot merge with different entities batch transform commands" )
	{
		ecs::Scene scene;
		std::vector<ecs::Entity> entities1;
		std::vector<ecs::Entity> entities2;
		
		entities1.push_back( scene.createEntity() );
		entities2.push_back( scene.createEntity() );
		
		for ( auto entity : entities1 )
		{
			scene.addComponent<components::Transform>( entity, components::Transform{} );
		}
		for ( auto entity : entities2 )
		{
			scene.addComponent<components::Transform>( entity, components::Transform{} );
		}
		
		editor::BatchTransformCommand command1( entities1, scene );
		editor::BatchTransformCommand command2( entities2, scene );
		
		REQUIRE_FALSE( command1.canMergeWith( &command2 ) );
		
		auto command2Ptr = std::make_unique<editor::BatchTransformCommand>( entities2, scene );
		REQUIRE_FALSE( command1.mergeWith( std::move( command2Ptr ) ) );
	}
}

TEST_CASE( "TransformCommandFactory creates mergeable commands", "[AF3.5][unit]" )
{
	SECTION( "Factory creates TransformEntityCommand for single entity" )
	{
		ecs::Scene scene;
		std::vector<ecs::Entity> entities;
		entities.push_back( scene.createEntity() );
		scene.addComponent<components::Transform>( entities[0], components::Transform{} );
		
		auto command = editor::TransformCommandFactory::createCommand( entities, scene );
		REQUIRE( command != nullptr );
		REQUIRE( command->getDescription() == "Transform Entity" );
		
		// Verify it's actually a TransformEntityCommand
		auto *transformCommand = dynamic_cast<editor::TransformEntityCommand *>( command.get() );
		REQUIRE( transformCommand != nullptr );
	}
	
	SECTION( "Factory creates BatchTransformCommand for multiple entities" )
	{
		ecs::Scene scene;
		std::vector<ecs::Entity> entities;
		entities.push_back( scene.createEntity() );
		entities.push_back( scene.createEntity() );
		
		for ( auto entity : entities )
		{
			scene.addComponent<components::Transform>( entity, components::Transform{} );
		}
		
		auto command = editor::TransformCommandFactory::createCommand( entities, scene );
		REQUIRE( command != nullptr );
		REQUIRE( command->getDescription() == "Transform 2 Entities" );
		
		// Verify it's actually a BatchTransformCommand
		auto *batchCommand = dynamic_cast<editor::BatchTransformCommand *>( command.get() );
		REQUIRE( batchCommand != nullptr );
	}
	
	SECTION( "Factory returns nullptr for empty entity list" )
	{
		ecs::Scene scene;
		std::vector<ecs::Entity> entities; // Empty
		
		auto command = editor::TransformCommandFactory::createCommand( entities, scene );
		REQUIRE( command == nullptr );
	}
}

TEST_CASE( "Transform command memory usage calculation", "[AF3.4][unit]" )
{
	SECTION( "TransformEntityCommand reports reasonable memory usage" )
	{
		ecs::Scene scene;
		const ecs::Entity entity = scene.createEntity();
		scene.addComponent<components::Transform>( entity, components::Transform{} );
		
		editor::TransformEntityCommand command( entity, scene );
		const size_t memoryUsage = command.getMemoryUsage();
		
		REQUIRE( memoryUsage >= sizeof( editor::TransformEntityCommand ) );
		REQUIRE( memoryUsage < 10000 ); // Reasonable upper bound
	}
	
	SECTION( "BatchTransformCommand memory usage scales with entity count" )
	{
		ecs::Scene scene;
		
		// Create single entity batch
		std::vector<ecs::Entity> singleEntity;
		singleEntity.push_back( scene.createEntity() );
		scene.addComponent<components::Transform>( singleEntity[0], components::Transform{} );
		
		editor::BatchTransformCommand singleCommand( singleEntity, scene );
		const size_t singleMemory = singleCommand.getMemoryUsage();
		
		// Create multi entity batch
		std::vector<ecs::Entity> multiEntities;
		for ( int i = 0; i < 5; ++i )
		{
			multiEntities.push_back( scene.createEntity() );
			scene.addComponent<components::Transform>( multiEntities.back(), components::Transform{} );
		}
		
		editor::BatchTransformCommand multiCommand( multiEntities, scene );
		const size_t multiMemory = multiCommand.getMemoryUsage();
		
		REQUIRE( multiMemory > singleMemory );
	}
}