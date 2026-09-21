#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Astral::Core {

struct FramePhaseTimingCaptureConfig {
    std::filesystem::path outputPath;
    std::uint64_t warmupFrames = 120;
    std::size_t maxSamples = 36000;
};

enum class FramePhaseTimingEnvironmentStatus {
    NotRequested,
    Enabled,
    Invalid,
};

struct FramePhaseTimingSample {
    std::uint64_t frameIndex = 0;
    double messagePumpMs = 0.0;
    double updateControlMs = 0.0;
    double renderSubmitMs = 0.0;
    double frameWaitMs = 0.0;
    double loopTotalMs = 0.0;
};

class FramePhaseTimingCapture {
public:
    static constexpr std::size_t kHardMaxSamples = 1000000;

    bool Configure(const FramePhaseTimingCaptureConfig& config, std::string& error);
    FramePhaseTimingEnvironmentStatus ConfigureFromEnvironment(std::string& error);
    bool Enabled() const noexcept;
    const FramePhaseTimingCaptureConfig& Config() const noexcept { return config_; }
    bool Record(std::uint64_t frameIndex, double messagePumpMs, double updateControlMs,
        double renderSubmitMs, double frameWaitMs) noexcept;
    bool Flush(std::string& error);

    std::size_t SampleCount() const noexcept;
    bool Saturated() const noexcept;
    bool InvalidSampleObserved() const noexcept;

private:
    FramePhaseTimingCaptureConfig config_{};
    std::vector<FramePhaseTimingSample> samples_;
    bool enabled_ = false;
    bool flushed_ = false;
    bool saturated_ = false;
    bool invalidSampleObserved_ = false;
};

} // namespace Astral::Core
