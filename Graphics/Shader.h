#pragma once

#include "Core/Types.h"
#include "Core/NonCopyable.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace XeSS::Graphics {

using Microsoft::WRL::ComPtr;

// Forward declarations
class Device;
class ShaderManager;

// Shader resource binding information
struct ShaderResource {
    std::string name;
    uint32 bindPoint;
    uint32 bindCount;
    D3D_SHADER_INPUT_TYPE type;
    uint32 size; // For constant buffers
};

struct ShaderBinding {
    std::vector<ShaderResource> constantBuffers;
    std::vector<ShaderResource> textures;
    std::vector<ShaderResource> samplers;
    std::vector<ShaderResource> uavs;
    std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayout;
};

// Shader compilation result with DirectX objects
struct CompiledD3DShader {
    CompiledShader compilationResult;
    ShaderBinding binding;

    // D3D11 objects
    ComPtr<ID3D11VertexShader> vertexShader;
    ComPtr<ID3D11HullShader> hullShader;
    ComPtr<ID3D11DomainShader> domainShader;
    ComPtr<ID3D11GeometryShader> geometryShader;
    ComPtr<ID3D11PixelShader> pixelShader;
    ComPtr<ID3D11ComputeShader> computeShader;
    ComPtr<ID3D11InputLayout> inputLayoutD3D;

    bool IsValid() const { return compilationResult.success; }
    ID3DBlob* GetBytecode() const;
};

// Shader parameters for easy binding
class ShaderParameters {
public:
    // Constant buffer data
    void SetConstantBuffer(const std::string& name, const void* data, uint32 size);
    void SetTexture(const std::string& name, ID3D11ShaderResourceView* srv);
    void SetSampler(const std::string& name, ID3D11SamplerState* sampler);
    void SetUAV(const std::string& name, ID3D11UnorderedAccessView* uav);

    // Apply all parameters to the device context
    void Apply(ID3D11DeviceContext* context, const ShaderBinding& binding, ShaderType type);

    // Clear all parameters
    void Clear();

private:
    struct ConstantBufferData {
        std::vector<uint8> data;
        bool dirty = true;
        ComPtr<ID3D11Buffer> buffer;
    };

    std::unordered_map<std::string, ConstantBufferData> m_constantBuffers;
    std::unordered_map<std::string, ComPtr<ID3D11ShaderResourceView>> m_textures;
    std::unordered_map<std::string, ComPtr<ID3D11SamplerState>> m_samplers;
    std::unordered_map<std::string, ComPtr<ID3D11UnorderedAccessView>> m_uavs;

    Device* m_device = nullptr;

    friend class Shader;
};

// Main Shader class
class Shader : public Core::NonCopyable {
public:
    Shader(Device& device, ShaderManager& manager);
    ~Shader();

    // Load from file
    bool LoadFromFile(const std::string& filename, const std::string& entryPoint,
                     ShaderType type, const CompileOptions& options = {});

    // Load from source
    bool LoadFromSource(const std::string& source, const std::string& entryPoint,
                       ShaderType type, const CompileOptions& options = {},
                       const std::string& sourceName = "");

    // Async loading
    void LoadFromFileAsync(const std::string& filename, const std::string& entryPoint,
                          ShaderType type, const CompileOptions& options = {});
    bool IsLoadingComplete() const { return !m_isLoading; }

    // Bind shader to pipeline
    void Bind(ID3D11DeviceContext* context);
    void Unbind(ID3D11DeviceContext* context);

    // Parameter management
    ShaderParameters& GetParameters() { return m_parameters; }
    const ShaderParameters& GetParameters() const { return m_parameters; }

    // Quick parameter setting
    template<typename T>
    void SetConstant(const std::string& name, const T& value) {
        m_parameters.SetConstantBuffer(name, &value, sizeof(T));
    }

    void SetTexture(const std::string& name, ID3D11ShaderResourceView* srv) {
        m_parameters.SetTexture(name, srv);
    }

    void SetSampler(const std::string& name, ID3D11SamplerState* sampler) {
        m_parameters.SetSampler(name, sampler);
    }

    // State queries
    bool IsValid() const { return m_shader && m_shader->IsValid(); }
    bool IsLoading() const { return m_isLoading; }
    ShaderType GetType() const { return m_type; }
    ShaderModel GetShaderModel() const;

    // Compilation info
    const std::vector<std::string>& GetErrors() const;
    const std::vector<std::string>& GetWarnings() const;
    const ShaderBinding& GetBinding() const;

