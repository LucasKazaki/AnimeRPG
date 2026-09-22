cmake_minimum_required(VERSION 3.25)
project(AstralAssetValidation LANGUAGES CXX)

# This headless subproject compiles the REAL loader and shared regression source.
# It is not a portable build of AstralGame or a replacement for Windows CTest.
get_filename_component(ASTRAL_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(ASTRAL_ASSET_SANITIZERS "Enable Clang/GCC address and undefined sanitizers" OFF)

add_executable(AssetValidationTests
    "${ASTRAL_ROOT}/Engine/Assets/StaticMesh.cpp"
    "${ASTRAL_ROOT}/Tests/AssetValidationTests.cpp")
target_include_directories(AssetValidationTests PRIVATE "${ASTRAL_ROOT}")
if(MSVC)
    target_compile_options(AssetValidationTests PRIVATE /W4 /WX)
else()
    target_compile_options(AssetValidationTests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
if(ASTRAL_ASSET_SANITIZERS)
    if(MSVC OR NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        message(FATAL_ERROR "This sanitizer configuration requires Clang or GCC on a Unix host")
    endif()
    target_compile_options(AssetValidationTests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(AssetValidationTests PRIVATE -fsanitize=address,undefined)
endif()
include(CTest)
if(BUILD_TESTING)
    add_test(NAME AssetValidationTests COMMAND AssetValidationTests)
    set_tests_properties(AssetValidationTests PROPERTIES TIMEOUT 60)
endif()
