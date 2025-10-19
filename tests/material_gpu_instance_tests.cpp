#include <catch2/catch_test_macros.hpp>

#include "graphics/gpu/material_gpu.h"
#include "graphics/texture/texture_manager.h"
#include "graphics/texture/bindless_texture_heap.h"
#include "engine/assets/assets.h"
#include "platform/dx12/dx12_device.h"

TEST_CASE( "MaterialGPU applies MaterialInstance overrides to constants", "[MaterialGPU][MaterialInstance][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );
	graphics::texture::TextureManager textureManager;
	textureManager.initialize( &device, 1024 );

	auto material = std::make_shared<assets::Material>();
	material->getPBRMaterial().baseColorFactor = { 0.5f, 0.5f, 0.5f, 1.0f };
	material->getPBRMaterial().metallicFactor = 0.3f;
	material->getPBRMaterial().roughnessFactor = 0.6f;
	material->getPBRMaterial().emissiveFactor = { 0.0f, 0.0f, 0.0f };
	material->setPath( "override_test_material" );
	material->setLoaded( true );

	assets::MaterialInstance instance;
	instance.baseMaterial = 0;
	instance.baseColorFactorOverride = math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f };
	instance.metallicFactorOverride = 0.8f;
	instance.roughnessFactorOverride = 0.2f;
	instance.emissiveFactorOverride = math::Vec3f{ 0.5f, 0.5f, 0.5f };

	// Act
	graphics::gpu::MaterialGPU materialGPU{ material, device, &instance, textureManager };

	// Assert - Should use override values, not base material values
	REQUIRE( materialGPU.isValid() );
	const auto &constants = materialGPU.getMaterialConstants();
	REQUIRE( constants.baseColorFactor == math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f } );
	REQUIRE( constants.metallicFactor == 0.8f );
	REQUIRE( constants.roughnessFactor == 0.2f );
	REQUIRE( constants.emissiveFactor == math::Vec3f{ 0.5f, 0.5f, 0.5f } );
}

TEST_CASE( "MaterialGPU with nullptr MaterialInstance uses base material values", "[MaterialGPU][MaterialInstance][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );
	graphics::texture::TextureManager textureManager;
	textureManager.initialize( &device, 1024 );

	auto material = std::make_shared<assets::Material>();
	material->getPBRMaterial().baseColorFactor = { 0.7f, 0.7f, 0.7f, 1.0f };
	material->getPBRMaterial().metallicFactor = 0.4f;
	material->getPBRMaterial().roughnessFactor = 0.5f;
	material->getPBRMaterial().emissiveFactor = { 0.1f, 0.1f, 0.1f };
	material->setPath( "no_override_material" );
	material->setLoaded( true );

	// Act
	graphics::gpu::MaterialGPU materialGPU{ material, device, nullptr, textureManager };

	// Assert - Should use base material values when no instance provided
	REQUIRE( materialGPU.isValid() );
	const auto &constants = materialGPU.getMaterialConstants();
	REQUIRE( constants.baseColorFactor == math::Vec4f{ 0.7f, 0.7f, 0.7f, 1.0f } );
	REQUIRE( constants.metallicFactor == 0.4f );
	REQUIRE( constants.roughnessFactor == 0.5f );
	REQUIRE( constants.emissiveFactor == math::Vec3f{ 0.1f, 0.1f, 0.1f } );
}

TEST_CASE( "MaterialGPU with partial MaterialInstance overrides", "[MaterialGPU][MaterialInstance][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );
	graphics::texture::TextureManager textureManager;
	textureManager.initialize( &device, 1024 );

	auto material = std::make_shared<assets::Material>();
	material->getPBRMaterial().baseColorFactor = { 0.5f, 0.5f, 0.5f, 1.0f };
	material->getPBRMaterial().metallicFactor = 0.3f;
	material->getPBRMaterial().roughnessFactor = 0.6f;
	material->getPBRMaterial().emissiveFactor = { 0.0f, 0.0f, 0.0f };
	material->setPath( "partial_override_material" );
	material->setLoaded( true );

	assets::MaterialInstance instance;
	instance.baseMaterial = 0;
	// Only override metallic, not others
	instance.metallicFactorOverride = 0.9f;

	// Act
	graphics::gpu::MaterialGPU materialGPU{ material, device, &instance, textureManager };

	// Assert - metallic is overridden, others use base material
	REQUIRE( materialGPU.isValid() );
	const auto &constants = materialGPU.getMaterialConstants();
	REQUIRE( constants.baseColorFactor == material->getPBRMaterial().baseColorFactor );
	REQUIRE( constants.metallicFactor == 0.9f );  // Override
	REQUIRE( constants.roughnessFactor == 0.6f ); // Base material
	REQUIRE( constants.emissiveFactor == material->getPBRMaterial().emissiveFactor );
}

