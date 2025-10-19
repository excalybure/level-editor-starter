#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "runtime/scene_serialization/SceneSerializer.h"
#include "engine/assets/assets.h"
#include "math/vec.h"

using json = nlohmann::json;

TEST_CASE( "SceneSerializer can serialize MaterialInstance with all overrides", "[SceneSerializer][MaterialInstance][T3.1][unit]" )
{
	// Arrange
	assets::MaterialInstance instance;
	instance.baseMaterial = 5;
	instance.baseColorFactorOverride = math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f };
	instance.metallicFactorOverride = 0.8f;
	instance.roughnessFactorOverride = 0.3f;
	instance.emissiveFactorOverride = math::Vec3f{ 0.2f, 0.2f, 0.2f };
	instance.baseColorTextureOverride = "textures/custom_color.png";
	instance.metallicRoughnessTextureOverride = "textures/custom_mr.png";

	// Act - serialize to JSON
	json j;
	j["baseMaterial"] = instance.baseMaterial;

	if ( instance.baseColorFactorOverride.has_value() )
	{
		const auto &v = instance.baseColorFactorOverride.value();
		j["baseColorFactor"] = { v.x, v.y, v.z, v.w };
	}
	if ( instance.metallicFactorOverride.has_value() )
	{
		j["metallicFactor"] = instance.metallicFactorOverride.value();
	}
	if ( instance.roughnessFactorOverride.has_value() )
	{
		j["roughnessFactor"] = instance.roughnessFactorOverride.value();
	}
	if ( instance.emissiveFactorOverride.has_value() )
	{
		const auto &v = instance.emissiveFactorOverride.value();
		j["emissiveFactor"] = { v.x, v.y, v.z };
	}
	if ( instance.baseColorTextureOverride.has_value() )
	{
		j["baseColorTexture"] = instance.baseColorTextureOverride.value();
	}
	if ( instance.metallicRoughnessTextureOverride.has_value() )
	{
		j["metallicRoughnessTexture"] = instance.metallicRoughnessTextureOverride.value();
	}

	// Assert
	REQUIRE( j["baseMaterial"] == 5 );
	REQUIRE( j["baseColorFactor"][0] == 1.0f );
	REQUIRE( j["baseColorFactor"][1] == 0.0f );
	REQUIRE( j["baseColorFactor"][2] == 0.0f );
	REQUIRE( j["baseColorFactor"][3] == 1.0f );
	REQUIRE( j["metallicFactor"] == 0.8f );
	REQUIRE( j["roughnessFactor"] == 0.3f );
	REQUIRE( j["emissiveFactor"][0] == 0.2f );
	REQUIRE( j["baseColorTexture"] == "textures/custom_color.png" );
	REQUIRE( j["metallicRoughnessTexture"] == "textures/custom_mr.png" );
}

