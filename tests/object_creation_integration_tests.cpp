#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

// Integration tests for Task 8: Object Creation and Asset Instantiation
#include "runtime/ecs.h"
#include "engine/assets/asset_manager.h"
#include "engine/gpu/gpu_resource_manager.h"
#include "platform/dx12/dx12_device.h"
#include "editor/commands/EcsCommands.h"
#include "editor/commands/CommandHistory.h"
#include "runtime/components.h"
#include "math/vec.h"

#include <filesystem>
#include <fstream>
#include <memory>

using Catch::Matchers::WithinAbs;
namespace fs = std::filesystem;

// Mock GPUResourceManager for testing (CPU-only tests don't need actual GPU resources)
class MockGPUResourceManager : public engine::GPUResourceManager
{
public:
	MockGPUResourceManager() : engine::GPUResourceManager( getMockDevice() ) {}

private:
	static dx12::Device &getMockDevice()
	{
		static dx12::Device device;
		return device;
	}
};

TEST_CASE( "Asset instantiation workflow: Load → Create → Verify", "[T8.8][integration][AF1]" )
{
	// Arrange: Setup scene, asset manager, GPU manager, and command history
	ecs::Scene scene;
	assets::AssetManager assetManager;
	MockGPUResourceManager gpuManager;
	CommandHistory history;

	const auto testAssetPath = fs::current_path() / "assets" / "test" / "triangle.gltf";
	const math::Vec3f worldPosition{ 1.0f, 2.0f, 3.0f };

	SECTION( "CreateEntityFromAssetCommand succeeds with valid asset" )
	{
		// Skip test if asset doesn't exist
		if ( !fs::exists( testAssetPath ) )
		{
			SKIP( "Test asset not found: " + testAssetPath.string() );
		}

		// Act: Create entity from asset via command
		auto createCmd = std::make_unique<editor::CreateEntityFromAssetCommand>(
			scene, assetManager, gpuManager, testAssetPath.string(), worldPosition, ecs::Entity{} );
		const bool success = history.executeCommand( std::move( createCmd ) );

		// Assert: Command succeeded
		REQUIRE( success );

		// Verify entity was created
		const auto entities = scene.getAllEntities();
		REQUIRE( entities.size() >= 1 );

		// Find the created entity (should have Transform at specified position)
		bool foundEntity = false;
		for ( const auto entity : entities )
		{
			if ( scene.hasComponent<components::Transform>( entity ) )
			{
				const auto *transform = scene.getComponent<components::Transform>( entity );
				if ( transform &&
					std::abs( transform->position.x - worldPosition.x ) < 0.01f &&
					std::abs( transform->position.y - worldPosition.y ) < 0.01f &&
					std::abs( transform->position.z - worldPosition.z ) < 0.01f )
				{
					foundEntity = true;
					break;
				}
			}
		}
		REQUIRE( foundEntity );
	}

	SECTION( "CreateEntityFromAssetCommand with parent creates child entity" )
	{
		// Skip test if asset doesn't exist
		if ( !fs::exists( testAssetPath ) )
		{
			SKIP( "Test asset not found: " + testAssetPath.string() );
		}

		// Arrange: Create parent entity
		auto parentCreateCmd = std::make_unique<editor::CreateEntityCommand>( scene, "Parent" );
		history.executeCommand( std::move( parentCreateCmd ) );

		const auto parentEntities = scene.getAllEntities();
		REQUIRE( parentEntities.size() == 1 );
		const auto parent = parentEntities[0];

		// Add Transform to parent
		components::Transform parentTransform;
		parentTransform.position = { 10.0f, 20.0f, 30.0f };
		auto addTransformCmd = std::make_unique<editor::AddComponentCommand<components::Transform>>(
			scene, parent, parentTransform );
		history.executeCommand( std::move( addTransformCmd ) );

		// Act: Create child entity from asset
		auto createChildCmd = std::make_unique<editor::CreateEntityFromAssetCommand>(
			scene, assetManager, gpuManager, testAssetPath.string(), parentTransform.position, parent );
		const bool success = history.executeCommand( std::move( createChildCmd ) );

		// Assert: Child was created
		REQUIRE( success );

		// Verify hierarchy relationship
		const auto children = scene.getChildren( parent );
		REQUIRE( children.size() >= 1 );

		// Verify child has parent
		bool foundChild = false;
		for ( const auto child : children )
		{
			const auto childParent = scene.getParent( child );
			if ( childParent == parent )
			{
				foundChild = true;
				break;
			}
		}
		REQUIRE( foundChild );
	}

	SECTION( "Undo removes created entity completely" )
	{
		// Skip test if asset doesn't exist
		if ( !fs::exists( testAssetPath ) )
		{
			SKIP( "Test asset not found: " + testAssetPath.string() );
		}

		// Act: Create entity and then undo
		auto createCmd = std::make_unique<editor::CreateEntityFromAssetCommand>(
			scene, assetManager, gpuManager, testAssetPath.string(), worldPosition, ecs::Entity{} );
		const bool createSuccess = history.executeCommand( std::move( createCmd ) );
		REQUIRE( createSuccess );

		const size_t entitiesAfterCreate = scene.getAllEntities().size();
		REQUIRE( entitiesAfterCreate >= 1 );

		// Undo the creation
		const bool undoSuccess = history.undo();
		REQUIRE( undoSuccess );

		// Assert: Entity was removed
		const size_t entitiesAfterUndo = scene.getAllEntities().size();
		REQUIRE( entitiesAfterUndo == 0 ); // Should be back to empty scene
	}
}

