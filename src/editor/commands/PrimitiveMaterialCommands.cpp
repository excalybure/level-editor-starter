#include "PrimitiveMaterialCommands.h"
#include "engine/assets/assets.h"
#include "graphics/gpu/gpu_resource_manager.h"
#include "graphics/gpu/material_gpu.h"
#include "core/console.h"
#include <format>
#include <imgui.h>
#include <nlohmann/json.hpp>

namespace editor
{

// =============================================================================
// Helper Functions
// =============================================================================

namespace
{
// Get default value for a material property type
MaterialPropertyValue getDefaultPropertyValue( MaterialPropertyType propertyType )
{
	switch ( propertyType )
	{
	case MaterialPropertyType::BaseColorFactor:
		return math::Vec4f{ 1.0f, 1.0f, 1.0f, 1.0f };
	case MaterialPropertyType::MetallicFactor:
	case MaterialPropertyType::RoughnessFactor:
		return 0.0f;
	case MaterialPropertyType::EmissiveFactor:
		return math::Vec3f{ 0.0f, 0.0f, 0.0f };
	default:
		return std::string{};
	}
}

// Check if a material instance has an override for a given property
bool instanceHasOverride( const assets::MaterialInstance &inst, MaterialPropertyType type )
{
	switch ( type )
	{
	case MaterialPropertyType::BaseColorFactor:
		return inst.baseColorFactorOverride.has_value();
	case MaterialPropertyType::MetallicFactor:
		return inst.metallicFactorOverride.has_value();
	case MaterialPropertyType::RoughnessFactor:
		return inst.roughnessFactorOverride.has_value();
	case MaterialPropertyType::EmissiveFactor:
		return inst.emissiveFactorOverride.has_value();
	case MaterialPropertyType::BaseColorTexture:
		return inst.baseColorTextureOverride.has_value();
	case MaterialPropertyType::MetallicRoughnessTexture:
		return inst.metallicRoughnessTextureOverride.has_value();
	case MaterialPropertyType::NormalTexture:
		return inst.normalTextureOverride.has_value();
	case MaterialPropertyType::EmissiveTexture:
		return inst.emissiveTextureOverride.has_value();
	default:
		return false;
	}
}

// Set a property on a MaterialInstance from a MaterialPropertyValue variant. Returns true if applied.
bool setPropertyOnInstance( assets::MaterialInstance &inst, MaterialPropertyType type, const MaterialPropertyValue &value )
{
	switch ( type )
	{
	case MaterialPropertyType::BaseColorFactor:
		if ( std::holds_alternative<math::Vec4f>( value ) )
		{
			inst.baseColorFactorOverride = std::get<math::Vec4f>( value );
			return true;
		}
		break;
	case MaterialPropertyType::MetallicFactor:
		if ( std::holds_alternative<float>( value ) )
		{
			inst.metallicFactorOverride = std::get<float>( value );
			return true;
		}
		break;
	case MaterialPropertyType::RoughnessFactor:
		if ( std::holds_alternative<float>( value ) )
		{
			inst.roughnessFactorOverride = std::get<float>( value );
			return true;
		}
		break;
	case MaterialPropertyType::EmissiveFactor:
		if ( std::holds_alternative<math::Vec3f>( value ) )
		{
			inst.emissiveFactorOverride = std::get<math::Vec3f>( value );
			return true;
		}
		break;
	case MaterialPropertyType::BaseColorTexture:
		if ( std::holds_alternative<std::string>( value ) )
		{
			inst.baseColorTextureOverride = std::get<std::string>( value );
			return true;
		}
		break;
	case MaterialPropertyType::MetallicRoughnessTexture:
		if ( std::holds_alternative<std::string>( value ) )
		{
			inst.metallicRoughnessTextureOverride = std::get<std::string>( value );
			return true;
		}
		break;
	case MaterialPropertyType::NormalTexture:
		if ( std::holds_alternative<std::string>( value ) )
		{
			inst.normalTextureOverride = std::get<std::string>( value );
			return true;
		}
		break;
	case MaterialPropertyType::EmissiveTexture:
		if ( std::holds_alternative<std::string>( value ) )
		{
			inst.emissiveTextureOverride = std::get<std::string>( value );
			return true;
		}
		break;
	default:
		break;
	}

	return false;
}

// Clear a property override on a MaterialInstance
void clearPropertyOnInstance( assets::MaterialInstance &inst, MaterialPropertyType type )
{
	switch ( type )
	{
	case MaterialPropertyType::BaseColorFactor:
		inst.baseColorFactorOverride.reset();
		break;
	case MaterialPropertyType::MetallicFactor:
		inst.metallicFactorOverride.reset();
		break;
	case MaterialPropertyType::RoughnessFactor:
		inst.roughnessFactorOverride.reset();
		break;
	case MaterialPropertyType::EmissiveFactor:
		inst.emissiveFactorOverride.reset();
		break;
	case MaterialPropertyType::BaseColorTexture:
		inst.baseColorTextureOverride.reset();
		break;
	case MaterialPropertyType::MetallicRoughnessTexture:
		inst.metallicRoughnessTextureOverride.reset();
		break;
	case MaterialPropertyType::NormalTexture:
		inst.normalTextureOverride.reset();
		break;
	case MaterialPropertyType::EmissiveTexture:
		inst.emissiveTextureOverride.reset();
		break;
	default:
		break;
	}
}

// Helper: unify MeshRenderer -> Mesh -> Primitive lookup and validation
// Return small structs wrapped in std::optional when valid, otherwise std::nullopt.
struct PrimitiveLookup
{
	components::MeshRenderer *meshRenderer;
	assets::Primitive *primitive;
};

struct PrimitiveLookupConst
{
	const components::MeshRenderer *meshRenderer;
	const assets::Primitive *primitive;
};

std::optional<PrimitiveLookup> lookupPrimitive(
	ecs::Entity entity,
	uint32_t primitiveIndex,
	ecs::Scene *ecsScene,
	assets::Scene *assetScene )
{
	if ( !ecsScene || !assetScene )
	{
		console::error( "lookupPrimitive: null ecsScene or assetScene" );
		return std::nullopt;
	}

	auto *meshRenderer = ecsScene->getComponent<components::MeshRenderer>( entity );
	if ( !meshRenderer || meshRenderer->meshHandle == assets::INVALID_MESH_HANDLE )
	{
		if ( !meshRenderer )
			console::error( "lookupPrimitive: missing MeshRenderer for entity" );
		else
			console::error( std::format( "lookupPrimitive: invalid mesh handle {}", meshRenderer->meshHandle ) );
		return std::nullopt;
	}

	const auto mesh = assetScene->getMesh( meshRenderer->meshHandle );
	if ( !mesh )
	{
		console::error( std::format( "lookupPrimitive: assetScene returned null for mesh handle {}", meshRenderer->meshHandle ) );
		return std::nullopt;
	}

	if ( primitiveIndex >= mesh->getPrimitiveCount() )
	{
		console::error( std::format( "lookupPrimitive: primitiveIndex {} >= primitiveCount {}", primitiveIndex, mesh->getPrimitiveCount() ) );
		return std::nullopt;
	}

	return PrimitiveLookup{ meshRenderer, &mesh->getPrimitive( primitiveIndex ) };
}

std::optional<PrimitiveLookupConst> lookupPrimitive(
	ecs::Entity entity,
	uint32_t primitiveIndex,
	const ecs::Scene *ecsScene,
	const assets::Scene *assetScene )
{
	if ( !ecsScene || !assetScene )
		return std::nullopt;

	const auto *meshRenderer = ecsScene->getComponent<components::MeshRenderer>( entity );
	if ( !meshRenderer || meshRenderer->meshHandle == assets::INVALID_MESH_HANDLE )
		return std::nullopt;

	const auto mesh = assetScene->getMesh( meshRenderer->meshHandle );
	if ( !mesh )
		return std::nullopt;

	if ( primitiveIndex >= mesh->getPrimitiveCount() )
		return std::nullopt;

	return PrimitiveLookupConst{ meshRenderer, &mesh->getPrimitive( primitiveIndex ) };
}
} // namespace

// Helper: update MaterialGPU from asset primitive's MaterialInstance
// Returns true if an update was performed.
bool updateMaterialGPUForPrimitive(
	ecs::Entity entity,
	uint32_t primitiveIndex,
	ecs::Scene *ecsScene,
	assets::Scene *assetScene,
	graphics::GPUResourceManager *gpuManager )
{
	if ( !gpuManager || !ecsScene || !assetScene )
		return false;

	const auto lookup = lookupPrimitive( entity, primitiveIndex, ecsScene, assetScene );
	if ( !lookup )
		return false;

	components::MeshRenderer *meshRenderer = lookup->meshRenderer;
	assets::Primitive *primitive = lookup->primitive;

	if ( !meshRenderer->gpuMesh )
		return false;

	auto &primitiveGPU = meshRenderer->gpuMesh->getPrimitive( primitiveIndex );
	const auto materialGPU = primitiveGPU.getMaterial();
	if ( !materialGPU )
		return false;

	const auto &materialInstance = primitive->getMaterialInstance();
	materialGPU->updateFromInstance( &materialInstance );
	return true;
}

// =============================================================================
// SetPrimitiveMaterialPropertyCommand Implementation
// =============================================================================

SetPrimitiveMaterialPropertyCommand::SetPrimitiveMaterialPropertyCommand(
	ecs::Entity entity,
	uint32_t primitiveIndex,
	MaterialPropertyType propertyType,
	MaterialPropertyValue newValue,
	ecs::Scene &ecsScene,
	assets::Scene &assetScene,
	graphics::GPUResourceManager *gpuManager )
	: m_entity( entity ), m_primitiveIndex( primitiveIndex ), m_propertyType( propertyType ), m_newValue( std::move( newValue ) ), m_hadOverrideBefore( false ), m_ecsScene( &ecsScene ), m_assetScene( &assetScene ), m_gpuManager( gpuManager )
{
	// Capture the current value for undo
	m_oldValue = getCurrentPropertyValue();

	// Check if an override already existed (use helper)
	const auto lookupCtor = lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene );
	if ( lookupCtor )
	{
		const auto &materialInstance = lookupCtor->primitive->getMaterialInstance();
		m_hadOverrideBefore = instanceHasOverride( materialInstance, m_propertyType );
	}
}

