# DXC Migration Plan

## Overview
Migrate from FXC (`D3DCompile`) to DXC (`IDxcCompiler3`) to enable Shader Model 6.x support and modern shader features.

## Background
- **Current State**: `shader_compiler.cpp` uses `D3DCompile` (FXC) which only supports SM 5.1 and below
- **Target State**: Use `IDxcCompiler3` API to compile shaders with SM 6.x support
- **Motivation**: Enable bindless texture arrays, dynamic indexing, and other SM 6.x features

## Dependencies

### Required Headers
- `dxcapi.h` - DXC compiler API
- Keep existing: `d3d12.h`, `wrl.h`
- Remove: `d3dcompiler.h` (FXC header)

### Required Libraries
- `dxcompiler.lib` - Link-time library
- `dxcompiler.dll` - Runtime DLL (must be distributed)
- `dxil.dll` - DXIL signing library (must be distributed alongside dxcompiler.dll)

### vcpkg Integration
Check if DXC is available via vcpkg, otherwise use Windows SDK version:
- Windows SDK includes DXC in `<SDK>\bin\<arch>\dxcompiler.dll`
- Typical path: `C:\Program Files (x86)\Windows Kits\10\bin\<version>\x64\`

## Implementation Steps

### Step 1: Update CMakeLists.txt ✅
- [X] Add `dxcompiler` library linkage to graphics target
- [X] Copy `dxcompiler.dll` and `dxil.dll` to output directory as post-build step
- [X] Update comment documentation about shader compilation

### Step 2: Update shader_compiler.h ✅
- [X] Keep existing `ShaderBlob` structure
- [X] Keep `CompileFromFile` signature (minimize API changes)
- [X] Add internal helper for DXC utils/compiler initialization
- [X] Add optional `CompilerVersion` getter for diagnostics

### Step 3: Rewrite shader_compiler.cpp ✅

**Status**: Complete. DXC-based shader compiler implemented with:
- DxcIncludeHandler for IDxcIncludeHandler interface
- Public InitializeDxc() called at application startup (not lazy)
- String conversion helpers moved to core/strings
- Conditional 16-bit types support (SM 6.2+ only)
- Complete error handling and blob conversion

**Initialization**: `ShaderCompiler::InitializeDxc()` must be called once at startup:
- `main.cpp`: Called after fixWorkingDirectory()
- `tests/test_main.cpp`: Called in custom Catch2 main before running tests

**Note**: DXC only supports SM 6.0+ profiles. SM 5.x profiles will fail with "invalid profile" error. Tests using vs_5_1/ps_5_1 need updating (see Step 4).

#### 3.1: Include Handler Migration
Current `ShaderIncludeHandler` implements `ID3DInclude` (FXC-specific).

**DXC Approach**: Use `IDxcIncludeHandler` interface
```cpp
class DxcIncludeHandler : public IDxcIncludeHandler
{
private:
    std::filesystem::path m_shaderDirectory;
    std::vector<std::filesystem::path> m_includedFiles;
    Microsoft::WRL::ComPtr<IDxcUtils> m_utils;

public:
    DxcIncludeHandler(const std::filesystem::path& dir, IDxcUtils* utils);
    
    HRESULT STDMETHODCALLTYPE LoadSource(
        LPCWSTR pFilename,
        IDxcBlob** ppIncludeSource) override;
    
    HRESULT QueryInterface(REFIID riid, void** ppvObject) override;
    ULONG AddRef() override;
    ULONG Release() override;
};
```

#### 3.2: Compiler Initialization
Create DXC instance (singleton or per-compilation):
```cpp
Microsoft::WRL::ComPtr<IDxcUtils> utils;
Microsoft::WRL::ComPtr<IDxcCompiler3> compiler;

DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
```

#### 3.3: Source Preparation
DXC requires `IDxcBlobEncoding` instead of raw string:
```cpp
Microsoft::WRL::ComPtr<IDxcBlobEncoding> sourceBlob;
utils->CreateBlob(finalSource, finalSourceSize, CP_UTF8, &sourceBlob);
```

#### 3.4: Compilation Arguments
Build argument list for DXC (replaces D3DCOMPILE flags):
```cpp
std::vector<LPCWSTR> arguments = {
    L"-E", L"VSMain",                    // Entry point (wide string)
    L"-T", L"vs_6_6",                    // Target profile
    L"-HV", L"2021",                     // HLSL version
    L"-enable-16bit-types",              // Optional: 16-bit type support
};

#ifdef _DEBUG
arguments.push_back(L"-Zi");             // Debug info
arguments.push_back(L"-Od");             // Disable optimization
arguments.push_back(L"-Qembed_debug");   // Embed debug info in shader
#else
arguments.push_back(L"-O3");             // Optimization level
#endif

// Add defines: -D DEFINE_NAME=VALUE
for (const auto& define : defines) {
    // Convert to wide string and add -D prefix
}
```

#### 3.5: Compilation Call
```cpp
DxcBuffer sourceBuffer = {
    sourceBlob->GetBufferPointer(),
    sourceBlob->GetBufferSize(),
    CP_UTF8
};

