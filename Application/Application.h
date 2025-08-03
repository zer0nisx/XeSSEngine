#pragma once

#include "Core/Types.h"
#include "Core/NonCopyable.h"
#include "Graphics/Device.h"
#include "Graphics/SwapChain.h"
#include "XeSS/XeSSContext.h"
#include "Window.h"
#include <memory>
#include <chrono>

namespace XeSS::Application {

struct ApplicationConfig {
    std::wstring title{L"XeSS Engine Application"};
    Resolution windowSize{1920, 1080};
    bool enableDebugLayer{false};
    bool enableVSync{false};
    bool fullscreen{false};
    int32 adapterId{-1};
    bool useWarp{false};

    // XeSS settings
    XeSSModule::QualityMode xessQuality{XeSSModule::QualityMode::Performance};
    XeSSModule::InitFlags xessFlags{XeSSModule::InitFlags::HighResMotionVectors};
};

class Application : public NonCopyable {
public:
    explicit Application(const ApplicationConfig& config = {});
    virtual ~Application();

    // Main application lifecycle
    int Run();
    void Quit();

    // Configuration
    void SetConfig(const ApplicationConfig& config);
    const ApplicationConfig& GetConfig() const { return m_config; }

    // XeSS control
    void SetXeSSQuality(XeSSModule::QualityMode quality);
    XeSSModule::QualityMode GetXeSSQuality() const;

    // Getters for subsystems
    Graphics::Device& GetDevice() { return *m_device; }
    Graphics::SwapChain& GetSwapChain() { return *m_swapChain; }
    XeSSModule::XeSSContext& GetXeSSContext() { return *m_xessContext; }
    Window& GetWindow() { return *m_window; }

    // Performance metrics
    float GetFrameTime() const { return m_frameTime; }
    float GetFPS() const { return m_fps; }
    uint64 GetFrameCount() const { return m_frameCount; }

protected:
    // Virtual methods for derived classes to override
    virtual void OnInitialize() {}
    virtual void OnShutdown() {}
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnRender() {}
    virtual void OnResize(const Resolution& newSize) {}
    virtual void OnKeyDown(uint32 key) {}
    virtual void OnKeyUp(uint32 key) {}
    virtual void OnMouseMove(int32 x, int32 y) {}
    virtual void OnMouseButton(uint32 button, bool pressed) {}

private:
    void Initialize();
    void Shutdown();
    void MainLoop();
    void Update();
    void Render();
    void Present();
    void HandleResize();
    void UpdatePerformanceMetrics();

    ApplicationConfig m_config;

    std::unique_ptr<Window> m_window;
    std::unique_ptr<Graphics::Device> m_device;
    std::unique_ptr<Graphics::SwapChain> m_swapChain;
    std::unique_ptr<XeSSModule::XeSSContext> m_xessContext;

    // Performance tracking
    std::chrono::high_resolution_clock::time_point m_lastFrameTime;
    std::chrono::high_resolution_clock::time_point m_fpsUpdateTime;
    float m_frameTime{0.0f};
    float m_fps{0.0f};
    uint64 m_frameCount{0};
    uint32 m_fpsFrameCount{0};

    bool m_initialized{false};
    bool m_running{false};
    bool m_resizePending{false};
    Resolution m_pendingSize;

    friend class Window; // Allow window to call event handlers
};

} // namespace XeSS::Application
