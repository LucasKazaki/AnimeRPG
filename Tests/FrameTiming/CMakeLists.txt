cmake_minimum_required(VERSION 3.25)
project(AstralFrameTimingValidation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ASTRAL_FRAME_TIMING_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_executable(AstralFrameTimingTests
    ../../Engine/Core/Clock.cpp
    ../../Engine/Core/FrameTimingCapture.cpp
    ../../Engine/Core/ProcessMemoryCapture.cpp
    ../../Engine/Core/SimulationTimeStep.cpp
    ../FrameTimingCaptureTests.cpp
)
target_include_directories(AstralFrameTimingTests PRIVATE ../..)

add_executable(AstralFramePhaseTimingTests
    ../../Engine/Core/FramePhaseTimingCapture.cpp
    ../FramePhaseTimingCaptureTests.cpp
)
target_include_directories(AstralFramePhaseTimingTests PRIVATE ../..)

add_executable(AstralProcessMemoryTests
    ../../Engine/Core/ProcessMemoryCapture.cpp
    ../ProcessMemoryCaptureTests.cpp
)
target_include_directories(AstralProcessMemoryTests PRIVATE ../..)

add_executable(AstralSimulationTimeStepTests
    ../../Engine/Core/Clock.cpp
    ../../Engine/Core/FrameTimingCapture.cpp
    ../../Engine/Core/ProcessMemoryCapture.cpp
    ../../Engine/Core/SimulationTimeStep.cpp
    ../SimulationTimeStepTests.cpp
)
target_include_directories(AstralSimulationTimeStepTests PRIVATE ../..)

add_executable(AstralBenchmarkRunControlTests
    ../../Engine/Core/BenchmarkRunControl.cpp
    ../BenchmarkRunControlTests.cpp
)
target_include_directories(AstralBenchmarkRunControlTests PRIVATE ../..)

add_executable(AstralProfilingCaptureStateReceiptTests
    ../../Engine/Core/BenchmarkRunControl.cpp
    ../../Engine/Core/ProfilingCaptureStateReceipt.cpp
    ../ProfilingCaptureStateReceiptTests.cpp
)
target_include_directories(AstralProfilingCaptureStateReceiptTests PRIVATE ../..)

if(ASTRAL_FRAME_TIMING_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    foreach(target AstralFrameTimingTests AstralFramePhaseTimingTests AstralProcessMemoryTests AstralSimulationTimeStepTests AstralBenchmarkRunControlTests AstralProfilingCaptureStateReceiptTests)
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endforeach()
endif()

enable_testing()
add_test(NAME AstralFrameTimingTests COMMAND AstralFrameTimingTests)
add_test(NAME AstralFramePhaseTimingTests COMMAND AstralFramePhaseTimingTests)
add_test(NAME AstralProcessMemoryTests COMMAND AstralProcessMemoryTests)
add_test(NAME AstralSimulationTimeStepTests COMMAND AstralSimulationTimeStepTests)
add_test(NAME AstralBenchmarkRunControlTests COMMAND AstralBenchmarkRunControlTests)
add_test(NAME AstralProfilingCaptureStateReceiptTests COMMAND AstralProfilingCaptureStateReceiptTests)
