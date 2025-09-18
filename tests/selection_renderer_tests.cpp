#include <catch2/catch_test_macros.hpp>

import editor.selection_renderer;
import runtime.ecs;
import runtime.components;
import platform.dx12;
import engine.shader_manager;
import engine.math;
import engine.vec;
import engine.matrix;

namespace
{

class MockDevice : public dx12::Device
{
public:
	MockDevice() = default;

	// Don't use override since these might not be virtual
	bool initializeHeadless()
	{
		return true;
	}

	void shutdown() {}

	ID3D12Device *get() const
	{
		return nullptr; // Mock implementation
	}

	ID3D12Device *operator->() const
	{
		return nullptr;
	}

	dx12::CommandContext *getCommandContext() const
	{
	completion:
		return nullptr;
	}
};

class MockShaderManager : public shader_manager::ShaderManager
{
public:
	MockShaderManager() = default;

	// For testing, we'll just use the base class implementation
	// but avoid actual file loading by using empty paths or mock data
};

} // anonymous namespace

TEST_CASE( "SelectionStyle - Default values", "[selection-renderer][style]" )
{
	editor::SelectionStyle style;

	REQUIRE( style.selectedColor.x == 1.0f );
	REQUIRE( style.selectedColor.y == 0.6f );
	REQUIRE( style.selectedColor.z == 0.0f );
	REQUIRE( style.selectedColor.w == 1.0f );

	REQUIRE( style.primaryColor.x == 1.0f );
	REQUIRE( style.primaryColor.y == 1.0f );
	REQUIRE( style.primaryColor.z == 0.0f );
	REQUIRE( style.primaryColor.w == 1.0f );

	REQUIRE( style.outlineWidth == 2.0f );
	REQUIRE( style.animateSelection == true );
}

TEST_CASE( "SelectionRenderer - Construction", "[selection-renderer][construction]" )
{
	MockDevice device;
	MockShaderManager shaderManager;

	SECTION( "Basic construction succeeds" )
	{
		REQUIRE_NOTHROW( editor::SelectionRenderer{ device, shaderManager } );
	}

	SECTION( "Style accessors work" )
	{
		editor::SelectionRenderer renderer{ device, shaderManager };

		auto &style = renderer.getStyle();
		REQUIRE( style.selectedColor.x == 1.0f );

		// Modify style
		style.selectedColor = math::Vec4<>{ 0.0f, 1.0f, 0.0f, 1.0f }; // Green

		const auto &constStyle = renderer.getStyle();
		REQUIRE( constStyle.selectedColor.x == 0.0f );
		REQUIRE( constStyle.selectedColor.y == 1.0f );
	}
}

TEST_CASE( "SelectionRenderer - Render methods", "[selection-renderer][render]" )
{
	MockDevice device;
	MockShaderManager shaderManager;
	editor::SelectionRenderer renderer{ device, shaderManager };

	ecs::Scene scene;
	auto entity = scene.createEntity( "TestEntity" );
	scene.addComponent( entity, components::Transform{} );
	scene.addComponent( entity, components::Selected{} );

	// Mock matrices
	math::Mat4<> viewMatrix = math::Mat4<>::identity();
	math::Mat4<> projMatrix = math::Mat4<>::identity();

	SECTION( "Render methods accept null command list (headless mode)" )
	{
		REQUIRE_NOTHROW( renderer.render( scene, nullptr, viewMatrix, projMatrix ) );
		REQUIRE_NOTHROW( renderer.renderSelectionOutlines( scene, nullptr, viewMatrix, projMatrix ) );
		REQUIRE_NOTHROW( renderer.renderHoverHighlight( entity, scene, nullptr, viewMatrix, projMatrix ) );
	}

	SECTION( "Rectangle selection rendering" )
	{
		math::Vec2<> startPos{ 100.0f, 100.0f };
		math::Vec2<> endPos{ 200.0f, 200.0f };

		REQUIRE_NOTHROW( renderer.renderRectSelection( startPos, endPos, nullptr ) );
	}
}

TEST_CASE( "SelectionRenderer - Selected entity rendering", "[selection-renderer][selected]" )
{
	MockDevice device;
	MockShaderManager shaderManager;
	editor::SelectionRenderer renderer{ device, shaderManager };

	ecs::Scene scene;

	// Create selected entity
	auto selectedEntity = scene.createEntity( "SelectedEntity" );
	scene.addComponent( selectedEntity, components::Transform{} );
	scene.addComponent( selectedEntity, components::Selected{ false } ); // Secondary selection

	// Create primary selected entity
	auto primaryEntity = scene.createEntity( "PrimaryEntity" );
	scene.addComponent( primaryEntity, components::Transform{} );
	scene.addComponent( primaryEntity, components::Selected{ true } ); // Primary selection

	// Create non-selected entity
	auto normalEntity = scene.createEntity( "NormalEntity" );
	scene.addComponent( normalEntity, components::Transform{} );

	math::Mat4<> viewMatrix = math::Mat4<>::identity();
	math::Mat4<> projMatrix = math::Mat4<>::identity();

	SECTION( "Render handles multiple selected entities" )
	{
		REQUIRE_NOTHROW( renderer.render( scene, nullptr, viewMatrix, projMatrix ) );
	}

	SECTION( "Outline rendering handles selection states" )
	{
		REQUIRE_NOTHROW( renderer.renderSelectionOutlines( scene, nullptr, viewMatrix, projMatrix ) );
	}
}

TEST_CASE( "SelectionRenderer - Animation support", "[selection-renderer][animation]" )
{
	MockDevice device;
	MockShaderManager shaderManager;
	editor::SelectionRenderer renderer{ device, shaderManager };

	auto &style = renderer.getStyle();

	SECTION( "Animation can be disabled" )
	{
		style.animateSelection = false;
		REQUIRE( style.animateSelection == false );
	}

	SECTION( "Animation speed configurable" )
	{
		style.animationSpeed = 5.0f;
		REQUIRE( style.animationSpeed == 5.0f );
	}
}