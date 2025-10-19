#include <catch2/catch_test_macros.hpp>

#include "engine/assets/assets.h"
#include "math/vec.h"

TEST_CASE( "MaterialInstance can be created with default values", "[MaterialInstance][unit]" )
{
	// Arrange & Act
	assets::MaterialInstance instance;

	// Assert
	REQUIRE( instance.baseMaterial == assets::INVALID_MATERIAL_HANDLE );
	REQUIRE( !instance.hasOverrides() );
}

TEST_CASE( "MaterialInstance baseColorFactor override", "[MaterialInstance][unit]" )
{
	// Arrange
	assets::MaterialInstance instance;
	instance.baseMaterial = 0;
	const math::Vec4f overrideColor{ 1.0f, 0.5f, 0.2f, 1.0f };
	const math::Vec4f baseColor{ 0.8f, 0.8f, 0.8f, 1.0f };

	// Act
	instance.baseColorFactorOverride = overrideColor;

	// Assert
	REQUIRE( instance.baseColorFactorOverride.has_value() );
	REQUIRE( instance.baseColorFactorOverride.value() == overrideColor );
	REQUIRE( instance.hasOverrides() );
}

TEST_CASE( "MaterialInstance metallicFactor override", "[MaterialInstance][unit]" )
{
	// Arrange
	assets::MaterialInstance instance;
	instance.baseMaterial = 0;
	const float overrideValue = 0.8f;

	// Act
	instance.metallicFactorOverride = overrideValue;

	// Assert
	REQUIRE( instance.metallicFactorOverride.has_value() );
	REQUIRE( instance.metallicFactorOverride.value() == 0.8f );
	REQUIRE( instance.hasOverrides() );
}

TEST_CASE( "MaterialInstance roughnessFactor override", "[MaterialInstance][unit]" )
{
	// Arrange
	assets::MaterialInstance instance;
	instance.baseMaterial = 0;
	const float overrideValue = 0.3f;

	// Act
	instance.roughnessFactorOverride = overrideValue;

	// Assert
	REQUIRE( instance.roughnessFactorOverride.has_value() );
	REQUIRE( instance.roughnessFactorOverride.value() == 0.3f );
	REQUIRE( instance.hasOverrides() );
}

TEST_CASE( "MaterialInstance emissiveFactor override", "[MaterialInstance][unit]" )
{
	// Arrange
	assets::MaterialInstance instance;
	instance.baseMaterial = 0;
	const math::Vec3f overrideValue{ 1.0f, 0.5f, 0.0f };

	// Act
	instance.emissiveFactorOverride = overrideValue;

	// Assert
	REQUIRE( instance.emissiveFactorOverride.has_value() );
	REQUIRE( instance.emissiveFactorOverride.value() == overrideValue );
	REQUIRE( instance.hasOverrides() );
}

TEST_CASE( "MaterialInstance texture overrides", "[MaterialInstance][unit]" )
{
	// Arrange
	assets::MaterialInstance instance;
	instance.baseMaterial = 0;

	// Act
	instance.baseColorTextureOverride = "textures/custom_color.png";
	instance.metallicRoughnessTextureOverride = "textures/custom_mr.png";
	instance.normalTextureOverride = "textures/custom_normal.png";
	instance.emissiveTextureOverride = "textures/custom_emissive.png";

	// Assert
	REQUIRE( instance.baseColorTextureOverride.has_value() );
	REQUIRE( instance.baseColorTextureOverride.value() == "textures/custom_color.png" );
	REQUIRE( instance.metallicRoughnessTextureOverride.has_value() );
	REQUIRE( instance.normalTextureOverride.has_value() );
	REQUIRE( instance.emissiveTextureOverride.has_value() );
	REQUIRE( instance.hasOverrides() );
}