bool SetPrimitiveMaterialPropertyCommand::execute()
{
	return applyPropertyValue( m_newValue, true );
}

bool SetPrimitiveMaterialPropertyCommand::undo()
{
	// If there was no override before, clear it; otherwise restore the old value
	return applyPropertyValue( m_oldValue, m_hadOverrideBefore );
}

std::string SetPrimitiveMaterialPropertyCommand::getDescription() const
{
	return std::format( "Set {} on Primitive {}", propertyTypeToString(), m_primitiveIndex );
}

size_t SetPrimitiveMaterialPropertyCommand::getMemoryUsage() const
{
	// Approximate memory usage
	size_t size = sizeof( *this );

	// Add string sizes for texture paths if applicable
	if ( std::holds_alternative<std::string>( m_oldValue ) )
	{
		size += std::get<std::string>( m_oldValue ).capacity();
	}
	if ( std::holds_alternative<std::string>( m_newValue ) )
	{
		size += std::get<std::string>( m_newValue ).capacity();
	}

	return size;
}

bool SetPrimitiveMaterialPropertyCommand::canMergeWith( const Command *other ) const
{
	const auto *otherCmd = dynamic_cast<const SetPrimitiveMaterialPropertyCommand *>( other );
	if ( !otherCmd )
		return false;

	// Can merge if same entity, same primitive, same property
	return m_entity.id == otherCmd->m_entity.id &&
		m_entity.generation == otherCmd->m_entity.generation &&
		m_primitiveIndex == otherCmd->m_primitiveIndex &&
		m_propertyType == otherCmd->m_propertyType;
}

