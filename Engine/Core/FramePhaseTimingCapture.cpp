#include "Engine/Core/FramePhaseTimingCapture.h"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
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

bool FramePhaseTimingCapture::Configure(
    const FramePhaseTimingCaptureConfig& config, std::string& error) {
    error.clear();
    enabled_ = false;
    flushed_ = false;
    saturated_ = false;
    invalidSampleObserved_ = false;
    samples_.clear();

    if (config.outputPath.empty()) {
        error = "frame phase timing output path is empty";
        return false;
    }
    if (!config.outputPath.is_absolute()) {
        error = "frame phase timing output path must be absolute";
        return false;
    }
    if (config.maxSamples == 0 || config.maxSamples > kHardMaxSamples) {
        error = "frame phase timing maxSamples must be in [1, 1000000]";
        return false;
    }

    std::error_code ec;
    const auto parent = config.outputPath.parent_path();
    if (parent.empty() || !std::filesystem::exists(parent, ec) || ec
        || !std::filesystem::is_directory(parent, ec) || ec) {
        error = "frame phase timing output parent must be an existing directory";
        return false;
    }
    if (std::filesystem::exists(config.outputPath, ec) || ec) {
        error = "frame phase timing output already exists or could not be inspected";
        return false;
    }
    const auto partialPath = PartialOutputPath(config.outputPath);
    ec.clear();
    if (std::filesystem::exists(partialPath, ec) || ec) {
        error = "frame phase timing partial output already exists or could not be inspected";
        return false;
    }

    config_ = config;
    samples_.reserve(config.maxSamples);
    enabled_ = true;
    return true;
}

FramePhaseTimingEnvironmentStatus FramePhaseTimingCapture::ConfigureFromEnvironment(
    std::string& error) {
    error.clear();
    FramePhaseTimingCaptureConfig config;

#ifdef _WIN32
    const wchar_t* output = _wgetenv(L"ASTRAL_FRAME_PHASE_TIMING_CSV");
    if (output == nullptr || output[0] == L'\0') {
        return FramePhaseTimingEnvironmentStatus::NotRequested;
    }
    config.outputPath = std::filesystem::path(output);

    if (const wchar_t* warmup = _wgetenv(L"ASTRAL_FRAME_PHASE_TIMING_WARMUP_FRAMES")) {
        if (warmup[0] != L'\0') {
            std::uint64_t parsed = 0;
            if (!ParseUnsigned(std::wstring_view(warmup),
                    std::numeric_limits<std::uint64_t>::max(), parsed)) {
                error = "ASTRAL_FRAME_PHASE_TIMING_WARMUP_FRAMES must be an unsigned decimal integer";
                return FramePhaseTimingEnvironmentStatus::Invalid;
            }
            config.warmupFrames = parsed;
        }
    }
    if (const wchar_t* maxSamples = _wgetenv(L"ASTRAL_FRAME_PHASE_TIMING_MAX_SAMPLES")) {
        if (maxSamples[0] != L'\0') {
            std::uint64_t parsed = 0;
            if (!ParseUnsigned(std::wstring_view(maxSamples), kHardMaxSamples, parsed)
                || parsed == 0) {
                error = "ASTRAL_FRAME_PHASE_TIMING_MAX_SAMPLES must be an integer in [1, 1000000]";
                return FramePhaseTimingEnvironmentStatus::Invalid;
            }
            config.maxSamples = static_cast<std::size_t>(parsed);
        }
    }
#else
    const char* output = std::getenv("ASTRAL_FRAME_PHASE_TIMING_CSV");
    if (output == nullptr || output[0] == '\0') {
        return FramePhaseTimingEnvironmentStatus::NotRequested;
    }
    config.outputPath = std::filesystem::u8path(output);

    if (const char* warmup = std::getenv("ASTRAL_FRAME_PHASE_TIMING_WARMUP_FRAMES")) {
        if (warmup[0] != '\0') {
            std::uint64_t parsed = 0;
            if (!ParseUnsigned(std::string_view(warmup),
                    std::numeric_limits<std::uint64_t>::max(), parsed)) {
                error = "ASTRAL_FRAME_PHASE_TIMING_WARMUP_FRAMES must be an unsigned decimal integer";
                return FramePhaseTimingEnvironmentStatus::Invalid;
            }
            config.warmupFrames = parsed;
        }
    }
    if (const char* maxSamples = std::getenv("ASTRAL_FRAME_PHASE_TIMING_MAX_SAMPLES")) {
        if (maxSamples[0] != '\0') {
            std::uint64_t parsed = 0;
            if (!ParseUnsigned(std::string_view(maxSamples), kHardMaxSamples, parsed)
                || parsed == 0) {
                error = "ASTRAL_FRAME_PHASE_TIMING_MAX_SAMPLES must be an integer in [1, 1000000]";
                return FramePhaseTimingEnvironmentStatus::Invalid;
            }
            config.maxSamples = static_cast<std::size_t>(parsed);
        }
    }
#endif

    if (!Configure(config, error)) {
        return FramePhaseTimingEnvironmentStatus::Invalid;
    }
    return FramePhaseTimingEnvironmentStatus::Enabled;
}

