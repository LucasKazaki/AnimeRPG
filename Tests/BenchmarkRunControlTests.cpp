#include "Engine/Core/BenchmarkRunControl.h"

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
    // BenchmarkRunControl intentionally reads the wide CRT environment on Windows so
    // Unicode receipt paths remain representable. Keep both CRT environment views in
    // sync explicitly instead of relying on narrow-to-wide synchronization semantics.
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

void ClearBenchmarkEnv() {
    UnsetEnv("ASTRAL_BENCHMARK_MODE");
    UnsetEnv("ASTRAL_BENCHMARK_WARMUP_FRAMES");
    UnsetEnv("ASTRAL_BENCHMARK_MEASURED_FRAMES");
    UnsetEnv("ASTRAL_BENCHMARK_CONTROL_JSON");
}

std::filesystem::path FreshDir(const char* label) {
    auto dir = std::filesystem::temp_directory_path()
        / (std::string("astral-benchmark-run-control-") + label);
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
    std::filesystem::create_directories(dir);
    return std::filesystem::absolute(dir);
}

void TestDefaultIsDisabled() {
    ClearBenchmarkEnv();
    Astral::Core::BenchmarkRunControl control;
    std::string error;
    const auto status = control.ConfigureFromEnvironment(60, 1280, 720, error);
    CHECK(status == Astral::Core::BenchmarkRunControlEnvironmentStatus::NotRequested);
    CHECK(error.empty());
    CHECK(!control.Enabled());
    CHECK(!control.SuppressLiveInput());
    CHECK(control.CompletedFrames() == 0);
}

void TestOrphanControlsFailClosed() {
    ClearBenchmarkEnv();
    SetEnv("ASTRAL_BENCHMARK_WARMUP_FRAMES", "120");
    Astral::Core::BenchmarkRunControl control;
    std::string error;
    const auto status = control.ConfigureFromEnvironment(60, 1280, 720, error);
    CHECK(status == Astral::Core::BenchmarkRunControlEnvironmentStatus::Invalid);
    CHECK(!error.empty());
    CHECK(!control.Enabled());
    ClearBenchmarkEnv();
}

void TestModeRequiresFixedSimulation() {
    ClearBenchmarkEnv();
    const auto dir = FreshDir("requires-fixed");
    SetEnv("ASTRAL_BENCHMARK_MODE", "1");
    SetEnv("ASTRAL_BENCHMARK_WARMUP_FRAMES", "120");
    SetEnv("ASTRAL_BENCHMARK_MEASURED_FRAMES", "3600");
    SetEnv("ASTRAL_BENCHMARK_CONTROL_JSON", (dir / "control.json").string());
    Astral::Core::BenchmarkRunControl control;
    std::string error;
    const auto status = control.ConfigureFromEnvironment(0, 1280, 720, error);
    CHECK(status == Astral::Core::BenchmarkRunControlEnvironmentStatus::Invalid);
    CHECK(!error.empty());
    ClearBenchmarkEnv();
    std::filesystem::remove_all(dir);
}

void TestMalformedEnvironmentRejected() {
    const auto dir = FreshDir("malformed");
    const std::string receipt = (dir / "control.json").string();
    const char* badModes[] = {"", "0", "true", "01"};
    for (const char* mode : badModes) {
        ClearBenchmarkEnv();
        SetEnv("ASTRAL_BENCHMARK_MODE", mode);
        SetEnv("ASTRAL_BENCHMARK_WARMUP_FRAMES", "120");
        SetEnv("ASTRAL_BENCHMARK_MEASURED_FRAMES", "3600");
        SetEnv("ASTRAL_BENCHMARK_CONTROL_JSON", receipt);
        Astral::Core::BenchmarkRunControl control;
        std::string error;
        CHECK(control.ConfigureFromEnvironment(60, 1280, 720, error)
            == Astral::Core::BenchmarkRunControlEnvironmentStatus::Invalid);
    }
    const char* badCounts[] = {"", "-1", "+1", " 1", "1 ", "1x", "1000001"};
    for (const char* value : badCounts) {
        ClearBenchmarkEnv();
        SetEnv("ASTRAL_BENCHMARK_MODE", "1");
        SetEnv("ASTRAL_BENCHMARK_WARMUP_FRAMES", value);
        SetEnv("ASTRAL_BENCHMARK_MEASURED_FRAMES", "1");
        SetEnv("ASTRAL_BENCHMARK_CONTROL_JSON", receipt);
        Astral::Core::BenchmarkRunControl control;
        std::string error;
        CHECK(control.ConfigureFromEnvironment(60, 1280, 720, error)
            == Astral::Core::BenchmarkRunControlEnvironmentStatus::Invalid);
    }
    ClearBenchmarkEnv();
    SetEnv("ASTRAL_BENCHMARK_MODE", "1");
    SetEnv("ASTRAL_BENCHMARK_WARMUP_FRAMES", "0");
    SetEnv("ASTRAL_BENCHMARK_MEASURED_FRAMES", "0");
    SetEnv("ASTRAL_BENCHMARK_CONTROL_JSON", receipt);
    Astral::Core::BenchmarkRunControl zeroMeasured;
    std::string error;
    CHECK(zeroMeasured.ConfigureFromEnvironment(60, 1280, 720, error)
        == Astral::Core::BenchmarkRunControlEnvironmentStatus::Invalid);

    ClearBenchmarkEnv();
    SetEnv("ASTRAL_BENCHMARK_MODE", "1");
    SetEnv("ASTRAL_BENCHMARK_WARMUP_FRAMES", "0");
    SetEnv("ASTRAL_BENCHMARK_MEASURED_FRAMES", "1");
    SetEnv("ASTRAL_BENCHMARK_CONTROL_JSON", receipt);
    Astral::Core::BenchmarkRunControl zeroWidth;
    CHECK(zeroWidth.ConfigureFromEnvironment(60, 0, 720, error)
        == Astral::Core::BenchmarkRunControlEnvironmentStatus::Invalid);
    ClearBenchmarkEnv();
    std::filesystem::remove_all(dir);
}

