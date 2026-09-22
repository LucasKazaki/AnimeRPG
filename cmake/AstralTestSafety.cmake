# Register native tests with assertions enabled in every configuration.
# Do not change AstralGame's optimization, NDEBUG, or runtime-library settings.
function(astral_add_test target)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "astral_add_test requires an existing target: ${target}")
    endif()

    target_sources("${target}" PRIVATE
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../Tests/AssertionsEnabled.cpp")
    if(MSVC)
        target_compile_options("${target}" PRIVATE /UNDEBUG)
    else()
        target_compile_options("${target}" PRIVATE -UNDEBUG)
    endif()

    add_test(NAME "${target}" COMMAND "${target}" ${ARGN})
    if("${target}" MATCHES "RuntimeSmoke$")
        # SendInput and foreground-window state are shared by the desktop.
        # This protects ONE CTest invocation, not separate concurrent invocations.
        set_tests_properties("${target}" PROPERTIES RUN_SERIAL TRUE TIMEOUT 180)
    else()
        set_tests_properties("${target}" PROPERTIES TIMEOUT 60)
    endif()
endfunction()
