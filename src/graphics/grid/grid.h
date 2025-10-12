#pragma once

// Grid rendering system - infinite world-space grid with adaptive density
// Copyright (c) 2025 Level Editor Project

#include <cstring>
#include <d3d12.h>
#include <memory>
#include <wrl.h>
#include "math/vec.h"
#include "math/matrix.h"
#include "graphics/material_system/material_system.h"
#include "graphics/material_system/material_instance.h"

namespace dx12
{
class Device;
}

namespace camera
{
class Camera;
}

namespace grid
{

// Grid rendering parameters
struct GridSettings
{
	// Grid appearance
	math::Vec3<> majorGridColor = { 0.5f, 0.5f, 0.5f };
	float majorGridAlpha = 0.8f;

	math::Vec3<> minorGridColor = { 0.3f, 0.3f, 0.3f };
	float minorGridAlpha = 0.4f;

	// Axis colors (X=Red, Y=Green, Z=Blue)
	math::Vec3<> axisXColor = { 1.0f, 0.2f, 0.2f };
	float axisXAlpha = 1.0f;

	math::Vec3<> axisYColor = { 0.2f, 1.0f, 0.2f };
	float axisYAlpha = 1.0f;

	math::Vec3<> axisZColor = { 0.2f, 0.2f, 1.0f };
	float axisZAlpha = 1.0f;

	// Grid properties
	float gridSpacing = 1.0f;			 // Units per grid line
	float majorGridInterval = 10.0f;	 // Major grid every N minor lines
	float fadeDistanceMultiplier = 5.0f; // Multiplier for camera distance to determine fade distance (fadeDistance = cameraDistance * multiplier)
	float axisThickness = 2.0f;			 // Thickness of axis lines

	// Visibility flags
	bool showGrid = true;
	bool showAxes = true;

	// Adaptive zoom settings
	float zoomThreshold = 0.1f; // When to switch grid density
};

// Forward declarations
class GridRenderer;

// Grid rendering system
class GridRenderer
{
public:
	GridRenderer();
	~GridRenderer();

	// No copy/move for now
	GridRenderer( const GridRenderer & ) = delete;
	GridRenderer &operator=( const GridRenderer & ) = delete;

	// Initialize the grid renderer with D3D12 device and material system
	bool initialize( dx12::Device *device, graphics::material_system::MaterialSystem *materialSystem );
	void shutdown();

	// Get the cached material handle for grid_material
	graphics::material_system::MaterialHandle getMaterialHandle() const { return m_materialHandle; }

	// Render the grid for a specific viewport
	bool render( const camera::Camera &camera,
		const math::Mat4<> &viewMatrix,
		const math::Mat4<> &projMatrix,
		float viewportWidth,
		float viewportHeight );

	// Settings management
	void setSettings( const GridSettings &settings ) { m_settings = settings; }
	const GridSettings &getSettings() const { return m_settings; }
	GridSettings &getSettings() { return m_settings; }

	// Adaptive grid density based on camera distance/zoom
	void updateAdaptiveSpacing( const camera::Camera &camera );

	// Utility functions
	static float calculateOptimalSpacing( const float cameraDistance, const float baseSpacing = 1.0f );
	static int calculateMajorInterval( const float spacing );

private:
	// D3D12 resources
	dx12::Device *m_device = nullptr;

	// Material system integration
	graphics::material_system::MaterialSystem *m_materialSystem = nullptr;
	graphics::material_system::MaterialHandle m_materialHandle;
	std::unique_ptr<graphics::material_system::MaterialInstance> m_materialInstance;

	// Constant buffer
	Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
	void *m_constantBufferData = nullptr;

	// Grid settings
	GridSettings m_settings;

	// Helper functions
	bool createConstantBuffer();

	void updateConstantBuffer( const camera::Camera &camera,
		const math::Mat4<> &viewMatrix,
		const math::Mat4<> &projMatrix,
		float viewportWidth,
		float viewportHeight );

	math::Mat4<> calculateInverseViewProjMatrix( const math::Mat4<> &viewMatrix,
		const math::Mat4<> &projMatrix ) const;
};

// Utility functions for grid calculations
namespace GridUtils
{
// Calculate appropriate grid spacing for a given camera distance
float calculateAdaptiveSpacing( const float cameraDistance, const float baseSpacing = 1.0f );

// Calculate major grid interval based on spacing
int calculateMajorInterval( const float spacing );

// Check if a point is on a grid line
bool isOnGridLine( const math::Vec2<> &point, const float spacing, const float tolerance = 0.01f );

// Snap a point to the nearest grid intersection
math::Vec2<> snapToGrid( const math::Vec2<> &point, const float spacing );
math::Vec3<> snapToGrid( const math::Vec3<> &point, const float spacing );

// Calculate grid bounds for a given view frustum
struct GridBounds
{
	math::Vec2<> min;
	math::Vec2<> max;
	float optimalSpacing;
	int majorInterval;
};

GridBounds calculateGridBounds( const camera::Camera &camera,
	const math::Mat4<> &viewMatrix,
	const math::Mat4<> &projMatrix,
	const float viewportWidth,
	const float viewportHeight );

// Grid color utilities
math::Vec3<> getAxisColor( const int axis ); // 0=X(red), 1=Y(green), 2=Z(blue)
float calculateGridFade( const math::Vec3<> &worldPos,
	const math::Vec3<> &cameraPos,
	const float fadeDistance );
} // namespace GridUtils

} // namespace grid
