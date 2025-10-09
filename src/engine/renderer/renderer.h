#pragma once

#include <d3d12.h>
#include <functional>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl.h>
#include "math/vec.h"
#include "math/matrix.h"
#include "math/color.h"
#include "platform/dx12/dx12_device.h"

namespace shader_manager
{
class ShaderManager;
}

namespace renderer
{

// Simple RGBA color struct for rendering
// TODO: most likely want to move that to color.h
struct Color
{
	float r, g, b, a;

	constexpr Color() : r( 0 ), g( 0 ), b( 0 ), a( 1 ) {}
	constexpr Color( float r_, float g_, float b_, float a_ = 1.0f ) : r( r_ ), g( g_ ), b( b_ ), a( a_ ) {}

	static constexpr Color white() noexcept { return Color{ 1, 1, 1, 1 }; }
	static constexpr Color black() noexcept { return Color{ 0, 0, 0, 1 }; }
	static constexpr Color red() noexcept { return Color{ 1, 0, 0, 1 }; }
	static constexpr Color green() noexcept { return Color{ 0, 1, 0, 1 }; }
	static constexpr Color blue() noexcept { return Color{ 0, 0, 1, 1 }; }
	static constexpr Color transparent() noexcept { return Color{ 0, 0, 0, 0 }; }
};

// Simple vertex format for basic rendering
struct Vertex
{
	math::Vec3<> position;
	Color color;

	constexpr Vertex() = default;
	constexpr Vertex( const math::Vec3<> &pos, const Color &col ) noexcept
		: position( pos ), color( col ) {}
};

// Shader compilation result
struct ShaderBlob
{
	Microsoft::WRL::ComPtr<ID3DBlob> blob;
	std::string entryPoint;
	std::string profile;
	std::vector<std::filesystem::path> includedFiles; // Track all included files for dependency checking

	bool isValid() const noexcept { return blob != nullptr; }
};

// Basic shader compiler
class ShaderCompiler
{
public:
	static ShaderBlob CompileFromSource(
		const std::string &source,
		const std::string &entryPoint,
		const std::string &profile,
		const std::vector<std::string> &defines = {},
		const std::filesystem::path *shaderDirectory = nullptr );

	static ShaderBlob CompileFromFile(
		const std::filesystem::path &filePath,
		const std::string &entryPoint,
		const std::string &profile,
		const std::vector<std::string> &defines = {} );

private:
	static std::string BuildDefineString( const std::vector<std::string> &defines );
};

// Render state configuration
class RenderState
{
public:
	RenderState();
	~RenderState() = default;

	// Depth testing
	void setDepthTest( bool enabled ) noexcept { m_depthTestEnabled = enabled; }
	void setDepthWrite( bool enabled ) noexcept { m_depthWriteEnabled = enabled; }

	// Rasterizer state
	void setWireframe( bool enabled ) noexcept { m_wireframeEnabled = enabled; }
	void setCullMode( D3D12_CULL_MODE mode ) noexcept { m_cullMode = mode; }

	// Blend state
	void setBlendEnabled( bool enabled ) noexcept { m_blendEnabled = enabled; }

	// Getters
	bool isDepthTestEnabled() const noexcept { return m_depthTestEnabled; }
	bool isDepthWriteEnabled() const noexcept { return m_depthWriteEnabled; }
	bool isWireframeEnabled() const noexcept { return m_wireframeEnabled; }
	bool isBlendEnabled() const noexcept { return m_blendEnabled; }
	D3D12_CULL_MODE getCullMode() const noexcept { return m_cullMode; }

	// Apply state to command list
	void apply( ID3D12GraphicsCommandList *cmdList ) const;

	// Create D3D12 pipeline state objects
	D3D12_DEPTH_STENCIL_DESC getDepthStencilDesc() const;
	D3D12_RASTERIZER_DESC getRasterizerDesc() const;
	D3D12_BLEND_DESC getBlendDesc() const;

private:
	bool m_depthTestEnabled = true;
	bool m_depthWriteEnabled = true;
	bool m_wireframeEnabled = false;
	bool m_blendEnabled = false;
	D3D12_CULL_MODE m_cullMode = D3D12_CULL_MODE_BACK;
};

// Vertex buffer wrapper
class VertexBuffer
{
public:
	VertexBuffer( dx12::Device &device, const std::vector<Vertex> &vertices );
	~VertexBuffer() = default;

