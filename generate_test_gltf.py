#!/usr/bin/env python3
"""
Generate test glTF files for texture loading tests:
1. test_textured_cube.gltf - Simple cube with external PNG texture
2. test_embedded_texture.gltf - Cube with base64-encoded embedded texture
3. test_missing_texture.gltf - Cube referencing non-existent texture (for error handling)
"""

import os
import json
import struct
import base64

def create_cube_mesh_data():
    """
    Generate binary data for a simple cube mesh.
    Returns: (positions_data, normals_data, texcoords_data, indices_data, byte_length)
    """
    # Cube vertices (8 corners, but need 24 for proper normals per face)
    # Position data: 24 vertices * 3 floats * 4 bytes = 288 bytes
    positions = [
        # Front face (Z+)
        -0.5, -0.5,  0.5,
         0.5, -0.5,  0.5,
         0.5,  0.5,  0.5,
        -0.5,  0.5,  0.5,
        # Back face (Z-)
         0.5, -0.5, -0.5,
        -0.5, -0.5, -0.5,
        -0.5,  0.5, -0.5,
         0.5,  0.5, -0.5,
        # Right face (X+)
         0.5, -0.5,  0.5,
         0.5, -0.5, -0.5,
         0.5,  0.5, -0.5,
         0.5,  0.5,  0.5,
        # Left face (X-)
        -0.5, -0.5, -0.5,
        -0.5, -0.5,  0.5,
        -0.5,  0.5,  0.5,
        -0.5,  0.5, -0.5,
        # Top face (Y+)
        -0.5,  0.5,  0.5,
         0.5,  0.5,  0.5,
         0.5,  0.5, -0.5,
        -0.5,  0.5, -0.5,
        # Bottom face (Y-)
        -0.5, -0.5, -0.5,
         0.5, -0.5, -0.5,
         0.5, -0.5,  0.5,
        -0.5, -0.5,  0.5,
    ]
    
    # Normals (one per vertex)
    normals = [
        # Front
        0, 0, 1,  0, 0, 1,  0, 0, 1,  0, 0, 1,
        # Back
        0, 0, -1,  0, 0, -1,  0, 0, -1,  0, 0, -1,
        # Right
        1, 0, 0,  1, 0, 0,  1, 0, 0,  1, 0, 0,
        # Left
        -1, 0, 0,  -1, 0, 0,  -1, 0, 0,  -1, 0, 0,
        # Top
        0, 1, 0,  0, 1, 0,  0, 1, 0,  0, 1, 0,
        # Bottom
        0, -1, 0,  0, -1, 0,  0, -1, 0,  0, -1, 0,
    ]
    
    # Texture coordinates (2D UVs)
    texcoords = [
        # Front
        0, 0,  1, 0,  1, 1,  0, 1,
        # Back
        0, 0,  1, 0,  1, 1,  0, 1,
        # Right
        0, 0,  1, 0,  1, 1,  0, 1,
        # Left
        0, 0,  1, 0,  1, 1,  0, 1,
        # Top
        0, 0,  1, 0,  1, 1,  0, 1,
        # Bottom
        0, 0,  1, 0,  1, 1,  0, 1,
    ]
    
    # Indices (2 triangles per face, 6 faces)
    indices = []
    for face in range(6):
        base = face * 4
        indices.extend([
            base + 0, base + 1, base + 2,  # First triangle
            base + 0, base + 2, base + 3,  # Second triangle
        ])
    
    # Pack binary data
    positions_data = struct.pack(f'<{len(positions)}f', *positions)
    normals_data = struct.pack(f'<{len(normals)}f', *normals)
    texcoords_data = struct.pack(f'<{len(texcoords)}f', *texcoords)
    indices_data = struct.pack(f'<{len(indices)}H', *indices)
    
    return positions_data, normals_data, texcoords_data, indices_data

