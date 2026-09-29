#include "Game/Showcase/CharacterPresenter.h"

#include <cmath>

namespace Astral::Showcase {

using namespace Math;

CharacterPresenter::CharacterPresenter(const Animation::CharacterLook& look, std::uint32_t objectId, bool rimLit)
    : objectId_(objectId) {
    skeleton_ = Animation::BuildHumanoidSkeleton(joints_);
    clips_ = Animation::BuildHumanoidClips(skeleton_, joints_);
    machine_ = Animation::BuildShadowbladeStateMachine(clips_);
    animator_ = std::make_unique<Animation::Animator>(skeleton_, machine_);
    bind_ = Animation::BuildHumanoidMeshes(skeleton_, joints_, look);

    bodyMaterial_.baseColor = {1, 1, 1};
    bodyMaterial_.shadeThreshold = 0.1f;
    bodyMaterial_.shadeSoftness = 0.03f;
    bodyMaterial_.shadeColor = {0.6f, 0.55f, 0.8f};
    bodyMaterial_.specularIntensity = 0.15f;
    bodyMaterial_.rimIntensity = rimLit ? 0.16f : 0.08f;
    bodyMaterial_.rimWidth = 0.25f;
    bodyMaterial_.rimColor = look.glow;
    glowMaterial_.shading = Graphics::ShadingModel::Unlit;
    glowMaterial_.baseColor = look.glow;
    glowMaterial_.emissive = look.glow * 1.6f;
    glowMaterial_.outline = false;
    bladeMaterial_.baseColor = {1, 1, 1};
    bladeMaterial_.specularIntensity = 1.2f;
    bladeMaterial_.specularThreshold = 0.9f;
    bladeMaterial_.rimIntensity = 0.5f;
    bladeMaterial_.rimColor = look.glow;
    ghostMaterial_.shading = Graphics::ShadingModel::Unlit;
    ghostMaterial_.blend = Graphics::BlendMode::Additive;
    ghostMaterial_.baseColor = look.glow * 0.35f;
    ghostMaterial_.vertexColor = false;
    ghostMaterial_.rimIntensity = 1.2f;
    ghostMaterial_.rimColor = look.glow;
    ghostMaterial_.outline = false;
    trailMaterial_.shading = Graphics::ShadingModel::Unlit;
    trailMaterial_.blend = Graphics::BlendMode::Additive;
    trailMaterial_.baseColor = {1, 1, 1};
    trailMaterial_.doubleSided = true;
    trailMaterial_.outline = false;
    guardMaterial_.shading = Graphics::ShadingModel::Unlit;
    guardMaterial_.blend = Graphics::BlendMode::Additive;
    guardMaterial_.baseColor = {0.35f, 0.26f, 0.05f};
    guardMaterial_.opacity = 0.4f;
    guardMaterial_.rimIntensity = 1.6f;
    guardMaterial_.rimColor = {1.6f, 1.2f, 0.3f};
    guardMaterial_.outline = false;
    Graphics::MeshBuilder(guardMesh_).AddSphere({0, 0.95f, 0}, 1.05f, 16, 24);
    guardMesh_.ComputeBounds();
    Skin();
}

void CharacterPresenter::Skin() {
    Animation::ComputeSkinMatrices(skeleton_, animator_->GetPose(), skin_);
    Animation::SkinMesh(bind_.body, skin_, body_);
    Animation::SkinMesh(bind_.glow, skin_, glow_);
    Animation::SkinMesh(bind_.blade, skin_, blade_);
    animator_->GetPose().ToModel(skeleton_, model_);
}

void CharacterPresenter::SetTarget(Vec3 world) {
    if (IsFinite(world)) target_ = world;
}

void CharacterPresenter::Teleport(Vec3 world) {
    if (!IsFinite(world)) return;
    position_ = target_ = world;
    dashTime_ = dashDuration_ = 0.0f;
}

void CharacterPresenter::BeginDash(Vec3 from, Vec3 to, float duration) {
    dashFrom_ = from;
    dashTo_ = to;
    target_ = to;
    dashDuration_ = std::max(0.05f, duration);
    dashTime_ = 0.0f;
    afterimageTimer_ = 0.0f;
    const Vec3 delta = Horizontal(to - from);
    if (LengthSquared(delta) > 1.0e-4f) targetYaw_ = YawFromDirection(delta);
}

void CharacterPresenter::FaceTowards(Vec3 point) {
    const Vec3 delta = Horizontal(point - position_);
    if (LengthSquared(delta) > 1.0e-4f) targetYaw_ = YawFromDirection(delta);
}

Mat4 CharacterPresenter::World() const { return Translation(position_) * RotationY(yaw_); }

Vec3 CharacterPresenter::JointWorld(int joint) const {
    if (joint < 0 || static_cast<std::size_t>(joint) >= model_.size()) return position_;
    return TransformPoint(World(), GetTranslation(model_[static_cast<std::size_t>(joint)]));
}

Vec3 CharacterPresenter::BladeBase() const {
    const Mat4& m = model_[static_cast<std::size_t>(joints_.weapon)];
    return TransformPoint(World(), TransformPoint(m, {0, 0.12f, 0}));
}

Vec3 CharacterPresenter::BladeTip() const {
    const Mat4& m = model_[static_cast<std::size_t>(joints_.weapon)];
    return TransformPoint(World(), TransformPoint(m, {0, 1.05f, 0}));
}

void CharacterPresenter::Update(float dt, float time) {
    time_ = time;
    const Vec3 before = position_;
    if (dashTime_ < dashDuration_) {
        dashTime_ = std::min(dashDuration_, dashTime_ + dt);
        const float t = dashTime_ / dashDuration_;
        const float eased = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t); // fast start, soft landing
        position_ = Lerp(dashFrom_, dashTo_, eased);
        afterimageTimer_ -= dt;
        if (afterimageTimer_ <= 0.0f) {
            afterimageTimer_ = 0.035f;
            auto snapshot = std::make_shared<Graphics::MeshData>(body_);
            afterimages_.push_back({snapshot, World(), 0.0f});
            if (afterimages_.size() > 10) afterimages_.pop_front();
        }
    } else {
        // Critically damped follow of the gameplay position (movement is already bounded by gameplay).
        position_ = Lerp(position_, target_, DampFactor(18.0f, dt));
        if (DistanceSquared(position_, target_) > 25.0f) position_ = target_; // teleports snap
    }
    const Vec3 velocity = dt > 0.0f ? (position_ - before) / dt : Vec3{};
    const float horizontalSpeed = Length(Horizontal(velocity));
    speed_ = Lerp(speed_, dashTime_ < dashDuration_ ? 0.0f : horizontalSpeed, DampFactor(12.0f, dt));
    if (horizontalSpeed > 0.5f && dashTime_ >= dashDuration_) targetYaw_ = YawFromDirection(Horizontal(velocity));
    yaw_ += WrapAngle(targetYaw_ - yaw_) * DampFactor(14.0f, dt);
    animator_->SetFloat("Speed", speed_);
    animator_->Update(dt);
    Skin();
    for (Afterimage& a : afterimages_) a.age += dt;
    while (!afterimages_.empty() && afterimages_.front().age > 0.35f) afterimages_.pop_front();
    if (trailActive_) trail_.AddSample(BladeBase(), BladeTip(), time);
    trail_.Prune(time);
    hitFlash_ = std::max(0.0f, hitFlash_ - dt * 6.0f);
}

