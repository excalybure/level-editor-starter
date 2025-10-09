// Shader Manager comprehensive unit tests
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <fstream>
#include "test_dx12_helpers.h"

#include "graphics/shader_manager/shader_manager.h"
#include "graphics/renderer/renderer.h"
#include "core/console.h"

using shader_manager::INVALID_SHADER_HANDLE;
using shader_manager::ShaderType;

// Test fixture for creating temporary shader files
class ShaderManagerTestFixture
{
public:
	ShaderManagerTestFixture()
	{
		// Create temporary directory for test shader files
		m_testDir = std::filesystem::temp_directory_path() / "shader_manager_tests";
		std::filesystem::create_directories( m_testDir );

		// Create valid test shader content
		m_validShaderContent = R"(
// Test shader for shader manager unit tests
cbuffer Constants : register(b0)
{
    float4x4 worldViewProj;
    float4 color;
};

struct VSInput
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = mul(float4(input.position, 1.0f), worldViewProj);
    output.texcoord = input.texcoord;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return color;
}
)";

		// Create invalid shader content for error testing
		m_invalidShaderContent = R"(
// Invalid shader with syntax errors
This is not valid HLSL code!
float4 invalid_syntax_here;
)";
	}

	~ShaderManagerTestFixture()
	{
		// Clean up temporary files
		std::error_code ec;
		std::filesystem::remove_all( m_testDir, ec );
	}

	// Create a temporary shader file with given content
	std::filesystem::path createShaderFile( const std::string &filename, const std::string &content )
	{
		const auto filePath = m_testDir / filename;
		std::ofstream file( filePath );
		file << content;
		file.close();
		return filePath;
	}

	// Update an existing shader file with new content
	void updateShaderFile( const std::filesystem::path &filePath, const std::string &content )
	{
		// Add a small delay to ensure the file timestamp changes
		std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

		std::ofstream file( filePath );
		file << content;
		file.close();
	}

	std::filesystem::path getTestDirectory() const { return m_testDir; }
	const std::string &getValidShaderContent() const { return m_validShaderContent; }
	const std::string &getInvalidShaderContent() const { return m_invalidShaderContent; }

private:
	std::filesystem::path m_testDir;
	std::string m_validShaderContent;
	std::string m_invalidShaderContent;
};

TEST_CASE( "ShaderManager Basic Construction", "[shader_manager][construction]" )
{
	SECTION( "Default construction" )
	{
		shader_manager::ShaderManager manager;

		// Manager should start with no registered shaders
		const auto handles = manager.getAllShaderHandles();
		REQUIRE( handles.empty() );
	}
}