bool FramePhaseTimingCapture::Enabled() const noexcept {
    return enabled_;
}

bool FramePhaseTimingCapture::Record(std::uint64_t frameIndex, double messagePumpMs,
    double updateControlMs, double renderSubmitMs, double frameWaitMs) noexcept {
    if (!enabled_ || flushed_) {
        return false;
    }
    if (frameIndex < config_.warmupFrames) {
        return true;
    }
    const double phases[] = {messagePumpMs, updateControlMs, renderSubmitMs, frameWaitMs};
    double total = 0.0;
    for (const double phase : phases) {
        if (!std::isfinite(phase) || phase < 0.0) {
            invalidSampleObserved_ = true;
            return false;
        }
        total += phase;
    }
    if (!std::isfinite(total) || total <= 0.0) {
        invalidSampleObserved_ = true;
        return false;
    }
    if (samples_.size() >= config_.maxSamples) {
        saturated_ = true;
        return true;
    }
    samples_.push_back({frameIndex, messagePumpMs, updateControlMs,
        renderSubmitMs, frameWaitMs, total});
    return true;
}

bool FramePhaseTimingCapture::Flush(std::string& error) {
    error.clear();
    if (!enabled_) {
        error = "frame phase timing capture is not enabled";
        return false;
    }
    if (flushed_) {
        error = "frame phase timing capture has already been flushed";
        return false;
    }
    if (invalidSampleObserved_) {
        error = "frame phase timing capture observed an invalid phase duration";
        return false;
    }
    if (samples_.empty()) {
        error = "frame phase timing capture contains no post-warmup samples";
        return false;
    }

    std::error_code ec;
    if (std::filesystem::exists(config_.outputPath, ec) || ec) {
        error = "frame phase timing output appeared before flush; refusing to overwrite";
        return false;
    }
    const auto partialPath = PartialOutputPath(config_.outputPath);
    ec.clear();
    if (std::filesystem::exists(partialPath, ec) || ec) {
        error = "frame phase timing partial output appeared before flush; refusing to overwrite";
        return false;
    }

    std::ofstream stream(partialPath, std::ios::out | std::ios::trunc);
    if (!stream) {
        error = "could not create frame phase timing partial output";
        return false;
    }

    stream << "# astral_frame_phase_timing_schema=1\n";
    stream << "# metric=main_thread_phase_wall_ms\n";
    stream << "# units=milliseconds\n";
    stream << "# measurement_semantics=wall_clock_intervals_for_contiguous_Win32_main_loop_phases\n";
    stream << "# timing_source=std::chrono::steady_clock\n";
    stream << "# phase_order=message_pump,update_control,render_submit,frame_wait\n";
    stream << "# cpu_scope=Win32_main_thread_only\n";
    stream << "# gpu_timing=unavailable\n";
    stream << "# acceptance_claim=none\n";
    stream << "# warmup_frames=" << config_.warmupFrames << "\n";
    stream << "# max_samples=" << config_.maxSamples << "\n";
    stream << "# samples_saturated=" << (saturated_ ? 1 : 0) << "\n";
    stream << "frame_index,message_pump_ms,update_control_ms,render_submit_ms,frame_wait_ms,loop_total_ms\n";
    stream << std::fixed << std::setprecision(6);
    for (const auto& sample : samples_) {
        stream << sample.frameIndex << ',' << sample.messagePumpMs << ','
               << sample.updateControlMs << ',' << sample.renderSubmitMs << ','
               << sample.frameWaitMs << ',' << sample.loopTotalMs << '\n';
    }
    stream.flush();
    if (!stream) {
        error = "failed while writing frame phase timing partial output";
        stream.close();
        return false;
    }
    stream.close();
    if (!stream) {
        error = "failed while closing frame phase timing partial output";
        return false;
    }

    std::filesystem::create_hard_link(partialPath, config_.outputPath, ec);
    if (ec) {
        error = "could not publish frame phase timing output without overwrite: " + ec.message();
        return false;
    }
    std::error_code removeError;
    std::filesystem::remove(partialPath, removeError);
    if (removeError) {
        error = "frame phase timing output was published but partial cleanup failed: "
            + removeError.message();
        return false;
    }

    flushed_ = true;
    return true;
}

std::size_t FramePhaseTimingCapture::SampleCount() const noexcept {
    return samples_.size();
}

bool FramePhaseTimingCapture::Saturated() const noexcept {
    return saturated_;
}

bool FramePhaseTimingCapture::InvalidSampleObserved() const noexcept {
    return invalidSampleObserved_;
}

} // namespace Astral::Core
