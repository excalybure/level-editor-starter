#include <catch2/catch_test_macros.hpp>

#include "engine/gpu/material_gpu.h"
#include "engine/assets/assets.h"
#include "platform/dx12/dx12_device.h"

TEST_CASE( "MaterialGPU can be created from assets::Material", "[MaterialGPU][unit]" )
{
	// Arrange
	auto material = std::make_shared<assets::Material>();
	material->getPBRMaterial().baseColorFactor = { 1.0f, 0.5f, 0.2f, 1.0f };
	material->getPBRMaterial().metallicFactor = 0.8f;
	material->getPBRMaterial().roughnessFactor = 0.3f;
	material->setPath( "test_material" );
	material->setLoaded( true );

	// Act
	engine::gpu::MaterialGPU materialGPU{ material };

	// Assert
	REQUIRE( materialGPU.isValid() );
	REQUIRE( materialGPU.getSourceMaterial() == material );

	const auto &constants = materialGPU.getMaterialConstants();
	REQUIRE( constants.baseColorFactor.x == 1.0f );
	REQUIRE( constants.baseColorFactor.y == 0.5f );
	REQUIRE( constants.baseColorFactor.z == 0.2f );
	REQUIRE( constants.baseColorFactor.w == 1.0f );
	REQUIRE( constants.metallicFactor == 0.8f );
	REQUIRE( constants.roughnessFactor == 0.3f );
}

TEST_CASE( "MaterialGPU with device creates valid constant buffer", "[MaterialGPU][gpu][integration]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	auto material = std::make_shared<assets::Material>();
	material->getPBRMaterial().baseColorFactor = { 0.8f, 0.6f, 0.4f, 1.0f };
	material->getPBRMaterial().metallicFactor = 0.5f;
	material->getPBRMaterial().roughnessFactor = 0.7f;
	material->setPath( "gpu_test_material" );
	material->setLoaded( true );

	// Act
	engine::gpu::MaterialGPU materialGPU{ material, device };

	// Assert
	REQUIRE( materialGPU.isValid() );
	REQUIRE( materialGPU.getSourceMaterial() == material );

	const auto &constants = materialGPU.getMaterialConstants();
	REQUIRE( constants.baseColorFactor.x == 0.8f );
	REQUIRE( constants.baseColorFactor.y == 0.6f );
	REQUIRE( constants.baseColorFactor.z == 0.4f );
	REQUIRE( constants.baseColorFactor.w == 1.0f );
	REQUIRE( constants.metallicFactor == 0.5f );
	REQUIRE( constants.roughnessFactor == 0.7f );
}

TEST_CASE( "MaterialGPU sets texture flags correctly based on material textures", "[MaterialGPU][unit]" )
{
	// Arrange
	auto material = std::make_shared<assets::Material>();
	auto &pbr = material->getPBRMaterial();
	pbr.baseColorTexture = "base_color.png";
	pbr.normalTexture = "normal.png";
	// Leave metallic roughness and emissive empty
	material->setPath( "textured_material" );
	material->setLoaded( true );

	// Act
	engine::gpu::MaterialGPU materialGPU{ material };

	// Assert
	REQUIRE( materialGPU.isValid() );
	const auto &constants = materialGPU.getMaterialConstants();

	// Check that only base color and normal texture bits are set
	REQUIRE( ( constants.textureFlags & engine::gpu::MaterialConstants::kBaseColorTextureBit ) != 0 );
	REQUIRE( ( constants.textureFlags & engine::gpu::MaterialConstants::kNormalTextureBit ) != 0 );
	REQUIRE( ( constants.textureFlags & engine::gpu::MaterialConstants::kMetallicRoughnessTextureBit ) == 0 );
	REQUIRE( ( constants.textureFlags & engine::gpu::MaterialConstants::kEmissiveTextureBit ) == 0 );
}

TEST_CASE( "MaterialGPU handles null material gracefully", "[MaterialGPU][unit]" )
{
	// Arrange & Act
	engine::gpu::MaterialGPU materialGPU{ nullptr };

	// Assert
	REQUIRE_FALSE( materialGPU.isValid() );
	REQUIRE( materialGPU.getSourceMaterial() == nullptr );
}

TEST_CASE( "MaterialGPU bindToCommandList handles null command list gracefully", "[MaterialGPU][unit]" )
{
	// Arrange
	auto material = std::make_shared<assets::Material>();
	material->setPath( "test_material" );
	material->setLoaded( true );
	engine::gpu::MaterialGPU materialGPU{ material };

	// Act & Assert - should not crash
	REQUIRE_NOTHROW( materialGPU.bindToCommandList( nullptr ) );
}

TEST_CASE( "MaterialGPU supports move semantics", "[MaterialGPU][unit]" )
{
	// Arrange
	auto material = std::make_shared<assets::Material>();
	material->getPBRMaterial().metallicFactor = 0.7f;
	material->setPath( "movable_material" );
	material->setLoaded( true );

	engine::gpu::MaterialGPU original{ material };
	REQUIRE( original.isValid() );

	// Act - Move constructor
	engine::gpu::MaterialGPU moved{ std::move( original ) };

	// Assert
	REQUIRE( moved.isValid() );
	REQUIRE( moved.getSourceMaterial() == material );
	REQUIRE( moved.getMaterialConstants().metallicFactor == 0.7f );
	REQUIRE_FALSE( original.isValid() ); // Original should be invalidated
}
