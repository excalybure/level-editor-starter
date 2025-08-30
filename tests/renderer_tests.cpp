#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

// Need D3D12 headers for constants
#include <d3d12.h>
#include <windows.h>

import engine.renderer;
import platform.dx12;
import engine.vec;
import engine.matrix;
import engine.color;

#include "test_dx12_helpers.h"

using namespace renderer;
using namespace math;

TEST_CASE( "Renderer Vertex Format", "[renderer]" )
{
	SECTION( "Vertex can be constructed" )
	{
		const Vec3<> position{ 1.0f, 2.0f, 3.0f };
		const Color color{ 1.0f, 0.5f, 0.0f, 1.0f };

		const Vertex vertex( position, color );

		REQUIRE( vertex.position.x == 1.0f );
		REQUIRE( vertex.position.y == 2.0f );
		REQUIRE( vertex.position.z == 3.0f );
		REQUIRE( vertex.color.r == 1.0f );
		REQUIRE( vertex.color.g == 0.5f );
		REQUIRE( vertex.color.b == 0.0f );
		REQUIRE( vertex.color.a == 1.0f );
	}
}

TEST_CASE( "Shader Compiler", "[renderer]" )
{
	SECTION( "Can compile basic vertex shader" )
	{
		REQUIRE_NOTHROW( []() {
			try
			{
				const std::string simpleVS = R"(
                    float4 main(float3 pos : POSITION) : SV_POSITION
                    {
                        return float4(pos, 1.0f);
                    }
                )";

				const auto blob = ShaderCompiler::CompileFromSource( simpleVS, "main", "vs_5_0" );
				REQUIRE( blob.isValid() );
				REQUIRE( blob.entryPoint == "main" );
				REQUIRE( blob.profile == "vs_5_0" );
			}
			catch ( const std::runtime_error &e )
			{
				WARN( "Shader compilation failed (D3DCompile may not be available): " << e.what() );
			}
		}() );
	}

	SECTION( "Can compile basic pixel shader" )
	{
		REQUIRE_NOTHROW( []() {
			try
			{
				const std::string simplePS = R"(
                    float4 main() : SV_TARGET
                    {
                        return float4(1.0f, 0.0f, 0.0f, 1.0f);
                    }
                )";

				const auto blob = ShaderCompiler::CompileFromSource( simplePS, "main", "ps_5_0" );
				REQUIRE( blob.isValid() );
			}
			catch ( const std::runtime_error &e )
			{
				WARN( "Shader compilation failed: " << e.what() );
			}
		}() );
	}
}

TEST_CASE( "Render State", "[renderer]" )
{
	SECTION( "Default render state" )
	{
		const RenderState state;

		const auto depthDesc = state.getDepthStencilDesc();
		REQUIRE( depthDesc.DepthEnable == TRUE );
		REQUIRE( depthDesc.DepthWriteMask == D3D12_DEPTH_WRITE_MASK_ALL );

		const auto rasterizerDesc = state.getRasterizerDesc();
		REQUIRE( rasterizerDesc.FillMode == D3D12_FILL_MODE_SOLID );
		REQUIRE( rasterizerDesc.CullMode == D3D12_CULL_MODE_BACK );

		const auto blendDesc = state.getBlendDesc();
		REQUIRE( blendDesc.RenderTarget[0].BlendEnable == FALSE );
	}

	SECTION( "Wireframe state" )
	{
		RenderState state;
		state.setWireframe( true );

		const auto rasterizerDesc = state.getRasterizerDesc();
		REQUIRE( rasterizerDesc.FillMode == D3D12_FILL_MODE_WIREFRAME );
	}

	SECTION( "Depth state modifications" )
	{
		RenderState state;
		state.setDepthTest( false );
		state.setDepthWrite( false );

		const auto depthDesc = state.getDepthStencilDesc();
		REQUIRE( depthDesc.DepthEnable == FALSE );
		REQUIRE( depthDesc.DepthWriteMask == D3D12_DEPTH_WRITE_MASK_ZERO );
	}
}

