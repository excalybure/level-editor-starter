// Test to verify entity reference fixup after entity deletion/recreation
#include <catch2/catch_test_macros.hpp>

#include "editor/commands/CommandHistory.h"
#include "editor/commands/EcsCommands.h"
#include "runtime/ecs.h"
#include "runtime/components.h"

using namespace editor;

// Helper function to find entity by name (copied from command_integration_tests.cpp)

TEST_CASE( "Entity reference fixup after delete/recreate cycle", "[entity-fixup][integration]" )
{
	SECTION( "AddComponentCommand redo works after entity delete/undo cycle" )
	{
		ecs::Scene scene;
		CommandHistory history;

		// Create an entity
		auto createCmd = EcsCommandFactory::createEntity( scene, "TestEntity" );
		REQUIRE( history.executeCommand( std::move( createCmd ) ) );

		// Get the entity reference
		auto entity = scene.findEntityByName( "TestEntity" );
		REQUIRE( entity.isValid() );
		const auto originalGeneration = entity.generation;

		// Add a component to the entity
		auto addCmd = EcsCommandFactory::addComponent( scene, entity, components::Visible{} );
		REQUIRE( history.executeCommand( std::move( addCmd ) ) );
		REQUIRE( scene.hasComponent<components::Visible>( entity ) );

		// Delete the entity
		auto deleteCmd = EcsCommandFactory::deleteEntity( scene, entity );
		REQUIRE( history.executeCommand( std::move( deleteCmd ) ) );
		REQUIRE( !scene.isValid( entity ) );

		// Undo the delete (this recreates the entity with new generation)
		REQUIRE( history.undo() );
		auto recreatedEntity = scene.findEntityByName( "TestEntity" );
		REQUIRE( recreatedEntity.isValid() );
		REQUIRE( recreatedEntity.id == entity.id );
		REQUIRE( recreatedEntity.generation != originalGeneration ); // New generation

		// Undo the add component command
		REQUIRE( history.undo() );
		REQUIRE( !scene.hasComponent<components::Visible>( recreatedEntity ) );

		// Now redo the add component command - this should work with entity reference fixup
		REQUIRE( history.redo() ); // This should succeed (AddComponentCommand with fixed entity reference)

		// Verify the component was added to the recreated entity
		REQUIRE( scene.hasComponent<components::Visible>( recreatedEntity ) );
	}
}