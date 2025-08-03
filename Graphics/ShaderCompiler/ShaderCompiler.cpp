#include "ShaderCompiler.h"
#include "Core/Logger.h"
#include "Core/Exception.h"
#include "Core/Utils.h"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <cassert>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxcompiler.lib")

namespace XeSS::Graphics {

namespace {
    constexpr uint32 CACHE_VERSION = 1;
    constexpr char CACHE_MAGIC[] = "XESS";

    struct CacheHeader {
        char magic[4];
        uint32 version;
        uint64 hash;
        uint32 size;
    };
}

// ShaderCache Implementation
bool ShaderCache::GetCachedShader(const std::string& filename, uint64 hash, std::vector<uint8>& bytecode) {
    // Check memory cache first
    auto it = m_memoryCache.find(filename);
    if (it != m_memoryCache.end() && it->second.hash == hash) {
        bytecode = it->second.bytecode;
        return true;
    }

    // Check disk cache
    std::string cachePath = GetCacheFilePath(filename, hash);
    if (!std::filesystem::exists(cachePath)) {
        return false;
    }

    try {
        std::ifstream file(cachePath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        CacheHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));

        if (memcmp(header.magic, CACHE_MAGIC, 4) != 0 ||
            header.version != CACHE_VERSION ||
            header.hash != hash) {
            return false;
        }

        bytecode.resize(header.size);
        file.read(reinterpret_cast<char*>(bytecode.data()), header.size);

        // Update memory cache
        CacheEntry entry;
        entry.bytecode = bytecode;
        entry.hash = hash;
        entry.timestamp = std::filesystem::last_write_time(cachePath).time_since_epoch().count();
        m_memoryCache[filename] = std::move(entry);

        return true;
    }
    catch (const std::exception& e) {
        XESS_WARNING("Failed to load cached shader {}: {}", filename, e.what());
        return false;
    }
}

void ShaderCache::CacheShader(const std::string& filename, uint64 hash, const std::vector<uint8>& bytecode) {
    try {
        // Create cache directory if it doesn't exist
        std::filesystem::create_directories(m_cacheDirectory);

        std::string cachePath = GetCacheFilePath(filename, hash);
        std::ofstream file(cachePath, std::ios::binary);
        if (!file.is_open()) {
            XESS_WARNING("Failed to create cache file: {}", cachePath);
            return;
        }

        CacheHeader header;
        memcpy(header.magic, CACHE_MAGIC, 4);
        header.version = CACHE_VERSION;
        header.hash = hash;
        header.size = static_cast<uint32>(bytecode.size());

        file.write(reinterpret_cast<const char*>(&header), sizeof(header));
        file.write(reinterpret_cast<const char*>(bytecode.data()), bytecode.size());

        // Update memory cache
        CacheEntry entry;
        entry.bytecode = bytecode;
        entry.hash = hash;
        entry.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        m_memoryCache[filename] = std::move(entry);

        XESS_DEBUG("Cached shader: {}", filename);
    }
    catch (const std::exception& e) {
        XESS_WARNING("Failed to cache shader {}: {}", filename, e.what());
    }
}

void ShaderCache::ClearCache() {
    m_memoryCache.clear();

    try {
        if (std::filesystem::exists(m_cacheDirectory)) {
            std::filesystem::remove_all(m_cacheDirectory);
        }
    }
    catch (const std::exception& e) {
        XESS_WARNING("Failed to clear shader cache: {}", e.what());
    }
}

void ShaderCache::SetCacheDirectory(const std::string& directory) {
    m_cacheDirectory = directory;
    if (!m_cacheDirectory.empty() && m_cacheDirectory.back() != '/' && m_cacheDirectory.back() != '\\') {
        m_cacheDirectory += '/';
    }
}

std::string ShaderCache::GetCacheFilePath(const std::string& filename, uint64 hash) const {
    std::stringstream ss;
    ss << m_cacheDirectory << std::filesystem::path(filename).stem().string()
       << "_" << std::hex << hash << ".cache";
    return ss.str();
}

