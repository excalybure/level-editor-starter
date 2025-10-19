#include <catch2/catch_test_macros.hpp>

#include "editor/commands/PrimitiveMaterialCommands.h"
#include "editor/commands/Command.h"
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "engine/assets/assets.h"
#include "math/vec.h"
#include <nlohmann/json.hpp>

// NOTE: Tests for Copy/Paste commands cannot use ImGui clipboard directly
// in test environment (requires ImGui context). Instead, we test:
// 1. Command structure and properties (getDescription, getMemoryUsage, etc)
// 2. Direct MaterialInstance operations without clipboard
// 3. Command virtual functions (undo, canMergeWith, updateEntityReference)

// Helper function to create a simple test scene with a mesh
namespace
{
struct TestSceneData
{
	std::shared_ptr<assets::Scene> assetScene;
	assets::MeshHandle meshHandle;
	assets::MaterialHandle materialHandle;
};

TestSceneData createTestAssetScene()
{
	TestSceneData data;
	data.assetScene = std::make_shared<assets::Scene>();

	// Create a simple material
	auto material = std::make_shared<assets::Material>();
	material->setName( "TestMaterial" );
	auto &pbr = material->getPBRMaterial();
	pbr.baseColorFactor = math::Vec4f{ 1.0f, 1.0f, 1.0f, 1.0f };
	pbr.metallicFactor = 0.5f;
	pbr.roughnessFactor = 0.5f;
	pbr.emissiveFactor = math::Vec3f{ 0.0f, 0.0f, 0.0f };

	data.materialHandle = data.assetScene->addMaterial( material );

	// Create a simple mesh with one primitive
	auto mesh = std::make_shared<assets::Mesh>();

	// Add a primitive with basic geometry
	assets::Primitive primitive;
	primitive.setMaterialHandle( data.materialHandle );

	// Add some basic vertices
	for ( int i = 0; i < 3; ++i )
	{
		assets::Vertex vertex;
		vertex.position = math::Vec3f{ static_cast<float>( i ), 0.0f, 0.0f };
		primitive.addVertex( vertex );
		primitive.addIndex( static_cast<std::uint32_t>( i ) );
	}

	mesh->addPrimitive( std::move( primitive ) );

	data.meshHandle = data.assetScene->addMesh( mesh );

	return data;
}
} // namespace

TEST_CASE( "SetPrimitiveMaterialPropertyCommand - BaseColorFactor", "[primitive-material][unit]" )
{
	SECTION( "Execute sets base color override" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		// Create entity with mesh renderer
		const auto entity = ecsScene.createEntity( "TestEntity" );
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		// Create command to set base color
		const math::Vec4f newColor{ 1.0f, 0.0f, 0.0f, 1.0f }; // Red
		editor::SetPrimitiveMaterialPropertyCommand command(
			entity,
			0, // primitive index
			editor::MaterialPropertyType::BaseColorFactor,
			newColor,
			ecsScene,
			*testData.assetScene,
			nullptr // No GPU manager in unit tests
		);

		// Execute command
		REQUIRE( command.execute() );

		// Verify the override was set
		const auto mesh = testData.assetScene->getMesh( meshRenderer.meshHandle );
		REQUIRE( mesh );
		const auto &primitive = mesh->getPrimitive( 0 );
		const auto &materialInstance = primitive.getMaterialInstance();

		REQUIRE( materialInstance.baseColorFactorOverride.has_value() );
		REQUIRE( materialInstance.baseColorFactorOverride.value() == newColor );
	}

	SECTION( "Undo restores original value" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity();
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		// Set initial override
		const auto mesh = testData.assetScene->getMesh( meshRenderer.meshHandle );
		auto &primitive = mesh->getPrimitive( 0 );
		auto materialInstance = primitive.getMaterialInstance();
		const math::Vec4f originalColor{ 0.5f, 0.5f, 0.5f, 1.0f };
		materialInstance.baseColorFactorOverride = originalColor;
		primitive.setMaterialInstance( materialInstance );

		// Create command to change it
		const math::Vec4f newColor{ 1.0f, 0.0f, 0.0f, 1.0f };
		editor::SetPrimitiveMaterialPropertyCommand command(
			entity, 0, editor::MaterialPropertyType::BaseColorFactor, newColor, ecsScene, *testData.assetScene, nullptr );

		// Execute then undo
		REQUIRE( command.execute() );
		REQUIRE( command.undo() );

		// Verify original value restored
		const auto &restoredPrimitive = mesh->getPrimitive( 0 );
		const auto &restoredInstance = restoredPrimitive.getMaterialInstance();
		REQUIRE( restoredInstance.baseColorFactorOverride.has_value() );
		REQUIRE( restoredInstance.baseColorFactorOverride.value() == originalColor );
	}

	SECTION( "Undo clears override if none existed before" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity();
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		// No initial override - verify clean state
		const auto mesh = testData.assetScene->getMesh( meshRenderer.meshHandle );
		const auto &primitive = mesh->getPrimitive( 0 );
		REQUIRE_FALSE( primitive.getMaterialInstance().baseColorFactorOverride.has_value() );

		// Create and execute command
		const math::Vec4f newColor{ 1.0f, 0.0f, 0.0f, 1.0f };
		editor::SetPrimitiveMaterialPropertyCommand command(
			entity, 0, editor::MaterialPropertyType::BaseColorFactor, newColor, ecsScene, *testData.assetScene, nullptr );

		REQUIRE( command.execute() );
		REQUIRE( primitive.getMaterialInstance().baseColorFactorOverride.has_value() );

		// Undo should clear the override
		REQUIRE( command.undo() );
		REQUIRE_FALSE( primitive.getMaterialInstance().baseColorFactorOverride.has_value() );
	}
}

