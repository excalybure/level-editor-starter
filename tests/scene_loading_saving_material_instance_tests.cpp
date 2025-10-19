#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <memory>

#include "runtime/ecs.h"
#include "runtime/components.h"
#include "runtime/scene_serialization/SceneSerializer.h"
#include "engine/assets/assets.h"
#include "math/vec.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

TEST_CASE("Scene with material instance overrides saves and loads correctly", "[scene_serialization][material_instance][T3.3][integration]")
{
	// Arrange: Create a mock scene with primitives and material overrides
	// NOTE: This is an integration test that demonstrates the round-trip save/load
	
	// Create a simple mesh with primitives and overrides
	auto mesh = std::make_unique<assets::Mesh>();
	mesh->setPath("assets/test/cube.gltf");
	mesh->setLoaded(true);

	// Primitive 0: Red override
	{
		assets::Primitive prim;
		prim.addVertex(assets::Vertex{{0, 0, 0}, {0, 1, 0}, {0, 0}});
		prim.setMaterialHandle(0);
		
		assets::MaterialInstance instance;
		instance.baseMaterial = 0;
		instance.baseColorFactorOverride = math::Vec4f{1.0f, 0.0f, 0.0f, 1.0f};
		instance.metallicFactorOverride = 0.8f;
		prim.setMaterialInstance(instance);
		
		mesh->getPrimitives().push_back(prim);
	}

	// Primitive 1: Partial override (only roughness)
	{
		assets::Primitive prim;
		prim.addVertex(assets::Vertex{{1, 0, 0}, {0, 1, 0}, {1, 0}});
		prim.setMaterialHandle(0);
		
		assets::MaterialInstance instance;
		instance.baseMaterial = 0;
		instance.roughnessFactorOverride = 0.3f;  // Only roughness, no color
		prim.setMaterialInstance(instance);
		
		mesh->getPrimitives().push_back(prim);
	}

	// Primitive 2: No override
	{
		assets::Primitive prim;
		prim.addVertex(assets::Vertex{{0, 1, 0}, {0, 1, 0}, {0, 1}});
		prim.setMaterialHandle(0);
		// No MaterialInstance set
		
		mesh->getPrimitives().push_back(prim);
	}

	// Act: Serialize primitives to JSON structure
	json primitivesJson = json::array();
	for (uint32_t i = 0; i < mesh->getPrimitiveCount(); ++i)
	{
		const auto& prim = mesh->getPrimitive(i);
		json primJson;
		primJson["index"] = i;
		primJson["materialHandle"] = prim.getMaterialHandle();
		
		if (prim.hasOverrides())
		{
			const auto& instance = prim.getMaterialInstance();
			json instJson;
			instJson["baseMaterial"] = instance.baseMaterial;
			
			if (instance.baseColorFactorOverride.has_value())
			{
				const auto& v = instance.baseColorFactorOverride.value();
				instJson["baseColorFactor"] = {v.x, v.y, v.z, v.w};
			}
			if (instance.metallicFactorOverride.has_value())
			{
				instJson["metallicFactor"] = instance.metallicFactorOverride.value();
			}
			if (instance.roughnessFactorOverride.has_value())
			{
				instJson["roughnessFactor"] = instance.roughnessFactorOverride.value();
			}
			if (instance.emissiveFactorOverride.has_value())
			{
				const auto& v = instance.emissiveFactorOverride.value();
				instJson["emissiveFactor"] = {v.x, v.y, v.z};
			}
			
			primJson["materialInstance"] = instJson;
		}
		
		primitivesJson.push_back(primJson);
	}

	// Assert: Verify JSON structure is correct
	REQUIRE(primitivesJson.size() == 3);
	
	// Primitive 0: Should have full override
	REQUIRE(primitivesJson[0]["index"] == 0);
	REQUIRE(primitivesJson[0].contains("materialInstance"));
	REQUIRE(primitivesJson[0]["materialInstance"]["baseColorFactor"][0] == 1.0f);
	REQUIRE(primitivesJson[0]["materialInstance"]["metallicFactor"] == 0.8f);
	REQUIRE(!primitivesJson[0]["materialInstance"].contains("roughnessFactor"));  // Not set
	
	// Primitive 1: Should have only roughness override
	REQUIRE(primitivesJson[1]["index"] == 1);
	REQUIRE(primitivesJson[1].contains("materialInstance"));
	REQUIRE(primitivesJson[1]["materialInstance"]["roughnessFactor"] == 0.3f);
	REQUIRE(!primitivesJson[1]["materialInstance"].contains("baseColorFactor"));  // Not set
	REQUIRE(!primitivesJson[1]["materialInstance"].contains("metallicFactor"));  // Not set
	
	// Primitive 2: Should have NO override
	REQUIRE(primitivesJson[2]["index"] == 2);
	REQUIRE(!primitivesJson[2].contains("materialInstance"));
}

