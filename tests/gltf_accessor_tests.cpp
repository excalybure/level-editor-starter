#include <catch2/catch_test_macros.hpp>
#include <span>
#include <vector>
#include <array>
#include <cstdint>

#include "engine/gltf_loader/gltf_loader.h"
#include "math/vec.h"

// RED Phase: Tests for accessor & buffer view handling (should fail initially)
TEST_CASE( "GLTF Accessor Utilities", "[gltf][accessor][unit]" )
{
	SECTION( "Extract float3 positions from accessor" )
	{
		// Test data: 3 vertices as float3 positions
		const std::vector<float> buffer = {
			0.0f, 0.0f, 0.0f, // vertex 0
			1.0f,
			0.0f,
			0.0f, // vertex 1
			0.5f,
			1.0f,
			0.0f // vertex 2
		};

		// This should extract positions as arrays of float[3]
		const auto positions = gltf_loader::extractFloat3Positions( buffer.data(), 3, 0, 12 );

		REQUIRE( positions.size() == 3 );
		REQUIRE( positions[0].x == 0.0f );
		REQUIRE( positions[0].y == 0.0f );
		REQUIRE( positions[0].z == 0.0f );
		REQUIRE( positions[1].x == 1.0f );
		REQUIRE( positions[1].y == 0.0f );
		REQUIRE( positions[1].z == 0.0f );
		REQUIRE( positions[2].x == 0.5f );
		REQUIRE( positions[2].y == 1.0f );
		REQUIRE( positions[2].z == 0.0f );
	}

	SECTION( "Extract uint32 indices from UNSIGNED_SHORT" )
	{
		// Test data: indices as uint16_t that should be converted to uint32_t
		const std::vector<std::uint16_t> buffer = { 0, 1, 2, 1, 3, 2 };

		const auto indices = gltf_loader::extractIndicesAsUint32(
			reinterpret_cast<const std::uint8_t *>( buffer.data() ),
			buffer.size(),
			gltf_loader::ComponentType::UnsignedShort,
			0,
			sizeof( std::uint16_t ) );

		REQUIRE( indices.size() == 6 );
		REQUIRE( indices[0] == 0 );
		REQUIRE( indices[1] == 1 );
		REQUIRE( indices[2] == 2 );
		REQUIRE( indices[3] == 1 );
		REQUIRE( indices[4] == 3 );
		REQUIRE( indices[5] == 2 );
	}

	SECTION( "Extract uint32 indices from UNSIGNED_BYTE" )
	{
		// Test data: indices as uint8_t that should be converted to uint32_t
		const std::vector<std::uint8_t> buffer = { 0, 1, 2 };

		const auto indices = gltf_loader::extractIndicesAsUint32(
			buffer.data(),
			buffer.size(),
			gltf_loader::ComponentType::UnsignedByte,
			0,
			sizeof( std::uint8_t ) );

		REQUIRE( indices.size() == 3 );
		REQUIRE( indices[0] == 0 );
		REQUIRE( indices[1] == 1 );
		REQUIRE( indices[2] == 2 );
	}

	SECTION( "Extract float2 texture coordinates" )
	{
		// Test data: UV coordinates as float2
		const std::vector<float> buffer = {
			0.0f, 0.0f, // uv 0
			1.0f,
			0.0f, // uv 1
			0.5f,
			1.0f // uv 2
		};

		const auto uvs = gltf_loader::extractFloat2UVs( buffer.data(), 3, 0, 8 );

		REQUIRE( uvs.size() == 3 );
		REQUIRE( uvs[0].x == 0.0f );
		REQUIRE( uvs[0].y == 0.0f );
		REQUIRE( uvs[1].x == 1.0f );
		REQUIRE( uvs[1].y == 0.0f );
		REQUIRE( uvs[2].x == 0.5f );
		REQUIRE( uvs[2].y == 1.0f );
	}

	SECTION( "Extract float3 normals" )
	{
		// Test data: normal vectors as float3
		const std::vector<float> buffer = {
			0.0f, 0.0f, 1.0f, // normal 0
			0.0f,
			0.0f,
			1.0f, // normal 1
			0.0f,
			0.0f,
			1.0f // normal 2
		};

		const auto normals = gltf_loader::extractFloat3Normals( buffer.data(), 3, 0, 12 );

		REQUIRE( normals.size() == 3 );
		for ( const auto &normal : normals )
		{
			REQUIRE( normal.x == 0.0f );
			REQUIRE( normal.y == 0.0f );
			REQUIRE( normal.z == 1.0f );
		}
	}

	SECTION( "Extract float4 tangents" )
	{
		// Test data: tangent vectors as float4 (xyz + w handedness)
		const std::vector<float> buffer = {
			1.0f, 0.0f, 0.0f, 1.0f, // tangent 0
			1.0f,
			0.0f,
			0.0f,
			1.0f, // tangent 1
			1.0f,
			0.0f,
			0.0f,
			1.0f // tangent 2
		};

		const auto tangents = gltf_loader::extractFloat4Tangents( buffer.data(), 3, 0, 16 );

		REQUIRE( tangents.size() == 3 );
		for ( const auto &tangent : tangents )
		{
			REQUIRE( tangent.x == 1.0f );
			REQUIRE( tangent.y == 0.0f );
			REQUIRE( tangent.z == 0.0f );
			REQUIRE( tangent.w == 1.0f );
		}
	}
}

