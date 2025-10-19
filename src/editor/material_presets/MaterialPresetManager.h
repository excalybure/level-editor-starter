#pragma once

#include <string>
#include <vector>
#include <optional>
#include "engine/assets/assets.h"

namespace editor
{

/**
 * @brief Manager for material preset library
 * 
 * Handles storage, retrieval, and persistence of material presets.
 * Presets are saved in editor configuration file.
 */
class MaterialPresetManager
{
public:
	/**
	 * @brief Construct a preset manager
	 */
	MaterialPresetManager() = default;

	/**
	 * @brief Add a new preset to the library
	 * @param preset The preset to add
	 * @return true if added successfully, false if preset with same name already exists
	 */
	bool addPreset( const assets::MaterialPreset &preset );

	/**
	 * @brief Remove a preset from the library
	 * @param presetName Name of the preset to remove
	 * @return true if removed successfully, false if preset not found
	 */
	bool removePreset( const std::string &presetName );

	/**
	 * @brief Get a preset by name
	 * @param presetName Name of the preset to retrieve
	 * @return Preset if found, nullopt otherwise
	 */
	std::optional<assets::MaterialPreset> getPreset( const std::string &presetName ) const;

	/**
	 * @brief Get all preset names
	 * @return Vector of preset names
	 */
	std::vector<std::string> getPresetNames() const;

	/**
	 * @brief Get all presets
	 * @return Vector of all presets
	 */
	const std::vector<assets::MaterialPreset> &getAllPresets() const { return m_presets; }

	/**
	 * @brief Check if a preset exists
	 * @param presetName Name of the preset
	 * @return true if preset exists
	 */
	bool hasPreset( const std::string &presetName ) const;

	/**
	 * @brief Clear all presets
	 */
	void clear();

	/**
	 * @brief Load presets from JSON file
	 * @param filePath Path to JSON file
	 * @return true if loaded successfully
	 */
	bool loadFromFile( const std::string &filePath );

	/**
	 * @brief Save presets to JSON file
	 * @param filePath Path to JSON file
	 * @return true if saved successfully
	 */
	bool saveToFile( const std::string &filePath ) const;

private:
	std::vector<assets::MaterialPreset> m_presets;

	/**
	 * @brief Find preset index by name
	 * @param presetName Name of the preset
	 * @return Index if found, -1 otherwise
	 */
	int findPresetIndex( const std::string &presetName ) const;
};

} // namespace editor
