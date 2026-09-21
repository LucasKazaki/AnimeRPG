#include "Engine/Core/Clock.h"
#include "Engine/Core/FrameTimingCapture.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <thread>

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
        / ("astral-frame-timing-tests-" + std::to_string(stamp));
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
    _wputenv_s(L"ASTRAL_FRAME_TIMING_CSV", value.c_str());
#else
    setenv("ASTRAL_FRAME_TIMING_CSV", value.u8string().c_str(), 1);
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
    ClearEnvironment("ASTRAL_FRAME_TIMING_CSV");
    ClearEnvironment("ASTRAL_FRAME_TIMING_WARMUP_FRAMES");
    ClearEnvironment("ASTRAL_FRAME_TIMING_MAX_SAMPLES");
}

std::string ReadAll(const std::filesystem::path& path) {
    std::ifstream stream(path);
    std::ostringstream out;
    out << stream.rdbuf();
    return out.str();
}

void TestConfigureRejectsEmptyAndRelativePaths(const std::filesystem::path& root) {
    Astral::Core::FrameTimingCapture capture;
    std::string error;
    Check(!capture.Configure({}, error), "empty path must be rejected");
    Check(error.find("empty") != std::string::npos, "empty path error should be explicit");

    Astral::Core::FrameTimingCaptureConfig config;
    config.outputPath = "relative.csv";
    Check(!capture.Configure(config, error), "relative path must be rejected");
    Check(error.find("absolute") != std::string::npos, "relative path error should mention absolute");

    config.outputPath = root / "missing" / "timing.csv";
    Check(!capture.Configure(config, error), "missing parent must be rejected");
}

void TestConfigureRejectsUnsafeSampleLimits(const std::filesystem::path& root) {
    Astral::Core::FrameTimingCapture capture;
    Astral::Core::FrameTimingCaptureConfig config;
    config.outputPath = root / "limit.csv";
    config.maxSamples = 0;
    std::string error;
    Check(!capture.Configure(config, error), "zero maxSamples must be rejected");
    config.maxSamples = Astral::Core::FrameTimingCapture::kHardMaxSamples + 1;
    Check(!capture.Configure(config, error), "maxSamples over hard cap must be rejected");
}

void TestConfigureRejectsExistingOutputs(const std::filesystem::path& root) {
    const auto output = root / "existing.csv";
    { std::ofstream stream(output); stream << "existing"; }
    Astral::Core::FrameTimingCapture capture;
    Astral::Core::FrameTimingCaptureConfig config;
    config.outputPath = output;
    std::string error;
    Check(!capture.Configure(config, error), "existing output must not be overwritten");

    const auto partialOutput = root / "partial.csv";
    { std::ofstream stream(partialOutput.string() + ".partial"); stream << "existing"; }
    config.outputPath = partialOutput;
    Check(!capture.Configure(config, error), "existing partial output must not be overwritten");
}

void TestWarmupCaptureAndFlush(const std::filesystem::path& root) {
    const auto output = root / "warmup.csv";
    Astral::Core::FrameTimingCapture capture;
    Astral::Core::FrameTimingCaptureConfig config;
    config.outputPath = output;
    config.warmupFrames = 2;
    config.maxSamples = 4;
    std::string error;
    Check(capture.Configure(config, error), "valid capture config should succeed");
    Check(capture.Enabled(), "capture should report enabled");
    Check(capture.Record(0, 1.25), "warmup frame zero should be accepted but not stored");
    Check(capture.Record(1, 2.50), "warmup frame one should be accepted but not stored");
    Check(capture.Record(2, 16.0), "first measured frame should be stored");
    Check(capture.Record(3, 17.25), "second measured frame should be stored");
    Check(capture.SampleCount() == 2, "only post-warmup frames should be stored");
    Check(capture.Flush(error), "valid capture should flush");
    Check(std::filesystem::exists(output), "flush should create final output");
    Check(!std::filesystem::exists(output.string() + ".partial"), "successful flush should leave no partial");
    const auto text = ReadAll(output);
    Check(text.find("# warmup_frames=2") != std::string::npos, "output should retain warmup metadata");
    Check(text.find("2,16.000000") != std::string::npos, "output should retain first sample");
    Check(text.find("3,17.250000") != std::string::npos, "output should retain second sample");
    Check(!capture.Flush(error), "second flush must be rejected");
}

void TestDiscardedWarmupDoesNotPoisonEvidence(const std::filesystem::path& root) {
    const auto output = root / "warmup-invalid.csv";
    Astral::Core::FrameTimingCapture capture;
    Astral::Core::FrameTimingCaptureConfig config;
    config.outputPath = output;
    config.warmupFrames = 1;
    config.maxSamples = 2;
    std::string error;
    Check(capture.Configure(config, error), "warmup-invalid fixture should configure");
    Check(capture.Record(0, 0.0), "discarded warmup interval should not become measured evidence");
    Check(!capture.InvalidSampleObserved(), "discarded warmup interval must not poison measured evidence");
    Check(capture.Record(1, 15.0), "post-warmup valid interval should record");
    Check(capture.Flush(error), "valid post-warmup evidence should publish");
    const auto text = ReadAll(output);
    Check(text.find("1,15.000000") != std::string::npos, "published evidence should contain measured interval");
    Check(text.find("# gpu_timing=unavailable") != std::string::npos,
        "capture must label the absence of GPU timing");
    Check(text.find("# acceptance_claim=none") != std::string::npos,
        "capture must not imply benchmark acceptance");
}

