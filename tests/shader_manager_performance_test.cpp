// Performance test for shader manager hash-based optimization
// This test demonstrates the performance improvement from O(n) to O(1) lookup

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <chrono>
#include <random>
#include <fstream>

#include "engine/shader_manager/shader_manager.h"

using shader_manager::INVALID_SHADER_HANDLE;

TEST_CASE( "shader_manager::ShaderManager Performance - Hash-based lookup optimization", "[shader_manager::ShaderManager][Performance]" )
{
	// Create a temporary directory for test shaders
	auto tempDir = std::filesystem::temp_directory_path() / "shader_manager_perf_tests";
	std::filesystem::create_directories( tempDir );

	// Generate many test shader files
	const int numShaders = 1000;
	std::vector<std::filesystem::path> shaderPaths;
	shaderPaths.reserve( numShaders );

	// Create test shader files
	for ( int i = 0; i < numShaders; ++i )
	{
		const auto shaderPath = tempDir / ( "test_shader_" + std::to_string( i ) + ".hlsl" );
		shaderPaths.push_back( shaderPath );

		// Create a simple vertex shader
		std::ofstream file( shaderPath );
		file << R"(
struct VSInput
{
    float3 position : POSITION;
};

struct VSOutput
{
    float4 position : SV_POSITION;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position = float4(input.position, 1.0f);
    return output;
}
)";
		file.close();
	}

	shader_manager::ShaderManager shaderManager;
	std::vector<shader_manager::ShaderHandle> handles;
	handles.reserve( numShaders );

	// Register all shaders first
	SECTION( "Register many unique shaders" )
	{
		auto start = std::chrono::high_resolution_clock::now();

		for ( int i = 0; i < numShaders; ++i )
		{
			shader_manager::ShaderHandle handle = shaderManager.registerShader(
				shaderPaths[i],
				"VSMain",
				"vs_5_0",
				shader_manager::ShaderType::Vertex );
			handles.push_back( handle );
			REQUIRE( handle != INVALID_SHADER_HANDLE );
		}

		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( end - start );

		INFO( "Registered " << numShaders << " shaders in " << duration.count() << "ms" );
	}

	// Test duplicate detection performance (this is where hash-based lookup shines)
	SECTION( "Test duplicate detection performance with hash-based lookup" )
	{
		// First register all shaders
		for ( int i = 0; i < numShaders; ++i )
		{
			shader_manager::ShaderHandle handle = shaderManager.registerShader(
				shaderPaths[i],
				"VSMain",
				"vs_5_0",
				shader_manager::ShaderType::Vertex );
			handles.push_back( handle );
		}

		// Now test duplicate detection performance
		auto start = std::chrono::high_resolution_clock::now();

		// Try to register the same shaders again (should find existing ones quickly)
		for ( int i = 0; i < numShaders; ++i )
		{
			shader_manager::ShaderHandle duplicateHandle = shaderManager.registerShader(
				shaderPaths[i],
				"VSMain",
				"vs_5_0",
				shader_manager::ShaderType::Vertex );

			// Should return the existing handle
			REQUIRE( duplicateHandle == handles[i] );
		}

		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>( end - start );

		INFO( "Found " << numShaders << " duplicate shaders in " << duration.count() << "us" );
		INFO( "Average time per duplicate lookup: " << ( duration.count() / numShaders ) << "us" );

		// With hash-based lookup, this should be very fast (O(1) per lookup)
		// Without optimization, this would be O(n) per lookup, getting slower as more shaders are registered
		REQUIRE( duration.count() < 50000 ); // Should complete in less than 50ms
	}

	// Test random access performance
	SECTION( "Test random shader lookup performance" )
	{
		// Register all shaders first
		for ( int i = 0; i < numShaders; ++i )
		{
			shader_manager::ShaderHandle handle = shaderManager.registerShader(
				shaderPaths[i],
				"VSMain",
				"vs_5_0",
				shader_manager::ShaderType::Vertex );
			handles.push_back( handle );
		}

		// Create random access pattern
		std::random_device rd;
		std::mt19937 gen( rd() );
		std::uniform_int_distribution<> dis( 0, numShaders - 1 );

		const int numRandomAccess = 10000;
		const auto start = std::chrono::high_resolution_clock::now();

		for ( int i = 0; i < numRandomAccess; ++i )
		{
			const int randomIndex = dis( gen );

			// Try to register the same shader (should use hash lookup)
			const shader_manager::ShaderHandle duplicateHandle = shaderManager.registerShader(
				shaderPaths[randomIndex],
				"VSMain",
				"vs_5_0",
				shader_manager::ShaderType::Vertex );

			REQUIRE( duplicateHandle == handles[randomIndex] );
		}

		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>( end - start );

		INFO( "Performed " << numRandomAccess << " random duplicate lookups in " << duration.count() << "us" );
		INFO( "Average time per random lookup: " << ( duration.count() / numRandomAccess ) << "us" );

		// Should be consistently fast regardless of number of registered shaders
		REQUIRE( duration.count() < 100000 ); // Should complete in less than 100ms
	}

	// Clean up test files
	std::error_code ec;
	std::filesystem::remove_all( tempDir, ec );
}
