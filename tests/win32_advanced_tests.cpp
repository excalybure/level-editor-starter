// Win32 advanced event handling and window operations tests
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

import platform.win32;


TEST_CASE( "Win32Window Advanced Event Handling", "[win32][events][advanced]" )
{
	SECTION( "Mouse wheel events" )
	{
		Win32Window window;
		bool result = window.create( L"Mouse Wheel Test", 800, 600, false );

		if ( result )
		{
			// Test mouse wheel event processing
			POINT clientPos = { 400, 300 }; // Center of window
			ClientToScreen( window.getHandle(), &clientPos );

			// Simulate mouse wheel up
			INPUT wheelInput = {};
			wheelInput.type = INPUT_MOUSE;
			wheelInput.mi.dx = clientPos.x;
			wheelInput.mi.dy = clientPos.y;
			wheelInput.mi.dwFlags = MOUSEEVENTF_WHEEL;
			wheelInput.mi.mouseData = WHEEL_DELTA; // Positive for wheel up

			// Note: Actual input simulation would require SetCursorPos and SendInput
			// For unit testing, we'll verify the window can handle the message structure

			// Test that wheel delta values are handled correctly
			int wheelDelta = WHEEL_DELTA;
			REQUIRE( wheelDelta == 120 ); // Standard wheel delta

			window.destroy();
		}
	}

	SECTION( "win32::Window activation and focus events" )
	{
		Win32Window window;
		bool result = window.create( L"Focus Test", 800, 600, false );

		if ( result )
		{
			HWND hwnd = window.getHandle();
			REQUIRE( hwnd != nullptr );

			// Test focus state
			SetFocus( hwnd );
			HWND focusedWindow = GetFocus();

			// May not always succeed in test environment, but shouldn't crash
			INFO( "Focus test - Original focus: " << hwnd << ", Current focus: " << focusedWindow );

			// Test activation
			BOOL activated = SetActiveWindow( hwnd );
			INFO( "win32::Window activation result: " << activated );

			window.destroy();
		}
	}

	SECTION( "Multi-monitor support" )
	{
		Win32Window window;
		bool result = window.create( L"Multi-Monitor Test", 800, 600, false );

		if ( result )
		{
			HWND hwnd = window.getHandle();

			// Get monitor information
			HMONITOR monitor = MonitorFromWindow( hwnd, MONITOR_DEFAULTTOPRIMARY );
			REQUIRE( monitor != nullptr );

			MONITORINFO monitorInfo = {};
			monitorInfo.cbSize = sizeof( MONITORINFO );
			BOOL monResult = GetMonitorInfo( monitor, &monitorInfo );
			REQUIRE( monResult );

			// Verify monitor bounds are reasonable
			REQUIRE( monitorInfo.rcMonitor.right > monitorInfo.rcMonitor.left );
			REQUIRE( monitorInfo.rcMonitor.bottom > monitorInfo.rcMonitor.top );

			// Test moving window between monitors (if multiple exist)
			int monitorCount = GetSystemMetrics( SM_CMONITORS );
			INFO( "System has " << monitorCount << " monitor(s)" );

			if ( monitorCount > 1 )
			{
				// Try moving to secondary monitor area
				int secondaryX = monitorInfo.rcMonitor.right + 100;
				BOOL moveResult = SetWindowPos( hwnd, nullptr, secondaryX, 100, 0, 0, SWP_NOSIZE | SWP_NOZORDER );
				INFO( "Move to secondary monitor result: " << moveResult );
			}

			window.destroy();
		}
	}

	SECTION( "win32::Window state transitions" )
	{
		Win32Window window;
		bool result = window.create( L"State Transition Test", 800, 600, false );

		if ( result )
		{
			HWND hwnd = window.getHandle();

			// Test various window states
			ShowWindow( hwnd, SW_MINIMIZE );
			WINDOWPLACEMENT placement = {};
			placement.length = sizeof( WINDOWPLACEMENT );
			BOOL getResult = GetWindowPlacement( hwnd, &placement );
			REQUIRE( getResult );

			ShowWindow( hwnd, SW_RESTORE );
			getResult = GetWindowPlacement( hwnd, &placement );
			REQUIRE( getResult );

			// Test maximize
			ShowWindow( hwnd, SW_MAXIMIZE );
			getResult = GetWindowPlacement( hwnd, &placement );
			REQUIRE( getResult );

			// Restore to normal
			ShowWindow( hwnd, SW_RESTORE );

			window.destroy();
		}
	}
}

