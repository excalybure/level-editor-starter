# Quickstart — Data‑Driven Material System

## 1. Author `materials.json`
Minimal example:
```jsonc
{
  "includes": ["states.json"],
  "defines": { "GLOBAL_QUALITY": "HIGH" },
  "renderPasses": [
    { "name": "DepthPrepass" },
    { "name": "LitOpaque" }
  ],
  "shaders": [
    { "id": "basic_vs", "stage": "vertex", "file": "shaders/basic_vs.hlsl" },
    { "id": "basic_ps", "stage": "pixel",  "file": "shaders/basic_ps.hlsl" }
  ],
  "materials": [
    {
      "id": "LitColor",
      "pass": "LitOpaque",
      "shaders": { "vertex": "basic_vs", "pixel": "basic_ps" },
      "parameters": [
        { "name": "Tint", "type": "float4", "defaultValue": [1,1,1,1] }
      ],
      "states": { "rasterizer": "DefaultRaster", "depthStencil": "LessEqualDepth" },
      "enabled": true
    }
  ]
}
```

## 2. Add shared states in `states.json`
```jsonc
{
  "rasterizerStates": [
    { "id": "DefaultRaster", "fill": "solid", "cull": "back" }
  ],
  "depthStencilStates": [
    { "id": "LessEqualDepth", "depthTest": true, "depthWrite": true, "comparison": "less_equal" }
  ]
}
```

## 3. Launch Editor
System parses includes, validates uniqueness, builds pipelines. Any error -> fatal log + termination.

## 4. Referencing Material in Code
Existing render submission fetches material handle by id:
```cpp
auto handle = materialSystem.getMaterialHandle("LitColor");
if(handle) submitMesh(mesh, handle);
```

## 5. Adding a New Material
Add a new JSON material block; restart application (no hot reload initial). Ensure new shader ids listed under top-level `shaders`.

## 6. Common Failure Cases (All Fatal)
- Duplicate id or define name
- Missing shader file
- Parameter type not in {float,int,bool,float4}
- Pass name not in enum
- Cycle in includes

## 7. Performance Tips
- Keep unused shaders out of `shaders` list to avoid unnecessary compilation.
- Group related materials to optimize cache locality (hash locality benefits pipeline reuse).

## 8. Future Extensions (Deferred)
- Hot reload
- Dynamic pass registration
- Additional parameter types (matrices, arrays, textures)
- Error aggregation mode
