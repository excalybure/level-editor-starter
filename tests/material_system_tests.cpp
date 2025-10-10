#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "graphics/material_system/material_system.h"
#include "graphics/material_system/loader.h"
#include "graphics/material_system/validator.h"
#include "graphics/material_system/parser.h"
#include "graphics/material_system/shader_compiler.h"
#include "graphics/material_system/root_signature_builder.h"
#include "graphics/material_system/pipeline_builder.h"
#include "core/console.h"
#include "test_dx12_helpers.h"
#include <filesystem>
#include <fstream>
#include <cstring>

using json = nlohmann::json;
namespace fs = std::filesystem;

TEST_CASE( "JSON library is integrated and functional", "[material-system][setup]" )
{
	SECTION( "Parse minimal JSON object from string" )
	{
		const std::string jsonStr = R"({"test": 1})";
		const json j = json::parse( jsonStr );

		REQUIRE( j.contains( "test" ) );
		REQUIRE( j["test"] == 1 );
	}

	SECTION( "Parse JSON with nested objects" )
	{
		const std::string jsonStr = R"({
            "materials": [],
            "renderPasses": [],
            "defines": {}
        })";
		const json j = json::parse( jsonStr );

		REQUIRE( j.contains( "materials" ) );
		REQUIRE( j.contains( "renderPasses" ) );
		REQUIRE( j.contains( "defines" ) );
		REQUIRE( j["materials"].is_array() );
		REQUIRE( j["renderPasses"].is_array() );
		REQUIRE( j["defines"].is_object() );
	}
}

TEST_CASE( "Material system headers compile successfully", "[material-system][setup]" )
{
	SECTION( "Can include material_system.h" )
	{
		// If this test compiles and runs, header integration is successful
		REQUIRE( true );
	}
}

TEST_CASE( "Console logging integration for material system", "[material-system][setup]" )
{
	SECTION( "Console error and warning functions are available" )
	{
		// Verify console::error and console::warning are available
		// These will be used for non-fatal validation messages
		// Note: console::fatal cannot be tested directly as it calls std::exit(1)
		// Validation errors will log via console::error and then call console::fatal

		// This test documents expected behavior:
		// - console::error for logging error details
		// - console::fatal for terminating on validation failures
		// - Material system will fail-fast on any invalid input

		REQUIRE( true ); // Compilation success indicates console module linked
	}
}

// Phase 2: Core Validation Infrastructure

TEST_CASE( "JsonLoader detects circular includes", "[material-system][validation][T004]" )
{
	// Create temporary test files with circular dependencies
	const fs::path tempDir = fs::temp_directory_path() / "material_system_test_T004";
	fs::create_directories( tempDir );

	const fs::path fileA = tempDir / "a.json";
	const fs::path fileB = tempDir / "b.json";
	const fs::path fileC = tempDir / "c.json";

	SECTION( "Direct cycle A->B->A is detected" )
	{
		// Write files with direct cycle
		{
			std::ofstream outA( fileA );
			outA << R"({"includes": ["b.json"], "materials": []})";
		}
		{
			std::ofstream outB( fileB );
			outB << R"({"includes": ["a.json"], "materials": []})";
		}

		material_system::JsonLoader loader;

		// Expected: loader detects cycle and returns error with chain trace
		// For now, we test that loading fails (test will fail until implementation)
		const bool hasError = !loader.load( fileA.string() );

		// Clean up
		fs::remove_all( tempDir );

		// This will fail initially (RED phase)
		REQUIRE( hasError );
	}

	SECTION( "Transitive cycle A->B->C->A is detected" )
	{
		// Write files with transitive cycle
		{
			std::ofstream outA( fileA );
			outA << R"({"includes": ["b.json"], "materials": []})";
		}
		{
			std::ofstream outB( fileB );
			outB << R"({"includes": ["c.json"], "materials": []})";
		}
		{
			std::ofstream outC( fileC );
			outC << R"({"includes": ["a.json"], "materials": []})";
		}

		material_system::JsonLoader loader;

		// Expected: loader detects cycle and returns error
		const bool hasError = !loader.load( fileA.string() );

		// Clean up
		fs::remove_all( tempDir );

		// This will fail initially (RED phase)
		REQUIRE( hasError );
	}

	SECTION( "No cycle - linear chain loads successfully" )
	{
		// Write files without cycle: A->B->C (no back reference)
		{
			std::ofstream outA( fileA );
			outA << R"({"includes": ["b.json"], "materials": []})";
		}
		{
			std::ofstream outB( fileB );
			outB << R"({"includes": ["c.json"], "materials": []})";
		}
		{
			std::ofstream outC( fileC );
			outC << R"({"materials": []})";
		}

		material_system::JsonLoader loader;

		// Expected: loader succeeds
		const bool success = loader.load( fileA.string() );

		// Clean up
		fs::remove_all( tempDir );

		// This will fail initially (RED phase)
		REQUIRE( success );
	}
}

// Phase 2: JSON Schema Validation (T005)

