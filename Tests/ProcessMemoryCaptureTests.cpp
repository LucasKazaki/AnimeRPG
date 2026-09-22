#include "Engine/Core/ProcessMemoryCapture.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
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
        / ("astral-process-memory-tests-" + std::to_string(stamp));
    std::filesystem::create_directories(root);
    return root;
}

std::string ReadAll(const std::filesystem::path& path) {
    std::ifstream stream(path);
    std::ostringstream out;
    out << stream.rdbuf();
    return out.str();
}

void ClearEnvironment() {
#ifdef _WIN32
    _putenv_s("ASTRAL_PROCESS_MEMORY_CSV", "");
    _putenv_s("ASTRAL_PROCESS_MEMORY_WARMUP_FRAMES", "");
    _putenv_s("ASTRAL_PROCESS_MEMORY_SAMPLE_EVERY_FRAMES", "");
    _putenv_s("ASTRAL_PROCESS_MEMORY_MAX_SAMPLES", "");
#else
    unsetenv("ASTRAL_PROCESS_MEMORY_CSV");
    unsetenv("ASTRAL_PROCESS_MEMORY_WARMUP_FRAMES");
    unsetenv("ASTRAL_PROCESS_MEMORY_SAMPLE_EVERY_FRAMES");
    unsetenv("ASTRAL_PROCESS_MEMORY_MAX_SAMPLES");
#endif
}

void TestConfigureRejectsInvalidPathsAndLimits(const std::filesystem::path& root) {
    Astral::Core::ProcessMemoryCapture capture;
    std::string error;
    Check(!capture.Configure({}, error), "empty path must be rejected");

    Astral::Core::ProcessMemoryCaptureConfig config;
    config.outputPath = "relative.csv";
    Check(!capture.Configure(config, error), "relative path must be rejected");

    config.outputPath = root / "stride-zero.csv";
    config.sampleEveryFrames = 0;
    Check(!capture.Configure(config, error), "zero sample stride must be rejected");

    config.sampleEveryFrames = Astral::Core::ProcessMemoryCapture::kHardMaxSampleEveryFrames + 1;
    Check(!capture.Configure(config, error), "sample stride over hard cap must be rejected");

    config.sampleEveryFrames = 1;
    config.maxSamples = 0;
    Check(!capture.Configure(config, error), "zero sample cap must be rejected");
}

void TestExistingOutputsRejected(const std::filesystem::path& root) {
    Astral::Core::ProcessMemoryCapture capture;
    Astral::Core::ProcessMemoryCaptureConfig config;
    config.outputPath = root / "existing.csv";
    { std::ofstream stream(config.outputPath); stream << "existing"; }
    std::string error;
    Check(!capture.Configure(config, error), "existing output must be rejected");

    config.outputPath = root / "partial.csv";
    { std::ofstream stream(config.outputPath.string() + ".partial"); stream << "partial"; }
    Check(!capture.Configure(config, error), "existing partial output must be rejected");
}

void TestWarmupStrideAndSchema(const std::filesystem::path& root) {
    Astral::Core::ProcessMemoryCapture capture;
    Astral::Core::ProcessMemoryCaptureConfig config;
    config.outputPath = root / "sample.csv";
    config.warmupFrames = 2;
    config.sampleEveryFrames = 2;
    config.maxSamples = 3;
    std::string error;
    Check(capture.Configure(config, error), "valid config should succeed");
    Check(capture.RecordSample({0, 99, 1, 2, 3}),
        "warmup sample must be ignored before validation");
    Check(capture.RecordSample({2, 100, 120, 80, 7}),
        "first measured sample should record");
    Check(capture.RecordSample({3, 200, 100, 90, 8}),
        "non-stride sample must be ignored before validation");
    Check(capture.RecordSample({4, 110, 130, 85, 9}),
        "second measured sample should record");
    Check(capture.SampleCount() == 2, "warmup and stride should bound samples");
    Check(capture.Flush(error), "valid capture should flush");

    const auto text = ReadAll(config.outputPath);
    Check(text.find("# sample_source=caller_supplied_contract_sample") != std::string::npos,
        "fixture source must be explicit");
    Check(text.find("# allocator_attribution=unavailable") != std::string::npos,
        "output must not imply allocator attribution");
    Check(text.find("# vram=unavailable") != std::string::npos,
        "output must not imply VRAM evidence");
    Check(text.find("# leak_detection=not_established") != std::string::npos,
        "output must not claim leak detection");
    Check(text.find("2,100,120,80,7") != std::string::npos,
        "first row should be retained");
    Check(text.find("4,110,130,85,9") != std::string::npos,
        "second row should be retained");
}

void TestInvalidPeakBlocksPublication(const std::filesystem::path& root) {
    Astral::Core::ProcessMemoryCapture capture;
    Astral::Core::ProcessMemoryCaptureConfig config;
    config.outputPath = root / "invalid.csv";
    config.warmupFrames = 0;
    std::string error;
    Check(capture.Configure(config, error), "invalid fixture should configure");
    Check(!capture.RecordSample({0, 200, 100, 50, 1}),
        "peak below current working set must be rejected");
    Check(capture.InvalidSampleObserved(), "invalid sample flag must latch");
    Check(!capture.Flush(error), "invalid sample must block publication");
}

