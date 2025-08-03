#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <functional>

namespace XeSS {

// Common types
using uint8 = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;

using int8 = std::int8_t;
using int16 = std::int16_t;
using int32 = std::int32_t;
using int64 = std::int64_t;

using float32 = float;
using float64 = double;

// 2D Vector
struct Vector2 {
    float32 x{0.0f};
    float32 y{0.0f};

    Vector2() = default;
    Vector2(float32 x_, float32 y_) : x(x_), y(y_) {}

    Vector2 operator+(const Vector2& other) const {
        return {x + other.x, y + other.y};
    }

    Vector2 operator-(const Vector2& other) const {
        return {x - other.x, y - other.y};
    }

    Vector2 operator*(float32 scale) const {
        return {x * scale, y * scale};
    }
};

// 3D Vector
struct Vector3 {
    float32 x{0.0f};
    float32 y{0.0f};
    float32 z{0.0f};

    Vector3() = default;
    Vector3(float32 x_, float32 y_, float32 z_) : x(x_), y(y_), z(z_) {}
};

// 4D Vector
struct Vector4 {
    float32 x{0.0f};
    float32 y{0.0f};
    float32 z{0.0f};
    float32 w{0.0f};

    Vector4() = default;
    Vector4(float32 x_, float32 y_, float32 z_, float32 w_) : x(x_), y(y_), z(z_), w(w_) {}
};

// Resolution
struct Resolution {
    uint32 width{0};
    uint32 height{0};

    Resolution() = default;
    Resolution(uint32 w, uint32 h) : width(w), height(h) {}

    bool IsValid() const {
        return width > 0 && height > 0;
    }

    float32 AspectRatio() const {
        return width > 0 && height > 0 ? static_cast<float32>(width) / static_cast<float32>(height) : 0.0f;
    }
};

// Vertex structure
struct Vertex {
    Vector3 position;
    Vector4 color;

    Vertex() = default;
    Vertex(const Vector3& pos, const Vector4& col) : position(pos), color(col) {}
};

// Quality settings enum
enum class Quality : uint32 {
    UltraPerformance = 0,
    Performance = 1,
    Balanced = 2,
    Quality = 3,
    UltraQuality = 4
};

// Color constants
namespace Colors {
    constexpr Vector4 Black{0.0f, 0.0f, 0.0f, 1.0f};
    constexpr Vector4 White{1.0f, 1.0f, 1.0f, 1.0f};
    constexpr Vector4 Red{1.0f, 0.0f, 0.0f, 1.0f};
    constexpr Vector4 Green{0.0f, 1.0f, 0.0f, 1.0f};
    constexpr Vector4 Blue{0.0f, 0.0f, 1.0f, 1.0f};
    constexpr Vector4 Yellow{1.0f, 1.0f, 0.0f, 1.0f};
    constexpr Vector4 Cyan{0.0f, 1.0f, 1.0f, 1.0f};
    constexpr Vector4 Magenta{1.0f, 0.0f, 1.0f, 1.0f};
}

} // namespace XeSS
