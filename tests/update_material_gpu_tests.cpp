#include <catch2/catch_test_macros.hpp>

#include "editor/commands/PrimitiveMaterialCommands.h"
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "engine/assets/assets.h"
#include "math/vec.h"

#include "graphics/gpu/gpu_resource_manager.h"
#include "graphics/texture/bindless_texture_heap.h"
#include "graphics/texture/texture_manager.h"
#include "platform/dx12/dx12_device.h"
#include "graphics/gpu/material_gpu.h"

TEST_CASE( "updateMaterialGPUForPrimitive applies MaterialInstance overrides to MaterialGPU", "[primitive-material][gpu][unit]" )
{
	// Arrange - create headless device and managers
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );
	graphics::texture::TextureManager textureManager;
	textureManager.initialize( &device, 1024 );
	graphics::GPUResourceManager manager( device, textureManager );
	REQUIRE( manager.isValid() );

	// Create a material with base values
	auto material = std::make_shared<assets::Material>();
	material->setName( "BaseMat" );
	auto &pbr = material->getPBRMaterial();
	pbr.baseColorFactor = math::Vec4f{ 0.2f, 0.3f, 0.4f, 1.0f };
	pbr.metallicFactor = 0.1f;
	pbr.roughnessFactor = 0.9f;
	material->setLoaded( true );

	// Create scene and add material
	auto scene = std::make_shared<assets::Scene>();
	const auto matHandle = scene->addMaterial( material );

	// Create primitive with that material and a MaterialInstance override
	assets::Primitive prim;
	// Add a simple triangle (vertices + indices) so GPU buffers can be created
	prim.addVertex( assets::Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	prim.addVertex( assets::Vertex{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	prim.addVertex( assets::Vertex{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	prim.addIndex( 0 );
	prim.addIndex( 1 );
	prim.addIndex( 2 );
	prim.setMaterialHandle( matHandle );
	assets::MaterialInstance instance;
	instance.baseMaterial = matHandle;
	instance.baseColorFactorOverride = math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f }; // override to red
	instance.metallicFactorOverride = 0.8f;
	prim.setMaterialInstance( instance );

	// Create mesh and add primitive
	auto mesh = std::make_shared<assets::Mesh>();
	mesh->addPrimitive( prim );
	const auto meshHandle = scene->addMesh( mesh );

	// Create MeshGPU via manager and configure materials
	const auto meshGPU = manager.getMeshGPU( mesh );
	REQUIRE( meshGPU != nullptr );
	meshGPU->configureMaterials( manager, *scene, *mesh );

	// Create ECS scene and entity with MeshRenderer that holds gpuMesh
	ecs::Scene ecsScene;
	const auto entity = ecsScene.createEntity();
	components::MeshRenderer mr;
	mr.meshHandle = meshHandle;
	mr.gpuMesh = meshGPU;
	ecsScene.addComponent( entity, mr );

	// Act - update material GPU for primitive 0
	const bool updated = editor::updateMaterialGPUForPrimitive( entity, 0u, &ecsScene, scene.get(), &manager );

	// Assert - update should have occurred and material constants reflect overrides
	REQUIRE( updated );
	const auto &primitiveGPU = mr.gpuMesh->getPrimitive( 0 );
	REQUIRE( primitiveGPU.hasMaterial() );
	const auto materialGPU = primitiveGPU.getMaterial();
	REQUIRE( materialGPU != nullptr );

	const auto &constants = materialGPU->getMaterialConstants();
	REQUIRE( constants.baseColorFactor == math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f } );
	REQUIRE( constants.metallicFactor == 0.8f );
}
