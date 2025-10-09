#include "core/strings.h"

#include <algorithm>

namespace strings
{

std::string getBaseFilename( const std::string &filePath )
{
	if ( filePath.empty() )
	{
		return "";
	}

	// Find the last directory separator (forward or backward slash)
	const size_t lastSlash = filePath.find_last_of( "/\\" );
	const size_t start = ( lastSlash == std::string::npos ) ? 0 : lastSlash + 1;

	// Find the last dot for the extension
	const size_t lastDot = filePath.find_last_of( '.' );

	// If no extension found, or dot is before the last slash, return everything after the slash
	if ( lastDot == std::string::npos || lastDot < start )
	{
		return filePath.substr( start );
	}

	// Return the substring between the last slash and the last dot
	return filePath.substr( start, lastDot - start );
}

} // namespace strings
