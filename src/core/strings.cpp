#include "core/strings.h"

#include <algorithm>

#ifdef _WIN32
#include <Windows.h>
#endif

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

	// If no extension found, or dot is at/before the directory separator, treat as no extension
	if ( lastDot == std::string::npos || lastDot <= start )
	{
		return filePath.substr( start );
	}

	// Return the substring between the last slash and the last dot
	return filePath.substr( start, lastDot - start );
}

std::string getDirectoryPath( const std::string &filePath )
{
	if ( filePath.empty() )
	{
		return "";
	}

	// Find the last directory separator (forward or backward slash)
	const size_t lastSlash = filePath.find_last_of( "/\\" );

	// If no separator found, there's no directory path
	if ( lastSlash == std::string::npos )
	{
		return "";
	}

	// Return the substring up to (but not including) the last slash
	return filePath.substr( 0, lastSlash );
}

std::wstring toWideString( const std::string &str )
{
	if ( str.empty() )
	{
		return {};
	}

#ifdef _WIN32
	const int sizeNeeded = MultiByteToWideChar( CP_UTF8, 0, str.data(), static_cast<int>( str.size() ), nullptr, 0 );
	std::wstring result( sizeNeeded, 0 );
	MultiByteToWideChar( CP_UTF8, 0, str.data(), static_cast<int>( str.size() ), result.data(), sizeNeeded );
	return result;
#else
	// Fallback for non-Windows platforms (basic conversion)
	return std::wstring( str.begin(), str.end() );
#endif
}

std::string toNarrowString( const std::wstring &wstr )
{
	if ( wstr.empty() )
	{
		return {};
	}

#ifdef _WIN32
	const int sizeNeeded = WideCharToMultiByte( CP_UTF8, 0, wstr.data(), static_cast<int>( wstr.size() ), nullptr, 0, nullptr, nullptr );
	std::string result( sizeNeeded, 0 );
	WideCharToMultiByte( CP_UTF8, 0, wstr.data(), static_cast<int>( wstr.size() ), result.data(), sizeNeeded, nullptr, nullptr );
	return result;
#else
	// Fallback for non-Windows platforms (basic conversion)
	return std::string( wstr.begin(), wstr.end() );
#endif
}

} // namespace strings
