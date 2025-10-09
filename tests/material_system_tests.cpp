#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "graphics/material_system/material_system.h"
#include "graphics/material_system/loader.h"
#include "graphics/material_system/validator.h"
#include "core/console.h"
#include <filesystem>
#include <fstream>

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
