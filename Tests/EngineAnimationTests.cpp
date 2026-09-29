#include "Engine/Animation/Animator.h"
#include "Engine/Animation/HumanoidRig.h"
#include "Engine/Animation/Skinning.h"
#include "Tests/EngineTestSupport.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>

using namespace Astral;
using namespace Astral::Animation;
using Math::Vec3;

namespace {
void CheckVec(Vec3 a, Vec3 b, float tolerance = 1e-4f) {
    ASTRAL_CHECK_NEAR(a.x, b.x, tolerance);
    ASTRAL_CHECK_NEAR(a.y, b.y, tolerance);
    ASTRAL_CHECK_NEAR(a.z, b.z, tolerance);
}
Skeleton TwoJointArm(int& shoulder, int& elbow, int& wrist) {
    Skeleton s;
    const int root = s.AddJoint("root", -1, {});
    shoulder = s.AddJoint("shoulder", root, {{0, 1, 0}, {}, {1, 1, 1}});
    elbow = s.AddJoint("elbow", shoulder, {{1, 0, 0}, {}, {1, 1, 1}});
    wrist = s.AddJoint("wrist", elbow, {{1, 0, 0}, {}, {1, 1, 1}});
    s.ComputeBindMatrices();
    return s;
}
bool HasEvent(const Animator& animator, const std::string& name) {
    for (const AnimEvent& e : animator.Events())
        if (e.name == name) return true;
    return false;
}
} // namespace

ASTRAL_TEST(SkeletonOrderingAndBindMatrices) {
    Skeleton s;
    ASTRAL_CHECK(s.AddJoint("orphan", 3, {}) == -1);
    int shoulder, elbow, wrist;
    s = TwoJointArm(shoulder, elbow, wrist);
    ASTRAL_CHECK(s.Find("elbow") == elbow && s.Find("missing") == -1);
    CheckVec(Math::GetTranslation(s.BindModelMatrices()[static_cast<std::size_t>(wrist)]), {2, 1, 0});
    const Math::Mat4 product = s.BindModelMatrices()[2] * s.InverseBindMatrices()[2];
    ASTRAL_CHECK(Math::NearlyEqual(product, Math::Mat4::Identity()));
}

ASTRAL_TEST(ClipSamplingLoopsAndClamps) {
    int shoulder, elbow, wrist;
    const Skeleton s = TwoJointArm(shoulder, elbow, wrist);
    AnimationClip clip;
    clip.duration = 1.0f;
    clip.AddKey(elbow, 0.0f, {{1, 0, 0}, {}, {1, 1, 1}});
    clip.AddKey(elbow, 0.5f, {{1, 2, 0}, Math::QuatFromAxisAngle({0, 0, 1}, 1.0f), {1, 1, 1}});
    std::string error;
    ASTRAL_CHECK(clip.Validate(s, error));
    Pose pose = Pose::Bind(s);
    clip.Sample(0.25f, pose);
    CheckVec(pose.local[static_cast<std::size_t>(elbow)].translation, {1, 1, 0});
    // Looping wraps the last key back toward the first across the seam.
    clip.Sample(0.75f, pose);
    CheckVec(pose.local[static_cast<std::size_t>(elbow)].translation, {1, 1, 0});
    clip.Sample(1.25f, pose);
    CheckVec(pose.local[static_cast<std::size_t>(elbow)].translation, {1, 1, 0});
    clip.looping = false;
    clip.Sample(3.0f, pose);
    CheckVec(pose.local[static_cast<std::size_t>(elbow)].translation, {1, 2, 0});
    clip.Sample(std::nanf(""), pose);
    CheckVec(pose.local[static_cast<std::size_t>(elbow)].translation, {1, 0, 0});
    AnimationClip bad = clip;
    bad.AddKey(9, 0.0f, {});
    ASTRAL_CHECK(!bad.Validate(s, error));
    AnimationClip unsorted;
    unsorted.AddKey(elbow, 0.5f, {});
    unsorted.AddKey(elbow, 0.2f, {});
    ASTRAL_CHECK(!unsorted.Validate(s, error));
    for (int component = 0; component < 4; ++component) {
        AnimationClip nonFinite;
        nonFinite.duration = 1.0f;
        Math::TRS key;
        float* q[4] = {&key.rotation.x, &key.rotation.y, &key.rotation.z, &key.rotation.w};
        *q[component] = component % 2 ? std::numeric_limits<float>::infinity() : std::nanf("");
        nonFinite.AddKey(elbow, 0.0f, key);
        ASTRAL_CHECK(!nonFinite.Validate(s, error));
    }
}