bool SetPrimitiveMaterialPropertyCommand::mergeWith( std::unique_ptr<Command> other )
{
	const auto *otherCmd = dynamic_cast<SetPrimitiveMaterialPropertyCommand *>( other.get() );
	if ( !otherCmd || !canMergeWith( otherCmd ) )
		return false;

	// Update our new value to the other command's new value
	// Keep our old value (the original before any changes)
	m_newValue = otherCmd->m_newValue;

	return true;
}

bool SetPrimitiveMaterialPropertyCommand::updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity )
{
	return editor::updateEntityReference( m_entity, oldEntity, newEntity );
}

bool SetPrimitiveMaterialPropertyCommand::applyPropertyValue( const MaterialPropertyValue &value, bool isOverride )
{
	// Lookup primitive (validation included)
	const auto lookup = lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene );
	if ( !lookup )
	{
		console::error( "SetPrimitiveMaterialPropertyCommand: Invalid entity/primitive" );
		return false;
	}
	components::MeshRenderer *meshRenderer = lookup->meshRenderer;
	assets::Primitive *primitive = lookup->primitive;

	auto materialInstance = primitive->getMaterialInstance();

	// Apply or clear the property using helper functions
	if ( isOverride )
	{
		setPropertyOnInstance( materialInstance, m_propertyType, value );
	}
	else
	{
		clearPropertyOnInstance( materialInstance, m_propertyType );
	}

	// Update the primitive's material instance
	lookup->primitive->setMaterialInstance( materialInstance );

	// Trigger GPU update
	triggerGPUUpdate();

	return true;
}

