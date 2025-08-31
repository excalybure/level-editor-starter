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

	// Initialize UI system
	editor::UI ui;
	if ( !ui.initialize(
			 static_cast<void *>( window.getHandle() ),
			 static_cast<void *>( device.getDevice() ),
			 static_cast<void *>( device.getImguiDescriptorHeap() ) ) )
	{
		std::cerr << "Failed to initialize UI system\n";
		return 1;
	}

	// Create application
	app::App app;

	std::cout << "Starting Level Editor with ImGui Docking...\n";

	// Main loop
	while ( window.poll() )
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

	// Cleanup
	ui.shutdown();
	device.shutdown();

	std::cout << "Exit level editor (ImGui docking integrated)\n";
	return 0;
}