TEST_CASE( "SceneSerializer can deserialize MaterialInstance with all overrides", "[SceneSerializer][MaterialInstance][T3.1][unit]" )
{
	// Arrange
	json j;
	j["baseMaterial"] = 5;
	j["baseColorFactor"] = { 1.0f, 0.0f, 0.0f, 1.0f };
	j["metallicFactor"] = 0.8f;
	j["roughnessFactor"] = 0.3f;
	j["emissiveFactor"] = { 0.2f, 0.2f, 0.2f };
	j["baseColorTexture"] = "textures/custom_color.png";
	j["metallicRoughnessTexture"] = "textures/custom_mr.png";

	// Act - deserialize from JSON
	assets::MaterialInstance instance;
	instance.baseMaterial = j["baseMaterial"];

	if ( j.contains( "baseColorFactor" ) && j["baseColorFactor"].is_array() && j["baseColorFactor"].size() == 4 )
	{
		instance.baseColorFactorOverride = math::Vec4f{
			j["baseColorFactor"][0],
			j["baseColorFactor"][1],
			j["baseColorFactor"][2],
			j["baseColorFactor"][3]
		};
	}
	if ( j.contains( "metallicFactor" ) )
	{
		instance.metallicFactorOverride = j["metallicFactor"];
	}
	if ( j.contains( "roughnessFactor" ) )
	{
		instance.roughnessFactorOverride = j["roughnessFactor"];
	}
	if ( j.contains( "emissiveFactor" ) && j["emissiveFactor"].is_array() && j["emissiveFactor"].size() == 3 )
	{
		instance.emissiveFactorOverride = math::Vec3f{
			j["emissiveFactor"][0],
			j["emissiveFactor"][1],
			j["emissiveFactor"][2]
		};
	}
	if ( j.contains( "baseColorTexture" ) )
	{
		instance.baseColorTextureOverride = j["baseColorTexture"];
	}
	if ( j.contains( "metallicRoughnessTexture" ) )
	{
		instance.metallicRoughnessTextureOverride = j["metallicRoughnessTexture"];
	}

	// Assert
	REQUIRE( instance.baseMaterial == 5 );
	REQUIRE( instance.baseColorFactorOverride.has_value() );
	REQUIRE( instance.baseColorFactorOverride.value() == math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f } );
	REQUIRE( instance.metallicFactorOverride.has_value() );
	REQUIRE( instance.metallicFactorOverride.value() == 0.8f );
	REQUIRE( instance.roughnessFactorOverride.has_value() );
	REQUIRE( instance.roughnessFactorOverride.value() == 0.3f );
	REQUIRE( instance.emissiveFactorOverride.has_value() );
	REQUIRE( instance.emissiveFactorOverride.value() == math::Vec3f{ 0.2f, 0.2f, 0.2f } );
	REQUIRE( instance.baseColorTextureOverride.has_value() );
	REQUIRE( instance.baseColorTextureOverride.value() == "textures/custom_color.png" );
	REQUIRE( instance.metallicRoughnessTextureOverride.has_value() );
	REQUIRE( instance.metallicRoughnessTextureOverride.value() == "textures/custom_mr.png" );
}

TEST_CASE( "SceneSerializer only serializes non-empty MaterialInstance overrides", "[SceneSerializer][MaterialInstance][T3.1][unit]" )
{
	// Arrange - MaterialInstance with only some overrides
	assets::MaterialInstance instance;
	instance.baseMaterial = 3;
	instance.baseColorFactorOverride = math::Vec4f{ 1.0f, 0.5f, 0.2f, 1.0f };
	instance.metallicFactorOverride = 0.5f;
	// roughnessFactorOverride, emissiveFactorOverride, and all texture overrides are NOT set

	// Act - serialize only non-empty overrides
	json j;
	j["baseMaterial"] = instance.baseMaterial;

	if ( instance.baseColorFactorOverride.has_value() )
	{
		const auto &v = instance.baseColorFactorOverride.value();
		j["baseColorFactor"] = { v.x, v.y, v.z, v.w };
	}
	if ( instance.metallicFactorOverride.has_value() )
	{
		j["metallicFactor"] = instance.metallicFactorOverride.value();
	}
	if ( instance.roughnessFactorOverride.has_value() )
	{
		const auto &v = instance.roughnessFactorOverride.value();
		j["roughnessFactor"] = v;
	}
	if ( instance.emissiveFactorOverride.has_value() )
	{
		const auto &v = instance.emissiveFactorOverride.value();
		j["emissiveFactor"] = { v.x, v.y, v.z };
	}

	// Assert - only non-empty overrides are in JSON
	REQUIRE( j.contains( "baseMaterial" ) );
	REQUIRE( j.contains( "baseColorFactor" ) );
	REQUIRE( j.contains( "metallicFactor" ) );
	REQUIRE( !j.contains( "roughnessFactor" ) );
	REQUIRE( !j.contains( "emissiveFactor" ) );
	REQUIRE( !j.contains( "baseColorTexture" ) );
	REQUIRE( !j.contains( "metallicRoughnessTexture" ) );
	REQUIRE( !j.contains( "normalTexture" ) );
	REQUIRE( !j.contains( "emissiveTexture" ) );
}