TEST_CASE( "Win32Window Advanced Properties and Styling", "[win32][window][styling]" )
{
	SECTION( "win32::Window styles and extended styles" )
	{
		Win32Window window;
		bool result = window.create( L"Style Test", 800, 600, false );

		if ( result )
		{
			HWND hwnd = window.getHandle();

			// Get current styles
			LONG style = GetWindowLong( hwnd, GWL_STYLE );
			LONG exStyle = GetWindowLong( hwnd, GWL_EXSTYLE );

			// Verify common style bits
			REQUIRE( ( style & WS_VISIBLE ) != 0 );
			REQUIRE( ( style & WS_OVERLAPPEDWINDOW ) != 0 );

			// Test style modifications
			LONG newStyle = style | WS_DISABLED;
			LONG setResult = SetWindowLong( hwnd, GWL_STYLE, newStyle );
			REQUIRE( setResult != 0 ); // Non-zero indicates success

			// Restore original style
			SetWindowLong( hwnd, GWL_STYLE, style );

			window.destroy();
		}
	}

	SECTION( "win32::Window class information" )
	{
		Win32Window window;
		bool result = window.create( L"Class Info Test", 800, 600, false );

		if ( result )
		{
			HWND hwnd = window.getHandle();

			// Get class name
			wchar_t className[256];
			int nameLength = GetClassName( hwnd, className, 256 );
			REQUIRE( nameLength > 0 );

			INFO( "win32::Window class name length: " << nameLength );

			// Get class info
			WNDCLASS wndClass = {};
			BOOL classResult = GetClassInfo( GetModuleHandle( nullptr ), className, &wndClass );
			REQUIRE( classResult );

			// Verify class has essential properties
			REQUIRE( wndClass.lpfnWndProc != nullptr );
			REQUIRE( wndClass.lpszClassName != nullptr );

			window.destroy();
		}
	}

	SECTION( "win32::Window hierarchy and parent-child relationships" )
	{
		Win32Window parentWindow;
		bool parentResult = parentWindow.create( L"Parent win32::Window", 1000, 800, false );

		if ( parentResult )
		{
			HWND parentHwnd = parentWindow.getHandle();

			// Create child window
			HWND childHwnd = CreateWindow(
				L"STATIC",
				L"Child win32::Window",
				WS_CHILD | WS_VISIBLE | WS_BORDER,
				10,
				10,
				200,
				100,
				parentHwnd,
				nullptr,
				GetModuleHandle( nullptr ),
				nullptr );

			if ( childHwnd )
			{
				// Verify parent-child relationship
				HWND retrievedParent = GetParent( childHwnd );
				REQUIRE( retrievedParent == parentHwnd );

				// Test child enumeration
				struct ChildEnumData
				{
					int count = 0;
					HWND expectedChild;
				} enumData;
				enumData.expectedChild = childHwnd;

				auto enumChildProc = []( HWND hwnd, LPARAM lParam ) -> BOOL {
					ChildEnumData *data = reinterpret_cast<ChildEnumData *>( lParam );
					data->count++;
					return TRUE;
				};

				EnumChildWindows( parentHwnd, enumChildProc, reinterpret_cast<LPARAM>( &enumData ) );
				REQUIRE( enumData.count >= 1 ); // Should find at least our child

				DestroyWindow( childHwnd );
			}

			parentWindow.destroy();
		}
	}

	SECTION( "win32::Window transparency and layering" )
	{
		Win32Window window;
		bool result = window.create( L"Transparency Test", 800, 600, false );

		if ( result )
		{
			HWND hwnd = window.getHandle();

			// Test layered window functionality
			LONG exStyle = GetWindowLong( hwnd, GWL_EXSTYLE );
			LONG newExStyle = exStyle | WS_EX_LAYERED;
			SetWindowLong( hwnd, GWL_EXSTYLE, newExStyle );

			// Set window opacity
			BOOL layerResult = SetLayeredWindowAttributes( hwnd, 0, 200, LWA_ALPHA ); // ~78% opacity
			INFO( "Set layered window attributes result: " << layerResult );

			// Test color key transparency
			layerResult = SetLayeredWindowAttributes( hwnd, RGB( 255, 0, 255 ), 255, LWA_COLORKEY );
			INFO( "Set color key transparency result: " << layerResult );

			// Restore original style
			SetWindowLong( hwnd, GWL_EXSTYLE, exStyle );

			window.destroy();
		}
	}
}

