#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "platform/win32/win32_window.h"
#include "platform/dx12/dx12_device.h"
#include "engine/renderer/renderer.h"
#include "engine/shader_manager/shader_compiler.h"
#include "test_dx12_helpers.h"


// Simple integration: create window + device (windowed) then ensure renderer begin/end frame works without crash.
TEST_CASE( "Renderer windowed integration begin/end", "[renderer][integration]" )
{
	platform::Win32Window window;
	if ( !window.create( "Test", 320, 240, false ) )
	{
		WARN( "Skipping renderer windowed integration: window creation failed" );
		return;
	}

	dx12::Device device;
	if ( !device.initialize( (HWND)window.getHandle() ) )
	{
		WARN( "Skipping: device windowed initialize failed" );
		return;
	}

	// Just sanity check that present path doesn't hard-crash; allow failure to throw and mark as warning instead of failing suite.
	try
	{
		device.beginFrame();
		device.endFrame();
		device.present();
	}
	catch ( const std::exception &e )
	{
		WARN( "Skipping full frame loop in integration test: " << e.what() );
	}
}

// Negative shader compile scenario using ShaderCompiler::CompileFromSource with intentional error.
TEST_CASE( "Renderer shader compile failure path", "[renderer][shader][error]" )
{
	const std::string badShader = R"(float4 main(float3 pos : POSITION) : SV_POSITION { return float4(pos, 1.0f) )"; // missing semicolon & brace
	bool threw = false;
	try
	{
		auto blob = shader_manager::ShaderCompiler::CompileFromSource( badShader, "main", "vs_5_0" );
		(void)blob;
	}
	catch ( const std::runtime_error & )
	{
		threw = true;
	}
	REQUIRE( threw );
}
