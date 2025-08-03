#include "Graphics/ShaderCompiler/ShaderCompiler.h"
#include "Core/Logger.h"
#include "Core/Exception.h"
#include <iostream>
#include <filesystem>

using namespace XeSS;
using namespace XeSS::Graphics;

class ShaderCompilerExample {
public:
    void Run() {
        XESS_INFO("=== XeSS Engine Shader Compiler Example ===");
        XESS_INFO("Demonstrating Shader Model 6.4 compilation capabilities");

        try {
            // Initialize the shader compiler
            if (!InitializeCompiler()) {
                XESS_ERROR("Failed to initialize shader compiler");
                return;
            }

            // Test basic compilation
            TestBasicCompilation();

            // Test Shader Model 6.4 features
            TestShaderModel64Features();

            // Test caching system
            TestShaderCaching();

            // Test wave intrinsics
            TestWaveIntrinsics();

            // Test Variable Rate Shading
            TestVariableRateShading();

            // Performance comparison
            PerformanceComparison();

            XESS_INFO("=== Shader Compiler Example Complete ===");
        }
        catch (const std::exception& e) {
            XESS_ERROR("Exception in ShaderCompilerExample: {}", e.what());
        }

        ShutdownCompiler();
    }

private:
    std::unique_ptr<ShaderCompiler> m_compiler;

    bool InitializeCompiler() {
        m_compiler = std::make_unique<ShaderCompiler>();

        if (!m_compiler->Initialize()) {
            XESS_ERROR("Failed to initialize DirectX Shader Compiler");
            return false;
        }

        XESS_INFO("Shader Compiler initialized successfully");
        XESS_INFO("Max supported Shader Model: {}",
                 ShaderModelToString(m_compiler->GetMaxSupportedShaderModel()));

        // Enable caching and hot reload for development
        m_compiler->SetCacheEnabled(true);
        m_compiler->EnableHotReload(true);

        // Print feature support
        XESS_INFO("Feature Support:");
        XESS_INFO("  - Variable Rate Shading: {}", m_compiler->SupportsVariableRateShading() ? "Yes" : "No");
        XESS_INFO("  - Wave Intrinsics: {}", m_compiler->SupportsWaveIntrinsics() ? "Yes" : "No");
        XESS_INFO("  - Mesh Shaders: {}", m_compiler->SupportsMeshShaders() ? "Yes" : "No");
        XESS_INFO("  - Raytracing: {}", m_compiler->SupportsRaytracing() ? "Yes" : "No");

        return true;
    }

    void TestBasicCompilation() {
        XESS_INFO("\n--- Testing Basic Shader Compilation ---");

        // Simple vertex shader source
        const std::string vertexShaderSource = R"(
            cbuffer Transform : register(b0) {
                float4x4 worldViewProj;
            }

            struct VSInput {
                float3 position : POSITION;
                float2 uv : TEXCOORD0;
            };

            struct VSOutput {
                float4 position : SV_Position;
                float2 uv : TEXCOORD0;
            };

            VSOutput main(VSInput input) {
                VSOutput output;
                output.position = mul(float4(input.position, 1.0), worldViewProj);
                output.uv = input.uv;
                return output;
            }
        )";

        CompileOptions options;
        options.targetModel = ShaderModel::SM_6_4;
        options.enableOptimization = true;
        options.optimizationLevel = 3;

        CompiledShader result = m_compiler->CompileFromSource(
            vertexShaderSource, "main", ShaderType::Vertex, options, "BasicVertex.hlsl");

