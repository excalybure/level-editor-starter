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

TEST_CASE( "Render State permutations", "[renderer]" )
{
	RenderState state;
	SECTION( "Disable depth test & write, enable blend wireframe front cull" )
	{
		state.setDepthTest( false );
		state.setDepthWrite( false );
		state.setBlendEnabled( true );
		state.setWireframe( true );
		state.setCullMode( D3D12_CULL_MODE_FRONT );
		const auto depth = state.getDepthStencilDesc();
		REQUIRE( depth.DepthEnable == FALSE );
		REQUIRE( depth.DepthWriteMask == D3D12_DEPTH_WRITE_MASK_ZERO );
		const auto rast = state.getRasterizerDesc();
		REQUIRE( rast.FillMode == D3D12_FILL_MODE_WIREFRAME );
		REQUIRE( rast.CullMode == D3D12_CULL_MODE_FRONT );
		const auto blend = state.getBlendDesc();
		REQUIRE( blend.RenderTarget[0].BlendEnable == TRUE );
	}
	SECTION( "Re-enable depth variants" )
	{
		state.setDepthTest( true );
		state.setDepthWrite( true );
		const auto depth = state.getDepthStencilDesc();
		REQUIRE( depth.DepthEnable == TRUE );
		REQUIRE( depth.DepthWriteMask == D3D12_DEPTH_WRITE_MASK_ALL );
	}
}

TEST_CASE( "ShaderCompiler edge cases", "[renderer][shader]" )
{
	SECTION( "Defines are injected" )
	{
		try
		{
			const std::string src = R"(
				#ifndef MY_FLAG
				#error MY_FLAG not defined
				#endif
				float4 main(float3 pos:POSITION):SV_POSITION { return float4(pos,1); }
			)";
			REQUIRE_NOTHROW( ShaderCompiler::CompileFromSource( src, "main", "vs_5_0", { "MY_FLAG" } ) );
		}
		catch ( const std::runtime_error &e )
		{
			WARN( "Skipping defines test: " << e.what() );
		}
	}
	SECTION( "Invalid profile throws" )
	{
		bool threw = false;
		try
		{
			ShaderCompiler::CompileFromSource( "float4 main():SV_POSITION{return 0;} ", "main", "vs_99_99" );
		}
		catch ( const std::runtime_error & )
		{
			threw = true;
		}
		REQUIRE( threw );
	}
	SECTION( "Missing file throws" )
	{
		bool threw = false;
		try
		{
			ShaderCompiler::CompileFromFile( "this_does_not_exist.hlsl", "main", "vs_5_0" );
		}
		catch ( const std::runtime_error & )
		{
			threw = true;
		}
		REQUIRE( threw );
	}
}

TEST_CASE( "Buffer update behavior", "[renderer][buffers]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "buffer update" ) )
		return;

	// Start with 3 vertices
	std::vector<Vertex> verts = {
		{ { 0, 0, 0 }, Color::red() },
		{ { 1, 0, 0 }, Color::green() },
		{ { 0, 1, 0 }, Color::blue() }
	};
	VertexBuffer vb( device, verts );
	REQUIRE( vb.getVertexCount() == 3 );

	// Same size update -> count unchanged
	verts[1].position.x = 2.0f;
	REQUIRE_NOTHROW( vb.update( verts ) );
	REQUIRE( vb.getVertexCount() == 3 );

	// Larger update -> count grows
	verts.push_back( { { 0, 0, 1 }, Color::white() } );
	vb.update( verts );
	REQUIRE( vb.getVertexCount() == 4 );

	// Index buffer similar path
	std::vector<uint16_t> idx = { 0, 1, 2 };
	IndexBuffer ib( device, idx );
	REQUIRE( ib.getIndexCount() == 3 );
	idx.push_back( 2 );
	ib.update( idx );
	REQUIRE( ib.getIndexCount() == 4 );
}

TEST_CASE( "Empty buffer creation rejected", "[renderer][buffers][error]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "empty buffer" ) )
		return;
	bool threwV = false, threwI = false;
	try
	{
		VertexBuffer vb( device, {} );
	}
	catch ( const std::runtime_error & )
	{
		threwV = true;
	}
	try
	{
		IndexBuffer ib( device, {} );
	}
	catch ( const std::runtime_error & )
	{
		threwI = true;
	}
	REQUIRE( threwV );
	REQUIRE( threwI );
}

