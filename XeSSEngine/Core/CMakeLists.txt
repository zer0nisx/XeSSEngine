set(CORE_SOURCES
    Types.h
    Logger.h
    Logger.cpp
    Utils.h
    Utils.cpp
    Exception.h
    Exception.cpp
    NonCopyable.h
)

add_library(XeSSCore STATIC ${CORE_SOURCES})

target_include_directories(XeSSCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(XeSSCore PUBLIC cxx_std_20)
