#pragma once

namespace core
{

/// Combines a hash value with the hash of another value using the boost-style algorithm.
/// This is the same algorithm used by boost::hash_combine and GLM's hash_combine.
///
/// The magic constant 0x9e3779b9 is the golden ratio constant used to reduce hash collisions.
/// Formula: hash ^= std::hash<T>{}(value) + 0x9e3779b9 + (hash << 6) + (hash >> 2)
///
/// @param seed The current hash value to combine with
/// @param value The value to hash and combine with seed
template <typename T>
inline constexpr void hash_combine( size_t &seed, const T &value ) noexcept
{
	std::hash<T> hasher;
	seed ^= hasher( value ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
}

/// Combines multiple hash values sequentially.
/// Useful for hashing complex objects with multiple fields.
///
/// Example:
///   std::size_t hash = 0;
///   hash_combine(hash, obj.field1, obj.field2, obj.field3);
template <typename T, typename... Rest>
inline constexpr void hash_combine( size_t &seed, const T &value, const Rest &...rest ) noexcept
{
	hash_combine( seed, value );
	hash_combine( seed, rest... );
}

} // namespace core
