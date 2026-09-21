#include "Engine/Core/BenchmarkRunControl.h"

#include <cstdlib>
#include <fstream>
#include <limits>
#include <string_view>
#include <system_error>

namespace {

bool ParseUnsigned(std::string_view text, std::uint64_t maximum, std::uint64_t& value) {
    if (text.empty()) return false;
    std::uint64_t result = 0;
    for (const char ch : text) {
        if (ch < '0' || ch > '9') return false;
        const auto digit = static_cast<std::uint64_t>(ch - '0');
        if (result > (maximum - digit) / 10) return false;
        result = result * 10 + digit;
    }
    value = result;
    return true;
}

std::filesystem::path PartialOutputPath(const std::filesystem::path& finalPath) {
    auto partial = finalPath;
    partial += ".partial";
    return partial;
}

#ifdef _WIN32
bool ParseUnsigned(std::wstring_view text, std::uint64_t maximum, std::uint64_t& value) {
    if (text.empty()) return false;
    std::uint64_t result = 0;
    for (const wchar_t ch : text) {
        if (ch < L'0' || ch > L'9') return false;
        const auto digit = static_cast<std::uint64_t>(ch - L'0');
        if (result > (maximum - digit) / 10) return false;
        result = result * 10 + digit;
    }
    value = result;
    return true;
}
#endif

} // namespace