TEST_CASE( "SetPrimitiveMaterialPropertyCommand - MetallicFactor", "[primitive-material][unit]" )
{
	SECTION( "Execute sets metallic override" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity();
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		const float newMetallic = 0.8f;
		editor::SetPrimitiveMaterialPropertyCommand command(
			entity, 0, editor::MaterialPropertyType::MetallicFactor, newMetallic, ecsScene, *testData.assetScene, nullptr );

		REQUIRE( command.execute() );

		const auto mesh = testData.assetScene->getMesh( meshRenderer.meshHandle );
		const auto &primitive = mesh->getPrimitive( 0 );
		const auto &materialInstance = primitive.getMaterialInstance();

		REQUIRE( materialInstance.metallicFactorOverride.has_value() );
		REQUIRE( materialInstance.metallicFactorOverride.value() == newMetallic );
	}

	SECTION( "Undo restores metallic value" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity();
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		const float newMetallic = 0.9f;
		editor::SetPrimitiveMaterialPropertyCommand command(
			entity, 0, editor::MaterialPropertyType::MetallicFactor, newMetallic, ecsScene, *testData.assetScene, nullptr );

		REQUIRE( command.execute() );
		REQUIRE( command.undo() );

		const auto mesh = testData.assetScene->getMesh( meshRenderer.meshHandle );
		const auto &primitive = mesh->getPrimitive( 0 );
		REQUIRE_FALSE( primitive.getMaterialInstance().metallicFactorOverride.has_value() );
	}
}

TEST_CASE( "SetPrimitiveMaterialPropertyCommand - RoughnessFactor", "[primitive-material][unit]" )
{
	SECTION( "Execute sets roughness override" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity();
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		const float newRoughness = 0.2f;
		editor::SetPrimitiveMaterialPropertyCommand command(
			entity, 0, editor::MaterialPropertyType::RoughnessFactor, newRoughness, ecsScene, *testData.assetScene, nullptr );

		REQUIRE( command.execute() );

		const auto mesh = testData.assetScene->getMesh( meshRenderer.meshHandle );
		const auto &primitive = mesh->getPrimitive( 0 );
		const auto &materialInstance = primitive.getMaterialInstance();

		REQUIRE( materialInstance.roughnessFactorOverride.has_value() );
		REQUIRE( materialInstance.roughnessFactorOverride.value() == newRoughness );
	}
}

TEST_CASE( "SetPrimitiveMaterialPropertyCommand - EmissiveFactor", "[primitive-material][unit]" )
{
	SECTION( "Execute sets emissive override" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity();
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		const math::Vec3f newEmissive{ 1.0f, 0.5f, 0.0f };
		editor::SetPrimitiveMaterialPropertyCommand command(
			entity, 0, editor::MaterialPropertyType::EmissiveFactor, newEmissive, ecsScene, *testData.assetScene, nullptr );

		REQUIRE( command.execute() );

		const auto mesh = testData.assetScene->getMesh( meshRenderer.meshHandle );
		const auto &primitive = mesh->getPrimitive( 0 );
		const auto &materialInstance = primitive.getMaterialInstance();

		REQUIRE( materialInstance.emissiveFactorOverride.has_value() );
		REQUIRE( materialInstance.emissiveFactorOverride.value() == newEmissive );
	}
}