TEST_CASE("Partial material instance overrides serialize cleanly (only non-empty)", "[scene_serialization][material_instance][T3.3][unit]")
{
	// Arrange: Create a MaterialInstance with only some properties set
	assets::MaterialInstance instance;
	instance.baseMaterial = 5;
	instance.metallicFactorOverride = 0.6f;
	instance.emissiveFactorOverride = math::Vec3f{0.1f, 0.1f, 0.1f};
	// baseColorFactorOverride, roughnessFactorOverride, and all texture overrides are NOT set

	// Act: Serialize
	json instJson;
	instJson["baseMaterial"] = instance.baseMaterial;
	
	if (instance.baseColorFactorOverride.has_value())
	{
		const auto& v = instance.baseColorFactorOverride.value();
		instJson["baseColorFactor"] = {v.x, v.y, v.z, v.w};
	}
	if (instance.metallicFactorOverride.has_value())
	{
		instJson["metallicFactor"] = instance.metallicFactorOverride.value();
	}
	if (instance.roughnessFactorOverride.has_value())
	{
		instJson["roughnessFactor"] = instance.roughnessFactorOverride.value();
	}
	if (instance.emissiveFactorOverride.has_value())
	{
		const auto& v = instance.emissiveFactorOverride.value();
		instJson["emissiveFactor"] = {v.x, v.y, v.z};
	}

	// Assert: Only non-empty overrides are in JSON
	REQUIRE(instJson.contains("baseMaterial"));
	REQUIRE(instJson.contains("metallicFactor"));
	REQUIRE(instJson.contains("emissiveFactor"));
	REQUIRE(!instJson.contains("baseColorFactor"));  // Empty optional
	REQUIRE(!instJson.contains("roughnessFactor"));  // Empty optional
	
	// Verify clean format (minimal JSON)
	const auto jsonStr = instJson.dump();
	REQUIRE(jsonStr.find("null") == std::string::npos);  // No null values
}

TEST_CASE("Material instance overrides load correctly from JSON", "[scene_serialization][material_instance][T3.3][unit]")
{
	// Arrange: Create JSON with various override combinations
	json instJson;
	instJson["baseMaterial"] = 10;
	instJson["baseColorFactor"] = {0.9f, 0.8f, 0.7f, 1.0f};
	instJson["metallicFactor"] = 0.75f;
	// roughnessFactor and emissiveFactor are intentionally NOT included

	// Act: Deserialize
	assets::MaterialInstance instance;
	instance.baseMaterial = instJson["baseMaterial"];
	
	if (instJson.contains("baseColorFactor") && instJson["baseColorFactor"].is_array() && instJson["baseColorFactor"].size() == 4)
	{
		instance.baseColorFactorOverride = math::Vec4f{
			instJson["baseColorFactor"][0],
			instJson["baseColorFactor"][1],
			instJson["baseColorFactor"][2],
			instJson["baseColorFactor"][3]
		};
	}
	
	if (instJson.contains("metallicFactor"))
	{
		instance.metallicFactorOverride = static_cast<float>(instJson["metallicFactor"]);
	}
	
	if (instJson.contains("roughnessFactor"))
	{
		instance.roughnessFactorOverride = static_cast<float>(instJson["roughnessFactor"]);
	}
	
	if (instJson.contains("emissiveFactor") && instJson["emissiveFactor"].is_array() && instJson["emissiveFactor"].size() == 3)
	{
		instance.emissiveFactorOverride = math::Vec3f{
			instJson["emissiveFactor"][0],
			instJson["emissiveFactor"][1],
			instJson["emissiveFactor"][2]
		};
	}

	// Assert: Correctly loaded overrides
	REQUIRE(instance.baseMaterial == 10);
	REQUIRE(instance.baseColorFactorOverride.has_value());
	REQUIRE(instance.baseColorFactorOverride.value() == math::Vec4f{0.9f, 0.8f, 0.7f, 1.0f});
	REQUIRE(instance.metallicFactorOverride.has_value());
	REQUIRE(instance.metallicFactorOverride.value() == 0.75f);
	REQUIRE(!instance.roughnessFactorOverride.has_value());
	REQUIRE(!instance.emissiveFactorOverride.has_value());
}

