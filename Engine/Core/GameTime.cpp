#include "Engine/Core/GameTime.h"

#include <algorithm>
#include <cmath>

namespace Astral::Core {

namespace {
float SanitizeScale(float scale) {
    if (!std::isfinite(scale) || scale < 0.0f) return 0.0f;
    return std::min(scale, 64.0f);
}
} // namespace

GameTime::GameTime(GameTimeConfig config) : config_(config) {
    if (!std::isfinite(config_.maxDeltaSeconds) || config_.maxDeltaSeconds <= 0.0f)
        config_.maxDeltaSeconds = 0.1f;
    if (!std::isfinite(config_.fixedStepSeconds) || config_.fixedStepSeconds <= 0.0f)
        config_.fixedStepSeconds = 1.0f / 60.0f;
    config_.maxFixedStepsPerFrame = std::max(1, config_.maxFixedStepsPerFrame);
    groups_.fill(1.0f);
}

void GameTime::Advance(float realDeltaSeconds) {
    if (!std::isfinite(realDeltaSeconds) || realDeltaSeconds < 0.0f) realDeltaSeconds = 0.0f;
    realDelta_ = std::min(realDeltaSeconds, config_.maxDeltaSeconds);
    realSeconds_ += realDelta_;
    ++frameIndex_;

    for (Channel& channel : channels_) {
        if (channel.rate <= 0.0f) {
            channel.current = channel.target;
            continue;
        }
        const float step = channel.rate * realDelta_;
        if (std::fabs(channel.target - channel.current) <= step) channel.current = channel.target;
        else channel.current += channel.current < channel.target ? step : -step;
    }

    float hitstopFactor = 1.0f;
    if (hitstopRemaining_ > 0.0f) {
        // Only the portion of this frame spent inside the hitstop is scaled.
        const float frozen = std::min(hitstopRemaining_, realDelta_);
        hitstopRemaining_ -= frozen;
        hitstopFactor = realDelta_ > 0.0f
            ? (frozen * hitstopScale_ + (realDelta_ - frozen)) / realDelta_ : hitstopScale_;
        if (hitstopRemaining_ <= 0.0f) {
            hitstopRemaining_ = 0.0f;
            hitstopScale_ = 1.0f;
        }
    }

    worldDelta_ = realDelta_ * GlobalScale() * hitstopFactor;
    worldSeconds_ += worldDelta_;

    accumulator_ += worldDelta_;
    fixedSteps_ = 0;
    while (accumulator_ >= config_.fixedStepSeconds) {
        accumulator_ -= config_.fixedStepSeconds;
        if (fixedSteps_ < config_.maxFixedStepsPerFrame) ++fixedSteps_;
        else ++droppedFixedSteps_;
    }
}

float GameTime::GroupDelta(int group) const {
    return worldDelta_ * GroupScale(group);
}

void GameTime::SetDilation(int channel, float targetScale, float blendSeconds) {
    if (channel < 0 || channel >= kChannelCount) return;
    Channel& c = channels_[static_cast<std::size_t>(channel)];
    c.target = SanitizeScale(targetScale);
    const float distance = std::fabs(c.target - c.current);
    c.rate = (std::isfinite(blendSeconds) && blendSeconds > 0.0f && distance > 0.0f)
        ? distance / blendSeconds : 0.0f;
    if (c.rate == 0.0f) c.current = c.target;
}

float GameTime::DilationChannel(int channel) const {
    if (channel < 0 || channel >= kChannelCount) return 1.0f;
    return channels_[static_cast<std::size_t>(channel)].current;
}

float GameTime::GlobalScale() const {
    float scale = 1.0f;
    for (const Channel& channel : channels_) scale *= channel.current;
    return scale;
}

void GameTime::SetGroupScale(int group, float scale) {
    if (group < 0 || group >= kGroupCount) return;
    groups_[static_cast<std::size_t>(group)] = SanitizeScale(scale);
}

float GameTime::GroupScale(int group) const {
    if (group < 0 || group >= kGroupCount) return 1.0f;
    return groups_[static_cast<std::size_t>(group)];
}

void GameTime::TriggerHitstop(float realSeconds, float scale) {
    if (!std::isfinite(realSeconds) || realSeconds <= 0.0f) return;
    realSeconds = std::min(realSeconds, 1.0f);
    const float sanitized = std::min(SanitizeScale(scale), 1.0f);
    if (hitstopRemaining_ > 0.0f) {
        hitstopScale_ = std::min(hitstopScale_, sanitized);
        hitstopRemaining_ = std::max(hitstopRemaining_, realSeconds);
    } else {
        hitstopScale_ = sanitized;
        hitstopRemaining_ = realSeconds;
    }
}

float GameTime::InterpolationAlpha() const {
    return static_cast<float>(accumulator_ / config_.fixedStepSeconds);
}

} // namespace Astral::Core