MaterialPropertyValue SetPrimitiveMaterialPropertyCommand::getCurrentPropertyValue() const
{
	// Lookup primitive
	const auto lookup = lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene );
	if ( !lookup )
	{
		return getDefaultPropertyValue( m_propertyType );
	}

	const components::MeshRenderer *meshRenderer = lookup->meshRenderer;
	const assets::Primitive *primitive = lookup->primitive;

	const auto &materialInstance = primitive->getMaterialInstance();

	// Get the current value based on property type
	switch ( m_propertyType )
	{
	case MaterialPropertyType::BaseColorFactor:
		if ( materialInstance.baseColorFactorOverride.has_value() )
			return materialInstance.baseColorFactorOverride.value();
		break;
	case MaterialPropertyType::MetallicFactor:
		if ( materialInstance.metallicFactorOverride.has_value() )
			return materialInstance.metallicFactorOverride.value();
		break;
	case MaterialPropertyType::RoughnessFactor:
		if ( materialInstance.roughnessFactorOverride.has_value() )
			return materialInstance.roughnessFactorOverride.value();
		break;
	case MaterialPropertyType::EmissiveFactor:
		if ( materialInstance.emissiveFactorOverride.has_value() )
			return materialInstance.emissiveFactorOverride.value();
		break;
	case MaterialPropertyType::BaseColorTexture:
		if ( materialInstance.baseColorTextureOverride.has_value() )
			return materialInstance.baseColorTextureOverride.value();
		break;
	case MaterialPropertyType::MetallicRoughnessTexture:
		if ( materialInstance.metallicRoughnessTextureOverride.has_value() )
			return materialInstance.metallicRoughnessTextureOverride.value();
		break;
	case MaterialPropertyType::NormalTexture:
		if ( materialInstance.normalTextureOverride.has_value() )
			return materialInstance.normalTextureOverride.value();
		break;
	case MaterialPropertyType::EmissiveTexture:
		if ( materialInstance.emissiveTextureOverride.has_value() )
			return materialInstance.emissiveTextureOverride.value();
		break;
	}

	// If no override exists, get value from base material
	const auto material = m_assetScene->getMaterial( primitive->getMaterialHandle() );
	if ( material )
	{
		const auto &pbr = material->getPBRMaterial();
		switch ( m_propertyType )
		{
		case MaterialPropertyType::BaseColorFactor:
			return pbr.baseColorFactor;
		case MaterialPropertyType::MetallicFactor:
			return pbr.metallicFactor;
		case MaterialPropertyType::RoughnessFactor:
			return pbr.roughnessFactor;
		case MaterialPropertyType::EmissiveFactor:
			return pbr.emissiveFactor;
		case MaterialPropertyType::BaseColorTexture:
			return pbr.baseColorTexture;
		case MaterialPropertyType::MetallicRoughnessTexture:
			return pbr.metallicRoughnessTexture;
		case MaterialPropertyType::NormalTexture:
			return pbr.normalTexture;
		case MaterialPropertyType::EmissiveTexture:
			return pbr.emissiveTexture;
		}
	}

	// Return defaults if material not found
	return getDefaultPropertyValue( m_propertyType );
}

void SetPrimitiveMaterialPropertyCommand::triggerGPUUpdate()
{
	updateMaterialGPUForPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene, m_gpuManager );
}

std::string SetPrimitiveMaterialPropertyCommand::propertyTypeToString() const
{
	switch ( m_propertyType )
	{
	case MaterialPropertyType::BaseColorFactor:
		return "Base Color";
	case MaterialPropertyType::MetallicFactor:
		return "Metallic";
	case MaterialPropertyType::RoughnessFactor:
		return "Roughness";
	case MaterialPropertyType::EmissiveFactor:
		return "Emissive";
	case MaterialPropertyType::BaseColorTexture:
		return "Base Color Texture";
	case MaterialPropertyType::MetallicRoughnessTexture:
		return "Metallic Roughness Texture";
	case MaterialPropertyType::NormalTexture:
		return "Normal Texture";
	case MaterialPropertyType::EmissiveTexture:
		return "Emissive Texture";
	default:
		return "Unknown Property";
	}
}

// =============================================================================
// ClearPrimitiveMaterialPropertyCommand Implementation
// =============================================================================

