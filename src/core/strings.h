#pragma once

#include <string>

namespace core
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

} // namespace core
