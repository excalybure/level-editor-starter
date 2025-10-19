// Sampler name parsing tests
// Tests for parsing addressMode and MaxAnisotropy from sampler names

#include <catch2/catch_test_macros.hpp>
#include "graphics/material_system/root_signature_builder.h"
#include <d3d12.h>

using namespace graphics::material_system;

TEST_CASE( "GetAddressModeForSamplerName parses wrap/clamp", "[sampler-parsing][address-mode][unit]" )
{
	SECTION( "linearWrapSampler returns WRAP" )
	{
		const auto mode = RootSignatureBuilder::GetAddressModeForSamplerName( "linearWrapSampler" );
		REQUIRE( mode == D3D12_TEXTURE_ADDRESS_MODE_WRAP );
	}

	SECTION( "linearClampSampler returns CLAMP" )
	{
		const auto mode = RootSignatureBuilder::GetAddressModeForSamplerName( "linearClampSampler" );
		REQUIRE( mode == D3D12_TEXTURE_ADDRESS_MODE_CLAMP );
	}

	SECTION( "pointWrapSampler returns WRAP" )
	{
		const auto mode = RootSignatureBuilder::GetAddressModeForSamplerName( "pointWrapSampler" );
		REQUIRE( mode == D3D12_TEXTURE_ADDRESS_MODE_WRAP );
	}

	SECTION( "pointClampSampler returns CLAMP" )
	{
		const auto mode = RootSignatureBuilder::GetAddressModeForSamplerName( "pointClampSampler" );
		REQUIRE( mode == D3D12_TEXTURE_ADDRESS_MODE_CLAMP );
	}

	SECTION( "anisoWrap4XSampler returns WRAP" )
	{
		const auto mode = RootSignatureBuilder::GetAddressModeForSamplerName( "anisoWrap4XSampler" );
		REQUIRE( mode == D3D12_TEXTURE_ADDRESS_MODE_WRAP );
	}

	SECTION( "anisoClamp16XSampler returns CLAMP" )
	{
		const auto mode = RootSignatureBuilder::GetAddressModeForSamplerName( "anisoClamp16XSampler" );
		REQUIRE( mode == D3D12_TEXTURE_ADDRESS_MODE_CLAMP );
	}

	SECTION( "Unknown sampler defaults to WRAP" )
	{
		const auto mode = RootSignatureBuilder::GetAddressModeForSamplerName( "unknownSampler" );
		REQUIRE( mode == D3D12_TEXTURE_ADDRESS_MODE_WRAP );
	}
}

TEST_CASE( "GetMaxAnisotropyForSamplerName parses anisotropy level", "[sampler-parsing][anisotropy][unit]" )
{
	SECTION( "linearWrapSampler returns 1 (not anisotropic)" )
	{
		const auto aniso = RootSignatureBuilder::GetMaxAnisotropyForSamplerName( "linearWrapSampler" );
		REQUIRE( aniso == 1 );
	}

	SECTION( "pointClampSampler returns 1 (not anisotropic)" )
	{
		const auto aniso = RootSignatureBuilder::GetMaxAnisotropyForSamplerName( "pointClampSampler" );
		REQUIRE( aniso == 1 );
	}

	SECTION( "anisoWrap4XSampler returns 4" )
	{
		const auto aniso = RootSignatureBuilder::GetMaxAnisotropyForSamplerName( "anisoWrap4XSampler" );
		REQUIRE( aniso == 4 );
	}

	SECTION( "anisoClamp16XSampler returns 16" )
	{
		const auto aniso = RootSignatureBuilder::GetMaxAnisotropyForSamplerName( "anisoClamp16XSampler" );
		REQUIRE( aniso == 16 );
	}

	SECTION( "anisoWrap8XSampler returns 8" )
	{
		const auto aniso = RootSignatureBuilder::GetMaxAnisotropyForSamplerName( "anisoWrap8XSampler" );
		REQUIRE( aniso == 8 );
	}

	SECTION( "anisoClamp2XSampler returns 2" )
	{
		const auto aniso = RootSignatureBuilder::GetMaxAnisotropyForSamplerName( "anisoClamp2XSampler" );
		REQUIRE( aniso == 2 );
	}

	SECTION( "Unknown sampler defaults to 1" )
	{
		const auto aniso = RootSignatureBuilder::GetMaxAnisotropyForSamplerName( "unknownSampler" );
		REQUIRE( aniso == 1 );
	}
}

