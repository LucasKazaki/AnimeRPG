#include "Engine/Animation/Skeleton.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

namespace Astral::Animation {

using namespace Math;

int Skeleton::AddJoint(const std::string& name, int parent, const TRS& bindLocal) {
    if (parent >= static_cast<int>(joints_.size()) || parent < -1) return -1;
    joints_.push_back({name, parent, bindLocal});
    return static_cast<int>(joints_.size() - 1);
}

int Skeleton::Find(const std::string& name) const {
    for (std::size_t i = 0; i < joints_.size(); ++i)
        if (joints_[i].name == name) return static_cast<int>(i);
    return -1;
}

void Skeleton::ComputeBindMatrices() {
    bindModel_.resize(joints_.size());
    inverseBind_.resize(joints_.size());
    for (std::size_t i = 0; i < joints_.size(); ++i) {
        const Mat4 local = ToMat4(joints_[i].bindLocal);
        bindModel_[i] = joints_[i].parent < 0 ? local : bindModel_[static_cast<std::size_t>(joints_[i].parent)] * local;
        if (!Inverse(bindModel_[i], inverseBind_[i])) inverseBind_[i] = Mat4::Identity();
    }
}

bool Skeleton::SetInverseBindMatrices(std::vector<Mat4> inverseBind) {
    if (inverseBind.size() != joints_.size()) return false;
    bindModel_.resize(inverseBind.size());
    for (std::size_t i = 0; i < inverseBind.size(); ++i) {
        if (!Inverse(inverseBind[i], bindModel_[i])) return false;
    }
    inverseBind_ = std::move(inverseBind);
    return true;
}

Pose Pose::Bind(const Skeleton& skeleton) {
    Pose pose;
    pose.local.reserve(static_cast<std::size_t>(skeleton.JointCount()));
    for (const Joint& joint : skeleton.Joints()) pose.local.push_back(joint.bindLocal);
    return pose;
}

void Pose::ToModel(const Skeleton& skeleton, std::vector<Mat4>& model) const {
    model.resize(local.size());
    for (std::size_t i = 0; i < local.size(); ++i) {
        const int parent = static_cast<int>(i) < skeleton.JointCount() ? skeleton.GetJoint(static_cast<int>(i)).parent : -1;
        const Mat4 m = ToMat4(local[i]);
        model[i] = parent < 0 ? m : model[static_cast<std::size_t>(parent)] * m;
    }
}

void BlendPoses(const Pose& a, const Pose& b, float weight, Pose& out) {
    weight = Saturate(weight);
    const std::size_t count = std::min(a.local.size(), b.local.size());
    out.local.resize(count);
    for (std::size_t i = 0; i < count; ++i) out.local[i] = Lerp(a.local[i], b.local[i], weight);
}

void BlendPosesMasked(const Pose& base, const Pose& layer, const std::vector<float>& mask, float weight, Pose& out) {
    const std::size_t count = std::min(base.local.size(), layer.local.size());
    out.local.resize(count);
    for (std::size_t i = 0; i < count; ++i) {
        const float w = Saturate(weight * (i < mask.size() ? mask[i] : 0.0f));
        out.local[i] = w > 0.0f ? Lerp(base.local[i], layer.local[i], w) : base.local[i];
    }
}

void AddPose(const Pose& base, const Pose& additive, const Pose& reference, float weight, Pose& out) {
    const std::size_t count = std::min(base.local.size(), std::min(additive.local.size(), reference.local.size()));
    out.local.resize(count);
    for (std::size_t i = 0; i < count; ++i) {
        const TRS& b = base.local[i];
        const TRS& a = additive.local[i];
        const TRS& r = reference.local[i];
        TRS result;
        result.translation = b.translation + (a.translation - r.translation) * weight;
        const Quat delta = Normalize(a.rotation * Conjugate(r.rotation));
        result.rotation = Normalize(Slerp(Quat{}, delta, weight) * b.rotation);
        result.scale = b.scale + (a.scale - r.scale) * weight;
        out.local[i] = result;
    }
}

void AnimationClip::AddKey(int joint, float time, const TRS& value) {
    for (Track& track : tracks) {
        if (track.joint == joint) {
            track.keys.push_back({time, value});
            return;
        }
    }
    tracks.push_back({joint, {{time, value}}});
}

TRS AnimationClip::SampleTrack(const Track& track, float time) const {
    if (track.keys.empty()) return {};
    if (track.keys.size() == 1 || time <= track.keys.front().time) return track.keys.front().value;
    if (time >= track.keys.back().time) {
        if (!looping) return track.keys.back().value;
        // Wrap: blend from the last key to the first key across the loop seam.
        const float span = duration - track.keys.back().time + track.keys.front().time;
        if (span <= 1.0e-6f) return track.keys.back().value;
        return Lerp(track.keys.back().value, track.keys.front().value, (time - track.keys.back().time) / span);
    }
    const auto next = std::upper_bound(track.keys.begin(), track.keys.end(), time,
        [](float t, const Keyframe& k) { return t < k.time; });
    const Keyframe& b = *next;
    const Keyframe& a = *(next - 1);
    const float span = b.time - a.time;
    return span > 1.0e-6f ? Lerp(a.value, b.value, (time - a.time) / span) : b.value;
}

void AnimationClip::Sample(float time, Pose& pose) const {
    if (!std::isfinite(time)) time = 0.0f;
    if (looping && duration > 0.0f) {
        time = std::fmod(time, duration);
        if (time < 0.0f) time += duration;
    } else {
        time = Clamp(time, 0.0f, duration);
    }
    for (const Track& track : tracks) {
        if (track.joint < 0 || static_cast<std::size_t>(track.joint) >= pose.local.size()) continue;
        TRS value = SampleTrack(track, time);
        if (extractRootMotion && track.joint == rootJoint) {
            // Root motion is consumed by the character; keep the root in place horizontally.
            value.translation.x = track.keys.front().value.translation.x;
            value.translation.z = track.keys.front().value.translation.z;
        }
        pose.local[static_cast<std::size_t>(track.joint)] = value;
    }
}

Vec3 AnimationClip::RootTranslationAt(float time) const {
    for (const Track& track : tracks) {
        if (track.joint == rootJoint) {
            const float clamped = Clamp(time, 0.0f, duration);
            // Sample without loop wrap so the end of the clip is its full displacement.
            if (clamped >= track.keys.back().time) return track.keys.back().value.translation;
            if (clamped <= track.keys.front().time) return track.keys.front().value.translation;
            const auto next = std::upper_bound(track.keys.begin(), track.keys.end(), clamped,
                [](float t, const Keyframe& k) { return t < k.time; });
            const Keyframe& b = *next;
            const Keyframe& a = *(next - 1);
            const float span = b.time - a.time;
            return span > 1.0e-6f ? Lerp(a.value.translation, b.value.translation, (clamped - a.time) / span) : b.value.translation;
        }
    }
    return {};
}

Vec3 AnimationClip::RootMotion(float fromTime, float toTime) const {
    if (!extractRootMotion || duration <= 0.0f) return {};
    auto horizontal = [](Vec3 v) { return Vec3{v.x, 0.0f, v.z}; };
    if (!looping) return horizontal(RootTranslationAt(toTime) - RootTranslationAt(fromTime));
    const float a = std::fmod(std::max(0.0f, fromTime), duration);
    const float cycles = std::floor(std::max(0.0f, toTime) / duration) - std::floor(std::max(0.0f, fromTime) / duration);
    const float b = std::fmod(std::max(0.0f, toTime), duration);
    const Vec3 full = RootTranslationAt(duration) - RootTranslationAt(0.0f);
    return horizontal(RootTranslationAt(b) - RootTranslationAt(a) + full * cycles);
}

void AnimationClip::FiredNotifies(float fromTime, float toTime, std::vector<const AnimNotify*>& out) const {
    if (!(toTime > fromTime)) return;
    for (const AnimNotify& notify : notifies) {
        if (looping && duration > 0.0f) {
            // Count every occurrence of the notify time inside the interval.
            const float first = std::ceil((fromTime - notify.time) / duration);
            for (float k = first; notify.time + k * duration <= toTime; k += 1.0f) {
                const float t = notify.time + k * duration;
                if (t > fromTime) out.push_back(&notify);
                if (k - first > 64.0f) break;
            }
        } else if (notify.time > fromTime && notify.time <= toTime) {
            out.push_back(&notify);
        }
    }
}

bool AnimationClip::IsWindowActive(const std::string& notifyName, float time) const {
    if (looping && duration > 0.0f) {
        time = std::fmod(time, duration);
        if (time < 0.0f) time += duration;
    }
    for (const AnimNotify& notify : notifies) {
        if (notify.name == notifyName && time >= notify.time && time <= notify.time + notify.duration) return true;
    }
    return false;
}

bool AnimationClip::Validate(const Skeleton& skeleton, std::string& error) const {
    if (!(duration > 0.0f) || !std::isfinite(duration)) {
        error = "clip duration must be positive";
        return false;
    }
    for (const Track& track : tracks) {
        if (track.joint < 0 || track.joint >= skeleton.JointCount()) {
            error = "track references a missing joint";
            return false;
        }
        for (std::size_t i = 0; i < track.keys.size(); ++i) {
            const Keyframe& key = track.keys[i];
            if (!std::isfinite(key.time) || key.time < 0.0f || key.time > duration + 1.0e-4f) {
                error = "key time outside the clip";
                return false;
            }
            if (i > 0 && key.time < track.keys[i - 1].time) {
                error = "keys are not sorted";
                return false;
            }
            const Math::Quat& q = key.value.rotation;
            if (!IsFinite(key.value.translation) || !IsFinite(key.value.scale) || !std::isfinite(q.x)
                || !std::isfinite(q.y) || !std::isfinite(q.z) || !std::isfinite(q.w)) {
                error = "non-finite key";
                return false;
            }
        }
    }
    return true;
}

} // namespace Astral::Animation
