#include "Engine/Core/FrameTimingCapture.h"

#include <cmath>
#include <cstdlib>
#include <limits>
#include <string_view>
#include <fstream>
#include <iomanip>
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

bool FrameTimingCapture::Configure(const FrameTimingCaptureConfig& config, std::string& error) {
    error.clear();
    enabled_ = false;
    flushed_ = false;
    saturated_ = false;
    invalidSampleObserved_ = false;
    samples_.clear();

    if (config.outputPath.empty()) {
        error = "frame timing output path is empty";
        return false;
    }
    if (!config.outputPath.is_absolute()) {
        error = "frame timing output path must be absolute";
        return false;
    }
    if (config.maxSamples == 0 || config.maxSamples > kHardMaxSamples) {
        error = "frame timing maxSamples must be in [1, 1000000]";
        return false;
    }

    std::error_code ec;
    const auto parent = config.outputPath.parent_path();
    if (parent.empty() || !std::filesystem::exists(parent, ec) || ec
        || !std::filesystem::is_directory(parent, ec) || ec) {
        error = "frame timing output parent must be an existing directory";
        return false;
    }
    if (std::filesystem::exists(config.outputPath, ec) || ec) {
        error = "frame timing output already exists or could not be inspected";
        return false;
    }
    const auto partialPath = PartialOutputPath(config.outputPath);
    ec.clear();
    if (std::filesystem::exists(partialPath, ec) || ec) {
        error = "frame timing partial output already exists or could not be inspected";
        return false;
    }

    config_ = config;
    samples_.reserve(config.maxSamples);
    enabled_ = true;
    return true;
}


FrameTimingEnvironmentStatus FrameTimingCapture::ConfigureFromEnvironment(std::string& error) {
    error.clear();
    FrameTimingCaptureConfig config;

#ifdef _WIN32
    const wchar_t* output = _wgetenv(L"ASTRAL_FRAME_TIMING_CSV");
    if (output == nullptr || output[0] == L'\0') {
        return FrameTimingEnvironmentStatus::NotRequested;
    }
    config.outputPath = std::filesystem::path(output);

    if (const wchar_t* warmup = _wgetenv(L"ASTRAL_FRAME_TIMING_WARMUP_FRAMES")) {
        if (warmup[0] != L'\0') {
            std::uint64_t parsed = 0;
            if (!ParseUnsigned(std::wstring_view(warmup), std::numeric_limits<std::uint64_t>::max(), parsed)) {
                error = "ASTRAL_FRAME_TIMING_WARMUP_FRAMES must be an unsigned decimal integer";
                return FrameTimingEnvironmentStatus::Invalid;
            }
            config.warmupFrames = parsed;
        }
    }
    if (const wchar_t* maxSamples = _wgetenv(L"ASTRAL_FRAME_TIMING_MAX_SAMPLES")) {
        if (maxSamples[0] != L'\0') {
            std::uint64_t parsed = 0;
            if (!ParseUnsigned(std::wstring_view(maxSamples), kHardMaxSamples, parsed) || parsed == 0) {
                error = "ASTRAL_FRAME_TIMING_MAX_SAMPLES must be an integer in [1, 1000000]";
                return FrameTimingEnvironmentStatus::Invalid;
            }
            config.maxSamples = static_cast<std::size_t>(parsed);
        }
    }
#else
    const char* output = std::getenv("ASTRAL_FRAME_TIMING_CSV");
    if (output == nullptr || output[0] == '\0') {
        return FrameTimingEnvironmentStatus::NotRequested;
    }
    config.outputPath = std::filesystem::u8path(output);

    if (const char* warmup = std::getenv("ASTRAL_FRAME_TIMING_WARMUP_FRAMES")) {
        if (warmup[0] != '\0') {
            std::uint64_t parsed = 0;
            if (!ParseUnsigned(std::string_view(warmup), std::numeric_limits<std::uint64_t>::max(), parsed)) {
                error = "ASTRAL_FRAME_TIMING_WARMUP_FRAMES must be an unsigned decimal integer";
                return FrameTimingEnvironmentStatus::Invalid;
            }
            config.warmupFrames = parsed;
        }
    }
    if (const char* maxSamples = std::getenv("ASTRAL_FRAME_TIMING_MAX_SAMPLES")) {
        if (maxSamples[0] != '\0') {
            std::uint64_t parsed = 0;
            if (!ParseUnsigned(std::string_view(maxSamples), kHardMaxSamples, parsed) || parsed == 0) {
                error = "ASTRAL_FRAME_TIMING_MAX_SAMPLES must be an integer in [1, 1000000]";
                return FrameTimingEnvironmentStatus::Invalid;
            }
            config.maxSamples = static_cast<std::size_t>(parsed);
        }
    }
#endif

    if (!Configure(config, error)) {
        return FrameTimingEnvironmentStatus::Invalid;
    }
    return FrameTimingEnvironmentStatus::Enabled;
}

