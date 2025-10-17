#include "catch2/catch_all.hpp"
#include <fstream>

#include "graphics/shader_manager/shader_manager.h"
#include "graphics/immediate_renderer/immediate_renderer.h"

using shader_manager::INVALID_SHADER_HANDLE;
using shader_manager::ShaderInfo;

class IncludeDependencyTestFixture
{
public:
	IncludeDependencyTestFixture()
	{
		// Create temporary directory for test files
		test_dir = std::filesystem::temp_directory_path() / "shader_include_tests";
		std::filesystem::create_directories( test_dir );

		// Create test include file
		include_file = test_dir / "common.hlsl";
		std::ofstream include_stream( include_file );
		include_stream << R"(
float4 CommonFunction(float4 input) {
    return input * 2.0f;
}
)";
		include_stream.close();

		// Create main shader file that includes the above
		main_shader_file = test_dir / "main_shader.hlsl";
		std::ofstream main_stream( main_shader_file );
		main_stream << R"(
#include "common.hlsl"

struct VSInput {
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    float4 pos = float4(input.position, 1.0f);
    output.position = CommonFunction(pos);  // Use function from include
    output.texCoord = input.texCoord;
    return output;
}
)";
		main_stream.close();
	}

	~IncludeDependencyTestFixture()
	{
		// Clean up test files
		std::filesystem::remove_all( test_dir );
	}

	std::filesystem::path test_dir;
	std::filesystem::path include_file;
	std::filesystem::path main_shader_file;
};

TEST_CASE_METHOD( IncludeDependencyTestFixture, "Shader Include Dependency Tracking", "[shader_manager][include_dependencies]" )
{
	shader_manager::ShaderManager shader_manager;

	SECTION( "Shader with includes compiles and tracks dependencies" )
	{
		// Compile the main shader that includes common.hlsl
		const auto handle = shader_manager.registerShader( main_shader_file.string(), "VSMain", "vs_5_0", shader_manager::ShaderType::Vertex );

		REQUIRE( handle != INVALID_SHADER_HANDLE );

		// Get the shader info to check if includes were tracked
		const ShaderInfo *shader_info = shader_manager.getShaderInfo( handle );
		REQUIRE( shader_info != nullptr );

		// Verify that the include file was detected and tracked
		bool found_include = false;
		for ( const auto &included_file : shader_info->includedFiles )
		{
			if ( std::filesystem::equivalent( included_file, include_file ) )
			{
				found_include = true;
				break;
			}
		}

		REQUIRE( found_include );
		REQUIRE( shader_info->includedFiles.size() >= 1 );
		REQUIRE( shader_info->includedFilesModTimes.size() == shader_info->includedFiles.size() );
	}

	SECTION( "Modifying included file triggers recompilation" )
	{
		// First, compile the shader
		const auto handle = shader_manager.registerShader( main_shader_file.string(), "VSMain", "vs_5_0", shader_manager::ShaderType::Vertex );
		REQUIRE( handle != INVALID_SHADER_HANDLE );

		// Get initial modification times
		const ShaderInfo *initial_info = shader_manager.getShaderInfo( handle );
		REQUIRE( initial_info != nullptr );
		const auto initial_main_mod_time = initial_info->lastModified;
		const auto initial_include_mod_times = initial_info->includedFilesModTimes;

		// Wait a bit to ensure file system timestamp resolution
		std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

		// Modify the include file
		std::ofstream include_stream( include_file );
		include_stream << R"(
// Modified version with comment
float4 CommonFunction(float4 input) {
    return input * 3.0f;  // Changed multiplier
}
)";
		include_stream.close();

		// Wait a bit more
		std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

		// Update the shader manager - should detect the change
		shader_manager.update();

		// Verify that the shader was recompiled
		const auto *updated_info = shader_manager.getShaderInfo( handle );

		// The main shader's compilation time should have been updated due to include change
		// (Note: this depends on the implementation details of how recompilation affects timestamps)
		bool recompilation_detected = false;

		// Check if any include file modification time changed
		for ( size_t i = 0; i < updated_info->includedFilesModTimes.size(); ++i )
		{
			if ( i < initial_include_mod_times.size() )
			{
				if ( updated_info->includedFilesModTimes[i] != initial_include_mod_times[i] )
				{
					recompilation_detected = true;
					break;
				}
			}
		}

		REQUIRE( recompilation_detected );
	}

	SECTION( "Multiple includes are tracked correctly" )
	{
		// Create a second include file
		const auto second_include = test_dir / "utils.hlsl";
		std::ofstream utils_stream( second_include );
		utils_stream << R"(
float4 UtilityFunction(float4 input) {
    return normalize(input);
}
)";
		utils_stream.close();

		// Create a shader that includes both files
		const auto multi_include_shader = test_dir / "multi_include.hlsl";
		std::ofstream multi_stream( multi_include_shader );
		multi_stream << R"(
