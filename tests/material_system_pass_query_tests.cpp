#include <catch2/catch_test_macros.hpp>
#include "graphics/material_system/material_system.h"
#include "graphics/material_system/parser.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

using namespace graphics::material_system;
using json = nlohmann::json;
namespace fs = std::filesystem;

// ============================================================================
// T304: MaterialSystem Pass Query Tests
// ============================================================================

TEST_CASE( "MaterialSystem::getMaterialPass returns MaterialPass for valid material and pass", "[material-system][T304][unit]" )
{
	// Arrange - Create temp JSON with multi-pass material
	const fs::path tempDir = fs::temp_directory_path() / "material_system_test_T304_getMaterialPass";
	fs::create_directories( tempDir );
	const fs::path jsonPath = tempDir / "materials.json";

	{
		const json materialsJson = {
			{ "materials",
				json::array( {
					{
						{ "id", "multipass_material" },
						{ "passes",
							json::array( {
								{
									{ "name", "depth_prepass" },
									{ "shaders",
										{
											{ "vertex", { { "file", "shaders/unlit.hlsl" }, { "profile", "vs_5_1" }, { "entry", "VSMain" } } },
										} },
									{ "primitiveTopology", "Triangle" },
								},
								{
									{ "name", "forward" },
									{ "shaders",
										{
											{ "vertex", { { "file", "shaders/simple.hlsl" }, { "profile", "vs_5_1" }, { "entry", "VSMain" } } },
											{ "pixel", { { "file", "shaders/simple.hlsl" }, { "profile", "ps_5_1" }, { "entry", "PSMain" } } },
										} },
									{ "primitiveTopology", "Triangle" },
								},
							} ) },
					},
				} ) },
			{ "renderPasses", json::array() },
		};

		std::ofstream( jsonPath ) << materialsJson.dump();
	}

	MaterialSystem materialSystem;
	REQUIRE( materialSystem.initialize( jsonPath.string(), nullptr ) );

	const auto handle = materialSystem.getMaterialHandle( "multipass_material" );
	REQUIRE( handle.isValid() );

	// Act - Query specific pass from material
	const auto *depthPass = materialSystem.getMaterialPass( handle, "depth_prepass" );
	const auto *forwardPass = materialSystem.getMaterialPass( handle, "forward" );

	// Assert - Both passes should be found
	REQUIRE( depthPass != nullptr );
	REQUIRE( forwardPass != nullptr );
	REQUIRE( depthPass->passName == "depth_prepass" );
	REQUIRE( forwardPass->passName == "forward" );

	// Cleanup
	fs::remove_all( tempDir );
}

TEST_CASE( "MaterialSystem::getMaterialPass returns nullptr for invalid pass name", "[material-system][T304][unit]" )
{
	// Arrange
	const fs::path tempDir = fs::temp_directory_path() / "material_system_test_T304_invalid_pass";
	fs::create_directories( tempDir );
	const fs::path jsonPath = tempDir / "materials.json";

	{
		const json materialsJson = {
			{ "materials",
				json::array( {
					{
						{ "id", "single_pass_material" },
						{ "passes",
							json::array( {
								{
									{ "name", "forward" },
									{ "shaders",
										{
											{ "vertex", { { "file", "shaders/simple.hlsl" }, { "profile", "vs_5_1" }, { "entry", "VSMain" } } },
										} },
								},
							} ) },
					},
				} ) },
			{ "renderPasses", json::array() },
		};

		std::ofstream( jsonPath ) << materialsJson.dump();
	}

	MaterialSystem materialSystem;
	REQUIRE( materialSystem.initialize( jsonPath.string(), nullptr ) );

	const auto handle = materialSystem.getMaterialHandle( "single_pass_material" );
	REQUIRE( handle.isValid() );

	// Act - Query non-existent pass
	const auto *shadowPass = materialSystem.getMaterialPass( handle, "shadow" );

	// Assert - Should return nullptr
	REQUIRE( shadowPass == nullptr );

	// Cleanup
	fs::remove_all( tempDir );
}

TEST_CASE( "MaterialSystem::getMaterialPass returns nullptr for invalid handle", "[material-system][T304][unit]" )
{
	// Arrange
	const fs::path tempDir = fs::temp_directory_path() / "material_system_test_T304_invalid_handle";
	fs::create_directories( tempDir );
	const fs::path jsonPath = tempDir / "materials.json";

	{
		const json materialsJson = {
			{ "materials", json::array() },
			{ "renderPasses", json::array() },
		};

		std::ofstream( jsonPath ) << materialsJson.dump();
	}

	MaterialSystem materialSystem;
	REQUIRE( materialSystem.initialize( jsonPath.string(), nullptr ) );

	// Create invalid handle
	MaterialHandle invalidHandle;
	REQUIRE_FALSE( invalidHandle.isValid() );

	// Act - Query with invalid handle
	const auto *pass = materialSystem.getMaterialPass( invalidHandle, "forward" );

	// Assert - Should return nullptr
	REQUIRE( pass == nullptr );

	// Cleanup
	fs::remove_all( tempDir );
}

TEST_CASE( "MaterialSystem::hasMaterialPass checks pass existence", "[material-system][T304][unit]" )
{
	// Arrange
	const fs::path tempDir = fs::temp_directory_path() / "material_system_test_T304_hasMaterialPass";
	fs::create_directories( tempDir );
	const fs::path jsonPath = tempDir / "materials.json";

	{
		const json materialsJson = {
			{ "materials",
				json::array( {
					{
						{ "id", "test_material" },
						{ "passes",
							json::array( {
								{
									{ "name", "forward" },
									{ "shaders",
										{
											{ "vertex", { { "file", "shaders/simple.hlsl" }, { "profile", "vs_5_1" }, { "entry", "VSMain" } } },
										} },
								},
								{
									{ "name", "wireframe" },
									{ "shaders",
										{
											{ "vertex", { { "file", "shaders/simple.hlsl" }, { "profile", "vs_5_1" }, { "entry", "VSMain" } } },
										} },
									{ "primitiveTopology", "Line" },
								},
							} ) },
					},
				} ) },
			{ "renderPasses", json::array() },
		};

		std::ofstream( jsonPath ) << materialsJson.dump();
	}

	MaterialSystem materialSystem;
	REQUIRE( materialSystem.initialize( jsonPath.string(), nullptr ) );

	const auto handle = materialSystem.getMaterialHandle( "test_material" );
	REQUIRE( handle.isValid() );

	// Act & Assert - Check existing passes
	REQUIRE( materialSystem.hasMaterialPass( handle, "forward" ) );
	REQUIRE( materialSystem.hasMaterialPass( handle, "wireframe" ) );

	// Act & Assert - Check non-existent passes
	REQUIRE_FALSE( materialSystem.hasMaterialPass( handle, "shadow" ) );
	REQUIRE_FALSE( materialSystem.hasMaterialPass( handle, "depth_prepass" ) );

	// Act & Assert - Invalid handle returns false
	MaterialHandle invalidHandle;
	REQUIRE_FALSE( materialSystem.hasMaterialPass( invalidHandle, "forward" ) );

	// Cleanup
	fs::remove_all( tempDir );
}
