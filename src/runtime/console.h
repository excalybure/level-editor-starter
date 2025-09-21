#pragma once

#include <iostream>
#include <string>
#include <cstdlib>
#include <format>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#endif

namespace console
{
// Console output functions with color coding
void fatal( const std::string &message );
void error( const std::string &message );
void errorAndThrow( const std::string &message ); // Prints error and throws std::runtime_error
void warning( const std::string &message );
void info( const std::string &message );
void debug( const std::string &message );

// Overloads for C-style strings
void fatal( const char *message );
void error( const char *message );
void errorAndThrow( const char *message ); // Prints error and throws std::runtime_error
void warning( const char *message );
void info( const char *message );
void debug( const char *message );

// Variadic template functions with std::format
template <typename... Args>
void fatal( std::format_string<Args...> fmt, Args &&...args )
{
	fatal( std::format( fmt, std::forward<Args>( args )... ) );
}

template <typename... Args>
void error( std::format_string<Args...> fmt, Args &&...args )
{
	error( std::format( fmt, std::forward<Args>( args )... ) );
}

template <typename... Args>
void errorAndThrow( std::format_string<Args...> fmt, Args &&...args )
{
	errorAndThrow( std::format( fmt, std::forward<Args>( args )... ) );
}

template <typename... Args>
void warning( std::format_string<Args...> fmt, Args &&...args )
{
	warning( std::format( fmt, std::forward<Args>( args )... ) );
}

template <typename... Args>
void info( std::format_string<Args...> fmt, Args &&...args )
{
	info( std::format( fmt, std::forward<Args>( args )... ) );
}

template <typename... Args>
void debug( std::format_string<Args...> fmt, Args &&...args )
{
	debug( std::format( fmt, std::forward<Args>( args )... ) );
}
} // namespace console
