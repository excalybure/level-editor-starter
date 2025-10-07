#include "win32_window.h"

#if defined( _WIN32 )
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <string>
#include "imgui.h"
#include "imgui_impl_win32.h"
#endif

#if defined( _WIN32 )
// Forward declare the ImGui Win32 message handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
#endif

namespace platform
{

Win32Window::~Win32Window()
{
	if ( m_hwnd )
	{
		DestroyWindow( m_hwnd );
		m_hwnd = nullptr;
	}
}

bool Win32Window::create( const char *title, int width, int height, bool visible )
{
#if defined( _WIN32 )
	// Prevent double creation on the same instance (would otherwise leak the old HWND)
	if ( m_hwnd )
		return false;
	m_hinstance = GetModuleHandle( nullptr );
	m_width = width;
	m_height = height;

	// Register window class
	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof( WNDCLASSEXW );
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = windowProc;
	wc.hInstance = m_hinstance;
	wc.hCursor = LoadCursor( nullptr, IDC_ARROW );
	wc.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
	wc.lpszClassName = L"WorldEditorWindow";

	// RegisterClassExW returns 0 on failure, but attempting to register the same class
	// multiple times across tests or multiple windows is benign. Accept ERROR_CLASS_ALREADY_EXISTS.
	if ( !RegisterClassExW( &wc ) )
	{
		const DWORD err = GetLastError();
		if ( err != ERROR_CLASS_ALREADY_EXISTS )
			return false; // genuine failure
	}

	// Determine window dimensions and style
	// Always create in windowed mode initially (fullscreen can be applied after creation)
	DWORD windowStyle = visible ? WS_OVERLAPPEDWINDOW : WS_OVERLAPPEDWINDOW;
	m_isFullscreen = false; // Start windowed

	// Adjust window size to account for borders/title bar to get correct client area
	RECT rect = { 0, 0, width, height };
	AdjustWindowRect( &rect, windowStyle, FALSE );
	const int windowWidth = rect.right - rect.left;
	const int windowHeight = rect.bottom - rect.top;

	// Store the requested dimensions (not screen dimensions)
	m_width = width;
	m_height = height;

	// Convert title to wide string
	const int title_len = MultiByteToWideChar( CP_UTF8, 0, title, -1, nullptr, 0 );
	std::wstring wide_title( title_len, 0 );
	MultiByteToWideChar( CP_UTF8, 0, title, -1, wide_title.data(), title_len );

	// Create window with determined dimensions and style
	m_hwnd = CreateWindowExW(
		0,
		L"WorldEditorWindow",
		wide_title.c_str(),
		windowStyle,
		0,
		0,
		windowWidth,
		windowHeight,
		nullptr,
		nullptr,
		m_hinstance,
		this // Pass this pointer for window proc
	);

	if ( !m_hwnd )
		return false;

	// Show or hide window based on visible parameter
	ShowWindow( m_hwnd, visible ? SW_SHOW : SW_HIDE );

	UpdateWindow( m_hwnd );

	return true;
#else
	return false;
#endif
}

bool Win32Window::poll()
{
#if defined( _WIN32 )
	MSG msg;
	while ( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );

		if ( msg.message == WM_QUIT )
			m_shouldClose = true;
	}

	return !m_shouldClose;
#else
	return false;
#endif
}

bool Win32Window::getEvent( WindowEvent &event )
{
	if ( m_eventQueue.empty() )
		return false;

	event = m_eventQueue.front();
	m_eventQueue.pop();
	return true;
}

void Win32Window::getSize( int &width, int &height ) const
{
	width = m_width;
	height = m_height;
}

void Win32Window::setTitle( const char *title )
{
	if ( m_hwnd && title )
	{
		SetWindowTextA( m_hwnd, title );
	}
}

void Win32Window::addEvent( const WindowEvent &event )
{
	m_eventQueue.push( event );
}

void Win32Window::setFullscreen( bool fullscreen )
{
#if defined( _WIN32 )
	if ( !m_hwnd || m_isFullscreen == fullscreen )
		return; // No change needed

	if ( fullscreen )
	{
		// Transitioning to fullscreen
		// Save current windowed position and size
		RECT rect;
		GetWindowRect( m_hwnd, &rect );
		m_savedX = rect.left;
		m_savedY = rect.top;
		m_savedWidth = m_width;
		m_savedHeight = m_height;

		// Get screen dimensions
		const int screenWidth = GetSystemMetrics( SM_CXSCREEN );
		const int screenHeight = GetSystemMetrics( SM_CYSCREEN );

		// Change to popup style (no borders)
		SetWindowLongW( m_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE );

		// Move to top-left corner and resize to screen dimensions
		SetWindowPos( m_hwnd, HWND_TOP, 0, 0, screenWidth, screenHeight, SWP_FRAMECHANGED );

		// Update internal size tracking
		m_width = screenWidth;
		m_height = screenHeight;
		m_isFullscreen = true;
	}
	else
	{
		// Transitioning to windowed mode
		// Restore windowed style (with borders)
		SetWindowLongW( m_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE );

		// Calculate window dimensions including borders/title bar
		RECT rect = { 0, 0, m_savedWidth, m_savedHeight };
		AdjustWindowRect( &rect, WS_OVERLAPPEDWINDOW, FALSE );
		const int windowWidth = rect.right - rect.left;
		const int windowHeight = rect.bottom - rect.top;

		// Restore position and size
		SetWindowPos( m_hwnd, nullptr, m_savedX, m_savedY, windowWidth, windowHeight, SWP_FRAMECHANGED | SWP_NOZORDER );

		// Update internal size tracking
		m_width = m_savedWidth;
		m_height = m_savedHeight;
		m_isFullscreen = false;
	}
#endif
}

