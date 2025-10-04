#include "strings/strings.h"

namespace strings
{

std::string getBaseFilename( const std::string &filePath )
{
	// Handle empty string
	if ( filePath.empty() )
	{
		return {};
	}

	// Start with the full path
	std::string baseFilename = filePath;

	// Extract base filename from path (remove directory and extension)
	// Find the last slash (forward or backward)
	const size_t lastSlash = baseFilename.find_last_of( "/\\" );
	if ( lastSlash != std::string::npos )
	{
		baseFilename = baseFilename.substr( lastSlash + 1 );
	}

	// Handle case where path ends with a slash (empty filename)
	if ( baseFilename.empty() )
	{
		return {};
	}

	// Remove extension (everything after the last dot)
	// But preserve hidden files (files starting with dot)
	const size_t lastDot = baseFilename.find_last_of( '.' );
	if ( lastDot != std::string::npos && lastDot != 0 )
	{
		baseFilename = baseFilename.substr( 0, lastDot );
	}

	return baseFilename;
}

} // namespace strings
