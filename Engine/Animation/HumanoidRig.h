#pragma once

// Procedural humanoid used for engine fixtures and the Shadowblade showcase: a
// 21-joint skeleton (+ weapon socket), a rigidly skinned stylised anime body
// built from primitives, and a hand-keyed action clip set with gameplay
// notifies (Swing, Hit windows, Impact, Afterimage). No external assets.

#include "Engine/Animation/Animator.h"
#include "Engine/Graphics/Mesh.h"

namespace Astral::Animation {

struct HumanoidJoints {
    int root{-1}, pelvis{-1}, spine{-1}, chest{-1}, neck{-1}, head{-1};
    int shoulderL{-1}, upperArmL{-1}, forearmL{-1}, handL{-1};
    int shoulderR{-1}, upperArmR{-1}, forearmR{-1}, handR{-1};
    int thighL{-1}, shinL{-1}, footL{-1};
    int thighR{-1}, shinR{-1}, footR{-1};
    int weapon{-1};
};

Skeleton BuildHumanoidSkeleton(HumanoidJoints& joints);

struct HumanoidClips {
    AnimationClip idle, walk, run, lightAttack, heavyAttack, dash, guard, fatalStrike, focus, hitReact, defeat;
};
HumanoidClips BuildHumanoidClips(const Skeleton& skeleton, const HumanoidJoints& joints);

// 1 for spine/arms/head/weapon, 0 for pelvis and legs.
std::vector<float> UpperBodyMask(const Skeleton& skeleton, const HumanoidJoints& joints);

struct CharacterLook {
    Graphics::Color skin{0.98f, 0.82f, 0.72f};
    Graphics::Color hair{0.86f, 0.88f, 0.96f};
    Graphics::Color eyes{0.45f, 0.22f, 0.85f};
    Graphics::Color coat{0.08f, 0.08f, 0.16f};
    Graphics::Color coatInner{0.22f, 0.12f, 0.36f};
    Graphics::Color pants{0.06f, 0.06f, 0.09f};
    Graphics::Color boots{0.12f, 0.1f, 0.12f};
    Graphics::Color scarf{0.45f, 0.18f, 0.75f};
    Graphics::Color glow{0.75f, 0.45f, 1.0f};
    Graphics::Color blade{0.82f, 0.86f, 0.95f};
    bool longCoat{true};
    bool spikyHair{true};
};

struct HumanoidMeshes {
    Graphics::MeshData body;  // toon-shaded, vertex coloured
    Graphics::MeshData glow;  // emissive accents
    Graphics::MeshData blade; // weapon (separate material)
};
HumanoidMeshes BuildHumanoidMeshes(const Skeleton& skeleton, const HumanoidJoints& joints, const CharacterLook& look);

// Standard Shadowblade locomotion/action state machine over the clip set.
// Parameters: float Speed; bool Guard, Focus, Defeated; triggers Light, Heavy,
// Dash, Fatal, Hit.
AnimStateMachine BuildShadowbladeStateMachine(const HumanoidClips& clips);

} // namespace Astral::Animation
