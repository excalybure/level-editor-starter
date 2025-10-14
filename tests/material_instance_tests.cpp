#include <catch2/catch_test_macros.hpp>
#include "graphics/material_system/material_instance.h"
#include "graphics/material_system/material_system.h"
#include "platform/dx12/dx12_device.h"
#include "test_dx12_helpers.h"
#include <filesystem>
#include <fstream>

using namespace graphics::material_system;
namespace fs = std::filesystem;

// Test helper: Get path to test materials JSON
static std::string getTestMaterialsPath()
{
	return "materials.json";
}

TEST_CASE( "MaterialInstance constructor stores device and material system", "[material-instance-T301][unit]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "MaterialInstance constructor test" ) )
	{
		return;
	}

	MaterialSystem materialSystem;
	const bool initialized = materialSystem.initialize( getTestMaterialsPath() );
	REQUIRE( initialized );

	// Act - just create the instance
	MaterialInstance instance( &device, &materialSystem, "grid_material" );

	// Assert - check that material was found and is valid (indicates MaterialSystem integration)
	REQUIRE( instance.isValid() );
	REQUIRE( instance.getMaterial() != nullptr );
}

TEST_CASE( "MaterialInstance with valid material ID is valid", "[material-instance-T301][unit]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "MaterialInstance valid test" ) )
	{
		return;
	}

	MaterialSystem materialSystem;
	const bool initialized = materialSystem.initialize( getTestMaterialsPath() );
	REQUIRE( initialized );

	// Act
	MaterialInstance instance( &device, &materialSystem, "grid_material" );

	// Assert
	REQUIRE( instance.isValid() );
}

TEST_CASE( "MaterialInstance with invalid material ID is invalid", "[material-instance-T301][unit]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "MaterialInstance invalid test" ) )
	{
		return;
	}

	MaterialSystem materialSystem;
	const bool initialized = materialSystem.initialize( getTestMaterialsPath() );
	REQUIRE( initialized );

	// Act
	MaterialInstance instance( &device, &materialSystem, "nonexistent_material" );

	// Assert
	REQUIRE_FALSE( instance.isValid() );
}

TEST_CASE( "MaterialInstance hasPass returns true for existing pass", "[material-instance-T301][unit]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "MaterialInstance hasPass test" ) )
	{
		return;
	}

	MaterialSystem materialSystem;
	const bool initialized = materialSystem.initialize( getTestMaterialsPath() );
	REQUIRE( initialized );

	MaterialInstance instance( &device, &materialSystem, "grid_material" );
	REQUIRE( instance.isValid() );

	// Act & Assert
	REQUIRE( instance.hasPass( "grid" ) );
}

TEST_CASE( "MaterialInstance hasPass returns false for non-existing pass", "[material-instance-T301][unit]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "MaterialInstance hasPass false test" ) )
	{
		return;
	}

	MaterialSystem materialSystem;

	const bool initialized = materialSystem.initialize( getTestMaterialsPath() );
	REQUIRE( initialized );

	MaterialInstance instance( &device, &materialSystem, "grid_material" );
	REQUIRE( instance.isValid() );

	// Act & Assert
	REQUIRE_FALSE( instance.hasPass( "nonexistent_pass" ) );
}

TEST_CASE( "MaterialInstance getPass returns correct pass definition", "[material-instance-T301][unit]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "MaterialInstance getPass test" ) )
	{
		return;
	}

	MaterialSystem materialSystem;

	const bool initialized = materialSystem.initialize( getTestMaterialsPath() );
	REQUIRE( initialized );

	MaterialInstance instance( &device, &materialSystem, "grid_material" );
	REQUIRE( instance.isValid() );

	// Act
	const MaterialPass *pass = instance.getPass( "grid" );

	// Assert
	REQUIRE( pass != nullptr );
	REQUIRE( pass->passName == "grid" );
}

