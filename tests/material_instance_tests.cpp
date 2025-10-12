#include <catch2/catch_test_macros.hpp>
#include "graphics/material_system/material_instance.h"
#include "graphics/material_system/material_system.h"
#include "platform/dx12/dx12_device.h"
#include "test_dx12_helpers.h"
#include <filesystem>

using namespace graphics::material_system;
namespace fs = std::filesystem;

// Test helper: Get path to test materials JSON
static std::string getTestMaterialsPath()
{
	return "tests/editor_config.json";
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

	// Assert - check that getHandle() works (indicates MaterialSystem integration)
	const MaterialHandle handle = instance.getHandle();
	REQUIRE( handle.isValid() );
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

TEST_CASE( "MaterialInstance getHandle returns valid handle", "[material-instance-T301][unit]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "MaterialInstance getHandle test" ) )
	{
		return;
	}

	MaterialSystem materialSystem;

	const bool initialized = materialSystem.initialize( getTestMaterialsPath() );
	REQUIRE( initialized );

	MaterialInstance instance( &device, &materialSystem, "grid_material" );
	REQUIRE( instance.isValid() );

	// Act
	const MaterialHandle handle = instance.getHandle();

	// Assert
	REQUIRE( handle.isValid() );
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
