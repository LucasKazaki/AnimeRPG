cmake_minimum_required(VERSION 3.25)
project(AstralNumericAudit LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
get_filename_component(ASTRAL_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
include(CTest)
option(ASTRAL_AUDIT_SANITIZERS "Instrument the audit component tests with Clang/GCC sanitizers" OFF)
if(BUILD_TESTING)
    include("${ASTRAL_SOURCE_ROOT}/cmake/AstralTestSafety.cmake")
    add_executable(NumericAuditTests
        NumericAuditTests.cpp
        "${ASTRAL_SOURCE_ROOT}/Engine/Scene/Camera.cpp"
        "${ASTRAL_SOURCE_ROOT}/Engine/Scene/PlayerController.cpp"
        "${ASTRAL_SOURCE_ROOT}/Engine/Scene/Transform.cpp"
        "${ASTRAL_SOURCE_ROOT}/Engine/Scene/CombatSandbox.cpp")
    add_executable(NumericAuditLegacySceneTests
        "${ASTRAL_SOURCE_ROOT}/Tests/SceneTests.cpp"
        "${ASTRAL_SOURCE_ROOT}/Engine/Assets/StaticMesh.cpp"
        "${ASTRAL_SOURCE_ROOT}/Engine/Scene/Camera.cpp"
        "${ASTRAL_SOURCE_ROOT}/Engine/Scene/PlayerController.cpp"
        "${ASTRAL_SOURCE_ROOT}/Engine/Scene/Transform.cpp")
    foreach(audit_target IN ITEMS NumericAuditTests NumericAuditLegacySceneTests)
        target_include_directories(${audit_target} PRIVATE "${ASTRAL_SOURCE_ROOT}")
        if(MSVC)
            target_compile_options(${audit_target} PRIVATE /W4 /WX)
        else()
            target_compile_options(${audit_target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
        endif()
        if(ASTRAL_AUDIT_SANITIZERS)
            if(MSVC OR NOT CMAKE_CXX_COMPILER_ID MATCHES "^(Clang|GNU|AppleClang)$")
                message(FATAL_ERROR "Audit sanitizers require the supported Clang/GCC flag interface")
            endif()
            target_compile_options(${audit_target} PRIVATE
                -fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=all -fno-omit-frame-pointer)
            target_link_options(${audit_target} PRIVATE -fsanitize=address,undefined,float-cast-overflow)
        endif()
        astral_add_test(${audit_target})
    endforeach()
    # The unchanged legacy fixture uses a fixed temporary filename. Serialize it
    # within this CTest invocation; concurrent processes still need separate hosts.
    set_tests_properties(NumericAuditLegacySceneTests PROPERTIES RUN_SERIAL TRUE)
endif()