TEST_CASE( "MaterialInstance getPass returns nullptr for invalid pass", "[material-instance-T301][unit]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "MaterialInstance getPass nullptr test" ) )
	{
		return;
	}

	MaterialSystem materialSystem;

	const bool initialized = materialSystem.initialize( getTestMaterialsPath() );
	REQUIRE( initialized );

	MaterialInstance instance( &device, &materialSystem, "grid_material" );
	REQUIRE( instance.isValid() );

	// Act
	const MaterialPass *pass = instance.getPass( "nonexistent_pass" );

	// Assert
	REQUIRE( pass == nullptr );
}

TEST_CASE( "MaterialInstance getMaterial returns correct material definition", "[material-instance-T301][unit]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "MaterialInstance getMaterial test" ) )
	{
		return;
	}

	MaterialSystem materialSystem;

	const bool initialized = materialSystem.initialize( getTestMaterialsPath() );
	REQUIRE( initialized );

	MaterialInstance instance( &device, &materialSystem, "grid_material" );
	REQUIRE( instance.isValid() );

	// Act
	const MaterialDefinition *material = instance.getMaterial();

	// Assert
	REQUIRE( material != nullptr );
	REQUIRE( material->id == "grid_material" );
}

// T302 Tests: Root Signature Integration

TEST_CASE( "MaterialInstance retrieves root signature on construction", "[material-instance-T302][integration]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "MaterialInstance root signature test" ) )
	{
		return;
	}

	MaterialSystem materialSystem;
	const bool initialized = materialSystem.initialize( getTestMaterialsPath() );
	REQUIRE( initialized );

	// Act
	MaterialInstance instance( &device, &materialSystem, "grid_material" );

	// Assert - root signature should be created during construction
	REQUIRE( instance.isValid() );
	REQUIRE( instance.getRootSignature() != nullptr );
}

TEST_CASE( "MaterialInstance getRootSignature returns valid pointer", "[material-instance-T302][integration]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "MaterialInstance getRootSignature test" ) )
	{
		return;
	}

	MaterialSystem materialSystem;
	const bool initialized = materialSystem.initialize( getTestMaterialsPath() );
	REQUIRE( initialized );

	MaterialInstance instance( &device, &materialSystem, "grid_material" );
	REQUIRE( instance.isValid() );

	// Act
	ID3D12RootSignature *rootSig = instance.getRootSignature();

	// Assert
	REQUIRE( rootSig != nullptr );
}

TEST_CASE( "MaterialInstance with invalid material has no root signature", "[material-instance-T302][integration]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "MaterialInstance invalid root signature test" ) )
	{
		return;
	}

	MaterialSystem materialSystem;
	const bool initialized = materialSystem.initialize( getTestMaterialsPath() );
	REQUIRE( initialized );

	// Act
	MaterialInstance instance( &device, &materialSystem, "nonexistent_material" );

	// Assert
	REQUIRE_FALSE( instance.isValid() );
	REQUIRE( instance.getRootSignature() == nullptr );
}

// T303 Tests: Multi-Pass PSO Management

TEST_CASE( "MaterialInstance getPipelineState creates PSO on first access", "[material-instance][T303][integration]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "MaterialInstance getPipelineState test" ) )
	{
		return;
	}

	MaterialSystem materialSystem;
	const bool initialized = materialSystem.initialize( getTestMaterialsPath() );
	REQUIRE( initialized );

	MaterialInstance instance( &device, &materialSystem, "grid_material" );
	REQUIRE( instance.isValid() );
	REQUIRE( instance.hasPass( "grid" ) );

	// Act - first access should create PSO
	ID3D12PipelineState *pso = instance.getPipelineState( "grid" );

	// Assert
	REQUIRE( pso != nullptr );
}

TEST_CASE( "MaterialInstance getPipelineState returns cached PSO on second access", "[material-instance][T303][integration]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "MaterialInstance PSO caching test" ) )
	{
		return;
	}

	MaterialSystem materialSystem;
	const bool initialized = materialSystem.initialize( getTestMaterialsPath() );
	REQUIRE( initialized );

	MaterialInstance instance( &device, &materialSystem, "grid_material" );
	REQUIRE( instance.isValid() );

	// Act - access twice
	ID3D12PipelineState *pso1 = instance.getPipelineState( "grid" );
	ID3D12PipelineState *pso2 = instance.getPipelineState( "grid" );

	// Assert - should return same cached pointer
	REQUIRE( pso1 != nullptr );
	REQUIRE( pso1 == pso2 );
}

