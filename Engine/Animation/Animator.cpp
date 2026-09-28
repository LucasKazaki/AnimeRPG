#include "Engine/Animation/Animator.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace Astral::Animation {

using namespace Math;

namespace {
constexpr float kTriggerLifetime = 0.25f; // unconsumed triggers act as a short input buffer

const AnimationClip* DominantClip(const AnimState& state, float blendValue) {
    if (state.blendSamples.empty()) return state.clip;
    const BlendSample* best = &state.blendSamples.front();
    float bestDistance = std::fabs(blendValue - best->value);
    for (const BlendSample& sample : state.blendSamples) {
        const float d = std::fabs(blendValue - sample.value);
        if (d < bestDistance) {
            bestDistance = d;
            best = &sample;
        }
    }
    return best->clip;
}

// Segment of a 1D blend space: indices i, j and weight toward j.
void BlendSegment(const AnimState& state, float value, std::size_t& i, std::size_t& j, float& w) {
    const auto& samples = state.blendSamples;
    i = j = 0;
    w = 0.0f;
    if (samples.size() == 1 || value <= samples.front().value) return;
    if (value >= samples.back().value) {
        i = j = samples.size() - 1;
        return;
    }
    for (std::size_t k = 0; k + 1 < samples.size(); ++k) {
        if (value >= samples[k].value && value <= samples[k + 1].value) {
            i = k;
            j = k + 1;
            const float span = samples[j].value - samples[i].value;
            w = span > 1.0e-6f ? (value - samples[i].value) / span : 0.0f;
            return;
        }
    }
}
} // namespace

int AnimStateMachine::AddState(const AnimState& state) {
    states_.push_back(state);
    std::sort(states_.back().blendSamples.begin(), states_.back().blendSamples.end(),
        [](const BlendSample& a, const BlendSample& b) { return a.value < b.value; });
    return static_cast<int>(states_.size() - 1);
}

void AnimStateMachine::AddTransition(const Transition& transition) { transitions_.push_back(transition); }

int AnimStateMachine::FindState(const std::string& name) const {
    for (std::size_t i = 0; i < states_.size(); ++i)
        if (states_[i].name == name) return static_cast<int>(i);
    return -1;
}

Animator::Animator(const Skeleton& skeleton, const AnimStateMachine& machine)
    : skeleton_(skeleton), machine_(machine) {
    current_ = std::clamp(machine.defaultState, 0, std::max(0, static_cast<int>(machine.States().size()) - 1));
    pose_ = Pose::Bind(skeleton);
    if (!machine.States().empty()) SampleState(current_, 0.0f, pose_);
}

Animator::Parameter& Animator::Param(const std::string& name) {
    for (Parameter& p : parameters_)
        if (p.name == name) return p;
    parameters_.push_back({name, 0.0f, false, 0.0f});
    return parameters_.back();
}

const Animator::Parameter* Animator::FindParam(const std::string& name) const {
    for (const Parameter& p : parameters_)
        if (p.name == name) return &p;
    return nullptr;
}

void Animator::SetFloat(const std::string& name, float value) { Param(name).value = std::isfinite(value) ? value : 0.0f; }
void Animator::SetBool(const std::string& name, bool value) { Param(name).value = value ? 1.0f : 0.0f; }
void Animator::SetTrigger(const std::string& name) {
    Parameter& p = Param(name);
    p.trigger = true;
    p.triggerAge = 0.0f;
}
float Animator::GetFloat(const std::string& name) const {
    const Parameter* p = FindParam(name);
    return p ? p->value : 0.0f;
}

void Animator::ForceState(const std::string& name) {
    const int state = machine_.FindState(name);
    if (state < 0) return;
    current_ = state;
    currentTime_ = 0.0f;
    previous_ = -1;
    blendDuration_ = blendElapsed_ = 0.0f;
}

void Animator::PlayOverlay(const AnimationClip* clip, const std::vector<float>& mask, float fadeIn, float fadeOut) {
    overlayClip_ = clip;
    overlayMask_ = mask;
    overlayTime_ = 0.0f;
    overlayFadeIn_ = std::max(0.0f, fadeIn);
    overlayFadeOut_ = std::max(0.0f, fadeOut);
}

float Animator::StateDuration(int state) const {
    const AnimState& s = machine_.States()[static_cast<std::size_t>(state)];
    if (s.blendSamples.empty()) return s.clip ? std::max(1.0e-3f, s.clip->duration) : 1.0f;
    std::size_t i, j;
    float w;
    BlendSegment(s, GetFloat(s.blendParameter), i, j, w);
    return std::max(1.0e-3f, Lerp(s.blendSamples[i].clip->duration, s.blendSamples[j].clip->duration, w));
}

