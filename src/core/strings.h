#pragma once

#include <string>

namespace strings
{

/**
 * @brief Extract the base filename from a file path.
 * 
 * Removes the directory path and file extension from a full file path.
 * 
 * Examples:
 *   - "assets/scenes/test.gltf" -> "test"
 *   - "C:\\Users\\test\\model.gltf" -> "model"
 *   - "simple.gltf" -> "simple"
 *   - "assets/no_extension" -> "no_extension"
 *   - "" -> ""
 * 
 * @param filePath The full file path to process
 * @return The base filename without directory or extension
 */
std::string getBaseFilename( const std::string &filePath );

/**
 * @brief Extract the directory path from a file path.
 * 
 * Returns the directory portion of a file path, excluding the filename.
 * 
 * Examples:
 *   - "assets/scenes/test.gltf" -> "assets/scenes"
 *   - "C:\\Users\\test\\model.gltf" -> "C:\\Users\\test"
 *   - "simple.gltf" -> ""
 *   - "assets/scenes/" -> "assets/scenes"
 *   - "" -> ""
 * 
 * @param filePath The full file path to process
 * @return The directory path without the filename
 */
std::string getDirectoryPath( const std::string &filePath );

} // namespace strings
