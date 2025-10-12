#pragma once

#include "graphics/material_system/material_system.h"
#include <string>
#include <d3d12.h>
#include <wrl/client.h>

// Forward declarations
namespace dx12
{
class Device;
}

namespace graphics::material_system
{

// Represents a runtime instance of a material with cached GPU resources for all passes
// Handles PSO/root signature lifecycle, hot-reloading, and multi-pass management
// Note: Does NOT manage constant buffers - that's the caller's responsibility
class MaterialInstance
{
public:
	// Create material instance from material ID
	// device: DX12 device for PSO/root signature creation
	// materialSystem: Material system for querying definitions
	// materialId: Material ID to look up (e.g., "grid_material", "pbr_material")
	MaterialInstance(
		dx12::Device *device,
		MaterialSystem *materialSystem,
		const std::string &materialId );

	~MaterialInstance();

	// No copy (manages GPU resources and callbacks)
	MaterialInstance( const MaterialInstance & ) = delete;
	MaterialInstance &operator=( const MaterialInstance & ) = delete;

	// Check if material instance is valid (material found with at least one pass)
	bool isValid() const;

	// Check if material has specific pass
	bool hasPass( const std::string &passName ) const;

	// Query material definition (nullptr if invalid)
	const MaterialDefinition *getMaterial() const;

	// Query specific material pass (nullptr if not found)
	const MaterialPass *getPass( const std::string &passName ) const;

	// Get material handle
	MaterialHandle getHandle() const { return m_materialHandle; }

	// Get root signature (shared across all passes)
	// Returns nullptr if root signature not created or material invalid
	ID3D12RootSignature *getRootSignature() const;

private:
	dx12::Device *m_device = nullptr;
	MaterialSystem *m_materialSystem = nullptr;
	MaterialHandle m_materialHandle;

	// Single root signature shared by all passes
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
};

} // namespace graphics::material_system
