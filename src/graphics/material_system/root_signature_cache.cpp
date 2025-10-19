#include "root_signature_cache.h"
#include "core/console.h"
#include "core/hash_utils.h"
#include <d3d12.h>
#include <functional>
#include <algorithm>
#include <optional>

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
	m_specCache[hash] = spec;

	return rootSignature;
}

uint64_t RootSignatureCache::hashSpec( const RootSignatureSpec &spec ) const
{
	// Hash the spec for cache lookup using new Phase 2 structure

	// Start with count of root descriptors, descriptor table resources, and static samplers
	uint64_t hash = spec.cbvRootDescriptors.size();
	core::hash_combine( hash, spec.descriptorTableResources.size() << 16 );
	core::hash_combine( hash, spec.staticSamplers.size() << 32 );

	// Hash root descriptor CBVs
	for ( const auto &binding : spec.cbvRootDescriptors )
	{
		// Combine name, type, and slot into hash
		core::hash_combine( hash, binding.name );
		core::hash_combine( hash, static_cast<int>( binding.type ) );
		core::hash_combine( hash, binding.slot );
	}

	// Hash descriptor table resources
	for ( const auto &binding : spec.descriptorTableResources )
	{
		// Combine name, type, and slot into hash
		core::hash_combine( hash, binding.name );
		core::hash_combine( hash, static_cast<int>( binding.type ) );
		core::hash_combine( hash, binding.slot );
	}

	// Hash static samplers
	for ( const auto &sampler : spec.staticSamplers )
	{
		// Combine name, filter, address mode, and slot into hash
		core::hash_combine( hash, sampler.name );
		core::hash_combine( hash, static_cast<int>( sampler.filter ) );
		core::hash_combine( hash, static_cast<int>( sampler.addressMode ) );
		core::hash_combine( hash, sampler.slot );
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

			// Sort bindings by slot to merge contiguous ranges
			std::sort( srvBindings.begin(), srvBindings.end(), []( const ResourceBinding &a, const ResourceBinding &b ) { return a.slot < b.slot; } );

			size_t i = 0;
			while ( i < srvBindings.size() )
			{
				D3D12_DESCRIPTOR_RANGE range = {};
				range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
				range.BaseShaderRegister = static_cast<UINT>( srvBindings[i].slot );
				range.RegisterSpace = 0;
				range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

				// Count consecutive SRV bindings starting from this slot
				size_t startSlot = srvBindings[i].slot;
				size_t count = 0;
				while ( i < srvBindings.size() && srvBindings[i].slot == startSlot + count )
				{
					count++;
					i++;
				}

				range.NumDescriptors = static_cast<UINT>( count );
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
				range.NumDescriptors = static_cast<UINT>( binding.arraySize ); // Use array size
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
				range.NumDescriptors = static_cast<UINT>( binding.arraySize ); // Use array size
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

	// Configure static samplers
	std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
	for ( const auto &sampler : spec.staticSamplers )
	{
		D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = sampler.filter;
		samplerDesc.AddressU = sampler.addressMode;
		samplerDesc.AddressV = sampler.addressMode;
		samplerDesc.AddressW = sampler.addressMode;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MaxAnisotropy = sampler.maxAnisotropy;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		samplerDesc.MinLOD = 0.0f;
		samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		samplerDesc.ShaderRegister = static_cast<UINT>( sampler.slot );
		samplerDesc.RegisterSpace = 0;
		samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		staticSamplers.push_back( samplerDesc );
	}

	rootSignatureDesc.NumStaticSamplers = static_cast<UINT>( staticSamplers.size() );
	rootSignatureDesc.pStaticSamplers = staticSamplers.empty() ? nullptr : staticSamplers.data();
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

std::optional<uint32_t> RootSignatureCache::getSrvDescriptorTableIndex( const RootSignatureSpec &spec )
{
	// Calculate root parameter index by walking through the layout in the same order as buildRootSignature
	uint32_t parameterIndex = 0;

	// CBV root descriptors come first (one parameter per CBV)
	parameterIndex += static_cast<uint32_t>( spec.cbvRootDescriptors.size() );

	// Check if we have any SRV bindings in descriptor table resources
	const bool hasSrvs = std::any_of(
		spec.descriptorTableResources.begin(),
		spec.descriptorTableResources.end(),
		[]( const ResourceBinding &binding ) { return binding.type == ResourceBindingType::SRV; } );

	if ( !hasSrvs )
	{
		// No SRV descriptor table exists
		return std::nullopt;
	}

	// SRV descriptor table is the first descriptor table parameter (after CBVs)
	return parameterIndex;
}

} // namespace graphics::material_system
