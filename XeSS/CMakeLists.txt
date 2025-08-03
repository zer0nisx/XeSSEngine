set(XESS_SOURCES
    XeSSContext.h
    XeSSContext.cpp
    XeSSTypes.h
)

add_library(XeSSModule STATIC ${XESS_SOURCES})

target_include_directories(XeSSModule PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../SDK/XeSS_SDK_2.1.0/inc
)

target_link_libraries(XeSSModule PUBLIC
    XeSSCore
    XeSSGraphics
    ${CMAKE_CURRENT_SOURCE_DIR}/../SDK/XeSS_SDK_2.1.0/lib/libxess_dx11.lib
)

target_compile_features(XeSSModule PUBLIC cxx_std_20)
