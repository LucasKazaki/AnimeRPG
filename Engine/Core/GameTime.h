#pragma once

// Layered game time: real time, dilated world time, per-group scales, hitstop
// and a fixed-step accumulator. This is the engine facility behind the game's
// Thought Focus slow-time (a dilation channel), anime hit-freeze on heavy
// impacts (hitstop), "witch time" style exemptions (group scales) and stable
// physics (fixed steps). Channel blends advance in real time, so a slow-motion
// transition is never slowed down by itself.

#include <array>
#include <cstdint>

namespace Astral::Core {

struct GameTimeConfig {
    float maxDeltaSeconds{0.1f};        // clamps hitches (breakpoints, window drags)
    float fixedStepSeconds{1.0f / 60.0f};
    int maxFixedStepsPerFrame{8};       // spiral-of-death guard
};

class GameTime {
public:
    static constexpr int kChannelCount = 8;
    static constexpr int kGroupCount = 8;

    explicit GameTime(GameTimeConfig config = {});

    // Advances all clocks by one real frame. Non-finite or negative input is treated as 0.
    void Advance(float realDeltaSeconds);

    float RealDelta() const { return realDelta_; }
    float WorldDelta() const { return worldDelta_; }
    // World delta times the group's scale (group 0 is the default world group).
    float GroupDelta(int group) const;
    double RealSeconds() const { return realSeconds_; }
    double WorldSeconds() const { return worldSeconds_; }
    std::uint64_t FrameIndex() const { return frameIndex_; }

    // Dilation channels multiply together; each blends linearly toward its target.
    void SetDilation(int channel, float targetScale, float blendSeconds);
    float DilationChannel(int channel) const;
    float GlobalScale() const;

    void SetGroupScale(int group, float scale);
    float GroupScale(int group) const;

    // Freezes (or nearly freezes) world time for a real-time duration. Overlapping
    // requests keep the longer remaining duration and the stronger freeze.
    void TriggerHitstop(float realSeconds, float scale = 0.0f);
    bool InHitstop() const { return hitstopRemaining_ > 0.0f; }

    int FixedStepsThisFrame() const { return fixedSteps_; }
    float FixedStepSeconds() const { return config_.fixedStepSeconds; }
    // Fraction of a fixed step left in the accumulator, for render interpolation.
    float InterpolationAlpha() const;
    std::uint64_t DroppedFixedSteps() const { return droppedFixedSteps_; }

private:
    struct Channel {
        float current{1.0f};
        float target{1.0f};
        float rate{0.0f}; // scale units per real second; 0 means snap
    };

    GameTimeConfig config_;
    std::array<Channel, kChannelCount> channels_{};
    std::array<float, kGroupCount> groups_{};
    float realDelta_{};
    float worldDelta_{};
    double realSeconds_{};
    double worldSeconds_{};
    std::uint64_t frameIndex_{};
    float hitstopRemaining_{};
    float hitstopScale_{1.0f};
    double accumulator_{};
    int fixedSteps_{};
    std::uint64_t droppedFixedSteps_{};
};

} // namespace Astral::Core
