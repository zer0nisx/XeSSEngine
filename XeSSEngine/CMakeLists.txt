cmake_minimum_required(VERSION 3.22)

project(XeSSEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/Graphics
    ${CMAKE_CURRENT_SOURCE_DIR}/XeSS
    ${CMAKE_CURRENT_SOURCE_DIR}/Rendering
    ${CMAKE_CURRENT_SOURCE_DIR}/Application
)

# Core module
add_subdirectory(Core)

# Graphics module
add_subdirectory(Graphics)

# XeSS module
add_subdirectory(XeSS)

# Rendering module
add_subdirectory(Rendering)

# Application module
add_subdirectory(Application)

# Examples
add_subdirectory(Examples)

# Compiler definitions
add_compile_definitions(UNICODE _UNICODE)

# Compiler options
if(MSVC)
    add_compile_options(/W4 /permissive-)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/Od /Zi)
        add_compile_definitions(_DEBUG)
    else()
        add_compile_options(/O2)
        add_compile_definitions(NDEBUG)
    endif()
endif()

# Copy XeSS DLLs and shaders
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../uploads/XeSS_SDK_2.1.0/bin/")
    set(XESS_BIN_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../uploads/XeSS_SDK_2.1.0/bin/")
    add_custom_target(CopyXeSSFiles ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${XESS_BIN_PATH}/libxess_dx11.dll"
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libxess_dx11.dll"
        COMMENT "Copying XeSS DLLs"
    )
endif()

# Copy shaders
add_custom_target(CopyShaders ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Shaders"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders"
    COMMENT "Copying shader files"
)
