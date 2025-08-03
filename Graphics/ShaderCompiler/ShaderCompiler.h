#pragma once

#include "Core/Types.h"
#include "Core/NonCopyable.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <d3d11.h>
#include <d3dcompiler.h>

// DirectX Shader Compiler (DXC) headers
#include <dxcapi.h>
#include <d3d12shader.h>

namespace XeSS::Graphics {

enum class ShaderModel : uint32 {
    SM_5_0 = 0x50,
    SM_5_1 = 0x51,
    SM_6_0 = 0x60,
    SM_6_1 = 0x61,
    SM_6_2 = 0x62,
    SM_6_3 = 0x63,
    SM_6_4 = 0x64
};

enum class ShaderType : uint32 {
    Vertex,
    Hull,
    Domain,
    Geometry,
    Pixel,
    Compute,
    Amplification,  // SM 6.5+ (Mesh Shaders)
    Mesh,           // SM 6.5+ (Mesh Shaders)
    RayGeneration,  // SM 6.5+ (DXR)
    Miss,           // SM 6.5+ (DXR)
    ClosestHit,     // SM 6.5+ (DXR)
    AnyHit          // SM 6.5+ (DXR)
};

struct ShaderMacro {
    std::string name;
    std::string definition;
};

struct CompileOptions {
    ShaderModel targetModel = ShaderModel::SM_6_4;
    std::vector<ShaderMacro> macros;
    std::vector<std::string> includePaths;
    bool enableDebugInfo = false;
    bool enableOptimization = true;
    bool warningsAsErrors = false;
    bool ieee754Compliance = false;
    bool enableUnboundedResourceArrays = false;  // SM 6.6+
    uint32 optimizationLevel = 3; // 0-3
};

struct CompiledShader {
    std::vector<uint8> bytecode;
    std::string disassembly;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    bool success = false;

    // Reflection data
    std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayout;
    std::unordered_map<std::string, uint32> constantBufferBindings;
    std::unordered_map<std::string, uint32> textureBindings;
    std::unordered_map<std::string, uint32> samplerBindings;
    std::unordered_map<std::string, uint32> uavBindings;
};

class ShaderCache {
public:
    struct CacheEntry {
        std::vector<uint8> bytecode;
        uint64 hash;
        uint64 timestamp;
    };

    bool GetCachedShader(const std::string& filename, uint64 hash, std::vector<uint8>& bytecode);
    void CacheShader(const std::string& filename, uint64 hash, const std::vector<uint8>& bytecode);
    void ClearCache();
    void SetCacheDirectory(const std::string& directory);

private:
    std::string m_cacheDirectory = "cache/shaders/";
    std::unordered_map<std::string, CacheEntry> m_memoryCache;

    std::string GetCacheFilePath(const std::string& filename, uint64 hash) const;
    uint64 CalculateFileHash(const std::string& filename) const;
};

class ShaderCompiler : public Core::NonCopyable {
public:
    ShaderCompiler();
    ~ShaderCompiler();

    // Initialize with DXC
    bool Initialize();
    void Shutdown();

    // Compile from file
    CompiledShader CompileFromFile(
        const std::string& filename,
        const std::string& entryPoint,
        ShaderType type,
        const CompileOptions& options = {});

    // Compile from source string
    CompiledShader CompileFromSource(
        const std::string& source,
        const std::string& entryPoint,
        ShaderType type,
        const CompileOptions& options = {},
        const std::string& sourceName = "");

    // Hot reload support
    void EnableHotReload(bool enable) { m_hotReloadEnabled = enable; }
    bool IsHotReloadEnabled() const { return m_hotReloadEnabled; }
    void CheckForShaderChanges();

    // Cache management
    ShaderCache& GetCache() { return m_cache; }
    void SetCacheEnabled(bool enabled) { m_cacheEnabled = enabled; }

    // Shader model support
    bool IsShaderModelSupported(ShaderModel model) const;
    ShaderModel GetMaxSupportedShaderModel() const;

    // Feature detection
    bool SupportsVariableRateShading() const;
    bool SupportsMeshShaders() const;
    bool SupportsRaytracing() const;
    bool SupportsWaveIntrinsics() const;

private:
    // DXC interfaces
    IDxcUtils* m_dxcUtils = nullptr;
    IDxcCompiler3* m_dxcCompiler = nullptr;
    IDxcIncludeHandler* m_includeHandler = nullptr;

    // Legacy D3DCompiler for fallback
    bool m_useLegacyCompiler = false;

    // Cache and hot reload
    ShaderCache m_cache;
    bool m_cacheEnabled = true;
    bool m_hotReloadEnabled = false;
    std::unordered_map<std::string, uint64> m_fileTimestamps;

    // Helper methods
    std::wstring GetTargetProfile(ShaderType type, ShaderModel model) const;
    std::vector<LPCWSTR> BuildCompilerArguments(const CompileOptions& options) const;
    std::string LoadShaderSource(const std::string& filename) const;
    uint64 CalculateSourceHash(const std::string& source, const CompileOptions& options) const;

    // Reflection
    void ExtractReflectionData(const std::vector<uint8>& bytecode, CompiledShader& shader) const;

    // Legacy compiler fallback
    CompiledShader CompileWithLegacyCompiler(
        const std::string& source,
        const std::string& entryPoint,
        ShaderType type,
        const CompileOptions& options,
        const std::string& sourceName) const;
};

// Utility functions
std::string ShaderModelToString(ShaderModel model);
std::string ShaderTypeToString(ShaderType type);
ShaderModel ParseShaderModel(const std::string& str);
ShaderType ParseShaderType(const std::string& str);

} // namespace XeSS::Graphics
