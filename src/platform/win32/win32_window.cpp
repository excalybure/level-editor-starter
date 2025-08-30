// Global module fragment - headers go here
module;

#if defined( _WIN32 )
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <string>
#endif

module platform.win32.win32_window;

import std;

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

bool Win32Window::create( const char *title, int width, int height )
{
#if defined( _WIN32 )
	m_hinstance = GetModuleHandle( nullptr );
	m_width = width;
	m_height = height;

	// Register window class
	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof( WNDCLASSEXW );
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = window_proc;
	wc.hInstance = m_hinstance;
	wc.hCursor = LoadCursor( nullptr, IDC_ARROW );
	wc.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
	wc.lpszClassName = L"LevelEditorWindow";

	if ( !RegisterClassExW( &wc ) )
		return false;

	// Calculate window size including borders
	RECT rect = { 0, 0, width, height };
	AdjustWindowRect( &rect, WS_OVERLAPPEDWINDOW, FALSE );

	// Convert title to wide string
	int title_len = MultiByteToWideChar( CP_UTF8, 0, title, -1, nullptr, 0 );
	std::wstring wide_title( title_len, 0 );
	MultiByteToWideChar( CP_UTF8, 0, title, -1, wide_title.data(), title_len );

	// Create window
	m_hwnd = CreateWindowExW(
		0,
		L"LevelEditorWindow",
		wide_title.c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rect.right - rect.left,
		rect.bottom - rect.top,
		nullptr,
		nullptr,
		m_hinstance,
		this // Pass this pointer for window proc
	);

	if ( !m_hwnd )
		return false;

	ShowWindow( m_hwnd, SW_SHOW );
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
			m_should_close = true;
	}

	return !m_should_close;
#else
	return false;
#endif
}

bool Win32Window::getEvent( WindowEvent &event )
{
	if ( m_event_queue.empty() )
		return false;

	event = m_event_queue.front();
	m_event_queue.pop();
	return true;
}

void Win32Window::getSize( int &width, int &height ) const
{
	width = m_width;
	height = m_height;
}

void Win32Window::add_event( const WindowEvent &event )
{
	m_event_queue.push( event );
}

#if defined( _WIN32 )
LRESULT CALLBACK Win32Window::window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
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
			window->add_event( event );
			window->m_should_close = true;
			return 0;

		case WM_SIZE: {
			int width = LOWORD( lparam );
			int height = HIWORD( lparam );
			window->m_width = width;
			window->m_height = height;

			event.type = WindowEvent::Type::Resize;
			event.resize.width = width;
			event.resize.height = height;
			window->add_event( event );
		}
			return 0;

		case WM_SETFOCUS:
			window->m_focused = true;
			event.type = WindowEvent::Type::Focus;
			window->add_event( event );
			return 0;

		case WM_KILLFOCUS:
			window->m_focused = false;
			event.type = WindowEvent::Type::LostFocus;
			window->add_event( event );
			return 0;

		case WM_MOUSEMOVE:
			event.type = WindowEvent::Type::MouseMove;
			event.mouse.x = static_cast<float>( GET_X_LPARAM( lparam ) );
			event.mouse.y = static_cast<float>( GET_Y_LPARAM( lparam ) );
			window->add_event( event );
			return 0;

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
			event.type = WindowEvent::Type::MouseButton;
			event.mouse.x = static_cast<float>( GET_X_LPARAM( lparam ) );
			event.mouse.y = static_cast<float>( GET_Y_LPARAM( lparam ) );
			switch ( msg )
			{
			case WM_LBUTTONDOWN:
				event.mouse.button = 0;
				break;
			case WM_RBUTTONDOWN:
				event.mouse.button = 1;
				break;
			case WM_MBUTTONDOWN:
				event.mouse.button = 2;
				break;
			default:
				event.mouse.button = -1;
				break;
			}
			event.mouse.pressed = true;
			window->add_event( event );
			return 0;
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
			event.type = WindowEvent::Type::MouseButton;
			event.mouse.x = static_cast<float>( GET_X_LPARAM( lparam ) );
			event.mouse.y = static_cast<float>( GET_Y_LPARAM( lparam ) );
			switch ( msg )
			{
			case WM_LBUTTONUP:
				event.mouse.button = 0;
				break;
			case WM_RBUTTONUP:
				event.mouse.button = 1;
				break;
			case WM_MBUTTONUP:
				event.mouse.button = 2;
				break;
			default:
				event.mouse.button = -1;
				break;
			}
			event.mouse.pressed = false;
			window->add_event( event );
			return 0;

		case WM_MOUSEWHEEL:
			event.type = WindowEvent::Type::MouseWheel;
			event.mouse.wheelDelta = static_cast<float>( GET_WHEEL_DELTA_WPARAM( wparam ) ) / WHEEL_DELTA;
			window->add_event( event );
			return 0;

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			event.type = WindowEvent::Type::KeyPress;
			event.keyboard.keycode = static_cast<int>( wparam );
			event.keyboard.pressed = true;
			event.keyboard.ctrl = ( GetKeyState( VK_CONTROL ) & 0x8000 ) != 0;
			event.keyboard.shift = ( GetKeyState( VK_SHIFT ) & 0x8000 ) != 0;
			event.keyboard.alt = ( GetKeyState( VK_MENU ) & 0x8000 ) != 0;
			window->add_event( event );
			return 0;

		case WM_KEYUP:
		case WM_SYSKEYUP:
			event.type = WindowEvent::Type::KeyRelease;
			event.keyboard.keycode = static_cast<int>( wparam );
			event.keyboard.pressed = false;
			event.keyboard.ctrl = ( GetKeyState( VK_CONTROL ) & 0x8000 ) != 0;
			event.keyboard.shift = ( GetKeyState( VK_SHIFT ) & 0x8000 ) != 0;
			event.keyboard.alt = ( GetKeyState( VK_MENU ) & 0x8000 ) != 0;
			window->add_event( event );
			return 0;
		}
	}

	return DefWindowProc( hwnd, msg, wparam, lparam );
}
#endif

} // namespace platform