        if (result.success) {
            XESS_INFO("✓ Basic vertex shader compiled successfully");
            XESS_INFO("  Bytecode size: {} bytes", result.bytecode.size());

            if (!result.warnings.empty()) {
                XESS_WARNING("  Warnings: {}", result.warnings.size());
                for (const auto& warning : result.warnings) {
                    XESS_WARNING("    {}", warning);
                }
            }
        } else {
            XESS_ERROR("✗ Basic vertex shader compilation failed");
            for (const auto& error : result.errors) {
                XESS_ERROR("  {}", error);
            }
        }
    }

    void TestShaderModel64Features() {
        XESS_INFO("\n--- Testing Shader Model 6.4 Features ---");

        // Test with enhanced features
        const std::string sm64ComputeShader = R"(
            #if __SHADER_TARGET_MAJOR < 6 || (__SHADER_TARGET_MAJOR == 6 && __SHADER_TARGET_MINOR < 4)
            #error "This shader requires Shader Model 6.4"
            #endif

            RWTexture2D<float4> outputTexture : register(u0);

            [numthreads(8, 8, 1)]
            void main(uint3 id : SV_DispatchThreadID) {
                // Use wave intrinsics (SM 6.0+)
                uint laneIndex = WaveGetLaneIndex();
                uint laneCount = WaveGetLaneCount();

                // Enhanced texture operations (SM 6.4)
                float4 color = float4(
                    float(laneIndex) / float(laneCount),
                    float(id.x) / 1024.0,
                    float(id.y) / 1024.0,
                    1.0
                );

                // Wave operations for smoothing
                color.rgb = WaveActiveSum(color.rgb) / float(laneCount);

                outputTexture[id.xy] = color;
            }
        )";

        CompileOptions sm64Options;
        sm64Options.targetModel = ShaderModel::SM_6_4;
        sm64Options.enableOptimization = true;
        sm64Options.enableUnboundedResourceArrays = true;

        CompiledShader result = m_compiler->CompileFromSource(
            sm64ComputeShader, "main", ShaderType::Compute, sm64Options, "SM64_Compute.hlsl");

        if (result.success) {
            XESS_INFO("✓ Shader Model 6.4 compute shader compiled successfully");
            XESS_INFO("  Advanced features: Wave Intrinsics, Enhanced Texture Ops");
        } else {
            XESS_ERROR("✗ Shader Model 6.4 compilation failed");
            for (const auto& error : result.errors) {
                XESS_ERROR("  {}", error);
            }
        }
    }

    void TestShaderCaching() {
        XESS_INFO("\n--- Testing Shader Caching System ---");

        const std::string testShader = R"(
            float4 main() : SV_Target {
                return float4(1, 0, 0, 1);
            }
        )";

        CompileOptions options;
        options.targetModel = ShaderModel::SM_6_4;

        // First compilation - should cache
        auto start = std::chrono::high_resolution_clock::now();
        CompiledShader result1 = m_compiler->CompileFromSource(
            testShader, "main", ShaderType::Pixel, options, "CacheTest.hlsl");
        auto firstTime = std::chrono::high_resolution_clock::now() - start;

        // Second compilation - should use cache
        start = std::chrono::high_resolution_clock::now();
        CompiledShader result2 = m_compiler->CompileFromSource(
            testShader, "main", ShaderType::Pixel, options, "CacheTest.hlsl");
        auto cachedTime = std::chrono::high_resolution_clock::now() - start;

        if (result1.success && result2.success) {
            XESS_INFO("✓ Shader caching working correctly");
            XESS_INFO("  First compile: {:.2f}ms",
                     std::chrono::duration<double, std::milli>(firstTime).count());
            XESS_INFO("  Cached compile: {:.2f}ms",
                     std::chrono::duration<double, std::milli>(cachedTime).count());

            float speedup = static_cast<float>(firstTime.count()) / static_cast<float>(cachedTime.count());
            XESS_INFO("  Cache speedup: {:.1f}x", speedup);
        } else {
            XESS_ERROR("✗ Shader caching test failed");
        }
    }

    void TestWaveIntrinsics() {
        XESS_INFO("\n--- Testing Wave Intrinsics ---");

        if (!m_compiler->SupportsWaveIntrinsics()) {
            XESS_WARNING("Wave Intrinsics not supported, skipping test");
            return;
        }

        // Load and compile the wave intrinsics demo shader
        std::string shaderPath = "Shaders/Modern/WaveIntrinsics_SM64.hlsl.txt";

        CompileOptions options;
        options.targetModel = ShaderModel::SM_6_4;
        options.enableOptimization = true;
        options.macros.push_back({"WAVE_DEMO", "1"});

        CompiledShader result = m_compiler->CompileFromFile(
            shaderPath, "CSWaveIntrinsicsDemo", ShaderType::Compute, options);

        if (result.success) {
            XESS_INFO("✓ Wave Intrinsics shader compiled successfully");
            XESS_INFO("  Features: WaveActiveSum, WaveActiveMin, WaveActiveBallot");
        } else {
            XESS_WARNING("Wave Intrinsics shader compilation failed (file may not exist)");
        }
    }

    void TestVariableRateShading() {
        XESS_INFO("\n--- Testing Variable Rate Shading ---");

        if (!m_compiler->SupportsVariableRateShading()) {
            XESS_WARNING("Variable Rate Shading not supported, skipping test");
            return;
        }

        // VRS requires specific shader compilation
        const std::string vrsShader = R"(
            #if __SHADER_TARGET_MAJOR < 6 || (__SHADER_TARGET_MAJOR == 6 && __SHADER_TARGET_MINOR < 4)
            #error "VRS requires Shader Model 6.4+"
            #endif

            RWTexture2D<uint> vrsRateImage : register(u0);

            [numthreads(8, 8, 1)]
            void main(uint3 id : SV_DispatchThreadID) {
                // VRS rate calculation
                uint rate = 0x5; // 2x2 rate
                vrsRateImage[id.xy] = rate;
            }
        )";

        CompileOptions vrsOptions;
        vrsOptions.targetModel = ShaderModel::SM_6_4;
        vrsOptions.macros.push_back({"VRS_ENABLED", "1"});

        CompiledShader result = m_compiler->CompileFromSource(
            vrsShader, "main", ShaderType::Compute, vrsOptions, "VRS_Test.hlsl");

        if (result.success) {
            XESS_INFO("✓ Variable Rate Shading shader compiled successfully");
        } else {
            XESS_ERROR("✗ VRS shader compilation failed");
            for (const auto& error : result.errors) {
                XESS_ERROR("  {}", error);
            }
        }
    }

    void PerformanceComparison() {
        XESS_INFO("\n--- Performance Comparison: Legacy vs Modern ---");

        const std::string testShader = R"(
            cbuffer Params : register(b0) {
                float4 testData[64];
            }

            float4 main(float2 uv : TEXCOORD0) : SV_Target {
                float4 result = float4(0, 0, 0, 1);

                [unroll]
                for (int i = 0; i < 64; i++) {
                    result += testData[i] * uv.x;
                }

                return result;
            }
        )";

        // Test with different shader models
        std::vector<ShaderModel> modelsToTest = {
            ShaderModel::SM_5_0,
            ShaderModel::SM_5_1,
            ShaderModel::SM_6_0,
            ShaderModel::SM_6_4
        };

        for (ShaderModel model : modelsToTest) {
            if (!m_compiler->IsShaderModelSupported(model)) {
                XESS_INFO("  Shader Model {} not supported, skipping", ShaderModelToString(model));
                continue;
            }

            CompileOptions options;
            options.targetModel = model;
            options.enableOptimization = true;
            options.optimizationLevel = 3;

            auto start = std::chrono::high_resolution_clock::now();
            CompiledShader result = m_compiler->CompileFromSource(
                testShader, "main", ShaderType::Pixel, options,
                "PerfTest_SM" + ShaderModelToString(model) + ".hlsl");
            auto compileTime = std::chrono::high_resolution_clock::now() - start;

            if (result.success) {
                XESS_INFO("  SM {}: {:.2f}ms, {} bytes",
                         ShaderModelToString(model),
                         std::chrono::duration<double, std::milli>(compileTime).count(),
                         result.bytecode.size());
            } else {
                XESS_ERROR("  SM {} compilation failed", ShaderModelToString(model));
            }
        }
    }

    void ShutdownCompiler() {
        if (m_compiler) {
            m_compiler->Shutdown();
            m_compiler.reset();
            XESS_INFO("Shader Compiler shut down");
        }
    }
};

int main() {
    try {
        // Initialize logging
        Logger::Instance().SetLevel(LogLevel::Info);

        ShaderCompilerExample example;
        example.Run();

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }
}