void TestSaturationIsExplicit(const std::filesystem::path& root) {
    Astral::Core::ProcessMemoryCapture capture;
    Astral::Core::ProcessMemoryCaptureConfig config;
    config.outputPath = root / "saturated.csv";
    config.warmupFrames = 0;
    config.maxSamples = 1;
    std::string error;
    Check(capture.Configure(config, error), "saturation fixture should configure");
    Check(capture.RecordSample({0, 100, 100, 80, 1}), "sample 0 should record");
    Check(capture.RecordSample({1, 101, 110, 81, 2}), "sample over cap should not grow buffer");
    Check(capture.SampleCount() == 1, "sample cap must be enforced");
    Check(capture.Saturated(), "saturation marker must latch");
    Check(capture.Flush(error), "saturated evidence may publish with explicit marker");
    Check(ReadAll(config.outputPath).find("# samples_saturated=1") != std::string::npos,
        "saturation marker should be serialized");
}

void TestNoSamplesCannotPublish(const std::filesystem::path& root) {
    Astral::Core::ProcessMemoryCapture capture;
    Astral::Core::ProcessMemoryCaptureConfig config;
    config.outputPath = root / "empty.csv";
    config.warmupFrames = 100;
    std::string error;
    Check(capture.Configure(config, error), "empty fixture should configure");
    Check(capture.RecordSample({0, 500, 100, 0, 0}), "warmup row should be ignored");
    Check(!capture.Flush(error), "empty measured capture must not publish");
}

void TestOutputRaceIsRejected(const std::filesystem::path& root) {
    Astral::Core::ProcessMemoryCapture capture;
    Astral::Core::ProcessMemoryCaptureConfig config;
    config.outputPath = root / "race.csv";
    config.warmupFrames = 0;
    std::string error;
    Check(capture.Configure(config, error), "race fixture should configure");
    Check(capture.RecordSample({0, 100, 100, 80, 1}), "race fixture should record");
    { std::ofstream stream(config.outputPath); stream << "competitor"; }
    Check(!capture.Flush(error), "output race must block overwrite");
    Check(ReadAll(config.outputPath) == "competitor", "competitor evidence must be preserved");
}

void TestSamplerSourceGuard(const std::filesystem::path& root) {
    Astral::Core::ProcessMemoryCapture capture;
    Astral::Core::ProcessMemoryCaptureConfig config;
    config.outputPath = root / "wrong-source.csv";
    config.warmupFrames = 0;
    std::string error;
    Check(capture.Configure(config, error), "source guard fixture should configure");
    Check(!capture.RecordCurrentProcess(0),
        "caller-supplied mode must not masquerade as Windows sampler");
    Check(capture.SamplingFailureObserved(), "source misuse must latch sampler failure");
    Check(!capture.Flush(error), "sampler failure must block publication");
}

void TestEnvironmentAbsence() {
    ClearEnvironment();
    Astral::Core::ProcessMemoryCapture capture;
    std::string error;
    Check(capture.ConfigureFromEnvironment(error)
        == Astral::Core::ProcessMemoryEnvironmentStatus::NotRequested,
        "capture should remain disabled when environment path is absent");
}

#ifdef _WIN32
void TestWindowsCurrentProcessSampler(const std::filesystem::path& root) {
    ClearEnvironment();
    const auto output = root / "windows-real.csv";
    _wputenv_s(L"ASTRAL_PROCESS_MEMORY_CSV", output.c_str());
    _putenv_s("ASTRAL_PROCESS_MEMORY_WARMUP_FRAMES", "0");
    _putenv_s("ASTRAL_PROCESS_MEMORY_SAMPLE_EVERY_FRAMES", "1");
    _putenv_s("ASTRAL_PROCESS_MEMORY_MAX_SAMPLES", "2");

    Astral::Core::ProcessMemoryCapture capture;
    std::string error;
    Check(capture.ConfigureFromEnvironment(error)
        == Astral::Core::ProcessMemoryEnvironmentStatus::Enabled,
        "Windows environment should enable production sampler");
    Check(capture.RecordCurrentProcess(0), "Windows current-process sample should succeed");
    Check(capture.SampleCount() == 1, "Windows sampler should record one sample");
    Check(capture.Flush(error), "Windows real sample should publish");
    const auto text = ReadAll(output);
    Check(text.find("# sample_source=Windows_GetProcessMemoryInfo_PROCESS_MEMORY_COUNTERS_EX")
            != std::string::npos,
        "Windows source identity must be serialized");
    ClearEnvironment();
}
#endif

} // namespace

int main() {
    const auto root = MakeTempRoot();
    TestConfigureRejectsInvalidPathsAndLimits(root);
    TestExistingOutputsRejected(root);
    TestWarmupStrideAndSchema(root);
    TestInvalidPeakBlocksPublication(root);
    TestSaturationIsExplicit(root);
    TestNoSamplesCannotPublish(root);
    TestOutputRaceIsRejected(root);
    TestSamplerSourceGuard(root);
    TestEnvironmentAbsence();
#ifdef _WIN32
    TestWindowsCurrentProcessSampler(root);
#endif
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    if (failures != 0) {
        std::cerr << failures << " process memory capture checks failed\n";
        return 1;
    }
#ifdef _WIN32
    std::cout << "PROCESS MEMORY CAPTURE TESTS: PASS (10 groups, including real Windows sampler)\n";
#else
    std::cout << "PROCESS MEMORY CAPTURE TESTS: PASS (9 portable groups)\n";
#endif
    return 0;
}