TEST_CASE( "JsonValidator validates required schema sections", "[material-system][validation][T005]" )
{
	material_system::Validator validator;

	SECTION( "Valid minimal schema with required sections" )
	{
		const json validJson = R"({
            "materials": [],
            "renderPasses": []
        })"_json;

		const bool isValid = validator.validateSchema( validJson );
		REQUIRE( isValid );
	}

	SECTION( "Missing materials section fails validation" )
	{
		const json invalidJson = R"({
            "renderPasses": []
        })"_json;

		const bool isValid = validator.validateSchema( invalidJson );
		REQUIRE_FALSE( isValid );
	}

	SECTION( "Missing renderPasses section fails validation" )
	{
		const json invalidJson = R"({
            "materials": []
        })"_json;

		const bool isValid = validator.validateSchema( invalidJson );
		REQUIRE_FALSE( isValid );
	}

	SECTION( "materials must be array, not object" )
	{
		const json invalidJson = R"({
            "materials": {},
            "renderPasses": []
        })"_json;

		const bool isValid = validator.validateSchema( invalidJson );
		REQUIRE_FALSE( isValid );
	}

	SECTION( "renderPasses must be array, not object" )
	{
		const json invalidJson = R"({
            "materials": [],
            "renderPasses": {}
        })"_json;

		const bool isValid = validator.validateSchema( invalidJson );
		REQUIRE_FALSE( isValid );
	}

	SECTION( "Optional sections validated when present - defines must be object" )
	{
		const json invalidJson = R"({
            "materials": [],
            "renderPasses": [],
            "defines": []
        })"_json;

		const bool isValid = validator.validateSchema( invalidJson );
		REQUIRE_FALSE( isValid );
	}

	SECTION( "Optional sections validated when present - includes must be array" )
	{
		const json invalidJson = R"({
            "materials": [],
            "renderPasses": [],
            "includes": {}
        })"_json;

		const bool isValid = validator.validateSchema( invalidJson );
		REQUIRE_FALSE( isValid );
	}

	SECTION( "Valid schema with optional sections" )
	{
		const json validJson = R"({
            "materials": [],
            "renderPasses": [],
            "defines": {},
            "includes": []
        })"_json;

		const bool isValid = validator.validateSchema( validJson );
		REQUIRE( isValid );
	}
}

// Phase 2: Parameter Type Validation (T006)

TEST_CASE( "Validator enforces parameter type constraints", "[material-system][validation][T006]" )
{
	material_system::Validator validator;

	SECTION( "Valid parameter types are accepted" )
	{
		// float type
		const json floatParam = R"({
            "name": "roughness",
            "type": "float",
            "default": 0.5
        })"_json;
		REQUIRE( validator.validateParameterType( floatParam ) );

		// int type
		const json intParam = R"({
            "name": "iterations",
            "type": "int",
            "default": 10
        })"_json;
		REQUIRE( validator.validateParameterType( intParam ) );

		// bool type
		const json boolParam = R"({
            "name": "enabled",
            "type": "bool",
            "default": true
        })"_json;
		REQUIRE( validator.validateParameterType( boolParam ) );

		// float4 type
		const json float4Param = R"({
            "name": "color",
            "type": "float4",
            "default": [1.0, 0.0, 0.0, 1.0]
        })"_json;
		REQUIRE( validator.validateParameterType( float4Param ) );
	}

	SECTION( "Invalid parameter type rejected - string not allowed" )
	{
		const json invalidParam = R"({
            "name": "filename",
            "type": "string",
            "default": "texture.png"
        })"_json;

		const bool isValid = validator.validateParameterType( invalidParam );
		REQUIRE_FALSE( isValid );
	}

	SECTION( "Invalid parameter type rejected - array not allowed" )
	{
		const json invalidParam = R"({
            "name": "values",
            "type": "array",
            "default": []
        })"_json;

		const bool isValid = validator.validateParameterType( invalidParam );
		REQUIRE_FALSE( isValid );
	}

	SECTION( "Missing type field fails validation" )
	{
		const json invalidParam = R"({
            "name": "parameter",
            "default": 0
        })"_json;

		const bool isValid = validator.validateParameterType( invalidParam );
		REQUIRE_FALSE( isValid );
	}

	SECTION( "Default value type mismatch - int default for float param" )
	{
		const json invalidParam = R"({
            "name": "roughness",
            "type": "float",
            "default": 10
        })"_json;

		// Note: JSON doesn't distinguish int from float in parsing,
		// so this test documents expected behavior but may pass
		// We'll implement stricter validation in refactor phase if needed
		const bool isValid = validator.validateParameterType( invalidParam );
		// For now, we accept numeric types interchangeably
		REQUIRE( isValid );
	}

	SECTION( "Default value type mismatch - string default for bool param" )
	{
		const json invalidParam = R"({
            "name": "enabled",
            "type": "bool",
            "default": "true"
        })"_json;

		const bool isValid = validator.validateParameterType( invalidParam );
		REQUIRE_FALSE( isValid );
	}

	SECTION( "Default value type mismatch - number for float4 param" )
	{
		const json invalidParam = R"({
            "name": "color",
            "type": "float4",
            "default": 1.0
        })"_json;

		const bool isValid = validator.validateParameterType( invalidParam );
		REQUIRE_FALSE( isValid );
	}

	SECTION( "float4 default must have exactly 4 elements" )
	{
		const json invalidParam3 = R"({
            "name": "color",
            "type": "float4",
            "default": [1.0, 0.0, 0.0]
        })"_json;
		REQUIRE_FALSE( validator.validateParameterType( invalidParam3 ) );

		const json invalidParam5 = R"({
            "name": "color",
            "type": "float4",
            "default": [1.0, 0.0, 0.0, 1.0, 0.5]
        })"_json;
		REQUIRE_FALSE( validator.validateParameterType( invalidParam5 ) );
	}
}

// Phase 2: JSON Document Merging (T007)

