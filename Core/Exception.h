#pragma once

#include "Types.h"
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace XeSS {

// Base exception class
class Exception : public std::runtime_error {
public:
    explicit Exception(const std::string& message)
        : std::runtime_error(message) {}

    explicit Exception(const char* message)
        : std::runtime_error(message) {}
};

// Graphics-specific exceptions
class GraphicsException : public Exception {
public:
    explicit GraphicsException(const std::string& message)
        : Exception("Graphics Error: " + message) {}
};

class XeSSException : public Exception {
public:
    explicit XeSSException(const std::string& message)
        : Exception("XeSS Error: " + message) {}
};

class ShaderException : public Exception {
public:
    explicit ShaderException(const std::string& message)
        : Exception("Shader Error: " + message) {}
};

// Helper functions for error checking
void ThrowIfFailed(long hr, const std::string& message = "Operation failed");

// Macros for convenient error checking
#define XESS_THROW_IF_FAILED(hr, msg) XeSS::ThrowIfFailed(hr, msg)

#ifdef _DEBUG
#define XESS_ASSERT(condition, message) \
    if (!(condition)) { \
        throw XeSS::Exception("Assertion failed: " + std::string(message)); \
    }
#else
#define XESS_ASSERT(condition, message) ((void)0)
#endif

} // namespace XeSS
