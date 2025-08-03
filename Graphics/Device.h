#pragma once

#include "Core/Types.h"
#include "Core/NonCopyable.h"
#include "Core/Exception.h"
#include <d3d11.h>
#include <dxgi1_3.h>
#include <wrl/client.h>
#include <memory>
#include <vector>

namespace XeSS::Graphics {

using Microsoft::WRL::ComPtr;

struct AdapterInfo {
    std::wstring description;
    uint32 vendorId;
    uint32 deviceId;
    size_t dedicatedVideoMemory;
    size_t dedicatedSystemMemory;
    size_t sharedSystemMemory;
    bool isSoftware;
};

class Device : public NonCopyable {
public:
    Device();
    ~Device();

    void Initialize(int32 adapterId = -1, bool useWarp = false, bool enableDebug = false);
    void Shutdown();

    // Getters
    ID3D11Device* GetDevice() const { return m_device.Get(); }
    ID3D11DeviceContext* GetContext() const { return m_context.Get(); }
    IDXGIFactory1* GetFactory() const { return m_factory.Get(); }
    IDXGIAdapter1* GetAdapter() const { return m_adapter.Get(); }

    D3D_FEATURE_LEVEL GetFeatureLevel() const { return m_featureLevel; }
    const AdapterInfo& GetAdapterInfo() const { return m_adapterInfo; }

    // Utility functions
    std::vector<AdapterInfo> EnumerateAdapters() const;
    bool IsFeatureSupported(D3D11_FEATURE feature) const;

private:
    void CreateFactory();
    void SelectAdapter(int32 adapterId, bool useWarp);
    void CreateDevice(bool enableDebug);
    void QueryAdapterInfo();

    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_context;
    ComPtr<IDXGIFactory1> m_factory;
    ComPtr<IDXGIAdapter1> m_adapter;

    D3D_FEATURE_LEVEL m_featureLevel{D3D_FEATURE_LEVEL_11_0};
    AdapterInfo m_adapterInfo{};

    bool m_initialized{false};
};

} // namespace XeSS::Graphics
