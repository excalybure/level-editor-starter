#include <catch2/catch_test_macros.hpp>

#include "engine/assets/assets.h"
#include "math/vec.h"

TEST_CASE( "Primitive can store and retrieve MaterialInstance", "[Primitive][MaterialInstance][unit]" )
{
	// Arrange
	assets::Primitive primitive;
	assets::MaterialInstance instance;
	instance.baseMaterial = 5;
	instance.baseColorFactorOverride = math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f };
	instance.metallicFactorOverride = 0.8f;

	// Act
	primitive.setMaterialInstance( instance );
	const auto &retrievedInstance = primitive.getMaterialInstance();

	// Assert
	REQUIRE( retrievedInstance.baseMaterial == 5 );
	REQUIRE( retrievedInstance.baseColorFactorOverride.has_value() );
	REQUIRE( retrievedInstance.baseColorFactorOverride.value() == math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f } );
	REQUIRE( retrievedInstance.metallicFactorOverride.has_value() );
	REQUIRE( retrievedInstance.metallicFactorOverride.value() == 0.8f );
}

TEST_CASE( "Primitive reports hasOverrides when MaterialInstance has overrides", "[Primitive][MaterialInstance][unit]" )
{
	// Arrange
	assets::Primitive primitive;
	assets::MaterialInstance instance;
	instance.baseMaterial = 0;
	instance.baseColorFactorOverride = math::Vec4f{ 1.0f, 0.5f, 0.2f, 1.0f };

	// Act
	primitive.setMaterialInstance( instance );

	// Assert
	REQUIRE( primitive.hasOverrides() );
}

TEST_CASE( "Primitive hasOverrides returns false for instance without overrides", "[Primitive][MaterialInstance][unit]" )
{
	// Arrange
	assets::Primitive primitive;
	assets::MaterialInstance instance;
	instance.baseMaterial = 0;
	// No overrides set

	// Act
	primitive.setMaterialInstance( instance );

	// Assert
	REQUIRE( !primitive.hasOverrides() );
}

TEST_CASE( "Primitive with default MaterialInstance has no overrides", "[Primitive][MaterialInstance][unit]" )
{
	// Arrange & Act
	assets::Primitive primitive;

	// Assert
	REQUIRE( !primitive.hasOverrides() );
	REQUIRE( primitive.getMaterialInstance().baseMaterial == assets::INVALID_MATERIAL_HANDLE );
}

TEST_CASE( "Primitive MaterialInstance persists through copy", "[Primitive][MaterialInstance][unit]" )
{
	// Arrange
	assets::Primitive original;
	assets::MaterialInstance instance;
	instance.baseMaterial = 3;
	instance.roughnessFactorOverride = 0.4f;
	instance.emissiveFactorOverride = math::Vec3f{ 0.2f, 0.2f, 0.2f };
	original.setMaterialInstance( instance );

	// Act
	assets::Primitive copied = original;

	// Assert
	REQUIRE( copied.getMaterialInstance().baseMaterial == 3 );
	REQUIRE( copied.getMaterialInstance().roughnessFactorOverride.has_value() );
	REQUIRE( copied.getMaterialInstance().roughnessFactorOverride.value() == 0.4f );
	REQUIRE( copied.getMaterialInstance().emissiveFactorOverride.has_value() );
	REQUIRE( copied.getMaterialInstance().emissiveFactorOverride.value() == math::Vec3f{ 0.2f, 0.2f, 0.2f } );
}

TEST_CASE( "Primitive MaterialInstance persists through move", "[Primitive][MaterialInstance][unit]" )
{
	// Arrange
	assets::Primitive original;
	assets::MaterialInstance instance;
	instance.baseMaterial = 7;
	instance.baseColorFactorOverride = math::Vec4f{ 0.5f, 0.5f, 0.5f, 1.0f };
	original.setMaterialInstance( instance );

	// Act
	assets::Primitive moved = std::move( original );

	// Assert
	REQUIRE( moved.getMaterialInstance().baseMaterial == 7 );
	REQUIRE( moved.getMaterialInstance().baseColorFactorOverride.has_value() );
	REQUIRE( moved.getMaterialInstance().baseColorFactorOverride.value() == math::Vec4f{ 0.5f, 0.5f, 0.5f, 1.0f } );
}

TEST_CASE( "Primitive can update MaterialInstance multiple times", "[Primitive][MaterialInstance][unit]" )
{
	// Arrange
	assets::Primitive primitive;

	// Act - First update
	assets::MaterialInstance instance1;
	instance1.baseMaterial = 1;
	instance1.metallicFactorOverride = 0.5f;
	primitive.setMaterialInstance( instance1 );
	REQUIRE( primitive.getMaterialInstance().baseMaterial == 1 );

	// Act - Second update
	assets::MaterialInstance instance2;
	instance2.baseMaterial = 2;
	instance2.roughnessFactorOverride = 0.3f;
	primitive.setMaterialInstance( instance2 );

	// Assert
	REQUIRE( primitive.getMaterialInstance().baseMaterial == 2 );
	REQUIRE( !primitive.getMaterialInstance().metallicFactorOverride.has_value() ); // Cleared in new instance
	REQUIRE( primitive.getMaterialInstance().roughnessFactorOverride.has_value() );
	REQUIRE( primitive.getMaterialInstance().roughnessFactorOverride.value() == 0.3f );
}