TEST_CASE( "SetPrimitiveMaterialPropertyCommand - Command merging", "[primitive-material][unit]" )
{
	SECTION( "Can merge commands for same entity, primitive, and property" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity();
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		const float metallic1 = 0.3f;
		const float metallic2 = 0.7f;

		editor::SetPrimitiveMaterialPropertyCommand command1(
			entity, 0, editor::MaterialPropertyType::MetallicFactor, metallic1, ecsScene, *testData.assetScene, nullptr );

		editor::SetPrimitiveMaterialPropertyCommand command2(
			entity, 0, editor::MaterialPropertyType::MetallicFactor, metallic2, ecsScene, *testData.assetScene, nullptr );

		REQUIRE( command1.canMergeWith( &command2 ) );

		auto command2Ptr = std::make_unique<editor::SetPrimitiveMaterialPropertyCommand>(
			entity, 0, editor::MaterialPropertyType::MetallicFactor, metallic2, ecsScene, *testData.assetScene, nullptr );

		REQUIRE( command1.mergeWith( std::move( command2Ptr ) ) );
	}

	SECTION( "Cannot merge commands for different entities" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity1 = ecsScene.createEntity();
		const auto entity2 = ecsScene.createEntity();

		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity1, meshRenderer );
		ecsScene.addComponent( entity2, meshRenderer );

		const float metallic = 0.5f;

		editor::SetPrimitiveMaterialPropertyCommand command1(
			entity1, 0, editor::MaterialPropertyType::MetallicFactor, metallic, ecsScene, *testData.assetScene, nullptr );

		editor::SetPrimitiveMaterialPropertyCommand command2(
			entity2, 0, editor::MaterialPropertyType::MetallicFactor, metallic, ecsScene, *testData.assetScene, nullptr );

		REQUIRE_FALSE( command1.canMergeWith( &command2 ) );
	}

	SECTION( "Cannot merge commands for different primitives" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		// Add second primitive to the mesh
		const auto mesh = testData.assetScene->getMesh( testData.meshHandle );
		assets::Primitive primitive2;
		primitive2.setMaterialHandle( testData.materialHandle );
		for ( int i = 0; i < 3; ++i )
		{
			assets::Vertex vertex;
			vertex.position = math::Vec3f{ static_cast<float>( i ), 1.0f, 0.0f };
			primitive2.addVertex( vertex );
			primitive2.addIndex( static_cast<std::uint32_t>( i ) );
		}
		mesh->addPrimitive( std::move( primitive2 ) );

		const auto entity = ecsScene.createEntity();
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		const float metallic = 0.5f;

		editor::SetPrimitiveMaterialPropertyCommand command1(
			entity, 0, editor::MaterialPropertyType::MetallicFactor, metallic, ecsScene, *testData.assetScene, nullptr );

		editor::SetPrimitiveMaterialPropertyCommand command2(
			entity, 1, editor::MaterialPropertyType::MetallicFactor, metallic, ecsScene, *testData.assetScene, nullptr );

		REQUIRE_FALSE( command1.canMergeWith( &command2 ) );
	}

	SECTION( "Cannot merge commands for different properties" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity();
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		const float metallic = 0.5f;
		const float roughness = 0.3f;

		editor::SetPrimitiveMaterialPropertyCommand command1(
			entity, 0, editor::MaterialPropertyType::MetallicFactor, metallic, ecsScene, *testData.assetScene, nullptr );

		editor::SetPrimitiveMaterialPropertyCommand command2(
			entity, 0, editor::MaterialPropertyType::RoughnessFactor, roughness, ecsScene, *testData.assetScene, nullptr );

		REQUIRE_FALSE( command1.canMergeWith( &command2 ) );
	}
}