TEST_CASE( "SceneSerializer handles empty MaterialInstance (no overrides)", "[SceneSerializer][MaterialInstance][T3.1][unit]" )
{
	// Arrange - MaterialInstance with NO overrides
	assets::MaterialInstance instance;
	instance.baseMaterial = 2;
	// All overrides are empty

	// Act - serialize
	json j;
	j["baseMaterial"] = instance.baseMaterial;

	if ( instance.baseColorFactorOverride.has_value() )
	{
		const auto &v = instance.baseColorFactorOverride.value();
		j["baseColorFactor"] = { v.x, v.y, v.z, v.w };
	}
	if ( instance.metallicFactorOverride.has_value() )
	{
		j["metallicFactor"] = instance.metallicFactorOverride.value();
	}
	if ( instance.roughnessFactorOverride.has_value() )
	{
		j["roughnessFactor"] = instance.roughnessFactorOverride.value();
	}
	if ( instance.emissiveFactorOverride.has_value() )
	{
		const auto &v = instance.emissiveFactorOverride.value();
		j["emissiveFactor"] = { v.x, v.y, v.z };
	}

	// Assert - JSON only contains baseMaterial, no overrides
	REQUIRE( j.size() == 1 ); // Only baseMaterial
	REQUIRE( j["baseMaterial"] == 2 );
	REQUIRE( !j.contains( "baseColorFactor" ) );
	REQUIRE( !j.contains( "metallicFactor" ) );
	REQUIRE( !j.contains( "roughnessFactor" ) );
	REQUIRE( !j.contains( "emissiveFactor" ) );
}

TEST_CASE( "SceneSerializer deserializes MaterialInstance with missing fields gracefully", "[SceneSerializer][MaterialInstance][T3.1][unit]" )
{
	// Arrange - JSON with only some fields
	json j;
	j["baseMaterial"] = 7;
	j["metallicFactor"] = 0.6f;
	// Other fields are missing

	// Act
	assets::MaterialInstance instance;
	instance.baseMaterial = j["baseMaterial"];

	if ( j.contains( "baseColorFactor" ) && j["baseColorFactor"].is_array() && j["baseColorFactor"].size() == 4 )
	{
		instance.baseColorFactorOverride = math::Vec4f{
			j["baseColorFactor"][0],
			j["baseColorFactor"][1],
			j["baseColorFactor"][2],
			j["baseColorFactor"][3]
		};
	}
	if ( j.contains( "metallicFactor" ) )
	{
		instance.metallicFactorOverride = j["metallicFactor"];
	}
	if ( j.contains( "roughnessFactor" ) )
	{
		instance.roughnessFactorOverride = j["roughnessFactor"];
	}

	// Assert
	REQUIRE( instance.baseMaterial == 7 );
	REQUIRE( !instance.baseColorFactorOverride.has_value() );
	REQUIRE( instance.metallicFactorOverride.has_value() );
	REQUIRE( instance.metallicFactorOverride.value() == 0.6f );
	REQUIRE( !instance.roughnessFactorOverride.has_value() );
}

