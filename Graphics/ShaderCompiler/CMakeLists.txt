set(SHADER_COMPILER_SOURCES
    ShaderCompiler.h
    ShaderCompiler.cpp
)

add_library(XeSSShaderCompiler STATIC ${SHADER_COMPILER_SOURCES})

target_include_directories(XeSSShaderCompiler PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../SDK/XeSS_SDK_2.1.0/inc
)

target_link_libraries(XeSSShaderCompiler PUBLIC
    XeSSCore
    d3d11.lib
    d3dcompiler.lib
    dxcompiler.lib
    dxguid.lib
)

target_compile_features(XeSSShaderCompiler PUBLIC cxx_std_20)

# Add preprocessor definitions for Shader Model support
target_compile_definitions(XeSSShaderCompiler PUBLIC
    XESS_SHADER_MODEL_64_SUPPORT=1
    XESS_DXC_SUPPORT=1
    XESS_WAVE_INTRINSICS_SUPPORT=1
    XESS_VRS_SUPPORT=1
)
