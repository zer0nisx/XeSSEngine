#include "Shader.h"
#include "Device.h"
#include "ShaderManager.h"
#include "Core/Logger.h"
#include "Core/Exception.h"
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include <filesystem>
#include <fstream>
#include <future>

namespace XeSS::Graphics {

// ShaderParameters Implementation
void ShaderParameters::SetConstantBuffer(const std::string& name, const void* data, uint32 size) {
    auto& cbData = m_constantBuffers[name];
    cbData.data.resize(size);
    memcpy(cbData.data.data(), data, size);
    cbData.dirty = true;
}

void ShaderParameters::SetTexture(const std::string& name, ID3D11ShaderResourceView* srv) {
    m_textures[name] = srv;
}

void ShaderParameters::SetSampler(const std::string& name, ID3D11SamplerState* sampler) {
    m_samplers[name] = sampler;
}

void ShaderParameters::SetUAV(const std::string& name, ID3D11UnorderedAccessView* uav) {
    m_uavs[name] = uav;
}

void ShaderParameters::Apply(ID3D11DeviceContext* context, const ShaderBinding& binding, ShaderType type) {
    if (!context || !m_device) return;

    // Apply constant buffers
    for (const auto& cb : binding.constantBuffers) {
        auto it = m_constantBuffers.find(cb.name);
        if (it != m_constantBuffers.end()) {
            auto& cbData = it->second;

            // Create or update buffer if dirty
            if (cbData.dirty || !cbData.buffer) {
                D3D11_BUFFER_DESC desc = {};
                desc.ByteWidth = static_cast<UINT>((cbData.data.size() + 15) & ~15); // Align to 16 bytes
                desc.Usage = D3D11_USAGE_DYNAMIC;
                desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

                D3D11_SUBRESOURCE_DATA initData = {};
                initData.pSysMem = cbData.data.data();

                HRESULT hr = m_device->GetDevice()->CreateBuffer(&desc, &initData, &cbData.buffer);
                if (SUCCEEDED(hr)) {
                    cbData.dirty = false;
                } else {
                    XESS_ERROR("Failed to create constant buffer for '{}'", cb.name);
                    continue;
                }
            }

            // Bind to appropriate shader stage
            switch (type) {
                case ShaderType::Vertex:
                    context->VSSetConstantBuffers(cb.bindPoint, 1, cbData.buffer.GetAddressOf());
                    break;
                case ShaderType::Hull:
                    context->HSSetConstantBuffers(cb.bindPoint, 1, cbData.buffer.GetAddressOf());
                    break;
                case ShaderType::Domain:
                    context->DSSetConstantBuffers(cb.bindPoint, 1, cbData.buffer.GetAddressOf());
                    break;
                case ShaderType::Geometry:
                    context->GSSetConstantBuffers(cb.bindPoint, 1, cbData.buffer.GetAddressOf());
                    break;
                case ShaderType::Pixel:
                    context->PSSetConstantBuffers(cb.bindPoint, 1, cbData.buffer.GetAddressOf());
                    break;
                case ShaderType::Compute:
                    context->CSSetConstantBuffers(cb.bindPoint, 1, cbData.buffer.GetAddressOf());
                    break;
            }
        }
    }

    // Apply textures
    for (const auto& tex : binding.textures) {
        auto it = m_textures.find(tex.name);
        if (it != m_textures.end()) {
            ID3D11ShaderResourceView* srv = it->second.Get();

            switch (type) {
                case ShaderType::Vertex:
                    context->VSSetShaderResources(tex.bindPoint, 1, &srv);
                    break;
                case ShaderType::Hull:
                    context->HSSetShaderResources(tex.bindPoint, 1, &srv);
                    break;
                case ShaderType::Domain:
                    context->DSSetShaderResources(tex.bindPoint, 1, &srv);
                    break;
                case ShaderType::Geometry:
                    context->GSSetShaderResources(tex.bindPoint, 1, &srv);
                    break;
                case ShaderType::Pixel:
                    context->PSSetShaderResources(tex.bindPoint, 1, &srv);
                    break;
                case ShaderType::Compute:
                    context->CSSetShaderResources(tex.bindPoint, 1, &srv);
                    break;
            }
        }
    }

    // Apply samplers
    for (const auto& samp : binding.samplers) {
        auto it = m_samplers.find(samp.name);
        if (it != m_samplers.end()) {
            ID3D11SamplerState* sampler = it->second.Get();

            switch (type) {
                case ShaderType::Vertex:
                    context->VSSetSamplers(samp.bindPoint, 1, &sampler);
                    break;
                case ShaderType::Hull:
                    context->HSSetSamplers(samp.bindPoint, 1, &sampler);
                    break;
                case ShaderType::Domain:
                    context->DSSetSamplers(samp.bindPoint, 1, &sampler);
                    break;
                case ShaderType::Geometry:
                    context->GSSetSamplers(samp.bindPoint, 1, &sampler);
                    break;
                case ShaderType::Pixel:
                    context->PSSetSamplers(samp.bindPoint, 1, &sampler);
                    break;
                case ShaderType::Compute:
                    context->CSSetSamplers(samp.bindPoint, 1, &sampler);
                    break;
            }
        }
    }

    // Apply UAVs (compute shaders only)
    if (type == ShaderType::Compute) {
        for (const auto& uav : binding.uavs) {
            auto it = m_uavs.find(uav.name);
            if (it != m_uavs.end()) {
                ID3D11UnorderedAccessView* uavPtr = it->second.Get();
                context->CSSetUnorderedAccessViews(uav.bindPoint, 1, &uavPtr, nullptr);
            }
        }
    }
}

void ShaderParameters::Clear() {
    m_constantBuffers.clear();
    m_textures.clear();
    m_samplers.clear();
    m_uavs.clear();
}

// CompiledD3DShader Implementation
ID3DBlob* CompiledD3DShader::GetBytecode() const {
    // This would require storing the ID3DBlob separately
    // For now, return nullptr - bytecode is stored in compilationResult.bytecode
    return nullptr;
}

// Shader Implementation
Shader::Shader(Device& device, ShaderManager& manager)
    : m_device(device), m_manager(manager) {
    m_parameters.m_device = &device;
}

Shader::~Shader() {
    if (m_isLoading && m_loadingFuture.valid()) {
        m_loadingFuture.wait();
    }
}

bool Shader::LoadFromFile(const std::string& filename, const std::string& entryPoint,
                         ShaderType type, const CompileOptions& options) {
    m_sourceFile = filename;
    m_entryPoint = entryPoint;
    m_type = type;
    m_compileOptions = options;

    try {
        // Load source from file
        std::ifstream file(filename);
        if (!file.is_open()) {
            XESS_ERROR("Failed to open shader file: {}", filename);
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        m_sourceCode = buffer.str();

        // Compile shader
        m_shader = CompileShader(m_sourceCode, entryPoint, type, options, filename);

        if (m_shader && m_shader->IsValid()) {
            m_lastFileTime = GetFileTime(filename);
            XESS_INFO("Shader loaded successfully: {}", filename);
            return true;
        } else {
            XESS_ERROR("Failed to compile shader: {}", filename);
            return false;
        }
    }
    catch (const std::exception& e) {
        XESS_ERROR("Exception loading shader {}: {}", filename, e.what());
        return false;
    }
}

bool Shader::LoadFromSource(const std::string& source, const std::string& entryPoint,
                           ShaderType type, const CompileOptions& options,
                           const std::string& sourceName) {
    m_sourceCode = source;
    m_entryPoint = entryPoint;
    m_type = type;
    m_compileOptions = options;
    m_sourceFile = sourceName; // For debugging

    m_shader = CompileShader(source, entryPoint, type, options, sourceName);

    if (m_shader && m_shader->IsValid()) {
        XESS_INFO("Shader compiled from source successfully");
        return true;
    } else {
        XESS_ERROR("Failed to compile shader from source");
        return false;
    }
}

void Shader::LoadFromFileAsync(const std::string& filename, const std::string& entryPoint,
                              ShaderType type, const CompileOptions& options) {
    m_sourceFile = filename;
    m_entryPoint = entryPoint;
    m_type = type;
    m_compileOptions = options;
    m_isLoading = true;

    m_loadingFuture = std::async(std::launch::async, [this, filename, entryPoint, type, options]() {
        try {
            std::ifstream file(filename);
            if (!file.is_open()) {
                XESS_ERROR("Failed to open shader file: {}", filename);
                return std::unique_ptr<CompiledD3DShader>(nullptr);
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string source = buffer.str();

            return CompileShader(source, entryPoint, type, options, filename);
        }
        catch (const std::exception& e) {
            XESS_ERROR("Exception in async shader loading {}: {}", filename, e.what());
            return std::unique_ptr<CompiledD3DShader>(nullptr);
        }
    });
}

void Shader::Bind(ID3D11DeviceContext* context) {
    if (!IsValid()) {
        XESS_WARNING("Attempting to bind invalid shader");
        return;
    }

    // Update from async loading if needed
    UpdateFromAsyncLoad();

    if (!m_shader || !m_shader->IsValid()) {
        return;
    }

    // Bind the appropriate shader type
    switch (m_type) {
        case ShaderType::Vertex:
            context->VSSetShader(m_shader->vertexShader.Get(), nullptr, 0);
            if (m_shader->inputLayoutD3D) {
                context->IASetInputLayout(m_shader->inputLayoutD3D.Get());
            }
            break;
        case ShaderType::Hull:
            context->HSSetShader(m_shader->hullShader.Get(), nullptr, 0);
            break;
        case ShaderType::Domain:
            context->DSSetShader(m_shader->domainShader.Get(), nullptr, 0);
            break;
        case ShaderType::Geometry:
            context->GSSetShader(m_shader->geometryShader.Get(), nullptr, 0);
            break;
        case ShaderType::Pixel:
            context->PSSetShader(m_shader->pixelShader.Get(), nullptr, 0);
            break;
        case ShaderType::Compute:
            context->CSSetShader(m_shader->computeShader.Get(), nullptr, 0);
            break;
    }

    // Apply parameters
    m_parameters.Apply(context, m_shader->binding, m_type);
}

void Shader::Unbind(ID3D11DeviceContext* context) {
    switch (m_type) {
        case ShaderType::Vertex:
            context->VSSetShader(nullptr, nullptr, 0);
            break;
        case ShaderType::Hull:
            context->HSSetShader(nullptr, nullptr, 0);
            break;
        case ShaderType::Domain:
            context->DSSetShader(nullptr, nullptr, 0);
            break;
        case ShaderType::Geometry:
            context->GSSetShader(nullptr, nullptr, 0);
            break;
        case ShaderType::Pixel:
            context->PSSetShader(nullptr, nullptr, 0);
            break;
        case ShaderType::Compute:
            context->CSSetShader(nullptr, nullptr, 0);
            break;
    }
}

ShaderModel Shader::GetShaderModel() const {
    return m_compileOptions.targetModel;
}

const std::vector<std::string>& Shader::GetErrors() const {
    static const std::vector<std::string> empty;
    return m_shader ? m_shader->compilationResult.errors : empty;
}

const std::vector<std::string>& Shader::GetWarnings() const {
    static const std::vector<std::string> empty;
    return m_shader ? m_shader->compilationResult.warnings : empty;
}

const ShaderBinding& Shader::GetBinding() const {
    static const ShaderBinding empty;
    return m_shader ? m_shader->binding : empty;
}

void Shader::CheckForReload() {
    if (!m_hotReloadEnabled || m_sourceFile.empty()) {
        return;
    }

    uint64 currentFileTime = GetFileTime(m_sourceFile);
    if (currentFileTime > m_lastFileTime) {
        XESS_INFO("Hot reloading shader: {}", m_sourceFile);

        if (LoadFromFile(m_sourceFile, m_entryPoint, m_type, m_compileOptions)) {
            XESS_INFO("Shader hot reload successful: {}", m_sourceFile);
        } else {
            XESS_ERROR("Shader hot reload failed: {}", m_sourceFile);
        }
    }
}

const std::vector<D3D11_INPUT_ELEMENT_DESC>& Shader::GetInputLayout() const {
    static const std::vector<D3D11_INPUT_ELEMENT_DESC> empty;
    return m_shader ? m_shader->binding.inputLayout : empty;
}

bool Shader::HasConstantBuffer(const std::string& name) const {
    if (!m_shader) return false;

    for (const auto& cb : m_shader->binding.constantBuffers) {
        if (cb.name == name) return true;
    }
    return false;
}

bool Shader::HasTexture(const std::string& name) const {
    if (!m_shader) return false;

    for (const auto& tex : m_shader->binding.textures) {
        if (tex.name == name) return true;
    }
    return false;
}

bool Shader::HasSampler(const std::string& name) const {
    if (!m_shader) return false;

    for (const auto& samp : m_shader->binding.samplers) {
        if (samp.name == name) return true;
    }
    return false;
}

uint32 Shader::GetBytecodeSize() const {
    return m_shader ? static_cast<uint32>(m_shader->compilationResult.bytecode.size()) : 0;
}

std::string Shader::GetDisassembly() const {
    return m_shader ? m_shader->compilationResult.disassembly : "";
}

std::unique_ptr<CompiledD3DShader> Shader::CompileShader(
    const std::string& source, const std::string& entryPoint,
    ShaderType type, const CompileOptions& options,
    const std::string& sourceName) {

    // Use shader manager for compilation (handles caching)
    auto compiledShader = m_manager.CompileShader(source, entryPoint, type, options, sourceName);

    return compiledShader;
}

void Shader::CreateD3DShaderObjects(CompiledD3DShader& shader) {
    if (shader.compilationResult.bytecode.empty()) {
        return;
    }

    const void* bytecode = shader.compilationResult.bytecode.data();
    size_t bytecodeSize = shader.compilationResult.bytecode.size();

    HRESULT hr = S_OK;

    switch (m_type) {
        case ShaderType::Vertex:
            hr = m_device.GetDevice()->CreateVertexShader(bytecode, bytecodeSize, nullptr, &shader.vertexShader);
            break;
        case ShaderType::Hull:
            hr = m_device.GetDevice()->CreateHullShader(bytecode, bytecodeSize, nullptr, &shader.hullShader);
            break;
        case ShaderType::Domain:
            hr = m_device.GetDevice()->CreateDomainShader(bytecode, bytecodeSize, nullptr, &shader.domainShader);
            break;
        case ShaderType::Geometry:
            hr = m_device.GetDevice()->CreateGeometryShader(bytecode, bytecodeSize, nullptr, &shader.geometryShader);
            break;
        case ShaderType::Pixel:
            hr = m_device.GetDevice()->CreatePixelShader(bytecode, bytecodeSize, nullptr, &shader.pixelShader);
            break;
        case ShaderType::Compute:
            hr = m_device.GetDevice()->CreateComputeShader(bytecode, bytecodeSize, nullptr, &shader.computeShader);
            break;
    }

    if (FAILED(hr)) {
        XESS_ERROR("Failed to create D3D11 shader object: 0x{:08X}", hr);
        shader.compilationResult.success = false;
    }
}

void Shader::ExtractShaderReflection(CompiledD3DShader& shader) {
    if (shader.compilationResult.bytecode.empty()) {
        return;
    }

    // This would implement shader reflection to extract binding information
    // For now, we'll use a simplified version

    // TODO: Implement full shader reflection using ID3D11ShaderReflection
    // This would extract:
    // - Constant buffer layouts
    // - Resource bindings
    // - Input layouts for vertex shaders
    // - UAV bindings for compute shaders
}

bool Shader::UpdateFromAsyncLoad() {
    if (!m_isLoading) {
        return false;
    }

    if (m_loadingFuture.valid() &&
        m_loadingFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {

        m_shader = m_loadingFuture.get();
        m_isLoading = false;

        if (m_shader && m_shader->IsValid()) {
            m_lastFileTime = GetFileTime(m_sourceFile);
            XESS_INFO("Async shader loading completed: {}", m_sourceFile);
            return true;
        } else {
            XESS_ERROR("Async shader loading failed: {}", m_sourceFile);
            return false;
        }
    }

    return false;
}

uint64 Shader::GetFileTime(const std::string& filename) const {
    try {
        if (std::filesystem::exists(filename)) {
            auto time = std::filesystem::last_write_time(filename);
            return time.time_since_epoch().count();
        }
    }
    catch (...) {
        // Ignore filesystem errors
    }
    return 0;
}

// ShaderEffect Implementation
ShaderEffect::ShaderEffect(Device& device, ShaderManager& manager)
    : m_device(device), m_manager(manager) {
}

ShaderEffect::~ShaderEffect() = default;

bool ShaderEffect::LoadFromFile(const std::string& effectFile) {
    // This would load an effect file format that describes multiple shaders
    // For now, return false as this is not implemented
    XESS_WARNING("ShaderEffect::LoadFromFile not yet implemented");
    return false;
}

void ShaderEffect::Bind(ID3D11DeviceContext* context) {
    if (m_vertexShader) m_vertexShader->Bind(context);
    if (m_hullShader) m_hullShader->Bind(context);
    if (m_domainShader) m_domainShader->Bind(context);
    if (m_geometryShader) m_geometryShader->Bind(context);
    if (m_pixelShader) m_pixelShader->Bind(context);
    if (m_computeShader) m_computeShader->Bind(context);

    ApplyGlobalParameters();
}

void ShaderEffect::Unbind(ID3D11DeviceContext* context) {
    if (m_vertexShader) m_vertexShader->Unbind(context);
    if (m_hullShader) m_hullShader->Unbind(context);
    if (m_domainShader) m_domainShader->Unbind(context);
    if (m_geometryShader) m_geometryShader->Unbind(context);
    if (m_pixelShader) m_pixelShader->Unbind(context);
    if (m_computeShader) m_computeShader->Unbind(context);
}

void ShaderEffect::SetGlobalConstant(const std::string& name, const void* data, uint32 size) {
    auto& param = m_globalConstants[name];
    param.data.resize(size);
    memcpy(param.data.data(), data, size);
    param.isDirty = true;
}

void ShaderEffect::SetGlobalTexture(const std::string& name, ID3D11ShaderResourceView* srv) {
    m_globalTextures[name] = srv;
}

void ShaderEffect::SetGlobalSampler(const std::string& name, ID3D11SamplerState* sampler) {
    m_globalSamplers[name] = sampler;
}

bool ShaderEffect::IsValid() const {
    // At least one shader should be valid
    return (m_vertexShader && m_vertexShader->IsValid()) ||
           (m_hullShader && m_hullShader->IsValid()) ||
           (m_domainShader && m_domainShader->IsValid()) ||
           (m_geometryShader && m_geometryShader->IsValid()) ||
           (m_pixelShader && m_pixelShader->IsValid()) ||
           (m_computeShader && m_computeShader->IsValid());
}

bool ShaderEffect::IsLoading() const {
    return (m_vertexShader && m_vertexShader->IsLoading()) ||
           (m_hullShader && m_hullShader->IsLoading()) ||
           (m_domainShader && m_domainShader->IsLoading()) ||
           (m_geometryShader && m_geometryShader->IsLoading()) ||
           (m_pixelShader && m_pixelShader->IsLoading()) ||
           (m_computeShader && m_computeShader->IsLoading());
}

void ShaderEffect::EnableHotReload(bool enable) {
    if (m_vertexShader) m_vertexShader->EnableHotReload(enable);
    if (m_hullShader) m_hullShader->EnableHotReload(enable);
    if (m_domainShader) m_domainShader->EnableHotReload(enable);
    if (m_geometryShader) m_geometryShader->EnableHotReload(enable);
    if (m_pixelShader) m_pixelShader->EnableHotReload(enable);
    if (m_computeShader) m_computeShader->EnableHotReload(enable);
}

void ShaderEffect::CheckForReload() {
    if (m_vertexShader) m_vertexShader->CheckForReload();
    if (m_hullShader) m_hullShader->CheckForReload();
    if (m_domainShader) m_domainShader->CheckForReload();
    if (m_geometryShader) m_geometryShader->CheckForReload();
    if (m_pixelShader) m_pixelShader->CheckForReload();
    if (m_computeShader) m_computeShader->CheckForReload();
}

void ShaderEffect::ApplyGlobalParameters() {
    // Apply global constants to all shaders
    for (const auto& [name, param] : m_globalConstants) {
        if (m_vertexShader && m_vertexShader->HasConstantBuffer(name)) {
            m_vertexShader->GetParameters().SetConstantBuffer(name, param.data.data(),
                                                              static_cast<uint32>(param.data.size()));
        }
        if (m_pixelShader && m_pixelShader->HasConstantBuffer(name)) {
            m_pixelShader->GetParameters().SetConstantBuffer(name, param.data.data(),
                                                             static_cast<uint32>(param.data.size()));
        }
        if (m_computeShader && m_computeShader->HasConstantBuffer(name)) {
            m_computeShader->GetParameters().SetConstantBuffer(name, param.data.data(),
                                                               static_cast<uint32>(param.data.size()));
        }
    }

    // Apply global textures
    for (const auto& [name, srv] : m_globalTextures) {
        if (m_vertexShader && m_vertexShader->HasTexture(name)) {
            m_vertexShader->SetTexture(name, srv.Get());
        }
        if (m_pixelShader && m_pixelShader->HasTexture(name)) {
            m_pixelShader->SetTexture(name, srv.Get());
        }
        if (m_computeShader && m_computeShader->HasTexture(name)) {
            m_computeShader->SetTexture(name, srv.Get());
        }
    }

    // Apply global samplers
    for (const auto& [name, sampler] : m_globalSamplers) {
        if (m_vertexShader && m_vertexShader->HasSampler(name)) {
            m_vertexShader->SetSampler(name, sampler.Get());
        }
        if (m_pixelShader && m_pixelShader->HasSampler(name)) {
            m_pixelShader->SetSampler(name, sampler.Get());
        }
        if (m_computeShader && m_computeShader->HasSampler(name)) {
            m_computeShader->SetSampler(name, sampler.Get());
        }
    }
}

} // namespace XeSS::Graphics
