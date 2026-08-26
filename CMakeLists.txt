cmake_minimum_required(VERSION 3.24)

project(PumpDXRebuild VERSION 0.0.1 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "PumpDX Rebuild currently targets Windows because it uses Direct3D 11 and Windows input APIs.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Static libraries exchange std::string/std::vector values across target boundaries.
# Keep every target on the same MSVC runtime so Debug iterator metadata has one layout.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

add_library(pumpdx_core
    src/framework/assets/ResourceCache.cpp
    src/framework/assets/ThemeManifest.cpp
    src/framework/render/LogicalViewport.cpp
    src/framework/render/SceneOverlayRenderer.cpp
)

target_include_directories(pumpdx_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(pumpdx_core PUBLIC cxx_std_20)
target_compile_definitions(pumpdx_core PUBLIC NOMINMAX)

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

add_library(pumpdx_session
    src/game/content/SongCatalog.cpp
    src/game/session/PlaySession.cpp
)

target_include_directories(pumpdx_session
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(pumpdx_session PUBLIC cxx_std_20)
target_link_libraries(pumpdx_scenes PUBLIC pumpdx_session)

add_library(pumpdx_gameflow
    src/game/application/GameFlow.cpp
)

target_include_directories(pumpdx_gameflow
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(pumpdx_gameflow PUBLIC cxx_std_20)
target_link_libraries(pumpdx_gameflow PUBLIC pumpdx_scenes pumpdx_session)

if(MSVC)
    target_compile_options(pumpdx_scenes PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
else()
    target_compile_options(pumpdx_scenes PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(pumpdx_chart
    src/game/chart/Beat.cpp
    src/game/chart/Chart.cpp
    src/game/chart/ChartValidator.cpp
    src/game/chart/TimingMap.cpp
)

target_include_directories(pumpdx_chart
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(pumpdx_chart PUBLIC cxx_std_20)
target_link_libraries(pumpdx_session PUBLIC pumpdx_chart)

add_library(pumpdx_gameplay
    src/game/gameplay/DebugSongClock.cpp
    src/game/gameplay/GameplayRuntime.cpp
)

target_include_directories(pumpdx_gameplay
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(pumpdx_gameplay PUBLIC cxx_std_20)
target_link_libraries(pumpdx_gameplay PUBLIC pumpdx_chart pumpdx_core)
target_link_libraries(pumpdx_gameflow PUBLIC pumpdx_gameplay)

if(MSVC)
    target_compile_options(pumpdx_chart PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
    target_compile_options(pumpdx_gameplay PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
else()
    target_compile_options(pumpdx_chart PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(pumpdx_gameplay PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(PumpDXRebuild WIN32
    src/game/application/main.cpp
    src/game/application/GameApplication.cpp
)

target_link_libraries(PumpDXRebuild
    PRIVATE
        pumpdx_core
        pumpdx_gameflow
        d3d11
        d2d1
        dwrite
        dxgi
)

target_compile_definitions(PumpDXRebuild PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    PUMP_DX_DEFAULT_THEME_MANIFEST=L\"${CMAKE_CURRENT_SOURCE_DIR}/assets/themes/default/theme.json\"
)

if(MSVC)
    target_compile_options(pumpdx_core PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
    target_compile_options(pumpdx_session PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
    target_compile_options(pumpdx_gameflow PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
    target_compile_options(PumpDXRebuild PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
else()
    target_compile_options(pumpdx_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(pumpdx_session PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(pumpdx_gameflow PRIVATE -Wall -Wextra -Wpedantic)
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

    add_executable(PumpDXThemeTests
        tests/ThemeManifestTests.cpp
    )

    target_link_libraries(PumpDXThemeTests PRIVATE pumpdx_core)
    target_compile_features(PumpDXThemeTests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(PumpDXThemeTests PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
    else()
        target_compile_options(PumpDXThemeTests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME PumpDXThemeTests COMMAND PumpDXThemeTests)

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

    add_executable(PumpDXGameFlowTests
        tests/GameFlowTests.cpp
    )

    target_link_libraries(PumpDXGameFlowTests PRIVATE pumpdx_gameflow)
    target_compile_features(PumpDXGameFlowTests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(PumpDXGameFlowTests PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
    else()
        target_compile_options(PumpDXGameFlowTests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME PumpDXGameFlowTests COMMAND PumpDXGameFlowTests)

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

    add_executable(PumpDXChartModelTests
        tests/ChartTests.cpp
    )

    target_link_libraries(PumpDXChartModelTests PRIVATE pumpdx_chart)
    target_compile_features(PumpDXChartModelTests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(PumpDXChartModelTests PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
    else()
        target_compile_options(PumpDXChartModelTests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME PumpDXChartModelTests COMMAND PumpDXChartModelTests)

    add_executable(PumpDXGameplayRuntimeTests
        tests/GameplayRuntimeTests.cpp
    )

    target_link_libraries(PumpDXGameplayRuntimeTests PRIVATE pumpdx_gameplay)
    target_compile_features(PumpDXGameplayRuntimeTests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(PumpDXGameplayRuntimeTests PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
    else()
        target_compile_options(PumpDXGameplayRuntimeTests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME PumpDXGameplayRuntimeTests COMMAND PumpDXGameplayRuntimeTests)
endif()