ClearPrimitiveMaterialPropertyCommand::ClearPrimitiveMaterialPropertyCommand(
	ecs::Entity entity,
	uint32_t primitiveIndex,
	MaterialPropertyType propertyType,
	ecs::Scene &ecsScene,
	assets::Scene &assetScene,
	graphics::GPUResourceManager *gpuManager )
	: m_entity( entity ), m_primitiveIndex( primitiveIndex ), m_propertyType( propertyType ), m_hadOverride( false ), m_ecsScene( &ecsScene ), m_assetScene( &assetScene ), m_gpuManager( gpuManager )
{
	// Capture the current value for undo
	m_oldValue = getCurrentPropertyValue();

	// Check if an override exists (use helper)
	const auto lookupCtor = lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene );
	if ( lookupCtor )
	{
		const auto &materialInstance = lookupCtor->primitive->getMaterialInstance();
		m_hadOverride = instanceHasOverride( materialInstance, m_propertyType );
	}
}

bool ClearPrimitiveMaterialPropertyCommand::execute()
{
	// Lookup primitive
	const auto lookupExec = lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene );
	if ( !lookupExec )
	{
		console::error( "ClearPrimitiveMaterialPropertyCommand: Invalid entity/primitive" );
		return false;
	}

	auto materialInstance = lookupExec->primitive->getMaterialInstance();

	// Clear the property override using helper
	clearPropertyOnInstance( materialInstance, m_propertyType );

	// Update the primitive's material instance
	lookupExec->primitive->setMaterialInstance( materialInstance );

	// Trigger GPU update
	triggerGPUUpdate();

	return true;
}

bool ClearPrimitiveMaterialPropertyCommand::undo()
{
	if ( !m_hadOverride )
	{
		// Nothing to restore
		return true;
	}

	const auto lookupUndo = lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene );
	if ( !lookupUndo )
		return false;

	auto materialInstance = lookupUndo->primitive->getMaterialInstance();
	setPropertyOnInstance( materialInstance, m_propertyType, m_oldValue );
	lookupUndo->primitive->setMaterialInstance( materialInstance );
	triggerGPUUpdate();

	return true;
}

std::string ClearPrimitiveMaterialPropertyCommand::getDescription() const
{
	return std::format( "Clear {} Override on Primitive {}", propertyTypeToString(), m_primitiveIndex );
}

size_t ClearPrimitiveMaterialPropertyCommand::getMemoryUsage() const
{
	size_t size = sizeof( *this );
	if ( std::holds_alternative<std::string>( m_oldValue ) )
	{
		size += std::get<std::string>( m_oldValue ).capacity();
	}
	return size;
}

bool ClearPrimitiveMaterialPropertyCommand::updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity )
{
	return editor::updateEntityReference( m_entity, oldEntity, newEntity );
}

MaterialPropertyValue ClearPrimitiveMaterialPropertyCommand::getCurrentPropertyValue() const
{
	const auto lookupGet = lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene );
	if ( !lookupGet )
		return getDefaultPropertyValue( m_propertyType );

	const auto &materialInstance = lookupGet->primitive->getMaterialInstance();

	switch ( m_propertyType )
	{
	case MaterialPropertyType::BaseColorFactor:
		if ( materialInstance.baseColorFactorOverride.has_value() )
			return materialInstance.baseColorFactorOverride.value();
		break;
	case MaterialPropertyType::MetallicFactor:
		if ( materialInstance.metallicFactorOverride.has_value() )
			return materialInstance.metallicFactorOverride.value();
		break;
	case MaterialPropertyType::RoughnessFactor:
		if ( materialInstance.roughnessFactorOverride.has_value() )
			return materialInstance.roughnessFactorOverride.value();
		break;
	case MaterialPropertyType::EmissiveFactor:
		if ( materialInstance.emissiveFactorOverride.has_value() )
			return materialInstance.emissiveFactorOverride.value();
		break;
	case MaterialPropertyType::BaseColorTexture:
		if ( materialInstance.baseColorTextureOverride.has_value() )
			return materialInstance.baseColorTextureOverride.value();
		break;
	case MaterialPropertyType::MetallicRoughnessTexture:
		if ( materialInstance.metallicRoughnessTextureOverride.has_value() )
			return materialInstance.metallicRoughnessTextureOverride.value();
		break;
	case MaterialPropertyType::NormalTexture:
		if ( materialInstance.normalTextureOverride.has_value() )
			return materialInstance.normalTextureOverride.value();
		break;
	case MaterialPropertyType::EmissiveTexture:
		if ( materialInstance.emissiveTextureOverride.has_value() )
			return materialInstance.emissiveTextureOverride.value();
		break;
	}

	// Return defaults
	return getDefaultPropertyValue( m_propertyType );
}

