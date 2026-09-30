#include "Game/Samples/Playground/PlaygroundBehaviours.h"

#include "Engine/Framework/Components.h"
#include "Engine/Framework/GameHost.h"
#include "Engine/Framework/GameWorld.h"
#include "Engine/Framework/Sequencer.h"
#include "Engine/Input/InputSystem.h"
#include "Engine/UI/Widgets.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace Astral::Samples {

using namespace Math;
using Framework::GameWorld;

namespace {

constexpr float kPi = 3.14159265358979f;
constexpr float kDegrees = kPi / 180.0f;

// Exponential smoothing factor for a rate (1/s) over dt, frame-rate independent.
float Smoothing(float rate, float dt) { return rate > 0.0f ? 1.0f - std::exp(-rate * dt) : 1.0f; }

// Yaw (radians) of a direction on the XZ plane; +Z is 0, +X is +pi/2.
float YawOf(Vec3 direction) { return std::atan2(direction.x, direction.z); }

Entity Resolve(GameWorld& world, Entity cached, const std::string& name) {
    if (world.IsAlive(cached)) return cached;
    return name.empty() ? Entity{} : world.Find(name);
}

std::shared_ptr<Audio::AudioClip> MakeChime() {
    auto clip = std::make_shared<Audio::AudioClip>();
    const int rate = clip->sampleRate;
    const int count = rate / 4;
    clip->samples.resize(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(rate);
        const float frequency = t < 0.08f ? 1046.5f : 1568.0f; // C6 then G6
        const float envelope = std::exp(-t * 14.0f) * std::min(1.0f, t * 400.0f);
        clip->samples[static_cast<std::size_t>(i)] = 0.35f * envelope * std::sin(2.0f * kPi * frequency * t);
    }
    return clip;
}

} // namespace

// ------------------------------------------------------------------ PlayerController

void PlayerController::OnStart() { spawn_ = World().GetWorld(Self()).translation; }

void PlayerController::OnFixedUpdate(float dt) {
    GameWorld& world = World();
    Framework::CharacterMover* mover = world.Get<Framework::CharacterMover>(Self());
    if (!mover) return;
    const TRS pose = world.GetWorld(Self());
    if (pose.translation.y < killHeight) {
        world.SetWorldPosition(Self(), spawn_); // the mover teleports with the transform
        mover->desiredVelocity = {};
        ++respawns_;
        return;
    }
    Input::InputSystem* input = world.Input();
    if (!input) return;

    // Camera-relative directions on the ground plane.
    Vec3 forward{0.0f, 0.0f, 1.0f};
    cameraEntity_ = Resolve(world, cameraEntity_, camera);
    if (!cameraEntity_.IsNull()) {
        forward = Normalize(Horizontal(Rotate(world.GetWorld(cameraEntity_).rotation, {0.0f, 0.0f, 1.0f})), forward);
    }
    const Vec3 right{forward.z, 0.0f, -forward.x};
    const Vec2 move = input->Axis("Move");
    Vec3 direction = right * move.x + forward * move.y;
    if (LengthSquared(direction) > 1.0f) direction = Normalize(direction);
    const float speed = moveSpeed * (input->Down("Sprint") ? sprintMultiplier : 1.0f);
    mover->desiredVelocity = direction * speed;
    if (mover->Grounded() && input->ConsumeBuffered("Jump")) {
        mover->jump = true;
        ++jumps_;
    }
    if (LengthSquared(direction) > 0.01f) {
        const Quat facing = QuatFromAxisAngle({0.0f, 1.0f, 0.0f}, YawOf(direction));
        world.SetWorldRotation(Self(), Slerp(pose.rotation, facing, Smoothing(turnSpeed, dt)));
    }
}

// ------------------------------------------------------------------ FollowCamera

