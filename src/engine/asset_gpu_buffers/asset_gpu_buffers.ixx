// Global module fragment - headers go here
module;

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <windows.h>
#include <combaseapi.h>
#include <winerror.h>
#include <comdef.h>

export module engine.asset_gpu_buffers;

import std;
import platform.dx12;
import engine.assets;
import engine.material_gpu;
import runtime.console;

export namespace asset_gpu_buffers
{

// Individual primitive GPU buffer management
export class PrimitiveGPUBuffer
{
public:
	PrimitiveGPUBuffer( dx12::Device &device, const assets::Primitive &primitive );
	PrimitiveGPUBuffer( dx12::Device &device, const assets::Primitive &primitive, std::shared_ptr<engine::MaterialGPU> material );
	~PrimitiveGPUBuffer() = default;

	// No copy/move for now to keep resource management simple
	PrimitiveGPUBuffer( const PrimitiveGPUBuffer & ) = delete;
	PrimitiveGPUBuffer &operator=( const PrimitiveGPUBuffer & ) = delete;

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
	std::shared_ptr<engine::MaterialGPU> getMaterial() const noexcept { return m_material; }
	bool hasMaterial() const noexcept { return static_cast<bool>( m_material ); }

	// Complete resource binding for rendering (geometry + material)
	void bindForRendering( ID3D12GraphicsCommandList *commandList ) const;

	// Check if buffers were created successfully
	bool isValid() const noexcept { return m_vertexBuffer && m_indexBuffer; }

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;

	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView = {};

	std::uint32_t m_vertexCount = 0;
	std::uint32_t m_indexCount = 0;

	dx12::Device &m_device;
	std::shared_ptr<engine::MaterialGPU> m_material;

	// Helper methods for buffer creation
	void createVertexBuffer( const assets::Primitive &primitive );
	void createIndexBuffer( const assets::Primitive &primitive );

	// Helper method to create upload heap buffers
	Microsoft::WRL::ComPtr<ID3D12Resource> createUploadBuffer( std::size_t bufferSize, const void *data );
};

// Collection of GPU buffers for all primitives in a mesh
export class MeshGPUBuffers
{
public:
	MeshGPUBuffers( dx12::Device &device, const assets::Mesh &mesh );
	~MeshGPUBuffers() = default;

	// No copy/move for now
	MeshGPUBuffers( const MeshGPUBuffers & ) = delete;
	MeshGPUBuffers &operator=( const MeshGPUBuffers & ) = delete;

	// Access individual primitive buffers
	std::uint32_t getPrimitiveCount() const noexcept { return static_cast<std::uint32_t>( m_primitiveBuffers.size() ); }

	const PrimitiveGPUBuffer &getPrimitiveBuffers( std::uint32_t index ) const;
	PrimitiveGPUBuffer &getPrimitiveBuffers( std::uint32_t index );

	// Check if all primitive buffers are valid
	bool isValid() const noexcept;

private:
	std::vector<std::unique_ptr<PrimitiveGPUBuffer>> m_primitiveBuffers;
	dx12::Device &m_device;
};

} // namespace asset_gpu_buffers