#include "PrimitiveMaterialCommands.h"
#include "engine/assets/assets.h"
#include "graphics/gpu/gpu_resource_manager.h"
#include "graphics/gpu/material_gpu.h"
#include "core/console.h"
#include <format>

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
// Returns true and fills out pointers when valid, otherwise returns false.
bool lookupPrimitive(
	ecs::Entity entity,
	uint32_t primitiveIndex,
	ecs::Scene *ecsScene,
	assets::Scene *assetScene,
	components::MeshRenderer **outMeshRenderer,
	assets::Primitive **outPrimitive )
{
	if ( !ecsScene || !assetScene )
		return false;

	auto *meshRenderer = ecsScene->getComponent<components::MeshRenderer>( entity );
	if ( !meshRenderer || meshRenderer->meshHandle == assets::INVALID_MESH_HANDLE )
		return false;

	const auto mesh = assetScene->getMesh( meshRenderer->meshHandle );
	if ( !mesh )
		return false;

	if ( primitiveIndex >= mesh->getPrimitiveCount() )
		return false;

	if ( outMeshRenderer )
		*outMeshRenderer = meshRenderer;
	if ( outPrimitive )
		*outPrimitive = &mesh->getPrimitive( primitiveIndex );
	return true;
}

bool lookupPrimitive(
	ecs::Entity entity,
	uint32_t primitiveIndex,
	const ecs::Scene *ecsScene,
	const assets::Scene *assetScene,
	const components::MeshRenderer **outMeshRenderer,
	const assets::Primitive **outPrimitive )
{
	if ( !ecsScene || !assetScene )
		return false;

	const auto *meshRenderer = ecsScene->getComponent<components::MeshRenderer>( entity );
	if ( !meshRenderer || meshRenderer->meshHandle == assets::INVALID_MESH_HANDLE )
		return false;

	const auto mesh = assetScene->getMesh( meshRenderer->meshHandle );
	if ( !mesh )
		return false;

	if ( primitiveIndex >= mesh->getPrimitiveCount() )
		return false;

	if ( outMeshRenderer )
		*outMeshRenderer = meshRenderer;
	if ( outPrimitive )
		*outPrimitive = &mesh->getPrimitive( primitiveIndex );
	return true;
}
} // namespace

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
	const components::MeshRenderer *meshRendererPtr = nullptr;
	const assets::Primitive *primitivePtr = nullptr;
	if ( lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene, &meshRendererPtr, &primitivePtr ) )
	{
		const auto &materialInstance = primitivePtr->getMaterialInstance();
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
	components::MeshRenderer *meshRenderer = nullptr;
	assets::Primitive *primitive = nullptr;
	if ( !lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene, &meshRenderer, &primitive ) )
	{
		console::error( "SetPrimitiveMaterialPropertyCommand: Invalid entity/primitive" );
		return false;
	}

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
	primitive->setMaterialInstance( materialInstance );

	// Trigger GPU update
	triggerGPUUpdate();

	return true;
}

MaterialPropertyValue SetPrimitiveMaterialPropertyCommand::getCurrentPropertyValue() const
{
	// Lookup primitive
	const components::MeshRenderer *meshRenderer = nullptr;
	const assets::Primitive *primitive = nullptr;
	if ( !lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene, &meshRenderer, &primitive ) )
	{
		return getDefaultPropertyValue( m_propertyType );
	}

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
	if ( !m_gpuManager )
		return;

	// Lookup GPU primitive and MaterialGPU
	components::MeshRenderer *meshRendererNonConst = nullptr;
	assets::Primitive *primitiveAsset = nullptr;
	if ( !lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene, &meshRendererNonConst, &primitiveAsset ) )
		return;

	if ( !meshRendererNonConst->gpuMesh )
		return;

	if ( m_primitiveIndex >= meshRendererNonConst->gpuMesh->getPrimitiveCount() )
		return;

	auto &primitiveGPU = meshRendererNonConst->gpuMesh->getPrimitive( m_primitiveIndex );
	const auto materialGPU = primitiveGPU.getMaterial();
	if ( !materialGPU )
		return;

	const auto &materialInstance = primitiveAsset->getMaterialInstance();
	materialGPU->updateFromInstance( &materialInstance );
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
	const components::MeshRenderer *meshRendererPtr = nullptr;
	const assets::Primitive *primitivePtr = nullptr;
	if ( lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene, &meshRendererPtr, &primitivePtr ) )
	{
		const auto &materialInstance = primitivePtr->getMaterialInstance();
		m_hadOverride = instanceHasOverride( materialInstance, m_propertyType );
	}
}

