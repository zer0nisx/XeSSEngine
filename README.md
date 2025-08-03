# XeSS Engine

Un motor de renderizado modular basado en DirectX 11 con integración de Intel XeSS Super Resolution.

## Características

- ⚡ **Arquitectura Modular**: Componentes separados y reutilizables
- 🎮 **DirectX 11**: Soporte completo para gráficos modernos
- 🚀 **Intel XeSS**: Super resolution integrado para mejor rendimiento
- 🔧 **C++20**: Utiliza las últimas características del lenguaje
- 📦 **Fácil de usar**: API simple y clara

## Estructura del Proyecto

```
XeSSEngine/
├── Core/           # Clases base y utilidades
├── Graphics/       # Wrapper de DirectX 11
├── XeSS/          # Integración Intel XeSS
├── Rendering/     # Pipeline de renderizado
├── Application/   # Framework de aplicación
├── Examples/      # Ejemplos de uso
└── Shaders/       # Shaders HLSL
```

## Dependencias

- Windows 10/11
- DirectX 11
- Intel XeSS SDK 2.1.0
- Visual Studio 2019 o superior con C++20

## Uso Rápido

```cpp
#include "XeSSEngine.h"

int main() {
    XeSS::Application app(1920, 1080, "Mi Aplicación");

    app.SetXeSSQuality(XeSS::Quality::Performance);
    app.Run();

    return 0;
}
```

## Características Técnicas

- **Jitter Anti-Aliasing**: Secuencia Halton para mejor calidad
- **Pipeline Modular**: Separación clara de responsabilidades
- **Resource Management**: Gestión automática de recursos DX11
- **Error Handling**: Manejo robusto de errores
- **Debug Support**: Soporte completo para debugging

## Compilación

Requiere CMake 3.22 o superior:

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```