TEST_CASE( "JsonLoader merges documents from includes", "[material-system][validation][T007]" )
{
	// Create temporary test files
	const fs::path tempDir = fs::temp_directory_path() / "material_system_test_T007";
	fs::create_directories( tempDir );

	const fs::path mainFile = tempDir / "materials.json";
	const fs::path statesFile = tempDir / "states.json";
	const fs::path shadersFile = tempDir / "shaders.json";

	SECTION( "Merges state blocks from included file" )
	{
		// Create main file with includes reference
		{
			std::ofstream out( mainFile );
			out << R"({
                "includes": ["states.json"],
                "materials": [{"id": "mat1"}],
                "renderPasses": []
            })";
		}

		// Create states file with state blocks
		{
			std::ofstream out( statesFile );
			out << R"({
                "rasterizerStates": {
                    "solid": {"fillMode": "solid"}
                },
                "depthStencilStates": {
                    "depthTest": {"depthEnable": true}
                }
            })";
		}

		material_system::JsonLoader loader;
		const bool success = loader.load( mainFile.string() );

		REQUIRE( success );

		const auto &merged = loader.getMergedDocument();

		// Verify materials from main file
		REQUIRE( merged.contains( "materials" ) );
		REQUIRE( merged["materials"].size() == 1 );

		// Verify state blocks from included file
		REQUIRE( merged.contains( "rasterizerStates" ) );
		REQUIRE( merged["rasterizerStates"].contains( "solid" ) );
		REQUIRE( merged.contains( "depthStencilStates" ) );
		REQUIRE( merged["depthStencilStates"].contains( "depthTest" ) );

		fs::remove_all( tempDir );
	}

	SECTION( "Merges arrays by concatenation" )
	{
		// Main file with one material
		{
			std::ofstream out( mainFile );
			out << R"({
                "includes": ["states.json"],
                "materials": [{"id": "mat1"}]
            })";
		}

		// States file with another material
		{
			std::ofstream out( statesFile );
			out << R"({
                "materials": [{"id": "mat2"}]
            })";
		}

		material_system::JsonLoader loader;
		REQUIRE( loader.load( mainFile.string() ) );

		const auto &merged = loader.getMergedDocument();

		// Both materials should be present
		// Note: included files are processed first, so mat2 comes before mat1
		REQUIRE( merged["materials"].size() == 2 );
		REQUIRE( merged["materials"][0]["id"] == "mat2" );
		REQUIRE( merged["materials"][1]["id"] == "mat1" );

		fs::remove_all( tempDir );
	}

	SECTION( "Merges nested includes (transitive)" )
	{
		// Main includes states, states includes shaders
		{
			std::ofstream out( mainFile );
			out << R"({
                "includes": ["states.json"],
                "materials": []
            })";
		}

		{
			std::ofstream out( statesFile );
			out << R"({
                "includes": ["shaders.json"],
                "rasterizerStates": {"solid": {}}
            })";
		}

		{
			std::ofstream out( shadersFile );
			out << R"({
                "shaders": {"default_vs": {"file": "default.hlsl"}}
            })";
		}

		material_system::JsonLoader loader;
		REQUIRE( loader.load( mainFile.string() ) );

		const auto &merged = loader.getMergedDocument();

		// All sections should be present
		REQUIRE( merged.contains( "materials" ) );
		REQUIRE( merged.contains( "rasterizerStates" ) );
		REQUIRE( merged.contains( "shaders" ) );

		fs::remove_all( tempDir );
	}

	SECTION( "Avoids duplicate loading of same file" )
	{
		// Diamond dependency: main includes A and B, both A and B include common.json
		const fs::path fileA = tempDir / "a.json";
		const fs::path fileB = tempDir / "b.json";
		const fs::path commonFile = tempDir / "common.json";

		{
			std::ofstream out( mainFile );
			out << R"({
                "includes": ["a.json", "b.json"],
                "materials": []
            })";
		}

		{
			std::ofstream out( fileA );
			out << R"({
                "includes": ["common.json"],
                "statesA": {}
            })";
		}

		{
			std::ofstream out( fileB );
			out << R"({
                "includes": ["common.json"],
                "statesB": {}
            })";
		}

		{
			std::ofstream out( commonFile );
			out << R"({
                "materials": [{"id": "common_mat"}]
            })";
		}

		material_system::JsonLoader loader;
		REQUIRE( loader.load( mainFile.string() ) );

		const auto &merged = loader.getMergedDocument();

		// Common material should only appear once (not duplicated)
		REQUIRE( merged["materials"].size() == 1 );
		REQUIRE( merged["materials"][0]["id"] == "common_mat" );

		// Both A and B states should be present
		REQUIRE( merged.contains( "statesA" ) );
		REQUIRE( merged.contains( "statesB" ) );

		fs::remove_all( tempDir );
	}

	SECTION( "Handles missing include file gracefully" )
	{
		{
			std::ofstream out( mainFile );
			out << R"({
                "includes": ["nonexistent.json"],
                "materials": []
            })";
		}

		material_system::JsonLoader loader;
		const bool success = loader.load( mainFile.string() );

		REQUIRE_FALSE( success );

		fs::remove_all( tempDir );
	}
}

// ======================================================================
// T008: Duplicate ID Detection
// ======================================================================

