#include <catch2/catch_test_macros.hpp>

#include "editor/transform_commands.h"
#include "runtime/ecs.h"
#include "runtime/components.h"

using namespace editor;
using namespace ecs;
using namespace components;

TEST_CASE( "Command interface exists and is pure virtual", "[AF4.1][unit]" )
{
	SECTION( "Commands can be polymorphically handled" )
	{
		Scene scene;
		const Entity entity = scene.createEntity();
		scene.addComponent<Transform>( entity, Transform{} );

		auto command = std::make_unique<TransformEntityCommand>( entity, scene );
		Command *basePtr = command.get();

		REQUIRE( basePtr != nullptr );
		REQUIRE( command->getDescription() == "Transform Entity" );
	}
}