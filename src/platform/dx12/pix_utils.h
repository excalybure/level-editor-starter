// PIX Debugging utilities for D3D12
// Copyright (c) 2025 Level Editor Project

#pragma once

// TODO: Remove
#if 0

#include <d3d12.h>
#include <string>
#include <format>

// PIX for Windows - optional, fallback to no-ops if not available
#ifdef PIX_AVAILABLE
#include <pix3.h>
#else
// Stub implementations when PIX is not available
#define PIXBeginEvent( cmdList, color, name ) ( (void)0 )
#define PIXEndEvent( cmdList )				  ( (void)0 )
#define PIXSetMarker( cmdList, color, name )  ( (void)0 )
#endif

namespace pix
{
// Color constants for PIX markers
enum class Color : UINT64
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

// RAII PIX event marker
class ScopedEvent
{
private:
	ID3D12GraphicsCommandList *m_commandList;
	bool m_active;

public:
	ScopedEvent( ID3D12GraphicsCommandList *commandList, Color color, const std::string &name )
		: m_commandList( commandList ), m_active( commandList != nullptr )
	{
		if ( m_active )
		{
			const std::wstring wname( name.begin(), name.end() );
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

// Simple marker utility
inline void SetMarker( ID3D12GraphicsCommandList *commandList, Color color, const std::string &name )
{
	if ( commandList )
	{
		const std::wstring wname( name.begin(), name.end() );
		PIXSetMarker( commandList, static_cast<UINT64>( color ), wname.c_str() );
	}
}

// Check if PIX is available
inline bool IsAvailable()
{
#ifdef PIX_AVAILABLE
	return true;
#else
	return false;
#endif
}
} // namespace pix

#endif