// Custom test runner main for Catch2
// Initializes DXC compiler before running tests

#define CATCH_CONFIG_RUNNER
#include <catch2/catch_session.hpp>

#include "graphics/shader_manager/shader_compiler.h"
#include "core/console.h"

int main( int argc, char *argv[] )
{
	// Initialize DXC compiler once before all tests
	try
	{
		shader_manager::ShaderCompiler::InitializeDxc();
		console::info( "DXC compiler initialized successfully for test suite" );
	}
	catch ( const std::exception &e )
	{
		console::error( "Failed to initialize DXC compiler: {}", e.what() );
		console::error( "Some shader tests may fail" );
		// Continue anyway - some tests don't require DXC
	}

	// Run Catch2 test session
	const int result = Catch::Session().run( argc, argv );

	return result;
}