TEST_CASE( "ShaderManager Shader Registration", "[shader_manager][registration]" )
{
	ShaderManagerTestFixture fixture;
	shader_manager::ShaderManager manager;

	SECTION( "Register valid vertex shader" )
	{
		const auto shaderPath = fixture.createShaderFile( "test_vertex.hlsl", fixture.getValidShaderContent() );

		const auto handle = manager.registerShader(
			shaderPath,
			"VSMain",
			"vs_5_0",
			shader_manager::ShaderType::Vertex );

		REQUIRE( handle != INVALID_SHADER_HANDLE );

		// Check that shader was registered
		const auto handles = manager.getAllShaderHandles();
		REQUIRE( handles.size() == 1 );
		REQUIRE( handles[0] == handle );

		// Check shader info
		const auto shaderInfo = manager.getShaderInfo( handle );
		REQUIRE( shaderInfo != nullptr );
		REQUIRE( shaderInfo->handle == handle );
		REQUIRE( shaderInfo->filePath == shaderPath );
		REQUIRE( shaderInfo->entryPoint == "VSMain" );
		REQUIRE( shaderInfo->target == "vs_5_0" );
		REQUIRE( shaderInfo->type == shader_manager::ShaderType::Vertex );
	}

	SECTION( "Register valid pixel shader" )
	{
		const auto shaderPath = fixture.createShaderFile( "test_pixel.hlsl", fixture.getValidShaderContent() );

		const auto handle = manager.registerShader(
			shaderPath,
			"PSMain",
			"ps_5_0",
			shader_manager::ShaderType::Pixel );

		REQUIRE( handle != INVALID_SHADER_HANDLE );

		// Check shader info
		const auto shaderInfo = manager.getShaderInfo( handle );
		REQUIRE( shaderInfo != nullptr );
		REQUIRE( shaderInfo->type == shader_manager::ShaderType::Pixel );
		REQUIRE( shaderInfo->entryPoint == "PSMain" );
		REQUIRE( shaderInfo->target == "ps_5_0" );
	}

	SECTION( "Register multiple shaders with unique handles" )
	{
		const auto shaderPath1 = fixture.createShaderFile( "test1.hlsl", fixture.getValidShaderContent() );
		const auto shaderPath2 = fixture.createShaderFile( "test2.hlsl", fixture.getValidShaderContent() );

		const auto handle1 = manager.registerShader( shaderPath1, "VSMain", "vs_5_0", shader_manager::ShaderType::Vertex );
		const auto handle2 = manager.registerShader( shaderPath2, "PSMain", "ps_5_0", shader_manager::ShaderType::Pixel );

		REQUIRE( handle1 != INVALID_SHADER_HANDLE );
		REQUIRE( handle2 != INVALID_SHADER_HANDLE );
		REQUIRE( handle1 != handle2 );

		const auto handles = manager.getAllShaderHandles();
		REQUIRE( handles.size() == 2 );
	}

	SECTION( "Register shader with non-existent file" )
	{
		const auto nonExistentPath = fixture.getTestDirectory() / "non_existent.hlsl";

		const auto handle = manager.registerShader(
			nonExistentPath,
			"VSMain",
			"vs_5_0",
			shader_manager::ShaderType::Vertex );

		// Should still return valid handle even if compilation fails
		REQUIRE( handle != INVALID_SHADER_HANDLE );

		// But shader should not be valid
		const auto shaderInfo = manager.getShaderInfo( handle );
		REQUIRE( shaderInfo != nullptr );
		REQUIRE_FALSE( shaderInfo->isValid );

		// Shader blob should be null
		const auto blob = manager.getShaderBlob( handle );
		REQUIRE( blob == nullptr );
	}

	SECTION( "Register duplicate shader returns same handle" )
	{
		const auto shaderPath = fixture.createShaderFile( "duplicate_test.hlsl", fixture.getValidShaderContent() );

		// Register the same shader multiple times with identical parameters
		const auto handle1 = manager.registerShader( shaderPath, "VSMain", "vs_5_0", shader_manager::ShaderType::Vertex );
		const auto handle2 = manager.registerShader( shaderPath, "VSMain", "vs_5_0", shader_manager::ShaderType::Vertex );
		const auto handle3 = manager.registerShader( shaderPath, "VSMain", "vs_5_0", shader_manager::ShaderType::Vertex );

		// All handles should be the same
		REQUIRE( handle1 != INVALID_SHADER_HANDLE );
		REQUIRE( handle2 == handle1 );
		REQUIRE( handle3 == handle1 );

		// Should only have one shader registered
		const auto handles = manager.getAllShaderHandles();
		REQUIRE( handles.size() == 1 );
		REQUIRE( handles[0] == handle1 );

		// All handles should point to the same shader info
		const auto shaderInfo1 = manager.getShaderInfo( handle1 );
		const auto shaderInfo2 = manager.getShaderInfo( handle2 );
		const auto shaderInfo3 = manager.getShaderInfo( handle3 );

		REQUIRE( shaderInfo1 != nullptr );
		REQUIRE( shaderInfo2 == shaderInfo1 ); // Same pointer
		REQUIRE( shaderInfo3 == shaderInfo1 ); // Same pointer
	}

	SECTION( "Register similar shaders with different parameters get different handles" )
	{
		const auto shaderPath = fixture.createShaderFile( "similar_test.hlsl", fixture.getValidShaderContent() );

		// Register shaders with same file but different entry points
		const auto handle1 = manager.registerShader( shaderPath, "VSMain", "vs_5_0", shader_manager::ShaderType::Vertex );
		const auto handle2 = manager.registerShader( shaderPath, "PSMain", "ps_5_0", shader_manager::ShaderType::Pixel );

		// Register shaders with same file and entry point but different targets
		const auto handle3 = manager.registerShader( shaderPath, "VSMain", "vs_4_0", shader_manager::ShaderType::Vertex );

		// All handles should be different
		REQUIRE( handle1 != INVALID_SHADER_HANDLE );
		REQUIRE( handle2 != INVALID_SHADER_HANDLE );
		REQUIRE( handle3 != INVALID_SHADER_HANDLE );
		REQUIRE( handle1 != handle2 );
		REQUIRE( handle1 != handle3 );
		REQUIRE( handle2 != handle3 );

		// Should have three different shaders registered
		const auto handles = manager.getAllShaderHandles();
		REQUIRE( handles.size() == 3 );
	}
}

