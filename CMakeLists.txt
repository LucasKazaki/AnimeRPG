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
constexpr DWORD kSupervisorSelfTestAliveMs = 100;

struct SupervisorContainedProcess {
    HANDLE job{};
    HANDLE process{};
};

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
        if (copied < buffer.size() - 1) return std::wstring(buffer.data(), copied);
        if (buffer.size() >= 32768) return {};
        buffer.resize(buffer.size() * 2);
    }
}

std::wstring SupervisorWin32Failure(const wchar_t* operation, DWORD error) {
    return std::wstring(operation) + L" failed with Win32 error " + std::to_wstring(error);
}

void SupervisorCloseProcessHandles(SupervisorContainedProcess& child) {
    if (child.process) {
        CloseHandle(child.process);
        child.process = nullptr;
    }
    if (child.job) {
        CloseHandle(child.job);
        child.job = nullptr;
    }
}

bool SupervisorLaunchContained(const std::wstring& application,
    std::wstring command, const wchar_t* workingDirectory,
    SupervisorContainedProcess& child, std::wstring& failure) {
    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    if (!job) {
        failure = SupervisorWin32Failure(L"CreateJobObjectW", GetLastError());
        return false;
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation,
            &limits, sizeof(limits))) {
        failure = SupervisorWin32Failure(L"SetInformationJobObject", GetLastError());
        CloseHandle(job);
        return false;
    }

    SIZE_T attributeBytes = 0;
    SetLastError(ERROR_SUCCESS);
    const BOOL probeResult = InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeBytes);
    if (probeResult != FALSE || GetLastError() != ERROR_INSUFFICIENT_BUFFER
        || attributeBytes == 0) {
        failure = SupervisorWin32Failure(
            L"InitializeProcThreadAttributeList probe", GetLastError());
        CloseHandle(job);
        return false;
    }

    std::vector<unsigned char> attributeStorage(attributeBytes);
    auto* attributes = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributeStorage.data());
    if (!InitializeProcThreadAttributeList(attributes, 1, 0, &attributeBytes)) {
        failure = SupervisorWin32Failure(
            L"InitializeProcThreadAttributeList", GetLastError());
        CloseHandle(job);
        return false;
    }

    HANDLE jobList[] = {job};
    if (!UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_JOB_LIST,
            jobList, sizeof(jobList), nullptr, nullptr)) {
        failure = SupervisorWin32Failure(
            L"UpdateProcThreadAttribute(PROC_THREAD_ATTRIBUTE_JOB_LIST)", GetLastError());
        DeleteProcThreadAttributeList(attributes);
        CloseHandle(job);
        return false;
    }

    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.lpAttributeList = attributes;
    PROCESS_INFORMATION process{};
    const BOOL created = CreateProcessW(application.c_str(), command.data(), nullptr, nullptr,
        FALSE, EXTENDED_STARTUPINFO_PRESENT, nullptr, workingDirectory,
        &startup.StartupInfo, &process);
    const DWORD createError = created ? ERROR_SUCCESS : GetLastError();
    DeleteProcThreadAttributeList(attributes);

    if (!created) {
        failure = SupervisorWin32Failure(L"CreateProcessW(contained child)", createError);
        CloseHandle(job);
        return false;
    }

    CloseHandle(process.hThread);
    child.job = job;
    child.process = process.hProcess;
    return true;
}

bool SupervisorCloseJobAndVerifyTermination(SupervisorContainedProcess& child,
    DWORD timeoutMs, DWORD& exitCode, std::wstring& failure) {
    if (!child.job || !child.process) {
        failure = L"contained-process handles were not both available";
        return false;
    }
    if (!CloseHandle(child.job)) {
        failure = SupervisorWin32Failure(L"CloseHandle(kill-on-close job)", GetLastError());
        return false;
    }
    child.job = nullptr;
    const DWORD waitState = WaitForSingleObject(child.process, timeoutMs);
    if (waitState != WAIT_OBJECT_0) {
        failure = waitState == WAIT_TIMEOUT
            ? L"kill-on-close job did not terminate contained child before deadline"
            : SupervisorWin32Failure(L"WaitForSingleObject(contained child)", GetLastError());
        return false;
    }
    if (!GetExitCodeProcess(child.process, &exitCode) || exitCode == STILL_ACTIVE) {
        failure = SupervisorWin32Failure(L"GetExitCodeProcess(contained child)", GetLastError());
        return false;
    }
    return true;
}

