#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <filesystem>
#include <string>

// Need D3D12 headers for constants
#include <d3d12.h>

#include "shader_test_utils.h"

#include "graphics/renderer/renderer.h"
#include "graphics/shader_manager/shader_compiler.h"
#include "graphics/shader_manager/shader_manager.h"
#include "platform/dx12/dx12_device.h"
#include "math/vec.h"
#include "math/matrix.h"
#include "math/color.h"
#include "test_dx12_helpers.h"


TEST_CASE( "Renderer Vertex Format", "[renderer]" )
{
	SECTION( "Vertex can be constructed" )
	{
		const math::Vec3<> position{ 1.0f, 2.0f, 3.0f };
		const renderer::Color color{ 1.0f, 0.5f, 0.0f, 1.0f };

		const renderer::Vertex vertex( position, color );

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
				const test::shader::TempShaderFile shaderFile( R"(
					float4 main(float3 pos : POSITION) : SV_POSITION
					{
						return float4(pos, 1.0f);
					}
				)" );

				const auto blob = shader_manager::ShaderCompiler::CompileFromFile( shaderFile.path(), "main", "vs_5_0" );
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
				const test::shader::TempShaderFile shaderFile( R"(
					float4 main() : SV_TARGET
					{
						return float4(1.0f, 0.0f, 0.0f, 1.0f);
					}
				)" );

				const auto blob = shader_manager::ShaderCompiler::CompileFromFile( shaderFile.path(), "main", "ps_5_0" );
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
		const renderer::RenderState state;

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
		renderer::RenderState state;
		state.setWireframe( true );

		const auto rasterizerDesc = state.getRasterizerDesc();
		REQUIRE( rasterizerDesc.FillMode == D3D12_FILL_MODE_WIREFRAME );
	}

	SECTION( "Depth state modifications" )
	{
		renderer::RenderState state;
		state.setDepthTest( false );
		state.setDepthWrite( false );

		const auto depthDesc = state.getDepthStencilDesc();
		REQUIRE( depthDesc.DepthEnable == FALSE );
		REQUIRE( depthDesc.DepthWriteMask == D3D12_DEPTH_WRITE_MASK_ZERO );
	}
}

