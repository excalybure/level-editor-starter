#pragma once

#include <string>

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

private:
	std::string m_filePath;
};

} // namespace editor
