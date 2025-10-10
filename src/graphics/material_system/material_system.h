#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>

namespace graphics::material_system
{

// Forward declarations
struct MaterialDefinition;
class JsonLoader;

// Opaque handle for material access
struct MaterialHandle
{
	uint32_t index = UINT32_MAX;

	bool isValid() const { return index != UINT32_MAX; }
};

// Main material system API for renderer integration
class MaterialSystem
{
public:
	MaterialSystem() = default;

	// Initialize system from JSON file
	// Returns true on success, false if file not found or validation fails
	bool initialize( const std::string &jsonPath );

	// Query material handle by ID
	// Returns invalid handle if material not found
	MaterialHandle getMaterialHandle( const std::string &materialId ) const;

	// Get material definition by handle
	// Returns nullptr if handle invalid
	const MaterialDefinition *getMaterial( MaterialHandle handle ) const;

private:
	std::unordered_map<std::string, uint32_t> m_materialIdToIndex;
	std::vector<MaterialDefinition> m_materials;
};

} // namespace graphics::material_system
