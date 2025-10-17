#pragma once

#include <memory>
#include <string>

// Forward declarations
namespace dx12
{
class Device;
}

namespace shader_manager
{
class ShaderManager;
}

namespace graphics
{
class GPUResourceManager;
class ImmediateRenderer;
class SamplerManager;

namespace material_system
{
class MaterialSystem;
}

// Central graphics context that owns and provides access to all core graphics systems.
// This simplifies function signatures by passing a single context instead of multiple system pointers.
class GraphicsContext
{
public:
	// Construct graphics context with a device
	// device: DirectX 12 device (required, cannot be null)
	// materialsPath: Optional path to materials JSON file for MaterialSystem initialization
	explicit GraphicsContext( dx12::Device *device, const std::string &materialsPath = "" );

	// Destructor
	~GraphicsContext();

	// Move semantics
	GraphicsContext( GraphicsContext &&other ) noexcept;
	GraphicsContext &operator=( GraphicsContext &&other ) noexcept;

	// Disable copy (manages unique system instances)
	GraphicsContext( const GraphicsContext & ) = delete;
	GraphicsContext &operator=( const GraphicsContext & ) = delete;

	// Access graphics systems
	dx12::Device *getDevice() const { return m_device; }
	shader_manager::ShaderManager *getShaderManager() const { return m_shaderManager.get(); }
	std::shared_ptr<shader_manager::ShaderManager> getShaderManagerShared() const { return m_shaderManager; }
	material_system::MaterialSystem *getMaterialSystem() const { return m_materialSystem.get(); }
	GPUResourceManager *getGPUResourceManager() const { return m_gpuResourceManager.get(); }
	ImmediateRenderer *getImmediateRenderer() const { return m_immediateRenderer.get(); }
	SamplerManager *getSamplerManager() const { return m_samplerManager.get(); }

private:
	dx12::Device *m_device = nullptr;
	std::shared_ptr<shader_manager::ShaderManager> m_shaderManager;
	std::unique_ptr<material_system::MaterialSystem> m_materialSystem;
	std::unique_ptr<GPUResourceManager> m_gpuResourceManager;
	std::unique_ptr<ImmediateRenderer> m_immediateRenderer;
	std::unique_ptr<SamplerManager> m_samplerManager;
};

} // namespace graphics
