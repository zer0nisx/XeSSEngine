set(APPLICATION_SOURCES
    Window.h
    Window.cpp
    Application.h
    Application.cpp
    Input.h
    Input.cpp
)

add_library(XeSSApplication STATIC ${APPLICATION_SOURCES})

target_include_directories(XeSSApplication PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(XeSSApplication PUBLIC
    XeSSCore
    XeSSGraphics
    XeSSModule
)
target_compile_features(XeSSApplication PUBLIC cxx_std_20)