void TestExactFrameCompletionAndReceipt() {
    const auto dir = FreshDir("exact");
    Astral::Core::BenchmarkRunControl control;
    std::string error;
    const auto receipt = dir / "control.json";
    CHECK(control.Configure({receipt, 60, 2, 3, 1280, 720}, error));
    CHECK(control.Enabled());
    CHECK(control.SuppressLiveInput());
    CHECK(control.SimulationFixedHz() == 60);
    CHECK(control.WarmupFrames() == 2);
    CHECK(control.MeasuredFrames() == 3);
    CHECK(control.ClientWidthPx() == 1280);
    CHECK(control.ClientHeightPx() == 720);
    CHECK(control.TotalFrames() == 5);

    for (int i = 0; i < 4; ++i) {
        CHECK(control.ObserveClientArea(1280, 720));
        CHECK(!control.CompleteFrame());
    }
    CHECK(control.CompletedFrames() == 4);
    CHECK(control.ClientAreaObservations() == 4);
    CHECK(!control.FlushCompletion(error));
    CHECK(!error.empty());
    CHECK(!std::filesystem::exists(receipt));

    error.clear();
    CHECK(control.ObserveClientArea(1280, 720));
    CHECK(control.CompleteFrame());
    CHECK(control.CompletedFrames() == 5);
    CHECK(control.ClientAreaObservations() == 5);
    CHECK(control.FlushCompletion(error));
    CHECK(error.empty());
    CHECK(std::filesystem::exists(receipt));
    CHECK(!std::filesystem::exists(receipt.string() + ".partial"));

    std::string text;
    {
        std::ifstream stream(receipt);
        CHECK(stream.good());
        text.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    }
    CHECK(text.find("\"simulation_fixed_hz\": 60") != std::string::npos);
    CHECK(text.find("\"warmup_frames\": 2") != std::string::npos);
    CHECK(text.find("\"measured_frames\": 3") != std::string::npos);
    CHECK(text.find("\"completed_frames\": 5") != std::string::npos);
    CHECK(text.find("\"client_width_px\": 1280") != std::string::npos);
    CHECK(text.find("\"client_height_px\": 720") != std::string::npos);
    CHECK(text.find("\"client_area_observations\": 5") != std::string::npos);
    CHECK(text.find("\"client_area_stable\": true") != std::string::npos);
    CHECK(text.find("\"window_mode\": \"windowed\"") != std::string::npos);
    CHECK(text.find("\"live_input\": \"suppressed\"") != std::string::npos);
    CHECK(text.find("\"termination\": \"exact_frame_limit\"") != std::string::npos);
    CHECK(text.find("\"comparative_parity_verified\": false") != std::string::npos);

    CHECK(!control.FlushCompletion(error));
    std::filesystem::remove_all(dir);
}

