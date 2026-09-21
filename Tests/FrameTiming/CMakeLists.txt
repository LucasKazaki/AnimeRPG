cmake_minimum_required(VERSION 3.25)
project(AstralFrameTimingValidation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ASTRAL_FRAME_TIMING_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_executable(AstralFrameTimingTests
    ../../Engine/Core/Clock.cpp
    ../../Engine/Core/FrameTimingCapture.cpp
    ../FrameTimingCaptureTests.cpp
)
target_include_directories(AstralFrameTimingTests PRIVATE ../..)

if(ASTRAL_FRAME_TIMING_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(AstralFrameTimingTests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(AstralFrameTimingTests PRIVATE -fsanitize=address,undefined)
endif()

enable_testing()
add_test(NAME AstralFrameTimingTests COMMAND AstralFrameTimingTests)
