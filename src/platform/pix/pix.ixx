// PIX Markers utility for D3D12 debugging
// Copyright (c) 2025 Level Editor Project

module;

#ifdef _WIN32
#include <windows.h>
#include <d3d12.h>
#include <wrl.h>
#include <string>

// PIX for Windows header - only include if available
// Set to 0 for now since PIX SDK is not installed
#define PIX_AVAILABLE 0

#if PIX_AVAILABLE
#include <pix3.h>
#else
// Fallback definitions when PIX is not available
inline void PIXBeginEvent( ID3D12CommandQueue *, UINT64, PCWSTR )
{
}
inline void PIXEndEvent( ID3D12CommandQueue * )
{
}
inline void PIXBeginEvent( ID3D12GraphicsCommandList *, UINT64, PCWSTR )
{
}
inline void PIXEndEvent( ID3D12GraphicsCommandList * )
{
}
inline void PIXSetMarker( ID3D12CommandQueue *, UINT64, PCWSTR )
{
}
inline void PIXSetMarker( ID3D12GraphicsCommandList *, UINT64, PCWSTR )
{
}
#endif
#endif

export module platform.pix;

import std;

export namespace pix
{
enum class MarkerColor : UINT64
{
	Red = 0xFF0000FF,
	Green = 0xFF00FF00,
	Blue = 0xFFFF0000,
	Yellow = 0xFF00FFFF,
	Magenta = 0xFFFF00FF,
	Cyan = 0xFFFFFF00,
	White = 0xFFFFFFFF,
	Orange = 0xFF0080FF,
	Purple = 0xFF800080,
	LightBlue = 0xFFFFB366,
	LightGreen = 0xFF80FF80,
	LightRed = 0xFF8080FF
};

// RAII PIX event marker for command lists
class ScopedEvent
{
private:
	ID3D12GraphicsCommandList *m_commandList;
	bool m_active;

public:
	ScopedEvent( ID3D12GraphicsCommandList *commandList, MarkerColor color, const std::string &name )
		: m_commandList( commandList ), m_active( commandList != nullptr )
	{
		if ( m_active )
		{
			std::wstring wname( name.begin(), name.end() );
			PIXBeginEvent( m_commandList, static_cast<UINT64>( color ), wname.c_str() );
		}
	}

	~ScopedEvent()
	{
		if ( m_active )
		{
			PIXEndEvent( m_commandList );
		}
	}

	// Non-copyable
	ScopedEvent( const ScopedEvent & ) = delete;
	ScopedEvent &operator=( const ScopedEvent & ) = delete;
};

// RAII PIX event marker for command queues
class ScopedQueueEvent
{
private:
	ID3D12CommandQueue *m_commandQueue;
	bool m_active;

public:
	ScopedQueueEvent( ID3D12CommandQueue *commandQueue, MarkerColor color, const std::string &name )
		: m_commandQueue( commandQueue ), m_active( commandQueue != nullptr )
	{
		if ( m_active )
		{
			std::wstring wname( name.begin(), name.end() );
			PIXBeginEvent( m_commandQueue, static_cast<UINT64>( color ), wname.c_str() );
		}
	}

	~ScopedQueueEvent()
	{
		if ( m_active )
		{
			PIXEndEvent( m_commandQueue );
		}
	}

	// Non-copyable
	ScopedQueueEvent( const ScopedQueueEvent & ) = delete;
	ScopedQueueEvent &operator=( const ScopedQueueEvent & ) = delete;
};

// Utility functions for simple markers
inline void SetMarker( ID3D12GraphicsCommandList *commandList, MarkerColor color, const std::string &name )
{
	if ( commandList )
	{
		std::wstring wname( name.begin(), name.end() );
		PIXSetMarker( commandList, static_cast<UINT64>( color ), wname.c_str() );
	}
}

inline void SetMarker( ID3D12CommandQueue *commandQueue, MarkerColor color, const std::string &name )
{
	if ( commandQueue )
	{
		std::wstring wname( name.begin(), name.end() );
		PIXSetMarker( commandQueue, static_cast<UINT64>( color ), wname.c_str() );
	}
}

// Utility functions for beginning and ending events
inline void BeginEvent( ID3D12GraphicsCommandList *commandList, MarkerColor color, const std::string &name )
{
	if ( commandList )
	{
		std::wstring wname( name.begin(), name.end() );
		PIXBeginEvent( commandList, static_cast<UINT64>( color ), wname.c_str() );
	}
}

inline void EndEvent( ID3D12GraphicsCommandList *commandList )
{
	if ( commandList )
	{
		PIXEndEvent( commandList );
	}
}

inline void BeginEvent( ID3D12CommandQueue *commandQueue, MarkerColor color, const std::string &name )
{
	if ( commandQueue )
	{
		std::wstring wname( name.begin(), name.end() );
		PIXBeginEvent( commandQueue, static_cast<UINT64>( color ), wname.c_str() );
	}
}

inline void EndEvent( ID3D12CommandQueue *commandQueue )
{
	if ( commandQueue )
	{
		PIXEndEvent( commandQueue );
	}
}

inline bool IsAvailable()
{
	return PIX_AVAILABLE;
}
} // namespace pix