	// No copy/move for now
	VertexBuffer( const VertexBuffer & ) = delete;
	VertexBuffer &operator=( const VertexBuffer & ) = delete;

	D3D12_VERTEX_BUFFER_VIEW getView() const noexcept { return m_vertexBufferView; }
	UINT getVertexCount() const noexcept { return m_vertexCount; }
	// Test instrumentation: expose underlying resource pointer
	ID3D12Resource *getResource() const noexcept { return m_vertexBuffer.Get(); }

	// Update buffer data (for dynamic buffers)
	void update( const std::vector<Vertex> &vertices );

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	UINT m_vertexCount;
	dx12::Device &m_device;

	void createBuffer( const std::vector<Vertex> &vertices );
};

// Index buffer wrapper
class IndexBuffer
{
public:
	IndexBuffer( dx12::Device &device, const std::vector<uint16_t> &indices );
	~IndexBuffer() = default;

	// No copy/move for now
	IndexBuffer( const IndexBuffer & ) = delete;
	IndexBuffer &operator=( const IndexBuffer & ) = delete;

	D3D12_INDEX_BUFFER_VIEW getView() const noexcept { return m_indexBufferView; }
	UINT getIndexCount() const noexcept { return m_indexCount; }
	ID3D12Resource *getResource() const noexcept { return m_indexBuffer.Get(); }

	// Update buffer data
	void update( const std::vector<uint16_t> &indices );

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	UINT m_indexCount;
	dx12::Device &m_device;

	void createBuffer( const std::vector<uint16_t> &indices );
};

// Simple immediate-mode renderer
class Renderer
{
public:
	explicit Renderer( dx12::Device &device, shader_manager::ShaderManager &shaderManager );
	~Renderer();

	// No copy/move for now
	Renderer( const Renderer & ) = delete;
	Renderer &operator=( const Renderer & ) = delete;

	// Frame lifecycle
	void beginFrame();
	void endFrame();

	// Clear operations
	void clear( const Color &clearColor = Color{ 0.1f, 0.1f, 0.1f, 1.0f } ) noexcept;
	void clearDepth( float depth = 1.0f ) noexcept;

	// Set render state and shaders
	void setRenderState( const RenderState &state ) noexcept;
	void setViewProjectionMatrix( const math::Mat4<> &viewProj ) noexcept;
	// Accessor (test instrumentation) for verifying setViewProjectionMatrix updates
	const math::Mat4f &getViewProjectionMatrix() const noexcept { return m_viewProjectionMatrix; }

	// Test instrumentation: expose dynamic buffer capacities
	UINT getDynamicVertexCapacity() const noexcept { return m_dynamicVertexBuffer ? m_dynamicVertexBuffer->getVertexCount() : 0; }
	UINT getDynamicIndexCapacity() const noexcept { return m_dynamicIndexBuffer ? m_dynamicIndexBuffer->getIndexCount() : 0; }
	ID3D12Resource *getDynamicVertexResource() const noexcept { return m_dynamicVertexBuffer ? m_dynamicVertexBuffer->getResource() : nullptr; }
	ID3D12Resource *getDynamicIndexResource() const noexcept { return m_dynamicIndexBuffer ? m_dynamicIndexBuffer->getResource() : nullptr; }

	// Access to command context for external rendering systems
	dx12::CommandContext *getCommandContext() const noexcept { return m_currentContext; }

	// Access to device for external systems that need to create resources
	dx12::Device &getDevice() noexcept { return m_device; }
	const dx12::Device &getDevice() const noexcept { return m_device; }

	// Begin a headless recording session for tests (no swap chain / RTV setup)
	void beginHeadlessForTests() noexcept
	{
		// For tests, reset frame state
		m_inFrame = false;
		m_currentContext = m_device.getCommandContext();
		m_currentSwapChain = nullptr;
		m_inFrame = true;
	}

	// Immediate drawing commands
	void drawVertices( const std::vector<Vertex> &vertices, D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST ) noexcept;
	void drawIndexed( const std::vector<Vertex> &vertices, const std::vector<uint16_t> &indices, D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST ) noexcept;

