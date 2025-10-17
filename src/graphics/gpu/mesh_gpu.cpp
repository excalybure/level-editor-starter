#include "mesh_gpu.h"

#include <comdef.h>

#include "platform/dx12/dx12_device.h"
#include "engine/assets/assets.h"
#include "graphics/gpu/material_gpu.h"
#include "core/console.h"

namespace graphics::gpu
{

PrimitiveGPU::PrimitiveGPU( dx12::Device &device, const assets::Primitive &primitive )
	: m_device( device ), m_vertexCount( primitive.getVertexCount() ), m_indexCount( primitive.getIndexCount() ), m_material( nullptr )
{
	try
	{
		createVertexBuffer( primitive );
		createIndexBuffer( primitive );
	}
	catch ( const std::exception &e )
	{
		console::error( "Failed to create GPU buffers for primitive: {}", e.what() );
	}
}


void PrimitiveGPU::createVertexBuffer( const assets::Primitive &primitive )
{
	const auto &vertices = primitive.getVertices();
	if ( vertices.empty() )
	{
		console::error( "Cannot create vertex buffer for empty primitive" );
		// Leave m_vertexBuffer as null to indicate failure
		return;
	}

	const std::size_t bufferSize = vertices.size() * sizeof( assets::Vertex );

	// Create upload heap buffer with vertex data
	m_vertexBuffer = createUploadBuffer( bufferSize, vertices.data() );

	if ( m_vertexBuffer )
	{
		// Setup vertex buffer view
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.SizeInBytes = static_cast<UINT>( bufferSize );
		m_vertexBufferView.StrideInBytes = sizeof( assets::Vertex );
	}
	else
	{
		console::error( "Failed to create vertex buffer resource" );
	}
}

void PrimitiveGPU::createIndexBuffer( const assets::Primitive &primitive )
{
	const auto &indices = primitive.getIndices();
	if ( indices.empty() )
	{
		console::error( "Cannot create index buffer for empty primitive" );
		// Leave m_indexBuffer as null to indicate failure
		return;
	}

	const std::size_t bufferSize = indices.size() * sizeof( std::uint32_t );

	// Create upload heap buffer with index data
	m_indexBuffer = createUploadBuffer( bufferSize, indices.data() );

	if ( m_indexBuffer )
	{
		// Setup index buffer view
		m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
		m_indexBufferView.SizeInBytes = static_cast<UINT>( bufferSize );
		m_indexBufferView.Format = DXGI_FORMAT_R32_UINT; // 32-bit indices
	}
	else
	{
		console::error( "Failed to create index buffer resource" );
	}
}

Microsoft::WRL::ComPtr<ID3D12Resource> PrimitiveGPU::createUploadBuffer( std::size_t bufferSize, const void *data )
{
	Microsoft::WRL::ComPtr<ID3D12Resource> buffer;

	// Create upload heap buffer
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = bufferSize;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	HRESULT hr = m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		__uuidof( ID3D12Resource ),
		reinterpret_cast<void **>( buffer.GetAddressOf() ) );

	if ( FAILED( hr ) )
	{
		console::error( "Failed to create D3D12 buffer resource" );
		return nullptr;
	}

	// Map and copy data
	void *mappedData = nullptr;
	hr = buffer->Map( 0, nullptr, &mappedData );
	if ( FAILED( hr ) )
	{
		console::error( "Failed to map D3D12 buffer for writing" );
		return nullptr;
	}

	std::memcpy( mappedData, data, bufferSize );
	buffer->Unmap( 0, nullptr );

	return buffer;
}

void PrimitiveGPU::bindForRendering( ID3D12GraphicsCommandList *commandList ) const
{
	if ( !commandList )
	{
		console::error( "PrimitiveGPU::bindForRendering - commandList is null" );
		return;
	}

	if ( !isValid() )
	{
		console::error( "PrimitiveGPU::bindForRendering - GPU buffers are not valid" );
		return;
	}

	// Bind geometry buffers
	commandList->IASetVertexBuffers( 0, 1, &m_vertexBufferView );

	if ( hasIndexBuffer() )
	{
		commandList->IASetIndexBuffer( &m_indexBufferView );
	}

	// Bind material resources if material is available
	if ( m_material && m_material->isValid() )
	{
		m_material->bindToCommandList( commandList );
	}
}