#include "common.hlsl"
#include "utils.hlsl"

struct VSInput {
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    float4 pos = float4(input.position, 1.0f);
    pos = CommonFunction(pos);
    output.position = UtilityFunction(pos);
    output.texCoord = input.texCoord;
    return output;
}
)";
		multi_stream.close();

		// Compile the shader
		const auto handle = shader_manager.registerShader( multi_include_shader.string(), "VSMain", "vs_5_0", shader_manager::ShaderType::Vertex );
		REQUIRE( handle != INVALID_SHADER_HANDLE );

		const auto *shader_info = shader_manager.getShaderInfo( handle );

		// Should have tracked both include files
		REQUIRE( shader_info->includedFiles.size() >= 2 );

		// Verify both includes are present
		bool found_common = false;
		bool found_utils = false;

		for ( const auto &included_file : shader_info->includedFiles )
		{
			if ( std::filesystem::equivalent( included_file, include_file ) )
			{
				found_common = true;
			}
			if ( std::filesystem::equivalent( included_file, second_include ) )
			{
				found_utils = true;
			}
		}

		REQUIRE( found_common );
		REQUIRE( found_utils );
		REQUIRE( shader_info->includedFilesModTimes.size() == shader_info->includedFiles.size() );

		// Clean up
		std::filesystem::remove( second_include );
		std::filesystem::remove( multi_include_shader );
	}
}

TEST_CASE_METHOD( IncludeDependencyTestFixture, "Include Path Resolution", "[shader_manager][include_paths]" )
{
	shader_manager::ShaderManager shader_manager;

	SECTION( "Relative include paths are resolved correctly" )
	{
		// Create a subdirectory with an include
		const auto sub_dir = test_dir / "includes";
		std::filesystem::create_directories( sub_dir );

		const auto sub_include = sub_dir / "sub_common.hlsl";
		std::ofstream sub_stream( sub_include );
		sub_stream << R"(
float4 SubFunction(float4 input) {
    return input * 0.5f;
}
)";
		sub_stream.close();

		// Create shader that includes from subdirectory
		const auto relative_shader = test_dir / "relative_include.hlsl";
		std::ofstream rel_stream( relative_shader );
		rel_stream << R"(
#include "includes/sub_common.hlsl"

struct VSInput {
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    float4 pos = float4(input.position, 1.0f);
    output.position = SubFunction(pos);
    output.texCoord = input.texCoord;
    return output;
}
)";
		rel_stream.close();

		// Compile the shader
		const auto handle = shader_manager.registerShader( relative_shader.string(), "VSMain", "vs_5_0", shader_manager::ShaderType::Vertex );
		REQUIRE( handle != INVALID_SHADER_HANDLE );

		const auto *shader_info = shader_manager.getShaderInfo( handle );

		// Should have tracked the include file with resolved path
		bool found_sub_include = false;
		for ( const auto &included_file : shader_info->includedFiles )
		{
			if ( std::filesystem::equivalent( included_file, sub_include ) )
			{
				found_sub_include = true;
				break;
			}
		}

		REQUIRE( found_sub_include );

		// Clean up
		std::filesystem::remove_all( sub_dir );
		std::filesystem::remove( relative_shader );
	}
}
