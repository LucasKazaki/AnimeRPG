#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Astral::Core {

struct FrameTimingCaptureConfig {
    std::filesystem::path outputPath;
    std::uint64_t warmupFrames = 120;
    std::size_t maxSamples = 36000;
};

enum class FrameTimingEnvironmentStatus {
    NotRequested,
    Enabled,
    Invalid,
};

struct FrameTimingSample {
    std::uint64_t frameIndex = 0;
    double cpuFrameIntervalMs = 0.0;
};

class FrameTimingCapture {
public:
    static constexpr std::size_t kHardMaxSamples = 1000000;

    bool Configure(const FrameTimingCaptureConfig& config, std::string& error);
    FrameTimingEnvironmentStatus ConfigureFromEnvironment(std::string& error);
    bool Enabled() const noexcept;
    const FrameTimingCaptureConfig& Config() const noexcept { return config_; }
    bool Record(std::uint64_t frameIndex, double cpuFrameIntervalMs) noexcept;
    bool Flush(std::string& error);

    std::size_t SampleCount() const noexcept;
    bool Saturated() const noexcept;
    bool InvalidSampleObserved() const noexcept;

private:
    FrameTimingCaptureConfig config_{};
    std::vector<FrameTimingSample> samples_;
    bool enabled_ = false;
    bool flushed_ = false;
    bool saturated_ = false;
    bool invalidSampleObserved_ = false;
};

} // namespace Astral::Core
