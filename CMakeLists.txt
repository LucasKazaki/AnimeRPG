cmake_minimum_required(VERSION 3.25)
project(AstralEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(AstralGame WIN32
    Engine/Assets/StaticMesh.cpp
    Engine/Core/Clock.cpp
    Engine/Core/Logger.cpp
    Engine/Platform/Win32Application.cpp
    Engine/Renderer/Renderer.cpp
    Engine/Scene/Camera.cpp
    Engine/Scene/CombatSandbox.cpp
    Engine/Scene/LandmarkEncounter.cpp
    Engine/Scene/LandmarkInteraction.cpp
    Engine/Scene/PlayerController.cpp
    Engine/Scene/ShadowbladeActions.cpp
    Engine/Scene/ThoughtCommands.cpp
    Engine/Scene/Transform.cpp
    Engine/Scene/WorldBlockout.cpp
    Game/Main.cpp
)

target_include_directories(AstralGame PRIVATE .)
target_compile_definitions(AstralGame PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(AstralGame PRIVATE user32 gdi32)

include(CTest)
if(BUILD_TESTING)
    include(cmake/AstralTestSafety.cmake)
    add_executable(AstralMathTests Tests/MathTests.cpp)
    target_include_directories(AstralMathTests PRIVATE .)
    astral_add_test(AstralMathTests)

    add_executable(AstralSceneTests
        Engine/Assets/StaticMesh.cpp
        Engine/Scene/Camera.cpp
        Engine/Scene/PlayerController.cpp
        Engine/Scene/Transform.cpp
        Tests/SceneTests.cpp
    )
    target_include_directories(AstralSceneTests PRIVATE .)
    astral_add_test(AstralSceneTests)

    add_executable(AssetValidationTests
        Engine/Assets/StaticMesh.cpp
        Tests/AssetValidationTests.cpp
    )
    target_include_directories(AssetValidationTests PRIVATE .)
    astral_add_test(AssetValidationTests)

    add_executable(WorldBlockoutTests
        Engine/Scene/WorldBlockout.cpp
        Tests/WorldBlockoutTests.cpp
    )
    target_include_directories(WorldBlockoutTests PRIVATE .)
    astral_add_test(WorldBlockoutTests)

    add_executable(CombatSandboxTests
        Engine/Scene/CombatSandbox.cpp
        Tests/CombatSandboxTests.cpp
    )
    target_include_directories(CombatSandboxTests PRIVATE .)
    astral_add_test(CombatSandboxTests)

    add_executable(ShadowbladeActionsTests
        Engine/Scene/CombatSandbox.cpp
        Engine/Scene/ShadowbladeActions.cpp
        Tests/ShadowbladeActionsTests.cpp
    )
    target_include_directories(ShadowbladeActionsTests PRIVATE .)
    astral_add_test(ShadowbladeActionsTests)

    add_executable(ThoughtCommandsTests
        Engine/Scene/CombatSandbox.cpp
        Engine/Scene/ShadowbladeActions.cpp
        Engine/Scene/ThoughtCommands.cpp
        Tests/ThoughtCommandsTests.cpp
    )
    target_include_directories(ThoughtCommandsTests PRIVATE .)
    astral_add_test(ThoughtCommandsTests)

    add_executable(LandmarkInteractionTests
        Engine/Scene/CombatSandbox.cpp
        Engine/Scene/LandmarkInteraction.cpp
        Engine/Scene/ShadowbladeActions.cpp
        Engine/Scene/WorldBlockout.cpp
        Tests/LandmarkInteractionTests.cpp
    )
    target_include_directories(LandmarkInteractionTests PRIVATE .)
    astral_add_test(LandmarkInteractionTests)

    add_executable(LandmarkEncounterTests
        Engine/Scene/CombatSandbox.cpp
        Engine/Scene/LandmarkEncounter.cpp
        Engine/Scene/LandmarkInteraction.cpp
        Engine/Scene/ShadowbladeActions.cpp
        Engine/Scene/WorldBlockout.cpp
        Tests/LandmarkEncounterTests.cpp
    )
    target_include_directories(LandmarkEncounterTests PRIVATE .)
    astral_add_test(LandmarkEncounterTests)

    add_executable(M4RuntimeSmoke Tests/CombatSandboxTests.cpp)
    target_include_directories(M4RuntimeSmoke PRIVATE .)
    target_compile_definitions(M4RuntimeSmoke PRIVATE
        ASTRAL_RUNTIME_SMOKE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(M4RuntimeSmoke PRIVATE user32)
    astral_add_test(M4RuntimeSmoke
        $<TARGET_FILE:AstralGame> ${CMAKE_SOURCE_DIR})

    add_executable(M5RuntimeSmoke Tests/M5RuntimeSmoke.cpp)
    target_include_directories(M5RuntimeSmoke PRIVATE .)
    target_compile_definitions(M5RuntimeSmoke PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(M5RuntimeSmoke PRIVATE user32 gdi32)
    astral_add_test(M5RuntimeSmoke
        $<TARGET_FILE:AstralGame> ${CMAKE_SOURCE_DIR})

    add_executable(M7RuntimeSmoke Tests/M7RuntimeSmoke.cpp)
    target_include_directories(M7RuntimeSmoke PRIVATE .)
    target_compile_definitions(M7RuntimeSmoke PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(M7RuntimeSmoke PRIVATE user32 gdi32)
    astral_add_test(M7RuntimeSmoke
        $<TARGET_FILE:AstralGame> ${CMAKE_SOURCE_DIR})

    add_executable(M8RuntimeSmoke Tests/M8RuntimeSmoke.cpp)
    target_include_directories(M8RuntimeSmoke PRIVATE .)
    target_compile_definitions(M8RuntimeSmoke PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(M8RuntimeSmoke PRIVATE user32 gdi32)
    astral_add_test(M8RuntimeSmoke
        $<TARGET_FILE:AstralGame> ${CMAKE_SOURCE_DIR})

    add_executable(M9RuntimeSmoke Tests/M9RuntimeSmoke.cpp)
    target_include_directories(M9RuntimeSmoke PRIVATE .)
    target_compile_definitions(M9RuntimeSmoke PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(M9RuntimeSmoke PRIVATE user32 gdi32)
    astral_add_test(M9RuntimeSmoke
        $<TARGET_FILE:AstralGame> ${CMAKE_SOURCE_DIR})

    add_executable(M10RuntimeSmoke Tests/M10RuntimeSmoke.cpp)
    target_include_directories(M10RuntimeSmoke PRIVATE .)
    target_compile_definitions(M10RuntimeSmoke PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(M10RuntimeSmoke PRIVATE user32 gdi32)
    astral_add_test(M10RuntimeSmoke
        $<TARGET_FILE:AstralGame> ${CMAKE_SOURCE_DIR})
endif()