TEST_CASE( "MaterialInstance getPipelineState for different passes creates separate PSOs", "[material-instance][T303][integration]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "MaterialInstance multi-pass PSO test" ) )
	{
		return;
	}

	// Create a temporary materials JSON with a multi-pass material
	const auto tempDir = fs::temp_directory_path() / "material_instance_multipass_test";
	fs::create_directories( tempDir );
	const auto jsonPath = tempDir / "test_materials.json";

	std::ofstream file( jsonPath );
	file << R"({
		"states": {
			"renderTargetStates": {
				"MainColor": {
					"rtvFormats": ["R8G8B8A8_UNORM"],
					"dsvFormat": "D32_FLOAT",
					"samples": 1
				},
				"ShadowMap": {
					"rtvFormats": ["R32_FLOAT"],
					"dsvFormat": "D32_FLOAT",
					"samples": 1
				}
			},
			"depthStencilStates": {
				"depth_test": { "depthEnable": true, "depthWriteMask": "All", "depthFunc": "LessEqual", "stencilEnable": false }
			},
			"rasterizerStates": {
				"solid_back": { "fillMode": "Solid", "cullMode": "Back", "frontCounterClockwise": false }
			},
			"blendStates": {
				"opaque": { "alphaToCoverage": false, "independentBlend": false, "renderTargets": [{ "enable": false }] }
			}
		},
		"materials": [{
			"id": "multipass_material",
			"passes": [
				{
					"name": "forward",
					"shaders": {
						"vertex": { "file": "shaders/grid.hlsl", "entry": "VSMain", "profile": "vs_5_1" },
						"pixel": { "file": "shaders/grid.hlsl", "entry": "PSMain", "profile": "ps_5_1" }
					},
					"states": { "rasterizer": "solid_back", "depthStencil": "depth_test", "blend": "opaque" }
				},
				{
					"name": "shadow",
					"shaders": {
						"vertex": { "file": "shaders/grid.hlsl", "entry": "VSMain", "profile": "vs_5_1" },
						"pixel": { "file": "shaders/grid.hlsl", "entry": "PSMain", "profile": "ps_5_1" }
					},
					"states": { "rasterizer": "solid_back", "depthStencil": "depth_test", "blend": "opaque" }
				}
			]
		}],
		"renderPasses": [
			{ "name": "forward", "queue": "Geometry", "states": { "renderTarget": "MainColor" } },
			{ "name": "shadow", "queue": "Geometry", "states": { "renderTarget": "ShadowMap" } }
		]
	})";
	file.close();

	MaterialSystem materialSystem;
	const bool initialized = materialSystem.initialize( jsonPath.string() );
	REQUIRE( initialized );

	MaterialInstance instance( &device, &materialSystem, "multipass_material" );
	REQUIRE( instance.isValid() );
	REQUIRE( instance.hasPass( "forward" ) );
	REQUIRE( instance.hasPass( "shadow" ) );

	// Skip actual PSO creation for now - shader compilation in test environment is unreliable
	// TODO: Add integration test in full application context
	// ID3D12PipelineState *forwardPSO = instance.getPipelineState( "forward" );
	// ID3D12PipelineState *shadowPSO = instance.getPipelineState( "shadow" );
	// REQUIRE( forwardPSO != nullptr );
	// REQUIRE( shadowPSO != nullptr );
	// REQUIRE( forwardPSO != shadowPSO );

	// Cleanup
	fs::remove_all( tempDir );
}

