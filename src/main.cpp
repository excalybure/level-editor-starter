import runtime.app;
import platform.win32.win32_window;
import platform.dx12;
import editor.ui;
#include <iostream>

int main()
{
	// Create the window
	platform::Win32Window window;
	if ( !window.create( "Level Editor - Multi-Viewport", 1600, 900 ) )
	{
		std::cerr << "Failed to create window\n";
		return 1;
	}

	// Initialize D3D12 device
	dx12::Device device;
	if ( !device.initialize( static_cast<HWND>( window.getHandle() ) ) )
	{
		std::cerr << "Failed to initialize D3D12 device\n";
		return 1;
	}

	// Initialize UI system with D3D12 device
	editor::UI ui;
	if ( !ui.initialize(
			 static_cast<void *>( window.getHandle() ),
			 &device ) )
	{
		std::cerr << "Failed to initialize UI system\n";
		return 1;
	}

	// Create application
	app::App app;

	std::cout << "Starting Level Editor with ImGui Docking...\n";

	// Main loop with exception handling
	try
	{
		while ( window.poll() && !ui.shouldExit() )
		{
			// Begin D3D12 frame
			device.beginFrame();
			// Begin UI frame (sets up docking)
			ui.beginFrame();
			// End UI frame (renders ImGui)
			ui.endFrame();
			// Submit ImGui draw data to command list
			ui.renderDrawData( static_cast<void *>( device.getCommandList() ) );
			// End D3D12 frame
			device.endFrame();
			// Present D3D12 frame
			device.present();

			// App tick
			app.tick();
		}
	}
	catch ( const std::exception &e )
	{
		std::cerr << "Exception caught in main loop: " << e.what() << std::endl;
		std::cerr << "Application will exit with error code 1" << std::endl;

		// Cleanup before exit
		ui.shutdown();
		device.shutdown();
		return 1;
	}
	catch ( ... )
	{
		std::cerr << "Unknown exception caught in main loop" << std::endl;
		std::cerr << "Application will exit with error code 1" << std::endl;

		// Cleanup before exit
		ui.shutdown();
		device.shutdown();
		return 1;
	}

	// Cleanup
	ui.shutdown();
	device.shutdown();

	return 0;
}