void TestInvalidSamplesBlockEvidence(const std::filesystem::path& root) {
    const auto output = root / "invalid.csv";
    Astral::Core::FrameTimingCapture capture;
    Astral::Core::FrameTimingCaptureConfig config;
    config.outputPath = output;
    config.warmupFrames = 0;
    config.maxSamples = 4;
    std::string error;
    Check(capture.Configure(config, error), "invalid-sample fixture should configure");
    Check(!capture.Record(0, 0.0), "zero interval must be rejected");
    Check(!capture.Record(1, std::numeric_limits<double>::infinity()), "infinite interval must be rejected");
    Check(capture.InvalidSampleObserved(), "invalid sample flag should latch");
    Check(!capture.Flush(error), "invalid sample must block evidence finalization");
    Check(!std::filesystem::exists(output), "invalid capture must not publish final evidence");
}

void TestSaturationIsExplicit(const std::filesystem::path& root) {
    const auto output = root / "saturated.csv";
    Astral::Core::FrameTimingCapture capture;
    Astral::Core::FrameTimingCaptureConfig config;
    config.outputPath = output;
    config.warmupFrames = 0;
    config.maxSamples = 2;
    std::string error;
    Check(capture.Configure(config, error), "saturation fixture should configure");
    Check(capture.Record(0, 10.0), "sample 0 should record");
    Check(capture.Record(1, 11.0), "sample 1 should record");
    Check(capture.Record(2, 12.0), "sample beyond cap should be handled without growing buffer");
    Check(capture.SampleCount() == 2, "capture must honor configured maxSamples");
    Check(capture.Saturated(), "saturation must be explicit");
    Check(capture.Flush(error), "saturated evidence should flush with explicit marker");
    Check(ReadAll(output).find("# samples_saturated=1") != std::string::npos,
        "saturated output must retain saturation marker");
}

void TestNoSamplesCannotPublish(const std::filesystem::path& root) {
    const auto output = root / "empty.csv";
    Astral::Core::FrameTimingCapture capture;
    Astral::Core::FrameTimingCaptureConfig config;
    config.outputPath = output;
    config.warmupFrames = 100;
    config.maxSamples = 2;
    std::string error;
    Check(capture.Configure(config, error), "empty fixture should configure");
    Check(capture.Record(0, 1.0), "warmup-only sample should be accepted");
    Check(!capture.Flush(error), "capture with no post-warmup samples must not publish");
    Check(!std::filesystem::exists(output), "empty capture must not create final evidence");
}

void TestOutputRaceIsRejected(const std::filesystem::path& root) {
    const auto output = root / "race.csv";
    Astral::Core::FrameTimingCapture capture;
    Astral::Core::FrameTimingCaptureConfig config;
    config.outputPath = output;
    config.warmupFrames = 0;
    config.maxSamples = 2;
    std::string error;
    Check(capture.Configure(config, error), "race fixture should configure");
    Check(capture.Record(0, 14.0), "race fixture should record");
    { std::ofstream stream(output); stream << "competitor"; }
    Check(!capture.Flush(error), "output appearing after configure must block overwrite");
    Check(ReadAll(output) == "competitor", "existing output must remain unchanged");
}


void TestClockEnvironmentIntegration(const std::filesystem::path& root) {
    const auto output = root / "clock.csv";
    ClearTimingEnvironment();
    SetPathEnvironment(output);
    SetEnvironment("ASTRAL_FRAME_TIMING_WARMUP_FRAMES", "1");
    SetEnvironment("ASTRAL_FRAME_TIMING_MAX_SAMPLES", "2");
    {
        Astral::Core::Clock clock;
        clock.Tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        clock.Tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        clock.Tick();
    }
    ClearTimingEnvironment();
    Check(std::filesystem::exists(output), "Clock destruction should publish requested frame timing evidence");
    const auto text = ReadAll(output);
    Check(text.find("# warmup_frames=1") != std::string::npos, "Clock capture should retain environment warmup");
    Check(text.find("# max_samples=2") != std::string::npos, "Clock capture should retain environment sample cap");
    Check(text.find("1,") != std::string::npos && text.find("2,") != std::string::npos,
        "Clock capture should record the two post-warmup frame intervals");
}

void TestEnvironmentRejectsMalformedLimit(const std::filesystem::path& root) {
    const auto output = root / "bad-env.csv";
    ClearTimingEnvironment();
    SetPathEnvironment(output);
    SetEnvironment("ASTRAL_FRAME_TIMING_MAX_SAMPLES", "12oops");
    Astral::Core::FrameTimingCapture capture;
    std::string error;
    Check(capture.ConfigureFromEnvironment(error) == Astral::Core::FrameTimingEnvironmentStatus::Invalid,
        "malformed max sample environment value must be rejected");
    Check(!capture.Enabled(), "invalid environment configuration must not enable capture");
    Check(error.find("ASTRAL_FRAME_TIMING_MAX_SAMPLES") != std::string::npos,
        "invalid environment error should name the setting");
    ClearTimingEnvironment();
}

} // namespace

int main() {
    const auto root = MakeTempRoot();
    TestConfigureRejectsEmptyAndRelativePaths(root);
    TestConfigureRejectsUnsafeSampleLimits(root);
    TestConfigureRejectsExistingOutputs(root);
    TestWarmupCaptureAndFlush(root);
    TestDiscardedWarmupDoesNotPoisonEvidence(root);
    TestInvalidSamplesBlockEvidence(root);
    TestSaturationIsExplicit(root);
    TestNoSamplesCannotPublish(root);
    TestOutputRaceIsRejected(root);
    TestClockEnvironmentIntegration(root);
    TestEnvironmentRejectsMalformedLimit(root);
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    if (failures != 0) {
        std::cerr << failures << " frame timing capture checks failed\n";
        return 1;
    }
    std::cout << "FRAME TIMING CAPTURE TESTS: PASS (11 groups)\n";
    return 0;
}
