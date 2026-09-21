#include "Engine/Core/ProcessMemoryCapture.h"

#include <cstdlib>
#include <fstream>
#include <limits>
#include <string_view>
#include <system_error>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <Psapi.h>
#endif

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

const char* SourceName(Astral::Core::ProcessMemorySampleSource source) {
    switch (source) {
    case Astral::Core::ProcessMemorySampleSource::CallerSupplied:
        return "caller_supplied_contract_sample";
    case Astral::Core::ProcessMemorySampleSource::WindowsProcessCounters:
        return "Windows_GetProcessMemoryInfo_PROCESS_MEMORY_COUNTERS_EX";
    }
    return "unknown";
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

bool ProcessMemoryCapture::Configure(const ProcessMemoryCaptureConfig& config, std::string& error) {
    error.clear();
    enabled_ = false;
    flushed_ = false;
    saturated_ = false;
    invalidSampleObserved_ = false;
    samplingFailureObserved_ = false;
    samples_.clear();

    if (config.outputPath.empty()) {
        error = "process memory output path is empty";
        return false;
    }
    if (!config.outputPath.is_absolute()) {
        error = "process memory output path must be absolute";
        return false;
    }
    if (config.sampleEveryFrames == 0 || config.sampleEveryFrames > kHardMaxSampleEveryFrames) {
        error = "process memory sampleEveryFrames must be in [1, 1000000]";
        return false;
    }
    if (config.maxSamples == 0 || config.maxSamples > kHardMaxSamples) {
        error = "process memory maxSamples must be in [1, 1000000]";
        return false;
    }

    std::error_code ec;
    const auto parent = config.outputPath.parent_path();
    if (parent.empty() || !std::filesystem::exists(parent, ec) || ec
        || !std::filesystem::is_directory(parent, ec) || ec) {
        error = "process memory output parent must be an existing directory";
        return false;
    }
    if (std::filesystem::exists(config.outputPath, ec) || ec) {
        error = "process memory output already exists or could not be inspected";
        return false;
    }
    const auto partialPath = PartialOutputPath(config.outputPath);
    ec.clear();
    if (std::filesystem::exists(partialPath, ec) || ec) {
        error = "process memory partial output already exists or could not be inspected";
        return false;
    }

    config_ = config;
    samples_.reserve(config.maxSamples);
    enabled_ = true;
    return true;
}

ProcessMemoryEnvironmentStatus ProcessMemoryCapture::ConfigureFromEnvironment(std::string& error) {
    error.clear();
    ProcessMemoryCaptureConfig config;

#ifdef _WIN32
    const wchar_t* output = _wgetenv(L"ASTRAL_PROCESS_MEMORY_CSV");
    if (output == nullptr || output[0] == L'\0') {
        return ProcessMemoryEnvironmentStatus::NotRequested;
    }
    config.outputPath = std::filesystem::path(output);
    config.sampleSource = ProcessMemorySampleSource::WindowsProcessCounters;

    if (const wchar_t* warmup = _wgetenv(L"ASTRAL_PROCESS_MEMORY_WARMUP_FRAMES")) {
        if (warmup[0] != L'\0') {
            std::uint64_t parsed = 0;
            if (!ParseUnsigned(std::wstring_view(warmup),
                    std::numeric_limits<std::uint64_t>::max(), parsed)) {
                error = "ASTRAL_PROCESS_MEMORY_WARMUP_FRAMES must be an unsigned decimal integer";
                return ProcessMemoryEnvironmentStatus::Invalid;
            }
            config.warmupFrames = parsed;
        }
    }
    if (const wchar_t* stride = _wgetenv(L"ASTRAL_PROCESS_MEMORY_SAMPLE_EVERY_FRAMES")) {
        if (stride[0] != L'\0') {
            std::uint64_t parsed = 0;
            if (!ParseUnsigned(std::wstring_view(stride), kHardMaxSampleEveryFrames, parsed)
                || parsed == 0) {
                error = "ASTRAL_PROCESS_MEMORY_SAMPLE_EVERY_FRAMES must be an integer in [1, 1000000]";
                return ProcessMemoryEnvironmentStatus::Invalid;
            }
            config.sampleEveryFrames = parsed;
        }
    }
    if (const wchar_t* maxSamples = _wgetenv(L"ASTRAL_PROCESS_MEMORY_MAX_SAMPLES")) {
        if (maxSamples[0] != L'\0') {
            std::uint64_t parsed = 0;
            if (!ParseUnsigned(std::wstring_view(maxSamples), kHardMaxSamples, parsed)
                || parsed == 0) {
                error = "ASTRAL_PROCESS_MEMORY_MAX_SAMPLES must be an integer in [1, 1000000]";
                return ProcessMemoryEnvironmentStatus::Invalid;
            }
            config.maxSamples = static_cast<std::size_t>(parsed);
        }
    }
#else
    const char* output = std::getenv("ASTRAL_PROCESS_MEMORY_CSV");
    if (output == nullptr || output[0] == '\0') {
        return ProcessMemoryEnvironmentStatus::NotRequested;
    }
    error = "ASTRAL_PROCESS_MEMORY_CSV requests a Windows process-memory sampler on a non-Windows build";
    return ProcessMemoryEnvironmentStatus::Invalid;
#endif

    if (!Configure(config, error)) {
        return ProcessMemoryEnvironmentStatus::Invalid;
    }
    return ProcessMemoryEnvironmentStatus::Enabled;
}

bool ProcessMemoryCapture::Enabled() const noexcept {
    return enabled_;
}

bool ProcessMemoryCapture::ShouldSample(std::uint64_t frameIndex) const noexcept {
    if (frameIndex < config_.warmupFrames) return false;
    return ((frameIndex - config_.warmupFrames) % config_.sampleEveryFrames) == 0;
}

bool ProcessMemoryCapture::RecordSample(const ProcessMemorySample& sample) noexcept {
    if (!enabled_ || flushed_) return false;
    if (!ShouldSample(sample.frameIndex)) return true;
    if (sample.peakWorkingSetBytes < sample.workingSetBytes) {
        invalidSampleObserved_ = true;
        return false;
    }
    if (samples_.size() >= config_.maxSamples) {
        saturated_ = true;
        return true;
    }
    samples_.push_back(sample);
    return true;
}

bool ProcessMemoryCapture::RecordCurrentProcess(std::uint64_t frameIndex) noexcept {
    if (!enabled_ || flushed_) return false;
    if (!ShouldSample(frameIndex)) return true;
    if (samples_.size() >= config_.maxSamples) {
        saturated_ = true;
        return true;
    }
    if (config_.sampleSource != ProcessMemorySampleSource::WindowsProcessCounters) {
        samplingFailureObserved_ = true;
        return false;
    }
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb = sizeof(counters);
    if (!GetProcessMemoryInfo(GetCurrentProcess(),
            reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters), sizeof(counters))) {
        samplingFailureObserved_ = true;
        return false;
    }
    ProcessMemorySample sample;
    sample.frameIndex = frameIndex;
    sample.workingSetBytes = static_cast<std::uint64_t>(counters.WorkingSetSize);
    sample.peakWorkingSetBytes = static_cast<std::uint64_t>(counters.PeakWorkingSetSize);
    sample.privateUsageBytes = static_cast<std::uint64_t>(counters.PrivateUsage);
    sample.pageFaultCount = static_cast<std::uint64_t>(counters.PageFaultCount);
    return RecordSample(sample);
#else
    (void)frameIndex;
    samplingFailureObserved_ = true;
    return false;
#endif
}