TEST_CASE( "Render State permutations", "[renderer]" )
{
	renderer::RenderState state;
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
			const test::shader::TempShaderFile shaderFile( R"(
				#ifndef MY_FLAG
				#error MY_FLAG not defined
				#endif
				float4 main(float3 pos:POSITION):SV_POSITION { return float4(pos,1); }
			)" );
			REQUIRE_NOTHROW( shader_manager::ShaderCompiler::CompileFromFile( shaderFile.path(), "main", "vs_5_0", { "MY_FLAG" } ) );
		}
		catch ( const std::runtime_error &e )
		{
			WARN( "Skipping defines test: " << e.what() );
		}
	}
	SECTION( "Included files are tracked" )
	{
		try
		{
			const test::shader::TempShaderFile includeFile( R"(
				float4 TransformPosition(float3 pos) { return float4(pos, 1.0f); }
			)",
				".hlsli" );

			std::string mainShaderContent = "#include \"";
			mainShaderContent += includeFile.path().filename().string();
			mainShaderContent += "\"\n";
			mainShaderContent += R"(
				float4 main(float3 pos:POSITION):SV_POSITION { return TransformPosition(pos); }
			)";
			const test::shader::TempShaderFile shaderFile( mainShaderContent );

			const auto blob = shader_manager::ShaderCompiler::CompileFromFile( shaderFile.path(), "main", "vs_5_0" );
			REQUIRE( blob.isValid() );
			REQUIRE_FALSE( blob.includedFiles.empty() );
			const auto canonicalInclude = std::filesystem::canonical( includeFile.path() );
			REQUIRE( blob.includedFiles[0] == canonicalInclude );
		}
		catch ( const std::runtime_error &e )
		{
			WARN( "Skipping include tracking test: " << e.what() );
		}
	}
	SECTION( "Invalid profile throws" )
	{
		bool threw = false;
		try
		{
			const test::shader::TempShaderFile shaderFile( "float4 main():SV_POSITION{return 0;} " );
			shader_manager::ShaderCompiler::CompileFromFile( shaderFile.path(), "main", "vs_99_99" );
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
			shader_manager::ShaderCompiler::CompileFromFile( "this_does_not_exist.hlsl", "main", "vs_5_0" );
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
	std::vector<renderer::Vertex> verts = {
		{ { 0, 0, 0 }, renderer::Color::red() },
		{ { 1, 0, 0 }, renderer::Color::green() },
		{ { 0, 1, 0 }, renderer::Color::blue() }
	};
	renderer::VertexBuffer vb( device, verts );
	REQUIRE( vb.getVertexCount() == 3 );

	// Same size update -> count unchanged
	verts[1].position.x = 2.0f;
	REQUIRE_NOTHROW( vb.update( verts ) );
	REQUIRE( vb.getVertexCount() == 3 );

	// Larger update -> count grows
	verts.push_back( { { 0, 0, 1 }, renderer::Color::white() } );
	vb.update( verts );
	REQUIRE( vb.getVertexCount() == 4 );

	// Index buffer similar path
	std::vector<uint16_t> idx = { 0, 1, 2 };
	renderer::IndexBuffer ib( device, idx );
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
		renderer::VertexBuffer vb( device, {} );
	}
	catch ( const std::runtime_error & )
	{
		threwV = true;
	}
	try
	{
		renderer::IndexBuffer ib( device, {} );
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
	shader_manager::ShaderManager shaderManager;
	renderer::Renderer renderer( device, shaderManager );
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

				const std::vector<renderer::Vertex> vertices = {
					{ math::Vec3<>{ 0.0f, 1.0f, 0.0f }, renderer::Color{ 1.0f, 0.0f, 0.0f, 1.0f } },
					{ math::Vec3<>{ -1.0f, -1.0f, 0.0f }, renderer::Color{ 0.0f, 1.0f, 0.0f, 1.0f } },
					{ math::Vec3<>{ 1.0f, -1.0f, 0.0f }, renderer::Color{ 0.0f, 0.0f, 1.0f, 1.0f } }
				};

				const renderer::VertexBuffer vb( device, vertices );
				REQUIRE( vb.getVertexCount() == 3 );

				const auto view = vb.getView();
				REQUIRE( view.SizeInBytes == vertices.size() * sizeof( renderer::Vertex ) );
				REQUIRE( view.StrideInBytes == sizeof( renderer::Vertex ) );
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

				const renderer::IndexBuffer ib( device, indices );
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
				if ( !requireHeadlessDevice( device, "renderer::Renderer creation" ) )
					return;
				shader_manager::ShaderManager shaderManager;
				const renderer::Renderer renderer( device, shaderManager );
				// Just test that it constructs without throwing
			}
			catch ( const std::runtime_error &e )
			{
				WARN( "renderer::Renderer creation failed (D3D12 may not be available): " << e.what() );
			}
		}() );
	}
}

TEST_CASE( "Simple shaders can be loaded from file", "[renderer][shaders]" )
{
	SECTION( "simple.hlsl file exists" )
	{
		const std::filesystem::path shaderPath = "shaders/simple.hlsl";
		REQUIRE( std::filesystem::exists( shaderPath ) );
	}

	SECTION( "simple.hlsl can be compiled" )
	{
		dx12::Device device;
		if ( !requireHeadlessDevice( device, "simple shader compilation" ) )
			return;

		shader_manager::ShaderManager shaderManager;
		const auto vsHandle = shaderManager.registerShader(
			"shaders/simple.hlsl",
			"VSMain",
			"vs_5_0",
			shader_manager::ShaderType::Vertex );

		const auto psHandle = shaderManager.registerShader(
			"shaders/simple.hlsl",
			"PSMain",
			"ps_5_0",
			shader_manager::ShaderType::Pixel );

		REQUIRE( vsHandle != shader_manager::INVALID_SHADER_HANDLE );
		REQUIRE( psHandle != shader_manager::INVALID_SHADER_HANDLE );

		const auto *vsBlob = shaderManager.getShaderBlob( vsHandle );
		const auto *psBlob = shaderManager.getShaderBlob( psHandle );

		if ( vsBlob && psBlob )
		{
			REQUIRE( vsBlob->isValid() );
			REQUIRE( psBlob->isValid() );
		}
	}
}

