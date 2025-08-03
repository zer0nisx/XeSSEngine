#include "XeSSContext.h"
#include "Core/Logger.h"
#include "Core/Exception.h"
#include <sstream>

namespace XeSS::XeSSModule {

std::string ResultToString(Result result) {
    switch (result) {
        case Result::Success: return "Success";
        case Result::WarningNonExistingFolder: return "Warning: Non-existing folder";
        case Result::WarningOldDriver: return "Warning: Old driver";
        case Result::ErrorUnsupportedDevice: return "Error: Unsupported device";
        case Result::ErrorUnsupportedDriver: return "Error: Unsupported driver";
        case Result::ErrorUninitialized: return "Error: Uninitialized";
        case Result::ErrorInvalidArgument: return "Error: Invalid argument";
        case Result::ErrorDeviceOutOfMemory: return "Error: Device out of memory";
        case Result::ErrorDevice: return "Error: Device error";
        case Result::ErrorNotImplemented: return "Error: Not implemented";
        case Result::ErrorInvalidContext: return "Error: Invalid context";
        case Result::ErrorOperationInProgress: return "Error: Operation in progress";
        case Result::ErrorUnsupported: return "Error: Unsupported";
        case Result::ErrorCantLoadLibrary: return "Error: Can't load library";
        case Result::ErrorWrongCallOrder: return "Error: Wrong call order";
        case Result::ErrorUnknown: return "Error: Unknown";
        default: return "Unknown error code";
    }
}

std::string QualityToString(QualityMode quality) {
    switch (quality) {
        case QualityMode::UltraPerformance: return "Ultra Performance";
        case QualityMode::Performance: return "Performance";
        case QualityMode::Balanced: return "Balanced";
        case QualityMode::Quality: return "Quality";
        case QualityMode::UltraQuality: return "Ultra Quality";
        default: return "Unknown";
    }
}

Resolution CalculateRenderResolution(const Resolution& outputRes, QualityMode quality) {
    float32 ratio = GetUpscaleRatio(quality);
    return {
        static_cast<uint32>(outputRes.width / ratio),
        static_cast<uint32>(outputRes.height / ratio)
    };
}

float32 GetUpscaleRatio(QualityMode quality) {
    switch (quality) {
        case QualityMode::UltraPerformance: return 3.0f;
        case QualityMode::Performance: return 2.0f;
        case QualityMode::Balanced: return 1.7f;
        case QualityMode::Quality: return 1.5f;
        case QualityMode::UltraQuality: return 1.3f;
        default: return 2.0f;
    }
}

void ThrowIfXeSSFailed(xess_result_t result, const std::string& message) {
    Result wrappedResult = FromNativeResult(result);

    if (wrappedResult > Result::Success) {
        // Warnings
        XESS_WARNING("XeSS Warning: {} - {}", message, ResultToString(wrappedResult));
    } else if (wrappedResult != Result::Success) {
        // Errors
        std::string errorMsg = message + " - " + ResultToString(wrappedResult);
        XESS_ERROR("XeSS Error: {}", errorMsg);
        throw XeSSException(errorMsg);
    }
}

XeSSContext::XeSSContext() = default;

XeSSContext::~XeSSContext() {
    Shutdown();
}

void XeSSContext::Initialize(Graphics::Device& device, const Resolution& outputResolution,
                            QualityMode quality, InitFlags flags) {
    if (m_initialized) {
        XESS_WARNING("XeSSContext already initialized");
        return;
    }

    if (!outputResolution.IsValid()) {
        throw XeSSException("Invalid output resolution");
    }

    m_outputResolution = outputResolution;
    m_quality = quality;
    m_initFlags = flags;

    XESS_INFO("Initializing XeSS context...");
    XESS_INFO("Output resolution: {}x{}", outputResolution.width, outputResolution.height);
    XESS_INFO("Quality: {}", QualityToString(quality));

    try {
        CreateContext(device);
        InitializeXeSS(device);
        QueryInputResolution();

        m_initialized = true;

        XESS_INFO("XeSS context initialized successfully");
        XESS_INFO("Input resolution: {}x{}", m_inputResolution.width, m_inputResolution.height);
        XESS_INFO("Upscale ratio: {:.2f}x", GetUpscaleRatio(quality));

        // Check for driver warnings
        if (IsOptimalDriver()) {
            XESS_INFO("Using optimal XeSS driver");
        } else {
            XESS_WARNING("Please update your graphics driver for optimal XeSS performance");
        }
    }
    catch (const Exception& e) {
        XESS_ERROR("Failed to initialize XeSS context: {}", e.what());
        throw;
    }
}

void XeSSContext::Shutdown() {
    if (!m_initialized) {
        return;
    }

    XESS_INFO("Shutting down XeSS context");

    if (m_context) {
        xess_result_t result = xessDestroyContext(m_context);
        if (result != XESS_RESULT_SUCCESS) {
            XESS_ERROR("Failed to destroy XeSS context: {}",
                      ResultToString(FromNativeResult(result)));
        }
        m_context = nullptr;
    }

    m_initialized = false;
}

void XeSSContext::Execute(Graphics::Device& device, const ExecuteParams& params) {
    if (!m_initialized) {
        throw XeSSException("XeSSContext not initialized");
    }

    if (!params.outputTexture) {
        throw XeSSException("Output texture is required");
    }

    xess_d3d11_execute_params_t execParams{};
    execParams.inputWidth = params.inputResolution.width;
    execParams.inputHeight = params.inputResolution.height;
    execParams.jitterOffsetX = params.jitterOffset.x;
    execParams.jitterOffsetY = params.jitterOffset.y;
    execParams.exposureScale = params.exposureScale;
    execParams.resetHistory = params.resetAccumulation;

    execParams.pColorTexture = params.colorTexture;
    execParams.pVelocityTexture = params.velocityTexture;
    execParams.pDepthTexture = params.depthTexture;
    execParams.pExposureScaleTexture = params.exposureTexture;
    execParams.pResponsivePixelMaskTexture = params.responsiveMaskTexture;
    execParams.pOutputTexture = params.outputTexture;

    xess_result_t result = xessD3D11Execute(m_context, &execParams);
    ThrowIfXeSSFailed(result, "Failed to execute XeSS");
}

void XeSSContext::CreateContext(Graphics::Device& device) {
    xess_result_t result = xessD3D11CreateContext(device.GetDevice(), &m_context);
    ThrowIfXeSSFailed(result, "Failed to create XeSS context");
}

void XeSSContext::InitializeXeSS(Graphics::Device& device) {
    xess_d3d11_init_params_t initParams{};
    initParams.outputResolution = ToNativeResolution(m_outputResolution);
    initParams.qualitySetting = ToNativeQuality(m_quality);
    initParams.initFlags = ToNativeInitFlags(m_initFlags);

    xess_result_t result = xessD3D11Init(m_context, &initParams);
    ThrowIfXeSSFailed(result, "Failed to initialize XeSS");
}

void XeSSContext::QueryInputResolution() {
    xess_2d_t outputRes = ToNativeResolution(m_outputResolution);
    xess_2d_t inputRes{};

    xess_result_t result = xessGetInputResolution(
        m_context, &outputRes, ToNativeQuality(m_quality), &inputRes);
    ThrowIfXeSSFailed(result, "Failed to get XeSS input resolution");

    m_inputResolution = FromNativeResolution(inputRes);
}

bool XeSSContext::IsOptimalDriver() const {
    if (!m_context) {
        return false;
    }

    xess_result_t result = xessIsOptimalDriver(m_context);
    return result == XESS_RESULT_SUCCESS;
}

std::string XeSSContext::GetVersion() const {
    if (!m_context) {
        return "Not initialized";
    }

    xess_version_t version{};
    xess_result_t result = xessGetVersion(m_context, &version);

    if (result == XESS_RESULT_SUCCESS) {
        std::ostringstream oss;
        oss << version.major << "." << version.minor << "." << version.patch;
        return oss.str();
    }

    return "Unknown";
}

std::vector<QualityMode> XeSSContext::GetSupportedQualityModes() {
    return {
        QualityMode::UltraPerformance,
        QualityMode::Performance,
        QualityMode::Balanced,
        QualityMode::Quality,
        QualityMode::UltraQuality
    };
}

bool XeSSContext::IsQualitySupported(QualityMode quality) {
    auto supported = GetSupportedQualityModes();
    return std::find(supported.begin(), supported.end(), quality) != supported.end();
}

} // namespace XeSS::XeSSModule
