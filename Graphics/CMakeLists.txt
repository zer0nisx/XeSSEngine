set(GRAPHICS_SOURCES
    Device.h
    Device.cpp
    SwapChain.h
    SwapChain.cpp
    Resource.h
    Resource.cpp
    Pipeline.h
    Pipeline.cpp
    Shader.h
    Shader.cpp
    Buffer.h
    Buffer.cpp
    RenderTarget.h
    RenderTarget.cpp
    Context.h
    Context.cpp
)

add_library(XeSSGraphics STATIC ${GRAPHICS_SOURCES})

target_include_directories(XeSSGraphics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(XeSSGraphics PUBLIC XeSSCore d3d11 dxgi dxguid d3dcompiler)
target_compile_features(XeSSGraphics PUBLIC cxx_std_20)
