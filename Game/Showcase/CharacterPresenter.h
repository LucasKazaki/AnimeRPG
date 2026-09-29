#pragma once

// Animated, skinned presentation of a gameplay character. Gameplay decides
// where the character is and what it did; the presenter turns that into
// animation state, smooth facing, a visual dash with afterimages, weapon
// ribbon trails, a guard bubble and notify-timed feedback.

#include "Engine/Animation/HumanoidRig.h"
#include "Engine/Animation/Skinning.h"
#include "Engine/Graphics/RenderScene.h"
#include "Engine/VFX/Particles.h"

#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace Astral::Showcase {

class CharacterPresenter {
public:
    CharacterPresenter(const Animation::CharacterLook& look, std::uint32_t objectId, bool rimLit);

    // Desired ground position from gameplay (world space).
    void SetTarget(Math::Vec3 worldPosition);
    void Teleport(Math::Vec3 worldPosition);
    // Visual dash from->to over `duration` seconds with afterimages.
    void BeginDash(Math::Vec3 from, Math::Vec3 to, float duration);
    void FaceTowards(Math::Vec3 worldPoint);
    void Update(float dt, float time);
    void AppendDraws(Graphics::RenderScene& scene, bool guarding, float time) const;

    Animation::Animator& Anim() { return *animator_; }
    const Animation::Animator& Anim() const { return *animator_; }
    const Animation::HumanoidClips& Clips() const { return clips_; }
    const Animation::Skeleton& Rig() const { return skeleton_; }
    const Animation::HumanoidJoints& Joints() const { return joints_; }
    Math::Vec3 Position() const { return position_; }
    float Yaw() const { return yaw_; }
    bool Dashing() const { return dashTime_ < dashDuration_; }
    Math::Mat4 World() const;
    Math::Vec3 JointWorld(int joint) const;
    Math::Vec3 BladeBase() const;
    Math::Vec3 BladeTip() const;
    float Speed() const { return speed_; }
    void StartTrail() { trailActive_ = true; }
    void StopTrail() { trailActive_ = false; }
    void SetTint(Graphics::Color tint) { tint_ = tint; }
    void FlashHit() { hitFlash_ = 1.0f; }

private:
    struct Afterimage {
        std::shared_ptr<Graphics::MeshData> body;
        Math::Mat4 world;
        float age{};
    };
    void Skin();

    Animation::HumanoidJoints joints_;
    Animation::Skeleton skeleton_;
    Animation::HumanoidClips clips_;
    Animation::AnimStateMachine machine_;
    std::unique_ptr<Animation::Animator> animator_;
    Animation::HumanoidMeshes bind_;
    Graphics::MeshData body_, glow_, blade_;
    std::vector<Math::Mat4> skin_;
    std::vector<Math::Mat4> model_;
    Graphics::Material bodyMaterial_, glowMaterial_, bladeMaterial_, ghostMaterial_, trailMaterial_, guardMaterial_;
    Graphics::MeshData guardMesh_;
    mutable Graphics::MeshData trailMesh_;
    VFX::RibbonTrail trail_{0.16f};
    std::deque<Afterimage> afterimages_;
    std::uint32_t objectId_;
    Math::Vec3 position_{};
    Math::Vec3 target_{};
    float yaw_{};
    float targetYaw_{};
    float speed_{};
    Math::Vec3 dashFrom_{}, dashTo_{};
    float dashTime_{1.0f}, dashDuration_{0.0f};
    float afterimageTimer_{};
    bool trailActive_{};
    float time_{};
    Graphics::Color tint_{1, 1, 1};
    float hitFlash_{};
};

} // namespace Astral::Showcase