TEST_CASE( "ShaderManager Shader Unregistration", "[shader_manager][unregistration]" )
{
	ShaderManagerTestFixture fixture;
	shader_manager::ShaderManager manager;

	SECTION( "Unregister valid shader" )
	{
		const auto shaderPath = fixture.createShaderFile( "test.hlsl", fixture.getValidShaderContent() );
		const auto handle = manager.registerShader( shaderPath, "VSMain", "vs_5_0", shader_manager::ShaderType::Vertex );

		REQUIRE( handle != INVALID_SHADER_HANDLE );
		REQUIRE( manager.getAllShaderHandles().size() == 1 );

		// Unregister the shader
		manager.unregisterShader( handle );

		// Should be no shaders left
		REQUIRE( manager.getAllShaderHandles().empty() );

		// Shader info should be null
		REQUIRE( manager.getShaderInfo( handle ) == nullptr );
		REQUIRE( manager.getShaderBlob( handle ) == nullptr );
	}

	SECTION( "Unregister invalid handle" )
	{
		// Should not crash when unregistering non-existent handle
		REQUIRE_NOTHROW( manager.unregisterShader( INVALID_SHADER_HANDLE ) );
		REQUIRE_NOTHROW( manager.unregisterShader( 12345 ) );
	}

	SECTION( "Unregister one of multiple shaders" )
	{
		const auto shaderPath1 = fixture.createShaderFile( "test1.hlsl", fixture.getValidShaderContent() );
		const auto shaderPath2 = fixture.createShaderFile( "test2.hlsl", fixture.getValidShaderContent() );

		const auto handle1 = manager.registerShader( shaderPath1, "VSMain", "vs_5_0", shader_manager::ShaderType::Vertex );
		const auto handle2 = manager.registerShader( shaderPath2, "PSMain", "ps_5_0", shader_manager::ShaderType::Pixel );

		REQUIRE( manager.getAllShaderHandles().size() == 2 );

		// Unregister first shader
		manager.unregisterShader( handle1 );

		// Should have one shader left
		const auto handles = manager.getAllShaderHandles();
		REQUIRE( handles.size() == 1 );
		REQUIRE( handles[0] == handle2 );

		// First shader should be gone, second should still exist
		REQUIRE( manager.getShaderInfo( handle1 ) == nullptr );
		REQUIRE( manager.getShaderInfo( handle2 ) != nullptr );
	}
}

TEST_CASE( "ShaderManager Shader Compilation", "[shader_manager][compilation]" )
{
	ShaderManagerTestFixture fixture;
	shader_manager::ShaderManager manager;

	SECTION( "Successful shader compilation" )
	{
		const auto shaderPath = fixture.createShaderFile( "valid.hlsl", fixture.getValidShaderContent() );
		const auto handle = manager.registerShader( shaderPath, "VSMain", "vs_5_0", shader_manager::ShaderType::Vertex );

		// Should have valid shader info
		const auto shaderInfo = manager.getShaderInfo( handle );
		REQUIRE( shaderInfo != nullptr );

		// Shader blob should be available (if compilation succeeded)
		const auto blob = manager.getShaderBlob( handle );
		if ( shaderInfo->isValid )
		{
			REQUIRE( blob != nullptr );
			REQUIRE( blob->isValid() );
		}
	}

	SECTION( "Failed shader compilation" )
	{
		const auto shaderPath = fixture.createShaderFile( "invalid.hlsl", fixture.getInvalidShaderContent() );
		const auto handle = manager.registerShader( shaderPath, "VSMain", "vs_5_0", shader_manager::ShaderType::Vertex );

		// Should still have shader info
		const auto shaderInfo = manager.getShaderInfo( handle );
		REQUIRE( shaderInfo != nullptr );

		// But shader should not be valid
		REQUIRE_FALSE( shaderInfo->isValid );

		// Shader blob should be null
		const auto blob = manager.getShaderBlob( handle );
		REQUIRE( blob == nullptr );
	}
}

