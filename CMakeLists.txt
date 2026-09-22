cmake_minimum_required(VERSION 3.25)
project(AstralEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

    set(ASTRAL_EDITOR_RUNTIME_SUPERVISOR_SOURCE
        "${CMAKE_CURRENT_BINARY_DIR}/EditorRuntimeSmokeSupervisor.cpp")
    file(WRITE "${ASTRAL_EDITOR_RUNTIME_SUPERVISOR_SOURCE}" [=[
#define wmain AstralEditorRuntimeSmokeWorkerMain
#include "Tests/EditorRuntimeSmoke.cpp"
#undef wmain

#include <cwchar>

namespace {
constexpr DWORD kSupervisorWorkerTimeoutMs = 150000;
constexpr DWORD kSupervisorCleanupTimeoutMs = 5000;

std::wstring SupervisorQuoteArgument(const wchar_t* value) {
    std::wstring quoted = L"\"";
    std::size_t backslashes = 0;
    for (const wchar_t* cursor = value; *cursor != L'\0'; ++cursor) {
        if (*cursor == L'\\') {
            ++backslashes;
            continue;
        }
        if (*cursor == L'\"') {
            quoted.append(backslashes * 2 + 1, L'\\');
            quoted.push_back(L'\"');
            backslashes = 0;
            continue;
        }
        quoted.append(backslashes, L'\\');
        backslashes = 0;
        quoted.push_back(*cursor);
    }
    quoted.append(backslashes * 2, L'\\');
    quoted.push_back(L'\"');
    return quoted;
}

std::wstring SupervisorModulePath() {
    std::vector<wchar_t> buffer(1024);
    while (true) {
        const DWORD copied = GetModuleFileNameW(
            nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (copied == 0) return {};
        if (copied < buffer.size() - 1) {
            return std::wstring(buffer.data(), copied);
        }
        if (buffer.size() >= 32768) return {};
        buffer.resize(buffer.size() * 2);
    }
}

void SupervisorPrintWin32Failure(const char* operation, DWORD error) {
    std::cerr << "EDITOR RUNTIME SUPERVISOR: FAIL (" << operation
              << ", Win32 error " << error << ")\n";
}
} // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc == 4 && std::wcscmp(argv[1], L"--worker") == 0) {
        wchar_t* workerArgv[] = {argv[0], argv[2], argv[3]};
        return AstralEditorRuntimeSmokeWorkerMain(3, workerArgv);
    }
    if (argc != 3) {
        std::cerr << "usage: EditorRuntimeSmoke <AstralEditor> <working-directory>\n";
        return 1;
    }

    const std::wstring selfPath = SupervisorModulePath();
    if (selfPath.empty()) {
        SupervisorPrintWin32Failure("GetModuleFileNameW", GetLastError());
        return 1;
    }

    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    if (!job) {
        SupervisorPrintWin32Failure("CreateJobObjectW", GetLastError());
        return 1;
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation,
            &limits, sizeof(limits))) {
        const DWORD error = GetLastError();
        CloseHandle(job);
        SupervisorPrintWin32Failure("SetInformationJobObject", error);
        return 1;
    }

    SIZE_T attributeBytes = 0;
    SetLastError(ERROR_SUCCESS);
    const BOOL probeResult = InitializeProcThreadAttributeList(
        nullptr, 1, 0, &attributeBytes);
    if (probeResult != FALSE || GetLastError() != ERROR_INSUFFICIENT_BUFFER
        || attributeBytes == 0) {
        const DWORD error = GetLastError();
        CloseHandle(job);
        SupervisorPrintWin32Failure("InitializeProcThreadAttributeList probe", error);
        return 1;
    }

    std::vector<unsigned char> attributeStorage(attributeBytes);
    auto* attributes = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(
        attributeStorage.data());
    if (!InitializeProcThreadAttributeList(attributes, 1, 0, &attributeBytes)) {
        const DWORD error = GetLastError();
        CloseHandle(job);
        SupervisorPrintWin32Failure("InitializeProcThreadAttributeList", error);
        return 1;
    }

    HANDLE jobList[] = {job};
    if (!UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_JOB_LIST,
            jobList, sizeof(jobList), nullptr, nullptr)) {
        const DWORD error = GetLastError();
        DeleteProcThreadAttributeList(attributes);
        CloseHandle(job);
        SupervisorPrintWin32Failure("UpdateProcThreadAttribute(PROC_THREAD_ATTRIBUTE_JOB_LIST)", error);
        return 1;
    }

    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.lpAttributeList = attributes;
    PROCESS_INFORMATION worker{};
    std::wstring command = SupervisorQuoteArgument(selfPath.c_str()) + L" --worker "
        + SupervisorQuoteArgument(argv[1]) + L" " + SupervisorQuoteArgument(argv[2]);

    const BOOL created = CreateProcessW(selfPath.c_str(), command.data(), nullptr, nullptr,
        FALSE, EXTENDED_STARTUPINFO_PRESENT, nullptr, argv[2],
        &startup.StartupInfo, &worker);
    const DWORD createError = created ? ERROR_SUCCESS : GetLastError();
    DeleteProcThreadAttributeList(attributes);

    if (!created) {
        CloseHandle(job);
        SupervisorPrintWin32Failure("CreateProcessW(worker)", createError);
        return 1;
    }

    CloseHandle(worker.hThread);
    const DWORD waitState = WaitForSingleObject(
        worker.hProcess, kSupervisorWorkerTimeoutMs);
    if (waitState != WAIT_OBJECT_0) {
        std::cerr << "EDITOR RUNTIME SUPERVISOR: FAIL (worker "
                  << (waitState == WAIT_TIMEOUT ? "timeout" : "wait") << ")\n";
        const BOOL closed = CloseHandle(job);
        job = nullptr;
        if (!closed) {
            std::cerr << "EDITOR RUNTIME SUPERVISOR: FAIL (could not close kill-on-close job)\n";
        }
        const DWORD cleanupState = WaitForSingleObject(
            worker.hProcess, kSupervisorCleanupTimeoutMs);
        if (cleanupState != WAIT_OBJECT_0) {
            std::cerr << "EDITOR RUNTIME SUPERVISOR: FAIL (kill-on-close job did not terminate worker in time)\n";
        }
        CloseHandle(worker.hProcess);
        return 1;
    }

    DWORD workerExitCode = 1;
    if (!GetExitCodeProcess(worker.hProcess, &workerExitCode)
        || workerExitCode == STILL_ACTIVE) {
        const DWORD error = GetLastError();
        CloseHandle(job);
        CloseHandle(worker.hProcess);
        SupervisorPrintWin32Failure("GetExitCodeProcess(worker)", error);
        return 1;
    }

    CloseHandle(worker.hProcess);
    CloseHandle(job);
    if (workerExitCode != 0) {
        std::cerr << "EDITOR RUNTIME SUPERVISOR: FAIL (worker exit "
                  << workerExitCode << ")\n";
        return 1;
    }

    std::cout << "EDITOR RUNTIME SUPERVISOR: PASS (worker completed inside kill-on-close job)\n";
    return 0;
}
]=])

    add_executable(EditorRuntimeSmoke "${ASTRAL_EDITOR_RUNTIME_SUPERVISOR_SOURCE}")
    target_include_directories(EditorRuntimeSmoke PRIVATE .)
    target_compile_definitions(EditorRuntimeSmoke PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(EditorRuntimeSmoke PRIVATE user32 kernel32)
    astral_add_test(EditorRuntimeSmoke
        $<TARGET_FILE:AstralEditor> ${CMAKE_SOURCE_DIR})

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
