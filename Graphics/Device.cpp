#include "Device.h"
#include "ShaderManager.h"
#include "Core/Logger.h"
#include "Core/Utils.h"

namespace XeSS::Graphics {

Device::Device() = default;

Device::~Device() {
    Shutdown();
}

void Device::Initialize(int32 adapterId, bool useWarp, bool enableDebug) {
    if (m_initialized) {
        XESS_WARNING("Device already initialized");
        return;
    }

    XESS_INFO("Initializing DirectX 11 device...");

    try {
        CreateFactory();
        SelectAdapter(adapterId, useWarp);
        CreateDevice(enableDebug);
        QueryAdapterInfo();
        InitializeShaderManager();

        m_initialized = true;

        XESS_INFO("DirectX 11 device initialized successfully");
        XESS_INFO("Adapter: {}", Utils::WideToString(m_adapterInfo.description));
        XESS_INFO("Feature Level: {}", static_cast<uint32>(m_featureLevel));
        XESS_INFO("Video Memory: {} MB", m_adapterInfo.dedicatedVideoMemory / (1024 * 1024));
    }
    catch (const Exception& e) {
        XESS_ERROR("Failed to initialize DirectX 11 device: {}", e.what());
        throw;
    }
}

void Device::Shutdown() {
    if (!m_initialized) {
        return;
    }

    XESS_INFO("Shutting down DirectX 11 device");

    // Shutdown shader manager first
    if (m_shaderManager) {
        m_shaderManager->Shutdown();
        m_shaderManager.reset();
    }

    if (m_context) {
        m_context->ClearState();
        m_context->Flush();
    }

    m_context.Reset();
    m_device.Reset();
    m_adapter.Reset();
    m_factory.Reset();

    m_initialized = false;
}

void Device::CreateFactory() {
    XESS_THROW_IF_FAILED(
        CreateDXGIFactory1(IID_PPV_ARGS(&m_factory)),
        "Failed to create DXGI factory"
    );
}

void Device::SelectAdapter(int32 adapterId, bool useWarp) {
    if (useWarp) {
        XESS_INFO("Using WARP adapter (software)");
        // WARP adapter will be handled in CreateDevice
        return;
    }

    auto adapters = EnumerateAdapters();

    if (adapterId >= 0) {
        if (adapterId >= static_cast<int32>(adapters.size())) {
            throw GraphicsException("Invalid adapter ID: " + std::to_string(adapterId));
        }

        XESS_THROW_IF_FAILED(
            m_factory->EnumAdapters1(adapterId, &m_adapter),
            "Failed to get specified adapter"
        );

        XESS_INFO("Using adapter {}: {}", adapterId,
                  Utils::WideToString(adapters[adapterId].description));
    } else {
        // Auto-select best adapter
        uint32 bestScore = 0;
        uint32 bestIndex = 0;

        for (uint32 i = 0; i < adapters.size(); ++i) {
            const auto& info = adapters[i];
            if (info.isSoftware) continue;

            uint32 score = static_cast<uint32>(info.dedicatedVideoMemory / (1024 * 1024));
            if (score > bestScore) {
                bestScore = score;
                bestIndex = i;
            }
        }

        XESS_THROW_IF_FAILED(
            m_factory->EnumAdapters1(bestIndex, &m_adapter),
            "Failed to get best adapter"
        );

        XESS_INFO("Auto-selected adapter {}: {}", bestIndex,
                  Utils::WideToString(adapters[bestIndex].description));
    }
}

void Device::CreateDevice(bool enableDebug) {
    uint32 createFlags = 0;

    if (enableDebug) {
        createFlags |= D3D11_CREATE_DEVICE_DEBUG;
    }

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    D3D_DRIVER_TYPE driverType = m_adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE;
    if (!m_adapter) {
        // Use WARP if no adapter selected
        driverType = D3D_DRIVER_TYPE_WARP;
    }

    HRESULT hr = D3D11CreateDevice(
        m_adapter.Get(),
        driverType,
        nullptr,
        createFlags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &m_device,
        &m_featureLevel,
        &m_context
    );

    XESS_THROW_IF_FAILED(hr, "Failed to create DirectX 11 device");
}

void Device::QueryAdapterInfo() {
    if (!m_adapter) {
        // WARP adapter info
        m_adapterInfo.description = L"Microsoft Basic Render Driver (WARP)";
        m_adapterInfo.isSoftware = true;
        return;
    }

    DXGI_ADAPTER_DESC1 desc;
    XESS_THROW_IF_FAILED(
        m_adapter->GetDesc1(&desc),
        "Failed to get adapter description"
    );

    m_adapterInfo.description = desc.Description;
    m_adapterInfo.vendorId = desc.VendorId;
    m_adapterInfo.deviceId = desc.DeviceId;
    m_adapterInfo.dedicatedVideoMemory = desc.DedicatedVideoMemory;
    m_adapterInfo.dedicatedSystemMemory = desc.DedicatedSystemMemory;
    m_adapterInfo.sharedSystemMemory = desc.SharedSystemMemory;
    m_adapterInfo.isSoftware = (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
}

std::vector<AdapterInfo> Device::EnumerateAdapters() const {
    std::vector<AdapterInfo> adapters;

    ComPtr<IDXGIAdapter1> adapter;
    for (uint32 i = 0; m_factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        if (SUCCEEDED(adapter->GetDesc1(&desc))) {
            AdapterInfo info;
            info.description = desc.Description;
            info.vendorId = desc.VendorId;
            info.deviceId = desc.DeviceId;
            info.dedicatedVideoMemory = desc.DedicatedVideoMemory;
            info.dedicatedSystemMemory = desc.DedicatedSystemMemory;
            info.sharedSystemMemory = desc.SharedSystemMemory;
            info.isSoftware = (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;

            adapters.push_back(std::move(info));
        }
        adapter.Reset();
    }

    return adapters;
}

bool Device::IsFeatureSupported(D3D11_FEATURE feature) const {
    if (!m_device) return false;

    // This would need specific implementation for each feature type
    // For now, just return false as a placeholder
    return false;
}

} // namespace XeSS::Graphics
