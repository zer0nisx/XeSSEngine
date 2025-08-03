#pragma once

#include "Core/Types.h"
#include "xess/xess_d3d11.h"
#include <string>

namespace XeSS::XeSSModule {

// Quality settings wrapper
enum class QualityMode : uint32 {
    UltraPerformance = XESS_QUALITY_SETTING_ULTRA_PERFORMANCE,
    Performance = XESS_QUALITY_SETTING_PERFORMANCE,
    Balanced = XESS_QUALITY_SETTING_BALANCED,
    Quality = XESS_QUALITY_SETTING_QUALITY,
    UltraQuality = XESS_QUALITY_SETTING_ULTRA_QUALITY
};

// Initialization flags
enum class InitFlags : uint32 {
    None = XESS_INIT_FLAG_NONE,
    HighResMotionVectors = XESS_INIT_FLAG_HIGH_RES_MV,
    LowResMotionVectors = XESS_INIT_FLAG_NONE,
    ExposureScaleTexture = XESS_INIT_FLAG_EXPOSURE_SCALE_TEXTURE,
    ResponsiveMask = XESS_INIT_FLAG_RESPONSIVE_PIXEL_MASK,
    UseExternalDescriptorHeap = XESS_INIT_FLAG_USE_EXTERNAL_DESCRIPTOR_HEAP
};

// Result codes
enum class Result : int32 {
    Success = XESS_RESULT_SUCCESS,
    WarningNonExistingFolder = XESS_RESULT_WARNING_NONEXISTING_FOLDER,
    WarningOldDriver = XESS_RESULT_WARNING_OLD_DRIVER,
    ErrorUnsupportedDevice = XESS_RESULT_ERROR_UNSUPPORTED_DEVICE,
    ErrorUnsupportedDriver = XESS_RESULT_ERROR_UNSUPPORTED_DRIVER,
    ErrorUninitialized = XESS_RESULT_ERROR_UNINITIALIZED,
    ErrorInvalidArgument = XESS_RESULT_ERROR_INVALID_ARGUMENT,
    ErrorDeviceOutOfMemory = XESS_RESULT_ERROR_DEVICE_OUT_OF_MEMORY,
    ErrorDevice = XESS_RESULT_ERROR_DEVICE,
    ErrorNotImplemented = XESS_RESULT_ERROR_NOT_IMPLEMENTED,
    ErrorInvalidContext = XESS_RESULT_ERROR_INVALID_CONTEXT,
    ErrorOperationInProgress = XESS_RESULT_ERROR_OPERATION_IN_PROGRESS,
    ErrorUnsupported = XESS_RESULT_ERROR_UNSUPPORTED,
    ErrorCantLoadLibrary = XESS_RESULT_ERROR_CANT_LOAD_LIBRARY,
    ErrorWrongCallOrder = XESS_RESULT_ERROR_WRONG_CALL_ORDER,
    ErrorUnknown = XESS_RESULT_ERROR_UNKNOWN
};

// Execute parameters
struct ExecuteParams {
    Resolution inputResolution;
    Vector2 jitterOffset{0.0f, 0.0f};
    float32 exposureScale{1.0f};
    bool resetAccumulation{false};

    ID3D11Resource* colorTexture{nullptr};
    ID3D11Resource* velocityTexture{nullptr};
    ID3D11Resource* depthTexture{nullptr};
    ID3D11Resource* exposureTexture{nullptr};
    ID3D11Resource* responsiveMaskTexture{nullptr};
    ID3D11Resource* outputTexture{nullptr};
};

// Utility functions
inline xess_quality_settings_t ToNativeQuality(QualityMode quality) {
    return static_cast<xess_quality_settings_t>(quality);
}

inline xess_init_flag_t ToNativeInitFlags(InitFlags flags) {
    return static_cast<xess_init_flag_t>(flags);
}

inline Result FromNativeResult(xess_result_t result) {
    return static_cast<Result>(result);
}

inline xess_2d_t ToNativeResolution(const Resolution& res) {
    return {res.width, res.height};
}

inline Resolution FromNativeResolution(const xess_2d_t& res) {
    return {res.x, res.y};
}

// Result to string conversion
std::string ResultToString(Result result);

// Quality to string conversion
std::string QualityToString(QualityMode quality);

// Calculate optimal render resolution for given output resolution and quality
Resolution CalculateRenderResolution(const Resolution& outputRes, QualityMode quality);

// Calculate upscaling ratio for quality mode
float32 GetUpscaleRatio(QualityMode quality);

} // namespace XeSS::XeSSModule
