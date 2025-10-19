#pragma once

#include "root_signature_builder.h"
#include "platform/dx12/dx12_device.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <unordered_map>
#include <cstdint>
#include <optional>

namespace graphics::material_system
{

// Cache for D3D12 root signatures
// Generates and caches root signatures from RootSignatureSpec
class RootSignatureCache
{
public:
	// Get or create root signature from spec
	// Returns cached root signature if spec hash matches existing entry
	// Otherwise creates new D3D12 root signature, caches it, and returns it
	Microsoft::WRL::ComPtr<ID3D12RootSignature> getOrCreate(
		dx12::Device *device,
		const RootSignatureSpec &spec );

	// Get root parameter index for SRV descriptor table from spec
	// Returns std::nullopt if no SRV descriptor table exists
	// This allows dynamic lookup of the texture binding slot instead of hardcoding
	static std::optional<uint32_t> getSrvDescriptorTableIndex( const RootSignatureSpec &spec );

private:
	// Compute hash of RootSignatureSpec for cache lookup
	uint64_t hashSpec( const RootSignatureSpec &spec ) const;

	// Build D3D12 root signature from spec
	Microsoft::WRL::ComPtr<ID3D12RootSignature> buildRootSignature(
		dx12::Device *device,
		const RootSignatureSpec &spec );

	// Cache: hash -> root signature
	std::unordered_map<uint64_t, Microsoft::WRL::ComPtr<ID3D12RootSignature>> m_cache;
	// Cache: hash -> root signature spec (for querying parameter indices)
	std::unordered_map<uint64_t, RootSignatureSpec> m_specCache;
};

} // namespace graphics::material_system
