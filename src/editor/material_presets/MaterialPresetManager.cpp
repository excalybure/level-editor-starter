#include "MaterialPresetManager.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include "core/console.h"

using json = nlohmann::json;

namespace editor
{

bool MaterialPresetManager::addPreset( const assets::MaterialPreset &preset )
{
	// Check if preset with same name already exists
	if ( hasPreset( preset.name ) )
	{
		console::warning( "Material preset '{}' already exists", preset.name );
		return false;
	}

	m_presets.push_back( preset );
	return true;
}

bool MaterialPresetManager::removePreset( const std::string &presetName )
{
	const int index = findPresetIndex( presetName );
	if ( index < 0 )
	{
		console::warning( "Material preset '{}' not found", presetName );
		return false;
	}

	m_presets.erase( m_presets.begin() + index );
	return true;
}

std::optional<assets::MaterialPreset> MaterialPresetManager::getPreset( const std::string &presetName ) const
{
	const int index = findPresetIndex( presetName );
	if ( index < 0 )
	{
		return std::nullopt;
	}

	return m_presets[index];
}

std::vector<std::string> MaterialPresetManager::getPresetNames() const
{
	std::vector<std::string> names;
	names.reserve( m_presets.size() );

	for ( const auto &preset : m_presets )
	{
		names.push_back( preset.name );
	}

	return names;
}

bool MaterialPresetManager::hasPreset( const std::string &presetName ) const
{
	return findPresetIndex( presetName ) >= 0;
}

void MaterialPresetManager::clear()
{
	m_presets.clear();
}

bool MaterialPresetManager::loadFromFile( const std::string &filePath )
{
	std::ifstream file( filePath );
	if ( !file.is_open() )
	{
		console::warning( "Failed to open material presets file: {}", filePath );
		return false;
	}

	try
	{
		json j;
		file >> j;

		if ( !j.contains( "materialPresets" ) || !j["materialPresets"].is_array() )
		{
			console::warning( "Invalid material presets file format: {}", filePath );
			return false;
		}

		clear();

		for ( const auto &presetJson : j["materialPresets"] )
		{
			assets::MaterialPreset preset;

			if ( presetJson.contains( "name" ) && presetJson["name"].is_string() )
			{
				preset.name = presetJson["name"].get<std::string>();
			}
			else
			{
				console::warning( "Skipping preset without name" );
				continue;
			}

			// Load optional overrides
			if ( presetJson.contains( "baseColorFactor" ) && presetJson["baseColorFactor"].is_array() &&
				presetJson["baseColorFactor"].size() == 4 )
			{
				const auto &arr = presetJson["baseColorFactor"];
				preset.baseColorFactorOverride = math::Vec4f{
					arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>(), arr[3].get<float>()
				};
			}

			if ( presetJson.contains( "metallicFactor" ) && presetJson["metallicFactor"].is_number() )
			{
				preset.metallicFactorOverride = presetJson["metallicFactor"].get<float>();
			}

			if ( presetJson.contains( "roughnessFactor" ) && presetJson["roughnessFactor"].is_number() )
			{
				preset.roughnessFactorOverride = presetJson["roughnessFactor"].get<float>();
			}

			if ( presetJson.contains( "emissiveFactor" ) && presetJson["emissiveFactor"].is_array() &&
				presetJson["emissiveFactor"].size() == 3 )
			{
				const auto &arr = presetJson["emissiveFactor"];
				preset.emissiveFactorOverride = math::Vec3f{
					arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>()
				};
			}

			if ( presetJson.contains( "baseColorTexture" ) && presetJson["baseColorTexture"].is_string() )
			{
				preset.baseColorTextureOverride = presetJson["baseColorTexture"].get<std::string>();
			}

			if ( presetJson.contains( "metallicRoughnessTexture" ) && presetJson["metallicRoughnessTexture"].is_string() )
			{
				preset.metallicRoughnessTextureOverride = presetJson["metallicRoughnessTexture"].get<std::string>();
			}

			if ( presetJson.contains( "normalTexture" ) && presetJson["normalTexture"].is_string() )
			{
				preset.normalTextureOverride = presetJson["normalTexture"].get<std::string>();
			}

			if ( presetJson.contains( "emissiveTexture" ) && presetJson["emissiveTexture"].is_string() )
			{
				preset.emissiveTextureOverride = presetJson["emissiveTexture"].get<std::string>();
			}

			m_presets.push_back( preset );
		}

		console::info( "Loaded {} material presets from {}", m_presets.size(), filePath );
		return true;
	}
	catch ( const json::exception &e )
	{
		console::error( "JSON error loading material presets: {}", e.what() );
		return false;
	}
}

bool MaterialPresetManager::saveToFile( const std::string &filePath ) const
{
	try
	{
		json j;
		json presetsArray = json::array();

		for ( const auto &preset : m_presets )
		{
			json presetJson;
			presetJson["name"] = preset.name;

			// Only save overrides that are present
			if ( preset.baseColorFactorOverride.has_value() )
			{
				const auto &color = preset.baseColorFactorOverride.value();
				presetJson["baseColorFactor"] = { color.x, color.y, color.z, color.w };
			}

			if ( preset.metallicFactorOverride.has_value() )
			{
				presetJson["metallicFactor"] = preset.metallicFactorOverride.value();
			}

			if ( preset.roughnessFactorOverride.has_value() )
			{
				presetJson["roughnessFactor"] = preset.roughnessFactorOverride.value();
			}

			if ( preset.emissiveFactorOverride.has_value() )
			{
				const auto &emissive = preset.emissiveFactorOverride.value();
				presetJson["emissiveFactor"] = { emissive.x, emissive.y, emissive.z };
			}

			if ( preset.baseColorTextureOverride.has_value() )
			{
				presetJson["baseColorTexture"] = preset.baseColorTextureOverride.value();
			}

			if ( preset.metallicRoughnessTextureOverride.has_value() )
			{
				presetJson["metallicRoughnessTexture"] = preset.metallicRoughnessTextureOverride.value();
			}

			if ( preset.normalTextureOverride.has_value() )
			{
				presetJson["normalTexture"] = preset.normalTextureOverride.value();
			}

			if ( preset.emissiveTextureOverride.has_value() )
			{
				presetJson["emissiveTexture"] = preset.emissiveTextureOverride.value();
			}

			presetsArray.push_back( presetJson );
		}

		j["materialPresets"] = presetsArray;

		std::ofstream file( filePath );
		if ( !file.is_open() )
		{
			console::error( "Failed to open material presets file for writing: {}", filePath );
			return false;
		}

		file << j.dump( 2 ); // Pretty print with 2-space indentation
		console::info( "Saved {} material presets to {}", m_presets.size(), filePath );
		return true;
	}
	catch ( const json::exception &e )
	{
		console::error( "JSON error saving material presets: {}", e.what() );
		return false;
	}
}

int MaterialPresetManager::findPresetIndex( const std::string &presetName ) const
{
	for ( size_t i = 0; i < m_presets.size(); ++i )
	{
		if ( m_presets[i].name == presetName )
		{
			return static_cast<int>( i );
		}
	}
	return -1;
}

} // namespace editor
