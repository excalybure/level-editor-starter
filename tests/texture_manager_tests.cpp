// D3D12 dx12::TextureManager comprehensive tests
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "test_dx12_helpers.h"

#include "platform/dx12/dx12_device.h"
#include "engine/assets/assets.h"
#include "engine/gltf_loader/gltf_loader.h"
#include "graphics/texture/scene_texture_loader.h"

using Catch::Matchers::WithinAbs;

TEST_CASE( "TextureManager Initialization and Lifecycle", "[dx12][texture][manager]" )
{
	SECTION( "TextureManager initialization with valid device" )
	{
		dx12::Device device;
		if ( !requireHeadlessDevice( device, "dx12::TextureManager initialization" ) )
			return;

		dx12::TextureManager *manager = device.getTextureManager();
		REQUIRE( manager != nullptr );

		// Should be able to initialize successfully
		REQUIRE( manager->initialize( &device ) );
	}

	SECTION( "TextureManager initialization with null device" )
	{
		dx12::TextureManager manager;

		// Should fail with null device
		REQUIRE_FALSE( manager.initialize( nullptr ) );
	}

	SECTION( "TextureManager shutdown safety" )
	{
		dx12::Device device;
		if ( !requireHeadlessDevice( device, "dx12::TextureManager shutdown" ) )
			return;

		dx12::TextureManager *manager = device.getTextureManager();
		REQUIRE( manager->initialize( &device ) );

		// Should be safe to shut down multiple times
		REQUIRE_NOTHROW( manager->shutdown() );
		REQUIRE_NOTHROW( manager->shutdown() );
	}
}

TEST_CASE( "TextureManager Viewport Render Target Creation", "[dx12][texture][viewport]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "dx12::TextureManager viewport render targets" ) )
		return;

	dx12::TextureManager *manager = device.getTextureManager();
	REQUIRE( manager->initialize( &device ) );

	SECTION( "Valid render target creation" )
	{
		// Test standard resolutions
		auto texture1 = manager->createViewportRenderTarget( 800, 600 );
		REQUIRE( texture1 != nullptr );
		REQUIRE( texture1->getWidth() == 800 );
		REQUIRE( texture1->getHeight() == 600 );

		auto texture2 = manager->createViewportRenderTarget( 1920, 1080 );
		REQUIRE( texture2 != nullptr );
		REQUIRE( texture2->getWidth() == 1920 );
		REQUIRE( texture2->getHeight() == 1080 );

		// Different instances should be unique
		REQUIRE( texture1.get() != texture2.get() );
	}

	SECTION( "Multiple render targets" )
	{
		// Create multiple render targets
		std::vector<std::shared_ptr<dx12::Texture>> textures;

		for ( int i = 0; i < 8; ++i )
		{
			auto texture = manager->createViewportRenderTarget( 640 + i * 64, 480 + i * 48 );
			REQUIRE( texture != nullptr );
			textures.push_back( texture );
		}

		// All should be unique
		for ( size_t i = 0; i < textures.size(); ++i )
		{
			for ( size_t j = i + 1; j < textures.size(); ++j )
			{
				REQUIRE( textures[i].get() != textures[j].get() );
			}
		}
	}

	SECTION( "Invalid dimensions" )
	{
		// Test zero dimensions
		auto texture1 = manager->createViewportRenderTarget( 0, 600 );
		REQUIRE( texture1 == nullptr );

		auto texture2 = manager->createViewportRenderTarget( 800, 0 );
		REQUIRE( texture2 == nullptr );

		auto texture3 = manager->createViewportRenderTarget( 0, 0 );
		REQUIRE( texture3 == nullptr );
	}

	SECTION( "Extreme dimensions" )
	{
		// Test very large dimensions (should handle gracefully)
		auto largeTexture = manager->createViewportRenderTarget( 8192, 8192 );
		// Note: This might succeed or fail depending on system capabilities
		// The key is it shouldn't crash

		// Test very small but valid dimensions
		auto smallTexture = manager->createViewportRenderTarget( 1, 1 );
		REQUIRE( smallTexture != nullptr );
		REQUIRE( smallTexture->getWidth() == 1 );
		REQUIRE( smallTexture->getHeight() == 1 );
	}
}