namespace Astral::Core {

bool BenchmarkRunControl::Configure(
    const BenchmarkRunControlConfig& config, std::string& error) {
    error.clear();
    enabled_ = false;
    flushed_ = false;
    overrun_ = false;
    completedFrames_ = 0;
    config_ = {};

    if (config.simulationFixedHz < 1 || config.simulationFixedHz > 1000) {
        error = "benchmark mode requires an active fixed simulation rate in [1, 1000] Hz";
        return false;
    }
    if (config.measuredFrames == 0 || config.measuredFrames > kHardMaxFrames) {
        error = "benchmark measured frames must be in [1, 1000000]";
        return false;
    }
    if (config.warmupFrames > kHardMaxFrames
        || config.warmupFrames > kHardMaxFrames - config.measuredFrames) {
        error = "benchmark warmup plus measured frames must be <= 1000000";
        return false;
    }
    if (config.receiptPath.empty() || !config.receiptPath.is_absolute()) {
        error = "benchmark control receipt path must be absolute";
        return false;
    }

    std::error_code ec;
    const auto parent = config.receiptPath.parent_path();
    if (parent.empty() || !std::filesystem::exists(parent, ec) || ec
        || !std::filesystem::is_directory(parent, ec) || ec) {
        error = "benchmark control receipt parent must be an existing directory";
        return false;
    }
    if (std::filesystem::exists(config.receiptPath, ec) || ec) {
        error = "benchmark control receipt already exists or could not be inspected";
        return false;
    }
    const auto partialPath = PartialOutputPath(config.receiptPath);
    ec.clear();
    if (std::filesystem::exists(partialPath, ec) || ec) {
        error = "benchmark control partial receipt already exists or could not be inspected";
        return false;
    }

    config_ = config;
    enabled_ = true;
    return true;
}

BenchmarkRunControlEnvironmentStatus BenchmarkRunControl::ConfigureFromEnvironment(
    std::uint32_t simulationFixedHz, std::string& error) {
    error.clear();

#ifdef _WIN32
    const wchar_t* mode = _wgetenv(L"ASTRAL_BENCHMARK_MODE");
    const wchar_t* warmup = _wgetenv(L"ASTRAL_BENCHMARK_WARMUP_FRAMES");
    const wchar_t* measured = _wgetenv(L"ASTRAL_BENCHMARK_MEASURED_FRAMES");
    const wchar_t* receipt = _wgetenv(L"ASTRAL_BENCHMARK_CONTROL_JSON");
    if (mode == nullptr) {
        if (warmup != nullptr || measured != nullptr || receipt != nullptr) {
            error = "ASTRAL_BENCHMARK_MODE is required when benchmark controls are present";
            return BenchmarkRunControlEnvironmentStatus::Invalid;
        }
        return BenchmarkRunControlEnvironmentStatus::NotRequested;
    }
    if (std::wstring_view(mode) != L"1") {
        error = "ASTRAL_BENCHMARK_MODE must be exactly 1";
        return BenchmarkRunControlEnvironmentStatus::Invalid;
    }
    if (warmup == nullptr || measured == nullptr || receipt == nullptr || receipt[0] == L'\0') {
        error = "benchmark mode requires warmup frames, measured frames, and control receipt path";
        return BenchmarkRunControlEnvironmentStatus::Invalid;
    }
    std::uint64_t warmupFrames = 0;
    std::uint64_t measuredFrames = 0;
    if (!ParseUnsigned(std::wstring_view(warmup), kHardMaxFrames, warmupFrames)
        || !ParseUnsigned(std::wstring_view(measured), kHardMaxFrames, measuredFrames)
        || measuredFrames == 0) {
        error = "benchmark frame counts must be unsigned decimals within the admitted bounds";
        return BenchmarkRunControlEnvironmentStatus::Invalid;
    }
    BenchmarkRunControlConfig config{
        std::filesystem::path(receipt), simulationFixedHz, warmupFrames, measuredFrames};
#else
    const char* mode = std::getenv("ASTRAL_BENCHMARK_MODE");
    const char* warmup = std::getenv("ASTRAL_BENCHMARK_WARMUP_FRAMES");
    const char* measured = std::getenv("ASTRAL_BENCHMARK_MEASURED_FRAMES");
    const char* receipt = std::getenv("ASTRAL_BENCHMARK_CONTROL_JSON");
    if (mode == nullptr) {
        if (warmup != nullptr || measured != nullptr || receipt != nullptr) {
            error = "ASTRAL_BENCHMARK_MODE is required when benchmark controls are present";
            return BenchmarkRunControlEnvironmentStatus::Invalid;
        }
        return BenchmarkRunControlEnvironmentStatus::NotRequested;
    }
    if (std::string_view(mode) != "1") {
        error = "ASTRAL_BENCHMARK_MODE must be exactly 1";
        return BenchmarkRunControlEnvironmentStatus::Invalid;
    }
    if (warmup == nullptr || measured == nullptr || receipt == nullptr || receipt[0] == '\0') {
        error = "benchmark mode requires warmup frames, measured frames, and control receipt path";
        return BenchmarkRunControlEnvironmentStatus::Invalid;
    }
    std::uint64_t warmupFrames = 0;
    std::uint64_t measuredFrames = 0;
    if (!ParseUnsigned(std::string_view(warmup), kHardMaxFrames, warmupFrames)
        || !ParseUnsigned(std::string_view(measured), kHardMaxFrames, measuredFrames)
        || measuredFrames == 0) {
        error = "benchmark frame counts must be unsigned decimals within the admitted bounds";
        return BenchmarkRunControlEnvironmentStatus::Invalid;
    }
    BenchmarkRunControlConfig config{
        std::filesystem::u8path(receipt), simulationFixedHz, warmupFrames, measuredFrames};
#endif

    if (!Configure(config, error)) {
        return BenchmarkRunControlEnvironmentStatus::Invalid;
    }
    return BenchmarkRunControlEnvironmentStatus::Enabled;
}

std::uint64_t BenchmarkRunControl::TotalFrames() const noexcept {
    return config_.warmupFrames + config_.measuredFrames;
}

bool BenchmarkRunControl::CompleteFrame() noexcept {
    if (!enabled_ || flushed_) return false;
    if (completedFrames_ >= TotalFrames()) {
        overrun_ = true;
        return false;
    }
    ++completedFrames_;
    return completedFrames_ == TotalFrames();
}

bool BenchmarkRunControl::FlushCompletion(std::string& error) {
    error.clear();
    if (!enabled_) {
        error = "benchmark run control is not enabled";
        return false;
    }
    if (flushed_) {
        error = "benchmark control receipt has already been flushed";
        return false;
    }
    if (overrun_ || completedFrames_ != TotalFrames()) {
        error = "benchmark run did not complete exactly the configured frame count";
        return false;
    }

    std::error_code ec;
    if (std::filesystem::exists(config_.receiptPath, ec) || ec) {
        error = "benchmark control receipt appeared before flush; refusing to overwrite";
        return false;
    }
    const auto partialPath = PartialOutputPath(config_.receiptPath);
    ec.clear();
    if (std::filesystem::exists(partialPath, ec) || ec) {
        error = "benchmark control partial receipt appeared before flush; refusing to overwrite";
        return false;
    }

    std::ofstream stream(partialPath, std::ios::out | std::ios::trunc);
    if (!stream) {
        error = "could not create benchmark control partial receipt";
        return false;
    }
    stream
        << "{\n"
        << "  \"schema_version\": 1,\n"
        << "  \"mode\": \"fixed_frame_count\",\n"
        << "  \"simulation_fixed_hz\": " << config_.simulationFixedHz << ",\n"
        << "  \"warmup_frames\": " << config_.warmupFrames << ",\n"
        << "  \"measured_frames\": " << config_.measuredFrames << ",\n"
        << "  \"total_frames\": " << TotalFrames() << ",\n"
        << "  \"completed_frames\": " << completedFrames_ << ",\n"
        << "  \"live_input\": \"suppressed\",\n"
        << "  \"termination\": \"exact_frame_limit\",\n"
        << "  \"performance_budget_verified\": false,\n"
        << "  \"comparative_parity_verified\": false,\n"
        << "  \"independent_acceptance\": false\n"
        << "}\n";
    stream.flush();
    if (!stream) {
        error = "failed while writing benchmark control partial receipt";
        stream.close();
        return false;
    }
    stream.close();
    if (!stream) {
        error = "failed while closing benchmark control partial receipt";
        return false;
    }

    std::filesystem::create_hard_link(partialPath, config_.receiptPath, ec);
    if (ec) {
        error = "could not publish benchmark control receipt without overwrite: " + ec.message();
        return false;
    }
    std::error_code removeError;
    std::filesystem::remove(partialPath, removeError);
    if (removeError) {
        error = "benchmark control receipt published but partial cleanup failed: "
            + removeError.message();
        return false;
    }

    flushed_ = true;
    return true;
}

} // namespace Astral::Core
