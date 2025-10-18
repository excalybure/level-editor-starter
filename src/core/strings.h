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

/**
 * @brief Convert UTF-8 string to wide string (UTF-16 on Windows).
 * 
 * Uses Windows MultiByteToWideChar API for proper UTF-8 to UTF-16 conversion.
 * Required for DXC compiler and other Windows APIs that use wide strings.
 * 
 * @param str UTF-8 encoded string to convert
 * @return Wide string (std::wstring)
 */
std::wstring toWideString( const std::string &str );

/**
 * @brief Convert wide string (UTF-16) to UTF-8 string.
 * 
 * Uses Windows WideCharToMultiByte API for proper UTF-16 to UTF-8 conversion.
 * Required for converting DXC output and Windows API strings to UTF-8.
 * 
 * @param wstr Wide string (UTF-16) to convert
 * @return UTF-8 encoded string
 */
std::string toNarrowString( const std::wstring &wstr );

} // namespace strings
