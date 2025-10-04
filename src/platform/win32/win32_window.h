#pragma once

#if defined( _WIN32 )
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <queue>

namespace platform
{

// Window event data
struct WindowEvent
{
	enum class Type
	{
		None,
		Close,
		Resize,
		Focus,
		LostFocus,
		MouseMove,
		MouseButton,
		MouseWheel,
		KeyPress,
		KeyRelease
	};

	Type type = Type::None;

	// Mouse data
	struct MouseData
	{
		float x = 0.0f, y = 0.0f;
		float deltaX = 0.0f, deltaY = 0.0f;
		int button = 0; // 0=left, 1=right, 2=middle
		bool pressed = false;
		float wheelDelta = 0.0f;
	} mouse;

	// Keyboard data
	struct KeyboardData
	{
		int keycode = 0;
		bool pressed = false;
		bool ctrl = false;
		bool shift = false;
		bool alt = false;
	} keyboard;

	// Resize data
	struct ResizeData
	{
		int width = 0;
		int height = 0;
	} resize;
};

class Win32Window
{
public:
	Win32Window() = default;
	~Win32Window();

	// No copy/move for simplicity
	Win32Window( const Win32Window & ) = delete;
	Win32Window &operator=( const Win32Window & ) = delete;

	// Create the window
	// @param visible: if false, window is created hidden (useful for unit tests)
	bool create( const char *title, int width, int height, bool visible = true );

	// Pump messages and return if window should close
	bool poll();

	// Get the next event (returns false if no events)
	bool getEvent( WindowEvent &event );

	// Window properties
	void getSize( int &width, int &height ) const;
	bool isFocused() const { return m_focused; }

	// Native handles for graphics API integration
	HWND getHandle() const { return m_hwnd; }
	HINSTANCE getInstance() const { return m_hinstance; }

private:
	HWND m_hwnd = nullptr;
	HINSTANCE m_hinstance = nullptr;
	bool m_shouldClose = false;
	bool m_focused = true;
	int m_width = 0;
	int m_height = 0;

	// Event queue
	std::queue<WindowEvent> m_eventQueue;

	// Win32 window procedure
	static LRESULT CALLBACK windowProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

	// Helper to add events to queue
	void addEvent( const WindowEvent &event );
};

} // namespace platform
