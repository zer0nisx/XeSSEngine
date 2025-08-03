#pragma once

#include "Core/Types.h"
#include "Core/NonCopyable.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "Shader.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <future>
#include <filesystem>

namespace XeSS::Graphics {

// Forward declarations
class Device;

// Shader key for caching
struct ShaderKey {
    std::string source;
    std::string entryPoint;
    ShaderType type;
    CompileOptions options;
    uint64 hash;

    ShaderKey(const std::string& src, const std::string& entry,
              ShaderType t, const CompileOptions& opts);

    bool operator==(const ShaderKey& other) const;

    struct Hash {
        size_t operator()(const ShaderKey& key) const {
            return static_cast<size_t>(key.hash);
        }
    };
};

// Shader cache entry
struct ShaderCacheEntry {
    std::shared_ptr<CompiledD3DShader> shader;
    std::chrono::system_clock::time_point creationTime;
    std::chrono::system_clock::time_point lastAccessTime;
    uint32 accessCount = 0;
    bool isPrecompiled = false;
};

// Hot reload file watcher
class FileWatcher {
public:
    struct WatchEntry {
        std::string filename;
        uint64 lastWriteTime;
        std::function<void()> callback;
    };

    void AddWatch(const std::string& filename, std::function<void()> callback);
    void RemoveWatch(const std::string& filename);
    void CheckForChanges();
    void SetEnabled(bool enabled) { m_enabled = enabled; }

private:
    std::vector<WatchEntry> m_watchList;
    bool m_enabled = true;
    std::mutex m_mutex;

    uint64 GetFileWriteTime(const std::string& filename) const;
};

// Shader manager configuration
struct ShaderManagerConfig {
    bool enableAsyncCompilation = true;
    bool enableHotReload = true;
    bool enablePrecompilation = true;
    bool enableStatistics = true;
    uint32 maxCacheSize = 1000; // Maximum number of cached shaders
    uint32 maxMemoryMB = 256;    // Maximum cache memory usage
    std::string cacheDirectory = "cache/shaders/";
    std::string shaderDirectory = "shaders/";
    ShaderModel defaultShaderModel = ShaderModel::SM_6_4;
    uint32 hotReloadCheckIntervalMs = 1000;
};

// Shader compilation statistics
struct ShaderStatistics {
    uint32 totalCompilations = 0;
    uint32 cacheHits = 0;
    uint32 cacheMisses = 0;
    uint32 compilationErrors = 0;
    uint32 asyncCompilations = 0;
    uint32 hotReloads = 0;

    double averageCompileTimeMs = 0.0;
    double totalCompileTimeMs = 0.0;

    uint32 currentCacheSize = 0;
    uint32 currentMemoryUsageMB = 0;

    void Reset() { *this = ShaderStatistics{}; }
    double GetCacheHitRatio() const {
        uint32 total = cacheHits + cacheMisses;
        return total > 0 ? static_cast<double>(cacheHits) / total : 0.0;
    }
};

// Main shader manager class
class ShaderManager : public Core::NonCopyable {
public:
    ShaderManager(Device& device);
    ~ShaderManager();

    // Initialization
    bool Initialize(const ShaderManagerConfig& config = {});
    void Shutdown();

    // Shader compilation
    std::shared_ptr<CompiledD3DShader> CompileShader(
        const std::string& source, const std::string& entryPoint,
        ShaderType type, const CompileOptions& options = {},
        const std::string& sourceName = "");

    std::shared_ptr<CompiledD3DShader> CompileShaderFromFile(
        const std::string& filename, const std::string& entryPoint,
        ShaderType type, const CompileOptions& options = {});

    // Async compilation
    std::future<std::shared_ptr<CompiledD3DShader>> CompileShaderAsync(
        const std::string& source, const std::string& entryPoint,
        ShaderType type, const CompileOptions& options = {},
        const std::string& sourceName = "");

    // Shader creation (high-level interface)
    std::shared_ptr<Shader> CreateShader(
        const std::string& filename, const std::string& entryPoint,
        ShaderType type, const CompileOptions& options = {});

    std::shared_ptr<Shader> CreateShaderFromSource(
        const std::string& source, const std::string& entryPoint,
        ShaderType type, const CompileOptions& options = {},
        const std::string& sourceName = "");

    // Effect creation
    std::shared_ptr<ShaderEffect> CreateEffect(const std::string& effectFile);