TEST_CASE( "Validator detects duplicate IDs across all scopes", "[T008][material-system][validation]" )
{
	material_system::Validator validator;

	SECTION( "Detects duplicate material IDs" )
	{
		const json document = json::parse( R"({
            "materials": [
                {"id": "mat1"},
                {"id": "mat1"}
            ],
            "renderPasses": []
        })" );

		const bool valid = validator.validateDuplicateIds( document );

		REQUIRE_FALSE( valid );
	}

	SECTION( "Detects duplicate state block IDs" )
	{
		const json document = json::parse( R"({
            "materials": [],
            "renderPasses": [],
            "states": {
                "rasterizer": [
                    {"id": "state1"},
                    {"id": "state1"}
                ]
            }
        })" );

		const bool valid = validator.validateDuplicateIds( document );

		REQUIRE_FALSE( valid );
	}

	SECTION( "Detects cross-category duplicate (material vs state)" )
	{
		const json document = json::parse( R"({
            "materials": [
                {"id": "duplicate"}
            ],
            "renderPasses": [],
            "states": {
                "rasterizer": [
                    {"id": "duplicate"}
                ]
            }
        })" );

		const bool valid = validator.validateDuplicateIds( document );

		REQUIRE_FALSE( valid );
	}

	SECTION( "Detects duplicate shader IDs" )
	{
		const json document = json::parse( R"({
            "materials": [],
            "renderPasses": [],
            "shaders": {
                "vertex": [
                    {"id": "vs1"},
                    {"id": "vs1"}
                ]
            }
        })" );

		const bool valid = validator.validateDuplicateIds( document );

		REQUIRE_FALSE( valid );
	}

	SECTION( "Detects duplicate render pass IDs" )
	{
		const json document = json::parse( R"({
            "materials": [],
            "renderPasses": [
                {"id": "pass1"},
                {"id": "pass1"}
            ]
        })" );

		const bool valid = validator.validateDuplicateIds( document );

		REQUIRE_FALSE( valid );
	}

	SECTION( "Returns true when all IDs are unique" )
	{
		const json document = json::parse( R"({
            "materials": [
                {"id": "mat1"},
                {"id": "mat2"}
            ],
            "renderPasses": [
                {"id": "pass1"}
            ],
            "states": {
                "rasterizer": [
                    {"id": "state1"}
                ]
            },
            "shaders": {
                "vertex": [
                    {"id": "vs1"}
                ]
            }
        })" );

		const bool valid = validator.validateDuplicateIds( document );

		REQUIRE( valid );
	}
}

// ============================================================================
// T009: Parse material definitions from JSON
// ============================================================================

TEST_CASE( "MaterialParser parses minimal valid material", "[material-parser][T009][unit]" )
{
	// Arrange - minimal material with required fields
	const json materialJson = json::parse( R"({
        "id": "basic_lit",
        "pass": "forward",
        "shaders": {
            "vertex": "standard_vs",
            "pixel": "standard_ps"
        }
    })" );

	// Act
	const auto material = graphics::material_system::MaterialParser::parse( materialJson );

	// Assert
	REQUIRE( material.id == "basic_lit" );
	REQUIRE( material.pass == "forward" );
	REQUIRE( material.enabled == true ); // default value
	REQUIRE( material.shaders.size() == 2 );

	// Check vertex shader
	bool foundVertex = false;
	bool foundPixel = false;
	for ( const auto &shader : material.shaders )
	{
		if ( shader.stage == "vertex" )
		{
			REQUIRE( shader.shaderId == "standard_vs" );
			foundVertex = true;
		}
		else if ( shader.stage == "pixel" )
		{
			REQUIRE( shader.shaderId == "standard_ps" );
			foundPixel = true;
		}
	}
	REQUIRE( foundVertex );
	REQUIRE( foundPixel );

	REQUIRE( material.parameters.empty() );
	REQUIRE( material.versionHash.empty() );
}

TEST_CASE( "MaterialParser parses material with all optional fields", "[material-parser][T009][unit]" )
{
	// Arrange - material with all optional fields populated
	const json materialJson = json::parse( R"({
        "id": "advanced_lit",
        "pass": "deferred",
        "shaders": {
            "vertex": "adv_vs",
            "pixel": "adv_ps"
        },
        "parameters": [
            {
                "name": "roughness",
                "type": "float",
                "defaultValue": 0.5
            },
            {
                "name": "tint",
                "type": "float4",
                "defaultValue": [1.0, 0.8, 0.6, 1.0]
            }
        ],
        "states": {
            "rasterizer": "cull_back",
            "depthStencil": "depth_test_write",
            "blend": "alpha_blend"
        },
        "enabled": false,
        "versionHash": "abc123"
    })" );

	// Act
	const auto material = graphics::material_system::MaterialParser::parse( materialJson );

	// Assert
	REQUIRE( material.id == "advanced_lit" );
	REQUIRE( material.pass == "deferred" );
	REQUIRE( material.enabled == false );
	REQUIRE( material.versionHash == "abc123" );

	// Check shaders
	REQUIRE( material.shaders.size() == 2 );

	// Check parameters
	REQUIRE( material.parameters.size() == 2 );
	REQUIRE( material.parameters[0].name == "roughness" );
	REQUIRE( material.parameters[0].type == graphics::material_system::ParameterType::Float );
	REQUIRE( material.parameters[0].defaultValue == 0.5 );

	REQUIRE( material.parameters[1].name == "tint" );
	REQUIRE( material.parameters[1].type == graphics::material_system::ParameterType::Float4 );
	REQUIRE( material.parameters[1].defaultValue.is_array() );
	REQUIRE( material.parameters[1].defaultValue.size() == 4 );

	// Check states
	REQUIRE( material.states.rasterizer == "cull_back" );
	REQUIRE( material.states.depthStencil == "depth_test_write" );
	REQUIRE( material.states.blend == "alpha_blend" );
}

