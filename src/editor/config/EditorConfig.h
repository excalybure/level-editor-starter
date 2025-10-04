#pragma once

#include <string>
#include <vector>
#include <nlohmann/json_fwd.hpp>
#include <memory>

namespace editor
{

/**
 * @brief Configuration system for persisting editor settings
 * 
 * Handles loading/saving JSON configuration files with hierarchical key-value pairs.
 * Complements ImGui's imgui.ini by storing window visibility and user preferences.
 */
class EditorConfig
{
public:
	/**
	 * @brief Construct config with default file path
	 */
	EditorConfig();

	/**
	 * @brief Construct config with custom file path
	 * @param filePath Path to JSON configuration file
	 */
	explicit EditorConfig( const std::string &filePath );

	/**
	 * @brief Destructor
	 */
	~EditorConfig();

	// No copy/move for now (may add later if needed)
	EditorConfig( const EditorConfig & ) = delete;
	EditorConfig &operator=( const EditorConfig & ) = delete;

	/**
	 * @brief Get the configuration file path
	 * @return File path string
	 */
	const std::string &getFilePath() const noexcept;

	/**
	 * @brief Load configuration from file
	 * @return true if loaded successfully, false otherwise
	 */
	bool load();

	/**
	 * @brief Save configuration to file
	 * @return true if saved successfully, false otherwise
	 */
	bool save();

	/**
	 * @brief Get boolean value from configuration
	 * @param key Hierarchical key path (e.g., "ui.panels.hierarchy")
	 * @param defaultValue Value to return if key doesn't exist
	 * @return Boolean value or default
	 */
	bool getBool( const std::string &key, bool defaultValue ) const;

	/**
	 * @brief Set boolean value in configuration
	 * @param key Hierarchical key path (e.g., "ui.panels.hierarchy")
	 * @param value Boolean value to store
	 */
	void setBool( const std::string &key, bool value );

private:
	std::string m_filePath;
	std::unique_ptr<nlohmann::json> m_data;

	/**
	 * @brief Split dot-notation key into path segments
	 * @param key Dot-separated key path
	 * @return Vector of path segments
	 */
	std::vector<std::string> splitKey( const std::string &key ) const;
};

} // namespace editor