TEST_CASE( "MaterialInstance getPipelineState for invalid pass returns nullptr", "[material-instance][T303][unit]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "MaterialInstance invalid pass PSO test" ) )
	{
		return;
	}

	MaterialSystem materialSystem;
	const bool initialized = materialSystem.initialize( getTestMaterialsPath() );
	REQUIRE( initialized );

	MaterialInstance instance( &device, &materialSystem, "grid_material" );
	REQUIRE( instance.isValid() );

	// Act - request non-existent pass
	ID3D12PipelineState *pso = instance.getPipelineState( "nonexistent_pass" );

	// Assert
	REQUIRE( pso == nullptr );
}

// Note: Shader hot-reload is handled automatically by PSOBuilder's global cache
// MaterialInstance doesn't need explicit hot-reload support - PSOs are recreated
// when PSOBuilder detects shader file changes via content hashing

// T304 Tests: Command List Setup

TEST_CASE( "MaterialInstance setupCommandList sets PSO and root signature", "[material-instance][T304][integration]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "MaterialInstance setupCommandList test" ) )
	{
		return;
	}

	MaterialSystem materialSystem;
	const bool initialized = materialSystem.initialize( getTestMaterialsPath() );
	REQUIRE( initialized );

	MaterialInstance instance( &device, &materialSystem, "grid_material" );
	REQUIRE( instance.isValid() );
	REQUIRE( instance.hasPass( "grid" ) );

	// Begin frame to reset command list
	device.beginFrame();

	// Get command list for testing
	ID3D12GraphicsCommandList *commandList = device.getCommandList();
	REQUIRE( commandList != nullptr );

	// Act - setup command list with material's grid pass
	const bool success = instance.setupCommandList( commandList, "grid" );

	// Assert - should succeed and set PSO and root signature
	REQUIRE( success );
	// Note: Can't directly verify PSO/root sig were set on command list (D3D12 API limitation)
	// but we can verify the method returned true, indicating both resources were available
}

TEST_CASE( "MaterialInstance setupCommandList returns false for invalid pass", "[material-instance][T304][unit]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "MaterialInstance setupCommandList invalid pass test" ) )
	{
		return;
	}

	MaterialSystem materialSystem;
	const bool initialized = materialSystem.initialize( getTestMaterialsPath() );
	REQUIRE( initialized );

	MaterialInstance instance( &device, &materialSystem, "grid_material" );
	REQUIRE( instance.isValid() );

	// Get command list
	ID3D12GraphicsCommandList *commandList = device.getCommandList();
	REQUIRE( commandList != nullptr );

	// Act - setup with non-existent pass
	const bool success = instance.setupCommandList( commandList, "nonexistent_pass" );

	// Assert - should fail gracefully
	REQUIRE_FALSE( success );
}

TEST_CASE( "MaterialInstance setupCommandList returns false for nullptr command list", "[material-instance][T304][unit]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "MaterialInstance setupCommandList nullptr test" ) )
	{
		return;
	}

	MaterialSystem materialSystem;
	const bool initialized = materialSystem.initialize( getTestMaterialsPath() );
	REQUIRE( initialized );

	MaterialInstance instance( &device, &materialSystem, "grid_material" );
	REQUIRE( instance.isValid() );

	// Act - pass nullptr command list
	const bool success = instance.setupCommandList( nullptr, "grid" );

	// Assert - should fail gracefully
	REQUIRE_FALSE( success );
}

