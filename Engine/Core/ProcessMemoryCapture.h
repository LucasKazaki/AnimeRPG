#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Astral::Core {

enum class ProcessMemorySampleSource {
    CallerSupplied,
    WindowsProcessCounters,
};

struct ProcessMemoryCaptureConfig {
    std::filesystem::path outputPath;
    std::uint64_t warmupFrames = 120;
    std::uint64_t sampleEveryFrames = 1;
    std::size_t maxSamples = 36000;
    ProcessMemorySampleSource sampleSource = ProcessMemorySampleSource::CallerSupplied;
};

enum class ProcessMemoryEnvironmentStatus {
    NotRequested,
    Enabled,
    Invalid,
};

struct ProcessMemorySample {
    std::uint64_t frameIndex = 0;
    std::uint64_t workingSetBytes = 0;
    std::uint64_t peakWorkingSetBytes = 0;
    std::uint64_t privateUsageBytes = 0;
    std::uint64_t pageFaultCount = 0;
};

class ProcessMemoryCapture {
public:
    static constexpr std::size_t kHardMaxSamples = 1000000;
    static constexpr std::uint64_t kHardMaxSampleEveryFrames = 1000000;

    bool Configure(const ProcessMemoryCaptureConfig& config, std::string& error);
    ProcessMemoryEnvironmentStatus ConfigureFromEnvironment(std::string& error);
    bool Enabled() const noexcept;
    bool RecordSample(const ProcessMemorySample& sample) noexcept;
    bool RecordCurrentProcess(std::uint64_t frameIndex) noexcept;
    bool Flush(std::string& error);

    std::size_t SampleCount() const noexcept;
    bool Saturated() const noexcept;
    bool InvalidSampleObserved() const noexcept;
    bool SamplingFailureObserved() const noexcept;

private:
    bool ShouldSample(std::uint64_t frameIndex) const noexcept;

    ProcessMemoryCaptureConfig config_{};
    std::vector<ProcessMemorySample> samples_;
    bool enabled_ = false;
    bool flushed_ = false;
    bool saturated_ = false;
    bool invalidSampleObserved_ = false;
    bool samplingFailureObserved_ = false;
};

} // namespace Astral::Core
