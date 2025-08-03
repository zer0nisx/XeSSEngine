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
    # ${CMAKE_CURRENT_SOURCE_DIR}/Rendering
    ${CMAKE_CURRENT_SOURCE_DIR}/Application
    ${CMAKE_CURRENT_SOURCE_DIR}/SDK/XeSS_SDK_2.1.0/inc
)

# Core module
add_subdirectory(Core)

# Graphics module
add_subdirectory(Graphics)

# ShaderCompiler module
add_subdirectory(Graphics/ShaderCompiler)

# XeSS module
add_subdirectory(XeSS)

# Rendering module (commented out until implemented)
# add_subdirectory(Rendering)

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

# Link XeSS libraries
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/SDK/XeSS_SDK_2.1.0/lib)

# Copy shaders
add_custom_target(CopyShaders ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Shaders"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders"
    COMMENT "Copying shader files"
)