TEST_CASE( "TextureManager SRV Handle Management", "[dx12][texture][srv]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "dx12::TextureManager SRV handles" ) )
		return;

	dx12::TextureManager *manager = device.getTextureManager();
	REQUIRE( manager->initialize( &device ) );

	SECTION( "SRV handle allocation" )
	{
		// Get multiple SRV handles
		auto handle1 = manager->getNextSrvHandle();
		auto handle2 = manager->getNextSrvHandle();
		auto handle3 = manager->getNextSrvHandle();

		// Handles should have different pointer values
		REQUIRE( handle1.ptr != handle2.ptr );
		REQUIRE( handle2.ptr != handle3.ptr );
		REQUIRE( handle1.ptr != handle3.ptr );
	}

	SECTION( "SRV handle consistency" )
	{
		// Sequential handles should be incremental
		auto handle1 = manager->getNextSrvHandle();
		auto handle2 = manager->getNextSrvHandle();

		// The difference should be consistent with descriptor size
		// (exact value depends on implementation)
		REQUIRE( handle2.ptr > handle1.ptr );
	}

	SECTION( "SRV handle exhaustion resilience" )
	{
		// Create many handles to test bounds checking
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> handles;

		for ( int i = 0; i < TextureManager::kMaxTextures + 10; ++i )
		{
			auto handle = manager->getNextSrvHandle();
			handles.push_back( handle );
		}

		// Should handle gracefully without crashing
		REQUIRE( handles.size() == TextureManager::kMaxTextures + 10 );
	}
}

TEST_CASE( "TextureManager Constants and Limits", "[dx12][texture][constants]" )
{
	SECTION( "Compile-time constants" )
	{
		// Verify expected constants
		REQUIRE( TextureManager::kMaxTextures == 64 );
		REQUIRE( TextureManager::kSrvIndexOffset == 16 );

		// Constants should be reasonable
		REQUIRE( TextureManager::kMaxTextures > 0 );
		REQUIRE( TextureManager::kSrvIndexOffset >= 0 );
		REQUIRE( TextureManager::kMaxTextures + TextureManager::kSrvIndexOffset < 1024 ); // Reasonable upper bound
	}

	SECTION( "Maximum texture creation" )
	{
		dx12::Device device;
		if ( !requireHeadlessDevice( device, "Maximum texture creation" ) )
			return;

		dx12::TextureManager *manager = device.getTextureManager();
		REQUIRE( manager->initialize( &device ) );

		// Create up to the maximum number of textures
		std::vector<std::shared_ptr<dx12::Texture>> textures;

		for ( UINT i = 0; i < TextureManager::kMaxTextures && i < 16; ++i ) // Limit to 16 for test performance
		{
			auto texture = manager->createViewportRenderTarget( 256, 256 );
			if ( texture )
			{
				textures.push_back( texture );
			}
		}

		// Should have created at least some textures
		REQUIRE( textures.size() > 0 );
		REQUIRE( textures.size() <= TextureManager::kMaxTextures );
	}
}

TEST_CASE( "TextureManager Scene Texture Loading", "[dx12][texture][scene][integration]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "dx12::TextureManager scene texture loading" ) )
		return;

	dx12::TextureManager *manager = device.getTextureManager();
	REQUIRE( manager->initialize( &device ) );

	SECTION( "Load textures for scene materials - missing textures" )
	{
		// Create a simple scene with materials that have texture paths
		auto scene = std::make_shared<assets::Scene>();
		auto material = std::make_shared<assets::Material>();
		auto &pbr = material->getPBRMaterial();

		// Set up texture paths (these don't need to exist for structure test)
		pbr.baseColorTexture = "textures/albedo.png";
		pbr.metallicRoughnessTexture = "textures/metal_rough.png";

		scene->addMaterial( material );
		scene->setBasePath( "assets/test" );

		// Initially handles should be invalid
		REQUIRE( pbr.baseColorTextureHandle == 0 );
		REQUIRE( pbr.metallicRoughnessTextureHandle == 0 );

		// Call utility function to load textures
		const int texturesLoaded = graphics::texture::loadSceneTextures( scene, manager );

		// Textures don't exist, so expect warnings but graceful handling
		REQUIRE( texturesLoaded == 0 );				// No textures loaded (files don't exist)
		REQUIRE( pbr.baseColorTextureHandle == 0 ); // Should remain invalid
		REQUIRE( pbr.metallicRoughnessTextureHandle == 0 );
	}

	SECTION( "Load textures from real glTF file with valid texture" )
	{
		// Load glTF that references test_red_2x2.png
		const gltf_loader::GLTFLoader loader;
		auto scene = loader.loadScene( "assets/test/triangle_with_texture.gltf" );
		REQUIRE( scene );

		// Verify scene has materials and texture paths
		REQUIRE( scene->getMaterials().size() == 1 );
		auto &pbr = scene->getMaterials()[0]->getPBRMaterial();
		REQUIRE( !pbr.baseColorTexture.empty() );
		REQUIRE( pbr.baseColorTextureHandle == 0 ); // Not loaded yet

		// Load textures
		const int texturesLoaded = graphics::texture::loadSceneTextures( scene, manager );

		// Should have loaded the base color texture
		REQUIRE( texturesLoaded == 1 );
		REQUIRE( pbr.baseColorTextureHandle != 0 ); // Valid handle
		REQUIRE( pbr.baseColorTextureHandle != graphics::texture::kInvalidTextureHandle );
	}
}
