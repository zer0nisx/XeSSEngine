#pragma once

#include "Application/Application.h"
#include "Core/Utils.h"
#include <vector>
#include <memory>

namespace BasicExample {

using namespace XeSS;

class BasicExampleApp : public Application::Application {
public:
    BasicExampleApp();
    ~BasicExampleApp() override;

protected:
    void OnInitialize() override;
    void OnShutdown() override;
    void OnUpdate(float deltaTime) override;
    void OnRender() override;
    void OnKeyUp(uint32 key) override;

private:
    void CreateTriangleGeometry();
    void CreateShaders();
    void CreateConstantBuffer();
    void SetupJitter();
    void UpdateConstantBuffer(float deltaTime);
    void RenderScene();
    void RunXeSS();
    void PresentToScreen();

    // Vertex structure matching the original sample
    struct Vertex {
        Vector3 position;
        Vector4 color;
    };

    struct SceneConstants {
        Vector4 offset{0.0f, 0.0f, 0.0f, 0.0f};  // x,y = movement, z,w = jitter
        Vector4 velocity{0.0f, 0.0f, 0.0f, 0.0f};
        Vector4 resolution{0.0f, 0.0f, 0.0f, 0.0f};
        float padding[52]{}; // Pad to 256 bytes
    };
    static_assert(sizeof(SceneConstants) == 256, "Constant buffer must be 256 bytes");

    // Resources
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBuffer;

    // Shaders
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_colorVertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_colorPixelShader;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_velocityVertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_velocityPixelShader;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_fullscreenVertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_fullscreenPixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;

    // Render targets
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_colorTexture;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_velocityTexture;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_depthTexture;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_xessOutputTexture;

    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_colorRTV;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_velocityRTV;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthDSV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_colorSRV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_velocitySRV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_depthSRV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_xessOutputSRV;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_xessOutputUAV;

    // Rasterizer and other states
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizerState;
    Microsoft::WRL::ComPtr<ID3D11BlendState> m_blendState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthStencilState;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samplerState;

    // Animation and jitter
    SceneConstants m_sceneConstants;
    std::vector<std::pair<float, float>> m_haltonSequence;
    size_t m_haltonIndex{0};
    float m_animationOffset{0.0f};
    bool m_paused{false};

    // Clear colors
    static constexpr float ClearColor[4] = {0.0f, 0.2f, 0.4f, 1.0f};
    static constexpr float ClearBlack[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

} // namespace BasicExample
