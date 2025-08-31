// Global module fragment - headers go here
module;

#include <iostream>
#include <string>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define NOMINMAX
#endif

export module runtime.console;

export namespace console
{
// Console output functions with color coding
void fatal( const std::string &message );
void error( const std::string &message );
void warning( const std::string &message );
void info( const std::string &message );
void debug( const std::string &message );

// Overloads for C-style strings
void fatal( const char *message );
void error( const char *message );
void warning( const char *message );
void info( const char *message );
void debug( const char *message );
} // namespace console
