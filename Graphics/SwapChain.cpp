#include "SwapChain.h"
#include "Core/Logger.h"
#include "Core/Exception.h"

namespace XeSS::Graphics {

SwapChain::SwapChain() = default;

SwapChain::~SwapChain() {
    Shutdown();
}

void SwapChain::Initialize(Device& device, const SwapChainDesc& desc) {
    if (m_initialized) {
        XESS_WARNING("SwapChain already initialized");
        return;
    }

    if (!desc.windowHandle) {
        throw GraphicsException("Invalid window handle for SwapChain");
    }

    m_desc = desc;

    XESS_INFO("Creating SwapChain {}x{} with {} buffers",
              desc.resolution.width, desc.resolution.height, desc.bufferCount);

    try {
        CreateSwapChain(device);
        CreateBackBufferViews(device);

        m_initialized = true;
        XESS_INFO("SwapChain created successfully");
    }
    catch (const Exception& e) {
        XESS_ERROR("Failed to create SwapChain: {}", e.what());
        throw;
    }
}

void SwapChain::Shutdown() {
    if (!m_initialized) {
        return;
    }

    XESS_INFO("Shutting down SwapChain");

    ReleaseBackBufferViews();

    if (m_swapChain) {
        m_swapChain->SetFullscreenState(FALSE, nullptr);
        m_swapChain.Reset();
    }

    m_initialized = false;
}

void SwapChain::Present(bool vsync) {
    if (!m_swapChain) {
        throw GraphicsException("SwapChain not initialized");
    }

    uint32 syncInterval = vsync ? 1 : 0;
    uint32 flags = 0;

    HRESULT hr = m_swapChain->Present(syncInterval, flags);

    // Handle device lost scenarios
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
        throw GraphicsException("Device lost during Present");
    }

    XESS_THROW_IF_FAILED(hr, "Failed to present SwapChain");
}

void SwapChain::Resize(const Resolution& newResolution) {
    if (!m_swapChain) {
        throw GraphicsException("SwapChain not initialized");
    }

    if (newResolution.width == m_desc.resolution.width &&
        newResolution.height == m_desc.resolution.height) {
        return; // No change needed
    }

    XESS_INFO("Resizing SwapChain from {}x{} to {}x{}",
              m_desc.resolution.width, m_desc.resolution.height,
              newResolution.width, newResolution.height);

    // Release back buffer views
    ReleaseBackBufferViews();

    // Resize buffers
    XESS_THROW_IF_FAILED(
        m_swapChain->ResizeBuffers(
            m_desc.bufferCount,
            newResolution.width,
            newResolution.height,
            m_desc.format,
            0
        ),
        "Failed to resize SwapChain buffers"
    );

    m_desc.resolution = newResolution;

    // Recreate back buffer views - need device reference
    // This would need to be handled by the calling code
}

void SwapChain::CreateSwapChain(Device& device) {
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.Width = m_desc.resolution.width;
    swapChainDesc.Height = m_desc.resolution.height;
    swapChainDesc.Format = m_desc.format;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = m_desc.bufferCount;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = 0;

    ComPtr<IDXGISwapChain1> swapChain1;
    XESS_THROW_IF_FAILED(
        device.GetFactory()->CreateSwapChainForHwnd(
            device.GetDevice(),
            m_desc.windowHandle,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain1
        ),
        "Failed to create SwapChain"
    );

    XESS_THROW_IF_FAILED(
        swapChain1.As(&m_swapChain),
        "Failed to get IDXGISwapChain3 interface"
    );

    // Disable Alt+Enter fullscreen toggle
    XESS_THROW_IF_FAILED(
        device.GetFactory()->MakeWindowAssociation(m_desc.windowHandle, DXGI_MWA_NO_ALT_ENTER),
        "Failed to disable Alt+Enter"
    );
}

void SwapChain::CreateBackBufferViews(Device& device) {
    m_backBuffers.resize(m_desc.bufferCount);
    m_backBufferRTVs.resize(m_desc.bufferCount);

    for (uint32 i = 0; i < m_desc.bufferCount; ++i) {
        // Get back buffer texture
        XESS_THROW_IF_FAILED(
            m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i])),
            "Failed to get back buffer " + std::to_string(i)
        );

        // Create render target view
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
        rtvDesc.Format = m_desc.format;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = 0;

        XESS_THROW_IF_FAILED(
            device.GetDevice()->CreateRenderTargetView(
                m_backBuffers[i].Get(),
                &rtvDesc,
                &m_backBufferRTVs[i]
            ),
            "Failed to create back buffer RTV " + std::to_string(i)
        );
    }
}

void SwapChain::ReleaseBackBufferViews() {
    m_backBufferRTVs.clear();
    m_backBuffers.clear();
}

ID3D11Texture2D* SwapChain::GetBackBuffer(uint32 index) const {
    if (index >= m_backBuffers.size()) {
        return nullptr;
    }
    return m_backBuffers[index].Get();
}

ID3D11RenderTargetView* SwapChain::GetBackBufferRTV(uint32 index) const {
    if (index >= m_backBufferRTVs.size()) {
        return nullptr;
    }
    return m_backBufferRTVs[index].Get();
}

uint32 SwapChain::GetCurrentBackBufferIndex() const {
    if (!m_swapChain) {
        return 0;
    }
    return m_swapChain->GetCurrentBackBufferIndex();
}

void SwapChain::SetFullscreenState(bool fullscreen) {
    if (!m_swapChain) {
        throw GraphicsException("SwapChain not initialized");
    }

    XESS_THROW_IF_FAILED(
        m_swapChain->SetFullscreenState(fullscreen, nullptr),
        "Failed to set fullscreen state"
    );
}

bool SwapChain::IsFullscreen() const {
    if (!m_swapChain) {
        return false;
    }

    BOOL fullscreen = FALSE;
    m_swapChain->GetFullscreenState(&fullscreen, nullptr);
    return fullscreen == TRUE;
}

} // namespace XeSS::Graphics