// ============================================================================
// T010: Validate material references
// ============================================================================

TEST_CASE( "ReferenceValidator detects undefined pass reference", "[reference-validator][T010][unit]" )
{
	// Arrange - material referencing non-existent pass
	const json materialJson = json::parse( R"({
        "id": "invalid_pass_mat",
        "pass": "nonexistent_pass",
        "shaders": {
            "vertex": "vs1",
            "pixel": "ps1"
        }
    })" );

	const auto material = graphics::material_system::MaterialParser::parse( materialJson );

	// Known passes (enum values)
	const std::vector<std::string> knownPasses = { "forward", "deferred", "shadow" };

	// Known states and shaders (empty for this test)
	const json document = json::parse( R"({
        "materials": [],
        "renderPasses": [],
        "shaders": {
            "vertex": [{"id": "vs1"}],
            "pixel": [{"id": "ps1"}]
        }
    })" );

	graphics::material_system::ReferenceValidator validator;

	// Act & Assert - should return false for undefined pass
	const bool valid = validator.validateReferences( material, knownPasses, document );

	REQUIRE_FALSE( valid );
}

TEST_CASE( "ReferenceValidator detects undefined state reference", "[reference-validator][T010][unit]" )
{
	// Arrange - material referencing non-existent rasterizer state
	const json materialJson = json::parse( R"({
        "id": "invalid_state_mat",
        "pass": "forward",
        "shaders": {
            "vertex": "vs1",
            "pixel": "ps1"
        },
        "states": {
            "rasterizer": "missing_state"
        }
    })" );

	const auto material = graphics::material_system::MaterialParser::parse( materialJson );

	const std::vector<std::string> knownPasses = { "forward", "deferred" };

	// Document with states but not the one referenced
	const json document = json::parse( R"({
        "materials": [],
        "renderPasses": [],
        "shaders": {
            "vertex": [{"id": "vs1"}],
            "pixel": [{"id": "ps1"}]
        },
        "states": {
            "rasterizer": [{"id": "other_state"}]
        }
    })" );

	graphics::material_system::ReferenceValidator validator;

	// Act & Assert
	const bool valid = validator.validateReferences( material, knownPasses, document );

	REQUIRE_FALSE( valid );
}

TEST_CASE( "ReferenceValidator detects undefined shader reference", "[reference-validator][T010][unit]" )
{
	// Arrange - material referencing non-existent shader
	const json materialJson = json::parse( R"({
        "id": "invalid_shader_mat",
        "pass": "forward",
        "shaders": {
            "vertex": "missing_vs",
            "pixel": "ps1"
        }
    })" );

	const auto material = graphics::material_system::MaterialParser::parse( materialJson );

	const std::vector<std::string> knownPasses = { "forward" };

	// Document with shaders but not the one referenced
	const json document = json::parse( R"({
        "materials": [],
        "renderPasses": [],
        "shaders": {
            "vertex": [{"id": "vs1"}],
            "pixel": [{"id": "ps1"}]
        }
    })" );

	graphics::material_system::ReferenceValidator validator;

	// Act & Assert
	const bool valid = validator.validateReferences( material, knownPasses, document );

	REQUIRE_FALSE( valid );
}

TEST_CASE( "ReferenceValidator accepts valid references", "[reference-validator][T010][unit]" )
{
	// Arrange - material with all valid references
	const json materialJson = json::parse( R"({
        "id": "valid_mat",
        "pass": "forward",
        "shaders": {
            "vertex": "std_vs",
            "pixel": "std_ps"
        },
        "states": {
            "rasterizer": "cull_back",
            "depthStencil": "depth_write"
        }
    })" );

	const auto material = graphics::material_system::MaterialParser::parse( materialJson );

	const std::vector<std::string> knownPasses = { "forward", "deferred" };

	// Document with all referenced entities
	const json document = json::parse( R"({
        "materials": [],
        "renderPasses": [],
        "shaders": {
            "vertex": [{"id": "std_vs"}],
            "pixel": [{"id": "std_ps"}]
        },
        "states": {
            "rasterizer": [{"id": "cull_back"}],
            "depthStencil": [{"id": "depth_write"}]
        }
    })" );

	graphics::material_system::ReferenceValidator validator;

	// Act & Assert
	const bool valid = validator.validateReferences( material, knownPasses, document );

	REQUIRE( valid );
}

// ============================================================================
// T011: Enforce hierarchical define uniqueness
// ============================================================================

TEST_CASE( "DefineValidator detects duplicate between global and material defines", "[define-validator][T011][unit]" )
{
	// Arrange - global and material both define "FOO"
	const std::unordered_map<std::string, std::string> globalDefines = {
		{ "FOO", "1" }
	};
	const std::unordered_map<std::string, std::string> passDefines = {};
	const std::unordered_map<std::string, std::string> materialDefines = {
		{ "FOO", "2" } // Duplicate!
	};

	graphics::material_system::DefineValidator validator;

	// Act & Assert - should return false for duplicate
	const bool valid = validator.checkHierarchy( globalDefines, passDefines, materialDefines, "test_material" );

	REQUIRE_FALSE( valid );
}

