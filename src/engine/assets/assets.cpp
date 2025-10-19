#include "assets.h"

// Placeholder implementations for basic asset system

namespace assets
{

math::Vec4f MaterialInstance::getEffectiveBaseColor( const Material *baseMat ) const
{
	if ( baseColorFactorOverride.has_value() )
	{
		return baseColorFactorOverride.value();
	}
	return ( baseMat != nullptr ) ? baseMat->getPBRMaterial().baseColorFactor : math::Vec4f{ 1.0f, 1.0f, 1.0f, 1.0f };
}

float MaterialInstance::getEffectiveMetallic( const Material *baseMat ) const
{
	if ( metallicFactorOverride.has_value() )
	{
		return metallicFactorOverride.value();
	}
	return ( baseMat != nullptr ) ? baseMat->getPBRMaterial().metallicFactor : 0.0f;
}

float MaterialInstance::getEffectiveRoughness( const Material *baseMat ) const
{
	if ( roughnessFactorOverride.has_value() )
	{
		return roughnessFactorOverride.value();
	}
	return ( baseMat != nullptr ) ? baseMat->getPBRMaterial().roughnessFactor : 1.0f;
}

math::Vec3f MaterialInstance::getEffectiveEmissive( const Material *baseMat ) const
{
	if ( emissiveFactorOverride.has_value() )
	{
		return emissiveFactorOverride.value();
	}
	return ( baseMat != nullptr ) ? baseMat->getPBRMaterial().emissiveFactor : math::Vec3f{ 0.0f, 0.0f, 0.0f };
}

bool MaterialInstance::hasOverrides() const
{
	return baseColorFactorOverride.has_value() ||
		metallicFactorOverride.has_value() ||
		roughnessFactorOverride.has_value() ||
		emissiveFactorOverride.has_value() ||
		baseColorTextureOverride.has_value() ||
		metallicRoughnessTextureOverride.has_value() ||
		normalTextureOverride.has_value() ||
		emissiveTextureOverride.has_value();
}

void MaterialInstance::clearOverrides()
{
	baseColorFactorOverride.reset();
	metallicFactorOverride.reset();
	roughnessFactorOverride.reset();
	emissiveFactorOverride.reset();
	baseColorTextureOverride.reset();
	metallicRoughnessTextureOverride.reset();
	normalTextureOverride.reset();
	emissiveTextureOverride.reset();
}

} // namespace assets
