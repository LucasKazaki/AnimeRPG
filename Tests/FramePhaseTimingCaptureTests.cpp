#include "Engine/Core/FramePhaseTimingCapture.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace {

int failures = 0;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

std::filesystem::path MakeTempRoot() {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto root = std::filesystem::temp_directory_path()
        / ("astral-frame-phase-timing-tests-" + std::to_string(stamp));
    std::filesystem::create_directories(root);
    return root;
}

void SetEnvironment(const char* name, const std::string& value) {
#ifdef _WIN32
    _putenv_s(name, value.c_str());
#else
    setenv(name, value.c_str(), 1);
#endif
}

void SetPathEnvironment(const std::filesystem::path& value) {
#ifdef _WIN32
    _wputenv_s(L"ASTRAL_FRAME_PHASE_TIMING_CSV", value.c_str());
#else
    setenv("ASTRAL_FRAME_PHASE_TIMING_CSV", value.u8string().c_str(), 1);
#endif
}

void ClearEnvironment(const char* name) {
#ifdef _WIN32
    _putenv_s(name, "");
#else
    unsetenv(name);
#endif
}

void ClearTimingEnvironment() {
    ClearEnvironment("ASTRAL_FRAME_PHASE_TIMING_CSV");
    ClearEnvironment("ASTRAL_FRAME_PHASE_TIMING_WARMUP_FRAMES");
    ClearEnvironment("ASTRAL_FRAME_PHASE_TIMING_MAX_SAMPLES");
}

std::string ReadAll(const std::filesystem::path& path) {
    std::ifstream stream(path);
    std::ostringstream out;
    out << stream.rdbuf();
    return out.str();
}

void TestConfigureRejectsUnsafePathsAndLimits(const std::filesystem::path& root) {
    Astral::Core::FramePhaseTimingCapture capture;
    std::string error;
    Check(!capture.Configure({}, error), "empty path must be rejected");
    Astral::Core::FramePhaseTimingCaptureConfig config;
    config.outputPath = "relative.csv";
    Check(!capture.Configure(config, error), "relative path must be rejected");
    config.outputPath = root / "zero.csv";
    config.maxSamples = 0;
    Check(!capture.Configure(config, error), "zero maxSamples must be rejected");
    config.maxSamples = Astral::Core::FramePhaseTimingCapture::kHardMaxSamples + 1;
    Check(!capture.Configure(config, error), "maxSamples over hard cap must be rejected");
}

void TestExistingOutputsRejected(const std::filesystem::path& root) {
    Astral::Core::FramePhaseTimingCapture capture;
    Astral::Core::FramePhaseTimingCaptureConfig config;
    config.outputPath = root / "existing.csv";
    { std::ofstream stream(config.outputPath); stream << "existing"; }
    std::string error;
    Check(!capture.Configure(config, error), "existing output must not be overwritten");

    config.outputPath = root / "partial.csv";
    { std::ofstream stream(config.outputPath.string() + ".partial"); stream << "partial"; }
    Check(!capture.Configure(config, error), "existing partial output must be rejected");
}

void TestWarmupAndSchema(const std::filesystem::path& root) {
    Astral::Core::FramePhaseTimingCapture capture;
    Astral::Core::FramePhaseTimingCaptureConfig config;
    config.outputPath = root / "warmup.csv";
    config.warmupFrames = 1;
    config.maxSamples = 3;
    std::string error;
    Check(capture.Configure(config, error), "valid phase capture config should succeed");
    Check(capture.Record(0, 0.0, 0.0, 0.0, 0.0), "warmup row should be discarded before validation");
    Check(capture.Record(1, 0.25, 1.50, 2.25, 1.0), "measured row should record");
    Check(capture.Flush(error), "valid phase capture should flush");
    const auto text = ReadAll(config.outputPath);
    Check(text.find("# metric=main_thread_phase_wall_ms") != std::string::npos,
        "output should retain metric identity");
    Check(text.find("# cpu_scope=Win32_main_thread_only") != std::string::npos,
        "output must limit the CPU scope claim");
    Check(text.find("# gpu_timing=unavailable") != std::string::npos,
        "output must not imply GPU timing");
    Check(text.find("1,0.250000,1.500000,2.250000,1.000000,5.000000") != std::string::npos,
        "output should contain exact phase values and derived total");
}

void TestInvalidMeasuredSamplesBlockPublication(const std::filesystem::path& root) {
    Astral::Core::FramePhaseTimingCapture capture;
    Astral::Core::FramePhaseTimingCaptureConfig config;
    config.outputPath = root / "invalid.csv";
    config.warmupFrames = 0;
    std::string error;
    Check(capture.Configure(config, error), "invalid-sample fixture should configure");
    Check(!capture.Record(0, -0.1, 1.0, 1.0, 1.0), "negative phase must be rejected");
    Check(!capture.Record(1, 1.0, std::numeric_limits<double>::infinity(), 1.0, 1.0),
        "non-finite phase must be rejected");
    Check(capture.InvalidSampleObserved(), "invalid phase flag should latch");
    Check(!capture.Flush(error), "invalid measured samples must block publication");
    Check(!std::filesystem::exists(config.outputPath), "invalid capture must not publish final evidence");
}

