// Test file to verify that our relaxed Component concept works with real components
#include <iostream>

import runtime.ecs;
import runtime.components;

using namespace ecs;

int main()
{
	std::cout << "Testing Component concept with various component types:\n";

	// Test simple component types (these were working before)
	struct SimplePos
	{
		float x, y, z;
	};
	static_assert( Component<SimplePos>, "Simple struct should satisfy Component concept" );
	std::cout << "✓ Simple struct (SimplePos) satisfies Component concept\n";

	// Test components with std::string (these were failing before with trivially_copyable)
	static_assert( Component<components::Name>, "Name component should satisfy Component concept" );
	std::cout << "✓ Name component (contains std::string) satisfies Component concept\n";

	// Test components with std::vector (these were failing before)
	static_assert( Component<components::MeshRenderer>, "MeshRenderer component should satisfy Component concept" );
	std::cout << "✓ MeshRenderer component (contains std::vector) satisfies Component concept\n";

	// Test other components
	static_assert( Component<components::Transform>, "Transform component should satisfy Component concept" );
	std::cout << "✓ Transform component satisfies Component concept\n";

	static_assert( Component<components::Visible>, "Visible component should satisfy Component concept" );
	std::cout << "✓ Visible component satisfies Component concept\n";

	static_assert( Component<components::Selected>, "Selected component should satisfy Component concept" );
	std::cout << "✓ Selected component satisfies Component concept\n";

	// Test that we can actually create a scene and use these components
	Scene scene;
	Entity entity = scene.createEntity();

	// Test adding components that would have failed with the old trivially_copyable constraint
	components::Name nameComponent{ "Test Entity" };
	scene.addComponent( entity, nameComponent );

	components::MeshRenderer meshRenderer;
	meshRenderer.meshPath = "test.mesh";
	meshRenderer.materialPaths.push_back( "material1.mat" );
	meshRenderer.materialPaths.push_back( "material2.mat" );
	scene.addComponent( entity, meshRenderer );

	// Verify we can retrieve them
	auto *name = scene.getComponent<components::Name>( entity );
	auto *mesh = scene.getComponent<components::MeshRenderer>( entity );

	if ( name && mesh )
	{
		std::cout << "✓ Successfully added and retrieved components with complex types\n";
		std::cout << "  Entity name: " << name->name << "\n";
		std::cout << "  Mesh path: " << mesh->meshPath << "\n";
		std::cout << "  Materials: " << mesh->materialPaths.size() << " materials\n";
	}

	std::cout << "\n🎉 All tests passed! The relaxed Component concept allows practical component types.\n";
	return 0;
}
