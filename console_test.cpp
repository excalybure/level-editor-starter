#include "src/runtime/console.h"
#include <iostream>
#include <string>
#include <vector>

int main()
{
	console::info( "=== Console Module Demo ===" );

	// Basic string functions
	console::info( "Basic string messages:" );
	console::debug( "This is a debug message" );
	console::warning( "This is a warning message" );
	console::error( "This is an error message" );

	// Variadic template functions with std::format
	console::info( "\n=== Formatted Messages ===" );

	int count = 42;
	float percentage = 75.5f;
	std::string username = "Developer";

	console::info( "User {} has {} items ({:.1f}% complete)", username, count, percentage );
	console::debug( "Memory allocated: {}KB, free: {}KB", 1024, 512 );
	console::warning( "CPU usage at {:.1f}%, temperature: {}°C", 85.2f, 72 );
	console::error( "Failed to load {} files out of {} total", 3, count );

	// Number formatting examples
	console::info( "\n=== Number Formatting ===" );
	int value = 255;
	console::debug( "Decimal: {}, Hex: {:#x}, Binary: {:#b}", value, value, value );
	console::info( "Padded numbers: {:04d}, {:>8.2f}", 42, 3.14159f );

	// Container formatting
	console::info( "\n=== Advanced Usage ===" );
	std::vector<int> values = { 1, 2, 3, 4, 5 };
	console::debug( "Vector size: {}, first element: {}", values.size(), values[0] );

	console::info( "\nPress Enter to test fatal() function (this will exit)..." );
	std::cin.get();

	console::fatal( "Application terminated with error code: {}", 1 );

	// This line should never be reached
	std::cout << "This should not be printed\n";

	return 0;
}