void ClearPrimitiveMaterialPropertyCommand::triggerGPUUpdate()
{
	updateMaterialGPUForPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene, m_gpuManager );
}

std::string ClearPrimitiveMaterialPropertyCommand::propertyTypeToString() const
{
	switch ( m_propertyType )
	{
	case MaterialPropertyType::BaseColorFactor:
		return "Base Color";
	case MaterialPropertyType::MetallicFactor:
		return "Metallic";
	case MaterialPropertyType::RoughnessFactor:
		return "Roughness";
	case MaterialPropertyType::EmissiveFactor:
		return "Emissive";
	case MaterialPropertyType::BaseColorTexture:
		return "Base Color Texture";
	case MaterialPropertyType::MetallicRoughnessTexture:
		return "Metallic Roughness Texture";
	case MaterialPropertyType::NormalTexture:
		return "Normal Texture";
	case MaterialPropertyType::EmissiveTexture:
		return "Emissive Texture";
	default:
		return "Unknown Property";
	}
}

// =============================================================================
// ClearAllPrimitiveMaterialOverridesCommand Implementation
// =============================================================================

ClearAllPrimitiveMaterialOverridesCommand::ClearAllPrimitiveMaterialOverridesCommand(
	ecs::Entity entity,
	uint32_t primitiveIndex,
	ecs::Scene &ecsScene,
	assets::Scene &assetScene,
	graphics::GPUResourceManager *gpuManager )
	: m_entity( entity ), m_primitiveIndex( primitiveIndex ), m_ecsScene( &ecsScene ), m_assetScene( &assetScene ), m_gpuManager( gpuManager )
{
	// Capture the current material instance for undo
	const auto lookupCtor = lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene );
	if ( lookupCtor )
	{
		m_oldMaterialInstance = lookupCtor->primitive->getMaterialInstance();
	}
}

bool ClearAllPrimitiveMaterialOverridesCommand::execute()
{
	const auto lookupExec = lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene );
	if ( !lookupExec )
	{
		console::error( "ClearAllPrimitiveMaterialOverridesCommand: Invalid entity/primitive" );
		return false;
	}

	auto materialInstance = lookupExec->primitive->getMaterialInstance();
	materialInstance.clearOverrides();
	lookupExec->primitive->setMaterialInstance( materialInstance );
	triggerGPUUpdate();
	return true;
}

bool ClearAllPrimitiveMaterialOverridesCommand::undo()
{
	const auto lookupUndo = lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene );
	if ( !lookupUndo )
		return false;

	lookupUndo->primitive->setMaterialInstance( m_oldMaterialInstance );
	triggerGPUUpdate();
	return true;
}

std::string ClearAllPrimitiveMaterialOverridesCommand::getDescription() const
{
	return std::format( "Clear All Overrides on Primitive {}", m_primitiveIndex );
}

size_t ClearAllPrimitiveMaterialOverridesCommand::getMemoryUsage() const
{
	size_t size = sizeof( *this );

	// Add sizes for any string overrides in the material instance
	if ( m_oldMaterialInstance.baseColorTextureOverride.has_value() )
		size += m_oldMaterialInstance.baseColorTextureOverride.value().capacity();
	if ( m_oldMaterialInstance.metallicRoughnessTextureOverride.has_value() )
		size += m_oldMaterialInstance.metallicRoughnessTextureOverride.value().capacity();
	if ( m_oldMaterialInstance.normalTextureOverride.has_value() )
		size += m_oldMaterialInstance.normalTextureOverride.value().capacity();
	if ( m_oldMaterialInstance.emissiveTextureOverride.has_value() )
		size += m_oldMaterialInstance.emissiveTextureOverride.value().capacity();

	return size;
}

bool ClearAllPrimitiveMaterialOverridesCommand::updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity )
{
	return editor::updateEntityReference( m_entity, oldEntity, newEntity );
}

void ClearAllPrimitiveMaterialOverridesCommand::triggerGPUUpdate()
{
	updateMaterialGPUForPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene, m_gpuManager );
}

// =============================================================================
// CopyPrimitiveMaterialCommand Implementation
// =============================================================================