bool ProcessMemoryCapture::Flush(std::string& error) {
    error.clear();
    if (!enabled_) {
        error = "process memory capture is not enabled";
        return false;
    }
    if (flushed_) {
        error = "process memory capture has already been flushed";
        return false;
    }
    if (invalidSampleObserved_) {
        error = "process memory capture observed an invalid sample";
        return false;
    }
    if (samplingFailureObserved_) {
        error = "process memory capture observed a sampler failure";
        return false;
    }
    if (samples_.empty()) {
        error = "process memory capture contains no post-warmup samples";
        return false;
    }

    std::error_code ec;
    if (std::filesystem::exists(config_.outputPath, ec) || ec) {
        error = "process memory output appeared before flush; refusing to overwrite";
        return false;
    }
    const auto partialPath = PartialOutputPath(config_.outputPath);
    ec.clear();
    if (std::filesystem::exists(partialPath, ec) || ec) {
        error = "process memory partial output appeared before flush; refusing to overwrite";
        return false;
    }

    std::ofstream stream(partialPath, std::ios::out | std::ios::trunc);
    if (!stream) {
        error = "could not create process memory partial output";
        return false;
    }
    stream << "# astral_process_memory_schema=1\n";
    stream << "# metric=process_os_memory_counters\n";
    stream << "# units=bytes_except_page_fault_count\n";
    stream << "# sample_source=" << SourceName(config_.sampleSource) << "\n";
    stream << "# memory_scope=current_process\n";
    stream << "# working_set_semantics=resident_working_set_bytes\n";
    stream << "# private_usage_semantics=process_commit_charge_bytes\n";
    stream << "# allocator_attribution=unavailable\n";
    stream << "# vram=unavailable\n";
    stream << "# leak_detection=not_established\n";
    stream << "# performance_budget_claim=none\n";
    stream << "# acceptance_claim=none\n";
    stream << "# warmup_frames=" << config_.warmupFrames << "\n";
    stream << "# sample_every_frames=" << config_.sampleEveryFrames << "\n";
    stream << "# max_samples=" << config_.maxSamples << "\n";
    stream << "# samples_saturated=" << (saturated_ ? 1 : 0) << "\n";
    stream << "frame_index,working_set_bytes,peak_working_set_bytes,private_usage_bytes,page_fault_count\n";
    for (const auto& sample : samples_) {
        stream << sample.frameIndex << ',' << sample.workingSetBytes << ','
               << sample.peakWorkingSetBytes << ',' << sample.privateUsageBytes << ','
               << sample.pageFaultCount << '\n';
    }
    stream.flush();
    if (!stream) {
        error = "failed while writing process memory partial output";
        stream.close();
        return false;
    }
    stream.close();
    if (!stream) {
        error = "failed while closing process memory partial output";
        return false;
    }

    std::filesystem::create_hard_link(partialPath, config_.outputPath, ec);
    if (ec) {
        error = "could not publish process memory output without overwrite: " + ec.message();
        return false;
    }
    std::error_code removeError;
    std::filesystem::remove(partialPath, removeError);
    if (removeError) {
        error = "process memory output was published but partial cleanup failed: "
            + removeError.message();
        return false;
    }

    flushed_ = true;
    return true;
}

std::size_t ProcessMemoryCapture::SampleCount() const noexcept {
    return samples_.size();
}

bool ProcessMemoryCapture::Saturated() const noexcept {
    return saturated_;
}

bool ProcessMemoryCapture::InvalidSampleObserved() const noexcept {
    return invalidSampleObserved_;
}

bool ProcessMemoryCapture::SamplingFailureObserved() const noexcept {
    return samplingFailureObserved_;
}

} // namespace Astral::Core