ASTRAL_TEST(NotifiesAndRootMotion) {
    AnimationClip clip;
    clip.duration = 1.0f;
    clip.rootJoint = 0;
    clip.extractRootMotion = true;
    clip.AddKey(0, 0.0f, {{0, 0, 0}, {}, {1, 1, 1}});
    clip.AddKey(0, 1.0f, {{0, 0, 2}, {}, {1, 1, 1}});
    clip.notifies = {{"Step", 0.1f, 0.0f}, {"Hit", 0.4f, 0.2f}};
    std::vector<const AnimNotify*> fired;
    clip.FiredNotifies(0.0f, 0.5f, fired);
    ASTRAL_CHECK(fired.size() == 2);
    fired.clear();
    clip.FiredNotifies(0.95f, 2.15f, fired); // crosses two loop boundaries
    int steps = 0;
    for (const AnimNotify* n : fired) steps += n->name == "Step";
    ASTRAL_CHECK(steps == 2);
    ASTRAL_CHECK(clip.IsWindowActive("Hit", 0.5f));
    ASTRAL_CHECK(!clip.IsWindowActive("Hit", 0.7f));
    ASTRAL_CHECK(clip.IsWindowActive("Hit", 1.5f)); // wrapped
    CheckVec(clip.RootMotion(0.25f, 0.75f), {0, 0, 1});
    CheckVec(clip.RootMotion(0.0f, 2.5f), {0, 0, 5});
    // With extraction on, the sampled root stays in place.
    Skeleton s;
    s.AddJoint("root", -1, {});
    Pose pose = Pose::Bind(s);
    clip.Sample(0.5f, pose);
    CheckVec(pose.local[0].translation, {0, 0, 0});
}

ASTRAL_TEST(PoseBlendingMaskedAndAdditive) {
    Pose a, b, out;
    a.local = {{{0, 0, 0}, {}, {1, 1, 1}}, {{0, 0, 0}, {}, {1, 1, 1}}};
    b.local = {{{2, 0, 0}, Math::QuatFromYaw(1.0f), {1, 1, 1}}, {{0, 4, 0}, {}, {1, 1, 1}}};
    BlendPoses(a, b, 0.5f, out);
    CheckVec(out.local[0].translation, {1, 0, 0});
    CheckVec(Math::Rotate(out.local[0].rotation, {0, 0, 1}), Math::Rotate(Math::QuatFromYaw(0.5f), {0, 0, 1}));
    BlendPosesMasked(a, b, {0.0f, 1.0f}, 1.0f, out);
    CheckVec(out.local[0].translation, {0, 0, 0});
    CheckVec(out.local[1].translation, {0, 4, 0});
    Pose reference = a;
    AddPose(b, b, reference, 1.0f, out); // b + (b - identity) = double
    CheckVec(out.local[1].translation, {0, 8, 0});
    CheckVec(Math::Rotate(out.local[0].rotation, {0, 0, 1}), Math::Rotate(Math::QuatFromYaw(2.0f), {0, 0, 1}), 1e-3f);
}

ASTRAL_TEST(TwoBoneIkReachesTargets) {
    int shoulder, elbow, wrist;
    const Skeleton s = TwoJointArm(shoulder, elbow, wrist);
    for (const Vec3& target : {Vec3{1.2f, 1.8f, 0.3f}, Vec3{0.5f, 0.2f, 0.8f}, Vec3{-1.0f, 1.5f, 0.2f}}) {
        Pose pose = Pose::Bind(s);
        SolveTwoBoneIK(s, pose, shoulder, elbow, wrist, target, {1, 1, 1});
        std::vector<Math::Mat4> model;
        pose.ToModel(s, model);
        CheckVec(Math::GetTranslation(model[static_cast<std::size_t>(wrist)]), target, 2e-3f);
        // Bone lengths are preserved.
        ASTRAL_CHECK_NEAR(Math::Distance(Math::GetTranslation(model[1]), Math::GetTranslation(model[2])), 1.0f, 1e-4);
    }
    // Out of reach: the chain straightens toward the target.
    Pose pose = Pose::Bind(s);
    SolveTwoBoneIK(s, pose, shoulder, elbow, wrist, {0, 1, 5}, {1, 1, 1});
    std::vector<Math::Mat4> model;
    pose.ToModel(s, model);
    const Vec3 hand = Math::GetTranslation(model[static_cast<std::size_t>(wrist)]);
    ASTRAL_CHECK_NEAR(hand.z, 2.0f, 5e-3);
    // Zero weight leaves the pose untouched.
    Pose untouched = Pose::Bind(s);
    SolveTwoBoneIK(s, untouched, shoulder, elbow, wrist, {0.5f, 0.2f, 0.8f}, {1, 1, 1}, 0.0f);
    ASTRAL_CHECK(untouched.local[1].rotation.w == 1.0f);
}

