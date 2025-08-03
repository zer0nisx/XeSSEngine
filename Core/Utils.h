#pragma once

#include "Types.h"
#include <vector>
#include <utility>
#include <string>

namespace XeSS::Utils {

/**
 * Generates halton sequence with optional point shift using van der Corput sequence
 * @param base1 - base for first Corput sequence (X coordinate)
 * @param base2 - base for second Corput sequence (Y coordinate)
 * @param startIndex - initial index for halton set
 * @param count - number of points to generate
 * @param offset1 - offset for first Corput sequence
 * @param offset2 - offset for second Corput sequence
 * @return vector of pairs with jitter values
 */
std::vector<std::pair<float32, float32>> GenerateHalton(
    uint32 base1, uint32 base2, uint32 startIndex, uint32 count,
    float32 offset1 = -0.5f, float32 offset2 = -0.5f);

/**
 * Van der Corput sequence generator
 * @param index - sequence index
 * @param base - number base for sequence
 * @return computed value
 */
float32 GetVanDerCorput(uint32 index, uint32 base);

/**
 * Convert wide string to string
 */
std::string WideToString(const std::wstring& wstr);

/**
 * Convert string to wide string
 */
std::wstring StringToWide(const std::string& str);

/**
 * Get the directory containing the executable
 */
std::wstring GetExecutableDirectory();

/**
 * Get full path for an asset
 */
std::wstring GetAssetPath(const std::wstring& assetName);

/**
 * File existence check
 */
bool FileExists(const std::wstring& path);

/**
 * String formatting utility
 */
template<typename... Args>
std::string Format(const std::string& format, Args&&... args) {
    size_t size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1);
}

/**
 * Clamp value between min and max
 */
template<typename T>
constexpr T Clamp(T value, T min, T max) {
    return value < min ? min : (value > max ? max : value);
}

/**
 * Linear interpolation
 */
template<typename T>
constexpr T Lerp(T a, T b, float32 t) {
    return a + t * (b - a);
}

/**
 * Convert degrees to radians
 */
constexpr float32 ToRadians(float32 degrees) {
    return degrees * 3.14159265359f / 180.0f;
}

/**
 * Convert radians to degrees
 */
constexpr float32 ToDegrees(float32 radians) {
    return radians * 180.0f / 3.14159265359f;
}

} // namespace XeSS::Utils
