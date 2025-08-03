#pragma once

#include "Core/Types.h"
#include "Core/NonCopyable.h"
#include "Device.h"
#include <dxgi1_3.h>
#include <wrl/client.h>

namespace XeSS::Graphics {

using Microsoft::WRL::ComPtr;

struct SwapChainDesc {
    Resolution resolution{1920, 1080};
    uint32 bufferCount{2};
    DXGI_FORMAT format{DXGI_FORMAT_R8G8B8A8_UNORM};
    bool windowed{true};
    bool enableVSync{false};
    HWND windowHandle{nullptr};
};

class SwapChain : public NonCopyable {
public:
    SwapChain();
    ~SwapChain();

    void Initialize(Device& device, const SwapChainDesc& desc);
    void Shutdown();

    void Present(bool vsync = false);
    void Resize(const Resolution& newResolution);

    // Getters
    IDXGISwapChain3* GetSwapChain() const { return m_swapChain.Get(); }
    ID3D11Texture2D* GetBackBuffer(uint32 index = 0) const;
    ID3D11RenderTargetView* GetBackBufferRTV(uint32 index = 0) const;

    uint32 GetCurrentBackBufferIndex() const;
    const SwapChainDesc& GetDesc() const { return m_desc; }
    Resolution GetResolution() const { return m_desc.resolution; }

    // Utility
    void SetFullscreenState(bool fullscreen);
    bool IsFullscreen() const;

private:
    void CreateSwapChain(Device& device);
    void CreateBackBufferViews(Device& device);
    void ReleaseBackBufferViews();

    ComPtr<IDXGISwapChain3> m_swapChain;
    std::vector<ComPtr<ID3D11Texture2D>> m_backBuffers;
    std::vector<ComPtr<ID3D11RenderTargetView>> m_backBufferRTVs;

    SwapChainDesc m_desc{};
    bool m_initialized{false};
};

} // namespace XeSS::Graphics