bool ClearPrimitiveMaterialPropertyCommand::execute()
{
	// Lookup primitive
	components::MeshRenderer *meshRenderer = nullptr;
	assets::Primitive *primitive = nullptr;
	if ( !lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene, &meshRenderer, &primitive ) )
	{
		console::error( "ClearPrimitiveMaterialPropertyCommand: Invalid entity/primitive" );
		return false;
	}

	auto materialInstance = primitive->getMaterialInstance();

	// Clear the property override
	switch ( m_propertyType )
	{
	case MaterialPropertyType::BaseColorFactor:
		materialInstance.baseColorFactorOverride.reset();
		break;
	case MaterialPropertyType::MetallicFactor:
		materialInstance.metallicFactorOverride.reset();
		break;
	case MaterialPropertyType::RoughnessFactor:
		materialInstance.roughnessFactorOverride.reset();
		break;
	case MaterialPropertyType::EmissiveFactor:
		materialInstance.emissiveFactorOverride.reset();
		break;
	case MaterialPropertyType::BaseColorTexture:
		materialInstance.baseColorTextureOverride.reset();
		break;
	case MaterialPropertyType::MetallicRoughnessTexture:
		materialInstance.metallicRoughnessTextureOverride.reset();
		break;
	case MaterialPropertyType::NormalTexture:
		materialInstance.normalTextureOverride.reset();
		break;
	case MaterialPropertyType::EmissiveTexture:
		materialInstance.emissiveTextureOverride.reset();
		break;
	}

	// Update the primitive's material instance
	primitive->setMaterialInstance( materialInstance );

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

	components::MeshRenderer *meshRendererPtr = nullptr;
	assets::Primitive *primitivePtr = nullptr;
	if ( !lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene, &meshRendererPtr, &primitivePtr ) )
		return false;

	auto materialInstance = primitivePtr->getMaterialInstance();
	setPropertyOnInstance( materialInstance, m_propertyType, m_oldValue );
	primitivePtr->setMaterialInstance( materialInstance );
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
	const components::MeshRenderer *meshRenderer = nullptr;
	const assets::Primitive *primitive = nullptr;
	if ( !lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene, &meshRenderer, &primitive ) )
		return getDefaultPropertyValue( m_propertyType );

	const auto &materialInstance = primitive->getMaterialInstance();

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
	if ( !m_gpuManager )
		return;

	auto *meshRenderer = m_ecsScene->getComponent<components::MeshRenderer>( m_entity );
	if ( !meshRenderer || !meshRenderer->gpuMesh )
		return;

	if ( m_primitiveIndex >= meshRenderer->gpuMesh->getPrimitiveCount() )
		return;

	auto &primitiveGPU = meshRenderer->gpuMesh->getPrimitive( m_primitiveIndex );
	const auto materialGPU = primitiveGPU.getMaterial();
	if ( !materialGPU )
		return;

	// Lookup asset primitive
	components::MeshRenderer *meshRendererNonConst = nullptr;
	assets::Primitive *primitiveAsset = nullptr;
	if ( !lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene, &meshRendererNonConst, &primitiveAsset ) )
		return;

	const auto &materialInstance = primitiveAsset->getMaterialInstance();
	materialGPU->updateFromInstance( &materialInstance );
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
	const components::MeshRenderer *meshRendererPtr = nullptr;
	const assets::Primitive *primitivePtr = nullptr;
	if ( lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene, &meshRendererPtr, &primitivePtr ) )
	{
		m_oldMaterialInstance = primitivePtr->getMaterialInstance();
	}
}

bool ClearAllPrimitiveMaterialOverridesCommand::execute()
{
	components::MeshRenderer *meshRenderer = nullptr;
	assets::Primitive *primitive = nullptr;
	if ( !lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene, &meshRenderer, &primitive ) )
	{
		console::error( "ClearAllPrimitiveMaterialOverridesCommand: Invalid entity/primitive" );
		return false;
	}

	auto materialInstance = primitive->getMaterialInstance();
	materialInstance.clearOverrides();
	primitive->setMaterialInstance( materialInstance );
	triggerGPUUpdate();
	return true;
}

bool ClearAllPrimitiveMaterialOverridesCommand::undo()
{
	// Get the MeshRenderer component
	auto *meshRenderer = m_ecsScene->getComponent<components::MeshRenderer>( m_entity );
	if ( !meshRenderer || meshRenderer->meshHandle == assets::INVALID_MESH_HANDLE )
	{
		return false;
	}

	// Get the mesh from the asset scene
	const auto mesh = m_assetScene->getMesh( meshRenderer->meshHandle );
	if ( !mesh || m_primitiveIndex >= mesh->getPrimitiveCount() )
	{
		return false;
	}

	// Get the primitive and restore the old material instance
	auto &primitive = mesh->getPrimitive( m_primitiveIndex );
	primitive.setMaterialInstance( m_oldMaterialInstance );

	// Trigger GPU update
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
	if ( !m_gpuManager )
		return;

	components::MeshRenderer *meshRendererNonConst = nullptr;
	assets::Primitive *primitiveAsset = nullptr;
	if ( !lookupPrimitive( m_entity, m_primitiveIndex, m_ecsScene, m_assetScene, &meshRendererNonConst, &primitiveAsset ) )
		return;

	if ( !meshRendererNonConst->gpuMesh )
		return;

	if ( m_primitiveIndex >= meshRendererNonConst->gpuMesh->getPrimitiveCount() )
		return;

	auto &primitiveGPU = meshRendererNonConst->gpuMesh->getPrimitive( m_primitiveIndex );
	const auto materialGPU = primitiveGPU.getMaterial();
	if ( !materialGPU )
		return;

	const auto &materialInstance = primitiveAsset->getMaterialInstance();
	materialGPU->updateFromInstance( &materialInstance );
}

} // namespace editor
