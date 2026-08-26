cmake_minimum_required(VERSION 3.24)

project(PumpDXRebuild VERSION 0.0.1 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "PumpDX Rebuild currently targets Windows because it uses Direct3D 11 and Windows input APIs.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(pumpdx_core
    src/core/LogicalViewport.cpp
)

target_include_directories(pumpdx_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(pumpdx_core PUBLIC cxx_std_20)

add_executable(PumpDXRebuild WIN32
    src/app/main.cpp
    src/app/GameApplication.cpp
)

target_link_libraries(PumpDXRebuild
    PRIVATE
        pumpdx_core
        d3d11
        dxgi
)

target_compile_definitions(PumpDXRebuild PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

if(MSVC)
    target_compile_options(pumpdx_core PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
    target_compile_options(PumpDXRebuild PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
else()
    target_compile_options(pumpdx_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(PumpDXRebuild PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)

if(BUILD_TESTING)
    add_executable(PumpDXCoreTests
        tests/LogicalViewportTests.cpp
    )

    target_link_libraries(PumpDXCoreTests PRIVATE pumpdx_core)
    target_compile_features(PumpDXCoreTests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(PumpDXCoreTests PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
    else()
        target_compile_options(PumpDXCoreTests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME PumpDXCoreTests COMMAND PumpDXCoreTests)
endif()