TEST_CASE( "Primitive preserves material handle and MaterialInstance independently", "[Primitive][MaterialInstance][unit]" )
{
	// Arrange
	assets::Primitive primitive;
	primitive.setMaterialHandle( 42 );

	assets::MaterialInstance instance;
	instance.baseMaterial = 5;
	instance.baseColorFactorOverride = math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f };

	// Act
	primitive.setMaterialInstance( instance );

	// Assert
	REQUIRE( primitive.getMaterialHandle() == 42 );				  // Material handle unchanged
	REQUIRE( primitive.getMaterialInstance().baseMaterial == 5 ); // Instance's base material is different
	REQUIRE( primitive.getMaterialInstance().baseColorFactorOverride.has_value() );
}

TEST_CASE( "Primitive MaterialInstance with all texture overrides", "[Primitive][MaterialInstance][unit]" )
{
	// Arrange
	assets::Primitive primitive;
	assets::MaterialInstance instance;
	instance.baseMaterial = 0;
	instance.baseColorTextureOverride = "textures/custom_color.png";
	instance.metallicRoughnessTextureOverride = "textures/custom_mr.png";
	instance.normalTextureOverride = "textures/custom_normal.png";
	instance.emissiveTextureOverride = "textures/custom_emissive.png";

	// Act
	primitive.setMaterialInstance( instance );
	const auto &retrieved = primitive.getMaterialInstance();

	// Assert
	REQUIRE( retrieved.baseColorTextureOverride.has_value() );
	REQUIRE( retrieved.baseColorTextureOverride.value() == "textures/custom_color.png" );
	REQUIRE( retrieved.metallicRoughnessTextureOverride.has_value() );
	REQUIRE( retrieved.normalTextureOverride.has_value() );
	REQUIRE( retrieved.emissiveTextureOverride.has_value() );
	REQUIRE( primitive.hasOverrides() );
}

TEST_CASE( "Primitive MaterialInstance with mixed value and texture overrides", "[Primitive][MaterialInstance][unit]" )
{
	// Arrange
	assets::Primitive primitive;
	assets::MaterialInstance instance;
	instance.baseMaterial = 2;
	instance.baseColorFactorOverride = math::Vec4f{ 1.0f, 0.5f, 0.2f, 1.0f };
	instance.metallicFactorOverride = 0.7f;
	instance.baseColorTextureOverride = "textures/override_color.png";
	instance.normalTextureOverride = "textures/override_normal.png";

	// Act
	primitive.setMaterialInstance( instance );
	const auto &retrieved = primitive.getMaterialInstance();

	// Assert - Value overrides
	REQUIRE( retrieved.baseColorFactorOverride.has_value() );
	REQUIRE( retrieved.metallicFactorOverride.has_value() );
	REQUIRE( !retrieved.roughnessFactorOverride.has_value() ); // Not set
	REQUIRE( !retrieved.emissiveFactorOverride.has_value() );  // Not set

	// Assert - Texture overrides
	REQUIRE( retrieved.baseColorTextureOverride.has_value() );
	REQUIRE( !retrieved.metallicRoughnessTextureOverride.has_value() ); // Not set
	REQUIRE( retrieved.normalTextureOverride.has_value() );
	REQUIRE( !retrieved.emissiveTextureOverride.has_value() ); // Not set

	REQUIRE( primitive.hasOverrides() );
}

TEST_CASE( "Primitive MaterialInstance clearOverrides affects retrieved instance", "[Primitive][MaterialInstance][unit]" )
{
	// Arrange
	assets::Primitive primitive;
	assets::MaterialInstance instance;
	instance.baseMaterial = 0;
	instance.baseColorFactorOverride = math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f };
	instance.metallicFactorOverride = 0.8f;
	primitive.setMaterialInstance( instance );
	REQUIRE( primitive.hasOverrides() );

	// Act
	assets::MaterialInstance clearedInstance = primitive.getMaterialInstance();
	clearedInstance.clearOverrides();
	primitive.setMaterialInstance( clearedInstance );

	// Assert
	REQUIRE( !primitive.hasOverrides() );
	REQUIRE( !primitive.getMaterialInstance().baseColorFactorOverride.has_value() );
	REQUIRE( !primitive.getMaterialInstance().metallicFactorOverride.has_value() );
}

TEST_CASE( "Multiple primitives can have independent MaterialInstances", "[Primitive][MaterialInstance][unit]" )
{
	// Arrange
	assets::Primitive prim1, prim2, prim3;

	assets::MaterialInstance inst1;
	inst1.baseMaterial = 1;
	inst1.baseColorFactorOverride = math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f };

	assets::MaterialInstance inst2;
	inst2.baseMaterial = 2;
	inst2.metallicFactorOverride = 0.5f;

	assets::MaterialInstance inst3;
	inst3.baseMaterial = 3;
	// No overrides

	// Act
	prim1.setMaterialInstance( inst1 );
	prim2.setMaterialInstance( inst2 );
	prim3.setMaterialInstance( inst3 );

	// Assert
	REQUIRE( prim1.getMaterialInstance().baseMaterial == 1 );
	REQUIRE( prim1.getMaterialInstance().baseColorFactorOverride.has_value() );
	REQUIRE( prim1.hasOverrides() );

	REQUIRE( prim2.getMaterialInstance().baseMaterial == 2 );
	REQUIRE( prim2.getMaterialInstance().metallicFactorOverride.has_value() );
	REQUIRE( prim2.hasOverrides() );

	REQUIRE( prim3.getMaterialInstance().baseMaterial == 3 );
	REQUIRE( !prim3.hasOverrides() );
}
