#include <catch2/catch_test_macros.hpp>
#include <unordered_map>

#include "graphics/gpu/mesh_gpu.h"
#include "graphics/gpu/material_gpu.h"
#include "graphics/texture/texture_manager.h"
#include "graphics/texture/bindless_texture_heap.h"
#include "engine/assets/assets.h"
#include "platform/dx12/dx12_device.h"

// Mock MaterialProvider for testing
class TestMaterialProvider : public graphics::gpu::MaterialProvider
{
private:
	dx12::Device *m_device;
	graphics::texture::TextureManager *m_textureManager;
	std::shared_ptr<graphics::gpu::MaterialGPU> m_defaultMaterial;

public:
	TestMaterialProvider( dx12::Device &device, graphics::texture::TextureManager &textureManager ) noexcept
		: m_device( &device ), m_textureManager( &textureManager )
	{
		auto defaultMat = std::make_shared<assets::Material>();
		defaultMat->setName( "default" );
		defaultMat->setPath( "default" );
		defaultMat->setLoaded( true );
		m_defaultMaterial = std::make_shared<graphics::gpu::MaterialGPU>( defaultMat, device, textureManager );
	}

	~TestMaterialProvider() noexcept override = default;

	std::shared_ptr<graphics::gpu::MaterialGPU> getMaterialGPU( std::shared_ptr<assets::Material> material ) override
	{
		if ( !material )
			return getDefaultMaterialGPU();

		// Simple implementation for test - just create new MaterialGPU each time
		auto materialGPU = std::make_shared<graphics::gpu::MaterialGPU>( material, *m_device, *m_textureManager );
		return materialGPU;
	}

	std::shared_ptr<graphics::gpu::MaterialGPU> getMaterialGPU( std::shared_ptr<assets::Material> material, const assets::MaterialInstance *instance ) override
	{
		if ( !material )
			return getDefaultMaterialGPU();

		// Create new MaterialGPU with instance - don't cache since instance-specific
		return std::make_shared<graphics::gpu::MaterialGPU>( material, *m_device, instance, *m_textureManager );
	}

	std::shared_ptr<graphics::gpu::MaterialGPU> getDefaultMaterialGPU() override
	{
		return m_defaultMaterial;
	}
};

TEST_CASE( "MeshGPU::configureMaterials applies primitive MaterialInstance to MaterialGPU", "[MeshGPU][MaterialInstance][integration]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );
	graphics::texture::TextureManager textureManager;
	textureManager.initialize( &device, 1024 );

	// Create a material for the primitive
	auto baseMaterial = std::make_shared<assets::Material>();
	baseMaterial->setName( "test_material" );
	baseMaterial->getPBRMaterial().baseColorFactor = { 0.5f, 0.5f, 0.5f, 1.0f };
	baseMaterial->getPBRMaterial().metallicFactor = 0.3f;
	baseMaterial->getPBRMaterial().roughnessFactor = 0.6f;
	baseMaterial->setPath( "test_material" );
	baseMaterial->setLoaded( true );

	// Create a scene with the material
	auto scene = std::make_shared<assets::Scene>();
	assets::MaterialHandle materialHandle = scene->addMaterial( baseMaterial );

	// Create a primitive with material and MaterialInstance override
	assets::Primitive primitive;
	primitive.addVertex( assets::Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } } );
	primitive.addVertex( assets::Vertex{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } } );
	primitive.addVertex( assets::Vertex{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f } } );
	primitive.addIndex( 0 );
	primitive.addIndex( 1 );
	primitive.addIndex( 2 );
	primitive.setMaterialHandle( materialHandle );

	// Add MaterialInstance override to primitive
	assets::MaterialInstance instance;
	instance.baseMaterial = materialHandle;
	instance.baseColorFactorOverride = math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f }; // Red override
	instance.metallicFactorOverride = 0.8f;
	primitive.setMaterialInstance( instance );

	// Create mesh with the primitive
	auto mesh = std::make_shared<assets::Mesh>();
	mesh->addPrimitive( primitive );

	// Create MeshGPU
	graphics::gpu::MeshGPU meshGPU{ device, *mesh };
	REQUIRE( meshGPU.isValid() );

	// Create test material provider
	TestMaterialProvider materialProvider{ device, textureManager };

	// Act - Configure materials with MaterialInstance support
	meshGPU.configureMaterials( materialProvider, *scene, *mesh );

	// Assert - Primitive should have material with overrides applied
	REQUIRE( meshGPU.getPrimitive( 0 ).hasMaterial() );
	const auto &material = meshGPU.getPrimitive( 0 ).getMaterial();
	REQUIRE( material->isValid() );

	// The material should have the override values, not base material values
	const auto &constants = material->getMaterialConstants();
	REQUIRE( constants.baseColorFactor == math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f } ); // Override, not base 0.5f
	REQUIRE( constants.metallicFactor == 0.8f );								   // Override, not base 0.3f
	REQUIRE( constants.roughnessFactor == 0.6f );								   // Base value (not overridden)
}

