cmake_minimum_required(VERSION 3.25)
project(AstralExternalConsumer LANGUAGES CXX)

set(ASTRAL_ENGINE_ROOT "" CACHE PATH "Explicit Astral source checkout")
if(NOT EXISTS "${ASTRAL_ENGINE_ROOT}/cmake/AstralRuntime.cmake")
    message(FATAL_ERROR "Set ASTRAL_ENGINE_ROOT to the explicit engine checkout")
endif()
include("${ASTRAL_ENGINE_ROOT}/cmake/AstralRuntime.cmake")
astral_add_runtime()

add_executable(ExternalProjectConsumer Main.cpp)
target_link_libraries(ExternalProjectConsumer PRIVATE Astral::Runtime)
# No root add_subdirectory, Showcase, sample registration or private includes.
# C++17, headers and Threads arrive through the public Runtime target.
