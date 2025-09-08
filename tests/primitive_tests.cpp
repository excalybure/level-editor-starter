#include <catch2/catch_test_macros.hpp>

import engine.assets;

TEST_CASE( "Primitive Basic Operations", "[primitive][tdd]" )
{
	SECTION( "Create empty primitive" )
	{
		assets::Primitive primitive;

		REQUIRE( primitive.getVertexCount() == 0 );
		REQUIRE( primitive.getIndexCount() == 0 );
		REQUIRE( !primitive.hasMaterial() );
		REQUIRE( !primitive.hasBounds() );
	}

	SECTION( "Add vertex to primitive" )
	{
		assets::Primitive primitive;
		assets::Vertex vertex;
		vertex.position.x = 1.0f;
		vertex.position.y = 2.0f;
		vertex.position.z = 3.0f;

		primitive.addVertex( vertex );

		REQUIRE( primitive.getVertexCount() == 1 );
		const auto &vertices = primitive.getVertices();
		REQUIRE( vertices.size() == 1 );
		REQUIRE( vertices[0].position.x == 1.0f );
		REQUIRE( vertices[0].position.y == 2.0f );
		REQUIRE( vertices[0].position.z == 3.0f );
		REQUIRE( primitive.hasBounds() );
	}

	SECTION( "Add index to primitive" )
	{
		assets::Primitive primitive;

		primitive.addIndex( 0 );
		primitive.addIndex( 1 );
		primitive.addIndex( 2 );

		REQUIRE( primitive.getIndexCount() == 3 );
		const auto &indices = primitive.getIndices();
		REQUIRE( indices.size() == 3 );
		REQUIRE( indices[0] == 0 );
		REQUIRE( indices[1] == 1 );
		REQUIRE( indices[2] == 2 );
	}

	SECTION( "Set material path" )
	{
		assets::Primitive primitive;
		const std::string materialPath = "materials/test.mat";

		primitive.setMaterialPath( materialPath );

		REQUIRE( primitive.hasMaterial() );
		REQUIRE( primitive.getMaterialPath() == materialPath );
	}
}

TEST_CASE( "Mesh Primitive-based Operations", "[mesh][primitive][tdd]" )
{
	SECTION( "Create empty mesh" )
	{
		assets::Mesh mesh;

		REQUIRE( mesh.getPrimitiveCount() == 0 );
	}

	SECTION( "Add primitive to mesh" )
	{
		assets::Mesh mesh;
		assets::Primitive primitive;

		// Add some data to the primitive
		assets::Vertex vertex;
		vertex.position.x = 1.0f;
		primitive.addVertex( vertex );
		primitive.addIndex( 0 );

		mesh.addPrimitive( primitive );

		REQUIRE( mesh.getPrimitiveCount() == 1 );
		const auto &retrievedPrimitive = mesh.getPrimitive( 0 );
		REQUIRE( retrievedPrimitive.getVertexCount() == 1 );
		REQUIRE( retrievedPrimitive.getIndexCount() == 1 );
	}
}