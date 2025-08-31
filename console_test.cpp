import runtime.console;
#include <iostream>

int main()
{
	console::info( "Console module test started" );
	console::debug( "This is a debug message - should appear in blue" );
	console::warning( "This is a warning message - should appear in yellow" );
	console::error( "This is an error message - should appear in red" );
	console::info( "All console functions tested successfully" );

	// Test C-string overloads
	console::debug( "Testing C-string overload" );
	console::info( "Test with const char*" );

	std::cout << "\nPress Enter to test fatal() function (this will exit)...\n";
	std::cin.get();

	console::fatal( "This is a fatal error - should appear in red and exit" );

	// This line should never be reached
	std::cout << "This should not be printed\n";

	return 0;
}