void TestClientAreaObservationSafety() {
    const auto dir = FreshDir("client-area");
    std::string error;

    Astral::Core::BenchmarkRunControl changed;
    CHECK(changed.Configure({dir / "changed.json", 60, 0, 2, 1280, 720}, error));
    CHECK(changed.ObserveClientArea(1280, 720));
    CHECK(!changed.CompleteFrame());
    CHECK(!changed.ObserveClientArea(1279, 720));
    CHECK(!changed.CompleteFrame());
    CHECK(!changed.FlushCompletion(error));
    CHECK(!std::filesystem::exists(dir / "changed.json"));

    Astral::Core::BenchmarkRunControl missing;
    CHECK(missing.Configure({dir / "missing.json", 60, 0, 1, 1280, 720}, error));
    CHECK(!missing.CompleteFrame());
    CHECK(!missing.FlushCompletion(error));
    CHECK(!std::filesystem::exists(dir / "missing.json"));

    Astral::Core::BenchmarkRunControl duplicate;
    CHECK(duplicate.Configure({dir / "duplicate.json", 60, 0, 1, 1280, 720}, error));
    CHECK(duplicate.ObserveClientArea(1280, 720));
    CHECK(!duplicate.ObserveClientArea(1280, 720));
    CHECK(!duplicate.CompleteFrame());
    CHECK(!duplicate.FlushCompletion(error));
    CHECK(!std::filesystem::exists(dir / "duplicate.json"));

    Astral::Core::BenchmarkRunControl invalidDimensions;
    CHECK(!invalidDimensions.Configure({dir / "zero.json", 60, 0, 1, 0, 720}, error));
    CHECK(!invalidDimensions.Configure({dir / "huge.json", 60, 0, 1, 16385, 720}, error));

    std::filesystem::remove_all(dir);
}

void TestOverrunAndOutputSafety() {
    const auto dir = FreshDir("safety");
    std::string error;

    Astral::Core::BenchmarkRunControl overrun;
    const auto receipt = dir / "overrun.json";
    CHECK(overrun.Configure({receipt, 120, 0, 1, 1280, 720}, error));
    CHECK(overrun.ObserveClientArea(1280, 720));
    CHECK(overrun.CompleteFrame());
    CHECK(!overrun.CompleteFrame());
    CHECK(!overrun.FlushCompletion(error));
    CHECK(!std::filesystem::exists(receipt));

    const auto existing = dir / "existing.json";
    {
        std::ofstream stream(existing);
        stream << "keep";
    }
    Astral::Core::BenchmarkRunControl noOverwrite;
    CHECK(!noOverwrite.Configure({existing, 60, 0, 1, 1280, 720}, error));

    const auto partialFinal = dir / "partial.json";
    {
        std::ofstream stream(partialFinal.string() + ".partial");
        stream << "keep";
    }
    Astral::Core::BenchmarkRunControl noPartialOverwrite;
    CHECK(!noPartialOverwrite.Configure({partialFinal, 60, 0, 1, 1280, 720}, error));

    Astral::Core::BenchmarkRunControl relative;
    CHECK(!relative.Configure(
        {std::filesystem::path("relative.json"), 60, 0, 1, 1280, 720}, error));

    Astral::Core::BenchmarkRunControl invalidRate;
    CHECK(!invalidRate.Configure({dir / "bad-rate.json", 1001, 0, 1, 1280, 720}, error));

    Astral::Core::BenchmarkRunControl invalidTotal;
    CHECK(!invalidTotal.Configure({dir / "bad-total.json", 60, 999999, 2, 1280, 720}, error));

    std::filesystem::remove_all(dir);
}

void TestEnvironmentHappyPath() {
    ClearBenchmarkEnv();
    const auto dir = FreshDir("env-happy");
    const auto receipt = dir / "control.json";
    SetEnv("ASTRAL_BENCHMARK_MODE", "1");
    SetEnv("ASTRAL_BENCHMARK_WARMUP_FRAMES", "120");
    SetEnv("ASTRAL_BENCHMARK_MEASURED_FRAMES", "3600");
    SetEnv("ASTRAL_BENCHMARK_CONTROL_JSON", receipt.string());

    Astral::Core::BenchmarkRunControl control;
    std::string error;
    CHECK(control.ConfigureFromEnvironment(60, 1920, 1080, error)
        == Astral::Core::BenchmarkRunControlEnvironmentStatus::Enabled);
    CHECK(control.TotalFrames() == 3720);
    CHECK(control.SimulationFixedHz() == 60);
    CHECK(control.ClientWidthPx() == 1920);
    CHECK(control.ClientHeightPx() == 1080);
    CHECK(control.SuppressLiveInput());

    ClearBenchmarkEnv();
    std::filesystem::remove_all(dir);
}

} // namespace

int main() {
    TestDefaultIsDisabled();
    TestOrphanControlsFailClosed();
    TestModeRequiresFixedSimulation();
    TestMalformedEnvironmentRejected();
    TestExactFrameCompletionAndReceipt();
    TestClientAreaObservationSafety();
    TestOverrunAndOutputSafety();
    TestEnvironmentHappyPath();
    std::cout << "BenchmarkRunControlTests: 8 groups passed\n";
    return 0;
}