ASTRAL_TEST(SkinningFollowsJoints) {
    int shoulder, elbow, wrist;
    const Skeleton s = TwoJointArm(shoulder, elbow, wrist);
    Graphics::MeshData mesh;
    Graphics::MeshBuilder builder(mesh);
    builder.SetSkinJoint(elbow);
    builder.AddBox({1.5f, 1, 0}, {0.4f, 0.1f, 0.1f});
    builder.SetSkinJoint(shoulder);
    builder.AddBox({0.5f, 1, 0}, {0.4f, 0.1f, 0.1f});
    std::string error;
    ASTRAL_CHECK(mesh.Validate(error));
    std::vector<Math::Mat4> skin;
    Graphics::MeshData out;
    Pose pose = Pose::Bind(s);
    ComputeSkinMatrices(s, pose, skin);
    SkinMesh(mesh, skin, out);
    for (std::size_t i = 0; i < mesh.vertices.size(); ++i) CheckVec(out.vertices[i].position, mesh.vertices[i].position);
    // Bend the elbow 90 degrees about Z: forearm vertices rotate, upper arm stays.
    pose.local[static_cast<std::size_t>(elbow)].rotation = Math::QuatFromAxisAngle({0, 0, 1}, Math::kHalfPi);
    ComputeSkinMatrices(s, pose, skin);
    SkinMesh(mesh, skin, out);
    for (std::size_t i = 0; i < mesh.vertices.size(); ++i) {
        if (mesh.skin[i].joints[0] == elbow) {
            ASTRAL_CHECK(out.vertices[i].position.y > 1.05f); // forearm now points up
            ASTRAL_CHECK_NEAR(Math::Length(out.vertices[i].normal), 1.0f, 1e-4);
        } else {
            CheckVec(out.vertices[i].position, mesh.vertices[i].position);
        }
    }
    ASTRAL_CHECK(out.bounds.max.y > 1.8f);
}

ASTRAL_TEST(HumanoidRigClipsAndMeshesAreValid) {
    HumanoidJoints joints;
    const Skeleton s = BuildHumanoidSkeleton(joints);
    ASTRAL_CHECK(s.JointCount() == 21);
    ASTRAL_CHECK(joints.weapon == s.JointCount() - 1);
    CheckVec(Math::GetTranslation(s.BindModelMatrices()[static_cast<std::size_t>(joints.head)]), {0, 1.55f, 0});
    const HumanoidClips clips = BuildHumanoidClips(s, joints);
    std::string error;
    for (const AnimationClip* clip : {&clips.idle, &clips.walk, &clips.run, &clips.lightAttack, &clips.heavyAttack,
             &clips.dash, &clips.guard, &clips.fatalStrike, &clips.focus, &clips.hitReact, &clips.defeat}) {
        ASTRAL_CHECK(clip->Validate(s, error));
    }
    const HumanoidMeshes meshes = BuildHumanoidMeshes(s, joints, {});
    ASTRAL_CHECK(meshes.body.Validate(error) && meshes.glow.Validate(error) && meshes.blade.Validate(error));
    ASTRAL_CHECK(meshes.body.skin.size() == meshes.body.vertices.size());
    ASTRAL_CHECK(meshes.body.TriangleCount() > 1000);
    ASTRAL_CHECK_NEAR(meshes.body.bounds.min.y, 0.0f, 0.05f);
    ASTRAL_CHECK(meshes.body.bounds.max.y > 1.7f && meshes.body.bounds.max.y < 2.0f);
    // Run cycle keeps feet near the ground and the character at plausible heights.
    Pose pose = Pose::Bind(s);
    std::vector<Math::Mat4> model;
    for (float t = 0.0f; t < 0.64f; t += 0.04f) {
        clips.run.Sample(t, pose);
        pose.ToModel(s, model);
        const float footL = Math::GetTranslation(model[static_cast<std::size_t>(joints.footL)]).y;
        const float footR = Math::GetTranslation(model[static_cast<std::size_t>(joints.footR)]).y;
        ASTRAL_CHECK(std::min(footL, footR) < 0.25f);
        ASTRAL_CHECK(std::max(footL, footR) < 0.8f);
    }
    const std::vector<float> mask = UpperBodyMask(s, joints);
    ASTRAL_CHECK(mask[static_cast<std::size_t>(joints.thighL)] == 0.0f && mask[static_cast<std::size_t>(joints.upperArmR)] == 1.0f);
}

