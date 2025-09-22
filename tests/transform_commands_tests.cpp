#include <catch2/catch_test_macros.hpp>

#include "editor/transform_commands.h"
#include "runtime/ecs.h"
#include "runtime/components.h"


TEST_CASE( "Command interface exists and is pure virtual", "[AF4.1][unit]" )
{
	SECTION( "Commands can be polymorphically handled" )
	{
		ecs::Scene scene;
		const ecs::Entity entity = scene.createEntity();
		scene.addComponent<components::Transform>( entity, components::Transform{} );

		auto command = std::make_unique<editor::TransformEntityCommand>( entity, scene );
		editor::Command *basePtr = command.get();

		REQUIRE( basePtr != nullptr );
		REQUIRE( command->getDescription() == "Transform Entity" );
	}
}