CopyPrimitiveMaterialCommand::CopyPrimitiveMaterialCommand(
	ecs::Entity entity,
	uint32_t primitiveIndex,
	ecs::Scene &ecsScene,
	assets::Scene &assetScene )
	: m_entity( entity ), m_primitiveIndex( primitiveIndex ), m_ecsScene( &ecsScene ), m_assetScene( &assetScene )
{
}

bool CopyPrimitiveMaterialCommand::execute()
{
	const auto lookup = lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene );
	if ( !lookup )
	{
		console::error( "CopyPrimitiveMaterialCommand: Invalid entity/primitive" );
		return false;
	}

	const auto &materialInstance = lookup->primitive->getMaterialInstance();

	// Serialize MaterialInstance to JSON
	using json = nlohmann::json;
	json instanceJson;

	// Serialize value overrides if present
	if ( materialInstance.baseColorFactorOverride.has_value() )
	{
		const auto &v = materialInstance.baseColorFactorOverride.value();
		instanceJson["baseColorFactor"] = { v.x, v.y, v.z, v.w };
	}

	if ( materialInstance.metallicFactorOverride.has_value() )
	{
		instanceJson["metallicFactor"] = materialInstance.metallicFactorOverride.value();
	}

	if ( materialInstance.roughnessFactorOverride.has_value() )
	{
		instanceJson["roughnessFactor"] = materialInstance.roughnessFactorOverride.value();
	}

	if ( materialInstance.emissiveFactorOverride.has_value() )
	{
		const auto &v = materialInstance.emissiveFactorOverride.value();
		instanceJson["emissiveFactor"] = { v.x, v.y, v.z };
	}

	// Serialize texture overrides if present
	if ( materialInstance.baseColorTextureOverride.has_value() )
	{
		instanceJson["baseColorTexture"] = materialInstance.baseColorTextureOverride.value();
	}

	if ( materialInstance.metallicRoughnessTextureOverride.has_value() )
	{
		instanceJson["metallicRoughnessTexture"] = materialInstance.metallicRoughnessTextureOverride.value();
	}

	if ( materialInstance.normalTextureOverride.has_value() )
	{
		instanceJson["normalTexture"] = materialInstance.normalTextureOverride.value();
	}

	if ( materialInstance.emissiveTextureOverride.has_value() )
	{
		instanceJson["emissiveTexture"] = materialInstance.emissiveTextureOverride.value();
	}

	// Copy JSON string to clipboard
	const std::string jsonString = instanceJson.dump();
	ImGui::SetClipboardText( jsonString.c_str() );

	console::info( "Copied material overrides from primitive {} to clipboard", m_primitiveIndex );
	return true;
}

std::string CopyPrimitiveMaterialCommand::getDescription() const
{
	return std::format( "Copy Material Overrides from Primitive {}", m_primitiveIndex );
}

size_t CopyPrimitiveMaterialCommand::getMemoryUsage() const
{
	return sizeof( *this );
}

bool CopyPrimitiveMaterialCommand::updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity )
{
	return editor::updateEntityReference( m_entity, oldEntity, newEntity );
}

// =============================================================================
// PastePrimitiveMaterialCommand Implementation
// =============================================================================