TEST_CASE( "GLTF Buffer View Resolution", "[gltf][buffer-view][unit]" )
{
	SECTION( "Resolve accessor to buffer data with stride" )
	{
		// Test interleaved data: position (12 bytes) + normal (12 bytes) = 24 bytes per vertex
		const std::vector<float> buffer = {
			// Vertex 0: position(0,0,0) + normal(0,0,1)
			0.0f,
			0.0f,
			0.0f,
			0.0f,
			0.0f,
			1.0f,
			// Vertex 1: position(1,0,0) + normal(0,0,1)
			1.0f,
			0.0f,
			0.0f,
			0.0f,
			0.0f,
			1.0f,
			// Vertex 2: position(0.5,1,0) + normal(0,0,1)
			0.5f,
			1.0f,
			0.0f,
			0.0f,
			0.0f,
			1.0f
		};

		// Extract positions with stride (skip normals)
		const auto positions = gltf_loader::extractFloat3Positions(
			buffer.data(), 3, 0, 24 // stride = 24 bytes (6 floats)
		);

		REQUIRE( positions.size() == 3 );
		REQUIRE( positions[0].x == 0.0f );
		REQUIRE( positions[1].x == 1.0f );
		REQUIRE( positions[2].x == 0.5f );

		// Extract normals with stride and offset
		const auto normals = gltf_loader::extractFloat3Normals(
			buffer.data(), 3, 12, 24 // offset = 12 bytes, stride = 24 bytes
		);

		REQUIRE( normals.size() == 3 );
		for ( const auto &normal : normals )
		{
			REQUIRE( normal.z == 1.0f );
		}
	}

	SECTION( "Handle component type validation" )
	{
		const std::vector<float> buffer = { 1.0f, 2.0f, 3.0f };

		// Should throw for unsupported component type for positions
		REQUIRE_THROWS_AS(
			gltf_loader::validateComponentType( gltf_loader::ComponentType::UnsignedByte, gltf_loader::AttributeType::Position ),
			std::invalid_argument );

		// Should not throw for valid combinations
		REQUIRE_NOTHROW(
			gltf_loader::validateComponentType( gltf_loader::ComponentType::Float, gltf_loader::AttributeType::Position ) );
		REQUIRE_NOTHROW(
			gltf_loader::validateComponentType( gltf_loader::ComponentType::UnsignedShort, gltf_loader::AttributeType::Indices ) );
	}
}