uint64 ShaderCache::CalculateFileHash(const std::string& filename) const {
    // Simple hash based on filename and file size/modification time
    std::hash<std::string> hasher;
    uint64 hash = hasher(filename);

    try {
        if (std::filesystem::exists(filename)) {
            auto size = std::filesystem::file_size(filename);
            auto time = std::filesystem::last_write_time(filename).time_since_epoch().count();
            hash ^= size + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= time + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
    }
    catch (...) {
        // Ignore filesystem errors
    }

    return hash;
}

// ShaderCompiler Implementation
ShaderCompiler::ShaderCompiler() = default;

ShaderCompiler::~ShaderCompiler() {
    Shutdown();
}

bool ShaderCompiler::Initialize() {
    XESS_INFO("Initializing Shader Compiler with DXC support");

    // Try to initialize DXC first
    HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_dxcUtils));
    if (SUCCEEDED(hr)) {
        hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_dxcCompiler));
        if (SUCCEEDED(hr)) {
            hr = m_dxcUtils->CreateDefaultIncludeHandler(&m_includeHandler);
            if (SUCCEEDED(hr)) {
                XESS_INFO("DXC initialized successfully - Shader Model 6.4 supported");
                m_useLegacyCompiler = false;
                return true;
            }
        }
    }

    // Fallback to legacy compiler
    XESS_WARNING("Failed to initialize DXC, falling back to legacy D3DCompiler");
    XESS_WARNING("Shader Model support limited to 5.1");
    m_useLegacyCompiler = true;

    // Clean up partially initialized DXC
    if (m_includeHandler) { m_includeHandler->Release(); m_includeHandler = nullptr; }
    if (m_dxcCompiler) { m_dxcCompiler->Release(); m_dxcCompiler = nullptr; }
    if (m_dxcUtils) { m_dxcUtils->Release(); m_dxcUtils = nullptr; }

    return true; // Legacy compiler is always available
}

void ShaderCompiler::Shutdown() {
    if (m_includeHandler) {
        m_includeHandler->Release();
        m_includeHandler = nullptr;
    }
    if (m_dxcCompiler) {
        m_dxcCompiler->Release();
        m_dxcCompiler = nullptr;
    }
    if (m_dxcUtils) {
        m_dxcUtils->Release();
        m_dxcUtils = nullptr;
    }

    m_fileTimestamps.clear();
    XESS_INFO("Shader Compiler shut down");
}

CompiledShader ShaderCompiler::CompileFromFile(
    const std::string& filename,
    const std::string& entryPoint,
    ShaderType type,
    const CompileOptions& options) {

    std::string source = LoadShaderSource(filename);
    if (source.empty()) {
        CompiledShader result;
        result.errors.push_back("Failed to load shader file: " + filename);
        return result;
    }

    return CompileFromSource(source, entryPoint, type, options, filename);
}