void CharacterPresenter::AppendDraws(Graphics::RenderScene& scene, bool guarding, float time) const {
    const Mat4 world = World();
    Graphics::DrawItem body{&body_, &bodyMaterial_, world, objectId_};
    body.tint = tint_;
    body.emissiveBoost = Vec3{1.0f, 0.9f, 0.8f} * hitFlash_;
    scene.draws.push_back(body);
    scene.draws.push_back({&glow_, &glowMaterial_, world, objectId_});
    scene.draws.push_back({&blade_, &bladeMaterial_, world, objectId_ + 1});
    for (const Afterimage& a : afterimages_) {
        Graphics::DrawItem ghost{a.body.get(), &ghostMaterial_, a.world, 0};
        ghost.opacity = 0.55f * (1.0f - a.age / 0.35f);
        scene.draws.push_back(ghost);
    }
    if (!trail_.Empty()) {
        trail_.BuildMesh(time, {1.6f, 1.0f, 3.2f, 1.0f}, {0.5f, 0.2f, 1.4f, 1.0f}, trailMesh_);
        scene.draws.push_back({&trailMesh_, &trailMaterial_, Mat4::Identity(), 0});
    }
    if (guarding) {
        Graphics::DrawItem bubble{&guardMesh_, &guardMaterial_, Translation(position_), 0};
        bubble.opacity = 0.7f + 0.3f * std::sin(time * 9.0f);
        scene.draws.push_back(bubble);
    }
}

} // namespace Astral::Showcase
