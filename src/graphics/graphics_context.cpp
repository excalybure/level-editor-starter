#include "graphics/graphics_context.h"
#include "graphics/gpu/gpu_resource_manager.h"
#include "graphics/immediate_renderer/immediate_renderer.h"
#include "graphics/material_system/material_system.h"
#include "graphics/shader_manager/shader_manager.h"
#include "graphics/sampler/sampler_manager.h"
#include "platform/dx12/dx12_device.h"
#include "core/console.h"

#include <stdexcept>

namespace graphics
{

GraphicsContext::GraphicsContext( dx12::Device *device, const std::string &materialsPath )
	: m_device( device )
{
	if ( !m_device )
	{
		throw std::invalid_argument( "GraphicsContext: device cannot be null" );
	}

	// Create shader manager for automatic shader reloading
	m_shaderManager = std::make_shared<shader_manager::ShaderManager>();

	// Initialize material system for data-driven materials
	m_materialSystem = std::make_unique<material_system::MaterialSystem>();
	if ( !materialsPath.empty() )
	{
		if ( !m_materialSystem->initialize( materialsPath, m_shaderManager.get() ) )
		{
			console::error( "GraphicsContext: Failed to initialize material system from {}", materialsPath );
			console::info( "GraphicsContext: Material system will continue with default initialization" );
		}
	}
	else
	{
		// Initialize with empty path to setup shader manager connection
		m_materialSystem->initialize( "", m_shaderManager.get() );
	}

	// Create GPU resource manager for GPU resource creation and management
	m_gpuResourceManager = std::make_unique<GPUResourceManager>( *m_device );

	// Create sampler manager for texture sampling
	m_samplerManager = std::make_unique<SamplerManager>();
	if ( !m_samplerManager->initialize( m_device ) )
	{
		console::error( "GraphicsContext: Failed to initialize sampler manager" );
		// Note: We don't throw here as the application can continue without samplers
	}

	// Create immediate renderer for debug shapes and UI overlays
	m_immediateRenderer = std::make_unique<ImmediateRenderer>( *m_device, *m_shaderManager );
}

GraphicsContext::~GraphicsContext() = default;

GraphicsContext::GraphicsContext( GraphicsContext &&other ) noexcept
	: m_device( other.m_device ), m_shaderManager( std::move( other.m_shaderManager ) ), m_materialSystem( std::move( other.m_materialSystem ) ), m_gpuResourceManager( std::move( other.m_gpuResourceManager ) ), m_immediateRenderer( std::move( other.m_immediateRenderer ) ), m_samplerManager( std::move( other.m_samplerManager ) )
{
	other.m_device = nullptr;
}

GraphicsContext &GraphicsContext::operator=( GraphicsContext &&other ) noexcept
{
	if ( this != &other )
	{
		// CRITICAL: Destroy systems that hold references to ShaderManager BEFORE moving ShaderManager
		// Otherwise MaterialSystem destructor will try to unregister callback from destroyed ShaderManager
		m_materialSystem.reset();
		m_immediateRenderer.reset();
		m_gpuResourceManager.reset();
		m_samplerManager.reset();

		// Now safe to move the new state
		m_device = other.m_device;
		m_shaderManager = std::move( other.m_shaderManager );
		m_materialSystem = std::move( other.m_materialSystem );
		m_gpuResourceManager = std::move( other.m_gpuResourceManager );
		m_immediateRenderer = std::move( other.m_immediateRenderer );
		m_samplerManager = std::move( other.m_samplerManager );

		other.m_device = nullptr;
	}
	return *this;
}

} // namespace graphics
