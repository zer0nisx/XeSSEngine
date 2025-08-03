set(BASIC_EXAMPLE_SOURCES
    BasicExample.cpp
    BasicExampleApp.h
    BasicExampleApp.cpp
)

add_executable(BasicExample WIN32 ${BASIC_EXAMPLE_SOURCES})

target_link_libraries(BasicExample PRIVATE
    XeSSCore
    XeSSGraphics
    XeSSModule
    XeSSApplication
)

target_compile_features(BasicExample PRIVATE cxx_std_20)
target_compile_definitions(BasicExample PRIVATE UNICODE _UNICODE)

# Copy required files
add_dependencies(BasicExample CopyXeSSFiles CopyShaders)