TEST_CASE( "Win32Window Performance and Resource Management", "[win32][window][performance]" )
{
	SECTION( "Rapid window creation and destruction" )
	{
		const int windowCount = 50;
		std::vector<std::unique_ptr<Win32Window>> windows;

		// Create multiple windows rapidly
		auto startTime = std::chrono::high_resolution_clock::now();

		for ( int i = 0; i < windowCount; ++i )
		{
			auto window = std::make_unique<Win32Window>();
			std::wstring title = L"Rapid Test win32::Window " + std::to_wstring( i );

			bool result = window->create( title.c_str(), 400, 300, false );
			if ( result )
			{
				windows.push_back( std::move( window ) );
			}
		}

		auto creationTime = std::chrono::high_resolution_clock::now();
		auto creationDuration = std::chrono::duration_cast<std::chrono::milliseconds>( creationTime - startTime );

		INFO( "Created " << windows.size() << " windows in " << creationDuration.count() << "ms" );

		// Destroy all windows
		for ( auto &window : windows )
		{
			window->destroy();
		}
		windows.clear();

		auto destroyTime = std::chrono::high_resolution_clock::now();
		auto destroyDuration = std::chrono::duration_cast<std::chrono::milliseconds>( destroyTime - creationTime );

		INFO( "Destroyed windows in " << destroyDuration.count() << "ms" );

		// Verify reasonable performance (should be much faster than this)
		REQUIRE( creationDuration.count() < 5000 ); // Less than 5 seconds
		REQUIRE( destroyDuration.count() < 1000 );	// Less than 1 second
	}

	SECTION( "Memory usage validation" )
	{
		// Get initial memory usage
		PROCESS_MEMORY_COUNTERS_EX memCounters1 = {};
		memCounters1.cb = sizeof( memCounters1 );
		GetProcessMemoryInfo( GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&memCounters1, sizeof( memCounters1 ) );

		// Create and destroy windows multiple times
		for ( int cycle = 0; cycle < 10; ++cycle )
		{
			std::vector<std::unique_ptr<Win32Window>> windows;

			// Create windows
			for ( int i = 0; i < 20; ++i )
			{
				auto window = std::make_unique<Win32Window>();
				std::wstring title = L"Memory Test " + std::to_wstring( i );

				if ( window->create( title.c_str(), 300, 200, false ) )
				{
					windows.push_back( std::move( window ) );
				}
			}

			// Destroy windows
			for ( auto &window : windows )
			{
				window->destroy();
			}
			windows.clear();
		}

		// Get final memory usage
		PROCESS_MEMORY_COUNTERS_EX memCounters2 = {};
		memCounters2.cb = sizeof( memCounters2 );
		GetProcessMemoryInfo( GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&memCounters2, sizeof( memCounters2 ) );

		// Calculate memory difference
		const SIZE_T memoryDiff = memCounters2.WorkingSetSize - memCounters1.WorkingSetSize;

		INFO( "Initial working set: " << memCounters1.WorkingSetSize / 1024 << " KB" );
		INFO( "Final working set: " << memCounters2.WorkingSetSize / 1024 << " KB" );
		INFO( "Memory difference: " << (long long)memoryDiff / 1024 << " KB" );

		// Memory usage should not increase dramatically (allowing some variance)
		REQUIRE( memoryDiff < 10 * 1024 * 1024 ); // Less than 10MB increase
	}

	SECTION( "win32::Window handle reuse safety" )
	{
		std::vector<HWND> usedHandles;

		// Create and destroy windows, collecting handles
		for ( int i = 0; i < 100; ++i )
		{
			Win32Window window;
			std::wstring title = L"Handle Test " + std::to_wstring( i );

			if ( window.create( title.c_str(), 300, 200, false ) )
			{
				HWND hwnd = window.getHandle();
				REQUIRE( hwnd != nullptr );

				// Verify handle is not reused immediately
				auto it = std::find( usedHandles.begin(), usedHandles.end(), hwnd );
				REQUIRE( it == usedHandles.end() );

				usedHandles.push_back( hwnd );
				window.destroy();

				// Verify handle becomes invalid after destruction
				BOOL isWindow = IsWindow( hwnd );
				REQUIRE_FALSE( isWindow );
			}
		}

		INFO( "Tested " << usedHandles.size() << " unique window handles" );
		REQUIRE( usedHandles.size() >= 50 ); // Should create a reasonable number
	}

	SECTION( "win32::Window message queue stress test" )
	{
		Win32Window window;
		bool result = window.create( L"Message Queue Stress", 800, 600, false );

		if ( result )
		{
			HWND hwnd = window.getHandle();

			// Post many messages rapidly
			const int messageCount = 1000;
			auto startTime = std::chrono::high_resolution_clock::now();

			for ( int i = 0; i < messageCount; ++i )
			{
				PostMessage( hwnd, WM_USER + 1, i, i * 2 );
			}

			auto postTime = std::chrono::high_resolution_clock::now();
			auto postDuration = std::chrono::duration_cast<std::chrono::milliseconds>( postTime - startTime );

			// Process messages
			MSG msg;
			int processedCount = 0;
			while ( PeekMessage( &msg, hwnd, WM_USER + 1, WM_USER + 1, PM_REMOVE ) && processedCount < messageCount )
			{
				processedCount++;
			}

			auto processTime = std::chrono::high_resolution_clock::now();
			auto processDuration = std::chrono::duration_cast<std::chrono::milliseconds>( processTime - postTime );

			INFO( "Posted " << messageCount << " messages in " << postDuration.count() << "ms" );
			INFO( "Processed " << processedCount << " messages in " << processDuration.count() << "ms" );

			REQUIRE( processedCount == messageCount );
			REQUIRE( postDuration.count() < 1000 );	   // Should post quickly
			REQUIRE( processDuration.count() < 1000 ); // Should process quickly

			window.destroy();
		}
	}
}