ASTRAL_TEST(ShadowbladeStateMachineDrivesActions) {
    HumanoidJoints joints;
    const Skeleton s = BuildHumanoidSkeleton(joints);
    const HumanoidClips clips = BuildHumanoidClips(s, joints);
    const AnimStateMachine machine = BuildShadowbladeStateMachine(clips);
    Animator animator(s, machine);
    const float dt = 1.0f / 60.0f;
    animator.SetFloat("Speed", 6.0f);
    bool footstep = false;
    for (int i = 0; i < 60; ++i) {
        animator.Update(dt);
        footstep |= HasEvent(animator, "Footstep");
    }
    ASTRAL_CHECK(animator.CurrentStateName() == "Locomotion");
    ASTRAL_CHECK(footstep);
    // Light attack: swing and hit window, then back to locomotion.
    animator.SetTrigger("Light");
    bool swing = false, hitWindow = false, entered = false;
    for (int i = 0; i < 40; ++i) {
        animator.Update(dt);
        swing |= HasEvent(animator, "Swing");
        hitWindow |= animator.IsWindowActive("Hit");
        entered |= animator.CurrentStateName() == "LightAttack";
    }
    ASTRAL_CHECK(entered && swing && hitWindow);
    ASTRAL_CHECK(animator.CurrentStateName() == "Locomotion");
    // Guard holds while the bool is set.
    animator.SetBool("Guard", true);
    for (int i = 0; i < 20; ++i) animator.Update(dt);
    ASTRAL_CHECK(animator.CurrentStateName() == "Guard");
    animator.SetBool("Guard", false);
    for (int i = 0; i < 20; ++i) animator.Update(dt);
    ASTRAL_CHECK(animator.CurrentStateName() == "Locomotion");
    // Fatal strike fires Impact notifies (used for hitstop).
    animator.SetTrigger("Fatal");
    int impacts = 0;
    for (int i = 0; i < 80; ++i) {
        animator.Update(dt);
        for (const AnimEvent& e : animator.Events()) impacts += e.name == "Impact";
    }
    ASTRAL_CHECK(impacts == 2);
    // Unconsumed triggers expire (input buffer window) instead of firing much later.
    animator.SetBool("Focus", true);
    for (int i = 0; i < 30; ++i) animator.Update(dt);
    ASTRAL_CHECK(animator.CurrentStateName() == "Focus");
    animator.SetTrigger("Dash");
    animator.Update(dt);
    ASTRAL_CHECK(animator.CurrentStateName() == "Dash");
    ASTRAL_CHECK(animator.InTransition());
    // Upper-body overlay over running legs.
    animator.SetBool("Focus", false);
    for (int i = 0; i < 60; ++i) animator.Update(dt);
    animator.PlayOverlay(&clips.lightAttack, UpperBodyMask(s, joints));
    bool overlayHit = false;
    for (int i = 0; i < 40; ++i) {
        animator.Update(dt);
        overlayHit |= animator.IsWindowActive("Hit");
    }
    ASTRAL_CHECK(overlayHit && !animator.OverlayActive());
    animator.SetBool("Defeated", true);
    for (int i = 0; i < 90; ++i) animator.Update(dt);
    ASTRAL_CHECK(animator.CurrentStateName() == "Defeat");
    for (const Math::TRS& trs : animator.GetPose().local) {
        ASTRAL_CHECK(Math::IsFinite(trs.translation));
        ASTRAL_CHECK_NEAR(Math::Dot(trs.rotation, trs.rotation), 1.0f, 1e-3);
    }
}

ASTRAL_TEST_MAIN("EngineAnimationTests")