bool FrameTimingCapture::Enabled() const noexcept {
    return enabled_;
}

bool FrameTimingCapture::Record(std::uint64_t frameIndex, double cpuFrameIntervalMs) noexcept {
    if (!enabled_ || flushed_) {
        return false;
    }
    if (frameIndex < config_.warmupFrames) {
        return true;
    }
    if (!std::isfinite(cpuFrameIntervalMs) || cpuFrameIntervalMs <= 0.0) {
        invalidSampleObserved_ = true;
        return false;
    }
    if (samples_.size() >= config_.maxSamples) {
        saturated_ = true;
        return true;
    }
    samples_.push_back({frameIndex, cpuFrameIntervalMs});
    return true;
}

bool FrameTimingCapture::Flush(std::string& error) {
    error.clear();
    if (!enabled_) {
        error = "frame timing capture is not enabled";
        return false;
    }
    if (flushed_) {
        error = "frame timing capture has already been flushed";
        return false;
    }
    if (invalidSampleObserved_) {
        error = "frame timing capture observed a non-finite or non-positive frame interval";
        return false;
    }
    if (samples_.empty()) {
        error = "frame timing capture contains no post-warmup samples";
        return false;
    }

    std::error_code ec;
    if (std::filesystem::exists(config_.outputPath, ec) || ec) {
        error = "frame timing output appeared before flush; refusing to overwrite";
        return false;
    }
    const auto partialPath = PartialOutputPath(config_.outputPath);
    ec.clear();
    if (std::filesystem::exists(partialPath, ec) || ec) {
        error = "frame timing partial output appeared before flush; refusing to overwrite";
        return false;
    }

    std::ofstream stream(partialPath, std::ios::out | std::ios::trunc);
    if (!stream) {
        error = "could not create frame timing partial output";
        return false;
    }

    stream << "# astral_frame_timing_schema=1\n";
    stream << "# metric=cpu_frame_interval_ms\n";
    stream << "# units=milliseconds\n";
    stream << "# measurement_semantics=wall_clock_interval_between_consecutive_Clock_Tick_calls\n";
    stream << "# timing_source=Engine/Core/Clock.cpp:std::chrono::steady_clock\n";
    stream << "# gpu_timing=unavailable\n";
    stream << "# acceptance_claim=none\n";
    stream << "# warmup_frames=" << config_.warmupFrames << "\n";
    stream << "# max_samples=" << config_.maxSamples << "\n";
    stream << "# samples_saturated=" << (saturated_ ? 1 : 0) << "\n";
    stream << "frame_index,cpu_frame_interval_ms\n";
    stream << std::fixed << std::setprecision(6);
    for (const auto& sample : samples_) {
        stream << sample.frameIndex << ',' << sample.cpuFrameIntervalMs << '\n';
    }
    stream.flush();
    if (!stream) {
        error = "failed while writing frame timing partial output";
        stream.close();
        return false;
    }
    stream.close();
    if (!stream) {
        error = "failed while closing frame timing partial output";
        return false;
    }

    // Publish without an overwrite race. create_hard_link fails when final output already
    // exists, unlike POSIX rename which may replace an evidence file created concurrently.
    std::filesystem::create_hard_link(partialPath, config_.outputPath, ec);
    if (ec) {
        error = "could not publish frame timing output without overwrite: " + ec.message();
        return false;
    }
    std::error_code removeError;
    std::filesystem::remove(partialPath, removeError);
    if (removeError) {
        error = "frame timing output was published but partial cleanup failed: " + removeError.message();
        return false;
    }

    flushed_ = true;
    return true;
}

std::size_t FrameTimingCapture::SampleCount() const noexcept {
    return samples_.size();
}

bool FrameTimingCapture::Saturated() const noexcept {
    return saturated_;
}

bool FrameTimingCapture::InvalidSampleObserved() const noexcept {
    return invalidSampleObserved_;
}

} // namespace Astral::Core