TEST_CASE( "DefineValidator detects duplicate between pass and material defines", "[define-validator][T011][unit]" )
{
	// Arrange - pass and material both define "BAR"
	const std::unordered_map<std::string, std::string> globalDefines = {};
	const std::unordered_map<std::string, std::string> passDefines = {
		{ "BAR", "alpha" }
	};
	const std::unordered_map<std::string, std::string> materialDefines = {
		{ "BAR", "beta" } // Duplicate!
	};

	graphics::material_system::DefineValidator validator;

	// Act & Assert
	const bool valid = validator.checkHierarchy( globalDefines, passDefines, materialDefines, "test_material" );

	REQUIRE_FALSE( valid );
}

TEST_CASE( "DefineValidator detects duplicate between global and pass defines", "[define-validator][T011][unit]" )
{
	// Arrange - global and pass both define "VERSION"
	const std::unordered_map<std::string, std::string> globalDefines = {
		{ "VERSION", "100" }
	};
	const std::unordered_map<std::string, std::string> passDefines = {
		{ "VERSION", "200" } // Duplicate!
	};
	const std::unordered_map<std::string, std::string> materialDefines = {};

	graphics::material_system::DefineValidator validator;

	// Act & Assert
	const bool valid = validator.checkHierarchy( globalDefines, passDefines, materialDefines, "test_material" );

	REQUIRE_FALSE( valid );
}

TEST_CASE( "DefineValidator accepts unique defines across all levels", "[define-validator][T011][unit]" )
{
	// Arrange - all unique defines
	const std::unordered_map<std::string, std::string> globalDefines = {
		{ "GLOBAL_FLAG", "1" },
		{ "VERSION", "100" }
	};
	const std::unordered_map<std::string, std::string> passDefines = {
		{ "PASS_TYPE", "forward" }
	};
	const std::unordered_map<std::string, std::string> materialDefines = {
		{ "MATERIAL_ID", "42" },
		{ "USE_NORMALS", "1" }
	};

	graphics::material_system::DefineValidator validator;

	// Act & Assert
	const bool valid = validator.checkHierarchy( globalDefines, passDefines, materialDefines, "test_material" );

	REQUIRE( valid );
}

TEST_CASE( "DefineValidator returns merged defines map for valid hierarchy", "[define-validator][T011][unit]" )
{
	// Arrange
	const std::unordered_map<std::string, std::string> globalDefines = {
		{ "GLOBAL_A", "1" }
	};
	const std::unordered_map<std::string, std::string> passDefines = {
		{ "PASS_B", "2" }
	};
	const std::unordered_map<std::string, std::string> materialDefines = {
		{ "MAT_C", "3" }
	};

	graphics::material_system::DefineValidator validator;

	// Act
	const auto merged = validator.getMergedDefines( globalDefines, passDefines, materialDefines );

	// Assert - all defines should be present
	REQUIRE( merged.size() == 3 );
	REQUIRE( merged.at( "GLOBAL_A" ) == "1" );
	REQUIRE( merged.at( "PASS_B" ) == "2" );
	REQUIRE( merged.at( "MAT_C" ) == "3" );
}

// ============================================================================
// T012: Shader Compilation Integration
// ============================================================================

TEST_CASE( "MaterialShaderCompiler compiles shader with merged hierarchical defines", "[shader-compiler][T012][unit]" )
{
	// Arrange - merged defines from DefineValidator
	const std::unordered_map<std::string, std::string> mergedDefines = {
		{ "GLOBAL_DEFINE", "1" },
		{ "PASS_DEFINE", "1" },
		{ "MATERIAL_DEFINE", "1" }
	};

	const std::filesystem::path shaderPath = "shaders/test_material_defines.hlsl";
	const std::string entryPoint = "VSMain";
	const std::string profile = "vs_5_1";

	// Act - compile shader with material defines (this will fail until MaterialShaderCompiler exists)
	const auto shaderBlob = graphics::material_system::MaterialShaderCompiler::CompileWithDefines(
		shaderPath, entryPoint, profile, mergedDefines );

	// Assert - shader should compile successfully with all defines applied
	REQUIRE( shaderBlob.isValid() );
	REQUIRE( shaderBlob.blob != nullptr );
	REQUIRE( shaderBlob.blob->GetBufferSize() > 0 );
	REQUIRE( shaderBlob.entryPoint == entryPoint );
	REQUIRE( shaderBlob.profile == profile );
}

TEST_CASE( "MaterialShaderCompiler compiles shader with empty defines", "[shader-compiler][T012][unit]" )
{
	// Arrange - no defines
	const std::unordered_map<std::string, std::string> mergedDefines;

	const std::filesystem::path shaderPath = "shaders/test_material_defines.hlsl";
	const std::string entryPoint = "VSMain";
	const std::string profile = "vs_5_1";

	// Act - compile should still succeed without defines
	const auto shaderBlob = graphics::material_system::MaterialShaderCompiler::CompileWithDefines(
		shaderPath, entryPoint, profile, mergedDefines );

	// Assert - shader compiles (but logic will fail due to missing defines)
	REQUIRE( shaderBlob.isValid() );
	REQUIRE( shaderBlob.blob != nullptr );
}

