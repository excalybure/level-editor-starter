module engine.material_gpu;

import runtime.console;

namespace engine::gpu
{

MaterialGPU::MaterialGPU( const std::shared_ptr<assets::Material> &material )
	: m_material( material )
{
	if ( !material )
	{
		console::error( "MaterialGPU: Cannot create from null material" );
		return;
	}

	updateMaterialConstants();
	createConstantBuffer();
	createPipelineState();
	loadTextures();

	m_isValid = true;
}

MaterialGPU::MaterialGPU( MaterialGPU &&other ) noexcept
	: m_material( std::move( other.m_material ) ), m_materialConstants( other.m_materialConstants ), m_pipelineState( std::move( other.m_pipelineState ) ), m_constantBuffer( std::move( other.m_constantBuffer ) ), m_isValid( other.m_isValid )
{
	other.m_isValid = false;
}

MaterialGPU &MaterialGPU::operator=( MaterialGPU &&other ) noexcept
{
	if ( this != &other )
	{
		m_material = std::move( other.m_material );
		m_materialConstants = other.m_materialConstants;
		m_pipelineState = std::move( other.m_pipelineState );
		m_constantBuffer = std::move( other.m_constantBuffer );
		m_isValid = other.m_isValid;

		other.m_isValid = false;
	}
	return *this;
}

MaterialGPU::~MaterialGPU() = default;

void MaterialGPU::bindToCommandList( ID3D12GraphicsCommandList *commandList ) const
{
	if ( !isValid() || !commandList )
	{
		console::error( "MaterialGPU::bindToCommandList: Invalid state or null command list" );
		return;
	}

	// For now, this is a stub - actual rendering integration will come later
	console::info( "MaterialGPU: Binding material resources to command list" );
}

void MaterialGPU::createPipelineState()
{
	// For now, this is a stub - actual pipeline state creation will come later
	console::info( "MaterialGPU: Creating pipeline state" );
}

void MaterialGPU::createConstantBuffer()
{
	// For now, this is a stub - actual constant buffer creation will come later
	console::info( "MaterialGPU: Creating constant buffer" );
}

void MaterialGPU::updateMaterialConstants()
{
	if ( !m_material )
	{
		return;
	}

	const auto &pbr = m_material->getPBRMaterial();

	m_materialConstants.baseColorFactor = pbr.baseColorFactor;

	m_materialConstants.metallicFactor = pbr.metallicFactor;
	m_materialConstants.roughnessFactor = pbr.roughnessFactor;

	m_materialConstants.emissiveFactor = pbr.emissiveFactor;

	// Set texture flags based on available textures
	m_materialConstants.textureFlags = 0;
	if ( !pbr.baseColorTexture.empty() )
	{
		m_materialConstants.textureFlags |= MaterialConstants::kBaseColorTextureBit;
	}
	if ( !pbr.metallicRoughnessTexture.empty() )
	{
		m_materialConstants.textureFlags |= MaterialConstants::kMetallicRoughnessTextureBit;
	}
	if ( !pbr.normalTexture.empty() )
	{
		m_materialConstants.textureFlags |= MaterialConstants::kNormalTextureBit;
	}
	if ( !pbr.emissiveTexture.empty() )
	{
		m_materialConstants.textureFlags |= MaterialConstants::kEmissiveTextureBit;
	}
}

void MaterialGPU::loadTextures()
{
	// For now, this is a stub - actual texture loading will come later
	console::info( "MaterialGPU: Loading textures" );
}

} // namespace engine::gpu