def create_gltf_with_external_texture():
    """Create a glTF file referencing an external PNG texture."""
    
    positions_data, normals_data, texcoords_data, indices_data = create_cube_mesh_data()
    
    # Calculate byte offsets
    positions_offset = 0
    positions_length = len(positions_data)
    normals_offset = positions_length
    normals_length = len(normals_data)
    texcoords_offset = normals_offset + normals_length
    texcoords_length = len(texcoords_data)
    indices_offset = texcoords_offset + texcoords_length
    indices_length = len(indices_data)
    total_length = indices_offset + indices_length
    
    # Combine all binary data
    binary_data = positions_data + normals_data + texcoords_data + indices_data
    
    gltf = {
        "asset": {
            "version": "2.0",
            "generator": "test-generator"
        },
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0}],
        "meshes": [{
            "primitives": [{
                "attributes": {
                    "POSITION": 0,
                    "NORMAL": 1,
                    "TEXCOORD_0": 2
                },
                "indices": 3,
                "material": 0
            }]
        }],
        "materials": [{
            "name": "TexturedMaterial",
            "pbrMetallicRoughness": {
                "baseColorTexture": {"index": 0},
                "metallicFactor": 0.0,
                "roughnessFactor": 1.0
            }
        }],
        "textures": [{
            "source": 0,
            "sampler": 0
        }],
        "images": [{
            "uri": "textures/checkerboard_256.png"
        }],
        "samplers": [{
            "magFilter": 9729,  # LINEAR
            "minFilter": 9729,
            "wrapS": 10497,     # REPEAT
            "wrapT": 10497
        }],
        "buffers": [{
            "uri": "test_textured_cube.bin",
            "byteLength": total_length
        }],
        "bufferViews": [
            {
                "buffer": 0,
                "byteOffset": positions_offset,
                "byteLength": positions_length,
                "target": 34962  # ARRAY_BUFFER
            },
            {
                "buffer": 0,
                "byteOffset": normals_offset,
                "byteLength": normals_length,
                "target": 34962
            },
            {
                "buffer": 0,
                "byteOffset": texcoords_offset,
                "byteLength": texcoords_length,
                "target": 34962
            },
            {
                "buffer": 0,
                "byteOffset": indices_offset,
                "byteLength": indices_length,
                "target": 34963  # ELEMENT_ARRAY_BUFFER
            }
        ],
        "accessors": [
            {
                "bufferView": 0,
                "componentType": 5126,  # FLOAT
                "count": 24,
                "type": "VEC3",
                "min": [-0.5, -0.5, -0.5],
                "max": [0.5, 0.5, 0.5]
            },
            {
                "bufferView": 1,
                "componentType": 5126,
                "count": 24,
                "type": "VEC3"
            },
            {
                "bufferView": 2,
                "componentType": 5126,
                "count": 24,
                "type": "VEC2"
            },
            {
                "bufferView": 3,
                "componentType": 5123,  # UNSIGNED_SHORT
                "count": 36,
                "type": "SCALAR"
            }
        ]
    }
    
    return gltf, binary_data

def create_gltf_with_embedded_texture():
    """Create a glTF file with base64-encoded embedded texture."""
    
    # Read the checkerboard texture
    texture_path = os.path.join('assets', 'test', 'textures', 'checkerboard_256.png')
    with open(texture_path, 'rb') as f:
        texture_data = f.read()
    
    # Base64 encode
    texture_base64 = base64.b64encode(texture_data).decode('ascii')
    data_uri = f"data:image/png;base64,{texture_base64}"
    
    positions_data, normals_data, texcoords_data, indices_data = create_cube_mesh_data()
    
    # Calculate byte offsets
    positions_offset = 0
    positions_length = len(positions_data)
    normals_offset = positions_length
    normals_length = len(normals_data)
    texcoords_offset = normals_offset + normals_length
    texcoords_length = len(texcoords_data)
    indices_offset = texcoords_offset + texcoords_length
    indices_length = len(indices_data)
    total_length = indices_offset + indices_length
    
    # Combine binary data
    binary_data = positions_data + normals_data + texcoords_data + indices_data
    binary_base64 = base64.b64encode(binary_data).decode('ascii')
    buffer_uri = f"data:application/octet-stream;base64,{binary_base64}"
    
    gltf = {
        "asset": {
            "version": "2.0",
            "generator": "test-generator"
        },
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0}],
        "meshes": [{
            "primitives": [{
                "attributes": {
                    "POSITION": 0,
                    "NORMAL": 1,
                    "TEXCOORD_0": 2
                },
                "indices": 3,
                "material": 0
            }]
        }],
        "materials": [{
            "name": "EmbeddedTextureMaterial",
            "pbrMetallicRoughness": {
                "baseColorTexture": {"index": 0},
                "metallicFactor": 0.0,
                "roughnessFactor": 1.0
            }
        }],
        "textures": [{
            "source": 0,
            "sampler": 0
        }],
        "images": [{
            "uri": data_uri
        }],
        "samplers": [{
            "magFilter": 9729,
            "minFilter": 9729,
            "wrapS": 10497,
            "wrapT": 10497
        }],
        "buffers": [{
            "uri": buffer_uri,
            "byteLength": total_length
        }],
        "bufferViews": [
            {
                "buffer": 0,
                "byteOffset": positions_offset,
                "byteLength": positions_length,
                "target": 34962
            },
            {
                "buffer": 0,
                "byteOffset": normals_offset,
                "byteLength": normals_length,
                "target": 34962
            },
            {
                "buffer": 0,
                "byteOffset": texcoords_offset,
                "byteLength": texcoords_length,
                "target": 34962
            },
            {
                "buffer": 0,
                "byteOffset": indices_offset,
                "byteLength": indices_length,
                "target": 34963
            }
        ],
        "accessors": [
            {
                "bufferView": 0,
                "componentType": 5126,
                "count": 24,
                "type": "VEC3",
                "min": [-0.5, -0.5, -0.5],
                "max": [0.5, 0.5, 0.5]
            },
            {
                "bufferView": 1,
                "componentType": 5126,
                "count": 24,
                "type": "VEC3"
            },
            {
                "bufferView": 2,
                "componentType": 5126,
                "count": 24,
                "type": "VEC2"
            },
            {
                "bufferView": 3,
                "componentType": 5123,
                "count": 36,
                "type": "SCALAR"
            }
        ]
    }
    
    return gltf

