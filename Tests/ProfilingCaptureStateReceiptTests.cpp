#include "Engine/Core/ProfilingCaptureStateReceipt.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

void Require(bool condition, const char* expression, int line) {
    if (!condition) {
        std::cerr << "CHECK failed at line " << line << ": " << expression << "\n";
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expression) Require(static_cast<bool>(expression), #expression, __LINE__)

void SetEnv(const char* name, const std::string& value) {
#ifdef _WIN32
    CHECK(_putenv_s(name, value.c_str()) == 0);
    const std::wstring wideName = std::filesystem::path(name).wstring();
    const std::wstring wideValue = std::filesystem::path(value).wstring();
    CHECK(_wputenv_s(wideName.c_str(), wideValue.c_str()) == 0);
#else
    CHECK(setenv(name, value.c_str(), 1) == 0);
#endif
}

void UnsetEnv(const char* name) {
#ifdef _WIN32
    CHECK(_putenv_s(name, "") == 0);
    const std::wstring wideName = std::filesystem::path(name).wstring();
    CHECK(_wputenv_s(wideName.c_str(), L"") == 0);
#else
    CHECK(unsetenv(name) == 0);
#endif
}

std::filesystem::path FreshDir(const char* label) {
    auto dir = std::filesystem::temp_directory_path()
        / (std::string("astral-profiling-capture-state-") + label);
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
    std::filesystem::create_directories(dir);
    return std::filesystem::absolute(dir);
}

std::string ReadText(const std::filesystem::path& path) {
    std::ifstream stream(path);
    CHECK(stream.good());
    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

Astral::Core::BenchmarkRunControl ConfiguredControl(
    const std::filesystem::path& dir,
    const char* name = "run-control.json") {
    Astral::Core::BenchmarkRunControl control;
    std::string error;
    CHECK(control.Configure({dir / name, 60, 120, 3600, 1920, 1080}, error));
    return control;
}

Astral::Core::FrameTimingCaptureConfig FrameConfig(const std::filesystem::path& dir) {
    return {dir / "frame.csv", 120, 3600};
}

Astral::Core::FramePhaseTimingCaptureConfig PhaseConfig(const std::filesystem::path& dir) {
    return {dir / "phase.csv", 120, 3600};
}

Astral::Core::ProcessMemoryCaptureConfig MemoryConfig(const std::filesystem::path& dir) {
    return {dir / "memory.csv", 120, 10, 360,
        Astral::Core::ProcessMemorySampleSource::WindowsProcessCounters};
}

void TestReceiptNotRequested() {
    UnsetEnv("ASTRAL_PROFILING_CAPTURE_STATE_JSON");
    const auto dir = FreshDir("not-requested");
    auto control = ConfiguredControl(dir);
    std::string error;
    CHECK(Astral::Core::ProfilingCaptureStateReceipt::WriteFromEnvironment(
        control,
        Astral::Core::FrameTimingEnvironmentStatus::NotRequested, {},
        Astral::Core::FramePhaseTimingEnvironmentStatus::NotRequested, {},
        Astral::Core::ProcessMemoryEnvironmentStatus::NotRequested, {}, error)
        == Astral::Core::ProfilingCaptureStateReceiptStatus::NotRequested);
    CHECK(error.empty());
    std::filesystem::remove_all(dir);
}

void TestOrphanReceiptRejected() {
    const auto dir = FreshDir("orphan");
    SetEnv("ASTRAL_PROFILING_CAPTURE_STATE_JSON", (dir / "capture-state.json").string());
    Astral::Core::BenchmarkRunControl disabled;
    std::string error;
    CHECK(Astral::Core::ProfilingCaptureStateReceipt::WriteFromEnvironment(
        disabled,
        Astral::Core::FrameTimingEnvironmentStatus::NotRequested, {},
        Astral::Core::FramePhaseTimingEnvironmentStatus::NotRequested, {},
        Astral::Core::ProcessMemoryEnvironmentStatus::NotRequested, {}, error)
        == Astral::Core::ProfilingCaptureStateReceiptStatus::Invalid);
    CHECK(!error.empty());
    CHECK(!std::filesystem::exists(dir / "capture-state.json"));
    UnsetEnv("ASTRAL_PROFILING_CAPTURE_STATE_JSON");
    std::filesystem::remove_all(dir);
}

void TestAllEnabledReceiptAndPrivacyBoundary() {
    const auto dir = FreshDir("enabled");
    auto control = ConfiguredControl(dir);
    const auto receipt = dir / "capture-state.json";
    SetEnv("ASTRAL_PROFILING_CAPTURE_STATE_JSON", receipt.string());
    SetEnv("ASTRAL_UNRELATED_SECRET_SENTINEL", "SECRET_DO_NOT_SERIALIZE");
    std::string error;
    CHECK(Astral::Core::ProfilingCaptureStateReceipt::WriteFromEnvironment(
        control,
        Astral::Core::FrameTimingEnvironmentStatus::Enabled, FrameConfig(dir),
        Astral::Core::FramePhaseTimingEnvironmentStatus::Enabled, PhaseConfig(dir),
        Astral::Core::ProcessMemoryEnvironmentStatus::Enabled, MemoryConfig(dir), error)
        == Astral::Core::ProfilingCaptureStateReceiptStatus::Written);
    CHECK(error.empty());
    const auto text = ReadText(receipt);
    CHECK(text.find("\"schema_version\": 1") != std::string::npos);
    CHECK(text.find("\"capture_state_semantics\": \"post_configuration_pre_frame_loop\"") != std::string::npos);
    CHECK(text.find("\"raw_environment_dumped\": false") != std::string::npos);
    CHECK(text.find("\"output_path\": \"frame.csv\"") != std::string::npos);
    CHECK(text.find("\"output_path\": \"phase.csv\"") != std::string::npos);
    CHECK(text.find("\"output_path\": \"memory.csv\"") != std::string::npos);
    CHECK(text.find("\"sample_every_frames\": 10") != std::string::npos);
    CHECK(text.find("\"sample_source\": \"windows_process_counters\"") != std::string::npos);
    CHECK(text.find("SECRET_DO_NOT_SERIALIZE") == std::string::npos);
    CHECK(text.find("ASTRAL_UNRELATED_SECRET_SENTINEL") == std::string::npos);
    UnsetEnv("ASTRAL_UNRELATED_SECRET_SENTINEL");
    UnsetEnv("ASTRAL_PROFILING_CAPTURE_STATE_JSON");
    std::filesystem::remove_all(dir);
}

void TestAllDisabledAndMixedStates() {
    {
        const auto dir = FreshDir("disabled");
        auto control = ConfiguredControl(dir);
        const auto receipt = dir / "capture-state.json";
        SetEnv("ASTRAL_PROFILING_CAPTURE_STATE_JSON", receipt.string());
        std::string error;
        CHECK(Astral::Core::ProfilingCaptureStateReceipt::WriteFromEnvironment(
            control,
            Astral::Core::FrameTimingEnvironmentStatus::NotRequested, {},
            Astral::Core::FramePhaseTimingEnvironmentStatus::NotRequested, {},
            Astral::Core::ProcessMemoryEnvironmentStatus::NotRequested, {}, error)
            == Astral::Core::ProfilingCaptureStateReceiptStatus::Written);
        const auto text = ReadText(receipt);
        std::size_t position = 0;
        int disabledCount = 0;
        while ((position = text.find("\"state\": \"not_requested\"", position))
            != std::string::npos) {
            ++disabledCount;
            ++position;
        }
        CHECK(disabledCount == 3);
        CHECK(text.find("\"output_path\": null") != std::string::npos);
        std::filesystem::remove_all(dir);
    }
    {
        const auto dir = FreshDir("mixed");
        auto control = ConfiguredControl(dir);
        const auto receipt = dir / "capture-state.json";
        SetEnv("ASTRAL_PROFILING_CAPTURE_STATE_JSON", receipt.string());
        std::string error;
        CHECK(Astral::Core::ProfilingCaptureStateReceipt::WriteFromEnvironment(
            control,
            Astral::Core::FrameTimingEnvironmentStatus::Enabled, FrameConfig(dir),
            Astral::Core::FramePhaseTimingEnvironmentStatus::NotRequested, {},
            Astral::Core::ProcessMemoryEnvironmentStatus::NotRequested, {}, error)
            == Astral::Core::ProfilingCaptureStateReceiptStatus::Written);
        const auto text = ReadText(receipt);
        CHECK(text.find("\"state\": \"enabled\"") != std::string::npos);
        CHECK(text.find("\"state\": \"not_requested\"") != std::string::npos);
        std::filesystem::remove_all(dir);
    }
    UnsetEnv("ASTRAL_PROFILING_CAPTURE_STATE_JSON");
}

void TestInvalidCaptureStatusAndIncoherentCapacityRejected() {
    const auto dir = FreshDir("invalid");
    auto control = ConfiguredControl(dir);
    SetEnv("ASTRAL_PROFILING_CAPTURE_STATE_JSON", (dir / "capture-state.json").string());
    std::string error;
    CHECK(Astral::Core::ProfilingCaptureStateReceipt::WriteFromEnvironment(
        control,
        Astral::Core::FrameTimingEnvironmentStatus::Invalid, FrameConfig(dir),
        Astral::Core::FramePhaseTimingEnvironmentStatus::NotRequested, {},
        Astral::Core::ProcessMemoryEnvironmentStatus::NotRequested, {}, error)
        == Astral::Core::ProfilingCaptureStateReceiptStatus::Invalid);
    CHECK(!std::filesystem::exists(dir / "capture-state.json"));

    error.clear();
    auto tooSmall = FrameConfig(dir);
    tooSmall.maxSamples = 3599;
    CHECK(Astral::Core::ProfilingCaptureStateReceipt::WriteFromEnvironment(
        control,
        Astral::Core::FrameTimingEnvironmentStatus::Enabled, tooSmall,
        Astral::Core::FramePhaseTimingEnvironmentStatus::NotRequested, {},
        Astral::Core::ProcessMemoryEnvironmentStatus::NotRequested, {}, error)
        == Astral::Core::ProfilingCaptureStateReceiptStatus::Invalid);
    CHECK(!std::filesystem::exists(dir / "capture-state.json"));
    UnsetEnv("ASTRAL_PROFILING_CAPTURE_STATE_JSON");
    std::filesystem::remove_all(dir);
}

void TestEscapingAndConflictingOutputsRejected() {
    const auto dir = FreshDir("paths");
    const auto outside = FreshDir("outside");
    auto control = ConfiguredControl(dir);
    const auto receipt = dir / "capture-state.json";
    SetEnv("ASTRAL_PROFILING_CAPTURE_STATE_JSON", receipt.string());
    std::string error;
    auto escaping = FrameConfig(outside);
    CHECK(Astral::Core::ProfilingCaptureStateReceipt::WriteFromEnvironment(
        control,
        Astral::Core::FrameTimingEnvironmentStatus::Enabled, escaping,
        Astral::Core::FramePhaseTimingEnvironmentStatus::NotRequested, {},
        Astral::Core::ProcessMemoryEnvironmentStatus::NotRequested, {}, error)
        == Astral::Core::ProfilingCaptureStateReceiptStatus::Invalid);

    error.clear();
    auto frame = FrameConfig(dir);
    auto phase = PhaseConfig(dir);
    phase.outputPath = frame.outputPath;
    CHECK(Astral::Core::ProfilingCaptureStateReceipt::WriteFromEnvironment(
        control,
        Astral::Core::FrameTimingEnvironmentStatus::Enabled, frame,
        Astral::Core::FramePhaseTimingEnvironmentStatus::Enabled, phase,
        Astral::Core::ProcessMemoryEnvironmentStatus::NotRequested, {}, error)
        == Astral::Core::ProfilingCaptureStateReceiptStatus::Invalid);
    UnsetEnv("ASTRAL_PROFILING_CAPTURE_STATE_JSON");
    std::filesystem::remove_all(dir);
    std::filesystem::remove_all(outside);
}

void TestNoOverwrite() {
    const auto dir = FreshDir("overwrite");
    auto control = ConfiguredControl(dir);
    const auto receipt = dir / "capture-state.json";
    {
        std::ofstream stream(receipt);
        stream << "keep";
    }
    SetEnv("ASTRAL_PROFILING_CAPTURE_STATE_JSON", receipt.string());
    std::string error;
    CHECK(Astral::Core::ProfilingCaptureStateReceipt::WriteFromEnvironment(
        control,
        Astral::Core::FrameTimingEnvironmentStatus::NotRequested, {},
        Astral::Core::FramePhaseTimingEnvironmentStatus::NotRequested, {},
        Astral::Core::ProcessMemoryEnvironmentStatus::NotRequested, {}, error)
        == Astral::Core::ProfilingCaptureStateReceiptStatus::Invalid);
    CHECK(ReadText(receipt) == "keep");
    UnsetEnv("ASTRAL_PROFILING_CAPTURE_STATE_JSON");
    std::filesystem::remove_all(dir);
}

} // namespace

int main() {
    TestReceiptNotRequested();
    TestOrphanReceiptRejected();
    TestAllEnabledReceiptAndPrivacyBoundary();
    TestAllDisabledAndMixedStates();
    TestInvalidCaptureStatusAndIncoherentCapacityRejected();
    TestEscapingAndConflictingOutputsRejected();
    TestNoOverwrite();
    std::cout << "Profiling capture-state receipt tests passed (7 groups).\n";
    return 0;
}
