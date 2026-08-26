cmake_minimum_required(VERSION 3.24)

project(PumpDXRebuild VERSION 0.0.1 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "PumpDX Rebuild currently targets Windows because it uses Direct3D 11 and Windows input APIs.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(pumpdx_core
    src/framework/render/LogicalViewport.cpp
)

target_include_directories(pumpdx_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(pumpdx_core PUBLIC cxx_std_20)

add_library(pumpdx_scenes
    src/game/scenes/SceneManager.cpp
    src/game/scenes/main_menu/MainMenuScene.cpp
    src/game/scenes/song_select/SongSelectScene.cpp
    src/game/scenes/gameplay/GameplayScene.cpp
    src/game/scenes/result/ResultScene.cpp
)

target_include_directories(pumpdx_scenes
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(pumpdx_scenes PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(pumpdx_scenes PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
else()
    target_compile_options(pumpdx_scenes PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(pumpdx_chart
    src/game/chart/Beat.cpp
    src/game/chart/TimingMap.cpp
)

target_include_directories(pumpdx_chart
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(pumpdx_chart PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(pumpdx_chart PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
else()
    target_compile_options(pumpdx_chart PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(PumpDXRebuild WIN32
    src/framework/application/main.cpp
    src/framework/application/GameApplication.cpp
)

target_link_libraries(PumpDXRebuild
    PRIVATE
        pumpdx_core
        pumpdx_scenes
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

    add_executable(PumpDXSceneTests
        tests/SceneManagerTests.cpp
    )

    target_link_libraries(PumpDXSceneTests PRIVATE pumpdx_scenes)
    target_compile_features(PumpDXSceneTests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(PumpDXSceneTests PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
    else()
        target_compile_options(PumpDXSceneTests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME PumpDXSceneTests COMMAND PumpDXSceneTests)

    add_executable(PumpDXChartTests
        tests/TimingMapTests.cpp
    )

    target_link_libraries(PumpDXChartTests PRIVATE pumpdx_chart)
    target_compile_features(PumpDXChartTests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(PumpDXChartTests PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
    else()
        target_compile_options(PumpDXChartTests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME PumpDXChartTests COMMAND PumpDXChartTests)
endif()
