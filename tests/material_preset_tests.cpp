#include <catch2/catch_test_macros.hpp>
#include "engine/assets/assets.h"
#include "editor/material_presets/MaterialPresetManager.h"
#include <filesystem>

TEST_CASE( "MaterialPreset can be created from MaterialInstance", "[preset][unit]" )
{
	// Arrange
	assets::MaterialInstance instance;
	instance.baseColorFactorOverride = math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f };
	instance.metallicFactorOverride = 0.8f;
	instance.roughnessFactorOverride = 0.2f;

	// Act
	const auto preset = assets::MaterialPreset::fromMaterialInstance( "Red Metal", instance );

	// Assert
	REQUIRE( preset.name == "Red Metal" );
	REQUIRE( preset.baseColorFactorOverride.has_value() );
	REQUIRE( preset.baseColorFactorOverride.value() == math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f } );
	REQUIRE( preset.metallicFactorOverride.has_value() );
	REQUIRE( preset.metallicFactorOverride.value() == 0.8f );
	REQUIRE( preset.roughnessFactorOverride.has_value() );
	REQUIRE( preset.roughnessFactorOverride.value() == 0.2f );
}

TEST_CASE( "MaterialPreset can be applied to MaterialInstance", "[preset][unit]" )
{
	// Arrange
	assets::MaterialPreset preset;
	preset.name = "Blue Glossy";
	preset.baseColorFactorOverride = math::Vec4f{ 0.0f, 0.0f, 1.0f, 1.0f };
	preset.metallicFactorOverride = 0.5f;
	preset.roughnessFactorOverride = 0.1f;

	assets::MaterialInstance instance;

	// Act
	preset.applyTo( instance );

	// Assert
	REQUIRE( instance.baseColorFactorOverride.has_value() );
	REQUIRE( instance.baseColorFactorOverride.value() == math::Vec4f{ 0.0f, 0.0f, 1.0f, 1.0f } );
	REQUIRE( instance.metallicFactorOverride.has_value() );
	REQUIRE( instance.metallicFactorOverride.value() == 0.5f );
	REQUIRE( instance.roughnessFactorOverride.has_value() );
	REQUIRE( instance.roughnessFactorOverride.value() == 0.1f );
}

TEST_CASE( "MaterialPreset partially overrides MaterialInstance", "[preset][unit]" )
{
	// Arrange - preset only overrides baseColor
	assets::MaterialPreset preset;
	preset.name = "Just Red";
	preset.baseColorFactorOverride = math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f };

	// Instance already has metallic set
	assets::MaterialInstance instance;
	instance.metallicFactorOverride = 0.9f;

	// Act - apply preset
	preset.applyTo( instance );

	// Assert - baseColor changed, metallic preserved
	REQUIRE( instance.baseColorFactorOverride.has_value() );
	REQUIRE( instance.baseColorFactorOverride.value() == math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f } );
	REQUIRE( instance.metallicFactorOverride.has_value() );
	REQUIRE( instance.metallicFactorOverride.value() == 0.9f );
}

TEST_CASE( "MaterialPreset hasOverrides works correctly", "[preset][unit]" )
{
	assets::MaterialPreset emptyPreset;
	emptyPreset.name = "Empty";
	REQUIRE_FALSE( emptyPreset.hasOverrides() );

	assets::MaterialPreset withOverrides;
	withOverrides.name = "With Color";
	withOverrides.baseColorFactorOverride = math::Vec4f{ 1.0f, 1.0f, 1.0f, 1.0f };
	REQUIRE( withOverrides.hasOverrides() );
}

TEST_CASE( "MaterialPresetManager can add and retrieve presets", "[preset][unit]" )
{
	// Arrange
	editor::MaterialPresetManager manager;
	assets::MaterialPreset preset;
	preset.name = "Gold";
	preset.baseColorFactorOverride = math::Vec4f{ 1.0f, 0.84f, 0.0f, 1.0f };
	preset.metallicFactorOverride = 1.0f;

	// Act
	const bool added = manager.addPreset( preset );

	// Assert
	REQUIRE( added );
	REQUIRE( manager.hasPreset( "Gold" ) );

	const auto retrieved = manager.getPreset( "Gold" );
	REQUIRE( retrieved.has_value() );
	REQUIRE( retrieved.value().name == "Gold" );
	REQUIRE( retrieved.value().baseColorFactorOverride.value() == math::Vec4f{ 1.0f, 0.84f, 0.0f, 1.0f } );
}