int SupervisorRunContainmentSelfTest(const std::wstring& selfPath) {
    std::wstring failure;
    SupervisorContainedProcess child{};
    std::wstring command = SupervisorQuoteArgument(selfPath.c_str())
        + L" --contained-self-test-child";
    if (!SupervisorLaunchContained(selfPath, command, nullptr, child, failure)) {
        std::wcerr << L"EDITOR RUNTIME SUPERVISOR CONTAINMENT TEST: FAIL\nReason: "
                   << failure << L"\n";
        return 1;
    }

    const DWORD initialWait = WaitForSingleObject(child.process, kSupervisorSelfTestAliveMs);
    if (initialWait != WAIT_TIMEOUT) {
        failure = initialWait == WAIT_OBJECT_0
            ? L"contained self-test child exited before kill-on-close was exercised"
            : SupervisorWin32Failure(L"WaitForSingleObject(self-test liveness)", GetLastError());
        DWORD ignoredExit = 0;
        std::wstring cleanupFailure;
        if (!SupervisorCloseJobAndVerifyTermination(
                child, kSupervisorCleanupTimeoutMs, ignoredExit, cleanupFailure)
            && !cleanupFailure.empty()) {
            failure += L"; " + cleanupFailure;
        }
        SupervisorCloseProcessHandles(child);
        std::wcerr << L"EDITOR RUNTIME SUPERVISOR CONTAINMENT TEST: FAIL\nReason: "
                   << failure << L"\n";
        return 1;
    }

    DWORD exitCode = STILL_ACTIVE;
    const bool terminated = SupervisorCloseJobAndVerifyTermination(
        child, kSupervisorCleanupTimeoutMs, exitCode, failure);
    SupervisorCloseProcessHandles(child);
    if (!terminated) {
        std::wcerr << L"EDITOR RUNTIME SUPERVISOR CONTAINMENT TEST: FAIL\nReason: "
                   << failure << L"\n";
        return 1;
    }

    std::wcout << L"EDITOR RUNTIME SUPERVISOR CONTAINMENT TEST: PASS\n";
    return 0;
}
} // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc == 2 && std::wcscmp(argv[1], L"--contained-self-test-child") == 0) {
        Sleep(30000);
        return 99;
    }

    const std::wstring selfPath = SupervisorModulePath();
    if (selfPath.empty()) {
        std::wcerr << L"EDITOR RUNTIME SUPERVISOR: FAIL (module path)\n";
        return 1;
    }

    if (argc == 2 && std::wcscmp(argv[1], L"--self-test") == 0) {
        return SupervisorRunContainmentSelfTest(selfPath);
    }
    if (argc == 4 && std::wcscmp(argv[1], L"--worker") == 0) {
        wchar_t* workerArgv[] = {argv[0], argv[2], argv[3]};
        return AstralEditorRuntimeSmokeWorkerMain(3, workerArgv);
    }
    if (argc != 3) {
        std::cerr << "usage: EditorRuntimeSmoke <AstralEditor> <working-directory>\n";
        return 1;
    }

    std::wstring failure;
    SupervisorContainedProcess worker{};
    std::wstring command = SupervisorQuoteArgument(selfPath.c_str()) + L" --worker "
        + SupervisorQuoteArgument(argv[1]) + L" " + SupervisorQuoteArgument(argv[2]);
    if (!SupervisorLaunchContained(selfPath, command, argv[2], worker, failure)) {
        std::wcerr << L"EDITOR RUNTIME SUPERVISOR: FAIL\nReason: " << failure << L"\n";
        return 1;
    }

    const DWORD waitState = WaitForSingleObject(worker.process, kSupervisorWorkerTimeoutMs);
    if (waitState != WAIT_OBJECT_0) {
        failure = waitState == WAIT_TIMEOUT
            ? L"contained worker exceeded the 150-second supervisor deadline"
            : SupervisorWin32Failure(L"WaitForSingleObject(worker)", GetLastError());
        DWORD cleanupExit = STILL_ACTIVE;
        std::wstring cleanupFailure;
        if (!SupervisorCloseJobAndVerifyTermination(
                worker, kSupervisorCleanupTimeoutMs, cleanupExit, cleanupFailure)) {
            failure += L"; " + cleanupFailure;
        }
        SupervisorCloseProcessHandles(worker);
        std::wcerr << L"EDITOR RUNTIME SUPERVISOR: FAIL\nReason: " << failure << L"\n";
        return 1;
    }

    DWORD workerExitCode = 1;
    if (!GetExitCodeProcess(worker.process, &workerExitCode)
        || workerExitCode == STILL_ACTIVE) {
        failure = SupervisorWin32Failure(L"GetExitCodeProcess(worker)", GetLastError());
        SupervisorCloseProcessHandles(worker);
        std::wcerr << L"EDITOR RUNTIME SUPERVISOR: FAIL\nReason: " << failure << L"\n";
        return 1;
    }

    SupervisorCloseProcessHandles(worker);
    if (workerExitCode != 0) {
        std::wcerr << L"EDITOR RUNTIME SUPERVISOR: FAIL (worker exit "
                   << workerExitCode << L")\n";
        return 1;
    }

    std::wcout << L"EDITOR RUNTIME SUPERVISOR: PASS (worker completed inside kill-on-close job)\n";
    return 0;
}
]=])

    add_executable(EditorRuntimeSmoke "${ASTRAL_EDITOR_RUNTIME_SUPERVISOR_SOURCE}")
    target_include_directories(EditorRuntimeSmoke PRIVATE .)
    target_compile_definitions(EditorRuntimeSmoke PRIVATE
        UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
    target_link_libraries(EditorRuntimeSmoke PRIVATE user32 kernel32)
    astral_add_test(EditorRuntimeSmoke
        $<TARGET_FILE:AstralEditor> ${CMAKE_SOURCE_DIR})

    add_executable(EditorRuntimeSmokeContainmentTests "${ASTRAL_EDITOR_RUNTIME_SUPERVISOR_SOURCE}")
    target_include_directories(EditorRuntimeSmokeContainmentTests PRIVATE .)
    target_compile_definitions(EditorRuntimeSmokeContainmentTests PRIVATE
        UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
    target_link_libraries(EditorRuntimeSmokeContainmentTests PRIVATE user32 kernel32)
    astral_add_test(EditorRuntimeSmokeContainmentTests --self-test)

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