Microsoft::WRL::ComPtr<IDxcResult> result;
HRESULT hr = compiler->Compile(
    &sourceBuffer,
    arguments.data(),
    static_cast<UINT32>(arguments.size()),
    includeHandler.Get(),
    IID_PPV_ARGS(&result)
);
```

#### 3.6: Error Handling
```cpp
if (SUCCEEDED(hr)) {
    result->GetStatus(&hr);
}

if (FAILED(hr)) {
    Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors;
    result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
    
    if (errors && errors->GetStringLength() > 0) {
        std::string errorMsg = errors->GetStringPointer();
        console::errorAndThrow("Shader compilation failed: {}", errorMsg);
    }
}
```

#### 3.7: Retrieve Compiled Shader
```cpp
Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob;
result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);

// Convert to ID3DBlob for compatibility (or update ShaderBlob to use IDxcBlob)
shaderBlob.As(&result.blob);
```

### Step 4: Update Shader Profiles
Update all shader compilation call sites to use SM 6.x profiles:
- `vs_5_1` → `vs_6_6`
- `ps_5_1` → `ps_6_6`
- `cs_5_1` → `cs_6_6`

Files to check:
- [ ] `materials.json` - Shader profile specifications
- [ ] `shader_manager.cpp` - Hardcoded profiles
- [ ] Test files - Shader compilation tests

### Step 5: String Encoding Helpers
DXC uses wide strings extensively. Add utility functions:
```cpp
namespace {
    std::wstring ToWideString(const std::string& str) {
        // Convert UTF-8 to wide string
    }
    
    std::string ToNarrowString(const std::wstring& wstr) {
        // Convert wide string to UTF-8
    }
}
```

### Step 6: Update Tests
- [ ] `shader_manager_tests.cpp` - Update expected behavior
- [ ] `shader_compiler_tests.cpp` - If exists, update compilation tests
- [ ] Add test for SM 6.6 feature compilation (e.g., dynamic indexing)
- [ ] Verify error message format changes

### Step 7: Distribution & Deployment
- [ ] Ensure `dxcompiler.dll` and `dxil.dll` are copied to build output
- [ ] Add runtime check for DLL presence with helpful error message
- [ ] Update README with DXC requirements
- [ ] Document minimum Windows SDK version requirement

## Compatibility Considerations

### Backward Compatibility
- Keep `ShaderBlob` structure unchanged to minimize API surface changes
- Consider feature detection: fall back to FXC if DXC unavailable (optional)

### Platform Support
- DXC requires Windows 10+ and appropriate Windows SDK
- Consider version checking for older systems

## Testing Strategy

### Unit Tests
1. Compile simple shader with SM 6.6
2. Verify include file tracking still works
3. Test define injection
4. Test error reporting format

### Integration Tests
1. Compile all existing shaders with new compiler
2. Verify rendered output matches previous behavior
3. Test new SM 6.x features (dynamic indexing, bindless)

### Regression Testing
1. Run full test suite after migration
2. Visual comparison of rendered scenes
3. Performance benchmarking (DXC may have different optimization characteristics)

## Risk Assessment

### High Risk
- **Binary compatibility**: `IDxcBlob` vs `ID3DBlob` - may need wrapper or conversion
- **String encoding**: Wide string conversion throughout
- **Error messages**: Different format may break error parsing

### Medium Risk
- **Include handler**: Different interface requires rewrite
- **Compilation flags**: Not 1:1 mapping from D3DCOMPILE flags
- **DLL distribution**: Must ship additional runtime files

### Low Risk
- **Performance**: DXC should be comparable or faster
- **Build time**: Minimal impact on build configuration

## Rollback Plan
If migration fails or causes critical issues:
1. Revert `shader_compiler.cpp` and `shader_compiler.h`
2. Revert CMakeLists.txt changes
3. Restore shader profiles to SM 5.1
4. Keep migration branch for future attempt

## Future Enhancements (Post-Migration)
- [ ] Shader reflection using `IDxcContainerReflection`
- [ ] PDB generation for shader debugging
- [ ] Shader caching using hash of source + arguments
- [ ] Parallel shader compilation
- [ ] Hot reload support with shader recompilation

## References
- [DXC GitHub Repository](https://github.com/microsoft/DirectXShaderCompiler)
- [DXC Wiki - Using dxc.exe and dxcompiler.dll](https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll)
- [Microsoft Docs - IDxcCompiler3](https://docs.microsoft.com/en-us/windows/win32/api/dxcapi/nn-dxcapi-idxccompiler3)
- [Shader Model 6.6 Features](https://docs.microsoft.com/en-us/windows/win32/direct3d12/hlsl-shader-model-6-6-features)

## Timeline Estimate
- Step 1-2 (Build setup): 1 hour
- Step 3 (Core implementation): 4-6 hours
- Step 4-6 (Profile updates, tests): 2-3 hours
- Step 7 (Distribution): 1 hour
- **Total**: 8-11 hours

## Status
- [X] Plan reviewed
- [X] Implementation started
- [X] Step 1: CMakeLists.txt updated
- [X] Step 2: shader_compiler.h updated
- [X] Step 3: shader_compiler.cpp rewritten for DXC
- [ ] Step 4: Update shader profiles to SM 6.x
- [ ] Initial tests passing
- [ ] All shaders migrated
- [ ] Integration tests passing
- [ ] Documentation updated
- [ ] Migration complete
