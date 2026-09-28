#pragma once

// Animation state machine and runtime animator (the role of UE Animation
// Blueprints / Unity Animator Controllers): named parameters (float, bool,
// trigger), states that play a clip or a 1D blend space with synchronised
// phase, conditional transitions with crossfades and exit times, notify events,
// root motion, and a masked upper-body overlay layer for actions that play over
// locomotion.

#include "Engine/Animation/Skeleton.h"

#include <string>
#include <vector>

namespace Astral::Animation {

struct BlendSample {
    float value{};
    const AnimationClip* clip{};
};

struct AnimState {
    std::string name;
    const AnimationClip* clip{};           // used when blendSamples is empty
    std::vector<BlendSample> blendSamples; // sorted by value
    std::string blendParameter;
    float speed{1.0f};
};

enum class ConditionOp : std::uint8_t { Greater, Less, IsTrue, IsFalse, Triggered };

struct Condition {
    std::string parameter;
    ConditionOp op{ConditionOp::IsTrue};
    float value{};
};

struct Transition {
    int from{-1}; // -1 = any state
    int to{};
    float duration{0.15f};
    std::vector<Condition> conditions; // all must hold
    float exitTime{-1.0f};             // normalised time that must be reached first (<0 = none)
    bool allowSelf{false};
};

class AnimStateMachine {
public:
    int AddState(const AnimState& state);
    void AddTransition(const Transition& transition);
    int FindState(const std::string& name) const;
    const std::vector<AnimState>& States() const { return states_; }
    const std::vector<Transition>& Transitions() const { return transitions_; }
    int defaultState{0};

private:
    std::vector<AnimState> states_;
    std::vector<Transition> transitions_;
};

struct AnimEvent {
    std::string name;
    std::string state;
};

class Animator {
public:
    Animator(const Skeleton& skeleton, const AnimStateMachine& machine);

    void SetFloat(const std::string& name, float value);
    void SetBool(const std::string& name, bool value);
    void SetTrigger(const std::string& name);
    float GetFloat(const std::string& name) const;
    // Jumps straight to a state (no crossfade), e.g. on respawn.
    void ForceState(const std::string& name);

    // Plays a clip on the masked overlay layer (e.g. upper-body slash while running).
    void PlayOverlay(const AnimationClip* clip, const std::vector<float>& mask, float fadeIn = 0.08f, float fadeOut = 0.12f);
    bool OverlayActive() const { return overlayClip_ != nullptr; }

    void Update(float dt);

    const Pose& GetPose() const { return pose_; }
    const std::vector<AnimEvent>& Events() const { return events_; }
    bool IsWindowActive(const std::string& notify) const;
    Vec3 ConsumeRootMotion();
    const std::string& CurrentStateName() const;
    float NormalizedTime() const;
    bool InTransition() const { return blendDuration_ > 0.0f && blendElapsed_ < blendDuration_; }

private:
    struct Parameter {
        std::string name;
        float value{};
        bool trigger{};
        float triggerAge{};
    };
    Parameter& Param(const std::string& name);
    const Parameter* FindParam(const std::string& name) const;
    float StateDuration(int state) const;
    void SampleState(int state, float normalizedTime, Pose& out) const;
    void AdvanceState(int state, float& normalizedTime, float dt, bool collectEvents);
    bool Evaluate(const Transition& t) const;

    const Skeleton& skeleton_;
    const AnimStateMachine& machine_;
    std::vector<Parameter> parameters_;
    int current_{0};
    float currentTime_{};
    int previous_{-1};
    float previousTime_{};
    float blendElapsed_{};
    float blendDuration_{};
    Pose pose_;
    Pose scratchA_, scratchB_;
    std::vector<AnimEvent> events_;
    Vec3 rootMotion_{};
    const AnimationClip* overlayClip_{};
    std::vector<float> overlayMask_;
    float overlayTime_{};
    float overlayFadeIn_{}, overlayFadeOut_{};
};

// Two-bone analytic IK (arm/leg). Rotates `root` and `mid` so `end` reaches the
// model-space target, bending toward the pole. Blended by `weight`.
void SolveTwoBoneIK(const Skeleton& skeleton, Pose& pose, int root, int mid, int end, Vec3 targetModel,
    Vec3 poleModel, float weight = 1.0f);

} // namespace Astral::Animation
