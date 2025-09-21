#pragma once

#include <catch2/catch_test_macros.hpp>
#if defined( _WIN32 )
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include "platform/dx12/dx12_device.h"
#include "runtime/console.h"
#include "platform/win32/win32_window.h"

#include <utility> // for std::pair

// Helper to create a Win32Window and initialize a dx12::Device for integration tests.
// Returns a std::pair<HWND, dx12::Device> if successful, otherwise emits a WARN and returns {nullptr, {}}.
inline bool requireDevice( platform::Win32Window &window, dx12::Device &device, const char *windowTitle = "UI Test", int width = 640, int height = 480 )
{
	if ( !window.create( windowTitle, width, height ) )
	{
		console::error( "Skipping UI integration: failed to create Win32 window" );
		return false;
	}

	if ( !device.initialize( static_cast<HWND>( window.getHandle() ) ) )
	{
		console::error( "Skipping UI integration: D3D12 initialize failed (hardware not available)" );
		return false;
	}

	return true;
}

// Utility to attempt headless initialization and emit a WARN then early-return from a test section
// Usage:
//   dx12::Device device;
//   if(!requireHeadlessDevice(device)) return; // inside a lambda passed to REQUIRE_NOTHROW or directly in TEST_CASE
inline bool requireHeadlessDevice( dx12::Device &device, const char *context = nullptr )
{
	if ( device.initializeHeadless() )
		return true;
	if ( context )
		console::error( "Skipping {}: headless device initialization failed", context );
	else
		console::error( "Skipping test: headless device initialization failed" );
	return false;
}