TEST_CASE( "MaterialInstance hasOverrides returns false when no overrides set", "[MaterialInstance][unit]" )
{
	// Arrange
	assets::MaterialInstance instance;
	instance.baseMaterial = 0; // base material set but no overrides

	// Act & Assert
	REQUIRE( !instance.hasOverrides() );
}

TEST_CASE( "MaterialInstance getEffectiveBaseColor uses override when present", "[MaterialInstance][unit]" )
{
	// Arrange
	auto baseMaterial = std::make_shared<assets::Material>();
	baseMaterial->setName( "base_mat" );
	baseMaterial->getPBRMaterial().baseColorFactor = { 0.8f, 0.8f, 0.8f, 1.0f };

	assets::MaterialInstance instance;
	instance.baseMaterial = 0;
	const math::Vec4f overrideColor{ 1.0f, 0.0f, 0.0f, 1.0f };
	instance.baseColorFactorOverride = overrideColor;

	// Act
	const auto effectiveColor = instance.getEffectiveBaseColor( baseMaterial.get() );

	// Assert
	REQUIRE( effectiveColor == overrideColor );
}

TEST_CASE( "MaterialInstance getEffectiveBaseColor uses base when no override", "[MaterialInstance][unit]" )
{
	// Arrange
	auto baseMaterial = std::make_shared<assets::Material>();
	baseMaterial->setName( "base_mat" );
	const math::Vec4f baseColor{ 0.8f, 0.8f, 0.8f, 1.0f };
	baseMaterial->getPBRMaterial().baseColorFactor = baseColor;

	assets::MaterialInstance instance;
	instance.baseMaterial = 0;
	// No override set

	// Act
	const auto effectiveColor = instance.getEffectiveBaseColor( baseMaterial.get() );

	// Assert
	REQUIRE( effectiveColor == baseColor );
}

TEST_CASE( "MaterialInstance getEffectiveMetallic uses override when present", "[MaterialInstance][unit]" )
{
	// Arrange
	auto baseMaterial = std::make_shared<assets::Material>();
	baseMaterial->setName( "base_mat" );
	baseMaterial->getPBRMaterial().metallicFactor = 0.2f;

	assets::MaterialInstance instance;
	instance.baseMaterial = 0;
	const float overrideValue = 0.8f;
	instance.metallicFactorOverride = overrideValue;

	// Act
	const auto effectiveValue = instance.getEffectiveMetallic( baseMaterial.get() );

	// Assert
	REQUIRE( effectiveValue == 0.8f );
}

TEST_CASE( "MaterialInstance getEffectiveMetallic uses base when no override", "[MaterialInstance][unit]" )
{
	// Arrange
	auto baseMaterial = std::make_shared<assets::Material>();
	baseMaterial->setName( "base_mat" );
	baseMaterial->getPBRMaterial().metallicFactor = 0.5f;

	assets::MaterialInstance instance;
	instance.baseMaterial = 0;
	// No override set

	// Act
	const auto effectiveValue = instance.getEffectiveMetallic( baseMaterial.get() );

	// Assert
	REQUIRE( effectiveValue == 0.5f );
}

TEST_CASE( "MaterialInstance getEffectiveRoughness uses override when present", "[MaterialInstance][unit]" )
{
	// Arrange
	auto baseMaterial = std::make_shared<assets::Material>();
	baseMaterial->setName( "base_mat" );
	baseMaterial->getPBRMaterial().roughnessFactor = 0.5f;

	assets::MaterialInstance instance;
	instance.baseMaterial = 0;
	const float overrideValue = 0.2f;
	instance.roughnessFactorOverride = overrideValue;

	// Act
	const auto effectiveValue = instance.getEffectiveRoughness( baseMaterial.get() );

	// Assert
	REQUIRE( effectiveValue == 0.2f );
}

