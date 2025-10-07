#include "runtime/scene_serialization/SceneSerializer.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include "runtime/components.h"
#include "engine/assets/asset_manager.h"

using json = nlohmann::json;

namespace scene
{

std::string SceneSerializer::getCurrentISO8601Timestamp()
{
	const auto now = std::chrono::system_clock::now();
	const auto time = std::chrono::system_clock::to_time_t( now );
	std::tm tm;
	localtime_s( &tm, &time );

	std::ostringstream oss;
	oss << std::put_time( &tm, "%Y-%m-%dT%H:%M:%S" ) << "Z";
	return oss.str();
}

SerializationErrorInfo SceneSerializer::makeError( SerializationError error, const std::string &message, const std::filesystem::path &filepath, int lineNumber )
{
	return SerializationErrorInfo{ error, message, filepath.string(), lineNumber };
}

// Anonymous namespace for private helper functions
namespace
{
void serializeTransform( json &componentJson, const components::Transform &transform )
{
	componentJson["position"] = { transform.position.x, transform.position.y, transform.position.z };
	componentJson["rotation"] = { transform.rotation.x, transform.rotation.y, transform.rotation.z };
	componentJson["scale"] = { transform.scale.x, transform.scale.y, transform.scale.z };
}

void serializeVisible( json &componentJson, const components::Visible &visible )
{
	componentJson["visible"] = visible.visible;
	componentJson["castShadows"] = visible.castShadows;
	componentJson["receiveShadows"] = visible.receiveShadows;
}

void serializeMeshRenderer( json &componentJson, const components::MeshRenderer &meshRenderer )
{
	// Serialize meshPath if present; otherwise fall back to meshHandle for programmatic entities
	if ( !meshRenderer.meshPath.empty() )
	{
		componentJson["meshPath"] = meshRenderer.meshPath;
	}
	else
	{
		// Fallback for entities created programmatically without asset reference
		componentJson["meshHandle"] = meshRenderer.meshHandle;
	}

	componentJson["lodBias"] = meshRenderer.lodBias;
}

void deserializeTransform( const json &componentJson, components::Transform &transform )
{
	if ( componentJson.contains( "position" ) && componentJson["position"].is_array() && componentJson["position"].size() == 3 )
	{
		transform.position.x = componentJson["position"][0];
		transform.position.y = componentJson["position"][1];
		transform.position.z = componentJson["position"][2];
	}

	if ( componentJson.contains( "rotation" ) && componentJson["rotation"].is_array() && componentJson["rotation"].size() == 3 )
	{
		transform.rotation.x = componentJson["rotation"][0];
		transform.rotation.y = componentJson["rotation"][1];
		transform.rotation.z = componentJson["rotation"][2];
	}

	if ( componentJson.contains( "scale" ) && componentJson["scale"].is_array() && componentJson["scale"].size() == 3 )
	{
		transform.scale.x = componentJson["scale"][0];
		transform.scale.y = componentJson["scale"][1];
		transform.scale.z = componentJson["scale"][2];
	}

	transform.markDirty();
}

void deserializeVisible( const json &componentJson, components::Visible &visible )
{
	if ( componentJson.contains( "visible" ) )
	{
		visible.visible = componentJson["visible"];
	}
	if ( componentJson.contains( "castShadows" ) )
	{
		visible.castShadows = componentJson["castShadows"];
	}
	if ( componentJson.contains( "receiveShadows" ) )
	{
		visible.receiveShadows = componentJson["receiveShadows"];
	}
}

void deserializeMeshRenderer( const json &componentJson, components::MeshRenderer &meshRenderer )
{
	// Try to load meshPath first (new format)
	if ( componentJson.contains( "meshPath" ) )
	{
		meshRenderer.meshPath = componentJson["meshPath"];
		// Set handle to 0 as placeholder (will be resolved during asset loading)
		meshRenderer.meshHandle = 0;
	}
	// Fall back to meshHandle for backward compatibility with old format
	else if ( componentJson.contains( "meshHandle" ) )
	{
		meshRenderer.meshHandle = componentJson["meshHandle"];
		// Leave meshPath empty for old format files
		meshRenderer.meshPath = "";
	}

	if ( componentJson.contains( "lodBias" ) )
	{
		meshRenderer.lodBias = componentJson["lodBias"];
	}
}

} // anonymous namespace

std::expected<void, SerializationErrorInfo> SceneSerializer::saveScene(
	const ecs::Scene &scene,
	const std::filesystem::path &filepath,
	const SceneMetadata &metadata )
{
	try
	{
		json sceneJson;

		// Metadata
		sceneJson["version"] = metadata.version;
		sceneJson["metadata"]["name"] = metadata.name;
		sceneJson["metadata"]["created"] = metadata.created.empty() ? getCurrentISO8601Timestamp() : metadata.created;
		sceneJson["metadata"]["modified"] = getCurrentISO8601Timestamp();
		sceneJson["metadata"]["author"] = metadata.author;

		// Entities array
		sceneJson["entities"] = json::array();

		// Get all entities
		const auto entities = scene.getAllEntities();

		// Map entity handles to JSON IDs (1-based for readability)
		std::unordered_map<ecs::Entity, int> entityToID;
		int nextID = 1;
		for ( const auto entity : entities )
		{
			entityToID[entity] = nextID++;
		}

		// Serialize each entity
		for ( const auto entity : entities )
		{
			json entityJson;
			entityJson["id"] = entityToID[entity];

			// Name component
			if ( scene.hasComponent<components::Name>( entity ) )
			{
				const auto *name = scene.getComponent<components::Name>( entity );
				if ( name )
				{
					entityJson["name"] = name->name;
				}
			}

			// Parent reference
			const auto parent = scene.getParent( entity );
			if ( parent != ecs::Entity{} )
			{
				entityJson["parent"] = entityToID[parent];
			}
			else
			{
				entityJson["parent"] = nullptr;
			}

			// Components object
			entityJson["components"] = json::object();

			// Transform component (always serialize)
			if ( scene.hasComponent<components::Transform>( entity ) )
			{
				const auto *transform = scene.getComponent<components::Transform>( entity );
				if ( transform )
				{
					serializeTransform( entityJson["components"]["transform"], *transform );
				}
			}

			// Visible component
			if ( scene.hasComponent<components::Visible>( entity ) )
			{
				const auto *visible = scene.getComponent<components::Visible>( entity );
				if ( visible )
				{
					serializeVisible( entityJson["components"]["visible"], *visible );
				}
			}

			// MeshRenderer component
			if ( scene.hasComponent<components::MeshRenderer>( entity ) )
			{
				const auto *meshRenderer = scene.getComponent<components::MeshRenderer>( entity );
				if ( meshRenderer )
				{
					serializeMeshRenderer( entityJson["components"]["meshRenderer"], *meshRenderer );
				}
			}

			sceneJson["entities"].push_back( entityJson );
		}

		// Write to file
		std::ofstream file( filepath );
		if ( !file.is_open() )
		{
			return std::unexpected( makeError( SerializationError::FileAccessDenied, "Could not open file for writing", filepath ) );
		}

		file << sceneJson.dump( 2 ); // Pretty print with 2-space indent
		file.close();

		return {};
	}
	catch ( const json::exception &e )
	{
		return std::unexpected( makeError( SerializationError::InvalidJSON, std::string( "JSON error: " ) + e.what(), filepath ) );
	}
	catch ( const std::exception &e )
	{
		return std::unexpected( makeError( SerializationError::Unknown, std::string( "Unknown error: " ) + e.what(), filepath ) );
	}
}

std::expected<void, SerializationErrorInfo> SceneSerializer::loadScene(
	ecs::Scene &scene,
	const std::filesystem::path &filepath )
{
	try
	{
		// Check if file exists
		if ( !std::filesystem::exists( filepath ) )
		{
			return std::unexpected( makeError( SerializationError::FileNotFound, "Scene file not found", filepath ) );
		}

		// Read file
		std::ifstream file( filepath );
		if ( !file.is_open() )
		{
			return std::unexpected( makeError( SerializationError::FileAccessDenied, "Could not open file for reading", filepath ) );
		}

		json sceneJson;
		try
		{
			file >> sceneJson;
		}
		catch ( const json::parse_error &e )
		{
			return std::unexpected( makeError( SerializationError::InvalidJSON, std::string( "JSON parse error: " ) + e.what(), filepath, e.byte ) );
		}

		// Validate version
		if ( !sceneJson.contains( "version" ) )
		{
			return std::unexpected( makeError( SerializationError::MissingRequiredField, "Missing version field", filepath ) );
		}

		const std::string version = sceneJson["version"];
		if ( version != "1.0" )
		{
			return std::unexpected( makeError( SerializationError::UnsupportedVersion, "Unsupported scene version: " + version, filepath ) );
		}

		// Clear existing scene
		// Copy entities list before destroying to avoid iterator invalidation
		const auto entitiesToDestroy = std::vector<ecs::Entity>( scene.getAllEntities().begin(), scene.getAllEntities().end() );
		for ( const auto entity : entitiesToDestroy )
		{
			scene.destroyEntity( entity );
		}

		// Load entities
		if ( !sceneJson.contains( "entities" ) || !sceneJson["entities"].is_array() )
		{
			return std::unexpected( makeError( SerializationError::MissingRequiredField, "Missing or invalid entities array", filepath ) );
		}

		// Map old IDs to new entity handles
		std::unordered_map<int, ecs::Entity> idToEntity;

		// First pass: create entities and deserialize components (except hierarchy)
		for ( const auto &entityJson : sceneJson["entities"] )
		{
			if ( !entityJson.contains( "id" ) )
			{
				return std::unexpected( makeError( SerializationError::MissingRequiredField, "Entity missing id field", filepath ) );
			}

			const int id = entityJson["id"];
			const auto entity = scene.createEntity();
			idToEntity[id] = entity;

			// Name component
			if ( entityJson.contains( "name" ) )
			{
				const std::string name = entityJson["name"];
				scene.addComponent<components::Name>( entity, components::Name{ name } );
			}

			// Components
			if ( entityJson.contains( "components" ) )
			{
				const auto &components = entityJson["components"];

				// Transform
				if ( components.contains( "transform" ) )
				{
					components::Transform transform;
					deserializeTransform( components["transform"], transform );
					scene.addComponent<components::Transform>( entity, transform );
				}

				// Visible
				if ( components.contains( "visible" ) )
				{
					components::Visible visible;
					deserializeVisible( components["visible"], visible );
					scene.addComponent<components::Visible>( entity, visible );
				}

				// MeshRenderer
				if ( components.contains( "meshRenderer" ) )
				{
					components::MeshRenderer meshRenderer;
					deserializeMeshRenderer( components["meshRenderer"], meshRenderer );
					scene.addComponent<components::MeshRenderer>( entity, meshRenderer );
				}
			}
		}

		// Second pass: rebuild hierarchy
		for ( const auto &entityJson : sceneJson["entities"] )
		{
			const int id = entityJson["id"];
			const auto entity = idToEntity[id];

			if ( entityJson.contains( "parent" ) && !entityJson["parent"].is_null() )
			{
				const int parentID = entityJson["parent"];
				if ( idToEntity.find( parentID ) != idToEntity.end() )
				{
					const auto parent = idToEntity[parentID];
					scene.setParent( entity, parent );
				}
				else
				{
					// Parent ID not found - log warning but continue
					// (In production, might want to track this as a warning)
				}
			}
		}

		return {};
	}
	catch ( const json::exception &e )
	{
		return std::unexpected( makeError( SerializationError::InvalidJSON, std::string( "JSON error: " ) + e.what(), filepath ) );
	}
	catch ( const std::exception &e )
	{
		return std::unexpected( makeError( SerializationError::Unknown, std::string( "Unknown error: " ) + e.what(), filepath ) );
	}
}

std::expected<SceneMetadata, SerializationErrorInfo> SceneSerializer::getSceneMetadata(
	const std::filesystem::path &filepath )
{
	try
	{
		if ( !std::filesystem::exists( filepath ) )
		{
			return std::unexpected( makeError( SerializationError::FileNotFound, "Scene file not found", filepath ) );
		}

		std::ifstream file( filepath );
		if ( !file.is_open() )
		{
			return std::unexpected( makeError( SerializationError::FileAccessDenied, "Could not open file for reading", filepath ) );
		}

		json sceneJson;
		file >> sceneJson;

		SceneMetadata metadata;
		metadata.version = sceneJson.value( "version", "1.0" );

		if ( sceneJson.contains( "metadata" ) )
		{
			const auto &metaJson = sceneJson["metadata"];
			metadata.name = metaJson.value( "name", "Untitled Scene" );
			metadata.created = metaJson.value( "created", "" );
			metadata.modified = metaJson.value( "modified", "" );
			metadata.author = metaJson.value( "author", "Level Editor" );
		}

		return metadata;
	}
	catch ( const json::exception &e )
	{
		return std::unexpected( makeError( SerializationError::InvalidJSON, std::string( "JSON error: " ) + e.what(), filepath ) );
	}
	catch ( const std::exception &e )
	{
		return std::unexpected( makeError( SerializationError::Unknown, std::string( "Unknown error: " ) + e.what(), filepath ) );
	}
}

std::expected<void, SerializationErrorInfo> SceneSerializer::validateSceneFile(
	const std::filesystem::path &filepath )
{
	try
	{
		if ( !std::filesystem::exists( filepath ) )
		{
			return std::unexpected( makeError( SerializationError::FileNotFound, "Scene file not found", filepath ) );
		}

		std::ifstream file( filepath );
		if ( !file.is_open() )
		{
			return std::unexpected( makeError( SerializationError::FileAccessDenied, "Could not open file for reading", filepath ) );
		}

		json sceneJson;
		file >> sceneJson;

		// Validate required fields
		if ( !sceneJson.contains( "version" ) )
		{
			return std::unexpected( makeError( SerializationError::MissingRequiredField, "Missing version field", filepath ) );
		}

		if ( !sceneJson.contains( "entities" ) || !sceneJson["entities"].is_array() )
		{
			return std::unexpected( makeError( SerializationError::MissingRequiredField, "Missing or invalid entities array", filepath ) );
		}

		return {};
	}
	catch ( const json::parse_error &e )
	{
		return std::unexpected( makeError( SerializationError::InvalidJSON, std::string( "JSON parse error: " ) + e.what(), filepath, e.byte ) );
	}
	catch ( const json::exception &e )
	{
		return std::unexpected( makeError( SerializationError::InvalidJSON, std::string( "JSON error: " ) + e.what(), filepath ) );
	}
	catch ( const std::exception &e )
	{
		return std::unexpected( makeError( SerializationError::Unknown, std::string( "Unknown error: " ) + e.what(), filepath ) );
	}
}

} // namespace scene