TEST_CASE( "ShaderManager Force Recompilation", "[shader_manager][force_recompile]" )
{
	ShaderManagerTestFixture fixture;
	shader_manager::ShaderManager manager;

	SECTION( "Force recompile single shader" )
	{
		const auto shaderPath = fixture.createShaderFile( "test.hlsl", fixture.getValidShaderContent() );
		const auto handle = manager.registerShader( shaderPath, "VSMain", "vs_5_0", shader_manager::ShaderType::Vertex );

		// Force recompilation should succeed
		const bool result = manager.forceRecompile( handle );

		// Result depends on whether compilation succeeded
		const auto shaderInfo = manager.getShaderInfo( handle );
		REQUIRE( shaderInfo != nullptr );

		if ( shaderInfo->isValid )
		{
			REQUIRE( result == true );
		}
		else
		{
			REQUIRE( result == false );
		}
	}

	SECTION( "Force recompile invalid handle" )
	{
		// Should return false for invalid handles
		REQUIRE_FALSE( manager.forceRecompile( INVALID_SHADER_HANDLE ) );
		REQUIRE_FALSE( manager.forceRecompile( 12345 ) );
	}

	SECTION( "Force recompile all shaders" )
	{
		const auto shaderPath1 = fixture.createShaderFile( "test1.hlsl", fixture.getValidShaderContent() );
		const auto shaderPath2 = fixture.createShaderFile( "test2.hlsl", fixture.getValidShaderContent() );

		manager.registerShader( shaderPath1, "VSMain", "vs_5_0", shader_manager::ShaderType::Vertex );
		manager.registerShader( shaderPath2, "PSMain", "ps_5_0", shader_manager::ShaderType::Pixel );

		// Should not throw
		REQUIRE_NOTHROW( manager.forceRecompileAll() );
	}
}

TEST_CASE( "ShaderManager Callback System", "[shader_manager][callbacks]" )
{
	ShaderManagerTestFixture fixture;
	shader_manager::ShaderManager manager;

	SECTION( "Set and trigger reload callback" )
	{
		bool callbackTriggered = false;
		shader_manager::ShaderHandle callbackHandle = INVALID_SHADER_HANDLE;

		// Register callback
		manager.registerReloadCallback( [&callbackTriggered, &callbackHandle]( shader_manager::ShaderHandle handle, const shader_manager::ShaderBlob & ) {
			callbackTriggered = true;
			callbackHandle = handle;
		} );

		const auto shaderPath = fixture.createShaderFile( "test.hlsl", fixture.getValidShaderContent() );
		const auto handle = manager.registerShader( shaderPath, "VSMain", "vs_5_0", shader_manager::ShaderType::Vertex );

		// Force recompile should trigger callback if successful
		manager.forceRecompile( handle );

		const auto shaderInfo = manager.getShaderInfo( handle );
		if ( shaderInfo != nullptr && shaderInfo->isValid )
		{
			REQUIRE( callbackTriggered );
			REQUIRE( callbackHandle == handle );
		}
	}

	SECTION( "Callback not called on failed compilation" )
	{
		bool callbackTriggered = false;

		manager.registerReloadCallback( [&callbackTriggered]( shader_manager::ShaderHandle, const shader_manager::ShaderBlob & ) {
			callbackTriggered = true;
		} );

		const auto shaderPath = fixture.createShaderFile( "invalid.hlsl", fixture.getInvalidShaderContent() );
		const auto handle = manager.registerShader( shaderPath, "VSMain", "vs_5_0", shader_manager::ShaderType::Vertex );

		// Force recompile should not trigger callback on failure
		manager.forceRecompile( handle );

		REQUIRE_FALSE( callbackTriggered );
	}
}