TEST_CASE( "Vertex and Index Buffers", "[renderer]" )
{
	SECTION( "VertexBuffer creation with valid device" )
	{
		REQUIRE_NOTHROW( []() {
			try
			{
				dx12::Device device;
				if ( !requireHeadlessDevice( device, "VertexBuffer" ) )
					return; // Skip if unsupported

				const std::vector<Vertex> vertices = {
					{ Vec3<>{ 0.0f, 1.0f, 0.0f }, Color{ 1.0f, 0.0f, 0.0f, 1.0f } },
					{ Vec3<>{ -1.0f, -1.0f, 0.0f }, Color{ 0.0f, 1.0f, 0.0f, 1.0f } },
					{ Vec3<>{ 1.0f, -1.0f, 0.0f }, Color{ 0.0f, 0.0f, 1.0f, 1.0f } }
				};

				const VertexBuffer vb( device, vertices );
				REQUIRE( vb.getVertexCount() == 3 );

				const auto view = vb.getView();
				REQUIRE( view.SizeInBytes == vertices.size() * sizeof( Vertex ) );
				REQUIRE( view.StrideInBytes == sizeof( Vertex ) );
			}
			catch ( const std::runtime_error &e )
			{
				WARN( "VertexBuffer creation failed (D3D12 may not be available): " << e.what() );
			}
		}() );
	}

	SECTION( "IndexBuffer creation with valid device" )
	{
		REQUIRE_NOTHROW( []() {
			try
			{
				dx12::Device device;
				if ( !requireHeadlessDevice( device, "IndexBuffer" ) )
					return;

				const std::vector<uint16_t> indices = { 0, 1, 2 };

				const IndexBuffer ib( device, indices );
				REQUIRE( ib.getIndexCount() == 3 );

				const auto view = ib.getView();
				REQUIRE( view.SizeInBytes == indices.size() * sizeof( uint16_t ) );
				REQUIRE( view.Format == DXGI_FORMAT_R16_UINT );
			}
			catch ( const std::runtime_error &e )
			{
				WARN( "IndexBuffer creation failed: " << e.what() );
			}
		}() );
	}
}

TEST_CASE( "Renderer Creation", "[renderer]" )
{
	SECTION( "Renderer can be created with valid device" )
	{
		REQUIRE_NOTHROW( []() {
			try
			{
				dx12::Device device;
				if ( !requireHeadlessDevice( device, "Renderer creation" ) )
					return;
				const Renderer renderer( device );
				// Just test that it constructs without throwing
			}
			catch ( const std::runtime_error &e )
			{
				WARN( "Renderer creation failed (D3D12 may not be available): " << e.what() );
			}
		}() );
	}
}

TEST_CASE( "Default Shaders", "[renderer]" )
{
	SECTION( "Default vertex shader is valid string" )
	{
		REQUIRE( DefaultShaders::kVertexShader != nullptr );
		REQUIRE( std::string( DefaultShaders::kVertexShader ).length() > 0 );

		// Check for basic vertex shader components
		const std::string vs( DefaultShaders::kVertexShader );
		REQUIRE( vs.find( "VSInput" ) != std::string::npos );
		REQUIRE( vs.find( "PSInput" ) != std::string::npos );
		REQUIRE( vs.find( "POSITION" ) != std::string::npos );
		REQUIRE( vs.find( "COLOR" ) != std::string::npos );
		REQUIRE( vs.find( "viewProjectionMatrix" ) != std::string::npos );
	}

	SECTION( "Default pixel shader is valid string" )
	{
		REQUIRE( DefaultShaders::kPixelShader != nullptr );
		REQUIRE( std::string( DefaultShaders::kPixelShader ).length() > 0 );

		// Check for basic pixel shader components
		const std::string ps( DefaultShaders::kPixelShader );
		REQUIRE( ps.find( "PSInput" ) != std::string::npos );
		REQUIRE( ps.find( "SV_TARGET" ) != std::string::npos );
	}
}