TEST_CASE( "MaterialGPU::updateFromInstance applies new overrides", "[MaterialGPU][MaterialInstance][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );
	graphics::texture::TextureManager textureManager;
	textureManager.initialize( &device, 1024 );

	auto material = std::make_shared<assets::Material>();
	material->getPBRMaterial().baseColorFactor = { 0.5f, 0.5f, 0.5f, 1.0f };
	material->getPBRMaterial().metallicFactor = 0.3f;
	material->getPBRMaterial().roughnessFactor = 0.6f;
	material->setPath( "update_override_material" );
	material->setLoaded( true );

	graphics::gpu::MaterialGPU materialGPU{ material, device, nullptr, textureManager };
	REQUIRE( materialGPU.getMaterialConstants().metallicFactor == 0.3f );

	// Act - Apply new overrides
	assets::MaterialInstance newInstance;
	newInstance.baseMaterial = 0;
	newInstance.metallicFactorOverride = 0.7f;
	newInstance.roughnessFactorOverride = 0.4f;
	materialGPU.updateFromInstance( &newInstance );

	// Assert - Constants should be updated
	REQUIRE( materialGPU.getMaterialConstants().metallicFactor == 0.7f );
	REQUIRE( materialGPU.getMaterialConstants().roughnessFactor == 0.4f );
	REQUIRE( materialGPU.getMaterialConstants().baseColorFactor == material->getPBRMaterial().baseColorFactor );
}

TEST_CASE( "MaterialGPU::updateFromInstance with nullptr clears overrides", "[MaterialGPU][MaterialInstance][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );
	graphics::texture::TextureManager textureManager;
	textureManager.initialize( &device, 1024 );

	auto material = std::make_shared<assets::Material>();
	material->getPBRMaterial().baseColorFactor = { 0.5f, 0.5f, 0.5f, 1.0f };
	material->getPBRMaterial().metallicFactor = 0.3f;
	material->getPBRMaterial().roughnessFactor = 0.6f;
	material->setPath( "clear_override_material" );
	material->setLoaded( true );

	assets::MaterialInstance initialInstance;
	initialInstance.baseMaterial = 0;
	initialInstance.metallicFactorOverride = 0.8f;
	initialInstance.roughnessFactorOverride = 0.2f;

	graphics::gpu::MaterialGPU materialGPU{ material, device, &initialInstance, textureManager };
	REQUIRE( materialGPU.getMaterialConstants().metallicFactor == 0.8f );
	REQUIRE( materialGPU.getMaterialConstants().roughnessFactor == 0.2f );

	// Act - Clear overrides by passing nullptr
	materialGPU.updateFromInstance( nullptr );

	// Assert - Should revert to base material values
	REQUIRE( materialGPU.getMaterialConstants().metallicFactor == 0.3f );
	REQUIRE( materialGPU.getMaterialConstants().roughnessFactor == 0.6f );
}

TEST_CASE( "MaterialGPU with MaterialInstance emissive override", "[MaterialGPU][MaterialInstance][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );
	graphics::texture::TextureManager textureManager;
	textureManager.initialize( &device, 1024 );

	auto material = std::make_shared<assets::Material>();
	material->getPBRMaterial().emissiveFactor = { 0.0f, 0.0f, 0.0f };
	material->setPath( "emissive_override_material" );
	material->setLoaded( true );

	assets::MaterialInstance instance;
	instance.baseMaterial = 0;
	instance.emissiveFactorOverride = math::Vec3f{ 1.0f, 1.0f, 0.0f }; // Yellow emissive

	// Act
	graphics::gpu::MaterialGPU materialGPU{ material, device, &instance, textureManager };

	// Assert
	REQUIRE( materialGPU.isValid() );
	REQUIRE( materialGPU.getMaterialConstants().emissiveFactor == math::Vec3f{ 1.0f, 1.0f, 0.0f } );
}

TEST_CASE( "MaterialGPU with all MaterialInstance properties overridden", "[MaterialGPU][MaterialInstance][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );
	graphics::texture::TextureManager textureManager;
	textureManager.initialize( &device, 1024 );

	auto material = std::make_shared<assets::Material>();
	material->getPBRMaterial().baseColorFactor = { 0.5f, 0.5f, 0.5f, 1.0f };
	material->getPBRMaterial().metallicFactor = 0.3f;
	material->getPBRMaterial().roughnessFactor = 0.6f;
	material->getPBRMaterial().emissiveFactor = { 0.0f, 0.0f, 0.0f };
	material->setPath( "all_override_material" );
	material->setLoaded( true );

	assets::MaterialInstance instance;
	instance.baseMaterial = 0;
	instance.baseColorFactorOverride = math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f };
	instance.metallicFactorOverride = 0.9f;
	instance.roughnessFactorOverride = 0.1f;
	instance.emissiveFactorOverride = math::Vec3f{ 0.2f, 0.3f, 0.4f };

	// Act
	graphics::gpu::MaterialGPU materialGPU{ material, device, &instance, textureManager };

	// Assert - All should be overridden
	REQUIRE( materialGPU.isValid() );
	const auto &constants = materialGPU.getMaterialConstants();
	REQUIRE( constants.baseColorFactor == math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f } );
	REQUIRE( constants.metallicFactor == 0.9f );
	REQUIRE( constants.roughnessFactor == 0.1f );
	REQUIRE( constants.emissiveFactor == math::Vec3f{ 0.2f, 0.3f, 0.4f } );
}
