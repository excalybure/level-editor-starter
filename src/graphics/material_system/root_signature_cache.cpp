#include "root_signature_cache.h"
#include "core/console.h"
#include <d3d12.h>
#include <functional>

namespace graphics::material_system
{

Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignatureCache::getOrCreate(
	dx12::Device *device,
	const RootSignatureSpec &spec )
{
	if ( !device || !device->get() )
	{
		console::error( "RootSignatureCache::getOrCreate: invalid device" );
		return nullptr;
	}

	// Compute hash of spec
	const uint64_t hash = hashSpec( spec );

	// Check cache
	const auto it = m_cache.find( hash );
	if ( it != m_cache.end() )
	{
		// Cache hit - return existing root signature
		return it->second;
	}

	// Cache miss - build new root signature
	auto rootSignature = buildRootSignature( device, spec );
	if ( !rootSignature )
	{
		console::error( "RootSignatureCache::getOrCreate: failed to build root signature" );
		return nullptr;
	}

	// Store in cache
	m_cache[hash] = rootSignature;

	return rootSignature;
}

uint64_t RootSignatureCache::hashSpec( const RootSignatureSpec &spec ) const
{
	// Hash the spec for cache lookup
	// Use std::hash to combine binding count and each binding's properties
	std::hash<std::string> stringHasher;
	std::hash<int> intHasher;

	uint64_t hash = spec.resourceBindings.size();

	for ( const auto &binding : spec.resourceBindings )
	{
		// Combine name, type, and slot into hash
		hash ^= stringHasher( binding.name ) + 0x9e3779b9 + ( hash << 6 ) + ( hash >> 2 );
		hash ^= intHasher( static_cast<int>( binding.type ) ) + 0x9e3779b9 + ( hash << 6 ) + ( hash >> 2 );
		hash ^= intHasher( binding.slot ) + 0x9e3779b9 + ( hash << 6 ) + ( hash >> 2 );
	}

	return hash;
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignatureCache::buildRootSignature(
	dx12::Device *device,
	const RootSignatureSpec &spec )
{
	// Build D3D12_ROOT_PARAMETER array from spec bindings
	std::vector<D3D12_ROOT_PARAMETER> rootParameters;
	rootParameters.reserve( spec.resourceBindings.size() );

	for ( const auto &binding : spec.resourceBindings )
	{
		D3D12_ROOT_PARAMETER param = {};
		param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		switch ( binding.type )
		{
		case ResourceBindingType::CBV:
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			param.Descriptor.ShaderRegister = static_cast<UINT>( binding.slot );
			param.Descriptor.RegisterSpace = 0;
			break;

		case ResourceBindingType::SRV:
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
			param.Descriptor.ShaderRegister = static_cast<UINT>( binding.slot );
			param.Descriptor.RegisterSpace = 0;
			break;

		case ResourceBindingType::UAV:
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
			param.Descriptor.ShaderRegister = static_cast<UINT>( binding.slot );
			param.Descriptor.RegisterSpace = 0;
			break;

		case ResourceBindingType::Sampler:
			// Samplers use descriptor tables, not root descriptors
			// Simplified: skip samplers for now (would need descriptor table setup)
			console::error( "RootSignatureCache: Sampler bindings not yet supported" );
			return nullptr;
		}

		rootParameters.push_back( param );
	}

	// Build root signature descriptor
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.NumParameters = static_cast<UINT>( rootParameters.size() );
	rootSignatureDesc.pParameters = rootParameters.empty() ? nullptr : rootParameters.data();
	rootSignatureDesc.NumStaticSamplers = 0;
	rootSignatureDesc.pStaticSamplers = nullptr;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// Serialize root signature
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
	HRESULT hr = D3D12SerializeRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signatureBlob,
		&errorBlob );

	if ( FAILED( hr ) )
	{
		if ( errorBlob )
		{
			console::error( "Root signature serialization failed: {}",
				static_cast<const char *>( errorBlob->GetBufferPointer() ) );
		}
		else
		{
			console::error( "Root signature serialization failed with HRESULT={:#x}",
				static_cast<unsigned int>( hr ) );
		}
		return nullptr;
	}

	// Create root signature
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	hr = device->get()->CreateRootSignature(
		0,
		signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(),
		IID_PPV_ARGS( &rootSignature ) );

	if ( FAILED( hr ) )
	{
		console::error( "Root signature creation failed with HRESULT={:#x}",
			static_cast<unsigned int>( hr ) );
		return nullptr;
	}

	return rootSignature;
}

} // namespace graphics::material_system