    // Precompilation system
    void PrecompileShader(const std::string& filename, const std::string& entryPoint,
                         ShaderType type, const CompileOptions& options = {});
    void PrecompileDirectory(const std::string& directory, bool recursive = true);
    void PrecompileFromManifest(const std::string& manifestFile);

    // Cache management
    void ClearCache();
    void CompactCache(); // Remove least recently used entries
    void SaveCacheToDisk();
    void LoadCacheFromDisk();

    // Hot reload system
    void EnableHotReload(bool enable);
    void CheckForFileChanges();
    void RegisterFileWatch(const std::string& filename, std::function<void()> callback);
    void UnregisterFileWatch(const std::string& filename);

    // Configuration
    const ShaderManagerConfig& GetConfig() const { return m_config; }
    void UpdateConfig(const ShaderManagerConfig& config);

    // Statistics
    const ShaderStatistics& GetStatistics() const { return m_statistics; }
    void ResetStatistics() { m_statistics.Reset(); }

    // Utility
    ShaderCompiler& GetCompiler() { return m_compiler; }
    Device& GetDevice() { return m_device; }

    // Shader model support
    bool IsShaderModelSupported(ShaderModel model) const {
        return m_compiler.IsShaderModelSupported(model);
    }

    ShaderModel GetMaxSupportedShaderModel() const {
        return m_compiler.GetMaxSupportedShaderModel();
    }

    // Include system
    void AddIncludeDirectory(const std::string& directory);
    void RemoveIncludeDirectory(const std::string& directory);
    std::string ResolveIncludePath(const std::string& filename) const;

private:
    Device& m_device;
    ShaderCompiler m_compiler;
    ShaderManagerConfig m_config;
    ShaderStatistics m_statistics;

    // Cache system
    std::unordered_map<ShaderKey, ShaderCacheEntry, ShaderKey::Hash> m_shaderCache;
    std::mutex m_cacheMutex;

    // Hot reload system
    FileWatcher m_fileWatcher;
    std::chrono::steady_clock::time_point m_lastHotReloadCheck;

    // Include directories
    std::vector<std::string> m_includeDirectories;

    // Async compilation
    std::vector<std::future<void>> m_asyncTasks;

    // Helper methods
    ShaderKey CreateShaderKey(const std::string& source, const std::string& entryPoint,
                             ShaderType type, const CompileOptions& options) const;

    std::shared_ptr<CompiledD3DShader> GetFromCache(const ShaderKey& key);
    void AddToCache(const ShaderKey& key, std::shared_ptr<CompiledD3DShader> shader);
    void UpdateCacheStatistics();

    std::shared_ptr<CompiledD3DShader> CompileShaderInternal(
        const std::string& source, const std::string& entryPoint,
        ShaderType type, const CompileOptions& options,
        const std::string& sourceName);

    void CreateD3DShaderFromBytecode(CompiledD3DShader& shader);
    void ExtractShaderReflection(CompiledD3DShader& shader);

    std::string LoadShaderSource(const std::string& filename) const;
    std::string ProcessIncludes(const std::string& source, const std::string& baseDir) const;

    // Cache persistence
    void SerializeCache(const std::string& filename) const;
    void DeserializeCache(const std::string& filename);

    // Cleanup
    void CleanupAsyncTasks();
    void RemoveLeastRecentlyUsed();

    // Statistics tracking
    void RecordCompilation(double timeMs, bool fromCache, bool error = false);
    void RecordCacheAccess(const ShaderKey& key);
};

// Global shader manager instance
class GlobalShaderManager {
public:
    static void Initialize(Device& device, const ShaderManagerConfig& config = {});
    static void Shutdown();
    static ShaderManager& Instance();
    static bool IsInitialized();

private:
    static std::unique_ptr<ShaderManager> s_instance;
    static bool s_initialized;
};

// Helper macros for easy shader creation
#define CREATE_SHADER(filename, entry, type, ...) \
    GlobalShaderManager::Instance().CreateShader(filename, entry, type, ##__VA_ARGS__)

#define CREATE_VERTEX_SHADER(filename, entry, ...) \
    CREATE_SHADER(filename, entry, ShaderType::Vertex, ##__VA_ARGS__)

#define CREATE_PIXEL_SHADER(filename, entry, ...) \
    CREATE_SHADER(filename, entry, ShaderType::Pixel, ##__VA_ARGS__)

#define CREATE_COMPUTE_SHADER(filename, entry, ...) \
    CREATE_SHADER(filename, entry, ShaderType::Compute, ##__VA_ARGS__)

} // namespace XeSS::Graphics