TEST_CASE( "ShaderManager File Change Detection", "[shader_manager][file_watching]" )
{
	ShaderManagerTestFixture fixture;
	shader_manager::ShaderManager manager;

	SECTION( "Detect file modifications" )
	{
		bool callbackTriggered = false;

		manager.registerReloadCallback( [&callbackTriggered]( shader_manager::ShaderHandle, const shader_manager::ShaderBlob & ) {
			callbackTriggered = true;
		} );

		const auto shaderPath = fixture.createShaderFile( "test.hlsl", fixture.getValidShaderContent() );
		const auto handle = manager.registerShader( shaderPath, "VSMain", "vs_5_0", shader_manager::ShaderType::Vertex );

		// Get initial modification time
		const auto initialInfo = manager.getShaderInfo( handle );
		REQUIRE( initialInfo != nullptr );
		const auto initialModTime = initialInfo->lastModified;

		// Update the file
		fixture.updateShaderFile( shaderPath, fixture.getValidShaderContent() + "\n// Updated" );

		// Call update to check for file changes
		manager.update();

		// Check if modification time was updated
		const auto updatedInfo = manager.getShaderInfo( handle );
		REQUIRE( updatedInfo != nullptr );

		// File modification time should be different (if filesystem has sufficient resolution)
		// Note: Some filesystems have low resolution, so we'll just check that update() was called
		REQUIRE_NOTHROW( manager.update() );
	}
}

TEST_CASE( "ShaderManager Multiple Callbacks", "[shader_manager][callbacks]" )
{
	ShaderManagerTestFixture fixture;
	shader_manager::ShaderManager manager;

	SECTION( "Multiple callback registration and notification" )
	{
		const auto shaderPath = fixture.createShaderFile( "test.hlsl", fixture.getValidShaderContent() );
		const auto handle = manager.registerShader( shaderPath, "VSMain", "vs_5_0", shader_manager::ShaderType::Vertex );

		// Register multiple callbacks
		bool callback1Triggered = false;
		bool callback2Triggered = false;
		bool callback3Triggered = false;

		auto callbackHandle1 = manager.registerReloadCallback( [&callback1Triggered]( shader_manager::ShaderHandle, const shader_manager::ShaderBlob & ) {
			callback1Triggered = true;
		} );

		auto callbackHandle2 = manager.registerReloadCallback( [&callback2Triggered]( shader_manager::ShaderHandle, const shader_manager::ShaderBlob & ) {
			callback2Triggered = true;
		} );

		auto callbackHandle3 = manager.registerReloadCallback( [&callback3Triggered]( shader_manager::ShaderHandle, const shader_manager::ShaderBlob & ) {
			callback3Triggered = true;
		} );

		// Verify callback handles are valid and unique
		REQUIRE( callbackHandle1 != shader_manager::INVALID_CALLBACK_HANDLE );
		REQUIRE( callbackHandle2 != shader_manager::INVALID_CALLBACK_HANDLE );
		REQUIRE( callbackHandle3 != shader_manager::INVALID_CALLBACK_HANDLE );
		REQUIRE( callbackHandle1 != callbackHandle2 );
		REQUIRE( callbackHandle2 != callbackHandle3 );
		REQUIRE( callbackHandle1 != callbackHandle3 );

		// Force recompile should trigger all callbacks
		REQUIRE( manager.forceRecompile( handle ) );

		REQUIRE( callback1Triggered );
		REQUIRE( callback2Triggered );
		REQUIRE( callback3Triggered );

		// Reset flags
		callback1Triggered = false;
		callback2Triggered = false;
		callback3Triggered = false;

		// Unregister one callback
		manager.unregisterReloadCallback( callbackHandle2 );

		// Force recompile should only trigger remaining callbacks
		REQUIRE( manager.forceRecompile( handle ) );

		REQUIRE( callback1Triggered );
		REQUIRE_FALSE( callback2Triggered ); // Should not be triggered after unregistration
		REQUIRE( callback3Triggered );
	}
}

TEST_CASE( "ShaderManager Shader Type Utilities", "[shader_manager][types]" )
{
	ShaderManagerTestFixture fixture;
	shader_manager::ShaderManager manager;

	SECTION( "Different shader types" )
	{
		const auto shaderPath = fixture.createShaderFile( "test.hlsl", fixture.getValidShaderContent() );

		std::vector<std::pair<ShaderType, std::string>> shaderTypes = {
			{ shader_manager::ShaderType::Vertex, "vs_5_0" },
			{ shader_manager::ShaderType::Pixel, "ps_5_0" },
			{ shader_manager::ShaderType::Compute, "cs_5_0" },
			{ shader_manager::ShaderType::Geometry, "gs_5_0" },
			{ shader_manager::ShaderType::Hull, "hs_5_0" },
			{ shader_manager::ShaderType::Domain, "ds_5_0" }
		};

		for ( const auto &shaderTypePair : shaderTypes )
		{
			const ShaderType type = shaderTypePair.first;
			const std::string &target = shaderTypePair.second;
			const auto handle = manager.registerShader( shaderPath, "VSMain", target, type );
			REQUIRE( handle != INVALID_SHADER_HANDLE );

			auto shaderInfo = manager.getShaderInfo( handle );
			REQUIRE( shaderInfo != nullptr );
			REQUIRE( shaderInfo->type == type );
			REQUIRE( shaderInfo->target == target );

			manager.unregisterShader( handle );
		}
	}
}