TEST_CASE( "SetPrimitiveMaterialPropertyCommand - Command interface", "[primitive-material][unit]" )
{
	SECTION( "getDescription returns meaningful text" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity();
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		editor::SetPrimitiveMaterialPropertyCommand command(
			entity, 0, editor::MaterialPropertyType::MetallicFactor, 0.5f, ecsScene, *testData.assetScene, nullptr );

		const auto description = command.getDescription();
		REQUIRE_FALSE( description.empty() );
		REQUIRE( description.find( "Metallic" ) != std::string::npos );
		REQUIRE( description.find( "Primitive 0" ) != std::string::npos );
	}

	SECTION( "getMemoryUsage returns reasonable value" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity();
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		editor::SetPrimitiveMaterialPropertyCommand command(
			entity, 0, editor::MaterialPropertyType::MetallicFactor, 0.5f, ecsScene, *testData.assetScene, nullptr );

		const auto memoryUsage = command.getMemoryUsage();
		REQUIRE( memoryUsage > 0 );
		REQUIRE( memoryUsage >= sizeof( command ) );
	}

	SECTION( "updateEntityReference updates entity" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity();
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		editor::SetPrimitiveMaterialPropertyCommand command(
			entity, 0, editor::MaterialPropertyType::MetallicFactor, 0.5f, ecsScene, *testData.assetScene, nullptr );

		const ecs::Entity newEntity{ 999, 1 };
		REQUIRE( command.updateEntityReference( entity, newEntity ) );
		REQUIRE( command.getEntity().id == newEntity.id );
		REQUIRE( command.getEntity().generation == newEntity.generation );
	}
}

TEST_CASE( "ClearPrimitiveMaterialPropertyCommand", "[primitive-material][unit]" )
{
	SECTION( "Execute clears property override" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity();
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		// Set an initial override
		const auto mesh = testData.assetScene->getMesh( meshRenderer.meshHandle );
		auto &primitive = mesh->getPrimitive( 0 );
		auto materialInstance = primitive.getMaterialInstance();
		materialInstance.metallicFactorOverride = 0.8f;
		primitive.setMaterialInstance( materialInstance );

		// Create clear command
		editor::ClearPrimitiveMaterialPropertyCommand command(
			entity, 0, editor::MaterialPropertyType::MetallicFactor, ecsScene, *testData.assetScene, nullptr );

		REQUIRE( command.execute() );

		// Verify override is cleared
		const auto &clearedPrimitive = mesh->getPrimitive( 0 );
		const auto &clearedInstance = clearedPrimitive.getMaterialInstance();
		REQUIRE_FALSE( clearedInstance.metallicFactorOverride.has_value() );
	}

	SECTION( "Undo restores cleared override" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity();
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		// Set an initial override
		const auto mesh = testData.assetScene->getMesh( meshRenderer.meshHandle );
		auto &primitive = mesh->getPrimitive( 0 );
		auto materialInstance = primitive.getMaterialInstance();
		const float originalMetallic = 0.8f;
		materialInstance.metallicFactorOverride = originalMetallic;
		primitive.setMaterialInstance( materialInstance );

		// Create and execute clear command
		editor::ClearPrimitiveMaterialPropertyCommand command(
			entity, 0, editor::MaterialPropertyType::MetallicFactor, ecsScene, *testData.assetScene, nullptr );

		REQUIRE( command.execute() );
		REQUIRE( command.undo() );

		// Verify override is restored
		const auto &restoredPrimitive = mesh->getPrimitive( 0 );
		const auto &restoredInstance = restoredPrimitive.getMaterialInstance();
		REQUIRE( restoredInstance.metallicFactorOverride.has_value() );
		REQUIRE( restoredInstance.metallicFactorOverride.value() == originalMetallic );
	}

	SECTION( "getDescription returns meaningful text" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity();
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		editor::ClearPrimitiveMaterialPropertyCommand command(
			entity, 0, editor::MaterialPropertyType::RoughnessFactor, ecsScene, *testData.assetScene, nullptr );

		const auto description = command.getDescription();
		REQUIRE_FALSE( description.empty() );
		REQUIRE( description.find( "Clear" ) != std::string::npos );
		REQUIRE( description.find( "Roughness" ) != std::string::npos );
	}
}