CompiledShader ShaderCompiler::CompileFromSource(
    const std::string& source,
    const std::string& entryPoint,
    ShaderType type,
    const CompileOptions& options,
    const std::string& sourceName) {

    // Check cache if enabled
    if (m_cacheEnabled && !sourceName.empty()) {
        uint64 hash = CalculateSourceHash(source, options);
        std::vector<uint8> cachedBytecode;
        if (m_cache.GetCachedShader(sourceName, hash, cachedBytecode)) {
            XESS_DEBUG("Using cached shader: {}", sourceName);
            CompiledShader result;
            result.bytecode = std::move(cachedBytecode);
            result.success = true;
            ExtractReflectionData(result.bytecode, result);
            return result;
        }
    }

    CompiledShader result;

    // Use legacy compiler for SM 5.x or if DXC is not available
    if (m_useLegacyCompiler || options.targetModel <= ShaderModel::SM_5_1) {
        result = CompileWithLegacyCompiler(source, entryPoint, type, options, sourceName);
    }
    else {
        // Use DXC for SM 6.0+
        try {
            std::wstring wSource = Utils::StringToWide(source);
            std::wstring wEntryPoint = Utils::StringToWide(entryPoint);
            std::wstring wSourceName = sourceName.empty() ? L"" : Utils::StringToWide(sourceName);
            std::wstring targetProfile = GetTargetProfile(type, options.targetModel);

            // Create source blob
            IDxcBlobEncoding* sourceBlob = nullptr;
            HRESULT hr = m_dxcUtils->CreateBlob(wSource.c_str(),
                static_cast<UINT32>(wSource.length() * sizeof(wchar_t)),
                CP_UTF8, &sourceBlob);

            if (FAILED(hr)) {
                result.errors.push_back("Failed to create source blob");
                return result;
            }

            // Build arguments
            std::vector<LPCWSTR> arguments = BuildCompilerArguments(options);
            arguments.push_back(L"-E");
            arguments.push_back(wEntryPoint.c_str());
            arguments.push_back(L"-T");
            arguments.push_back(targetProfile.c_str());

            // Compile
            DxcBuffer sourceBuffer;
            sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
            sourceBuffer.Size = sourceBlob->GetBufferSize();
            sourceBuffer.Encoding = DXC_CP_ACP;

            IDxcResult* compileResult = nullptr;
            hr = m_dxcCompiler->Compile(&sourceBuffer, arguments.data(),
                static_cast<UINT32>(arguments.size()), m_includeHandler,
                IID_PPV_ARGS(&compileResult));

            sourceBlob->Release();

            if (SUCCEEDED(hr)) {
                // Get compilation status
                HRESULT compileStatus;
                compileResult->GetStatus(&compileStatus);

                // Get bytecode
                if (SUCCEEDED(compileStatus)) {
                    IDxcBlob* bytecodeBlob = nullptr;
                    hr = compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&bytecodeBlob), nullptr);
                    if (SUCCEEDED(hr) && bytecodeBlob) {
                        result.bytecode.resize(bytecodeBlob->GetBufferSize());
                        memcpy(result.bytecode.data(), bytecodeBlob->GetBufferPointer(),
                               bytecodeBlob->GetBufferSize());
                        result.success = true;
                        bytecodeBlob->Release();
                    }
                }

                // Get errors/warnings
                IDxcBlobUtf8* errorBlob = nullptr;
                hr = compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errorBlob), nullptr);
                if (SUCCEEDED(hr) && errorBlob && errorBlob->GetStringLength() > 0) {
                    std::string errorText(errorBlob->GetStringPointer(), errorBlob->GetStringLength());
                    if (FAILED(compileStatus)) {
                        result.errors.push_back(errorText);
                    } else {
                        result.warnings.push_back(errorText);
                    }
                    errorBlob->Release();
                }

                compileResult->Release();
            }
            else {
                result.errors.push_back("DXC compilation failed");
            }
        }
        catch (const std::exception& e) {
            result.errors.push_back("Exception during compilation: " + std::string(e.what()));
        }
    }

    // Extract reflection data if compilation succeeded
    if (result.success) {
        ExtractReflectionData(result.bytecode, result);

        // Cache the result
        if (m_cacheEnabled && !sourceName.empty()) {
            uint64 hash = CalculateSourceHash(source, options);
            m_cache.CacheShader(sourceName, hash, result.bytecode);
        }
    }

    return result;
}

void ShaderCompiler::CheckForShaderChanges() {
    if (!m_hotReloadEnabled) {
        return;
    }

    // This would be implemented to check file modification times
    // and trigger recompilation if needed
    // Implementation depends on how shaders are managed in the application
}

bool ShaderCompiler::IsShaderModelSupported(ShaderModel model) const {
    if (m_useLegacyCompiler) {
        return model <= ShaderModel::SM_5_1;
    }
    return model <= ShaderModel::SM_6_4; // DXC supports up to 6.4
}

ShaderModel ShaderCompiler::GetMaxSupportedShaderModel() const {
    return m_useLegacyCompiler ? ShaderModel::SM_5_1 : ShaderModel::SM_6_4;
}

bool ShaderCompiler::SupportsVariableRateShading() const {
    return !m_useLegacyCompiler; // Requires SM 6.4+
}

bool ShaderCompiler::SupportsMeshShaders() const {
    return !m_useLegacyCompiler; // Requires SM 6.5+ (future)
}

bool ShaderCompiler::SupportsRaytracing() const {
    return !m_useLegacyCompiler; // Requires SM 6.5+ (future)
}