#if defined( _WIN32 )
LRESULT CALLBACK Win32Window::windowProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
	// Forward messages to ImGui first
	if ( ImGui_ImplWin32_WndProcHandler( hwnd, msg, wparam, lparam ) )
		return true;

	Win32Window *window = nullptr;

	if ( msg == WM_NCCREATE )
	{
		// Store the window instance pointer
		CREATESTRUCT *create_struct = reinterpret_cast<CREATESTRUCT *>( lparam );
		window = static_cast<Win32Window *>( create_struct->lpCreateParams );
		SetWindowLongPtr( hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>( window ) );
	}
	else
	{
		// Retrieve the window instance pointer
		window = reinterpret_cast<Win32Window *>( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );
	}

	if ( window )
	{
		WindowEvent event;

		switch ( msg )
		{
		case WM_CLOSE:
			event.type = WindowEvent::Type::Close;
			window->addEvent( event );
			window->m_shouldClose = true;
			return 0;

		case WM_SIZE: {
			int width = LOWORD( lparam );
			int height = HIWORD( lparam );
			// Only enqueue a resize event if the client size actually changed
			if ( width == window->m_width && height == window->m_height )
			{
				return 0; // no-op resize
			}
			window->m_width = width;
			window->m_height = height;

			event.type = WindowEvent::Type::Resize;
			event.resize.width = width;
			event.resize.height = height;
			window->addEvent( event );
		}
			return 0;

		case WM_SETFOCUS:
			window->m_focused = true;
			event.type = WindowEvent::Type::Focus;
			window->addEvent( event );
			return 0;

		case WM_KILLFOCUS:
			window->m_focused = false;
			event.type = WindowEvent::Type::LostFocus;
			window->addEvent( event );
			return 0;

		case WM_MOUSEMOVE:
			event.type = WindowEvent::Type::MouseMove;
			event.mouse.x = static_cast<float>( GET_X_LPARAM( lparam ) );
			event.mouse.y = static_cast<float>( GET_Y_LPARAM( lparam ) );
			window->addEvent( event );
			return 0;

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP: {
			// Unified mouse button handling (DOWN/UP)
			event.type = WindowEvent::Type::MouseButton;
			event.mouse.x = static_cast<float>( GET_X_LPARAM( lparam ) );
			event.mouse.y = static_cast<float>( GET_Y_LPARAM( lparam ) );
			// Map button index
			int buttonIndex = -1;
			switch ( msg )
			{
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
				buttonIndex = 0;
				break;
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
				buttonIndex = 1;
				break;
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
				buttonIndex = 2;
				break;
			default:
				break;
			}
			event.mouse.button = buttonIndex;
			// Pressed flag based on message variant
			event.mouse.pressed = ( msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN );
			window->addEvent( event );
			return 0;
		}

		case WM_MOUSEWHEEL:
			event.type = WindowEvent::Type::MouseWheel;
			event.mouse.wheelDelta = static_cast<float>( GET_WHEEL_DELTA_WPARAM( wparam ) ) / WHEEL_DELTA;
			window->addEvent( event );
			return 0;

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			event.type = WindowEvent::Type::KeyPress;
			event.keyboard.keycode = static_cast<int>( wparam );
			event.keyboard.pressed = true;
			event.keyboard.ctrl = ( GetKeyState( VK_CONTROL ) & 0x8000 ) != 0;
			event.keyboard.shift = ( GetKeyState( VK_SHIFT ) & 0x8000 ) != 0;
			event.keyboard.alt = ( GetKeyState( VK_MENU ) & 0x8000 ) != 0;
			window->addEvent( event );
			return 0;

		case WM_KEYUP:
		case WM_SYSKEYUP:
			event.type = WindowEvent::Type::KeyRelease;
			event.keyboard.keycode = static_cast<int>( wparam );
			event.keyboard.pressed = false;
			event.keyboard.ctrl = ( GetKeyState( VK_CONTROL ) & 0x8000 ) != 0;
			event.keyboard.shift = ( GetKeyState( VK_SHIFT ) & 0x8000 ) != 0;
			event.keyboard.alt = ( GetKeyState( VK_MENU ) & 0x8000 ) != 0;
			window->addEvent( event );
			return 0;

		case WM_DPICHANGED: {
			// Window's DPI has changed - adjust window size
			RECT *suggested_rect = reinterpret_cast<RECT *>( lparam );
			SetWindowPos( hwnd, nullptr, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE );
			return 0;
		}
		}
	}

	return DefWindowProcW( hwnd, msg, wparam, lparam );
}
#endif

} // namespace platform