PastePrimitiveMaterialCommand::PastePrimitiveMaterialCommand(
	ecs::Entity entity,
	uint32_t primitiveIndex,
	ecs::Scene &ecsScene,
	assets::Scene &assetScene,
	graphics::GPUResourceManager *gpuManager )
	: m_entity( entity ), m_primitiveIndex( primitiveIndex ), m_ecsScene( &ecsScene ), m_assetScene( &assetScene ), m_gpuManager( gpuManager )
{
	// Capture old state for undo
	const auto lookup = lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene );
	if ( lookup )
	{
		m_oldMaterialInstance = lookup->primitive->getMaterialInstance();
	}

	// Parse clipboard content to get new material instance
	const char *clipboardText = ImGui::GetClipboardText();
	if ( clipboardText && clipboardText[0] != '\0' )
	{
		try
		{
			using json = nlohmann::json;
			const auto clipJson = json::parse( clipboardText );

			// Deserialize value overrides
			if ( clipJson.contains( "baseColorFactor" ) && clipJson["baseColorFactor"].is_array() && clipJson["baseColorFactor"].size() == 4 )
			{
				m_newMaterialInstance.baseColorFactorOverride = math::Vec4f{
					clipJson["baseColorFactor"][0],
					clipJson["baseColorFactor"][1],
					clipJson["baseColorFactor"][2],
					clipJson["baseColorFactor"][3]
				};
			}

			if ( clipJson.contains( "metallicFactor" ) && clipJson["metallicFactor"].is_number() )
			{
				m_newMaterialInstance.metallicFactorOverride = clipJson["metallicFactor"];
			}

			if ( clipJson.contains( "roughnessFactor" ) && clipJson["roughnessFactor"].is_number() )
			{
				m_newMaterialInstance.roughnessFactorOverride = clipJson["roughnessFactor"];
			}

			if ( clipJson.contains( "emissiveFactor" ) && clipJson["emissiveFactor"].is_array() && clipJson["emissiveFactor"].size() == 3 )
			{
				m_newMaterialInstance.emissiveFactorOverride = math::Vec3f{
					clipJson["emissiveFactor"][0],
					clipJson["emissiveFactor"][1],
					clipJson["emissiveFactor"][2]
				};
			}

			// Deserialize texture overrides
			if ( clipJson.contains( "baseColorTexture" ) && clipJson["baseColorTexture"].is_string() )
			{
				m_newMaterialInstance.baseColorTextureOverride = clipJson["baseColorTexture"];
			}

			if ( clipJson.contains( "metallicRoughnessTexture" ) && clipJson["metallicRoughnessTexture"].is_string() )
			{
				m_newMaterialInstance.metallicRoughnessTextureOverride = clipJson["metallicRoughnessTexture"];
			}

			if ( clipJson.contains( "normalTexture" ) && clipJson["normalTexture"].is_string() )
			{
				m_newMaterialInstance.normalTextureOverride = clipJson["normalTexture"];
			}

			if ( clipJson.contains( "emissiveTexture" ) && clipJson["emissiveTexture"].is_string() )
			{
				m_newMaterialInstance.emissiveTextureOverride = clipJson["emissiveTexture"];
			}
		}
		catch ( const std::exception &e )
		{
			console::error( "PastePrimitiveMaterialCommand: Failed to parse clipboard JSON: {}", e.what() );
		}
	}
}

bool PastePrimitiveMaterialCommand::execute()
{
	const auto lookup = lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene );
	if ( !lookup )
	{
		console::error( "PastePrimitiveMaterialCommand: Invalid entity/primitive" );
		return false;
	}

	lookup->primitive->setMaterialInstance( m_newMaterialInstance );
	triggerGPUUpdate();
	return true;
}

bool PastePrimitiveMaterialCommand::undo()
{
	const auto lookup = lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene );
	if ( !lookup )
		return false;

	lookup->primitive->setMaterialInstance( m_oldMaterialInstance );
	triggerGPUUpdate();
	return true;
}

std::string PastePrimitiveMaterialCommand::getDescription() const
{
	return std::format( "Paste Material Overrides to Primitive {}", m_primitiveIndex );
}

size_t PastePrimitiveMaterialCommand::getMemoryUsage() const
{
	size_t size = sizeof( *this );

	// Add sizes for old overrides
	if ( m_oldMaterialInstance.baseColorTextureOverride.has_value() )
		size += m_oldMaterialInstance.baseColorTextureOverride.value().capacity();
	if ( m_oldMaterialInstance.metallicRoughnessTextureOverride.has_value() )
		size += m_oldMaterialInstance.metallicRoughnessTextureOverride.value().capacity();
	if ( m_oldMaterialInstance.normalTextureOverride.has_value() )
		size += m_oldMaterialInstance.normalTextureOverride.value().capacity();
	if ( m_oldMaterialInstance.emissiveTextureOverride.has_value() )
		size += m_oldMaterialInstance.emissiveTextureOverride.value().capacity();

	// Add sizes for new overrides
	if ( m_newMaterialInstance.baseColorTextureOverride.has_value() )
		size += m_newMaterialInstance.baseColorTextureOverride.value().capacity();
	if ( m_newMaterialInstance.metallicRoughnessTextureOverride.has_value() )
		size += m_newMaterialInstance.metallicRoughnessTextureOverride.value().capacity();
	if ( m_newMaterialInstance.normalTextureOverride.has_value() )
		size += m_newMaterialInstance.normalTextureOverride.value().capacity();
	if ( m_newMaterialInstance.emissiveTextureOverride.has_value() )
		size += m_newMaterialInstance.emissiveTextureOverride.value().capacity();

	return size;
}

bool PastePrimitiveMaterialCommand::updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity )
{
	return editor::updateEntityReference( m_entity, oldEntity, newEntity );
}

void PastePrimitiveMaterialCommand::triggerGPUUpdate()
{
	updateMaterialGPUForPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene, m_gpuManager );
}

} // namespace editor