bool ShaderCompiler::SupportsWaveIntrinsics() const {
    return !m_useLegacyCompiler; // Requires SM 6.0+
}

std::wstring ShaderCompiler::GetTargetProfile(ShaderType type, ShaderModel model) const {
    std::wstring profile;

    switch (type) {
        case ShaderType::Vertex: profile = L"vs_"; break;
        case ShaderType::Hull: profile = L"hs_"; break;
        case ShaderType::Domain: profile = L"ds_"; break;
        case ShaderType::Geometry: profile = L"gs_"; break;
        case ShaderType::Pixel: profile = L"ps_"; break;
        case ShaderType::Compute: profile = L"cs_"; break;
        default: profile = L"vs_"; break;
    }

    // Convert shader model to string
    uint32 major = static_cast<uint32>(model) >> 4;
    uint32 minor = static_cast<uint32>(model) & 0xF;

    profile += std::to_wstring(major) + L"_" + std::to_wstring(minor);

    return profile;
}

std::vector<LPCWSTR> ShaderCompiler::BuildCompilerArguments(const CompileOptions& options) const {
    std::vector<LPCWSTR> args;

    if (options.enableDebugInfo) {
        args.push_back(L"-Zi");
        args.push_back(L"-Qembed_debug");
    }

    if (options.enableOptimization) {
        switch (options.optimizationLevel) {
            case 0: args.push_back(L"-O0"); break;
            case 1: args.push_back(L"-O1"); break;
            case 2: args.push_back(L"-O2"); break;
            case 3: args.push_back(L"-O3"); break;
        }
    } else {
        args.push_back(L"-Od");
    }

    if (options.warningsAsErrors) {
        args.push_back(L"-WX");
    }

    if (options.ieee754Compliance) {
        args.push_back(L"-Gis");
    }

    if (options.enableUnboundedResourceArrays) {
        args.push_back(L"-enable-unbounded-descriptor-tables");
    }

    return args;
}

std::string ShaderCompiler::LoadShaderSource(const std::string& filename) const {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            XESS_ERROR("Failed to open shader file: {}", filename);
            return "";
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    catch (const std::exception& e) {
        XESS_ERROR("Exception loading shader file {}: {}", filename, e.what());
        return "";
    }
}

uint64 ShaderCompiler::CalculateSourceHash(const std::string& source, const CompileOptions& options) const {
    std::hash<std::string> hasher;
    uint64 hash = hasher(source);

    // Include options in hash
    hash ^= static_cast<uint64>(options.targetModel) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= (options.enableDebugInfo ? 1 : 0) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= (options.enableOptimization ? 1 : 0) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= options.optimizationLevel + 0x9e3779b9 + (hash << 6) + (hash >> 2);

    // Include macros
    for (const auto& macro : options.macros) {
        hash ^= hasher(macro.name + "=" + macro.definition) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }

    return hash;
}

void ShaderCompiler::ExtractReflectionData(const std::vector<uint8>& bytecode, CompiledShader& shader) const {
    // This would implement shader reflection to extract binding information
    // For now, just clear the reflection data
    shader.inputLayout.clear();
    shader.constantBufferBindings.clear();
    shader.textureBindings.clear();
    shader.samplerBindings.clear();
    shader.uavBindings.clear();
}

