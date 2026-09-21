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
    std::uint32_t clientWidthPx{};
    std::uint32_t clientHeightPx{};
};

class BenchmarkRunControl {
public:
    static constexpr std::uint64_t kHardMaxFrames = 1000000;
    static constexpr std::uint32_t kHardMaxClientDimension = 16384;

    static BenchmarkRunControlEnvironmentStatus RequestedClientAreaFromEnvironment(
        std::uint32_t& widthPx,
        std::uint32_t& heightPx,
        std::string& error);

    bool Configure(const BenchmarkRunControlConfig& config, std::string& error);
    BenchmarkRunControlEnvironmentStatus ConfigureFromEnvironment(
        std::uint32_t simulationFixedHz,
        std::uint32_t clientWidthPx,
        std::uint32_t clientHeightPx,
        std::string& error);

    bool Enabled() const noexcept { return enabled_; }
    bool SuppressLiveInput() const noexcept { return enabled_; }
    std::uint32_t SimulationFixedHz() const noexcept { return config_.simulationFixedHz; }
    std::uint64_t WarmupFrames() const noexcept { return config_.warmupFrames; }
    std::uint64_t MeasuredFrames() const noexcept { return config_.measuredFrames; }
    std::uint32_t ClientWidthPx() const noexcept { return config_.clientWidthPx; }
    std::uint32_t ClientHeightPx() const noexcept { return config_.clientHeightPx; }
    std::uint64_t ClientAreaObservations() const noexcept { return clientAreaObservations_; }
    std::uint64_t TotalFrames() const noexcept;
    std::uint64_t CompletedFrames() const noexcept { return completedFrames_; }

    // Must be called exactly once before CompleteFrame() for every benchmark frame.
    bool ObserveClientArea(std::uint32_t widthPx, std::uint32_t heightPx) noexcept;
    // Returns true only when this call completes the exact configured frame count.
    bool CompleteFrame() noexcept;
    bool FlushCompletion(std::string& error);

private:
    BenchmarkRunControlConfig config_{};
    std::uint64_t completedFrames_{};
    std::uint64_t clientAreaObservations_{};
    bool enabled_{};
    bool flushed_{};
    bool overrun_{};
    bool clientAreaInvalid_{};
    bool explicitClientAreaRequested_{};
};

} // namespace Astral::Core