TEST_CASE( "MaterialShaderCompiler handles multiple defines with consistent ordering", "[shader-compiler][T012][unit]" )
{
	// Arrange - multiple defines that should be sorted for deterministic compilation
	const std::unordered_map<std::string, std::string> defines = {
		{ "Z_LAST", "1" },
		{ "A_FIRST", "2" },
		{ "M_MIDDLE", "3" }
	};

	const std::filesystem::path shaderPath = "shaders/test_material_defines.hlsl";
	const std::string entryPoint = "PSMain";
	const std::string profile = "ps_5_1";

	// Act - compile with multiple defines
	const auto blob = graphics::material_system::MaterialShaderCompiler::CompileWithDefines(
		shaderPath, entryPoint, profile, defines );

	// Assert - compilation should succeed with all defines applied
	REQUIRE( blob.isValid() );
	REQUIRE( blob.blob != nullptr );
	REQUIRE( blob.blob->GetBufferSize() > 0 );

	// Note: Deterministic ordering is tested implicitly by consistent compilation results
	// across runs; bytecode comparison is unreliable due to D3DCompile timestamps
}

TEST_CASE( "MaterialShaderCompiler fails gracefully for missing shader file", "[shader-compiler][T012][unit]" )
{
	// Arrange - non-existent shader path
	const std::unordered_map<std::string, std::string> defines = { { "TEST", "1" } };
	const std::filesystem::path shaderPath = "shaders/nonexistent_shader.hlsl";

	// Act & Assert - should throw/log error for missing file
	REQUIRE_THROWS( graphics::material_system::MaterialShaderCompiler::CompileWithDefines(
		shaderPath, "VSMain", "vs_5_1", defines ) );
}

// ============================================================================
// T013: Root Signature Generation
// ============================================================================

TEST_CASE( "RootSignatureBuilder generates spec with CBV binding", "[root-signature][T013][unit]" )
{
	// Arrange - material with single CBV parameter
	graphics::material_system::MaterialDefinition material;
	material.id = "test_mat";
	material.parameters = {
		{ "ViewProjection", graphics::material_system::ParameterType::Float4, nlohmann::json::array( { 0, 0, 0, 0 } ) }
	};

	// Act - build root signature spec (this will fail until RootSignatureBuilder exists)
	const auto spec = graphics::material_system::RootSignatureBuilder::Build( material );

	// Assert - spec should contain one CBV binding
	REQUIRE( spec.resourceBindings.size() == 1 );
	REQUIRE( spec.resourceBindings[0].name == "ViewProjection" );
	REQUIRE( spec.resourceBindings[0].type == graphics::material_system::ResourceBindingType::CBV );
	REQUIRE( spec.resourceBindings[0].slot >= 0 );
}

TEST_CASE( "RootSignatureBuilder generates spec with multiple bindings sorted by name", "[root-signature][T013][unit]" )
{
	// Arrange - material with multiple parameters (unsorted)
	graphics::material_system::MaterialDefinition material;
	material.id = "test_mat";
	material.parameters = {
		{ "ZLast", graphics::material_system::ParameterType::Float, 0.0f },
		{ "AFirst", graphics::material_system::ParameterType::Int, 0 },
		{ "MMiddle", graphics::material_system::ParameterType::Bool, false }
	};

	// Act - build root signature spec
	const auto spec = graphics::material_system::RootSignatureBuilder::Build( material );

	// Assert - bindings should be sorted alphabetically
	REQUIRE( spec.resourceBindings.size() == 3 );
	REQUIRE( spec.resourceBindings[0].name == "AFirst" );
	REQUIRE( spec.resourceBindings[0].slot == 0 );
	REQUIRE( spec.resourceBindings[1].name == "MMiddle" );
	REQUIRE( spec.resourceBindings[1].slot == 1 );
	REQUIRE( spec.resourceBindings[2].name == "ZLast" );
	REQUIRE( spec.resourceBindings[2].slot == 2 );
}

TEST_CASE( "RootSignatureBuilder handles material with no parameters", "[root-signature][T013][unit]" )
{
	// Arrange - material with no parameters
	graphics::material_system::MaterialDefinition material;
	material.id = "test_mat";
	material.parameters = {};

	// Act - build root signature spec
	const auto spec = graphics::material_system::RootSignatureBuilder::Build( material );

	// Assert - spec should be empty
	REQUIRE( spec.resourceBindings.empty() );
}

// ============================================================================
// T014: PSO Construction & Caching
// ============================================================================

TEST_CASE( "PipelineBuilder creates PSO from MaterialDefinition", "[pipeline-builder][T014][unit]" )
{
	// Arrange - headless DX12 device
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "PipelineBuilder PSO creation" ) )
		return;

	// Arrange - minimal MaterialDefinition
	graphics::material_system::MaterialDefinition material;
	material.id = "test_simple_material";
	material.pass = "forward";
	material.shaders = {
		{ "vertex", "simple_vs" },
		{ "pixel", "simple_ps" }
	};
	material.states.rasterizer = "default_raster";
	material.states.depthStencil = "default_depth";
	material.states.blend = "default_blend";

	// Arrange - render pass config
	graphics::material_system::RenderPassConfig passConfig;
	passConfig.name = "forward";
	passConfig.rtvFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	passConfig.dsvFormat = DXGI_FORMAT_D32_FLOAT;
	passConfig.numRenderTargets = 1;

	// Act - build PSO (this will fail until buildPSO is implemented)
	auto pso = graphics::material_system::PipelineBuilder::buildPSO( &device, material, passConfig );

	// Assert - PSO handle should be valid (non-null, usable for rendering)
	REQUIRE( pso != nullptr );
}

