#include "graphics/material_system/validator.h"
#include <nlohmann/json.hpp>
#include "core/console.h"
#include <unordered_set>
#include <string>

namespace material_system
{

bool Validator::validateSchema( const nlohmann::json &document )
{
	// Check required sections exist
	if ( !document.contains( "materials" ) )
	{
		console::error( "Schema validation failed: missing required section 'materials'" );
		return false;
	}

	if ( !document.contains( "renderPasses" ) )
	{
		console::error( "Schema validation failed: missing required section 'renderPasses'" );
		return false;
	}

	// Check required sections are correct type
	if ( !document["materials"].is_array() )
	{
		console::error( "Schema validation failed: 'materials' must be an array" );
		return false;
	}

	if ( !document["renderPasses"].is_array() )
	{
		console::error( "Schema validation failed: 'renderPasses' must be an array" );
		return false;
	}

	// Validate optional sections if present
	if ( document.contains( "defines" ) && !document["defines"].is_object() )
	{
		console::error( "Schema validation failed: 'defines' must be an object" );
		return false;
	}

	if ( document.contains( "includes" ) && !document["includes"].is_array() )
	{
		console::error( "Schema validation failed: 'includes' must be an array" );
		return false;
	}

	// All validations passed
	return true;
}

bool Validator::validateParameterType( const nlohmann::json &parameter )
{
	// Check that type field exists
	if ( !parameter.contains( "type" ) )
	{
		console::error( "Parameter validation failed: missing 'type' field" );
		return false;
	}

	const std::string type = parameter["type"];

	// Check type is one of the allowed values
	if ( type != "float" && type != "int" && type != "bool" && type != "float4" )
	{
		console::error( "Parameter validation failed: invalid type '" + type + "'. Allowed types: float, int, bool, float4" );
		return false;
	}

	// If default value present, validate it matches the type
	if ( parameter.contains( "default" ) )
	{
		const auto &defaultValue = parameter["default"];

		if ( type == "float" || type == "int" )
		{
			// Accept any numeric type for float/int (JSON doesn't distinguish)
			if ( !defaultValue.is_number() )
			{
				console::error( "Parameter validation failed: default value for '" + type + "' must be a number" );
				return false;
			}
		}
		else if ( type == "bool" )
		{
			if ( !defaultValue.is_boolean() )
			{
				console::error( "Parameter validation failed: default value for 'bool' must be a boolean" );
				return false;
			}
		}
		else if ( type == "float4" )
		{
			if ( !defaultValue.is_array() )
			{
				console::error( "Parameter validation failed: default value for 'float4' must be an array" );
				return false;
			}

			if ( defaultValue.size() != 4 )
			{
				console::error( "Parameter validation failed: default value for 'float4' must have exactly 4 elements, got " + std::to_string( defaultValue.size() ) );
				return false;
			}

			// Verify all elements are numbers
			for ( size_t i = 0; i < 4; ++i )
			{
				if ( !defaultValue[i].is_number() )
				{
					console::error( "Parameter validation failed: float4 default values must be numbers" );
					return false;
				}
			}
		}
	}

	// All validations passed
	return true;
}

bool Validator::validateDuplicateIds( const nlohmann::json &document )
{
	// Use unordered_set for O(1) lookup
	std::unordered_set<std::string> seenIds;
	bool allUnique = true;

	// Helper lambda to check for duplicates in an array of objects with "id" field
	const auto checkArray = [&]( const nlohmann::json &arr, const std::string &category ) {
		if ( !arr.is_array() )
		{
			return;
		}

		for ( const auto &item : arr )
		{
			if ( !item.contains( "id" ) )
			{
				continue; // Skip items without id field
			}

			const std::string id = item["id"];
			if ( seenIds.count( id ) > 0 )
			{
				console::error( "Duplicate ID detected: '" + id + "' in " + category );
				allUnique = false;
			}
			else
			{
				seenIds.insert( id );
			}
		}
	};

	// Check materials array
	if ( document.contains( "materials" ) )
	{
		checkArray( document["materials"], "materials" );
	}

	// Check renderPasses array
	if ( document.contains( "renderPasses" ) )
	{
		checkArray( document["renderPasses"], "renderPasses" );
	}

	// Check states object (nested arrays per state type)
	if ( document.contains( "states" ) && document["states"].is_object() )
	{
		for ( const auto &[stateType, stateArray] : document["states"].items() )
		{
			checkArray( stateArray, "states." + stateType );
		}
	}

	// Check shaders object (nested arrays per shader type)
	if ( document.contains( "shaders" ) && document["shaders"].is_object() )
	{
		for ( const auto &[shaderType, shaderArray] : document["shaders"].items() )
		{
			checkArray( shaderArray, "shaders." + shaderType );
		}
	}

	return allUnique;
}

} // namespace material_system