	// Convenience drawing methods
	void drawLine( const math::Vec3<> &start, const math::Vec3<> &end, const Color &color = Color::white() ) noexcept;
	void drawWireframeCube( const math::Vec3<> &center, const math::Vec3<> &size, const Color &color = Color::white() ) noexcept;

	// Resource management
	void waitForGPU() noexcept;

private:
	// Key for pipeline state cache
	struct PipelineStateKey
	{
		bool depthTest;
		bool depthWrite;
		bool wireframe;
		bool blend;
		D3D12_CULL_MODE cullMode;
		D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType;
		bool operator==( const PipelineStateKey &o ) const noexcept
		{
			return depthTest == o.depthTest && depthWrite == o.depthWrite && wireframe == o.wireframe && blend == o.blend && cullMode == o.cullMode && topologyType == o.topologyType;
		}
	};
	struct PipelineStateKeyHash
	{
		std::size_t operator()( const PipelineStateKey &k ) const noexcept
		{
			// Pack bits into a small integer for hashing
			uint32_t bits = ( k.depthTest ? 1u : 0u ) |
				( ( k.depthWrite ? 1u : 0u ) << 1 ) |
				( ( k.wireframe ? 1u : 0u ) << 2 ) |
				( ( k.blend ? 1u : 0u ) << 3 ) |
				( ( static_cast<uint32_t>( k.cullMode ) & 0x3u ) << 4 ) |
				( ( static_cast<uint32_t>( k.topologyType ) & 0x7u ) << 6 );
			return std::hash<uint32_t>{}( bits );
		}
	};

	dx12::Device &m_device;
	shader_manager::ShaderManager &m_shaderManager;
	dx12::CommandContext *m_currentContext = nullptr;
	dx12::SwapChain *m_currentSwapChain = nullptr;

	// Default shader handles
	size_t m_vertexShaderHandle = 0;
	size_t m_pixelShaderHandle = 0;

	// Pipeline state
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	// Cached compiled default shaders
	Microsoft::WRL::ComPtr<ID3DBlob> m_vsBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> m_psBlob;
	// Active PSO pointer (cached lookup)
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_activePipelineState;
	std::unordered_map<PipelineStateKey, Microsoft::WRL::ComPtr<ID3D12PipelineState>, PipelineStateKeyHash> m_psoCache;

	// NOTE: Render targets are managed by Device, no duplication needed

	// Constant buffer for view-projection matrix
	Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
	void *m_constantBufferData = nullptr;

	// Current render state
	RenderState m_currentRenderState;
	math::Mat4<> m_viewProjectionMatrix = math::Mat4<>::identity();

	// Dynamic vertex/index buffers for immediate mode
	std::unique_ptr<VertexBuffer> m_dynamicVertexBuffer;
	std::unique_ptr<IndexBuffer> m_dynamicIndexBuffer;

	// Pending deletion queue to defer destruction until frame end
	std::vector<std::unique_ptr<VertexBuffer>> m_pendingVertexBufferDeletions;
	std::vector<std::unique_ptr<IndexBuffer>> m_pendingIndexBufferDeletions;

	// Frame state tracking
	bool m_inFrame = false;

	// Initialization
	void createRootSignature();
	void compileDefaultShaders();
	void createPipelineStateForKey( const PipelineStateKey &key );

	void createConstantBuffer();

	// Helper methods
	void updateConstantBuffer();

	PipelineStateKey makeKeyFromState( const RenderState &state, D3D12_PRIMITIVE_TOPOLOGY_TYPE topology ) const noexcept;
	void ensurePipelineForCurrentState( D3D12_PRIMITIVE_TOPOLOGY_TYPE topology );

	// Helper to convert D3D_PRIMITIVE_TOPOLOGY to D3D12_PRIMITIVE_TOPOLOGY_TYPE
	static D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyToTopologyType( D3D_PRIMITIVE_TOPOLOGY topology ) noexcept;

public:
	// Test instrumentation: get current PSO cache size
	size_t getPipelineStateCacheSize() const noexcept { return m_psoCache.size(); }
};

} // namespace renderer