TEST_CASE( "GetFilterForSamplerName derives filter type from sampler name", "[sampler-parsing][filter-type][unit]" )
{
	SECTION( "linearWrapSampler creates LINEAR filter" )
	{
		const auto filter = RootSignatureBuilder::GetFilterForSamplerName( "linearWrapSampler" );
		REQUIRE( filter == D3D12_FILTER_MIN_MAG_MIP_LINEAR );
	}

	SECTION( "linearClampSampler creates LINEAR filter" )
	{
		const auto filter = RootSignatureBuilder::GetFilterForSamplerName( "linearClampSampler" );
		REQUIRE( filter == D3D12_FILTER_MIN_MAG_MIP_LINEAR );
	}

	SECTION( "pointWrapSampler creates POINT filter" )
	{
		const auto filter = RootSignatureBuilder::GetFilterForSamplerName( "pointWrapSampler" );
		REQUIRE( filter == D3D12_FILTER_MIN_MAG_MIP_POINT );
	}

	SECTION( "pointClampSampler creates POINT filter" )
	{
		const auto filter = RootSignatureBuilder::GetFilterForSamplerName( "pointClampSampler" );
		REQUIRE( filter == D3D12_FILTER_MIN_MAG_MIP_POINT );
	}

	SECTION( "anisoWrap4XSampler creates ANISOTROPIC filter" )
	{
		const auto filter = RootSignatureBuilder::GetFilterForSamplerName( "anisoWrap4XSampler" );
		REQUIRE( filter == D3D12_FILTER_ANISOTROPIC );
	}

	SECTION( "anisoClamp16XSampler creates ANISOTROPIC filter" )
	{
		const auto filter = RootSignatureBuilder::GetFilterForSamplerName( "anisoClamp16XSampler" );
		REQUIRE( filter == D3D12_FILTER_ANISOTROPIC );
	}

	SECTION( "Old linearSampler creates LINEAR filter" )
	{
		const auto filter = RootSignatureBuilder::GetFilterForSamplerName( "linearSampler" );
		REQUIRE( filter == D3D12_FILTER_MIN_MAG_MIP_LINEAR );
	}

	SECTION( "Old pointSampler creates POINT filter" )
	{
		const auto filter = RootSignatureBuilder::GetFilterForSamplerName( "pointSampler" );
		REQUIRE( filter == D3D12_FILTER_MIN_MAG_MIP_POINT );
	}

	SECTION( "Old anisotropicSampler creates ANISOTROPIC filter" )
	{
		const auto filter = RootSignatureBuilder::GetFilterForSamplerName( "anisotropicSampler" );
		REQUIRE( filter == D3D12_FILTER_ANISOTROPIC );
	}

	SECTION( "Old comparisonSampler creates comparison filter" )
	{
		const auto filter = RootSignatureBuilder::GetFilterForSamplerName( "comparisonSampler" );
		REQUIRE( filter == D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT );
	}

	SECTION( "Unknown sampler defaults to LINEAR filter" )
	{
		const auto filter = RootSignatureBuilder::GetFilterForSamplerName( "unknownSampler" );
		REQUIRE( filter == D3D12_FILTER_MIN_MAG_MIP_LINEAR );
	}
}

TEST_CASE( "IsStaticSamplerName recognizes sampler naming patterns", "[sampler-parsing][static-sampler][unit]" )
{
	SECTION( "linearWrapSampler is recognized" )
	{
		REQUIRE( RootSignatureBuilder::IsStaticSamplerName( "linearWrapSampler" ) );
	}

	SECTION( "linearClampSampler is recognized" )
	{
		REQUIRE( RootSignatureBuilder::IsStaticSamplerName( "linearClampSampler" ) );
	}

	SECTION( "pointWrapSampler is recognized" )
	{
		REQUIRE( RootSignatureBuilder::IsStaticSamplerName( "pointWrapSampler" ) );
	}

	SECTION( "pointClampSampler is recognized" )
	{
		REQUIRE( RootSignatureBuilder::IsStaticSamplerName( "pointClampSampler" ) );
	}

	SECTION( "anisoWrap4XSampler is recognized" )
	{
		REQUIRE( RootSignatureBuilder::IsStaticSamplerName( "anisoWrap4XSampler" ) );
	}

	SECTION( "anisoClamp16XSampler is recognized" )
	{
		REQUIRE( RootSignatureBuilder::IsStaticSamplerName( "anisoClamp16XSampler" ) );
	}

	SECTION( "Old linearSampler is still recognized for backward compatibility" )
	{
		REQUIRE( RootSignatureBuilder::IsStaticSamplerName( "linearSampler" ) );
	}

	SECTION( "Old pointSampler is still recognized for backward compatibility" )
	{
		REQUIRE( RootSignatureBuilder::IsStaticSamplerName( "pointSampler" ) );
	}

	SECTION( "Old anisotropicSampler is still recognized for backward compatibility" )
	{
		REQUIRE( RootSignatureBuilder::IsStaticSamplerName( "anisotropicSampler" ) );
	}

	SECTION( "Old comparisonSampler is still recognized for backward compatibility" )
	{
		REQUIRE( RootSignatureBuilder::IsStaticSamplerName( "comparisonSampler" ) );
	}

	SECTION( "randomTexture is not recognized as sampler" )
	{
		REQUIRE( !RootSignatureBuilder::IsStaticSamplerName( "randomTexture" ) );
	}

	SECTION( "diffuseSRV is not recognized as sampler" )
	{
		REQUIRE( !RootSignatureBuilder::IsStaticSamplerName( "diffuseSRV" ) );
	}
}