TEST_CASE( "MaterialInstance getEffectiveRoughness uses base when no override", "[MaterialInstance][unit]" )
{
	// Arrange
	auto baseMaterial = std::make_shared<assets::Material>();
	baseMaterial->setName( "base_mat" );
	baseMaterial->getPBRMaterial().roughnessFactor = 0.7f;

	assets::MaterialInstance instance;
	instance.baseMaterial = 0;
	// No override set

	// Act
	const auto effectiveValue = instance.getEffectiveRoughness( baseMaterial.get() );

	// Assert
	REQUIRE( effectiveValue == 0.7f );
}

TEST_CASE( "MaterialInstance getEffectiveEmissive uses override when present", "[MaterialInstance][unit]" )
{
	// Arrange
	auto baseMaterial = std::make_shared<assets::Material>();
	baseMaterial->setName( "base_mat" );
	baseMaterial->getPBRMaterial().emissiveFactor = { 0.0f, 0.0f, 0.0f };

	assets::MaterialInstance instance;
	instance.baseMaterial = 0;
	const math::Vec3f overrideValue{ 1.0f, 1.0f, 0.0f };
	instance.emissiveFactorOverride = overrideValue;

	// Act
	const auto effectiveValue = instance.getEffectiveEmissive( baseMaterial.get() );

	// Assert
	REQUIRE( effectiveValue == overrideValue );
}

TEST_CASE( "MaterialInstance getEffectiveEmissive uses base when no override", "[MaterialInstance][unit]" )
{
	// Arrange
	auto baseMaterial = std::make_shared<assets::Material>();
	baseMaterial->setName( "base_mat" );
	const math::Vec3f baseValue{ 0.5f, 0.5f, 0.5f };
	baseMaterial->getPBRMaterial().emissiveFactor = baseValue;

	assets::MaterialInstance instance;
	instance.baseMaterial = 0;
	// No override set

	// Act
	const auto effectiveValue = instance.getEffectiveEmissive( baseMaterial.get() );

	// Assert
	REQUIRE( effectiveValue == baseValue );
}

TEST_CASE( "MaterialInstance clearOverrides resets all overrides", "[MaterialInstance][unit]" )
{
	// Arrange
	assets::MaterialInstance instance;
	instance.baseMaterial = 0;
	instance.baseColorFactorOverride = math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f };
	instance.metallicFactorOverride = 0.8f;
	instance.roughnessFactorOverride = 0.3f;
	instance.emissiveFactorOverride = math::Vec3f{ 1.0f, 1.0f, 0.0f };
	instance.baseColorTextureOverride = "texture.png";
	REQUIRE( instance.hasOverrides() );

	// Act
	instance.clearOverrides();

	// Assert
	REQUIRE( !instance.baseColorFactorOverride.has_value() );
	REQUIRE( !instance.metallicFactorOverride.has_value() );
	REQUIRE( !instance.roughnessFactorOverride.has_value() );
	REQUIRE( !instance.emissiveFactorOverride.has_value() );
	REQUIRE( !instance.baseColorTextureOverride.has_value() );
	REQUIRE( !instance.metallicRoughnessTextureOverride.has_value() );
	REQUIRE( !instance.normalTextureOverride.has_value() );
	REQUIRE( !instance.emissiveTextureOverride.has_value() );
	REQUIRE( !instance.hasOverrides() );
}

TEST_CASE( "MaterialInstance multiple overrides all detected by hasOverrides", "[MaterialInstance][unit]" )
{
	// Arrange
	assets::MaterialInstance instance;
	instance.baseMaterial = 0;

	// Act
	instance.baseColorFactorOverride = math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f };
	REQUIRE( instance.hasOverrides() );

	instance.metallicFactorOverride = 0.8f;
	REQUIRE( instance.hasOverrides() );

	instance.roughnessFactorOverride = 0.3f;
	REQUIRE( instance.hasOverrides() );

	instance.emissiveFactorOverride = math::Vec3f{ 0.5f, 0.5f, 0.5f };
	REQUIRE( instance.hasOverrides() );

	// Assert all still has overrides
	REQUIRE( instance.hasOverrides() );
}
