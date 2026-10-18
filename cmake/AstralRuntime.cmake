# Reusable Runtime target for the root build, portable suites and independent
# consumers. No Game/Showcase, editor, player or test targets are created here.
include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/AstralEngineSources.cmake")
find_package(Threads REQUIRED)

function(astral_add_runtime)
    if(TARGET AstralEngineRuntime)
        message(FATAL_ERROR "AstralEngineRuntime already exists; call astral_add_runtime once")
    endif()
    add_library(AstralEngineRuntime STATIC ${ASTRAL_ENGINE_RUNTIME_SOURCES})
    target_include_directories(AstralEngineRuntime PUBLIC "${ASTRAL_ENGINE_ROOT}")
    target_compile_features(AstralEngineRuntime PUBLIC cxx_std_17)
    set_target_properties(AstralEngineRuntime PROPERTIES CXX_EXTENSIONS OFF)
    target_link_libraries(AstralEngineRuntime PUBLIC Threads::Threads)
    add_library(Astral::Runtime ALIAS AstralEngineRuntime)
endfunction()
