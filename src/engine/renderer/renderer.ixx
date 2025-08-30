// Global module fragment - headers go here
module;

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <windows.h>

export module engine.renderer;

import std;
import platform.dx12;
import engine.vec;
import engine.matrix;
import engine.color;

export namespace renderer
{

// Forward declarations
class Renderer;
class Shader;
class VertexBuffer;
class IndexBuffer;
class RenderState;

// Simple RGBA color struct for rendering
export struct Color
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
export struct Vertex
{
	math::Vec3<> position;
	Color color;

	constexpr Vertex() = default;
	constexpr Vertex( const math::Vec3<> &pos, const Color &col ) noexcept
		: position( pos ), color( col ) {}
};

// Shader compilation result
export struct ShaderBlob
{
	Microsoft::WRL::ComPtr<ID3DBlob> blob;
	std::string entryPoint;
	std::string profile;

	bool isValid() const noexcept { return blob != nullptr; }
};

// Basic shader compiler
export class ShaderCompiler
{
public:
	static ShaderBlob CompileFromSource(
		const std::string &source,
		const std::string &entryPoint,
		const std::string &profile,
		const std::vector<std::string> &defines = {} );

	static ShaderBlob CompileFromFile(
		const std::filesystem::path &filePath,
		const std::string &entryPoint,
		const std::string &profile,
		const std::vector<std::string> &defines = {} );

private:
	static std::string BuildDefineString( const std::vector<std::string> &defines );
};

// Render state configuration
export class RenderState
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
export class VertexBuffer
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
export class IndexBuffer
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
export class Renderer
{
public:
	explicit Renderer( dx12::Device &device );
	~Renderer();

	// No copy/move for now
	Renderer( const Renderer & ) = delete;
	Renderer &operator=( const Renderer & ) = delete;

	// Frame lifecycle
	void beginFrame( dx12::CommandContext &context, dx12::SwapChain &swapChain );
	void endFrame();

	// Clear operations
	void clear( const Color &clearColor = Color{ 0.1f, 0.1f, 0.1f, 1.0f } ) noexcept;
	void clearDepth( float depth = 1.0f ) noexcept;

	// Set render state and shaders
	void setRenderState( const RenderState &state ) noexcept;
	void setViewProjectionMatrix( const math::Mat4<> &viewProj ) noexcept;
	// Accessor (test instrumentation) for verifying setViewProjectionMatrix updates
	const math::Mat4<> &getViewProjectionMatrix() const noexcept { return m_viewProjectionMatrix; }

	// Test instrumentation: expose dynamic buffer capacities
	UINT getDynamicVertexCapacity() const noexcept { return m_dynamicVertexBuffer ? m_dynamicVertexBuffer->getVertexCount() : 0; }
	UINT getDynamicIndexCapacity() const noexcept { return m_dynamicIndexBuffer ? m_dynamicIndexBuffer->getIndexCount() : 0; }
	ID3D12Resource *getDynamicVertexResource() const noexcept { return m_dynamicVertexBuffer ? m_dynamicVertexBuffer->getResource() : nullptr; }
	ID3D12Resource *getDynamicIndexResource() const noexcept { return m_dynamicIndexBuffer ? m_dynamicIndexBuffer->getResource() : nullptr; }

	// Begin a headless recording session for tests (no swap chain / RTV setup)
	void beginHeadlessForTests( dx12::CommandContext &context ) noexcept { m_currentContext = &context; m_currentSwapChain = nullptr; }

	// Immediate drawing commands
	void drawVertices( const std::vector<Vertex> &vertices, D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST ) noexcept;
	void drawIndexed( const std::vector<Vertex> &vertices, const std::vector<uint16_t> &indices, D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST ) noexcept;

	// Convenience drawing methods
	void drawLine( const math::Vec3<> &start, const math::Vec3<> &end, const Color &color = Color::white() ) noexcept;
	void drawWireframeCube( const math::Vec3<> &center, const math::Vec3<> &size, const Color &color = Color::white() ) noexcept;

	// Resource management
	void waitForGPU() noexcept;

private:
	dx12::Device &m_device;
	dx12::CommandContext *m_currentContext = nullptr;
	dx12::SwapChain *m_currentSwapChain = nullptr;

	// Pipeline state
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

	// Render targets
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_depthBuffer;

	// Constant buffer for view-projection matrix
	Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
	void *m_constantBufferData = nullptr;

	// Current render state
	RenderState m_currentRenderState;
	math::Mat4<> m_viewProjectionMatrix = math::Mat4<>::identity();

	// Dynamic vertex/index buffers for immediate mode
	std::unique_ptr<VertexBuffer> m_dynamicVertexBuffer;
	std::unique_ptr<IndexBuffer> m_dynamicIndexBuffer;

	// Initialization
	void createRootSignature();
	void createPipelineState();
	void createRenderTargets( UINT width, UINT height );
	void createConstantBuffer();

	// Helper methods
	void updateConstantBuffer();
	D3D12_CPU_DESCRIPTOR_HANDLE getCurrentRTV() const;
	D3D12_CPU_DESCRIPTOR_HANDLE getDSV() const;
};

// Default shaders for basic rendering
export namespace DefaultShaders
{
extern const char *kVertexShader;
extern const char *kPixelShader;
} // namespace DefaultShaders

} // namespace renderer
