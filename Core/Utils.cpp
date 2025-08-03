#include "Utils.h"
#include <codecvt>
#include <locale>

#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#endif

namespace XeSS::Utils {

float32 GetVanDerCorput(uint32 index, uint32 base) {
    float32 result = 0.0f;
    float32 bk = 1.0f;

    while (index > 0) {
        bk /= static_cast<float32>(base);
        result += static_cast<float32>(index % base) * bk;
        index /= base;
    }

    return result;
}

std::vector<std::pair<float32, float32>> GenerateHalton(
    uint32 base1, uint32 base2, uint32 startIndex, uint32 count,
    float32 offset1, float32 offset2) {

    std::vector<std::pair<float32, float32>> result;
    result.reserve(count);

    for (uint32 i = startIndex; i < count + startIndex; ++i) {
        float32 x = GetVanDerCorput(i, base1) + offset1;
        float32 y = GetVanDerCorput(i, base2) + offset2;
        result.emplace_back(x, y);
    }

    return result;
}

std::string WideToString(const std::wstring& wstr) {
    if (wstr.empty()) return {};

#ifdef _WIN32
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0],
        static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
    std::string result(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()),
        &result[0], size_needed, nullptr, nullptr);
    return result;
#else
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    return converter.to_bytes(wstr);
#endif
}

std::wstring StringToWide(const std::string& str) {
    if (str.empty()) return {};

#ifdef _WIN32
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0],
        static_cast<int>(str.size()), nullptr, 0);
    std::wstring result(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], static_cast<int>(str.size()),
        &result[0], size_needed);
    return result;
#else
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    return converter.from_bytes(str);
#endif
}

std::wstring GetExecutableDirectory() {
#ifdef _WIN32
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    PathRemoveFileSpecW(path);
    return std::wstring(path);
#else
    return L".";
#endif
}

std::wstring GetAssetPath(const std::wstring& assetName) {
    return GetExecutableDirectory() + L"\\" + assetName;
}

bool FileExists(const std::wstring& path) {
#ifdef _WIN32
    return PathFileExistsW(path.c_str()) == TRUE;
#else
    // Simple fallback for non-Windows platforms
    return false;
#endif
}

} // namespace XeSS::Utils
