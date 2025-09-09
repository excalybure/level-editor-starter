// Global module fragment for Windows headers
module;

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>

export module engine.material_gpu;

import std;
import engine.assets;
import engine.vec;
import platform.dx12;
import engine.shader_manager;
import runtime.console;

export namespace engine
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
	// Constructor taking assets::Material reference
	explicit MaterialGPU( const std::shared_ptr<assets::Material> &material );

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
	ID3D12PipelineState *getPipelineState() const { return m_pipelineState.Get(); }
	const MaterialConstants &getMaterialConstants() const { return m_materialConstants; }

	// Validation methods
	bool isValid() const { return m_isValid; }

	// Material source access
	std::shared_ptr<assets::Material> getSourceMaterial() const { return m_material; }

private:
	std::shared_ptr<assets::Material> m_material;
	MaterialConstants m_materialConstants;

	// D3D12 GPU resources
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;

	// Resource state
	bool m_isValid = false;

	// Helper methods
	void createPipelineState();
	void createConstantBuffer();
	void updateMaterialConstants();
	void loadTextures();
};

} // namespace engine