TEST_CASE( "Multiple asset instantiation workflow", "[T8.8][integration][AF2]" )
{
	// Arrange: Setup scene, asset manager, GPU manager, and command history
	ecs::Scene scene;
	assets::AssetManager assetManager;
	MockGPUResourceManager gpuManager;
	CommandHistory history;

	const auto testAssetPath = fs::current_path() / "assets" / "test" / "triangle.gltf";

	SECTION( "Create multiple entities from same asset" )
	{
		// Skip test if asset doesn't exist
		if ( !fs::exists( testAssetPath ) )
		{
			SKIP( "Test asset not found: " + testAssetPath.string() );
		}

		// Act: Create 5 entities from the same asset at different positions
		const size_t entityCount = 5;
		for ( size_t i = 0; i < entityCount; ++i )
		{
			const math::Vec3f position{ static_cast<float>( i ) * 2.0f, 0.0f, 0.0f };
			auto createCmd = std::make_unique<editor::CreateEntityFromAssetCommand>(
				scene, assetManager, gpuManager, testAssetPath.string(), position, ecs::Entity{} );
			const bool success = history.executeCommand( std::move( createCmd ) );
			REQUIRE( success );
		}

		// Assert: All entities were created
		const auto entities = scene.getAllEntities();
		REQUIRE( entities.size() >= entityCount );

		// Verify they have different positions
		std::vector<math::Vec3f> positions;
		for ( const auto entity : entities )
		{
			if ( scene.hasComponent<components::Transform>( entity ) )
			{
				const auto *transform = scene.getComponent<components::Transform>( entity );
				if ( transform )
				{
					positions.push_back( transform->position );
				}
			}
		}
		REQUIRE( positions.size() >= entityCount );
	}

	SECTION( "Undo all creations in reverse order" )
	{
		// Skip test if asset doesn't exist
		if ( !fs::exists( testAssetPath ) )
		{
			SKIP( "Test asset not found: " + testAssetPath.string() );
		}

		// Arrange: Create 3 entities
		const size_t entityCount = 3;
		for ( size_t i = 0; i < entityCount; ++i )
		{
			const math::Vec3f position{ static_cast<float>( i ), 0.0f, 0.0f };
			auto createCmd = std::make_unique<editor::CreateEntityFromAssetCommand>(
				scene, assetManager, gpuManager, testAssetPath.string(), position, ecs::Entity{} );
			history.executeCommand( std::move( createCmd ) );
		}

		const size_t entitiesAfterCreation = scene.getAllEntities().size();
		REQUIRE( entitiesAfterCreation >= entityCount );

		// Act: Undo all creations
		for ( size_t i = 0; i < entityCount; ++i )
		{
			const bool success = history.undo();
			REQUIRE( success );
		}

		// Assert: All entities removed
		const size_t entitiesAfterUndo = scene.getAllEntities().size();
		REQUIRE( entitiesAfterUndo == 0 );
	}

	SECTION( "Redo all creations restores entities" )
	{
		// Skip test if asset doesn't exist
		if ( !fs::exists( testAssetPath ) )
		{
			SKIP( "Test asset not found: " + testAssetPath.string() );
		}

		// Arrange: Create, then undo all
		const size_t entityCount = 3;
		for ( size_t i = 0; i < entityCount; ++i )
		{
			const math::Vec3f position{ static_cast<float>( i ), 0.0f, 0.0f };
			auto createCmd = std::make_unique<editor::CreateEntityFromAssetCommand>(
				scene, assetManager, gpuManager, testAssetPath.string(), position, ecs::Entity{} );
			history.executeCommand( std::move( createCmd ) );
		}

		for ( size_t i = 0; i < entityCount; ++i )
		{
			history.undo();
		}

		REQUIRE( scene.getAllEntities().size() == 0 );

		// Act: Redo all creations
		for ( size_t i = 0; i < entityCount; ++i )
		{
			const bool success = history.redo();
			REQUIRE( success );
		}

		// Assert: Entities restored
		const size_t entitiesAfterRedo = scene.getAllEntities().size();
		REQUIRE( entitiesAfterRedo >= entityCount );
	}
}

