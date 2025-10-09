#pragma once

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string_view>
#include <system_error>

namespace test::shader
{

inline const std::filesystem::path &tempDirectory()
{
	static const std::filesystem::path directory = [] {
		auto dir = std::filesystem::temp_directory_path() / "shader_compiler_tests";
		std::filesystem::create_directories( dir );
		return dir;
	}();
	return directory;
}

inline std::string generateUniqueFilename( std::string_view prefix = "shader-" )
{
	static std::atomic<uint32_t> counter{ 0 };
	const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
	const auto count = counter.fetch_add( 1, std::memory_order_relaxed );

	std::ostringstream oss;
	oss << prefix << timestamp << "-" << count;
	return oss.str();
}

class TempShaderFile
{
public:
	explicit TempShaderFile( std::string_view content, std::string_view extension = ".hlsl", const std::filesystem::path &directory = tempDirectory() )
		: m_path( directory / generateUniqueFilename() )
	{
		m_path.replace_extension( extension );
		std::ofstream file( m_path, std::ios::binary );
		REQUIRE( file.is_open() );
		file << content;
	}

	TempShaderFile( const TempShaderFile & ) = delete;
	TempShaderFile &operator=( const TempShaderFile & ) = delete;

	TempShaderFile( TempShaderFile && ) = delete;
	TempShaderFile &operator=( TempShaderFile && ) = delete;

	~TempShaderFile()
	{
		std::error_code ec;
		std::filesystem::remove( m_path, ec );
	}

	const std::filesystem::path &path() const noexcept { return m_path; }

private:
	std::filesystem::path m_path;
};

} // namespace test::shader