TEST_CASE( "ClearAllPrimitiveMaterialOverridesCommand", "[primitive-material][unit]" )
{
	SECTION( "Execute clears all overrides" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity();
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		// Set multiple overrides
		const auto mesh = testData.assetScene->getMesh( meshRenderer.meshHandle );
		auto &primitive = mesh->getPrimitive( 0 );
		auto materialInstance = primitive.getMaterialInstance();
		materialInstance.baseColorFactorOverride = math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f };
		materialInstance.metallicFactorOverride = 0.8f;
		materialInstance.roughnessFactorOverride = 0.3f;
		materialInstance.emissiveFactorOverride = math::Vec3f{ 1.0f, 1.0f, 0.0f };
		primitive.setMaterialInstance( materialInstance );

		// Verify overrides exist
		REQUIRE( primitive.getMaterialInstance().hasOverrides() );

		// Create and execute clear all command
		editor::ClearAllPrimitiveMaterialOverridesCommand command(
			entity, 0, ecsScene, *testData.assetScene, nullptr );

		REQUIRE( command.execute() );

		// Verify all overrides are cleared
		const auto &clearedPrimitive = mesh->getPrimitive( 0 );
		const auto &clearedInstance = clearedPrimitive.getMaterialInstance();
		REQUIRE_FALSE( clearedInstance.hasOverrides() );
		REQUIRE_FALSE( clearedInstance.baseColorFactorOverride.has_value() );
		REQUIRE_FALSE( clearedInstance.metallicFactorOverride.has_value() );
		REQUIRE_FALSE( clearedInstance.roughnessFactorOverride.has_value() );
		REQUIRE_FALSE( clearedInstance.emissiveFactorOverride.has_value() );
	}

	SECTION( "Undo restores all cleared overrides" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity();
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		// Set multiple overrides
		const auto mesh = testData.assetScene->getMesh( meshRenderer.meshHandle );
		auto &primitive = mesh->getPrimitive( 0 );
		auto materialInstance = primitive.getMaterialInstance();
		const math::Vec4f originalColor{ 1.0f, 0.0f, 0.0f, 1.0f };
		const float originalMetallic = 0.8f;
		const float originalRoughness = 0.3f;
		const math::Vec3f originalEmissive{ 1.0f, 1.0f, 0.0f };

		materialInstance.baseColorFactorOverride = originalColor;
		materialInstance.metallicFactorOverride = originalMetallic;
		materialInstance.roughnessFactorOverride = originalRoughness;
		materialInstance.emissiveFactorOverride = originalEmissive;
		primitive.setMaterialInstance( materialInstance );

		// Create and execute clear all command
		editor::ClearAllPrimitiveMaterialOverridesCommand command(
			entity, 0, ecsScene, *testData.assetScene, nullptr );

		REQUIRE( command.execute() );
		REQUIRE_FALSE( mesh->getPrimitive( 0 ).getMaterialInstance().hasOverrides() );

		REQUIRE( command.undo() );

		// Verify all overrides are restored
		const auto &restoredPrimitive = mesh->getPrimitive( 0 );
		const auto &restoredInstance = restoredPrimitive.getMaterialInstance();
		REQUIRE( restoredInstance.hasOverrides() );
		REQUIRE( restoredInstance.baseColorFactorOverride.has_value() );
		REQUIRE( restoredInstance.baseColorFactorOverride.value() == originalColor );
		REQUIRE( restoredInstance.metallicFactorOverride.has_value() );
		REQUIRE( restoredInstance.metallicFactorOverride.value() == originalMetallic );
		REQUIRE( restoredInstance.roughnessFactorOverride.has_value() );
		REQUIRE( restoredInstance.roughnessFactorOverride.value() == originalRoughness );
		REQUIRE( restoredInstance.emissiveFactorOverride.has_value() );
		REQUIRE( restoredInstance.emissiveFactorOverride.value() == originalEmissive );
	}

	SECTION( "getDescription returns meaningful text" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity();
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		editor::ClearAllPrimitiveMaterialOverridesCommand command(
			entity, 0, ecsScene, *testData.assetScene, nullptr );

		const auto description = command.getDescription();
		REQUIRE_FALSE( description.empty() );
		REQUIRE( description.find( "Clear All" ) != std::string::npos );
		REQUIRE( description.find( "Primitive 0" ) != std::string::npos );
	}

	SECTION( "getMemoryUsage accounts for MaterialInstance size" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity();
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		editor::ClearAllPrimitiveMaterialOverridesCommand command(
			entity, 0, ecsScene, *testData.assetScene, nullptr );

		const auto memoryUsage = command.getMemoryUsage();
		REQUIRE( memoryUsage > 0 );
		REQUIRE( memoryUsage >= sizeof( command ) );
	}
}