    // Hot reload support
    void EnableHotReload(bool enable) { m_hotReloadEnabled = enable; }
    bool IsHotReloadEnabled() const { return m_hotReloadEnabled; }
    void CheckForReload();

    // Reflection data
    const std::vector<D3D11_INPUT_ELEMENT_DESC>& GetInputLayout() const;
    bool HasConstantBuffer(const std::string& name) const;
    bool HasTexture(const std::string& name) const;
    bool HasSampler(const std::string& name) const;

    // Statistics
    uint32 GetBytecodeSize() const;
    std::string GetDisassembly() const;

private:
    Device& m_device;
    ShaderManager& m_manager;
    ShaderType m_type = ShaderType::Vertex;

    std::unique_ptr<CompiledD3DShader> m_shader;
    ShaderParameters m_parameters;

    // Source tracking for hot reload
    std::string m_sourceFile;
    std::string m_sourceCode;
    std::string m_entryPoint;
    CompileOptions m_compileOptions;
    uint64 m_lastFileTime = 0;
    bool m_hotReloadEnabled = false;

    // Async loading
    bool m_isLoading = false;
    std::future<std::unique_ptr<CompiledD3DShader>> m_loadingFuture;

    // Helper methods
    std::unique_ptr<CompiledD3DShader> CompileShader(
        const std::string& source, const std::string& entryPoint,
        ShaderType type, const CompileOptions& options,
        const std::string& sourceName);

    void CreateD3DShaderObjects(CompiledD3DShader& shader);
    void ExtractShaderReflection(CompiledD3DShader& shader);
    bool UpdateFromAsyncLoad();
    uint64 GetFileTime(const std::string& filename) const;
};

// Shader effect - manages multiple shaders as a unit
class ShaderEffect : public Core::NonCopyable {
public:
    ShaderEffect(Device& device, ShaderManager& manager);
    ~ShaderEffect();

    // Load complete effect from file
    bool LoadFromFile(const std::string& effectFile);

    // Add individual shaders
    void SetVertexShader(std::shared_ptr<Shader> shader) { m_vertexShader = shader; }
    void SetHullShader(std::shared_ptr<Shader> shader) { m_hullShader = shader; }
    void SetDomainShader(std::shared_ptr<Shader> shader) { m_domainShader = shader; }
    void SetGeometryShader(std::shared_ptr<Shader> shader) { m_geometryShader = shader; }
    void SetPixelShader(std::shared_ptr<Shader> shader) { m_pixelShader = shader; }
    void SetComputeShader(std::shared_ptr<Shader> shader) { m_computeShader = shader; }

    // Bind entire effect
    void Bind(ID3D11DeviceContext* context);
    void Unbind(ID3D11DeviceContext* context);

    // Global parameters (applied to all shaders)
    void SetGlobalConstant(const std::string& name, const void* data, uint32 size);
    template<typename T>
    void SetGlobalConstant(const std::string& name, const T& value) {
        SetGlobalConstant(name, &value, sizeof(T));
    }

    void SetGlobalTexture(const std::string& name, ID3D11ShaderResourceView* srv);
    void SetGlobalSampler(const std::string& name, ID3D11SamplerState* sampler);

    // Individual shader access
    Shader* GetVertexShader() const { return m_vertexShader.get(); }
    Shader* GetPixelShader() const { return m_pixelShader.get(); }
    Shader* GetComputeShader() const { return m_computeShader.get(); }

    // State
    bool IsValid() const;
    bool IsLoading() const;

    // Hot reload
    void EnableHotReload(bool enable);
    void CheckForReload();

private:
    Device& m_device;
    ShaderManager& m_manager;

    std::shared_ptr<Shader> m_vertexShader;
    std::shared_ptr<Shader> m_hullShader;
    std::shared_ptr<Shader> m_domainShader;
    std::shared_ptr<Shader> m_geometryShader;
    std::shared_ptr<Shader> m_pixelShader;
    std::shared_ptr<Shader> m_computeShader;

    // Global parameters
    struct GlobalParameter {
        std::vector<uint8> data;
        bool isDirty = true;
    };

    std::unordered_map<std::string, GlobalParameter> m_globalConstants;
    std::unordered_map<std::string, ComPtr<ID3D11ShaderResourceView>> m_globalTextures;
    std::unordered_map<std::string, ComPtr<ID3D11SamplerState>> m_globalSamplers;

    void ApplyGlobalParameters();
};

} // namespace XeSS::Graphics
