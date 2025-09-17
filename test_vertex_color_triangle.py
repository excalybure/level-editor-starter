"""
Test script to create a simple glTF with vertex colors and verify they load correctly
"""
import struct
import json
import base64
from pathlib import Path

# Simple triangle with vertex colors
vertices = [
    # Position X, Y, Z, Color R, G, B, A  
    [ 0.5, -0.5, 0.0, 1.0, 0.0, 0.0, 1.0],  # Red vertex
    [-0.5, -0.5, 0.0, 0.0, 1.0, 0.0, 1.0],  # Green vertex  
    [ 0.0,  0.5, 0.0, 0.0, 0.0, 1.0, 1.0]   # Blue vertex
]

indices = [0, 1, 2]

# Pack position data (VEC3) - interleaved: x0,y0,z0, x1,y1,z1, x2,y2,z2
position_data = []
for v in vertices:
    position_data.extend([v[0], v[1], v[2]])  # x, y, z for each vertex
positions = struct.pack('<9f', *position_data)

# Pack color data (VEC4) - interleaved: r0,g0,b0,a0, r1,g1,b1,a1, r2,g2,b2,a2
color_data = []
for v in vertices:
    color_data.extend([v[3], v[4], v[5], v[6]])  # r, g, b, a for each vertex
colors = struct.pack('<12f', *color_data)

# Pack indices
index_data = struct.pack('<3H', *indices)

# Combine all buffers
buffer_data = positions + colors + index_data
b64_data = base64.b64encode(buffer_data).decode('ascii')

gltf = {
    "asset": {"version": "2.0", "generator": "vertex_color_test.py"},
    "buffers": [{"byteLength": len(buffer_data), "uri": f"data:application/octet-stream;base64,{b64_data}"}],
    "bufferViews": [
        {"buffer": 0, "byteOffset": 0, "byteLength": len(positions)},
        {"buffer": 0, "byteOffset": len(positions), "byteLength": len(colors)},
        {"buffer": 0, "byteOffset": len(positions) + len(colors), "byteLength": len(index_data), "target": 34963}
    ],
    "accessors": [
        {"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3"},  # Positions
        {"bufferView": 1, "componentType": 5126, "count": 3, "type": "VEC4"},  # Colors
        {"bufferView": 2, "componentType": 5123, "count": 3, "type": "SCALAR"} # Indices
    ],
    "materials": [
        {
            "name": "VertexColorMaterial",
            "pbrMetallicRoughness": {
                "baseColorFactor": [1.0, 1.0, 1.0, 1.0],
                "metallicFactor": 0.0,
                "roughnessFactor": 1.0
            }
        }
    ],
    "meshes": [{"primitives": [{"attributes": {"POSITION": 0, "COLOR_0": 1}, "indices": 2, "material": 0}]}],
    "nodes": [{"mesh": 0}],
    "scenes": [{"nodes": [0]}],
    "scene": 0
}

OUT_PATH = Path("assets/test/triangle_vertex_colors.gltf")
OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
OUT_PATH.write_text(json.dumps(gltf, indent=2))
print(f"Wrote {OUT_PATH}")