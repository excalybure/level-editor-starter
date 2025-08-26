module; // global module fragment (stuff here is outside the module)

#if defined( _WIN32 )
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

export module platform.win32.win32_window;
import std;

export namespace platform
{
export class Win32Window
{
public:
	void create( const char *, int, int )
	{
		// TODO: Real Win32 window creation
	}
	void poll()
	{
		// TODO: Pump messages
	}
};
} // namespace platform