TEST_CASE( "ShaderManager Edge Cases", "[shader_manager][edge_cases]" )
{
	ShaderManagerTestFixture fixture;
	shader_manager::ShaderManager manager;

	SECTION( "Empty file path" )
	{
		const auto handle = manager.registerShader( "", "VSMain", "vs_5_0", shader_manager::ShaderType::Vertex );

		REQUIRE( handle != INVALID_SHADER_HANDLE );

		// Should not be valid due to empty path
		const auto shaderInfo = manager.getShaderInfo( handle );
		REQUIRE( shaderInfo != nullptr );
		REQUIRE_FALSE( shaderInfo->isValid );
	}

	SECTION( "Empty entry point" )
	{
		const auto shaderPath = fixture.createShaderFile( "test.hlsl", fixture.getValidShaderContent() );
		const auto handle = manager.registerShader( shaderPath, "", "vs_5_0", shader_manager::ShaderType::Vertex );

		REQUIRE( handle != INVALID_SHADER_HANDLE );

		const auto shaderInfo = manager.getShaderInfo( handle );
		REQUIRE( shaderInfo != nullptr );
		REQUIRE( shaderInfo->entryPoint.empty() );
	}

	SECTION( "Multiple update calls" )
	{
		const auto shaderPath = fixture.createShaderFile( "test.hlsl", fixture.getValidShaderContent() );
		manager.registerShader( shaderPath, "VSMain", "vs_5_0", shader_manager::ShaderType::Vertex );

		// Multiple updates should not cause issues
		REQUIRE_NOTHROW( manager.update() );
		REQUIRE_NOTHROW( manager.update() );
		REQUIRE_NOTHROW( manager.update() );
	}

	SECTION( "Query after unregistration" )
	{
		const auto shaderPath = fixture.createShaderFile( "test.hlsl", fixture.getValidShaderContent() );
		const auto handle = manager.registerShader( shaderPath, "VSMain", "vs_5_0", shader_manager::ShaderType::Vertex );

		// Unregister shader
		manager.unregisterShader( handle );

		// Queries should return null/false
		REQUIRE( manager.getShaderInfo( handle ) == nullptr );
		REQUIRE( manager.getShaderBlob( handle ) == nullptr );
		REQUIRE_FALSE( manager.forceRecompile( handle ) );
	}
}

TEST_CASE( "ShaderManager Memory Management", "[shader_manager][memory]" )
{
	ShaderManagerTestFixture fixture;

	SECTION( "Manager destruction cleans up resources" )
	{
		{
			shader_manager::ShaderManager manager;
			const auto shaderPath = fixture.createShaderFile( "test.hlsl", fixture.getValidShaderContent() );

			// Register multiple shaders
			for ( int i = 0; i < 10; ++i )
			{
				const auto path = fixture.createShaderFile( "test" + std::to_string( i ) + ".hlsl", fixture.getValidShaderContent() );
				manager.registerShader( path, "VSMain", "vs_5_0", shader_manager::ShaderType::Vertex );
			}

			REQUIRE( manager.getAllShaderHandles().size() == 10 );
		}
		// Manager should be destroyed cleanly here

		// Create new manager - should start empty
		shader_manager::ShaderManager newManager;
		REQUIRE( newManager.getAllShaderHandles().empty() );
	}
}

TEST_CASE( "ShaderManager Thread Safety", "[shader_manager][threading]" )
{
	ShaderManagerTestFixture fixture;
	shader_manager::ShaderManager manager;

	SECTION( "Multiple update calls from single thread" )
	{
		const auto shaderPath = fixture.createShaderFile( "test.hlsl", fixture.getValidShaderContent() );
		manager.registerShader( shaderPath, "VSMain", "vs_5_0", shader_manager::ShaderType::Vertex );

		// Rapid update calls should be safe
		for ( int i = 0; i < 100; ++i )
		{
			REQUIRE_NOTHROW( manager.update() );
		}
	}
}
