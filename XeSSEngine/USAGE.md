# XeSS Engine - Guía de Uso

## Introducción

XeSS Engine es un motor de renderizado modular diseñado para simplificar la integración de Intel XeSS (Xe Super Sampling) en aplicaciones DirectX 11. El engine está construido con C++20 y sigue principios de diseño moderno.

## Arquitectura Modular

El engine está dividido en módulos independientes:

### Core
- **Types.h**: Tipos básicos, vectores, estructuras de datos
- **Logger.h**: Sistema de logging avanzado con diferentes niveles
- **Exception.h**: Manejo robusto de errores y excepciones
- **Utils.h**: Utilidades incluyendo secuencia Halton para jitter
- **NonCopyable.h**: Clase base para objetos no copiables

### Graphics
- **Device.h**: Wrapper para dispositivo DirectX 11
- **SwapChain.h**: Gestión de la cadena de intercambio
- **Resource.h**: Gestión de recursos gráficos
- **Pipeline.h**: Estados de pipeline de renderizado

### XeSS
- **XeSSContext.h**: Wrapper principal para Intel XeSS
- **XeSSTypes.h**: Tipos y conversiones específicas de XeSS

### Application
- **Application.h**: Framework principal de aplicación
- **Window.h**: Gestión de ventanas Win32

## Uso Básico

### 1. Aplicación Simple

```cpp
#include "XeSSEngine.h"

class MiApp : public XeSS::Application::Application {
protected:
    void OnInitialize() override {
        // Inicialización de recursos
    }

    void OnRender() override {
        // Lógica de renderizado
    }

    void OnKeyUp(uint32 key) override {
        if (key == VK_SPACE) {
            // Alternar pausa, cambiar calidad, etc.
        }
    }
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    try {
        MiApp app;
        return app.Run();
    }
    catch (const std::exception& e) {
        MessageBoxA(nullptr, e.what(), "Error", MB_OK);
        return -1;
    }
}
```

### 2. Configuración Avanzada

```cpp
XeSS::Application::ApplicationConfig config;
config.title = L"Mi Aplicación XeSS";
config.windowSize = {2560, 1440};
config.enableDebugLayer = true;
config.xessQuality = XeSS::XeSSModule::QualityMode::Quality;

MiApp app(config);
```

### 3. Control de Calidad XeSS

```cpp
// Cambiar calidad en tiempo real
app.SetXeSSQuality(XeSS::XeSSModule::QualityMode::Performance);

// Obtener información
auto quality = app.GetXeSSQuality();
auto inputRes = app.GetXeSSContext().GetInputResolution();
auto outputRes = app.GetXeSSContext().GetOutputResolution();
```

## Pipeline de Renderizado

### Estructura Típica

1. **Render Pass (Resolución Reducida)**
   - Renderizar geometría a resolución de entrada XeSS
   - Generar color, velocidad y profundidad

2. **XeSS Pass**
   - Ejecutar super resolution
   - Generar imagen de alta resolución

3. **Present Pass**
   - Mostrar resultado final

### Ejemplo de Implementación

```cpp
void MiApp::OnRender() {
    auto& device = GetDevice();
    auto& context = device.GetContext();
    auto& xess = GetXeSSContext();

    // 1. Render a resolución de entrada
    RenderLowResolution();

    // 2. Ejecutar XeSS
    XeSS::XeSSModule::ExecuteParams params;
    params.inputResolution = xess.GetInputResolution();
    params.jitterOffset = GetCurrentJitter();
    params.colorTexture = m_colorTexture.Get();
    params.velocityTexture = m_velocityTexture.Get();
    params.depthTexture = m_depthTexture.Get();
    params.outputTexture = m_xessOutput.Get();

    xess.Execute(device, params);

    // 3. Presentar resultado
    PresentFinalResult();
}
```

## Gestión de Jitter

El engine incluye generación automática de secuencias Halton para jitter:

```cpp
// Generar secuencia Halton
auto haltonPoints = XeSS::Utils::GenerateHalton(2, 3, 1, 32);

// Usar en renderizado
Vector2 currentJitter = {haltonPoints[frameIndex % 32].first,
                        haltonPoints[frameIndex % 32].second};
```

## Logging

Sistema de logging integrado con múltiples niveles:

```cpp
// Configurar nivel de log
XeSS::Logger::Instance().SetLevel(XeSS::LogLevel::Debug);

// Usar macros convenientes
XESS_INFO("Aplicación iniciada");
XESS_WARNING("Adaptador no óptimo detectado");
XESS_ERROR("Error al crear recurso: {}", errorMsg);
```

## Manejo de Errores

El engine usa un sistema robusto de excepciones:

```cpp
try {
    device.Initialize();
    xessContext.Initialize(device, outputRes, quality);
}
catch (const XeSS::GraphicsException& e) {
    // Error específico de gráficos
    HandleGraphicsError(e);
}
catch (const XeSS::XeSSException& e) {
    // Error específico de XeSS
    HandleXeSSError(e);
}
```

## Configuración del Proyecto

### CMake

```cmake
find_package(XeSSEngine REQUIRED)
target_link_libraries(MiApp PRIVATE XeSSEngine::XeSSEngine)
```

### Requisitos

- Windows 10/11
- DirectX 11 compatible GPU
- Intel XeSS compatible driver
- Visual Studio 2019+ con C++20

## Ejemplos Incluidos

- **BasicExample**: Triángulo animado con XeSS
- **AdvancedExample**: Escena compleja con múltiples objetos
- **BenchmarkExample**: Herramientas de medición de rendimiento

## Optimización

### Tips de Rendimiento

1. **Usa resoluciones óptimas**: El engine calcula automáticamente la resolución de entrada
2. **Gestiona recursos**: Reutiliza texturas y buffers cuando sea posible
3. **Motion Vectors precisos**: Mejoran significativamente la calidad XeSS
4. **Jitter consistente**: Usa la secuencia Halton proporcionada

### Debugging

```cpp
// Habilitar capa de debug DirectX
config.enableDebugLayer = true;

// Logging detallado
XeSS::Logger::Instance().SetLevel(XeSS::LogLevel::Trace);

// Verificar soporte XeSS
bool supported = XeSS::XeSSModule::XeSSContext::IsQualitySupported(quality);
```

## Integración con Motores Existentes

El engine está diseñado para integrarse fácilmente:

```cpp
// Usar solo el módulo XeSS
#include "XeSS/XeSSContext.h"

XeSS::XeSSModule::XeSSContext xessContext;
xessContext.Initialize(myDevice, resolution, quality);

// En el loop de renderizado
xessContext.Execute(myDevice, executeParams);
```

## Soporte y Comunidad

- Documentación completa en `/docs`
- Ejemplos en `/Examples`
- Issues y contribuciones bienvenidas
- Basado en el sample oficial de Intel XeSS