void Animator::SampleState(int state, float normalizedTime, Pose& out) const {
    out = Pose::Bind(skeleton_);
    const AnimState& s = machine_.States()[static_cast<std::size_t>(state)];
    if (s.blendSamples.empty()) {
        if (s.clip) s.clip->Sample(normalizedTime * s.clip->duration, out);
        return;
    }
    std::size_t i, j;
    float w;
    BlendSegment(s, GetFloat(s.blendParameter), i, j, w);
    // Phase-synchronised: both clips sample the same normalised time (feet stay in step).
    const AnimationClip* a = s.blendSamples[i].clip;
    const AnimationClip* b = s.blendSamples[j].clip;
    a->Sample(normalizedTime * a->duration, out);
    if (i == j || w <= 0.0f) return;
    Pose other = Pose::Bind(skeleton_);
    b->Sample(normalizedTime * b->duration, other);
    BlendPoses(out, other, w, out);
}

void Animator::AdvanceState(int state, float& normalizedTime, float dt, bool collectEvents) {
    const AnimState& s = machine_.States()[static_cast<std::size_t>(state)];
    const float duration = StateDuration(state);
    const float before = normalizedTime;
    normalizedTime += dt * s.speed / duration;
    const AnimationClip* clip = DominantClip(s, GetFloat(s.blendParameter));
    if (clip && !clip->looping) normalizedTime = std::min(normalizedTime, 1.0f);
    if (!collectEvents || !clip) return;
    std::vector<const AnimNotify*> fired;
    clip->FiredNotifies(before * clip->duration, normalizedTime * clip->duration, fired);
    // A non-looping clip fires its time-zero notifies on the first update.
    if (before == 0.0f && normalizedTime > 0.0f) {
        for (const AnimNotify& n : clip->notifies)
            if (n.time == 0.0f) fired.push_back(&n);
    }
    for (const AnimNotify* n : fired) events_.push_back({n->name, s.name});
    rootMotion_ += clip->RootMotion(before * clip->duration, normalizedTime * clip->duration);
}

bool Animator::Evaluate(const Transition& t) const {
    for (const Condition& c : t.conditions) {
        const Parameter* p = FindParam(c.parameter);
        const float value = p ? p->value : 0.0f;
        switch (c.op) {
        case ConditionOp::Greater: if (!(value > c.value)) return false; break;
        case ConditionOp::Less: if (!(value < c.value)) return false; break;
        case ConditionOp::IsTrue: if (value == 0.0f) return false; break;
        case ConditionOp::IsFalse: if (value != 0.0f) return false; break;
        case ConditionOp::Triggered: if (!p || !p->trigger) return false; break;
        }
    }
    return true;
}

void Animator::Update(float dt) {
    events_.clear();
    if (machine_.States().empty()) return;
    if (!std::isfinite(dt) || dt < 0.0f) dt = 0.0f;
    AdvanceState(current_, currentTime_, dt, true);
    if (previous_ >= 0 && InTransition()) AdvanceState(previous_, previousTime_, dt, false);
    blendElapsed_ += dt;

    for (const Transition& t : machine_.Transitions()) {
        if (t.from != -1 && t.from != current_) continue;
        if (t.to == current_ && !t.allowSelf) continue;
        if (t.to < 0 || t.to >= static_cast<int>(machine_.States().size())) continue;
        if (t.exitTime >= 0.0f && currentTime_ < t.exitTime) continue;
        if (!Evaluate(t)) continue;
        for (const Condition& c : t.conditions) {
            if (c.op == ConditionOp::Triggered) Param(c.parameter).trigger = false;
        }
        previous_ = current_;
        previousTime_ = currentTime_;
        current_ = t.to;
        currentTime_ = 0.0f;
        blendDuration_ = std::max(0.0f, t.duration);
        blendElapsed_ = 0.0f;
        events_.push_back({"StateEnter", machine_.States()[static_cast<std::size_t>(current_)].name});
        AdvanceState(current_, currentTime_, 0.0f, true);
        break;
    }
    for (Parameter& p : parameters_) {
        if (!p.trigger) continue;
        p.triggerAge += dt;
        if (p.triggerAge > kTriggerLifetime) p.trigger = false;
    }

    if (InTransition() && previous_ >= 0) {
        SampleState(previous_, previousTime_, scratchA_);
        SampleState(current_, currentTime_, scratchB_);
        BlendPoses(scratchA_, scratchB_, SmoothStep(0.0f, 1.0f, blendElapsed_ / blendDuration_), pose_);
    } else {
        previous_ = -1;
        SampleState(current_, currentTime_, pose_);
    }

    if (overlayClip_) {
        const float before = overlayTime_;
        overlayTime_ += dt;
        std::vector<const AnimNotify*> fired;
        overlayClip_->FiredNotifies(before, overlayTime_, fired);
        for (const AnimNotify* n : fired) events_.push_back({n->name, "Overlay"});
        const float duration = overlayClip_->duration;
        float weight = 1.0f;
        if (overlayFadeIn_ > 0.0f) weight = std::min(weight, overlayTime_ / overlayFadeIn_);
        if (overlayFadeOut_ > 0.0f) weight = std::min(weight, (duration - overlayTime_) / overlayFadeOut_);
        weight = Saturate(weight);
        scratchA_ = Pose::Bind(skeleton_);
        overlayClip_->Sample(std::min(overlayTime_, duration), scratchA_);
        BlendPosesMasked(pose_, scratchA_, overlayMask_, weight, pose_);
        if (overlayTime_ >= duration) overlayClip_ = nullptr;
    }
}

