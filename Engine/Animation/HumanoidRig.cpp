#include "Engine/Animation/HumanoidRig.h"

#include <cmath>

namespace Astral::Animation {

using namespace Math;
using Graphics::Color;
using Graphics::MeshBuilder;
using Graphics::MeshData;

namespace {

TRS Offset(Vec3 t) { return {t, {}, {1, 1, 1}}; }

// Writes rotation keys relative to the joint's bind pose (Euler degrees:
// pitch about X, yaw about Y, roll about Z; applied yaw * pitch * roll).
struct KeyWriter {
    AnimationClip& clip;
    const Skeleton& skeleton;
    void Rot(int joint, float t, float pitch, float yaw = 0.0f, float roll = 0.0f, Vec3 offset = {}) const {
        TRS trs = skeleton.GetJoint(joint).bindLocal;
        trs.rotation = Normalize(trs.rotation * QuatFromEuler(Radians(yaw), Radians(pitch), Radians(roll)));
        trs.translation += offset;
        clip.AddKey(joint, t, trs);
    }
};

AnimationClip MakeClip(const char* name, float duration, bool looping) {
    AnimationClip clip;
    clip.name = name;
    clip.duration = duration;
    clip.looping = looping;
    return clip;
}

} // namespace

Skeleton BuildHumanoidSkeleton(HumanoidJoints& j) {
    Skeleton s;
    j.root = s.AddJoint("root", -1, Offset({0, 0, 0}));
    j.pelvis = s.AddJoint("pelvis", j.root, Offset({0, 0.95f, 0}));
    j.spine = s.AddJoint("spine", j.pelvis, Offset({0, 0.12f, 0}));
    j.chest = s.AddJoint("chest", j.spine, Offset({0, 0.2f, 0}));
    j.neck = s.AddJoint("neck", j.chest, Offset({0, 0.2f, 0}));
    j.head = s.AddJoint("head", j.neck, Offset({0, 0.08f, 0}));
    j.shoulderL = s.AddJoint("shoulder_l", j.chest, Offset({-0.08f, 0.15f, 0}));
    j.upperArmL = s.AddJoint("upperarm_l", j.shoulderL, Offset({-0.12f, -0.01f, 0}));
    j.forearmL = s.AddJoint("forearm_l", j.upperArmL, Offset({0, -0.28f, 0}));
    j.handL = s.AddJoint("hand_l", j.forearmL, Offset({0, -0.25f, 0}));
    j.shoulderR = s.AddJoint("shoulder_r", j.chest, Offset({0.08f, 0.15f, 0}));
    j.upperArmR = s.AddJoint("upperarm_r", j.shoulderR, Offset({0.12f, -0.01f, 0}));
    j.forearmR = s.AddJoint("forearm_r", j.upperArmR, Offset({0, -0.28f, 0}));
    j.handR = s.AddJoint("hand_r", j.forearmR, Offset({0, -0.25f, 0}));
    j.thighL = s.AddJoint("thigh_l", j.pelvis, Offset({-0.1f, -0.03f, 0}));
    j.shinL = s.AddJoint("shin_l", j.thighL, Offset({0, -0.43f, 0}));
    j.footL = s.AddJoint("foot_l", j.shinL, Offset({0, -0.42f, 0}));
    j.thighR = s.AddJoint("thigh_r", j.pelvis, Offset({0.1f, -0.03f, 0}));
    j.shinR = s.AddJoint("shin_r", j.thighR, Offset({0, -0.43f, 0}));
    j.footR = s.AddJoint("foot_r", j.shinR, Offset({0, -0.42f, 0}));
    // Blade socket: +90 degrees about X turns the blade's +Y length toward +Z.
    j.weapon = s.AddJoint("weapon", j.handR, {{0, -0.07f, 0.02f}, QuatFromAxisAngle({1, 0, 0}, kHalfPi), {1, 1, 1}});
    s.ComputeBindMatrices();
    return s;
}

std::vector<float> UpperBodyMask(const Skeleton& skeleton, const HumanoidJoints& j) {
    std::vector<float> mask(static_cast<std::size_t>(skeleton.JointCount()), 0.0f);
    for (int joint : {j.spine, j.chest, j.neck, j.head, j.shoulderL, j.upperArmL, j.forearmL, j.handL, j.shoulderR,
             j.upperArmR, j.forearmR, j.handR, j.weapon}) {
        if (joint >= 0) mask[static_cast<std::size_t>(joint)] = 1.0f;
    }
    if (j.spine >= 0) mask[static_cast<std::size_t>(j.spine)] = 0.6f; // soften the waist seam
    return mask;
}

HumanoidClips BuildHumanoidClips(const Skeleton& s, const HumanoidJoints& j) {
    HumanoidClips c;

    // Idle: breathing, soft arm sway, relaxed blade hand.
    c.idle = MakeClip("Idle", 2.0f, true);
    {
        KeyWriter k{c.idle, s};
        for (float t : {0.0f, 1.0f, 2.0f}) {
            const float breath = t == 1.0f ? 1.0f : 0.0f;
            k.Rot(j.pelvis, t, 0, 0, 0, {0, -0.012f * breath, 0});
            k.Rot(j.chest, t, 2.0f - 3.0f * breath);
            k.Rot(j.head, t, 3.0f + 2.0f * breath);
            k.Rot(j.upperArmL, t, 4.0f, 0, -8.0f - 2.0f * breath);
            k.Rot(j.upperArmR, t, -6.0f, 0, 10.0f + 2.0f * breath);
            k.Rot(j.forearmL, t, -12.0f);
            k.Rot(j.forearmR, t, -25.0f);
            k.Rot(j.thighL, t, -3.0f, 0, -2.0f);
            k.Rot(j.thighR, t, 4.0f, 0, 3.0f);
            k.Rot(j.shinL, t, 5.0f);
            k.Rot(j.shinR, t, 3.0f);
        }
    }

    // Walk and run share the same gait timeline (phase-synchronised blend space).
    auto gait = [&](AnimationClip& clip, float duration, float thighAmp, float shinAmp, float armAmp, float bob, float lean) {
        clip = MakeClip(clip.name.c_str(), duration, true);
        KeyWriter k{clip, s};
        const float times[5] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
        const float thighL[5] = {-1.0f, 0.0f, 0.8f, -0.25f, -1.0f};
        const float shinL[5] = {0.15f, 0.2f, 0.35f, 1.0f, 0.15f};
        const float thighR[5] = {0.8f, -0.25f, -1.0f, 0.0f, 0.8f};
        const float shinR[5] = {0.35f, 1.0f, 0.15f, 0.2f, 0.35f};
        const float armL[5] = {0.9f, 0.0f, -1.0f, 0.0f, 0.9f};
        const float pelvisY[5] = {-1.0f, 0.0f, -1.0f, 0.0f, -1.0f};
        const float twist[5] = {1.0f, 0.0f, -1.0f, 0.0f, 1.0f};
        for (int i = 0; i < 5; ++i) {
            const float t = times[i] * duration;
            k.Rot(j.pelvis, t, 0, 6.0f * twist[i], 0, {0, bob * pelvisY[i], 0});
            k.Rot(j.chest, t, lean, -9.0f * twist[i]);
            k.Rot(j.head, t, -lean * 0.5f, 4.0f * twist[i]);
            k.Rot(j.thighL, t, thighAmp * thighL[i]);
            k.Rot(j.shinL, t, shinAmp * shinL[i]);
            k.Rot(j.thighR, t, thighAmp * thighR[i]);
            k.Rot(j.shinR, t, shinAmp * shinR[i]);
            k.Rot(j.upperArmL, t, armAmp * armL[i], 0, -8.0f);
            k.Rot(j.upperArmR, t, -armAmp * armL[i], 0, 8.0f);
            k.Rot(j.forearmL, t, -35.0f - 20.0f * std::fabs(armL[i]));
            k.Rot(j.forearmR, t, -35.0f - 20.0f * std::fabs(armL[i]));
        }
    };
    c.walk.name = "Walk";
    gait(c.walk, 1.0f, 26.0f, 60.0f, 18.0f, 0.02f, 4.0f);
    c.run.name = "Run";
    gait(c.run, 0.64f, 46.0f, 100.0f, 40.0f, 0.05f, 14.0f);
    c.run.notifies = {{"Footstep", 0.0f, 0.0f}, {"Footstep", 0.32f, 0.0f}};
    c.walk.notifies = {{"Footstep", 0.0f, 0.0f}, {"Footstep", 0.5f, 0.0f}};

    // Light attack: fast right-hand horizontal slash.
    c.lightAttack = MakeClip("LightAttack", 0.45f, false);
    {
        KeyWriter k{c.lightAttack, s};
        k.Rot(j.chest, 0.0f, 4.0f);
        k.Rot(j.chest, 0.1f, 6.0f, -28.0f);
        k.Rot(j.chest, 0.2f, 10.0f, 32.0f);
        k.Rot(j.chest, 0.45f, 4.0f, 5.0f);
        k.Rot(j.upperArmR, 0.0f, -20.0f, 0, 15.0f);
        k.Rot(j.upperArmR, 0.1f, -95.0f, 0, 70.0f);
        k.Rot(j.upperArmR, 0.2f, -80.0f, 0, -35.0f);
        k.Rot(j.upperArmR, 0.45f, -25.0f, 0, 12.0f);
        k.Rot(j.forearmR, 0.0f, -30.0f);
        k.Rot(j.forearmR, 0.1f, -70.0f);
        k.Rot(j.forearmR, 0.2f, -8.0f);
        k.Rot(j.forearmR, 0.45f, -28.0f);
        k.Rot(j.upperArmL, 0.0f, 5.0f, 0, -10.0f);
        k.Rot(j.upperArmL, 0.2f, 25.0f, 0, -35.0f);
        k.Rot(j.upperArmL, 0.45f, 5.0f, 0, -10.0f);
        k.Rot(j.thighL, 0.0f, -5.0f);
        k.Rot(j.thighL, 0.2f, -30.0f);
        k.Rot(j.thighL, 0.45f, -8.0f);
        k.Rot(j.shinL, 0.2f, 30.0f);
        k.Rot(j.thighR, 0.2f, 18.0f);
        c.lightAttack.notifies = {{"Swing", 0.11f, 0.0f}, {"Hit", 0.14f, 0.1f}};
    }

    // Heavy attack: overhead two-handed slam with a crouch.
    c.heavyAttack = MakeClip("HeavyAttack", 0.8f, false);
    {
        KeyWriter k{c.heavyAttack, s};
        const float times[4] = {0.0f, 0.25f, 0.42f, 0.8f};
        const float arms[4] = {-20.0f, -165.0f, -45.0f, -25.0f};
        const float chest[4] = {4.0f, -18.0f, 28.0f, 6.0f};
        const float drop[4] = {0.0f, 0.02f, -0.14f, -0.02f};
        const float thigh[4] = {-4.0f, -10.0f, -45.0f, -10.0f};
        const float shin[4] = {4.0f, 12.0f, 70.0f, 15.0f};
        for (int i = 0; i < 4; ++i) {
            k.Rot(j.pelvis, times[i], 0, 0, 0, {0, drop[i], 0});
            k.Rot(j.chest, times[i], chest[i]);
            k.Rot(j.upperArmR, times[i], arms[i], 0, 8.0f);
            k.Rot(j.upperArmL, times[i], arms[i], 0, -8.0f);
            k.Rot(j.forearmR, times[i], i == 1 ? -35.0f : -15.0f);
            k.Rot(j.forearmL, times[i], i == 1 ? -35.0f : -15.0f);
            k.Rot(j.thighL, times[i], thigh[i]);
            k.Rot(j.shinL, times[i], shin[i]);
            k.Rot(j.thighR, times[i], -thigh[i] * 0.4f);
            k.Rot(j.shinR, times[i], shin[i]);
        }
        c.heavyAttack.notifies = {{"Swing", 0.3f, 0.0f}, {"Hit", 0.35f, 0.13f}, {"Impact", 0.42f, 0.0f}};
    }

    // Shadow Dash: extreme forward lean, arms swept back (ninja dash), afterimages.
    c.dash = MakeClip("Dash", 0.35f, false);
    {
        KeyWriter k{c.dash, s};
        for (float t : {0.0f, 0.08f, 0.27f, 0.35f}) {
            const float f = (t == 0.0f || t == 0.35f) ? 0.3f : 1.0f;
            k.Rot(j.pelvis, t, 0, 0, 0, {0, -0.12f * f, 0});
            k.Rot(j.chest, t, 38.0f * f);
            k.Rot(j.head, t, -25.0f * f);
            k.Rot(j.upperArmL, t, 65.0f * f, 0, -25.0f * f);
            k.Rot(j.upperArmR, t, 60.0f * f, 0, 25.0f * f);
            k.Rot(j.forearmL, t, -10.0f);
            k.Rot(j.forearmR, t, -10.0f);
            k.Rot(j.thighL, t, -55.0f * f);
            k.Rot(j.shinL, t, 80.0f * f);
            k.Rot(j.thighR, t, 40.0f * f);
            k.Rot(j.shinR, t, 55.0f * f);
        }
        c.dash.notifies = {{"Afterimage", 0.0f, 0.0f}, {"Afterimage", 0.07f, 0.0f}, {"Afterimage", 0.14f, 0.0f},
            {"Afterimage", 0.21f, 0.0f}, {"DashTrail", 0.0f, 0.3f}};
    }

    // Guard: crossed arms, blade across the body, lowered stance.
    c.guard = MakeClip("Guard", 0.8f, true);
    {
        KeyWriter k{c.guard, s};
        for (float t : {0.0f, 0.4f, 0.8f}) {
            const float pulse = t == 0.4f ? 1.0f : 0.0f;
            k.Rot(j.pelvis, t, 0, 0, 0, {0, -0.08f - 0.01f * pulse, 0});
            k.Rot(j.chest, t, 12.0f);
            k.Rot(j.upperArmL, t, -75.0f, 0, 38.0f);
            k.Rot(j.forearmL, t, -85.0f);
            k.Rot(j.upperArmR, t, -80.0f, 0, -30.0f);
            k.Rot(j.forearmR, t, -70.0f);
            k.Rot(j.thighL, t, -25.0f, 0, -8.0f);
            k.Rot(j.shinL, t, 35.0f);
            k.Rot(j.thighR, t, 10.0f, 0, 8.0f);
            k.Rot(j.shinR, t, 30.0f);
        }
    }

    // Fatal Strike: crouch, full spin slash, heavy finish.
    c.fatalStrike = MakeClip("FatalStrike", 1.1f, false);
    {
        KeyWriter k{c.fatalStrike, s};
        const float times[8] = {0.0f, 0.25f, 0.35f, 0.45f, 0.55f, 0.65f, 0.8f, 1.1f};
        const float yaw[8] = {0.0f, -40.0f, 60.0f, 160.0f, 260.0f, 320.0f, 360.0f, 360.0f};
        const float drop[8] = {0.0f, -0.2f, -0.12f, -0.08f, -0.08f, -0.12f, -0.22f, -0.02f};
        for (int i = 0; i < 8; ++i) {
            const float spin = (i >= 2 && i <= 5) ? 1.0f : 0.0f;
            k.Rot(j.pelvis, times[i], 0, yaw[i], 0, {0, drop[i], 0});
            k.Rot(j.chest, times[i], i == 1 ? 25.0f : (i == 6 ? 30.0f : 8.0f));
            k.Rot(j.upperArmR, times[i], i == 1 ? 40.0f : (i == 6 ? -60.0f : -85.0f), 0, i == 1 ? 60.0f : (spin > 0 ? 88.0f : 20.0f));
            k.Rot(j.forearmR, times[i], spin > 0 ? -5.0f : -30.0f);
            k.Rot(j.upperArmL, times[i], i == 1 ? -20.0f : 20.0f, 0, spin > 0 ? -70.0f : -20.0f);
            k.Rot(j.thighL, times[i], i == 1 || i == 6 ? -50.0f : -15.0f);
            k.Rot(j.shinL, times[i], i == 1 || i == 6 ? 85.0f : 30.0f);
            k.Rot(j.thighR, times[i], i == 1 || i == 6 ? 30.0f : 10.0f);
            k.Rot(j.shinR, times[i], i == 1 || i == 6 ? 60.0f : 25.0f);
        }
        c.fatalStrike.notifies = {{"Swing", 0.3f, 0.0f}, {"Hit", 0.4f, 0.25f}, {"Impact", 0.55f, 0.0f}, {"Impact", 0.8f, 0.0f}};
    }

    // Thought Focus: two fingers to the temple, slight levitation.
    c.focus = MakeClip("Focus", 1.6f, true);
    {
        KeyWriter k{c.focus, s};
        for (float t : {0.0f, 0.8f, 1.6f}) {
            const float up = t == 0.8f ? 1.0f : 0.0f;
            k.Rot(j.pelvis, t, 0, 0, 0, {0, 0.02f + 0.02f * up, 0});
            k.Rot(j.chest, t, -3.0f);
            k.Rot(j.head, t, -6.0f, 8.0f);
            k.Rot(j.upperArmR, t, -115.0f, 0, -18.0f);
            k.Rot(j.forearmR, t, -135.0f);
            k.Rot(j.upperArmL, t, -10.0f, 0, -28.0f - 4.0f * up);
            k.Rot(j.forearmL, t, -15.0f);
            k.Rot(j.thighL, t, -4.0f, 0, -3.0f);
            k.Rot(j.thighR, t, 6.0f, 0, 3.0f);
            k.Rot(j.shinR, t, 10.0f + 4.0f * up);
        }
    }

    c.hitReact = MakeClip("HitReact", 0.35f, false);
    {
        KeyWriter k{c.hitReact, s};
        k.Rot(j.chest, 0.0f, 0.0f);
        k.Rot(j.chest, 0.08f, -22.0f, 8.0f);
        k.Rot(j.chest, 0.35f, 0.0f);
        k.Rot(j.head, 0.0f, 0.0f);
        k.Rot(j.head, 0.08f, -18.0f);
        k.Rot(j.head, 0.35f, 0.0f);
        k.Rot(j.pelvis, 0.0f, 0.0f);
        k.Rot(j.pelvis, 0.08f, 0, 0, 0, {0, -0.03f, -0.05f});
        k.Rot(j.pelvis, 0.35f, 0.0f);
    }

    c.defeat = MakeClip("Defeat", 1.0f, false);
    {
        KeyWriter k{c.defeat, s};
        const float times[3] = {0.0f, 0.4f, 1.0f};
        const float drop[3] = {0.0f, -0.35f, -0.62f};
        for (int i = 0; i < 3; ++i) {
            k.Rot(j.pelvis, times[i], i == 2 ? -12.0f : 0.0f, 0, 0, {0, drop[i], 0});
            k.Rot(j.chest, times[i], i * 22.0f);
            k.Rot(j.head, times[i], i * 15.0f);
            k.Rot(j.thighL, times[i], -i * 45.0f);
            k.Rot(j.thighR, times[i], -i * 40.0f);
            k.Rot(j.shinL, times[i], i * 60.0f);
            k.Rot(j.shinR, times[i], i * 65.0f);
            k.Rot(j.upperArmL, times[i], -i * 10.0f, 0, -i * 10.0f);
            k.Rot(j.upperArmR, times[i], -i * 10.0f, 0, i * 10.0f);
        }
    }
    return c;
}

HumanoidMeshes BuildHumanoidMeshes(const Skeleton& skeleton, const HumanoidJoints& j, const CharacterLook& look) {
    HumanoidMeshes meshes;
    const auto& bind = skeleton.BindModelMatrices();
    auto at = [&](int joint) { return GetTranslation(bind[static_cast<std::size_t>(joint)]); };

    MeshBuilder b(meshes.body);
    auto part = [&](int joint, Color color) {
        b.SetSkinJoint(joint);
        b.SetColor(color);
        b.SetTransform(Mat4::Identity());
    };
    // Legs: thighs, shins, boots.
    for (int side = 0; side < 2; ++side) {
        const int thigh = side == 0 ? j.thighL : j.thighR;
        const int shin = side == 0 ? j.shinL : j.shinR;
        const int foot = side == 0 ? j.footL : j.footR;
        part(thigh, look.pants);
        b.AddCapsule(at(shin) + Vec3{0, -0.02f, 0}, 0.075f, 0.5f, 6, 10);
        part(shin, look.pants);
        b.AddCapsule(at(foot) + Vec3{0, 0.1f, 0}, 0.062f, 0.36f, 6, 10);
        part(foot, look.boots);
        b.AddTaperedBox(at(foot) + Vec3{0, -0.07f, 0.035f}, {0.055f, 0.1f}, {0.06f, 0.07f}, 0.2f);
        b.AddBox(at(foot) + Vec3{0, -0.05f, 0.1f}, {0.055f, 0.025f, 0.065f});
    }
    // Pelvis and torso.
    part(j.pelvis, look.pants);
    b.AddTaperedBox(at(j.pelvis) + Vec3{0, -0.12f, 0}, {0.16f, 0.1f}, {0.15f, 0.1f}, 0.2f);
    part(j.spine, look.coat);
    b.AddTaperedBox(at(j.spine) + Vec3{0, -0.06f, 0}, {0.15f, 0.1f}, {0.17f, 0.11f}, 0.24f);
    part(j.chest, look.coat);
    b.AddTaperedBox(at(j.chest) + Vec3{0, -0.04f, 0}, {0.17f, 0.11f}, {0.2f, 0.1f}, 0.24f);
    b.SetColor(look.coatInner);
    b.AddTaperedBox(at(j.chest) + Vec3{0, -0.02f, 0.06f}, {0.07f, 0.06f}, {0.05f, 0.06f}, 0.2f); // collar V
    // High collar and scarf.
    part(j.neck, look.scarf);
    b.AddCylinder(at(j.neck) + Vec3{0, -0.06f, 0}, 0.09f, 0.1f, 12, true, 0.075f);
    part(j.chest, look.scarf);
    b.AddBox(at(j.neck) + Vec3{0.06f, -0.12f, -0.16f}, {0.05f, 0.1f, 0.03f});
    b.AddBox(at(j.neck) + Vec3{0.1f, -0.3f, -0.18f}, {0.045f, 0.09f, 0.025f});
    // Head, face and spiky anime hair.
    part(j.head, look.skin);
    b.AddSphere(at(j.head) + Vec3{0, 0.1f, 0}, 0.115f, 12, 16);
    b.AddTaperedBox(at(j.head) + Vec3{0, -0.01f, 0.03f}, {0.05f, 0.05f}, {0.08f, 0.07f}, 0.08f); // jaw
    b.SetColor(look.eyes);
    b.AddBox(at(j.head) + Vec3{-0.042f, 0.11f, 0.105f}, {0.022f, 0.028f, 0.01f});
    b.AddBox(at(j.head) + Vec3{0.042f, 0.11f, 0.105f}, {0.022f, 0.028f, 0.01f});
    b.SetColor(Color{0.1f, 0.08f, 0.12f});
    b.AddBox(at(j.head) + Vec3{-0.042f, 0.143f, 0.108f}, {0.026f, 0.006f, 0.008f}); // brows
    b.AddBox(at(j.head) + Vec3{0.042f, 0.143f, 0.108f}, {0.026f, 0.006f, 0.008f});
    b.SetColor(look.hair);
    b.AddSphere(at(j.head) + Vec3{0, 0.14f, -0.015f}, 0.122f, 10, 14);
    if (look.spikyHair) {
        struct Spike { float yaw, pitch, length, width; };
        const Spike spikes[] = {{0, -35, 0.2f, 0.06f}, {35, -20, 0.18f, 0.05f}, {-35, -20, 0.18f, 0.05f},
            {70, 5, 0.15f, 0.045f}, {-70, 5, 0.15f, 0.045f}, {150, 10, 0.2f, 0.06f}, {-150, 10, 0.2f, 0.06f},
            {180, 30, 0.22f, 0.06f}, {110, 40, 0.16f, 0.05f}, {-110, 40, 0.16f, 0.05f}, {0, 60, 0.12f, 0.05f}};
        for (const Spike& spike : spikes) {
            const Vec3 base = at(j.head) + Vec3{0, 0.16f, -0.015f};
            const Mat4 orient = Translation(base) * RotationY(Radians(spike.yaw)) * RotationX(Radians(spike.pitch + 90.0f))
                * Translation({0, 0.07f, 0});
            b.SetTransform(orient);
            b.AddPyramid({0, 0, 0}, {spike.width, spike.width * 0.7f}, spike.length);
        }
        b.SetTransform(Mat4::Identity());
        // Fringe falling over the forehead.
        for (float x : {-0.06f, 0.0f, 0.06f}) {
            b.SetTransform(Translation(at(j.head) + Vec3{x, 0.22f, 0.085f}) * RotationX(Radians(160.0f)));
            b.AddPyramid({0, 0, 0}, {0.03f, 0.02f}, 0.1f);
        }
        b.SetTransform(Mat4::Identity());
    }
    // Arms: coat sleeves, gloves.
    for (int side = 0; side < 2; ++side) {
        const float sx = side == 0 ? -1.0f : 1.0f;
        const int shoulder = side == 0 ? j.shoulderL : j.shoulderR;
        const int upper = side == 0 ? j.upperArmL : j.upperArmR;
        const int fore = side == 0 ? j.forearmL : j.forearmR;
        const int hand = side == 0 ? j.handL : j.handR;
        part(shoulder, look.coat);
        b.AddSphere(at(upper) + Vec3{0, 0.02f, 0}, 0.075f, 8, 10);
        b.AddBox(at(upper) + Vec3{sx * 0.02f, 0.07f, 0}, {0.07f, 0.02f, 0.07f}); // pauldron
        part(upper, look.coat);
        b.AddCapsule(at(fore) + Vec3{0, 0.0f, 0}, 0.06f, 0.32f, 6, 10);
        part(fore, look.coat);
        b.AddCylinder(at(hand) + Vec3{0, 0.02f, 0}, 0.058f, 0.24f, 10, true, 0.05f);
        b.SetColor(look.coatInner);
        b.AddCylinder(at(hand) + Vec3{0, 0.02f, 0}, 0.062f, 0.05f, 10, true); // cuff
        part(hand, Color{0.08f, 0.08f, 0.1f});
        b.AddBox(at(hand) + Vec3{0, -0.05f, 0.005f}, {0.04f, 0.055f, 0.025f});
    }
    // Long coat tails that follow the legs.
    if (look.longCoat) {
        for (int side = 0; side < 2; ++side) {
            const float sx = side == 0 ? -1.0f : 1.0f;
            const int thigh = side == 0 ? j.thighL : j.thighR;
            part(thigh, look.coat);
            b.AddTaperedBox(at(thigh) + Vec3{sx * 0.03f, -0.62f, 0.02f}, {0.12f, 0.11f}, {0.1f, 0.1f}, 0.68f);
            b.SetColor(look.coatInner);
            b.AddTaperedBox(at(thigh) + Vec3{sx * 0.03f, -0.63f, 0.02f}, {0.112f, 0.102f}, {0.112f, 0.102f}, 0.02f);
        }
        part(j.pelvis, look.coat);
        b.AddTaperedBox(at(j.pelvis) + Vec3{0, -0.6f, -0.1f}, {0.2f, 0.035f}, {0.17f, 0.04f}, 0.62f); // back tail
    }
    meshes.body.ComputeBounds();

    // Emissive accents: sleeve bands, coat piping, eye glints.
    MeshBuilder g(meshes.glow);
    auto glowPart = [&](int joint) {
        g.SetSkinJoint(joint);
        g.SetColor(look.glow);
    };
    for (int side = 0; side < 2; ++side) {
        const int fore = side == 0 ? j.forearmL : j.forearmR;
        const int hand = side == 0 ? j.handL : j.handR;
        glowPart(fore);
        g.AddCylinder(at(hand) + Vec3{0, 0.09f, 0}, 0.061f, 0.015f, 10, false);
        const int thigh = side == 0 ? j.thighL : j.thighR;
        if (look.longCoat) {
            glowPart(thigh);
            const float sx = side == 0 ? -1.0f : 1.0f;
            g.AddBox(at(thigh) + Vec3{sx * 0.03f, -0.6f, 0.125f}, {0.1f, 0.008f, 0.004f});
        }
    }
    glowPart(j.chest);
    g.AddBox(at(j.chest) + Vec3{0, 0.1f, 0.112f}, {0.005f, 0.1f, 0.004f});
    glowPart(j.head);
    g.AddBox(at(j.head) + Vec3{-0.036f, 0.118f, 0.116f}, {0.006f, 0.008f, 0.002f});
    g.AddBox(at(j.head) + Vec3{0.048f, 0.118f, 0.116f}, {0.006f, 0.008f, 0.002f});
    meshes.glow.ComputeBounds();

    // Blade in the weapon socket's frame, then placed at its bind transform.
    MeshBuilder w(meshes.blade);
    w.SetSkinJoint(j.weapon);
    w.SetTransform(bind[static_cast<std::size_t>(j.weapon)]);
    w.SetColor(Color{0.1f, 0.08f, 0.14f});
    w.AddCylinder({0, -0.12f, 0}, 0.018f, 0.2f, 8);     // grip
    w.SetColor(look.scarf);
    w.AddBox({0, 0.09f, 0}, {0.06f, 0.012f, 0.025f});  // guard
    w.SetColor(look.blade);
    w.AddBlade({0, 0.1f, 0}, 0.95f, 0.07f, 0.02f);
    meshes.blade.ComputeBounds();
    return meshes;
}

AnimStateMachine BuildShadowbladeStateMachine(const HumanoidClips& clips) {
    AnimStateMachine machine;
    AnimState locomotion;
    locomotion.name = "Locomotion";
    locomotion.blendParameter = "Speed";
    locomotion.blendSamples = {{0.0f, &clips.idle}, {2.0f, &clips.walk}, {6.0f, &clips.run}};
    const int move = machine.AddState(locomotion);
    const int light = machine.AddState({"LightAttack", &clips.lightAttack, {}, "", 1.0f});
    const int heavy = machine.AddState({"HeavyAttack", &clips.heavyAttack, {}, "", 1.0f});
    const int dash = machine.AddState({"Dash", &clips.dash, {}, "", 1.0f});
    const int guard = machine.AddState({"Guard", &clips.guard, {}, "", 1.0f});
    const int fatal = machine.AddState({"FatalStrike", &clips.fatalStrike, {}, "", 1.0f});
    const int focus = machine.AddState({"Focus", &clips.focus, {}, "", 1.0f});
    const int hit = machine.AddState({"HitReact", &clips.hitReact, {}, "", 1.0f});
    const int defeat = machine.AddState({"Defeat", &clips.defeat, {}, "", 1.0f});
    machine.defaultState = move;
    using Op = ConditionOp;
    // Highest priority first: defeat, hit, dash, fatal, heavy, light, guard, focus.
    machine.AddTransition({-1, defeat, 0.2f, {{"Defeated", Op::IsTrue, 0}}, -1.0f, false});
    machine.AddTransition({-1, hit, 0.05f, {{"Hit", Op::Triggered, 0}}, -1.0f, true});
    machine.AddTransition({-1, dash, 0.05f, {{"Dash", Op::Triggered, 0}}, -1.0f, true});
    machine.AddTransition({-1, fatal, 0.08f, {{"Fatal", Op::Triggered, 0}}, -1.0f, false});
    // Attacks can cancel from locomotion, guard or focus, and chain after the hit window.
    for (int from : {move, guard, focus}) {
        machine.AddTransition({from, heavy, 0.08f, {{"Heavy", Op::Triggered, 0}}, -1.0f, false});
        machine.AddTransition({from, light, 0.06f, {{"Light", Op::Triggered, 0}}, -1.0f, false});
    }
    machine.AddTransition({light, light, 0.06f, {{"Light", Op::Triggered, 0}}, 0.55f, true});
    machine.AddTransition({light, heavy, 0.08f, {{"Heavy", Op::Triggered, 0}}, 0.55f, false});
    machine.AddTransition({move, guard, 0.1f, {{"Guard", Op::IsTrue, 0}}, -1.0f, false});
    machine.AddTransition({guard, move, 0.15f, {{"Guard", Op::IsFalse, 0}}, -1.0f, false});
    machine.AddTransition({move, focus, 0.2f, {{"Focus", Op::IsTrue, 0}}, -1.0f, false});
    machine.AddTransition({focus, move, 0.25f, {{"Focus", Op::IsFalse, 0}}, -1.0f, false});
    // One-shots return to locomotion (or guard) when finished.
    for (int from : {light, heavy, dash, fatal, hit}) {
        machine.AddTransition({from, guard, 0.12f, {{"Guard", Op::IsTrue, 0}}, 1.0f, false});
        machine.AddTransition({from, move, 0.18f, {}, 1.0f, false});
    }
    return machine;
}

} // namespace Astral::Animation