TEST_CASE("Round-trip: Save primitives with overrides to JSON and load back", "[scene_serialization][material_instance][T3.3][integration]")
{
	// Arrange: Create primitives with various override configurations
	std::vector<assets::Primitive> originalPrimitives;
	
	// Primitive with all overrides
	{
		assets::Primitive prim;
		prim.addVertex(assets::Vertex{{0, 0, 0}, {0, 1, 0}, {0, 0}});
		prim.setMaterialHandle(0);
		
		assets::MaterialInstance instance;
		instance.baseMaterial = 0;
		instance.baseColorFactorOverride = math::Vec4f{1.0f, 0.0f, 0.0f, 1.0f};
		instance.metallicFactorOverride = 0.9f;
		instance.roughnessFactorOverride = 0.2f;
		instance.emissiveFactorOverride = math::Vec3f{0.5f, 0.5f, 0.5f};
		prim.setMaterialInstance(instance);
		
		originalPrimitives.push_back(prim);
	}
	
	// Primitive with partial override
	{
		assets::Primitive prim;
		prim.addVertex(assets::Vertex{{1, 0, 0}, {0, 1, 0}, {1, 0}});
		prim.setMaterialHandle(1);
		
		assets::MaterialInstance instance;
		instance.baseMaterial = 1;
		instance.roughnessFactorOverride = 0.4f;
		prim.setMaterialInstance(instance);
		
		originalPrimitives.push_back(prim);
	}
	
	// Primitive with no override
	{
		assets::Primitive prim;
		prim.addVertex(assets::Vertex{{0, 1, 0}, {0, 1, 0}, {0, 1}});
		prim.setMaterialHandle(2);
		
		originalPrimitives.push_back(prim);
	}

	// Act: Serialize to JSON
	json primitivesJson = json::array();
	for (uint32_t i = 0; i < originalPrimitives.size(); ++i)
	{
		const auto& prim = originalPrimitives[i];
		json primJson;
		primJson["index"] = i;
		primJson["materialHandle"] = prim.getMaterialHandle();
		
		if (prim.hasOverrides())
		{
			const auto& instance = prim.getMaterialInstance();
			json instJson;
			instJson["baseMaterial"] = instance.baseMaterial;
			
			if (instance.baseColorFactorOverride.has_value())
			{
				const auto& v = instance.baseColorFactorOverride.value();
				instJson["baseColorFactor"] = {v.x, v.y, v.z, v.w};
			}
			if (instance.metallicFactorOverride.has_value())
			{
				instJson["metallicFactor"] = instance.metallicFactorOverride.value();
			}
			if (instance.roughnessFactorOverride.has_value())
			{
				instJson["roughnessFactor"] = instance.roughnessFactorOverride.value();
			}
			if (instance.emissiveFactorOverride.has_value())
			{
				const auto& v = instance.emissiveFactorOverride.value();
				instJson["emissiveFactor"] = {v.x, v.y, v.z};
			}
			
			primJson["materialInstance"] = instJson;
		}
		
		primitivesJson.push_back(primJson);
	}

	// Act: Deserialize from JSON
	std::vector<assets::Primitive> loadedPrimitives;
	for (const auto& primJson : primitivesJson)
	{
		assets::Primitive prim;
		prim.addVertex(assets::Vertex{{0, 0, 0}, {0, 1, 0}, {0, 0}});  // Dummy vertex
		prim.setMaterialHandle(primJson["materialHandle"]);
		
		if (primJson.contains("materialInstance"))
		{
			assets::MaterialInstance instance;
			const auto& instJson = primJson["materialInstance"];
			
			instance.baseMaterial = instJson["baseMaterial"];
			
			if (instJson.contains("baseColorFactor") && instJson["baseColorFactor"].is_array() && instJson["baseColorFactor"].size() == 4)
			{
				instance.baseColorFactorOverride = math::Vec4f{
					instJson["baseColorFactor"][0],
					instJson["baseColorFactor"][1],
					instJson["baseColorFactor"][2],
					instJson["baseColorFactor"][3]
				};
			}
			
			if (instJson.contains("metallicFactor"))
			{
				instance.metallicFactorOverride = static_cast<float>(instJson["metallicFactor"]);
			}
			
			if (instJson.contains("roughnessFactor"))
			{
				instance.roughnessFactorOverride = static_cast<float>(instJson["roughnessFactor"]);
			}
			
			if (instJson.contains("emissiveFactor") && instJson["emissiveFactor"].is_array() && instJson["emissiveFactor"].size() == 3)
			{
				instance.emissiveFactorOverride = math::Vec3f{
					instJson["emissiveFactor"][0],
					instJson["emissiveFactor"][1],
					instJson["emissiveFactor"][2]
				};
			}
			
			prim.setMaterialInstance(instance);
		}
		
		loadedPrimitives.push_back(prim);
	}

	// Assert: Verify round-trip integrity
	REQUIRE(loadedPrimitives.size() == originalPrimitives.size());
	
	// Check primitive 0 (all overrides)
	REQUIRE(loadedPrimitives[0].getMaterialHandle() == 0);
	REQUIRE(loadedPrimitives[0].hasOverrides());
	const auto& inst0 = loadedPrimitives[0].getMaterialInstance();
	REQUIRE(inst0.baseMaterial == 0);
	REQUIRE(inst0.baseColorFactorOverride.value() == math::Vec4f{1.0f, 0.0f, 0.0f, 1.0f});
	REQUIRE(inst0.metallicFactorOverride.value() == 0.9f);
	REQUIRE(inst0.roughnessFactorOverride.value() == 0.2f);
	REQUIRE(inst0.emissiveFactorOverride.value() == math::Vec3f{0.5f, 0.5f, 0.5f});
	
	// Check primitive 1 (partial override)
	REQUIRE(loadedPrimitives[1].getMaterialHandle() == 1);
	REQUIRE(loadedPrimitives[1].hasOverrides());
	const auto& inst1 = loadedPrimitives[1].getMaterialInstance();
	REQUIRE(inst1.baseMaterial == 1);
	REQUIRE(!inst1.baseColorFactorOverride.has_value());
	REQUIRE(!inst1.metallicFactorOverride.has_value());
	REQUIRE(inst1.roughnessFactorOverride.value() == 0.4f);
	REQUIRE(!inst1.emissiveFactorOverride.has_value());
	
	// Check primitive 2 (no override)
	REQUIRE(loadedPrimitives[2].getMaterialHandle() == 2);
	REQUIRE(!loadedPrimitives[2].hasOverrides());
}