CompiledShader ShaderCompiler::CompileWithLegacyCompiler(
    const std::string& source,
    const std::string& entryPoint,
    ShaderType type,
    const CompileOptions& options,
    const std::string& sourceName) const {

    CompiledShader result;

    // Convert to target profile string
    std::string profile;
    switch (type) {
        case ShaderType::Vertex: profile = "vs_5_0"; break;
        case ShaderType::Hull: profile = "hs_5_0"; break;
        case ShaderType::Domain: profile = "ds_5_0"; break;
        case ShaderType::Geometry: profile = "gs_5_0"; break;
        case ShaderType::Pixel: profile = "ps_5_0"; break;
        case ShaderType::Compute: profile = "cs_5_0"; break;
        default:
            result.errors.push_back("Unsupported shader type for legacy compiler");
            return result;
    }

    // Set up compilation flags
    UINT flags = 0;
    if (options.enableDebugInfo) {
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    }
    if (options.enableOptimization && !options.enableDebugInfo) {
        flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
    }
    if (options.warningsAsErrors) {
        flags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
    }
    if (options.ieee754Compliance) {
        flags |= D3DCOMPILE_IEEE_STRICTNESS;
    }

    // Build defines
    std::vector<D3D_SHADER_MACRO> defines;
    for (const auto& macro : options.macros) {
        D3D_SHADER_MACRO define;
        define.Name = macro.name.c_str();
        define.Definition = macro.definition.c_str();
        defines.push_back(define);
    }
    D3D_SHADER_MACRO nullDefine = { nullptr, nullptr };
    defines.push_back(nullDefine);

    // Compile
    ID3DBlob* bytecodeBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    HRESULT hr = D3DCompile(
        source.c_str(),
        source.length(),
        sourceName.empty() ? nullptr : sourceName.c_str(),
        defines.data(),
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint.c_str(),
        profile.c_str(),
        flags,
        0,
        &bytecodeBlob,
        &errorBlob);

    if (SUCCEEDED(hr) && bytecodeBlob) {
        result.bytecode.resize(bytecodeBlob->GetBufferSize());
        memcpy(result.bytecode.data(), bytecodeBlob->GetBufferPointer(), bytecodeBlob->GetBufferSize());
        result.success = true;
        bytecodeBlob->Release();
    }

    if (errorBlob) {
        std::string errorText(static_cast<const char*>(errorBlob->GetBufferPointer()),
                             errorBlob->GetBufferSize());
        if (FAILED(hr)) {
            result.errors.push_back(errorText);
        } else {
            result.warnings.push_back(errorText);
        }
        errorBlob->Release();
    }

    if (FAILED(hr) && result.errors.empty()) {
        result.errors.push_back("Unknown compilation error");
    }

    return result;
}

// Utility functions
std::string ShaderModelToString(ShaderModel model) {
    uint32 major = static_cast<uint32>(model) >> 4;
    uint32 minor = static_cast<uint32>(model) & 0xF;
    return std::to_string(major) + "." + std::to_string(minor);
}

std::string ShaderTypeToString(ShaderType type) {
    switch (type) {
        case ShaderType::Vertex: return "Vertex";
        case ShaderType::Hull: return "Hull";
        case ShaderType::Domain: return "Domain";
        case ShaderType::Geometry: return "Geometry";
        case ShaderType::Pixel: return "Pixel";
        case ShaderType::Compute: return "Compute";
        case ShaderType::Amplification: return "Amplification";
        case ShaderType::Mesh: return "Mesh";
        case ShaderType::RayGeneration: return "RayGeneration";
        case ShaderType::Miss: return "Miss";
        case ShaderType::ClosestHit: return "ClosestHit";
        case ShaderType::AnyHit: return "AnyHit";
        default: return "Unknown";
    }
}

ShaderModel ParseShaderModel(const std::string& str) {
    if (str == "5.0") return ShaderModel::SM_5_0;
    if (str == "5.1") return ShaderModel::SM_5_1;
    if (str == "6.0") return ShaderModel::SM_6_0;
    if (str == "6.1") return ShaderModel::SM_6_1;
    if (str == "6.2") return ShaderModel::SM_6_2;
    if (str == "6.3") return ShaderModel::SM_6_3;
    if (str == "6.4") return ShaderModel::SM_6_4;
    return ShaderModel::SM_5_0;
}

ShaderType ParseShaderType(const std::string& str) {
    if (str == "vs" || str == "vertex") return ShaderType::Vertex;
    if (str == "hs" || str == "hull") return ShaderType::Hull;
    if (str == "ds" || str == "domain") return ShaderType::Domain;
    if (str == "gs" || str == "geometry") return ShaderType::Geometry;
    if (str == "ps" || str == "pixel") return ShaderType::Pixel;
    if (str == "cs" || str == "compute") return ShaderType::Compute;
    return ShaderType::Vertex;
}

} // namespace XeSS::Graphics