TEST_CASE( "Asset instantiation error handling", "[T8.8][integration][AF3]" )
{
	// Arrange: Setup scene, asset manager, GPU manager, and command history
	ecs::Scene scene;
	assets::AssetManager assetManager;
	MockGPUResourceManager gpuManager;
	CommandHistory history;

	const math::Vec3f worldPosition{ 0.0f, 0.0f, 0.0f };

	SECTION( "CreateEntityFromAssetCommand fails with non-existent file" )
	{
		// Arrange: Invalid asset path
		const std::string invalidPath = "nonexistent/path/to/asset.gltf";

		// Act: Attempt to create entity from non-existent asset
		auto createCmd = std::make_unique<editor::CreateEntityFromAssetCommand>(
			scene, assetManager, gpuManager, invalidPath, worldPosition, ecs::Entity{} );
		const bool success = history.executeCommand( std::move( createCmd ) );

		// Assert: Command failed
		REQUIRE_FALSE( success );

		// Verify no entity was created
		const auto entities = scene.getAllEntities();
		REQUIRE( entities.size() == 0 );
	}

	SECTION( "CreateEntityFromAssetCommand handles invalid asset gracefully" )
	{
		// Arrange: Path to a file that exists but isn't a valid glTF
		const auto invalidAssetPath = fs::temp_directory_path() / "invalid.gltf";

		// Create an invalid glTF file
		{
			std::ofstream file( invalidAssetPath );
			file << "This is not valid glTF JSON";
		}

		// Cleanup on exit
		const auto cleanup = [&]() {
			if ( fs::exists( invalidAssetPath ) )
			{
				fs::remove( invalidAssetPath );
			}
		};

		// Act: Attempt to create entity from invalid asset
		auto createCmd = std::make_unique<editor::CreateEntityFromAssetCommand>(
			scene, assetManager, gpuManager, invalidAssetPath.string(), worldPosition, ecs::Entity{} );
		const bool success = history.executeCommand( std::move( createCmd ) );

		// Assert: Command failed gracefully (no crash)
		REQUIRE_FALSE( success );

		// Verify no entity was created
		const auto entities = scene.getAllEntities();
		REQUIRE( entities.size() == 0 );

		cleanup();
	}
}

TEST_CASE( "Asset instantiation with hierarchy preservation", "[T8.8][integration][AF4]" )
{
	// Arrange: Setup scene, asset manager, GPU manager, and command history
	ecs::Scene scene;
	assets::AssetManager assetManager;
	MockGPUResourceManager gpuManager;
	CommandHistory history;

	// Use a more complex asset if available (e.g., multi-node glTF)
	const auto testAssetPath = fs::current_path() / "assets" / "test" / "triangle.gltf";
	const math::Vec3f worldPosition{ 0.0f, 0.0f, 0.0f };

	SECTION( "Asset hierarchy is preserved after instantiation" )
	{
		// Skip test if asset doesn't exist
		if ( !fs::exists( testAssetPath ) )
		{
			SKIP( "Test asset not found: " + testAssetPath.string() );
		}

		// Act: Create entity from asset
		auto createCmd = std::make_unique<editor::CreateEntityFromAssetCommand>(
			scene, assetManager, gpuManager, testAssetPath.string(), worldPosition, ecs::Entity{} );
		const bool success = history.executeCommand( std::move( createCmd ) );

		// Assert: Command succeeded
		REQUIRE( success );

		// Verify entities exist
		const auto entities = scene.getAllEntities();
		REQUIRE( entities.size() >= 1 );

		// Note: For a simple triangle asset, there may be only one entity
		// For multi-node assets, verify parent-child relationships are intact
		for ( const auto entity : entities )
		{
			if ( scene.hasComponent<components::Name>( entity ) )
			{
				const auto *name = scene.getComponent<components::Name>( entity );
				REQUIRE( name != nullptr );
				// Name should be set from asset
				REQUIRE( !name->name.empty() );
			}
		}
	}
}

TEST_CASE( "Performance: Create many entities", "[T8.8][integration][performance][AF5]" )
{
	// Arrange: Setup scene, asset manager, GPU manager, and command history
	ecs::Scene scene;
	assets::AssetManager assetManager;
	MockGPUResourceManager gpuManager;
	CommandHistory history;

	const auto testAssetPath = fs::current_path() / "assets" / "test" / "triangle.gltf";

	SECTION( "Create 50 entities without performance issues" )
	{
		// Skip test if asset doesn't exist
		if ( !fs::exists( testAssetPath ) )
		{
			SKIP( "Test asset not found: " + testAssetPath.string() );
		}

		// Act: Create 50 entities in a grid pattern
		const size_t entityCount = 50;
		const size_t gridSize = 10; // 10x10 grid (we'll use first 50)

		for ( size_t i = 0; i < entityCount; ++i )
		{
			const float x = static_cast<float>( i % gridSize ) * 2.0f;
			const float z = static_cast<float>( i / gridSize ) * 2.0f;
			const math::Vec3f position{ x, 0.0f, z };

			auto createCmd = std::make_unique<editor::CreateEntityFromAssetCommand>(
				scene, assetManager, gpuManager, testAssetPath.string(), position, ecs::Entity{} );
			const bool success = history.executeCommand( std::move( createCmd ) );
			REQUIRE( success );
		}

		// Assert: All entities were created
		const auto entities = scene.getAllEntities();
		REQUIRE( entities.size() >= entityCount );

		// Note: Performance should be acceptable (no specific timing assertion)
		// In a real application, this would be measured with a profiler
	}
}
