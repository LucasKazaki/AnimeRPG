#pragma once

#include "Engine/Core/BenchmarkRunControl.h"
#include "Engine/Core/FramePhaseTimingCapture.h"
#include "Engine/Core/FrameTimingCapture.h"
#include "Engine/Core/ProcessMemoryCapture.h"

#include <string>

namespace Astral::Core {

enum class ProfilingCaptureStateReceiptStatus {
    NotRequested,
    Written,
    Invalid,
};

class ProfilingCaptureStateReceipt {
public:
    static ProfilingCaptureStateReceiptStatus WriteFromEnvironment(
        const BenchmarkRunControl& benchmarkControl,
        FrameTimingEnvironmentStatus frameTimingStatus,
        const FrameTimingCaptureConfig& frameTimingConfig,
        FramePhaseTimingEnvironmentStatus phaseTimingStatus,
        const FramePhaseTimingCaptureConfig& phaseTimingConfig,
        ProcessMemoryEnvironmentStatus processMemoryStatus,
        const ProcessMemoryCaptureConfig& processMemoryConfig,
        std::string& error);
};

} // namespace Astral::Core