TEST_CASE( "MaterialPresetManager prevents duplicate names", "[preset][unit]" )
{
	// Arrange
	editor::MaterialPresetManager manager;
	assets::MaterialPreset preset1;
	preset1.name = "Copper";
	preset1.metallicFactorOverride = 1.0f;

	assets::MaterialPreset preset2;
	preset2.name = "Copper"; // Same name
	preset2.metallicFactorOverride = 0.5f;

	// Act
	const bool added1 = manager.addPreset( preset1 );
	const bool added2 = manager.addPreset( preset2 );

	// Assert
	REQUIRE( added1 );
	REQUIRE_FALSE( added2 );

	// First preset should still be there
	const auto retrieved = manager.getPreset( "Copper" );
	REQUIRE( retrieved.has_value() );
	REQUIRE( retrieved.value().metallicFactorOverride.value() == 1.0f );
}

TEST_CASE( "MaterialPresetManager can remove presets", "[preset][unit]" )
{
	// Arrange
	editor::MaterialPresetManager manager;
	assets::MaterialPreset preset;
	preset.name = "Test";
	manager.addPreset( preset );

	// Act
	const bool removed = manager.removePreset( "Test" );

	// Assert
	REQUIRE( removed );
	REQUIRE_FALSE( manager.hasPreset( "Test" ) );
}

TEST_CASE( "MaterialPresetManager can save and load from file", "[preset][unit]" )
{
	// Arrange
	const std::string testFile = "test_presets.json";

	// Clean up any existing test file
	if ( std::filesystem::exists( testFile ) )
	{
		std::filesystem::remove( testFile );
	}

	editor::MaterialPresetManager manager;

	assets::MaterialPreset preset1;
	preset1.name = "Red Metal";
	preset1.baseColorFactorOverride = math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f };
	preset1.metallicFactorOverride = 1.0f;
	preset1.roughnessFactorOverride = 0.3f;

	assets::MaterialPreset preset2;
	preset2.name = "Blue Glossy";
	preset2.baseColorFactorOverride = math::Vec4f{ 0.0f, 0.0f, 1.0f, 1.0f };
	preset2.roughnessFactorOverride = 0.1f;

	manager.addPreset( preset1 );
	manager.addPreset( preset2 );

	// Act - save
	const bool saved = manager.saveToFile( testFile );
	REQUIRE( saved );

	// Act - load into new manager
	editor::MaterialPresetManager loadedManager;
	const bool loaded = loadedManager.loadFromFile( testFile );

	// Assert
	REQUIRE( loaded );
	REQUIRE( loadedManager.hasPreset( "Red Metal" ) );
	REQUIRE( loadedManager.hasPreset( "Blue Glossy" ) );

	const auto loadedPreset1 = loadedManager.getPreset( "Red Metal" );
	REQUIRE( loadedPreset1.has_value() );
	REQUIRE( loadedPreset1.value().baseColorFactorOverride.value() == math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f } );
	REQUIRE( loadedPreset1.value().metallicFactorOverride.value() == 1.0f );
	REQUIRE( loadedPreset1.value().roughnessFactorOverride.value() == 0.3f );

	const auto loadedPreset2 = loadedManager.getPreset( "Blue Glossy" );
	REQUIRE( loadedPreset2.has_value() );
	REQUIRE( loadedPreset2.value().baseColorFactorOverride.value() == math::Vec4f{ 0.0f, 0.0f, 1.0f, 1.0f } );
	REQUIRE( loadedPreset2.value().roughnessFactorOverride.value() == 0.1f );
	REQUIRE_FALSE( loadedPreset2.value().metallicFactorOverride.has_value() ); // Should not have metallic

	// Cleanup
	std::filesystem::remove( testFile );
}

TEST_CASE( "MaterialPresetManager getPresetNames returns all names", "[preset][unit]" )
{
	// Arrange
	editor::MaterialPresetManager manager;

	assets::MaterialPreset preset1;
	preset1.name = "A";
	assets::MaterialPreset preset2;
	preset2.name = "B";
	assets::MaterialPreset preset3;
	preset3.name = "C";

	manager.addPreset( preset1 );
	manager.addPreset( preset2 );
	manager.addPreset( preset3 );

	// Act
	const auto names = manager.getPresetNames();

	// Assert
	REQUIRE( names.size() == 3 );
	REQUIRE( names[0] == "A" );
	REQUIRE( names[1] == "B" );
	REQUIRE( names[2] == "C" );
}