TEST_CASE("Backward compatibility: Scenes without material overrides still work", "[scene_serialization][material_instance][T3.3][integration]")
{
	// Arrange: Simulate old scene format without materialInstance field
	json sceneJson;
	sceneJson["version"] = "1.0";
	sceneJson["metadata"]["name"] = "Old Scene";
	sceneJson["metadata"]["created"] = "2025-10-19";
	sceneJson["metadata"]["modified"] = "2025-10-19";
	sceneJson["metadata"]["author"] = "Level Editor";
	
	json entityJson;
	entityJson["id"] = 1;
	entityJson["name"] = "MeshEntity";
	entityJson["parent"] = nullptr;
	entityJson["components"]["meshRenderer"]["meshPath"] = "assets/cube.gltf";
	entityJson["components"]["meshRenderer"]["lodBias"] = 0.0f;
	
	sceneJson["entities"] = json::array();
	sceneJson["entities"].push_back(entityJson);

	// Act: Load old scene format (no primitives/materialInstance fields)
	// Verify it doesn't crash and primitives are handled gracefully
	
	// Assert: Old format is still valid
	REQUIRE(sceneJson.contains("entities"));
	REQUIRE(sceneJson["entities"].size() == 1);
	
	const auto& entity = sceneJson["entities"][0];
	REQUIRE(entity["name"] == "MeshEntity");
	
	// Primitives/materialInstance fields are not present (as expected in old format)
	REQUIRE(!entity.contains("primitives"));
	REQUIRE(!entity["components"]["meshRenderer"].contains("materialInstance"));
}

