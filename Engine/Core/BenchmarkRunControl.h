#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace Astral::Core {

enum class BenchmarkRunControlEnvironmentStatus {
    NotRequested,
    Enabled,
    Invalid,
};

struct BenchmarkRunControlConfig {
    std::filesystem::path receiptPath;
    std::uint32_t simulationFixedHz{};
    std::uint64_t warmupFrames{};
    std::uint64_t measuredFrames{};
};

class BenchmarkRunControl {
public:
    static constexpr std::uint64_t kHardMaxFrames = 1000000;

    bool Configure(const BenchmarkRunControlConfig& config, std::string& error);
    BenchmarkRunControlEnvironmentStatus ConfigureFromEnvironment(
        std::uint32_t simulationFixedHz, std::string& error);

    bool Enabled() const noexcept { return enabled_; }
    bool SuppressLiveInput() const noexcept { return enabled_; }
    std::uint32_t SimulationFixedHz() const noexcept { return config_.simulationFixedHz; }
    std::uint64_t WarmupFrames() const noexcept { return config_.warmupFrames; }
    std::uint64_t MeasuredFrames() const noexcept { return config_.measuredFrames; }
    std::uint64_t TotalFrames() const noexcept;
    std::uint64_t CompletedFrames() const noexcept { return completedFrames_; }

    // Returns true only when this call completes the exact configured frame count.
    bool CompleteFrame() noexcept;
    bool FlushCompletion(std::string& error);

private:
    BenchmarkRunControlConfig config_{};
    std::uint64_t completedFrames_{};
    bool enabled_{};
    bool flushed_{};
    bool overrun_{};
};

} // namespace Astral::Core