TEST_CASE( "SetPrimitiveMaterialPropertyCommand - Error handling", "[primitive-material][unit]" )
{
	SECTION( "Returns false when entity has no MeshRenderer" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity();
		// No MeshRenderer component added

		editor::SetPrimitiveMaterialPropertyCommand command(
			entity, 0, editor::MaterialPropertyType::MetallicFactor, 0.5f, ecsScene, *testData.assetScene, nullptr );

		REQUIRE_FALSE( command.execute() );
	}

	SECTION( "Returns false when mesh handle is invalid" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity();
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = 999; // Invalid handle
		ecsScene.addComponent( entity, meshRenderer );

		editor::SetPrimitiveMaterialPropertyCommand command(
			entity, 0, editor::MaterialPropertyType::MetallicFactor, 0.5f, ecsScene, *testData.assetScene, nullptr );

		REQUIRE_FALSE( command.execute() );
	}

	SECTION( "Returns false when primitive index is out of range" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity();
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		editor::SetPrimitiveMaterialPropertyCommand command(
			entity, 999, // Invalid primitive index
			editor::MaterialPropertyType::MetallicFactor,
			0.5f,
			ecsScene,
			*testData.assetScene,
			nullptr );

		REQUIRE_FALSE( command.execute() );
	}
}

TEST_CASE( "CopyPrimitiveMaterialCommand basics", "[primitive-material][command][unit]" )
{
	SECTION( "Copy command returns false for invalid entity" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		ecs::Entity invalidEntity;
		invalidEntity.id = 9999;
		invalidEntity.generation = 0;

		editor::CopyPrimitiveMaterialCommand copyCmd( invalidEntity, 0, ecsScene, *testData.assetScene );
		REQUIRE_FALSE( copyCmd.execute() );
	}

	SECTION( "Copy command returns false for out-of-range primitive" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity( "TestEntity" );
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		editor::CopyPrimitiveMaterialCommand copyCmd( entity, 999, ecsScene, *testData.assetScene );
		REQUIRE_FALSE( copyCmd.execute() );
	}

	SECTION( "Copy command has meaningful description" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity( "TestEntity" );
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		editor::CopyPrimitiveMaterialCommand copyCmd( entity, 0, ecsScene, *testData.assetScene );
		const auto desc = copyCmd.getDescription();
		REQUIRE( desc.find( "Copy" ) != std::string::npos );
	}

	SECTION( "Copy command undo returns true (no-op)" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity( "TestEntity" );
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		editor::CopyPrimitiveMaterialCommand copyCmd( entity, 0, ecsScene, *testData.assetScene );
		REQUIRE( copyCmd.undo() ); // Copy's undo is a no-op and returns true
	}

	SECTION( "Copy command doesn't merge" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity( "TestEntity" );
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		editor::CopyPrimitiveMaterialCommand copyCmd1( entity, 0, ecsScene, *testData.assetScene );
		editor::CopyPrimitiveMaterialCommand copyCmd2( entity, 0, ecsScene, *testData.assetScene );

		REQUIRE_FALSE( copyCmd1.canMergeWith( &copyCmd2 ) );
		REQUIRE_FALSE( copyCmd1.mergeWith( std::make_unique<editor::CopyPrimitiveMaterialCommand>( entity, 0, ecsScene, *testData.assetScene ) ) );
	}

	SECTION( "Copy command updates entity reference" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity( "TestEntity" );
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		editor::CopyPrimitiveMaterialCommand copyCmd( entity, 0, ecsScene, *testData.assetScene );

		const auto newEntity = ecsScene.createEntity( "NewEntity" );
		components::MeshRenderer newMeshRenderer;
		newMeshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( newEntity, newMeshRenderer );

		REQUIRE( copyCmd.updateEntityReference( entity, newEntity ) );
	}
}

TEST_CASE( "PastePrimitiveMaterialCommand basics - No ImGui context", "[primitive-material][unit]" )
{
	// NOTE: PastePrimitiveMaterialCommand calls ImGui::GetClipboardText() in constructor
	// which requires ImGui context. Cannot unit test without it.
	// This test is marked as documentation that Paste tests require integration testing
	// or need refactoring to defer clipboard access to execute() instead of constructor.

	// For now, test the Copy command which doesn't have ImGui in constructor
	SECTION( "Copy command can be instantiated without crash" )
	{
		ecs::Scene ecsScene;
		auto testData = createTestAssetScene();

		const auto entity = ecsScene.createEntity( "TestEntity" );
		components::MeshRenderer meshRenderer;
		meshRenderer.meshHandle = testData.meshHandle;
		ecsScene.addComponent( entity, meshRenderer );

		// This should not crash - Copy constructor has no ImGui calls
		editor::CopyPrimitiveMaterialCommand copyCmd( entity, 0, ecsScene, *testData.assetScene );
		REQUIRE( copyCmd.getDescription().find( "Copy" ) != std::string::npos );
	}
}