TEST_CASE( "MeshGPU::configureMaterials without MaterialInstance uses base material", "[MeshGPU][MaterialInstance][integration]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );
	graphics::texture::TextureManager textureManager;
	textureManager.initialize( &device, 1024 );

	// Create a material
	auto baseMaterial = std::make_shared<assets::Material>();
	baseMaterial->setName( "test_material2" );
	baseMaterial->getPBRMaterial().baseColorFactor = { 0.7f, 0.7f, 0.7f, 1.0f };
	baseMaterial->getPBRMaterial().metallicFactor = 0.4f;
	baseMaterial->getPBRMaterial().roughnessFactor = 0.5f;
	baseMaterial->setPath( "test_material2" );
	baseMaterial->setLoaded( true );

	// Create scene with material
	auto scene = std::make_shared<assets::Scene>();
	assets::MaterialHandle materialHandle = scene->addMaterial( baseMaterial );

	// Create primitive without MaterialInstance override
	assets::Primitive primitive;
	primitive.addVertex( assets::Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } } );
	primitive.addVertex( assets::Vertex{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } } );
	primitive.addVertex( assets::Vertex{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f } } );
	primitive.addIndex( 0 );
	primitive.addIndex( 1 );
	primitive.addIndex( 2 );
	primitive.setMaterialHandle( materialHandle );
	// No MaterialInstance override

	// Create mesh
	auto mesh = std::make_shared<assets::Mesh>();
	mesh->addPrimitive( primitive );

	// Create MeshGPU
	graphics::gpu::MeshGPU meshGPU{ device, *mesh };
	REQUIRE( meshGPU.isValid() );

	// Create material provider
	TestMaterialProvider materialProvider{ device, textureManager };

	// Act - Configure materials
	meshGPU.configureMaterials( materialProvider, *scene, *mesh );

	// Assert - Should use base material values
	REQUIRE( meshGPU.getPrimitive( 0 ).hasMaterial() );
	const auto &material = meshGPU.getPrimitive( 0 ).getMaterial();
	REQUIRE( material->isValid() );

	const auto &constants = material->getMaterialConstants();
	REQUIRE( constants.baseColorFactor == math::Vec4f{ 0.7f, 0.7f, 0.7f, 1.0f } );
	REQUIRE( constants.metallicFactor == 0.4f );
	REQUIRE( constants.roughnessFactor == 0.5f );
}

TEST_CASE( "MeshGPU::configureMaterials with multiple primitives with different overrides", "[MeshGPU][MaterialInstance][integration]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );
	graphics::texture::TextureManager textureManager;
	textureManager.initialize( &device, 1024 );

	// Create base material
	auto baseMaterial = std::make_shared<assets::Material>();
	baseMaterial->setName( "multi_test" );
	baseMaterial->getPBRMaterial().baseColorFactor = { 0.5f, 0.5f, 0.5f, 1.0f };
	baseMaterial->getPBRMaterial().metallicFactor = 0.3f;
	baseMaterial->setPath( "multi_test" );
	baseMaterial->setLoaded( true );

	// Create scene
	auto scene = std::make_shared<assets::Scene>();
	assets::MaterialHandle materialHandle = scene->addMaterial( baseMaterial );

	// Create 3 primitives with different override configurations
	auto createPrimitive = []() {
		assets::Primitive p;
		p.addVertex( assets::Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } } );
		p.addVertex( assets::Vertex{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } } );
		p.addVertex( assets::Vertex{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f } } );
		p.addIndex( 0 );
		p.addIndex( 1 );
		p.addIndex( 2 );
		return p;
	};

	auto mesh = std::make_shared<assets::Mesh>();

	// Primitive 0: Red with high metallic
	auto prim0 = createPrimitive();
	prim0.setMaterialHandle( materialHandle );
	assets::MaterialInstance inst0;
	inst0.baseMaterial = materialHandle;
	inst0.baseColorFactorOverride = math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f };
	inst0.metallicFactorOverride = 0.9f;
	prim0.setMaterialInstance( inst0 );
	mesh->addPrimitive( prim0 );

	// Primitive 1: Green, no overrides
	auto prim1 = createPrimitive();
	prim1.setMaterialHandle( materialHandle );
	mesh->addPrimitive( prim1 );

	// Primitive 2: Blue with no metallic
	auto prim2 = createPrimitive();
	prim2.setMaterialHandle( materialHandle );
	assets::MaterialInstance inst2;
	inst2.baseMaterial = materialHandle;
	inst2.baseColorFactorOverride = math::Vec4f{ 0.0f, 0.0f, 1.0f, 1.0f };
	inst2.metallicFactorOverride = 0.0f;
	prim2.setMaterialInstance( inst2 );
	mesh->addPrimitive( prim2 );

	// Create MeshGPU
	graphics::gpu::MeshGPU meshGPU{ device, *mesh };
	REQUIRE( meshGPU.isValid() );
	REQUIRE( meshGPU.getPrimitiveCount() == 3 );

	// Create material provider
	TestMaterialProvider materialProvider{ device, textureManager };

	// Act - Configure materials
	meshGPU.configureMaterials( materialProvider, *scene, *mesh );

	// Assert - Each primitive should have correct material values
	// Primitive 0: Red, high metallic
	REQUIRE( meshGPU.getPrimitive( 0 ).hasMaterial() );
	const auto &mat0 = meshGPU.getPrimitive( 0 ).getMaterial();
	REQUIRE( mat0->getMaterialConstants().baseColorFactor == math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f } );
	REQUIRE( mat0->getMaterialConstants().metallicFactor == 0.9f );

	// Primitive 1: Base material values
	REQUIRE( meshGPU.getPrimitive( 1 ).hasMaterial() );
	const auto &mat1 = meshGPU.getPrimitive( 1 ).getMaterial();
	REQUIRE( mat1->getMaterialConstants().baseColorFactor == baseMaterial->getPBRMaterial().baseColorFactor );
	REQUIRE( mat1->getMaterialConstants().metallicFactor == baseMaterial->getPBRMaterial().metallicFactor );

	// Primitive 2: Blue, no metallic
	REQUIRE( meshGPU.getPrimitive( 2 ).hasMaterial() );
	const auto &mat2 = meshGPU.getPrimitive( 2 ).getMaterial();
	REQUIRE( mat2->getMaterialConstants().baseColorFactor == math::Vec4f{ 0.0f, 0.0f, 1.0f, 1.0f } );
	REQUIRE( mat2->getMaterialConstants().metallicFactor == 0.0f );
}
