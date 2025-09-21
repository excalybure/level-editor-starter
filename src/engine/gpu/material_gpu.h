#pragma once

#include <cstdint>
#include <d3d12.h>
#include <memory>
#include <wrl.h>

#include "engine/math/vec.h"

// Forward declarations
namespace assets
{
class Material;
}
namespace dx12
{
class Device;
} // namespace dx12

namespace engine::gpu
{

// MaterialConstants struct matching shader expectations for PBR materials
struct MaterialConstants
{
	math::Vec4f baseColorFactor;
	float metallicFactor = 0.0f;
	float roughnessFactor = 1.0f;
	float padding1 = 0.0f;
	float padding2 = 0.0f;
	math::Vec3f emissiveFactor;
	float padding3 = 0.0f;

	// Texture binding flags - used to indicate which textures are bound
	uint32_t textureFlags = 0; // Bitfield for texture availability
	uint32_t padding4[3] = { 0, 0, 0 };

	// Constructor to initialize Vec4f and Vec3f
	MaterialConstants()
		: baseColorFactor( 1.0f, 1.0f, 1.0f, 1.0f ), emissiveFactor( 0.0f, 0.0f, 0.0f ) {}

	static constexpr uint32_t kBaseColorTextureBit = 1 << 0;
	static constexpr uint32_t kMetallicRoughnessTextureBit = 1 << 1;
	static constexpr uint32_t kNormalTextureBit = 1 << 2;
	static constexpr uint32_t kEmissiveTextureBit = 1 << 3;
};

// MaterialGPU class for managing GPU resources for a material
class MaterialGPU
{
public:
	// Constructor taking assets::Material reference (material-only mode)
	explicit MaterialGPU( const std::shared_ptr<assets::Material> &material );

	// Constructor taking assets::Material and device for GPU resource creation
	MaterialGPU( const std::shared_ptr<assets::Material> &material, dx12::Device &device );

	// Move constructor and assignment
	MaterialGPU( MaterialGPU &&other ) noexcept;
	MaterialGPU &operator=( MaterialGPU &&other ) noexcept;

	// Disable copy
	MaterialGPU( const MaterialGPU & ) = delete;
	MaterialGPU &operator=( const MaterialGPU & ) = delete;

	// Destructor
	~MaterialGPU();

	// Bind all GPU resources to command list for rendering
	void bindToCommandList( ID3D12GraphicsCommandList *commandList ) const;

	// Resource accessor methods
	const MaterialConstants &getMaterialConstants() const { return m_materialConstants; }

	// Validation methods
	bool isValid() const { return m_isValid; }

	// Material source access
	std::shared_ptr<assets::Material> getSourceMaterial() const { return m_material; }

private:
	std::shared_ptr<assets::Material> m_material;
	MaterialConstants m_materialConstants;
	dx12::Device *m_device = nullptr; // Optional device for GPU resource creation

	// D3D12 GPU resources
	Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;

	// Resource state
	bool m_isValid = false;

	// Helper methods
	void createConstantBuffer();
	void updateMaterialConstants();
	void loadTextures();
};

} // namespace engine::gpu