def create_gltf_with_missing_texture():
    """Create a glTF file referencing a non-existent texture."""
    
    positions_data, normals_data, texcoords_data, indices_data = create_cube_mesh_data()
    
    # Calculate byte offsets
    positions_offset = 0
    positions_length = len(positions_data)
    normals_offset = positions_length
    normals_length = len(normals_data)
    texcoords_offset = normals_offset + normals_length
    texcoords_length = len(texcoords_data)
    indices_offset = texcoords_offset + texcoords_length
    indices_length = len(indices_data)
    total_length = indices_offset + indices_length
    
    # Combine binary data
    binary_data = positions_data + normals_data + texcoords_data + indices_data
    
    gltf = {
        "asset": {
            "version": "2.0",
            "generator": "test-generator"
        },
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0}],
        "meshes": [{
            "primitives": [{
                "attributes": {
                    "POSITION": 0,
                    "NORMAL": 1,
                    "TEXCOORD_0": 2
                },
                "indices": 3,
                "material": 0
            }]
        }],
        "materials": [{
            "name": "MissingTextureMaterial",
            "pbrMetallicRoughness": {
                "baseColorTexture": {"index": 0},
                "baseColorFactor": [1.0, 0.0, 1.0, 1.0],  # Magenta fallback
                "metallicFactor": 0.0,
                "roughnessFactor": 1.0
            }
        }],
        "textures": [{
            "source": 0,
            "sampler": 0
        }],
        "images": [{
            "uri": "textures/this_file_does_not_exist.png"
        }],
        "samplers": [{
            "magFilter": 9729,
            "minFilter": 9729,
            "wrapS": 10497,
            "wrapT": 10497
        }],
        "buffers": [{
            "uri": "test_missing_texture.bin",
            "byteLength": total_length
        }],
        "bufferViews": [
            {
                "buffer": 0,
                "byteOffset": positions_offset,
                "byteLength": positions_length,
                "target": 34962
            },
            {
                "buffer": 0,
                "byteOffset": normals_offset,
                "byteLength": normals_length,
                "target": 34962
            },
            {
                "buffer": 0,
                "byteOffset": texcoords_offset,
                "byteLength": texcoords_length,
                "target": 34962
            },
            {
                "buffer": 0,
                "byteOffset": indices_offset,
                "byteLength": indices_length,
                "target": 34963
            }
        ],
        "accessors": [
            {
                "bufferView": 0,
                "componentType": 5126,
                "count": 24,
                "type": "VEC3",
                "min": [-0.5, -0.5, -0.5],
                "max": [0.5, 0.5, 0.5]
            },
            {
                "bufferView": 1,
                "componentType": 5126,
                "count": 24,
                "type": "VEC3"
            },
            {
                "bufferView": 2,
                "componentType": 5126,
                "count": 24,
                "type": "VEC2"
            },
            {
                "bufferView": 3,
                "componentType": 5123,
                "count": 36,
                "type": "SCALAR"
            }
        ]
    }
    
    return gltf, binary_data

def main():
    output_dir = os.path.join('assets', 'test')
    os.makedirs(output_dir, exist_ok=True)
    
    print("Generating test glTF files...\n")
    
    # 1. External texture glTF
    gltf, binary = create_gltf_with_external_texture()
    gltf_path = os.path.join(output_dir, 'test_textured_cube.gltf')
    bin_path = os.path.join(output_dir, 'test_textured_cube.bin')
    with open(gltf_path, 'w') as f:
        json.dump(gltf, f, indent=2)
    with open(bin_path, 'wb') as f:
        f.write(binary)
    print(f"✓ Created: {gltf_path}")
    print(f"  Binary:  {bin_path}")
    print(f"  Texture: textures/checkerboard_256.png (external)\n")
    
    # 2. Embedded texture glTF
    gltf = create_gltf_with_embedded_texture()
    gltf_path = os.path.join(output_dir, 'test_embedded_texture.gltf')
    with open(gltf_path, 'w') as f:
        json.dump(gltf, f, indent=2)
    print(f"✓ Created: {gltf_path}")
    print(f"  Texture: embedded as base64 data URI\n")
    
    # 3. Missing texture glTF
    gltf, binary = create_gltf_with_missing_texture()
    gltf_path = os.path.join(output_dir, 'test_missing_texture.gltf')
    bin_path = os.path.join(output_dir, 'test_missing_texture.bin')
    with open(gltf_path, 'w') as f:
        json.dump(gltf, f, indent=2)
    with open(bin_path, 'wb') as f:
        f.write(binary)
    print(f"✓ Created: {gltf_path}")
    print(f"  Binary:  {bin_path}")
    print(f"  Texture: textures/this_file_does_not_exist.png (missing - tests error handling)\n")
    
    print("✓ All test glTF files generated successfully!")

if __name__ == '__main__':
    main()