void FollowCamera::OnLateUpdate(float dt) {
    GameWorld& world = World();
    targetEntity_ = Resolve(world, targetEntity_, target);
    if (targetEntity_.IsNull()) return;
    if (Input::InputSystem* input = world.Input()) {
        yawDegrees += input->Axis("Look").x * turnDegreesPerSecond * dt;
        yawDegrees = std::fmod(yawDegrees, 360.0f);
    }
    const Vec3 pivot = world.GetWorld(targetEntity_).translation + Vec3{0.0f, height, 0.0f};
    const Quat orbit = QuatFromEuler(yawDegrees * kDegrees, pitchDegrees * kDegrees, 0.0f);
    const Vec3 back = -Rotate(orbit, {0.0f, 0.0f, 1.0f});
    // Spring arm: pull in in front of walls between the pivot and the camera.
    float arm = distance;
    Framework::EntityRaycastHit hit;
    if (world.Raycast({pivot, back}, distance, hit, collisionMask, targetEntity_)) {
        arm = std::max(0.2f, hit.distance - wallMargin);
    }
    const Vec3 desired = pivot + back * arm;
    // Lag only lengthens the arm smoothly; pulling in for walls is immediate.
    if (!placed_ || arm < distance - 1.0e-3f) {
        position_ = desired;
        placed_ = true;
    } else {
        position_ = Lerp(position_, desired, Smoothing(lag, dt));
    }
    const Vec3 look = pivot - position_;
    const float flat = std::sqrt(look.x * look.x + look.z * look.z);
    world.SetWorldPosition(Self(), position_);
    world.SetWorldRotation(Self(), QuatFromEuler(YawOf(look), std::atan2(-look.y, std::max(flat, 1.0e-4f)), 0.0f));
}

// ------------------------------------------------------------------ Collectible

void Collectible::OnTriggerEnter(const Framework::CollisionInfo& info) {
    GameWorld& world = World();
    if (collected_ || !world.GetBehaviour<PlayerController>(info.other)) return;
    collected_ = true;
    const Entity modeEntity = world.Find("GameMode");
    if (GameMode* mode = modeEntity.IsNull() ? nullptr : world.GetBehaviour<GameMode>(modeEntity)) {
        mode->Collect(points, world.GetWorld(Self()).translation);
    }
    world.Destroy(Self());
}

// ------------------------------------------------------------------ JumpPad

void JumpPad::OnTriggerEnter(const Framework::CollisionInfo& info) {
    GameWorld& world = World();
    Framework::CharacterMover* mover = world.Get<Framework::CharacterMover>(info.other);
    if (!mover) return;
    const Vec3 carried = mover->controller ? Horizontal(mover->controller->Velocity()) * keepHorizontal : Vec3{};
    mover->launch = true;
    mover->launchVelocity = carried + Vec3{0.0f, launchSpeed, 0.0f};
    ++launches_;
    if (world.Get<Framework::ParticleSystem>(Self())) world.PlayParticles(Self());
}

// ------------------------------------------------------------------ Spinner, Bobber

void Spinner::OnUpdate(float dt) {
    TRS local = World().GetLocal(Self());
    local.rotation = Normalize(QuatFromAxisAngle(Normalize(axis, {0.0f, 1.0f, 0.0f}), degreesPerSecond * kDegrees * dt) * local.rotation);
    World().SetLocal(Self(), local);
}

void Bobber::OnStart() { base_ = World().GetLocal(Self()).translation; }

void Bobber::OnUpdate(float dt) {
    time_ += dt;
    TRS local = World().GetLocal(Self());
    local.translation = base_ + Vec3{0.0f, amplitude * std::sin(2.0f * kPi * (frequency * time_ + phase)), 0.0f};
    World().SetLocal(Self(), local);
}

// ------------------------------------------------------------------ GameMode

void GameMode::OnStart() {
    GameWorld& world = World();
    total_ = 0;
    for (Entity entity : world.Registry().EntitiesWith<Framework::Behaviours>()) {
        const Collectible* collectible = world.GetBehaviour<Collectible>(entity);
        if (collectible && !collectible->Collected() && world.IsAlive(entity)) ++total_;
    }
    chime_ = MakeChime();
    if (UI::CanvasPanel* hud = world.Hud()) {
        UI::Panel& panel = hud->Add<UI::Panel>("GameMode.panel");
        panel.useTheme = true;
        panel.padding = UI::Thickness::All(8.0f);
        panel.offsets = {16.0f, 16.0f, 0.0f, 0.0f};
        UI::StackPanel& stack = panel.Add<UI::StackPanel>("GameMode.stack");
        UI::Label& heading = stack.Add<UI::Label>(title, "GameMode.title");
        heading.scale = 2;
        status_ = &stack.Add<UI::Label>("", "GameMode.status");
        timer_ = &stack.Add<UI::Label>("", "GameMode.timer");
        panel_ = &panel;
        UI::Label& banner = hud->Add<UI::Label>("", "GameMode.banner");
        banner.scale = 3;
        banner.textAlign = UI::HAlign::Center;
        banner.anchorMin = banner.anchorMax = {0.5f, 0.35f};
        banner.pivot = {0.5f, 0.5f};
        banner.visible = false;
        banner_ = &banner;
    }
    Refresh();
}

