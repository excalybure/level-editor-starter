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
	std::vector<D3D12_ROOT_PARAMETER> rootParameters;

	// Storage for descriptor ranges (must persist until serialization)
	std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> tableRanges;

	// Add root descriptor CBVs (2 DWORDs each)
	for ( const auto &binding : spec.cbvRootDescriptors )
	{
		D3D12_ROOT_PARAMETER param = {};
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		param.Descriptor.ShaderRegister = static_cast<UINT>( binding.slot );
		param.Descriptor.RegisterSpace = 0;
		param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParameters.push_back( param );
	}

	// Add descriptor table resources (SRVs, UAVs, Samplers)
	// Group by type for efficient descriptor table layout
	if ( !spec.descriptorTableResources.empty() )
	{
		// Separate resources by type
		std::vector<ResourceBinding> srvBindings;
		std::vector<ResourceBinding> uavBindings;
		std::vector<ResourceBinding> samplerBindings;

		for ( const auto &binding : spec.descriptorTableResources )
		{
			switch ( binding.type )
			{
			case ResourceBindingType::SRV:
				srvBindings.push_back( binding );
				break;
			case ResourceBindingType::UAV:
				uavBindings.push_back( binding );
				break;
			case ResourceBindingType::Sampler:
				samplerBindings.push_back( binding );
				break;
			case ResourceBindingType::CBV:
				// CBVs should be in cbvRootDescriptors, not descriptorTableResources
				console::error( "RootSignatureCache: CBV found in descriptorTableResources (should be in cbvRootDescriptors)" );
				return nullptr;
			}
		}

		// Create descriptor table for SRVs if any exist
		if ( !srvBindings.empty() )
		{
			std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
			for ( const auto &binding : srvBindings )
			{
				D3D12_DESCRIPTOR_RANGE range = {};
				range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
				range.NumDescriptors = 1;
				range.BaseShaderRegister = static_cast<UINT>( binding.slot );
				range.RegisterSpace = 0;
				range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
				ranges.push_back( range );
			}

			tableRanges.push_back( ranges );

			D3D12_ROOT_PARAMETER param = {};
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>( ranges.size() );
			param.DescriptorTable.pDescriptorRanges = tableRanges.back().data();
			param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootParameters.push_back( param );
		}

		// Create descriptor table for UAVs if any exist
		if ( !uavBindings.empty() )
		{
			std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
			for ( const auto &binding : uavBindings )
			{
				D3D12_DESCRIPTOR_RANGE range = {};
				range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
				range.NumDescriptors = 1;
				range.BaseShaderRegister = static_cast<UINT>( binding.slot );
				range.RegisterSpace = 0;
				range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
				ranges.push_back( range );
			}

			tableRanges.push_back( ranges );

			D3D12_ROOT_PARAMETER param = {};
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>( ranges.size() );
			param.DescriptorTable.pDescriptorRanges = tableRanges.back().data();
			param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootParameters.push_back( param );
		}

		// Create descriptor table for Samplers if any exist
		if ( !samplerBindings.empty() )
		{
			std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
			for ( const auto &binding : samplerBindings )
			{
				D3D12_DESCRIPTOR_RANGE range = {};
				range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
				range.NumDescriptors = 1;
				range.BaseShaderRegister = static_cast<UINT>( binding.slot );
				range.RegisterSpace = 0;
				range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
				ranges.push_back( range );
			}

			tableRanges.push_back( ranges );

			D3D12_ROOT_PARAMETER param = {};
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>( ranges.size() );
			param.DescriptorTable.pDescriptorRanges = tableRanges.back().data();
			param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootParameters.push_back( param );
		}
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
