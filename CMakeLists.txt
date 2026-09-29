cmake_minimum_required(VERSION 3.25)
project(AstralEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Portable Astral engine runtime and the Astral-mode showcase (see
# cmake/AstralEngineSources.cmake; Tests/EngineRuntime builds the same sources
# headless on Linux with sanitizers).
include(cmake/AstralEngineSources.cmake)
find_package(Threads REQUIRED)
add_library(AstralEngineRuntime STATIC ${ASTRAL_ENGINE_RUNTIME_SOURCES})
target_include_directories(AstralEngineRuntime PUBLIC .)
target_link_libraries(AstralEngineRuntime PUBLIC Threads::Threads)
add_library(AstralShowcase STATIC ${ASTRAL_SHOWCASE_SOURCES} ${ASTRAL_GAMEPLAY_SOURCES})
target_link_libraries(AstralShowcase PUBLIC AstralEngineRuntime)

add_executable(AstralGame WIN32
    Engine/Assets/StaticMesh.cpp
    Engine/Core/BenchmarkRunControl.cpp
    Engine/Core/Clock.cpp
    Engine/Core/FramePhaseTimingCapture.cpp
    Engine/Core/FrameTimingCapture.cpp
    Engine/Core/ProcessMemoryCapture.cpp
    Engine/Core/ProfilingCaptureStateReceipt.cpp
    Engine/Core/SimulationTimeStep.cpp
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
# Opt-in Astral renderer (F2 / ASTRAL_RENDER_MODE=astral). AstralGame already
# compiles the gameplay sources, so it takes the showcase sources directly.
target_sources(AstralGame PRIVATE ${ASTRAL_SHOWCASE_SOURCES} Game/Showcase/Win32AstralPresenter.cpp)
target_link_libraries(AstralGame PRIVATE AstralEngineRuntime winmm)
if(MSVC)
    target_link_options(AstralGame PRIVATE "$<$<CONFIG:Release>:/Brepro>")
endif()

add_executable(AstralEditor WIN32
    Engine/Editor/EditorLayout.cpp
    Tools/AstralEditorMain.cpp
)
target_include_directories(AstralEditor PRIVATE .)
target_compile_definitions(AstralEditor PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(AstralEditor PRIVATE user32 gdi32)

# Headless National Mall capture: PNG frames, WAV mix and Chrome trace.
add_executable(AstralCapture Tools/AstralCapture.cpp)
target_link_libraries(AstralCapture PRIVATE AstralShowcase)

include(CTest)
if(BUILD_TESTING)
    include(cmake/AstralTestSafety.cmake)
    add_executable(AstralMathTests Tests/MathTests.cpp)
    target_include_directories(AstralMathTests PRIVATE .)
    astral_add_test(AstralMathTests)

    add_executable(FrameTimingCaptureTests
        Engine/Core/Clock.cpp
        Engine/Core/FrameTimingCapture.cpp
        Engine/Core/ProcessMemoryCapture.cpp
        Engine/Core/SimulationTimeStep.cpp
        Tests/FrameTimingCaptureTests.cpp
    )
    target_include_directories(FrameTimingCaptureTests PRIVATE .)
    astral_add_test(FrameTimingCaptureTests)

    add_executable(FramePhaseTimingCaptureTests
        Engine/Core/FramePhaseTimingCapture.cpp
        Tests/FramePhaseTimingCaptureTests.cpp
    )
    target_include_directories(FramePhaseTimingCaptureTests PRIVATE .)
    astral_add_test(FramePhaseTimingCaptureTests)

    add_executable(ProcessMemoryCaptureTests
        Engine/Core/ProcessMemoryCapture.cpp
        Tests/ProcessMemoryCaptureTests.cpp
    )
    target_include_directories(ProcessMemoryCaptureTests PRIVATE .)
    astral_add_test(ProcessMemoryCaptureTests)

    add_executable(SimulationTimeStepTests
        Engine/Core/Clock.cpp
        Engine/Core/FrameTimingCapture.cpp
        Engine/Core/ProcessMemoryCapture.cpp
        Engine/Core/SimulationTimeStep.cpp
        Tests/SimulationTimeStepTests.cpp
    )
    target_include_directories(SimulationTimeStepTests PRIVATE .)
    astral_add_test(SimulationTimeStepTests)

    add_executable(BenchmarkRunControlTests
        Engine/Core/BenchmarkRunControl.cpp
        Tests/BenchmarkRunControlTests.cpp
    )
    target_include_directories(BenchmarkRunControlTests PRIVATE .)
    astral_add_test(BenchmarkRunControlTests)

    add_executable(ProfilingCaptureStateReceiptTests
        Engine/Core/BenchmarkRunControl.cpp
        Engine/Core/ProfilingCaptureStateReceipt.cpp
        Tests/ProfilingCaptureStateReceiptTests.cpp
    )
    target_include_directories(ProfilingCaptureStateReceiptTests PRIVATE .)
    astral_add_test(ProfilingCaptureStateReceiptTests)

    add_executable(EditorLayoutTests
        Engine/Editor/EditorLayout.cpp
        Tests/EditorLayoutTests.cpp
    )
    target_include_directories(EditorLayoutTests PRIVATE .)
    astral_add_test(EditorLayoutTests)

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

    # Portable engine suites (ASTRAL_ENGINE_TEST_SUITES); renders make them slower in Debug.
    add_executable(EngineMathTests Tests/EngineMathTests.cpp)
    target_link_libraries(EngineMathTests PRIVATE AstralShowcase)
    astral_add_test(EngineMathTests)
    add_executable(EngineCoreTests Tests/EngineCoreTests.cpp)
    target_link_libraries(EngineCoreTests PRIVATE AstralShowcase)
    astral_add_test(EngineCoreTests)
    add_executable(EngineGraphicsTests Tests/EngineGraphicsTests.cpp)
    target_link_libraries(EngineGraphicsTests PRIVATE AstralShowcase)
    astral_add_test(EngineGraphicsTests)
    add_executable(Engine2DTests Tests/Engine2DTests.cpp)
    target_link_libraries(Engine2DTests PRIVATE AstralShowcase)
    astral_add_test(Engine2DTests)
    add_executable(EngineWorldTests Tests/EngineWorldTests.cpp)
    target_link_libraries(EngineWorldTests PRIVATE AstralShowcase)
    astral_add_test(EngineWorldTests)
    add_executable(EnginePhysicsTests Tests/EnginePhysicsTests.cpp)
    target_link_libraries(EnginePhysicsTests PRIVATE AstralShowcase)
    astral_add_test(EnginePhysicsTests)
    add_executable(EngineAnimationTests Tests/EngineAnimationTests.cpp)
    target_link_libraries(EngineAnimationTests PRIVATE AstralShowcase)
    astral_add_test(EngineAnimationTests)
    add_executable(EngineSystemsTests Tests/EngineSystemsTests.cpp)
    target_link_libraries(EngineSystemsTests PRIVATE AstralShowcase)
    astral_add_test(EngineSystemsTests)
    add_executable(EngineShowcaseTests Tests/EngineShowcaseTests.cpp)
    target_link_libraries(EngineShowcaseTests PRIVATE AstralShowcase)
    astral_add_test(EngineShowcaseTests)
    add_executable(EngineServicesTests Tests/EngineServicesTests.cpp)
    target_link_libraries(EngineServicesTests PRIVATE AstralShowcase)
    astral_add_test(EngineServicesTests)
    add_executable(EngineAssetsTests Tests/EngineAssetsTests.cpp)
    target_link_libraries(EngineAssetsTests PRIVATE AstralShowcase)
    astral_add_test(EngineAssetsTests)
    add_executable(EngineFrameworkTests Tests/EngineFrameworkTests.cpp)
    target_link_libraries(EngineFrameworkTests PRIVATE AstralShowcase)
    astral_add_test(EngineFrameworkTests)
    set_tests_properties(${ASTRAL_ENGINE_TEST_SUITES} PROPERTIES TIMEOUT 300)

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
