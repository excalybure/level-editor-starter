#pragma once

#include <cstdint>

namespace ecs
{

// Enhanced Entity with generation for safe handles
struct Entity
{
	std::uint32_t id{};
	std::uint32_t generation{};

	auto operator<=>( const Entity & ) const = default;
	bool operator==( const Entity &other ) const = default;

	bool isValid() const noexcept
	{
		return id != 0; // ID 0 is reserved for invalid entities
	}
};

} // namespace ecs

// Hash function for Entity to enable use in std::unordered_map
template <>
struct std::hash<ecs::Entity>
{
	std::size_t operator()( const ecs::Entity &entity ) const noexcept
	{
		// Combine id and generation for hash
		const std::uint64_t key = ( static_cast<std::uint64_t>( entity.generation ) << 32 ) | static_cast<std::uint64_t>( entity.id );
		return std::hash<std::uint64_t>{}( key );
	}
};