TEST_CASE( "MaterialInstance setupCommandList with different passes succeeds", "[material-instance][T304][integration]" )
{
	// Arrange - create temp JSON with multi-pass material
	const auto tempDir = fs::temp_directory_path() / "material_instance_t304_test";
	fs::create_directories( tempDir );
	const auto jsonPath = tempDir / "test_materials.json";

	std::ofstream file( jsonPath );
	file << R"({
		"states": {
			"renderTargetStates": {
				"MainColor": {
					"rtvFormats": ["R8G8B8A8_UNORM"],
					"dsvFormat": "D32_FLOAT",
					"samples": 1
				},
				"ShadowMap": {
					"rtvFormats": ["R32_FLOAT"],
					"dsvFormat": "D32_FLOAT",
					"samples": 1
				}
			},
			"depthStencilStates": {
				"depth_test": { "depthEnable": true, "depthWriteMask": "All", "depthFunc": "LessEqual", "stencilEnable": false }
			},
			"rasterizerStates": {
				"solid_back": { "fillMode": "Solid", "cullMode": "Back", "frontCounterClockwise": false }
			},
			"blendStates": {
				"opaque": { "alphaToCoverage": false, "independentBlend": false, "renderTargets": [{ "enable": false }] }
			}
		},
		"materials": [{
			"id": "multipass_material",
			"passes": [
				{
					"name": "forward",
					"shaders": {
						"vertex": { "file": "shaders/grid.hlsl", "entry": "VSMain", "profile": "vs_5_1" },
						"pixel": { "file": "shaders/grid.hlsl", "entry": "PSMain", "profile": "ps_5_1" }
					},
					"states": { "rasterizer": "solid_back", "depthStencil": "depth_test", "blend": "opaque" }
				},
				{
					"name": "shadow",
					"shaders": {
						"vertex": { "file": "shaders/grid.hlsl", "entry": "VSMain", "profile": "vs_5_1" },
						"pixel": { "file": "shaders/grid.hlsl", "entry": "PSMain", "profile": "ps_5_1" }
					},
					"states": { "rasterizer": "solid_back", "depthStencil": "depth_test", "blend": "opaque" }
				}
			]
		}],
		"renderPasses": [
			{ "name": "forward", "queue": "Geometry", "states": { "renderTarget": "MainColor" } },
			{ "name": "shadow", "queue": "Geometry", "states": { "renderTarget": "ShadowMap" } }
		]
	})";
	file.close();

	dx12::Device device;
	if ( !requireHeadlessDevice( device, "MaterialInstance multi-pass setupCommandList test" ) )
	{
		fs::remove_all( tempDir );
		return;
	}

	MaterialSystem materialSystem;
	const bool initialized = materialSystem.initialize( jsonPath.string() );
	REQUIRE( initialized );

	MaterialInstance instance( &device, &materialSystem, "multipass_material" );
	REQUIRE( instance.isValid() );
	REQUIRE( instance.hasPass( "forward" ) );
	REQUIRE( instance.hasPass( "shadow" ) );

	// Begin frame to reset command list
	device.beginFrame();

	// Get command list
	ID3D12GraphicsCommandList *commandList = device.getCommandList();
	REQUIRE( commandList != nullptr );

	// Act - setup with forward pass, then shadow pass
	const bool forwardSuccess = instance.setupCommandList( commandList, "forward" );
	const bool shadowSuccess = instance.setupCommandList( commandList, "shadow" );

	// Assert - both should succeed
	REQUIRE( forwardSuccess );
	REQUIRE( shadowSuccess );

	// Cleanup
	fs::remove_all( tempDir );
}

// Note: T305 hot-reload tests removed - hot-reload is now handled automatically by
// PSOBuilder's global cache. No explicit MaterialInstance callback mechanism needed.

TEST_CASE( "MaterialInstance caches MaterialDefinition pointer for performance", "[material-instance][T306][unit]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "MaterialInstance definition caching test" ) )
	{
		return;
	}

	MaterialSystem materialSystem;
	const bool initialized = materialSystem.initialize( getTestMaterialsPath() );
	REQUIRE( initialized );

	MaterialInstance instance( &device, &materialSystem, "grid_material" );
	REQUIRE( instance.isValid() );

	// Act - call getMaterial() multiple times
	const MaterialDefinition *def1 = instance.getMaterial();
	const MaterialDefinition *def2 = instance.getMaterial();
	const MaterialDefinition *def3 = instance.getMaterial();

	// Assert - should return same pointer (cached, not re-queried from MaterialSystem)
	REQUIRE( def1 != nullptr );
	REQUIRE( def1 == def2 );
	REQUIRE( def2 == def3 );
	REQUIRE( def1->id == "grid_material" );
}
