// Global module fragment - headers go here
module;

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <windows.h>
#include <combaseapi.h>
#include <winerror.h>
#include <comdef.h>

export module engine.gpu.mesh_gpu;

import std;
import platform.dx12;
import engine.assets;
import engine.gpu.material_gpu;
import runtime.console;

export namespace engine::gpu
{

// Interface for providing MaterialGPU objects from MaterialHandles
export class MaterialProvider
{
public:
	virtual ~MaterialProvider() = default;
	virtual std::shared_ptr<MaterialGPU> getMaterialGPU( std::shared_ptr<assets::Material> material ) = 0;
	virtual std::shared_ptr<MaterialGPU> getDefaultMaterialGPU() = 0;
};

// Individual primitive GPU buffer management
export class PrimitiveGPU
{
public:
	PrimitiveGPU( dx12::Device &device, const assets::Primitive &primitive );
	~PrimitiveGPU() = default;

	// No copy/move for now to keep resource management simple
	PrimitiveGPU( const PrimitiveGPU & ) = delete;
	PrimitiveGPU &operator=( const PrimitiveGPU & ) = delete;

	// Buffer view accessors for rendering
	D3D12_VERTEX_BUFFER_VIEW getVertexBufferView() const noexcept { return m_vertexBufferView; }
	D3D12_INDEX_BUFFER_VIEW getIndexBufferView() const noexcept { return m_indexBufferView; }

	// Resource count accessors
	std::uint32_t getVertexCount() const noexcept { return m_vertexCount; }
	std::uint32_t getIndexCount() const noexcept { return m_indexCount; }

	// Direct resource access for advanced usage
	ID3D12Resource *getVertexResource() const noexcept { return m_vertexBuffer.Get(); }
	ID3D12Resource *getIndexResource() const noexcept { return m_indexBuffer.Get(); }

	// Material access
	std::shared_ptr<MaterialGPU> getMaterial() const noexcept { return m_material; }
	bool hasMaterial() const noexcept { return static_cast<bool>( m_material ); }
	void setMaterial( std::shared_ptr<MaterialGPU> material ) noexcept { m_material = std::move( material ); }

	// Complete resource binding for rendering (geometry + material)
	void bindForRendering( ID3D12GraphicsCommandList *commandList ) const;

	// Check if buffers were created successfully
	bool isValid() const noexcept { return m_vertexBuffer && m_indexBuffer; }
	bool hasIndexBuffer() const noexcept { return static_cast<bool>( m_indexBuffer ); }

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;

	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView = {};

	std::uint32_t m_vertexCount = 0;
	std::uint32_t m_indexCount = 0;

	dx12::Device &m_device;
	std::shared_ptr<MaterialGPU> m_material;

	// Helper methods for buffer creation
	void createVertexBuffer( const assets::Primitive &primitive );
	void createIndexBuffer( const assets::Primitive &primitive );

	// Helper method to create upload heap buffers
	Microsoft::WRL::ComPtr<ID3D12Resource> createUploadBuffer( std::size_t bufferSize, const void *data );
};

// Collection of GPU buffers for all primitives in a mesh
export class MeshGPU
{
public:
	MeshGPU( dx12::Device &device, const assets::Mesh &mesh );

	~MeshGPU() = default;

	// No copy/move for now
	MeshGPU( const MeshGPU & ) = delete;
	MeshGPU &operator=( const MeshGPU & ) = delete;

	// Access individual primitive buffers
	std::uint32_t getPrimitiveCount() const noexcept { return static_cast<std::uint32_t>( m_primitives.size() ); }

	const PrimitiveGPU &getPrimitive( std::uint32_t index ) const;
	PrimitiveGPU &getPrimitive( std::uint32_t index );

	// Check if all primitive buffers are valid
	bool isValid() const noexcept;

	// Configure materials for primitives using GPUResourceManager and Scene
	// Material configuration using dependency injection
	void configureMaterials( MaterialProvider &materialProvider, const assets::Scene &scene, const assets::Mesh &mesh );

private:
	std::vector<std::unique_ptr<PrimitiveGPU>> m_primitives;
	dx12::Device &m_device;
};

} // namespace engine::gpu