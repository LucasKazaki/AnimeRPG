#include "Engine/Core/ProfilingCaptureStateReceipt.h"

#include <cstdlib>
#include <fstream>
#include <string_view>
#include <system_error>

namespace {

std::filesystem::path PartialOutputPath(const std::filesystem::path& finalPath) {
    auto partial = finalPath;
    partial += ".partial";
    return partial;
}

std::string JsonEscape(std::string_view value) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.reserve(value.size());
    for (const unsigned char ch : value) {
        switch (ch) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (ch < 0x20) {
                out += "\\u00";
                out += kHex[(ch >> 4) & 0x0f];
                out += kHex[ch & 0x0f];
            } else {
                out.push_back(static_cast<char>(ch));
            }
            break;
        }
    }
    return out;
}

bool SafeRelativeOutput(
    const std::filesystem::path& receiptPath,
    const std::filesystem::path& outputPath,
    std::string& relative,
    std::string& error) {
    if (outputPath.empty() || !outputPath.is_absolute()) {
        error = "enabled profiling capture output paths must be absolute";
        return false;
    }

    std::error_code ec;
    const auto receiptRoot = std::filesystem::weakly_canonical(receiptPath.parent_path(), ec);
    if (ec) {
        error = "could not resolve profiling capture-state receipt parent";
        return false;
    }
    ec.clear();
    const auto outputParent = std::filesystem::weakly_canonical(outputPath.parent_path(), ec);
    if (ec) {
        error = "could not resolve profiling capture output parent";
        return false;
    }
    const auto normalizedOutput = outputParent / outputPath.filename();
    ec.clear();
    const auto rel = std::filesystem::relative(normalizedOutput, receiptRoot, ec);
    if (ec || rel.empty() || rel.is_absolute()) {
        error = "profiling capture output is not relative to the receipt evidence root";
        return false;
    }
    for (const auto& part : rel) {
        if (part == "." || part == "..") {
            error = "profiling capture output escapes the receipt evidence root";
            return false;
        }
    }
    const auto receiptNormalized = receiptRoot / receiptPath.filename();
    if (normalizedOutput == receiptNormalized || normalizedOutput == PartialOutputPath(receiptNormalized)) {
        error = "profiling capture output conflicts with the capture-state receipt path";
        return false;
    }

    try {
        relative = rel.generic_u8string();
    } catch (const std::filesystem::filesystem_error&) {
        error = "profiling capture output path could not be encoded for the receipt";
        return false;
    }
    if (relative.empty()) {
        error = "profiling capture output relative path is empty";
        return false;
    }
    return true;
}

const char* ProcessMemorySourceName(Astral::Core::ProcessMemorySampleSource source) {
    switch (source) {
    case Astral::Core::ProcessMemorySampleSource::CallerSupplied:
        return "caller_supplied_contract_sample";
    case Astral::Core::ProcessMemorySampleSource::WindowsProcessCounters:
        return "windows_process_counters";
    }
    return "unknown";
}

template <typename Status>
bool StatusInvalid(Status status) {
    return status == Status::Invalid;
}

template <typename Status>
bool StatusEnabled(Status status) {
    return status == Status::Enabled;
}

void WriteFrameStream(
    std::ostream& stream,
    const char* key,
    bool enabled,
    const std::string& output,
    std::uint64_t warmup,
    std::size_t maxSamples,
    bool comma) {
    stream << "    \"" << key << "\": {\n"
           << "      \"state\": \"" << (enabled ? "enabled" : "not_requested") << "\",\n";
    if (enabled) {
        stream << "      \"output_path\": \"" << JsonEscape(output) << "\",\n"
               << "      \"warmup_frames\": " << warmup << ",\n"
               << "      \"max_samples\": " << maxSamples << "\n";
    } else {
        stream << "      \"output_path\": null,\n"
               << "      \"warmup_frames\": null,\n"
               << "      \"max_samples\": null\n";
    }
    stream << "    }" << (comma ? "," : "") << "\n";
}

void WriteMemoryStream(
    std::ostream& stream,
    bool enabled,
    const std::string& output,
    const Astral::Core::ProcessMemoryCaptureConfig& config) {
    stream << "    \"process_memory\": {\n"
           << "      \"state\": \"" << (enabled ? "enabled" : "not_requested") << "\",\n";
    if (enabled) {
        stream << "      \"output_path\": \"" << JsonEscape(output) << "\",\n"
               << "      \"warmup_frames\": " << config.warmupFrames << ",\n"
               << "      \"sample_every_frames\": " << config.sampleEveryFrames << ",\n"
               << "      \"max_samples\": " << config.maxSamples << ",\n"
               << "      \"sample_source\": \"" << ProcessMemorySourceName(config.sampleSource) << "\"\n";
    } else {
        stream << "      \"output_path\": null,\n"
               << "      \"warmup_frames\": null,\n"
               << "      \"sample_every_frames\": null,\n"
               << "      \"max_samples\": null,\n"
               << "      \"sample_source\": null\n";
    }
    stream << "    }\n";
}

} // namespace