void GameMode::OnUpdate(float dt) {
    if (!won_) elapsed_ += dt;
    Refresh();
}

void GameMode::OnDestroy() {
    if (panel_) panel_->RemoveFromParent();
    if (banner_) banner_->RemoveFromParent();
    panel_ = nullptr;
    status_ = timer_ = banner_ = nullptr;
}

void GameMode::Collect(int points, Vec3 position) {
    if (won_) return;
    score_ += points;
    ++collected_;
    World().PlaySoundAt(chime_.get(), position, 0.8f);
    if (collected_ < total_) return;
    won_ = true;
    if (banner_) {
        char text[96];
        std::snprintf(text, sizeof(text), "All %d collected in %.1f s!", total_, static_cast<double>(elapsed_));
        banner_->text = text;
        banner_->visible = true;
    }
    if (!winTimeline.empty()) {
        const Entity timelineEntity = World().Find(winTimeline);
        auto* timeline = timelineEntity.IsNull() ? nullptr : World().GetBehaviour<Framework::TimelineBehaviour>(timelineEntity);
        if (timeline && timeline->Player()) timeline->Player()->Play();
    }
    const std::string next = nextScene;
    Invoke(restartDelay, [this, next] { World().Events().Queue(Framework::LoadSceneRequest{next}); });
}

void GameMode::Refresh() {
    char text[64];
    if (status_) {
        std::snprintf(text, sizeof(text), "Stars %d / %d", collected_, total_);
        status_->text = text;
    }
    if (timer_) {
        std::snprintf(text, sizeof(text), "Time %.1f s", static_cast<double>(elapsed_));
        timer_->text = text;
    }
}

// ------------------------------------------------------------------ registration

void RegisterPlaygroundBehaviours() {
    Framework::BehaviourRegistry& registry = Framework::BehaviourRegistry::Global();
    Framework::RegisterFrameworkBehaviours(registry);
    registry.Register<PlayerController>("PlayerController")
        .Field("moveSpeed", &PlayerController::moveSpeed).Range(0, 100)
        .Field("sprintMultiplier", &PlayerController::sprintMultiplier).Range(1, 10)
        .Field("turnSpeed", &PlayerController::turnSpeed).Range(0, 100)
        .Field("camera", &PlayerController::camera, "entity whose view steers movement")
        .Field("killHeight", &PlayerController::killHeight);
    registry.Register<FollowCamera>("FollowCamera")
        .Field("target", &FollowCamera::target)
        .Field("distance", &FollowCamera::distance).Range(0.5, 100)
        .Field("height", &FollowCamera::height)
        .Field("pitchDegrees", &FollowCamera::pitchDegrees).Range(-89, 89)
        .Field("yawDegrees", &FollowCamera::yawDegrees)
        .Field("turnDegreesPerSecond", &FollowCamera::turnDegreesPerSecond)
        .Field("lag", &FollowCamera::lag).Range(0, 100)
        .Field("wallMargin", &FollowCamera::wallMargin).Range(0, 5)
        .Field("collisionMask", &FollowCamera::collisionMask);
    registry.Register<Collectible>("Collectible").Field("points", &Collectible::points);
    registry.Register<JumpPad>("JumpPad")
        .Field("launchSpeed", &JumpPad::launchSpeed).Range(0, 100)
        .Field("keepHorizontal", &JumpPad::keepHorizontal).Range(0, 2);
    registry.Register<Spinner>("Spinner").Field("degreesPerSecond", &Spinner::degreesPerSecond).Field("axis", &Spinner::axis);
    registry.Register<Bobber>("Bobber")
        .Field("amplitude", &Bobber::amplitude)
        .Field("frequency", &Bobber::frequency).Range(0, 100)
        .Field("phase", &Bobber::phase);
    registry.Register<GameMode>("GameMode")
        .Field("title", &GameMode::title)
        .Field("nextScene", &GameMode::nextScene, "scene loaded after winning (\"\" restarts)")
        .Field("restartDelay", &GameMode::restartDelay).Range(0, 600)
        .Field("winTimeline", &GameMode::winTimeline, "entity whose Timeline plays on winning");
}

} // namespace Astral::Samples
