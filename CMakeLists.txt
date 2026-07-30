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
    Engine/Scene/PlayerController.cpp
    Engine/Scene/Transform.cpp
    Game/Main.cpp
)

target_include_directories(AstralGame PRIVATE .)
target_compile_definitions(AstralGame PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(AstralGame PRIVATE user32 gdi32)

include(CTest)
if(BUILD_TESTING)
    add_executable(AstralMathTests Tests/MathTests.cpp)
    target_include_directories(AstralMathTests PRIVATE .)
    add_test(NAME AstralMathTests COMMAND AstralMathTests)

    add_executable(AstralSceneTests
        Engine/Assets/StaticMesh.cpp
        Engine/Scene/Camera.cpp
        Engine/Scene/PlayerController.cpp
        Engine/Scene/Transform.cpp
        Tests/SceneTests.cpp
    )
    target_include_directories(AstralSceneTests PRIVATE .)
    add_test(NAME AstralSceneTests COMMAND AstralSceneTests)
endif()