namespace Astral::Core {

ProfilingCaptureStateReceiptStatus ProfilingCaptureStateReceipt::WriteFromEnvironment(
    const BenchmarkRunControl& benchmarkControl,
    FrameTimingEnvironmentStatus frameTimingStatus,
    const FrameTimingCaptureConfig& frameTimingConfig,
    FramePhaseTimingEnvironmentStatus phaseTimingStatus,
    const FramePhaseTimingCaptureConfig& phaseTimingConfig,
    ProcessMemoryEnvironmentStatus processMemoryStatus,
    const ProcessMemoryCaptureConfig& processMemoryConfig,
    std::string& error) {
    error.clear();

#ifdef _WIN32
    const wchar_t* requestedPath = _wgetenv(L"ASTRAL_PROFILING_CAPTURE_STATE_JSON");
    if (requestedPath == nullptr) {
        return ProfilingCaptureStateReceiptStatus::NotRequested;
    }
    if (requestedPath[0] == L'\0') {
        error = "ASTRAL_PROFILING_CAPTURE_STATE_JSON must not be empty";
        return ProfilingCaptureStateReceiptStatus::Invalid;
    }
    const std::filesystem::path receiptPath(requestedPath);
#else
    const char* requestedPath = std::getenv("ASTRAL_PROFILING_CAPTURE_STATE_JSON");
    if (requestedPath == nullptr) {
        return ProfilingCaptureStateReceiptStatus::NotRequested;
    }
    if (requestedPath[0] == '\0') {
        error = "ASTRAL_PROFILING_CAPTURE_STATE_JSON must not be empty";
        return ProfilingCaptureStateReceiptStatus::Invalid;
    }
    const std::filesystem::path receiptPath = std::filesystem::u8path(requestedPath);
#endif

    if (!benchmarkControl.Enabled()) {
        error = "profiling capture-state receipt is only valid for an enabled benchmark run";
        return ProfilingCaptureStateReceiptStatus::Invalid;
    }
    if (StatusInvalid(frameTimingStatus) || StatusInvalid(phaseTimingStatus)
        || StatusInvalid(processMemoryStatus)) {
        error = "profiling capture-state receipt cannot certify an invalid capture configuration";
        return ProfilingCaptureStateReceiptStatus::Invalid;
    }
    if (receiptPath.empty() || !receiptPath.is_absolute()) {
        error = "profiling capture-state receipt path must be absolute";
        return ProfilingCaptureStateReceiptStatus::Invalid;
    }

    std::error_code ec;
    const auto parent = receiptPath.parent_path();
    if (parent.empty() || !std::filesystem::exists(parent, ec) || ec
        || !std::filesystem::is_directory(parent, ec) || ec) {
        error = "profiling capture-state receipt parent must be an existing directory";
        return ProfilingCaptureStateReceiptStatus::Invalid;
    }
    if (std::filesystem::exists(receiptPath, ec) || ec) {
        error = "profiling capture-state receipt already exists or could not be inspected";
        return ProfilingCaptureStateReceiptStatus::Invalid;
    }
    const auto partialPath = PartialOutputPath(receiptPath);
    ec.clear();
    if (std::filesystem::exists(partialPath, ec) || ec) {
        error = "profiling capture-state partial receipt already exists or could not be inspected";
        return ProfilingCaptureStateReceiptStatus::Invalid;
    }

    const bool frameEnabled = StatusEnabled(frameTimingStatus);
    const bool phaseEnabled = StatusEnabled(phaseTimingStatus);
    const bool memoryEnabled = StatusEnabled(processMemoryStatus);
    std::string frameOutput;
    std::string phaseOutput;
    std::string memoryOutput;

    if (frameEnabled) {
        if (frameTimingConfig.warmupFrames != benchmarkControl.WarmupFrames()
            || frameTimingConfig.maxSamples < benchmarkControl.MeasuredFrames()
            || frameTimingConfig.maxSamples > FrameTimingCapture::kHardMaxSamples) {
            error = "whole-frame timing configuration is not coherent with benchmark run control";
            return ProfilingCaptureStateReceiptStatus::Invalid;
        }
        if (!SafeRelativeOutput(receiptPath, frameTimingConfig.outputPath, frameOutput, error)) {
            return ProfilingCaptureStateReceiptStatus::Invalid;
        }
    }
    if (phaseEnabled) {
        if (phaseTimingConfig.warmupFrames != benchmarkControl.WarmupFrames()
            || phaseTimingConfig.maxSamples < benchmarkControl.MeasuredFrames()
            || phaseTimingConfig.maxSamples > FramePhaseTimingCapture::kHardMaxSamples) {
            error = "phase timing configuration is not coherent with benchmark run control";
            return ProfilingCaptureStateReceiptStatus::Invalid;
        }
        if (!SafeRelativeOutput(receiptPath, phaseTimingConfig.outputPath, phaseOutput, error)) {
            return ProfilingCaptureStateReceiptStatus::Invalid;
        }
    }
    if (memoryEnabled) {
        if (processMemoryConfig.warmupFrames != benchmarkControl.WarmupFrames()
            || processMemoryConfig.sampleEveryFrames == 0
            || processMemoryConfig.sampleEveryFrames > ProcessMemoryCapture::kHardMaxSampleEveryFrames
            || processMemoryConfig.maxSamples == 0
            || processMemoryConfig.maxSamples > ProcessMemoryCapture::kHardMaxSamples) {
            error = "process-memory capture configuration is not coherent with benchmark run control";
            return ProfilingCaptureStateReceiptStatus::Invalid;
        }
        const auto measured = benchmarkControl.MeasuredFrames();
        const auto requiredSamples =
            (measured + processMemoryConfig.sampleEveryFrames - 1)
            / processMemoryConfig.sampleEveryFrames;
        if (processMemoryConfig.maxSamples < requiredSamples) {
            error = "process-memory capture maxSamples cannot cover the measured benchmark interval";
            return ProfilingCaptureStateReceiptStatus::Invalid;
        }
        if (!SafeRelativeOutput(receiptPath, processMemoryConfig.outputPath, memoryOutput, error)) {
            return ProfilingCaptureStateReceiptStatus::Invalid;
        }
    }

    if ((frameEnabled && phaseEnabled && frameOutput == phaseOutput)
        || (frameEnabled && memoryEnabled && frameOutput == memoryOutput)
        || (phaseEnabled && memoryEnabled && phaseOutput == memoryOutput)) {
        error = "profiling capture streams must use distinct output paths";
        return ProfilingCaptureStateReceiptStatus::Invalid;
    }

    std::ofstream stream(partialPath, std::ios::out | std::ios::trunc);
    if (!stream) {
        error = "could not create profiling capture-state partial receipt";
        return ProfilingCaptureStateReceiptStatus::Invalid;
    }

    stream
        << "{\n"
        << "  \"schema_version\": 1,\n"
        << "  \"capture_state_semantics\": \"post_configuration_pre_frame_loop\",\n"
        << "  \"environment_scope\": \"known_astral_profiling_controls_only\",\n"
        << "  \"raw_environment_dumped\": false,\n"
        << "  \"run_control\": {\n"
        << "    \"simulation_fixed_hz\": " << benchmarkControl.SimulationFixedHz() << ",\n"
        << "    \"warmup_frames\": " << benchmarkControl.WarmupFrames() << ",\n"
        << "    \"measured_frames\": " << benchmarkControl.MeasuredFrames() << ",\n"
        << "    \"total_frames\": " << benchmarkControl.TotalFrames() << ",\n"
        << "    \"client_width_px\": " << benchmarkControl.ClientWidthPx() << ",\n"
        << "    \"client_height_px\": " << benchmarkControl.ClientHeightPx() << ",\n"
        << "    \"window_mode\": \"windowed\",\n"
        << "    \"vsync_requested\": false,\n"
        << "    \"presentation_backend\": \"win32_gdi_window_dc\",\n"
        << "    \"vsync_control\": \"unavailable_in_gdi_path\",\n"
        << "    \"frame_pacing\": \"sleep_1ms_not_refresh_locked\",\n"
        << "    \"live_input\": \"suppressed\",\n"
        << "    \"termination\": \"exact_frame_limit\"\n"
        << "  },\n"
        << "  \"streams\": {\n";
    WriteFrameStream(stream, "cpu_frame_timing", frameEnabled, frameOutput,
        frameTimingConfig.warmupFrames, frameTimingConfig.maxSamples, true);
    WriteFrameStream(stream, "cpu_phase_timing", phaseEnabled, phaseOutput,
        phaseTimingConfig.warmupFrames, phaseTimingConfig.maxSamples, true);
    WriteMemoryStream(stream, memoryEnabled, memoryOutput, processMemoryConfig);
    stream
        << "  },\n"
        << "  \"claim_boundaries\": {\n"
        << "    \"instrumentation_overhead_verified\": false,\n"
        << "    \"performance_budget_verified\": false,\n"
        << "    \"gpu_timing_verified\": false,\n"
        << "    \"comparative_parity_verified\": false,\n"
        << "    \"independent_acceptance\": false\n"
        << "  }\n"
        << "}\n";
    stream.flush();
    if (!stream) {
        error = "failed while writing profiling capture-state partial receipt";
        stream.close();
        return ProfilingCaptureStateReceiptStatus::Invalid;
    }
    stream.close();
    if (!stream) {
        error = "failed while closing profiling capture-state partial receipt";
        return ProfilingCaptureStateReceiptStatus::Invalid;
    }

    ec.clear();
    std::filesystem::create_hard_link(partialPath, receiptPath, ec);
    if (ec) {
        error = "could not publish profiling capture-state receipt without overwrite: " + ec.message();
        return ProfilingCaptureStateReceiptStatus::Invalid;
    }
    std::error_code removeError;
    std::filesystem::remove(partialPath, removeError);
    if (removeError) {
        error = "profiling capture-state receipt was published but partial cleanup failed: "
            + removeError.message();
        return ProfilingCaptureStateReceiptStatus::Invalid;
    }

    return ProfilingCaptureStateReceiptStatus::Written;
}

} // namespace Astral::Core
