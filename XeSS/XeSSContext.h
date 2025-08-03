#pragma once

#include "XeSSTypes.h"
#include "Core/NonCopyable.h"
#include "Graphics/Device.h"
#include "xess/xess_d3d11.h"

namespace XeSS::XeSSModule {

class XeSSContext : public NonCopyable {
public:
    XeSSContext();
    ~XeSSContext();

    // Initialization
    void Initialize(Graphics::Device& device, const Resolution& outputResolution,
                   QualityMode quality, InitFlags flags = InitFlags::HighResMotionVectors);
    void Shutdown();

    // Execution
    void Execute(Graphics::Device& device, const ExecuteParams& params);

    // Properties
    Resolution GetInputResolution() const { return m_inputResolution; }
    Resolution GetOutputResolution() const { return m_outputResolution; }
    QualityMode GetQuality() const { return m_quality; }
    InitFlags GetInitFlags() const { return m_initFlags; }

    // Utility
    bool IsInitialized() const { return m_initialized; }
    bool IsOptimalDriver() const;
    std::string GetVersion() const;

    // Quality settings
    static std::vector<QualityMode> GetSupportedQualityModes();
    static bool IsQualitySupported(QualityMode quality);

private:
    void CreateContext(Graphics::Device& device);
    void InitializeXeSS(Graphics::Device& device);
    void QueryInputResolution();

    xess_context_handle_t m_context{nullptr};

    Resolution m_outputResolution;
    Resolution m_inputResolution;
    QualityMode m_quality{QualityMode::Performance};
    InitFlags m_initFlags{InitFlags::HighResMotionVectors};

    bool m_initialized{false};
};

} // namespace XeSS::XeSSModule
