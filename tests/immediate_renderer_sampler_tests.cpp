#include <catch2/catch_test_macros.hpp>
#include "graphics/renderer/immediate_renderer.h"
#include "graphics/sampler/sampler_manager.h"
#include "graphics/shader_manager/shader_manager.h"
#include "test_dx12_helpers.h"

using namespace graphics;

TEST_CASE( "ImmediateRenderer initializes SamplerManager", "[immediate-renderer][sampler][unit]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "ImmediateRenderer sampler test" ) )
	{
		return;
	}

	shader_manager::ShaderManager shaderManager;
	SamplerManager samplerManager;
	samplerManager.initialize( &device );

	// Act
	ImmediateRenderer renderer( device, shaderManager, samplerManager );

	// Assert - verify sampler manager is initialized and accessible
	const SamplerManager &samplerManager = renderer.getSamplerManager();

	// Verify we can get GPU handles for each sampler type
	const D3D12_GPU_DESCRIPTOR_HANDLE linearWrapHandle = samplerManager.getGpuHandle( SamplerType::LinearWrap );
	const D3D12_GPU_DESCRIPTOR_HANDLE linearClampHandle = samplerManager.getGpuHandle( SamplerType::LinearClamp );
	const D3D12_GPU_DESCRIPTOR_HANDLE pointWrapHandle = samplerManager.getGpuHandle( SamplerType::PointWrap );
	const D3D12_GPU_DESCRIPTOR_HANDLE pointClampHandle = samplerManager.getGpuHandle( SamplerType::PointClamp );
	const D3D12_GPU_DESCRIPTOR_HANDLE anisotropicWrapHandle = samplerManager.getGpuHandle( SamplerType::AnisotropicWrap );
	const D3D12_GPU_DESCRIPTOR_HANDLE anisotropicClampHandle = samplerManager.getGpuHandle( SamplerType::AnisotropicClamp );

	// All handles should be non-null (GPU descriptor handles are initialized if ptr != 0)
	REQUIRE( linearWrapHandle.ptr != 0 );
	REQUIRE( linearClampHandle.ptr != 0 );
	REQUIRE( pointWrapHandle.ptr != 0 );
	REQUIRE( pointClampHandle.ptr != 0 );
	REQUIRE( anisotropicWrapHandle.ptr != 0 );
	REQUIRE( anisotropicClampHandle.ptr != 0 );
}

TEST_CASE( "ImmediateRenderer getSamplerManager returns valid reference", "[immediate-renderer][sampler][unit]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "ImmediateRenderer sampler accessor test" ) )
	{
		return;
	}

	shader_manager::ShaderManager shaderManager;
	SamplerManager samplerManager;
	samplerManager.initialize( &device );
	ImmediateRenderer renderer( device, shaderManager, samplerManager );

	// Act - get both const and non-const references
	SamplerManager &samplerManager = renderer.getSamplerManager();
	const SamplerManager &constSamplerManager = static_cast<const ImmediateRenderer &>( renderer ).getSamplerManager();

	// Assert - both references should provide valid handles
	REQUIRE( samplerManager.getGpuHandle( SamplerType::LinearWrap ).ptr != 0 );
	REQUIRE( constSamplerManager.getGpuHandle( SamplerType::LinearWrap ).ptr != 0 );
}
