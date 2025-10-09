#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <unordered_set>
#include <filesystem>

namespace material_system
{

class JsonLoader
{
public:
	JsonLoader() = default;

	// Load JSON from file with include resolution
	// Returns true on success, false on error (logs details via console)
	bool load( const std::string &rootPath );

	// Get the merged JSON document (valid only after successful load)
	const nlohmann::json &getDocument() const { return m_mergedDocument; }
	const nlohmann::json &getMergedDocument() const { return m_mergedDocument; } // Alias for clarity

	// Get list of error messages (populated on load failure)
	const std::vector<std::string> &getErrors() const { return m_errors; }

private:
	// Recursive include loading with cycle detection
	bool loadRecursive( const std::filesystem::path &filePath,
		std::vector<std::filesystem::path> &includeStack );

	// Merge included JSON into main document
	void mergeDocument( const nlohmann::json &source );

	nlohmann::json m_mergedDocument;
	std::vector<std::string> m_errors;
	std::unordered_set<std::string> m_loadedFiles;
};

} // namespace material_system