void TestAllZeroMeasuredFrameRejected(const std::filesystem::path& root) {
    Astral::Core::FramePhaseTimingCapture capture;
    Astral::Core::FramePhaseTimingCaptureConfig config;
    config.outputPath = root / "zero-total.csv";
    config.warmupFrames = 0;
    std::string error;
    Check(capture.Configure(config, error), "zero-total fixture should configure");
    Check(!capture.Record(0, 0.0, 0.0, 0.0, 0.0), "zero-total measured row must be rejected");
    Check(!capture.Flush(error), "zero-total capture must not publish");
}

void TestSaturationIsExplicit(const std::filesystem::path& root) {
    Astral::Core::FramePhaseTimingCapture capture;
    Astral::Core::FramePhaseTimingCaptureConfig config;
    config.outputPath = root / "saturated.csv";
    config.warmupFrames = 0;
    config.maxSamples = 2;
    std::string error;
    Check(capture.Configure(config, error), "saturation fixture should configure");
    Check(capture.Record(0, 1, 1, 1, 1), "sample 0 should record");
    Check(capture.Record(1, 1, 1, 1, 1), "sample 1 should record");
    Check(capture.Record(2, 1, 1, 1, 1), "sample beyond cap should not grow buffer");
    Check(capture.SampleCount() == 2, "capture must honor sample cap");
    Check(capture.Saturated(), "saturation must be explicit");
    Check(capture.Flush(error), "saturated capture should publish with marker");
    Check(ReadAll(config.outputPath).find("# samples_saturated=1") != std::string::npos,
        "output should retain saturation marker");
}

void TestNoSamplesCannotPublish(const std::filesystem::path& root) {
    Astral::Core::FramePhaseTimingCapture capture;
    Astral::Core::FramePhaseTimingCaptureConfig config;
    config.outputPath = root / "empty.csv";
    config.warmupFrames = 10;
    std::string error;
    Check(capture.Configure(config, error), "empty fixture should configure");
    Check(capture.Record(0, 0, 0, 0, 0), "warmup zero row should be discarded safely");
    Check(!capture.Flush(error), "capture with no measured rows must not publish");
}

void TestOutputRaceIsRejected(const std::filesystem::path& root) {
    Astral::Core::FramePhaseTimingCapture capture;
    Astral::Core::FramePhaseTimingCaptureConfig config;
    config.outputPath = root / "race.csv";
    config.warmupFrames = 0;
    std::string error;
    Check(capture.Configure(config, error), "race fixture should configure");
    Check(capture.Record(0, 1, 1, 1, 1), "race fixture should record");
    { std::ofstream stream(config.outputPath); stream << "competitor"; }
    Check(!capture.Flush(error), "output appearing after configure must block overwrite");
    Check(ReadAll(config.outputPath) == "competitor", "existing output must remain unchanged");
}

void TestEnvironmentConfiguration(const std::filesystem::path& root) {
    ClearTimingEnvironment();
    Astral::Core::FramePhaseTimingCapture capture;
    std::string error;
    Check(capture.ConfigureFromEnvironment(error)
        == Astral::Core::FramePhaseTimingEnvironmentStatus::NotRequested,
        "capture should be disabled when no environment path is present");

    const auto output = root / "env.csv";
    SetPathEnvironment(output);
    SetEnvironment("ASTRAL_FRAME_PHASE_TIMING_WARMUP_FRAMES", "2");
    SetEnvironment("ASTRAL_FRAME_PHASE_TIMING_MAX_SAMPLES", "4");
    Check(capture.ConfigureFromEnvironment(error)
        == Astral::Core::FramePhaseTimingEnvironmentStatus::Enabled,
        "valid environment should enable phase capture");
    Check(capture.Record(0, 0, 0, 0, 0), "warmup 0 should be discarded");
    Check(capture.Record(1, 0, 0, 0, 0), "warmup 1 should be discarded");
    Check(capture.Record(2, 0.1, 0.2, 0.3, 0.4), "post-warmup environment sample should record");
    Check(capture.Flush(error), "environment configured capture should publish");
    Check(ReadAll(output).find("# warmup_frames=2") != std::string::npos,
        "environment warmup should be retained");
    ClearTimingEnvironment();

    SetPathEnvironment(root / "bad-env.csv");
    SetEnvironment("ASTRAL_FRAME_PHASE_TIMING_MAX_SAMPLES", "oops");
    Astral::Core::FramePhaseTimingCapture invalid;
    Check(invalid.ConfigureFromEnvironment(error)
        == Astral::Core::FramePhaseTimingEnvironmentStatus::Invalid,
        "malformed environment sample cap must be rejected");
    Check(!invalid.Enabled(), "invalid environment must not enable capture");
    ClearTimingEnvironment();
}

} // namespace

int main() {
    const auto root = MakeTempRoot();
    TestConfigureRejectsUnsafePathsAndLimits(root);
    TestExistingOutputsRejected(root);
    TestWarmupAndSchema(root);
    TestInvalidMeasuredSamplesBlockPublication(root);
    TestAllZeroMeasuredFrameRejected(root);
    TestSaturationIsExplicit(root);
    TestNoSamplesCannotPublish(root);
    TestOutputRaceIsRejected(root);
    TestEnvironmentConfiguration(root);
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    if (failures != 0) {
        std::cerr << failures << " frame phase timing capture checks failed\n";
        return 1;
    }
    std::cout << "FRAME PHASE TIMING CAPTURE TESTS: PASS (9 groups)\n";
    return 0;
}