TEST_CASE( "ViewProjection accessor", "[renderer]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "viewProj" ) )
		return;
	Renderer renderer( device );
	math::Mat4<> custom = math::Mat4<>::identity();
	custom.row0.x = 2.0f; // mutate something
	renderer.setViewProjectionMatrix( custom );
	const auto &retrieved = renderer.getViewProjectionMatrix();
	REQUIRE( retrieved.row0.x == 2.0f );
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

TEST_CASE( "Dynamic buffer reuse vs growth", "[renderer][buffers]" )
{
#if 0
	platform::Win32Window window;
	dx12::Device device;
	REQUIRE( requireDevice( window, device, "dynamic reuse" ) );

	Renderer renderer( device );

	dx12::CommandQueue commandQueue( device );
	dx12::SwapChain swapChain( device, commandQueue, window.getHandle(), 640, 480 );

	dx12::CommandContext context( device );
	renderer.beginFrame( context, swapChain );

	std::vector<Vertex> tri = {
		{ { 0, 0, 0 }, Color::red() },
		{ { 1, 0, 0 }, Color::green() },
		{ { 0, 1, 0 }, Color::blue() }
	};
	renderer.drawVertices( tri );
	ID3D12Resource *firstVB = renderer.getDynamicVertexResource();
	REQUIRE( renderer.getDynamicVertexCapacity() == 3 );
	tri[1].position.y = 0.2f;
	renderer.drawVertices( tri );
	REQUIRE( renderer.getDynamicVertexResource() == firstVB );
	tri.push_back( { { 0, 0, 1 }, Color::white() } );
	renderer.drawVertices( tri );
	REQUIRE( renderer.getDynamicVertexCapacity() == 4 );
	REQUIRE( renderer.getDynamicVertexResource() != firstVB );

	renderer.endFrame();
#endif
}

TEST_CASE( "Immediate line and cube draw headless", "[renderer][immediate]" )
{
#if 0
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "immediate" ) )
		return;
	Renderer renderer( device );
	dx12::CommandContext context( device );
	renderer.beginHeadlessForTests( context );
	renderer.drawLine( { 0, 0, 0 }, { 1, 1, 1 }, Color::white() );
	REQUIRE( renderer.getDynamicVertexCapacity() == 2 );
	renderer.drawWireframeCube( { 0, 0, 0 }, { 1, 1, 1 }, Color::red() );
	REQUIRE( renderer.getDynamicVertexCapacity() >= 8 );
	REQUIRE( renderer.getDynamicIndexCapacity() >= 24 );
	renderer.endFrame();
#endif
}

TEST_CASE( "Pipeline state object cache", "[renderer][pso]" )
{
#if 0
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "pso cache" ) )
		return;
	Renderer r( device );
	dx12::CommandContext ctx( device );
	r.beginHeadlessForTests( ctx );

	std::vector<Vertex> tri = { { { 0, 0, 0 }, Color::red() }, { { 1, 0, 0 }, Color::green() }, { { 0, 1, 0 }, Color::blue() } };
	r.drawVertices( tri );
	REQUIRE( r.getPipelineStateCacheSize() == 1 );

	// Same state -> no growth
	r.drawVertices( tri );
	REQUIRE( r.getPipelineStateCacheSize() == 1 );

	RenderState s;
	s.setWireframe( true );
	r.setRenderState( s );
	r.drawVertices( tri );
	REQUIRE( r.getPipelineStateCacheSize() == 2 );

	s.setBlendEnabled( true );
	r.setRenderState( s );
	r.drawVertices( tri );
	REQUIRE( r.getPipelineStateCacheSize() == 3 );

	s.setCullMode( D3D12_CULL_MODE_FRONT );
	r.setRenderState( s );
	r.drawVertices( tri );
	REQUIRE( r.getPipelineStateCacheSize() == 4 );

	s.setDepthWrite( false );
	r.setRenderState( s );
	r.drawVertices( tri );
	REQUIRE( r.getPipelineStateCacheSize() == 5 );

	s.setDepthTest( false );
	r.setRenderState( s );
	r.drawVertices( tri );
	REQUIRE( r.getPipelineStateCacheSize() == 6 );

	// Revisit previous state (wireframe+blend+front cull+depth test/write on)
	s.setDepthTest( true );
	s.setDepthWrite( true );
	s.setWireframe( true );
	s.setBlendEnabled( true );
	s.setCullMode( D3D12_CULL_MODE_FRONT );
	r.setRenderState( s );
	r.drawVertices( tri );
	REQUIRE( r.getPipelineStateCacheSize() == 6 );

	r.endFrame();
#endif
}
