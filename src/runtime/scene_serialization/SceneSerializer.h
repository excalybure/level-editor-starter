#pragma once

#include <string>
#include <filesystem>
#include <expected>
#include <nlohmann/json_fwd.hpp>
#include "runtime/ecs.h"

namespace scene
{

// Error types for scene serialization
enum class SerializationError
{
	FileNotFound,
	FileAccessDenied,
	InvalidJSON,
	UnsupportedVersion,
	MissingRequiredField,
	InvalidHierarchy,
	AssetLoadFailed,
	Unknown
};

// Error message struct
struct SerializationErrorInfo
{
	SerializationError error;
	std::string message;
	std::string filePath;
	int lineNumber = -1; // For JSON parse errors
};

// Scene metadata
struct SceneMetadata
{
	std::string name = "Untitled Scene";
	std::string created;  // ISO 8601 timestamp
	std::string modified; // ISO 8601 timestamp
	std::string author = "Level Editor";
	std::string version = "1.0";
};

// Static serialization class
class SceneSerializer
{
public:
	// Save scene to JSON file
	static std::expected<void, SerializationErrorInfo> saveScene(
		const ecs::Scene &scene,
		const std::filesystem::path &filepath,
		const SceneMetadata &metadata = {} );

	// Load scene from JSON file
	static std::expected<void, SerializationErrorInfo> loadScene(
		ecs::Scene &scene,
		const std::filesystem::path &filepath );

	// Get metadata from file without loading full scene
	static std::expected<SceneMetadata, SerializationErrorInfo> getSceneMetadata(
		const std::filesystem::path &filepath );

	// Validate scene file format
	static std::expected<void, SerializationErrorInfo> validateSceneFile(
		const std::filesystem::path &filepath );

private:
	SceneSerializer() = delete; // Static class only

	// Utility functions
	static std::string getCurrentISO8601Timestamp();
	static SerializationErrorInfo makeError( SerializationError error, const std::string &message, const std::filesystem::path &filepath = {}, int lineNumber = -1 );
};

// Helper to convert error to string for display
inline std::string serializationErrorToString( SerializationError error )
{
	switch ( error )
	{
	case SerializationError::FileNotFound:
		return "File not found";
	case SerializationError::FileAccessDenied:
		return "File access denied";
	case SerializationError::InvalidJSON:
		return "Invalid JSON format";
	case SerializationError::UnsupportedVersion:
		return "Unsupported scene version";
	case SerializationError::MissingRequiredField:
		return "Missing required field";
	case SerializationError::InvalidHierarchy:
		return "Invalid entity hierarchy";
	case SerializationError::AssetLoadFailed:
		return "Asset load failed";
	case SerializationError::Unknown:
		return "Unknown error";
	default:
		return "Unspecified error";
	}
}

} // namespace scene