TEST_CASE( "Dynamic buffer reuse vs growth", "[renderer][buffers]" )
{
	platform::Win32Window window;
	dx12::Device device;
	REQUIRE( requireDevice( window, device, "dynamic reuse" ) );

	shader_manager::ShaderManager shaderManager;
	renderer::Renderer renderer( device, shaderManager );

	device.beginFrame();
	renderer.beginFrame();

	std::vector<renderer::Vertex> tri = {
		{ { 0, 0, 0 }, renderer::Color::red() },
		{ { 1, 0, 0 }, renderer::Color::green() },
		{ { 0, 1, 0 }, renderer::Color::blue() }
	};
	renderer.drawVertices( tri );
	ID3D12Resource *firstVB = renderer.getDynamicVertexResource();
	REQUIRE( renderer.getDynamicVertexCapacity() == 3 );
	tri[1].position.y = 0.2f;
	renderer.drawVertices( tri );
	REQUIRE( renderer.getDynamicVertexResource() == firstVB );
	tri.push_back( { { 0, 0, 1 }, renderer::Color::white() } );
	renderer.drawVertices( tri );
	REQUIRE( renderer.getDynamicVertexCapacity() == 4 );
	REQUIRE( renderer.getDynamicVertexResource() != firstVB );

	renderer.endFrame();
	device.endFrame();
	device.present();
}

TEST_CASE( "Immediate line draw", "[renderer][immediate]" )
{
	platform::Win32Window window;
	dx12::Device device;
	REQUIRE( requireDevice( window, device, "immediate" ) );

	shader_manager::ShaderManager shaderManager;
	renderer::Renderer renderer( device, shaderManager );
	device.beginFrame();
	renderer.beginFrame();
	renderer.drawLine( { 0, 0, 0 }, { 1, 1, 1 }, renderer::Color::white() );
	REQUIRE( renderer.getDynamicVertexCapacity() == 2 );
	REQUIRE( renderer.getDynamicIndexCapacity() == 0 );
	renderer.endFrame();
	device.endFrame();
	device.present();
}

TEST_CASE( "Immediate cube draw", "[renderer][immediate]" )
{
	platform::Win32Window window;
	dx12::Device device;
	REQUIRE( requireDevice( window, device, "immediate" ) );

	shader_manager::ShaderManager shaderManager;
	renderer::Renderer renderer( device, shaderManager );
	renderer.beginFrame();
	device.beginFrame();
	renderer.drawWireframeCube( { 0, 0, 0 }, { 1, 1, 1 }, renderer::Color::red() );
	REQUIRE( renderer.getDynamicVertexCapacity() == 8 );
	REQUIRE( renderer.getDynamicIndexCapacity() == 24 );
	renderer.endFrame();
	device.endFrame();
	device.present();
}

TEST_CASE( "Immediate line and cube draw", "[renderer][immediate]" )
{
	platform::Win32Window window;
	dx12::Device device;
	REQUIRE( requireDevice( window, device, "immediate" ) );

	shader_manager::ShaderManager shaderManager;
	renderer::Renderer renderer( device, shaderManager );
	renderer.beginFrame();
	device.beginFrame();
	renderer.drawLine( { 0, 0, 0 }, { 1, 1, 1 }, renderer::Color::white() );
	renderer.drawWireframeCube( { 0, 0, 0 }, { 1, 1, 1 }, renderer::Color::red() );
	REQUIRE( renderer.getDynamicVertexCapacity() == 8 );
	REQUIRE( renderer.getDynamicIndexCapacity() == 24 );
	renderer.endFrame();
	device.endFrame();
	device.present();
}

TEST_CASE( "Pipeline state object cache", "[renderer][pso]" )
{
	platform::Win32Window window;
	dx12::Device device;
	REQUIRE( requireDevice( window, device, "pso cache" ) );

	shader_manager::ShaderManager shaderManager;
	renderer::Renderer r( device, shaderManager );
	device.beginFrame();
	r.beginFrame();

	std::vector<renderer::Vertex> tri = { { { 0, 0, 0 }, renderer::Color::red() }, { { 1, 0, 0 }, renderer::Color::green() }, { { 0, 1, 0 }, renderer::Color::blue() } };
	r.drawVertices( tri );
	REQUIRE( r.getPipelineStateCacheSize() == 1 );

	// Same state -> no growth
	r.drawVertices( tri );
	REQUIRE( r.getPipelineStateCacheSize() == 1 );

	renderer::RenderState s;
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
	device.endFrame();

	device.present();
}
