#pragma once

// Skeletons, poses and keyframed clips.
//
// A Skeleton is an ordered joint list where every parent precedes its children,
// so forward kinematics is a single pass. A Pose stores local TRS per joint.
// AnimationClip tracks are sampled with linear/slerp interpolation, carry
// notify events (instant and windowed, e.g. an attack's active hit frames) and
// support root-motion extraction.

#include "Engine/Math/VectorMath.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Astral::Animation {

using Math::Mat4;
using Math::Quat;
using Math::TRS;
using Math::Vec3;

struct Joint {
    std::string name;
    int parent{-1};
    TRS bindLocal; // rest pose relative to the parent
};

class Skeleton {
public:
    // Returns the new joint index, or -1 if the parent is invalid (must precede the child).
    int AddJoint(const std::string& name, int parent, const TRS& bindLocal);
    int Find(const std::string& name) const;
    int JointCount() const { return static_cast<int>(joints_.size()); }
    const Joint& GetJoint(int index) const { return joints_[static_cast<std::size_t>(index)]; }
    const std::vector<Joint>& Joints() const { return joints_; }
    // Model-space bind matrices and their inverses (for skinning).
    void ComputeBindMatrices();
    const std::vector<Mat4>& InverseBindMatrices() const { return inverseBind_; }
    const std::vector<Mat4>& BindModelMatrices() const { return bindModel_; }

private:
    std::vector<Joint> joints_;
    std::vector<Mat4> bindModel_;
    std::vector<Mat4> inverseBind_;
};

struct Pose {
    std::vector<TRS> local;

    static Pose Bind(const Skeleton& skeleton);
    // Model-space matrices via forward kinematics.
    void ToModel(const Skeleton& skeleton, std::vector<Mat4>& model) const;
};

void BlendPoses(const Pose& a, const Pose& b, float weight, Pose& out);
// Blend only joints whose mask weight > 0 (e.g. upper body attack over running legs).
void BlendPosesMasked(const Pose& base, const Pose& layer, const std::vector<float>& mask, float weight, Pose& out);
// Additive: out = base + weight * (additive - reference), per joint.
void AddPose(const Pose& base, const Pose& additive, const Pose& reference, float weight, Pose& out);

struct Keyframe {
    float time{};
    TRS value;
};

struct Track {
    int joint{-1};
    std::vector<Keyframe> keys; // sorted by time
};

struct AnimNotify {
    std::string name;
    float time{};
    float duration{}; // > 0 makes a window (active between time and time + duration)
};

class AnimationClip {
public:
    std::string name;
    float duration{1.0f};
    bool looping{true};
    std::vector<Track> tracks;
    std::vector<AnimNotify> notifies;
    int rootJoint{0};
    bool extractRootMotion{false};

    // Keys must be added in increasing time per joint.
    void AddKey(int joint, float time, const TRS& value);
    // Fills pose (joints without a track keep their values, typically bind).
    void Sample(float time, Pose& pose) const;
    // Root translation delta between two times (handles loop wrap).
    Vec3 RootMotion(float fromTime, float toTime) const;
    // Notifies whose start lies in (fromTime, toTime], wrapping for loops.
    void FiredNotifies(float fromTime, float toTime, std::vector<const AnimNotify*>& out) const;
    bool IsWindowActive(const std::string& notify, float time) const;
    bool Validate(const Skeleton& skeleton, std::string& error) const;

private:
    TRS SampleTrack(const Track& track, float time) const;
    Vec3 RootTranslationAt(float time) const;
};

} // namespace Astral::Animation
