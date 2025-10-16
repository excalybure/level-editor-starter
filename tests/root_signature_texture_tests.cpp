// Root signature texture binding tests
// Tests for Task 4.1: Verify root signature includes texture SRVs and samplers

#include <catch2/catch_test_macros.hpp>
#include "graphics/material_system/root_signature_builder.h"
#include "graphics/material_system/root_signature_cache.h"
#include "graphics/material_system/shader_reflection.h"
#include "graphics/material_system/parser.h"
#include "graphics/shader_manager/shader_manager.h"
#include "platform/dx12/dx12_device.h"
#include "core/console.h"
#include "test_dx12_helpers.h"

using namespace graphics::material_system;

TEST_CASE( "Root signature includes SRV descriptor table for textures", "[root-signature][texture][T4.1][unit]" )
{
	// Arrange - Create RootSignatureSpec with texture SRVs
	RootSignatureSpec spec;

	// Add CBVs for frame, object, and material constants
	ResourceBinding frameCBV;
	frameCBV.name = "FrameConstants";
	frameCBV.type = ResourceBindingType::CBV;
	frameCBV.slot = 0;
	spec.cbvRootDescriptors.push_back( frameCBV );

	ResourceBinding objectCBV;
	objectCBV.name = "ObjectConstants";
	objectCBV.type = ResourceBindingType::CBV;
	objectCBV.slot = 1;
	spec.cbvRootDescriptors.push_back( objectCBV );

	ResourceBinding materialCBV;
	materialCBV.name = "MaterialConstants";
	materialCBV.type = ResourceBindingType::CBV;
	materialCBV.slot = 2;
	spec.cbvRootDescriptors.push_back( materialCBV );

	// Add SRVs for textures (t0-t3)
	ResourceBinding baseColorSRV;
	baseColorSRV.name = "baseColorTexture";
	baseColorSRV.type = ResourceBindingType::SRV;
	baseColorSRV.slot = 0;
	spec.descriptorTableResources.push_back( baseColorSRV );

	ResourceBinding normalSRV;
	normalSRV.name = "normalTexture";
	normalSRV.type = ResourceBindingType::SRV;
	normalSRV.slot = 1;
	spec.descriptorTableResources.push_back( normalSRV );

	ResourceBinding metallicRoughnessSRV;
	metallicRoughnessSRV.name = "metallicRoughnessTexture";
	metallicRoughnessSRV.type = ResourceBindingType::SRV;
	metallicRoughnessSRV.slot = 2;
	spec.descriptorTableResources.push_back( metallicRoughnessSRV );

	ResourceBinding emissiveSRV;
	emissiveSRV.name = "emissiveTexture";
	emissiveSRV.type = ResourceBindingType::SRV;
	emissiveSRV.slot = 3;
	spec.descriptorTableResources.push_back( emissiveSRV );

	// Add sampler (s0)
	ResourceBinding linearSampler;
	linearSampler.name = "linearSampler";
	linearSampler.type = ResourceBindingType::Sampler;
	linearSampler.slot = 0;
	spec.descriptorTableResources.push_back( linearSampler );

	// Act - Create root signature from spec
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "Root signature with textures" ) )
		return;

	RootSignatureCache cache;
	const auto rootSignature = cache.getOrCreate( &device, spec );

	// Assert - Root signature created successfully
	REQUIRE( rootSignature != nullptr );
}

TEST_CASE( "Shader reflection detects texture bindings from unlit shader", "[root-signature][texture][T4.1][integration]" )
{
	// Arrange - Register and compile unlit shader with textures
	shader_manager::ShaderManager shaderManager;

	const auto psHandle = shaderManager.registerShader(
		"shaders/unlit.hlsl",
		"PSMain",
		"ps_5_1",
		shader_manager::ShaderType::Pixel );

	REQUIRE( psHandle != shader_manager::INVALID_SHADER_HANDLE );

	// Get compiled shader blob
	auto psBlob = shaderManager.getShaderBlob( psHandle );
	REQUIRE( psBlob != nullptr );

	// Act - Reflect pixel shader to extract resource bindings
	const auto psResult = ShaderReflection::Reflect( psBlob );

	// Assert - Reflection succeeded
	REQUIRE( psResult.success );

	// Assert - Pixel shader should have texture SRVs and sampler
	// Note: Shader reflection only detects resources that are actually USED in the shader
	// Currently unlit.hlsl only samples baseColorTexture and emissiveTexture
	bool foundBaseColor = false;
	bool foundEmissive = false;
	bool foundSampler = false;

	for ( const auto &binding : psResult.bindings )
	{
		if ( binding.name == "baseColorTexture" && binding.type == ResourceBindingType::SRV && binding.slot == 0 )
		{
			foundBaseColor = true;
		}
		else if ( binding.name == "emissiveTexture" && binding.type == ResourceBindingType::SRV && binding.slot == 3 )
		{
			foundEmissive = true;
		}
		else if ( binding.name == "linearSampler" && binding.type == ResourceBindingType::Sampler && binding.slot == 0 )
		{
			foundSampler = true;
		}
	}

	// Check texture bindings that are actually used were found
	REQUIRE( foundBaseColor );
	REQUIRE( foundEmissive );
	REQUIRE( foundSampler );
}