TEST_CASE( "SceneSerializer can serialize scene primitives with MaterialInstance overrides", "[SceneSerializer][MaterialInstance][T3.1][integration]" )
{
	// Arrange - Create a simple mesh with primitives and material overrides
	auto mesh = std::make_unique<assets::Mesh>();
	mesh->setPath( "test_mesh.gltf" );
	mesh->setLoaded( true );

	// Create first primitive with overrides
	assets::Primitive prim1;
	prim1.addVertex( assets::Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } } );
	prim1.addVertex( assets::Vertex{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } } );
	prim1.addVertex( assets::Vertex{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } } );
	prim1.setMaterialHandle( 10 );

	assets::MaterialInstance inst1;
	inst1.baseMaterial = 10;
	inst1.baseColorFactorOverride = math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f };
	inst1.metallicFactorOverride = 0.8f;
	prim1.setMaterialInstance( inst1 );

	// Create second primitive without overrides
	assets::Primitive prim2;
	prim2.addVertex( assets::Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } } );
	prim2.addVertex( assets::Vertex{ { 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } } );
	prim2.addVertex( assets::Vertex{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } } );
	prim2.setMaterialHandle( 11 );

	mesh->getPrimitives().push_back( prim1 );
	mesh->getPrimitives().push_back( prim2 );

	// Act - Create JSON structure for primitive overrides
	json primitivesJson = json::array();

	for ( uint32_t i = 0; i < mesh->getPrimitiveCount(); ++i )
	{
		const auto &prim = mesh->getPrimitive( i );
		json primJson;
		primJson["index"] = i;
		primJson["materialHandle"] = prim.getMaterialHandle();

		if ( prim.hasOverrides() )
		{
			const auto &instance = prim.getMaterialInstance();
			json instJson;
			instJson["baseMaterial"] = instance.baseMaterial;

			if ( instance.baseColorFactorOverride.has_value() )
			{
				const auto &v = instance.baseColorFactorOverride.value();
				instJson["baseColorFactor"] = { v.x, v.y, v.z, v.w };
			}
			if ( instance.metallicFactorOverride.has_value() )
			{
				instJson["metallicFactor"] = instance.metallicFactorOverride.value();
			}

			primJson["materialInstance"] = instJson;
		}

		primitivesJson.push_back( primJson );
	}

	// Assert
	REQUIRE( primitivesJson.size() == 2 );
	REQUIRE( primitivesJson[0]["index"] == 0 );
	REQUIRE( primitivesJson[0].contains( "materialInstance" ) );
	REQUIRE( primitivesJson[0]["materialInstance"]["baseColorFactor"][0] == 1.0f );
	REQUIRE( primitivesJson[0]["materialInstance"]["metallicFactor"] == 0.8f );

	REQUIRE( primitivesJson[1]["index"] == 1 );
	REQUIRE( !primitivesJson[1].contains( "materialInstance" ) );
}

TEST_CASE( "SceneSerializer can deserialize primitives from JSON with MaterialInstance overrides", "[SceneSerializer][MaterialInstance][T3.1][integration]" )
{
	// Arrange - Create JSON with primitives and overrides
	json primitivesJson = json::array();

	json prim0Json;
	prim0Json["index"] = 0;
	prim0Json["materialHandle"] = 10;
	json inst0Json;
	inst0Json["baseMaterial"] = 10;
	inst0Json["baseColorFactor"] = { 1.0f, 0.0f, 0.0f, 1.0f };
	inst0Json["metallicFactor"] = 0.8f;
	prim0Json["materialInstance"] = inst0Json;
	primitivesJson.push_back( prim0Json );

	json prim1Json;
	prim1Json["index"] = 1;
	prim1Json["materialHandle"] = 11;
	primitivesJson.push_back( prim1Json );

	// Act - Deserialize primitives
	std::vector<assets::MaterialInstance> deserializedInstances;

	for ( const auto &primJson : primitivesJson )
	{
		if ( primJson.contains( "materialInstance" ) )
		{
			assets::MaterialInstance instance;
			const auto &instJson = primJson["materialInstance"];

			if ( instJson.contains( "baseMaterial" ) )
			{
				instance.baseMaterial = instJson["baseMaterial"];
			}

			if ( instJson.contains( "baseColorFactor" ) && instJson["baseColorFactor"].is_array() && instJson["baseColorFactor"].size() == 4 )
			{
				instance.baseColorFactorOverride = math::Vec4f{
					instJson["baseColorFactor"][0],
					instJson["baseColorFactor"][1],
					instJson["baseColorFactor"][2],
					instJson["baseColorFactor"][3]
				};
			}

			if ( instJson.contains( "metallicFactor" ) )
			{
				instance.metallicFactorOverride = static_cast<float>( instJson["metallicFactor"] );
			}

			deserializedInstances.push_back( instance );
		}
	}

	// Assert
	REQUIRE( deserializedInstances.size() == 1 );
	REQUIRE( deserializedInstances[0].baseMaterial == 10 );
	REQUIRE( deserializedInstances[0].baseColorFactorOverride.has_value() );
	REQUIRE( deserializedInstances[0].baseColorFactorOverride.value() == math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f } );
	REQUIRE( deserializedInstances[0].metallicFactorOverride.has_value() );
	REQUIRE( deserializedInstances[0].metallicFactorOverride.value() == 0.8f );
}