MeshGPU::MeshGPU( dx12::Device &device, const assets::Mesh &mesh )
	: m_device( device )
{
	// Create GPU buffers for each primitive in the mesh
	const auto &primitives = mesh.getPrimitives();
	m_primitives.reserve( primitives.size() );

	for ( const auto &srcPrimitive : primitives )
	{
		// For now, create without material - this will be enhanced when GPU resource manager is integrated
		auto primitive = std::make_unique<PrimitiveGPU>( device, srcPrimitive );
		if ( primitive->isValid() )
		{
			m_primitives.push_back( std::move( primitive ) );
		}
		else
		{
			console::error( "Failed to create GPU buffers for a primitive in mesh" );
		}
	}

	if ( m_primitives.size() != primitives.size() )
	{
		console::error( "Some primitive buffers failed to create. Expected: {}, Created: {}",
			primitives.size(),
			m_primitives.size() );
	}
}
void MeshGPU::configureMaterials( MaterialProvider &materialProvider, const assets::Scene &scene, const assets::Mesh &mesh )
{
	// Get the primitives from the original mesh to match with existing GPU buffers
	const auto &primitives = mesh.getPrimitives();

	if ( primitives.size() != m_primitives.size() )
	{
		console::error( "Primitive count mismatch - mesh has {} primitives but MeshGPU has {} primitive buffers",
			primitives.size(),
			m_primitives.size() );
		return;
	}

	// Configure materials for each primitive buffer
	for ( std::size_t i = 0; i < primitives.size(); ++i )
	{
		const auto &srcPrimitive = primitives[i];
		auto &gpuPrimitive = m_primitives[i];

		// Skip if primitive already has a material
		if ( gpuPrimitive->hasMaterial() )
		{
			continue;
		}

		std::shared_ptr<graphics::gpu::MaterialGPU> materialToAssign;

		// Check if primitive has a material assigned
		if ( srcPrimitive.hasMaterial() )
		{
			// Resolve material from scene
			const assets::MaterialHandle materialHandle = srcPrimitive.getMaterialHandle();
			if ( const auto material = scene.getMaterial( materialHandle ) )
			{
				materialToAssign = materialProvider.getMaterialGPU( material );
				if ( !materialToAssign )
				{
					console::error( "Failed to create MaterialGPU for material handle '{}'", materialHandle );
					// Fall back to default material
					materialToAssign = materialProvider.getDefaultMaterialGPU();
				}
			}
			else
			{
				console::error( "Invalid material handle '{}' in primitive {} - material not found in scene", materialHandle, i );
				// Fall back to default material
				materialToAssign = materialProvider.getDefaultMaterialGPU();
			}
		}
		else
		{
			// No material assigned, use default
			materialToAssign = materialProvider.getDefaultMaterialGPU();
			console::warning( "Primitive {} has no material - assigned default material", i );
		}

		// Assign the material (either specified or default)
		if ( materialToAssign )
		{
			gpuPrimitive->setMaterial( materialToAssign );
		}
		else
		{
			console::error( "Failed to get any material (including default) for primitive {}", i );
		}
	}
}

const PrimitiveGPU &MeshGPU::getPrimitive( std::uint32_t index ) const
{
	if ( index >= m_primitives.size() )
	{
		console::fatal( "Primitive buffer index {} out of range [0, {})", index, m_primitives.size() );
	}
	return *m_primitives[index];
}

PrimitiveGPU &MeshGPU::getPrimitive( std::uint32_t index )
{
	if ( index >= m_primitives.size() )
	{
		console::fatal( "Primitive buffer index {} out of range [0, {})", index, m_primitives.size() );
	}
	return *m_primitives[index];
}

bool MeshGPU::isValid() const noexcept
{
	return !m_primitives.empty() &&
		std::all_of( m_primitives.begin(), m_primitives.end(), []( const auto &buffer ) { return buffer && buffer->isValid(); } );
}

} // namespace graphics::gpu