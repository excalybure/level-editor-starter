#include "graphics/material_system/validator.h"
#include <nlohmann/json.hpp>
#include "core/console.h"

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

} // namespace material_system