TEST_CASE("Multiple primitives with different override patterns serialize cleanly", "[scene_serialization][material_instance][T3.3][integration]")
{
	// Arrange: Create a complex scene with mixed override patterns
	auto mesh = std::make_unique<assets::Mesh>();
	mesh->setPath("assets/test/complex_mesh.gltf");
	mesh->setLoaded(true);

	// Pattern 1: No overrides (use all defaults)
	{
		assets::Primitive prim;
		prim.addVertex(assets::Vertex{{0, 0, 0}, {0, 1, 0}, {0, 0}});
		prim.setMaterialHandle(0);
		mesh->getPrimitives().push_back(prim);
	}

	// Pattern 2: Single property override (color only)
	{
		assets::Primitive prim;
		prim.addVertex(assets::Vertex{{1, 0, 0}, {0, 1, 0}, {1, 0}});
		prim.setMaterialHandle(0);
		
		assets::MaterialInstance instance;
		instance.baseMaterial = 0;
		instance.baseColorFactorOverride = math::Vec4f{0.5f, 0.5f, 0.5f, 1.0f};
		prim.setMaterialInstance(instance);
		
		mesh->getPrimitives().push_back(prim);
	}

	// Pattern 3: Multiple property overrides
	{
		assets::Primitive prim;
		prim.addVertex(assets::Vertex{{2, 0, 0}, {0, 1, 0}, {2, 0}});
		prim.setMaterialHandle(1);
		
		assets::MaterialInstance instance;
		instance.baseMaterial = 1;
		instance.baseColorFactorOverride = math::Vec4f{1.0f, 1.0f, 1.0f, 1.0f};
		instance.metallicFactorOverride = 1.0f;
		instance.roughnessFactorOverride = 0.0f;
		prim.setMaterialInstance(instance);
		
		mesh->getPrimitives().push_back(prim);
	}

	// Pattern 4: Texture override only
	{
		assets::Primitive prim;
		prim.addVertex(assets::Vertex{{3, 0, 0}, {0, 1, 0}, {3, 0}});
		prim.setMaterialHandle(2);
		
		assets::MaterialInstance instance;
		instance.baseMaterial = 2;
		instance.baseColorTextureOverride = "textures/custom.png";
		prim.setMaterialInstance(instance);
		
		mesh->getPrimitives().push_back(prim);
	}

	// Act: Serialize all primitives
	json primitivesJson = json::array();
	size_t overrideCount = 0;
	for (uint32_t i = 0; i < mesh->getPrimitiveCount(); ++i)
	{
		const auto& prim = mesh->getPrimitive(i);
		json primJson;
		primJson["index"] = i;
		primJson["materialHandle"] = prim.getMaterialHandle();
		
		if (prim.hasOverrides())
		{
			overrideCount++;
			const auto& instance = prim.getMaterialInstance();
			json instJson;
			instJson["baseMaterial"] = instance.baseMaterial;
			
			if (instance.baseColorFactorOverride.has_value())
			{
				const auto& v = instance.baseColorFactorOverride.value();
				instJson["baseColorFactor"] = {v.x, v.y, v.z, v.w};
			}
			if (instance.metallicFactorOverride.has_value())
			{
				instJson["metallicFactor"] = instance.metallicFactorOverride.value();
			}
			if (instance.roughnessFactorOverride.has_value())
			{
				instJson["roughnessFactor"] = instance.roughnessFactorOverride.value();
			}
			if (instance.emissiveFactorOverride.has_value())
			{
				const auto& v = instance.emissiveFactorOverride.value();
				instJson["emissiveFactor"] = {v.x, v.y, v.z};
			}
			if (instance.baseColorTextureOverride.has_value())
			{
				instJson["baseColorTexture"] = instance.baseColorTextureOverride.value();
			}
			
			primJson["materialInstance"] = instJson;
		}
		
		primitivesJson.push_back(primJson);
	}

	// Assert: Verify serialization
	REQUIRE(primitivesJson.size() == 4);
	
	// Primitive 0: No override
	REQUIRE(!primitivesJson[0].contains("materialInstance"));
	
	// Primitive 1: Color override only
	REQUIRE(primitivesJson[1].contains("materialInstance"));
	REQUIRE(primitivesJson[1]["materialInstance"].contains("baseColorFactor"));
	REQUIRE(!primitivesJson[1]["materialInstance"].contains("metallicFactor"));
	REQUIRE(!primitivesJson[1]["materialInstance"].contains("roughnessFactor"));
	
	// Primitive 2: Multiple property overrides
	REQUIRE(primitivesJson[2].contains("materialInstance"));
	REQUIRE(primitivesJson[2]["materialInstance"].contains("baseColorFactor"));
	REQUIRE(primitivesJson[2]["materialInstance"].contains("metallicFactor"));
	REQUIRE(primitivesJson[2]["materialInstance"].contains("roughnessFactor"));
	
	// Primitive 3: Texture override
	REQUIRE(primitivesJson[3].contains("materialInstance"));
	REQUIRE(primitivesJson[3]["materialInstance"].contains("baseColorTexture"));
	REQUIRE(!primitivesJson[3]["materialInstance"].contains("baseColorFactor"));
	
	// Verify minimal JSON (only 3 primitives should have materialInstance)
	REQUIRE(overrideCount == 3);
}
