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

MaterialPreset MaterialPreset::fromMaterialInstance( const std::string &presetName, const MaterialInstance &instance )
{
	MaterialPreset preset;
	preset.name = presetName;
	preset.baseColorFactorOverride = instance.baseColorFactorOverride;
	preset.metallicFactorOverride = instance.metallicFactorOverride;
	preset.roughnessFactorOverride = instance.roughnessFactorOverride;
	preset.emissiveFactorOverride = instance.emissiveFactorOverride;
	preset.baseColorTextureOverride = instance.baseColorTextureOverride;
	preset.metallicRoughnessTextureOverride = instance.metallicRoughnessTextureOverride;
	preset.normalTextureOverride = instance.normalTextureOverride;
	preset.emissiveTextureOverride = instance.emissiveTextureOverride;
	return preset;
}

void MaterialPreset::applyTo( MaterialInstance &instance ) const
{
	// Only apply overrides that are present in the preset
	if ( baseColorFactorOverride.has_value() )
	{
		instance.baseColorFactorOverride = baseColorFactorOverride;
	}
	if ( metallicFactorOverride.has_value() )
	{
		instance.metallicFactorOverride = metallicFactorOverride;
	}
	if ( roughnessFactorOverride.has_value() )
	{
		instance.roughnessFactorOverride = roughnessFactorOverride;
	}
	if ( emissiveFactorOverride.has_value() )
	{
		instance.emissiveFactorOverride = emissiveFactorOverride;
	}
	if ( baseColorTextureOverride.has_value() )
	{
		instance.baseColorTextureOverride = baseColorTextureOverride;
	}
	if ( metallicRoughnessTextureOverride.has_value() )
	{
		instance.metallicRoughnessTextureOverride = metallicRoughnessTextureOverride;
	}
	if ( normalTextureOverride.has_value() )
	{
		instance.normalTextureOverride = normalTextureOverride;
	}
	if ( emissiveTextureOverride.has_value() )
	{
		instance.emissiveTextureOverride = emissiveTextureOverride;
	}
}

bool MaterialPreset::hasOverrides() const
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

} // namespace assets