bool Animator::IsWindowActive(const std::string& notify) const {
    if (overlayClip_ && overlayClip_->IsWindowActive(notify, overlayTime_)) return true;
    const AnimState& s = machine_.States()[static_cast<std::size_t>(current_)];
    const AnimationClip* clip = DominantClip(s, GetFloat(s.blendParameter));
    return clip && clip->IsWindowActive(notify, currentTime_ * clip->duration);
}

Vec3 Animator::ConsumeRootMotion() {
    const Vec3 motion = rootMotion_;
    rootMotion_ = {};
    return motion;
}

const std::string& Animator::CurrentStateName() const {
    static const std::string empty;
    return machine_.States().empty() ? empty : machine_.States()[static_cast<std::size_t>(current_)].name;
}

float Animator::NormalizedTime() const { return currentTime_; }

void SolveTwoBoneIK(const Skeleton& skeleton, Pose& pose, int root, int mid, int end, Vec3 target, Vec3 pole, float weight) {
    const int count = skeleton.JointCount();
    if (root < 0 || mid < 0 || end < 0 || root >= count || mid >= count || end >= count || !IsFinite(target)) return;
    weight = Saturate(weight);
    if (weight <= 0.0f) return;
    std::vector<Mat4> model;
    pose.ToModel(skeleton, model);
    auto globalRotation = [&](int joint) {
        Quat q{};
        std::vector<int> chain;
        for (int j = joint; j >= 0; j = skeleton.GetJoint(j).parent) chain.push_back(j);
        for (auto it = chain.rbegin(); it != chain.rend(); ++it) q = q * pose.local[static_cast<std::size_t>(*it)].rotation;
        return Normalize(q);
    };
    const Vec3 a = GetTranslation(model[static_cast<std::size_t>(root)]);
    const Vec3 b = GetTranslation(model[static_cast<std::size_t>(mid)]);
    const Vec3 c = GetTranslation(model[static_cast<std::size_t>(end)]);
    const float lab = Length(b - a), lcb = Length(b - c);
    if (lab < 1.0e-5f || lcb < 1.0e-5f) return;
    const float lat = Clamp(Length(target - a), 1.0e-4f, (lab + lcb) * 0.9999f);
    auto angleBetween = [](Vec3 u, Vec3 v) { return std::acos(Clamp(Dot(Normalize(u), Normalize(v)), -1.0f, 1.0f)); };
    const float acAb0 = angleBetween(c - a, b - a);
    const float baBc0 = angleBetween(a - b, c - b);
    const float acAt0 = angleBetween(c - a, target - a);
    const float acAb1 = std::acos(Clamp((lcb * lcb - lab * lab - lat * lat) / (-2.0f * lab * lat), -1.0f, 1.0f));
    const float baBc1 = std::acos(Clamp((lat * lat - lab * lab - lcb * lcb) / (-2.0f * lab * lcb), -1.0f, 1.0f));
    Vec3 axis0 = Cross(c - a, pole - a);
    if (LengthSquared(axis0) < 1.0e-10f) axis0 = Cross(c - a, b - a);
    axis0 = Normalize(axis0, {1, 0, 0});
    Vec3 axis1 = Cross(c - a, target - a);
    axis1 = LengthSquared(axis1) > 1.0e-10f ? Normalize(axis1) : axis0;
    const Quat aGlobal = globalRotation(root);
    const Quat bGlobal = globalRotation(mid);
    const Quat originalA = pose.local[static_cast<std::size_t>(root)].rotation;
    const Quat originalB = pose.local[static_cast<std::size_t>(mid)].rotation;
    // Local-frame rotations appended on the right act in world space as
    // G * r2 * r0 = R2 * R0 * G: shape the triangle (R0) first, then align the
    // unchanged a->c direction with the target (R2).
    const Quat r0 = QuatFromAxisAngle(Rotate(Conjugate(aGlobal), axis0), acAb1 - acAb0);
    const Quat r2 = QuatFromAxisAngle(Rotate(Conjugate(aGlobal), axis1), acAt0);
    const Quat newA = originalA * r2 * r0;
    const Quat newB = originalB * QuatFromAxisAngle(Rotate(Conjugate(bGlobal), axis0), baBc1 - baBc0);
    pose.local[static_cast<std::size_t>(root)].rotation = Slerp(originalA, Normalize(newA), weight);
    pose.local[static_cast<std::size_t>(mid)].rotation = Slerp(originalB, Normalize(newB), weight);
}

} // namespace Astral::Animation