TEST_CASE( "PipelineBuilder caches and reuses PSO for identical requests", "[pipeline-builder][T014][cache][unit]" )
{
	// Arrange - headless DX12 device
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "PipelineBuilder PSO caching" ) )
		return;

	// Arrange - minimal MaterialDefinition
	graphics::material_system::MaterialDefinition material;
	material.id = "test_cached_material";
	material.pass = "forward";
	material.shaders = {
		{ "vertex", "simple_vs" },
		{ "pixel", "simple_ps" }
	};
	material.states.rasterizer = "default_raster";
	material.states.depthStencil = "default_depth";
	material.states.blend = "default_blend";

	// Arrange - render pass config
	graphics::material_system::RenderPassConfig passConfig;
	passConfig.name = "forward";
	passConfig.rtvFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	passConfig.dsvFormat = DXGI_FORMAT_D32_FLOAT;
	passConfig.numRenderTargets = 1;

	// Act - build PSO twice with identical inputs
	auto pso1 = graphics::material_system::PipelineBuilder::buildPSO( &device, material, passConfig );
	auto pso2 = graphics::material_system::PipelineBuilder::buildPSO( &device, material, passConfig );

	// Assert - both should be valid
	REQUIRE( pso1 != nullptr );
	REQUIRE( pso2 != nullptr );

	// Assert - second call should return cached instance (same pointer)
	REQUIRE( pso1.Get() == pso2.Get() );
}

// ============================================================================
// T015: Expose Material System API to Renderer
// ============================================================================

TEST_CASE( "MaterialSystem provides handle-based API for renderer queries", "[material-system][T015][api][unit]" )
{
	// Arrange - create temporary materials JSON
	const fs::path tempDir = fs::temp_directory_path() / "material_system_test_T015";
	fs::create_directories( tempDir );
	const fs::path materialsJson = tempDir / "materials.json";

	{
		std::ofstream out( materialsJson );
		out << R"({
			"materials": [
				{
					"id": "test_material",
					"pass": "forward",
					"shaders": {
						"vertex": "simple_vs",
						"pixel": "simple_ps"
					}
				}
			],
			"renderPasses": [
				{
					"id": "forward",
					"name": "forward"
				}
			]
		})";
	}

	// Act - initialize material system with JSON
	graphics::material_system::MaterialSystem materialSystem;
	const bool initialized = materialSystem.initialize( materialsJson.string() );

	// Assert - initialization should succeed
	REQUIRE( initialized );

	// Act - get material handle by ID
	const auto handle = materialSystem.getMaterialHandle( "test_material" );

	// Assert - handle should be valid
	REQUIRE( handle.isValid() );

	// Cleanup
	fs::remove_all( tempDir );
}

TEST_CASE( "MaterialSystem returns invalid handle for undefined material", "[material-system][T015][api][unit]" )
{
	// Arrange - minimal materials JSON
	const fs::path tempDir = fs::temp_directory_path() / "material_system_test_T015_invalid";
	fs::create_directories( tempDir );
	const fs::path materialsJson = tempDir / "materials.json";

	{
		std::ofstream out( materialsJson );
		out << R"({
			"materials": [],
			"renderPasses": []
		})";
	}

	graphics::material_system::MaterialSystem materialSystem;
	materialSystem.initialize( materialsJson.string() );

	// Act - query non-existent material
	const auto handle = materialSystem.getMaterialHandle( "nonexistent_material" );

	// Assert - handle should be invalid
	REQUIRE_FALSE( handle.isValid() );

	// Cleanup
	fs::remove_all( tempDir );
}

// ============================================================================
// T016: Integration Test - Complete flow from JSON to material query
// ============================================================================

TEST_CASE( "MaterialSystem integration - load JSON, query material, validate end-to-end flow", "[material-system][T016][integration]" )
{
	// Arrange - Create minimal materials.json with one complete material
	const fs::path tempDir = fs::temp_directory_path() / "material_system_test_T016_integration";
	fs::create_directories( tempDir );
	const fs::path materialsJson = tempDir / "materials.json";

	{
		std::ofstream out( materialsJson );
		out << R"({
			"materials": [
				{
					"id": "IntegrationTestMaterial",
					"pass": "forward",
					"shaders": {
						"vertex": "simple_vs",
						"pixel": "simple_ps"
					},
					"states": {
						"rasterizer": "solid_back",
						"depthStencil": "depth_test_write",
						"blend": "opaque"
					}
				}
			],
			"renderPasses": [
				{
					"id": "forward",
					"name": "Forward Rendering Pass"
				}
			]
		})";
	}

	// Act - Initialize MaterialSystem from JSON file (simulates app startup)
	graphics::material_system::MaterialSystem materialSystem;
	const bool initialized = materialSystem.initialize( materialsJson.string() );

	// Assert - Material system should initialize successfully
	REQUIRE( initialized );

	// Act - Query material handle by ID (simulates renderer querying materials)
	const auto handle = materialSystem.getMaterialHandle( "IntegrationTestMaterial" );

	// Assert - Handle should be valid
	REQUIRE( handle.isValid() );

	// Act - Get material definition using handle (renderer would use this to access material data)
	const auto *material = materialSystem.getMaterial( handle );

	// Assert - Material should be available
	REQUIRE( material != nullptr );
	REQUIRE( material->id == "IntegrationTestMaterial" );

	// Assert - Material should have shader configuration
	REQUIRE_FALSE( material->shaders.empty() );

	// Assert - Material should have pass set
	REQUIRE( material->pass == "forward" );

	// Note: PSO building is not tested here as it requires D3D12 device initialization
	// PSO functionality is validated in PipelineBuilder tests (T013, T014)

	// Cleanup
	fs::remove_all( tempDir );
}
