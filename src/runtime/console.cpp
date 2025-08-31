// Global module fragment - headers go here
module;

#include <iostream>
#include <string>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#endif

module runtime.console;

#ifndef _WIN32
// Define WORD type for non-Windows platforms
typedef unsigned short WORD;
#endif

namespace console
{
namespace
{
// ANSI color codes
constexpr const char *RESET = "\033[0m";
constexpr const char *RED = "\033[31m";
constexpr const char *YELLOW = "\033[33m";
constexpr const char *GRAY = "\033[90m";
constexpr const char *BLUE = "\033[34m";

// Windows console color attributes
#ifdef _WIN32
constexpr WORD WIN_RED = FOREGROUND_RED | FOREGROUND_INTENSITY;
constexpr WORD WIN_YELLOW = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
constexpr WORD WIN_GRAY = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
constexpr WORD WIN_BLUE = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
constexpr WORD WIN_WHITE = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;

void setConsoleColor( WORD color )
{
	static HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
	SetConsoleTextAttribute( hConsole, color );
}

void resetConsoleColor()
{
	setConsoleColor( WIN_WHITE );
}
#endif

void printWithColor( const std::string &message, [[maybe_unused]] const char *ansiColor = nullptr, [[maybe_unused]] WORD winColor = 0 )
{
#ifdef _WIN32
	// Use Windows Console API for better compatibility
	if ( winColor != 0 )
	{
		setConsoleColor( winColor );
	}
	std::cout << message << std::flush;
	if ( winColor != 0 )
	{
		resetConsoleColor();
	}
#else
	// Use ANSI colors on non-Windows platforms
	if ( ansiColor )
	{
		std::cout << ansiColor << message << RESET << std::flush;
	}
	else
	{
		std::cout << message << std::flush;
	}
#endif
}
} // namespace

void fatal( const std::string &message )
{
	std::cout << "[FATAL] ";
#ifdef _WIN32
	printWithColor( message, RED, WIN_RED );
#else
	printWithColor( message, RED, 0 );
#endif
	std::cout << std::endl;
	std::exit( 1 );
}

void error( const std::string &message )
{
	std::cout << "[ERROR] ";
#ifdef _WIN32
	printWithColor( message, RED, WIN_RED );
#else
	printWithColor( message, RED, 0 );
#endif
	std::cout << std::endl;
}

void warning( const std::string &message )
{
	std::cout << "[WARNING] ";
#ifdef _WIN32
	printWithColor( message, YELLOW, WIN_YELLOW );
#else
	printWithColor( message, YELLOW, 0 );
#endif
	std::cout << std::endl;
}

void info( const std::string &message )
{
	std::cout << "[INFO] ";
#ifdef _WIN32
	printWithColor( message, GRAY, WIN_GRAY );
#else
	printWithColor( message, GRAY, 0 );
#endif
	std::cout << std::endl;
}

void debug( const std::string &message )
{
	std::cout << "[DEBUG] ";
#ifdef _WIN32
	printWithColor( message, BLUE, WIN_BLUE );
#else
	printWithColor( message, BLUE, 0 );
#endif
	std::cout << std::endl;
}

// C-string overloads
void fatal( const char *message )
{
	fatal( std::string( message ) );
}
void error( const char *message )
{
	error( std::string( message ) );
}
void warning( const char *message )
{
	warning( std::string( message ) );
}
void info( const char *message )
{
	info( std::string( message ) );
}
void debug( const char *message )
{
	debug( std::string( message ) );
}
} // namespace console
