#pragma once

// XeSS Engine - Modern C++20 Intel XeSS Integration Engine
//
// This engine provides a modular, easy-to-use wrapper around Intel XeSS
// with full DirectX 11 support and modern C++ features.

// Core module
#include "Core/Types.h"
#include "Core/Logger.h"
#include "Core/Exception.h"
#include "Core/Utils.h"
#include "Core/NonCopyable.h"

// Graphics module
#include "Graphics/Device.h"
#include "Graphics/SwapChain.h"

// XeSS module
#include "XeSS/XeSSContext.h"
#include "XeSS/XeSSTypes.h"

// Application framework
#include "Application/Application.h"

namespace XeSS {

// Version information
namespace Version {
    constexpr uint32 Major = 1;
    constexpr uint32 Minor = 0;
    constexpr uint32 Patch = 0;

    inline std::string GetVersionString() {
        return std::to_string(Major) + "." +
               std::to_string(Minor) + "." +
               std::to_string(Patch);
    }
}

// Engine information
namespace Engine {
    constexpr const char* Name = "XeSS Engine";
    constexpr const char* Description = "Modern C++20 Intel XeSS Integration Engine";

    inline std::string GetFullName() {
        return std::string(Name) + " v" + Version::GetVersionString();
    }
}

} // namespace XeSS

// Convenience macros for easy usage
#define XESS_VERSION_STRING XeSS::Version::GetVersionString()
#define XESS_ENGINE_NAME XeSS::Engine::GetFullName()
