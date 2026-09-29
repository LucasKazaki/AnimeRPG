#include "Game/Showcase/MallShowcase.h"

#include "Engine/Core/Profiler.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>

namespace Astral::Showcase {

using namespace Math;
using Graphics::Canvas;
using Graphics::Rgba8;
using Graphics::TextStyle;

namespace {

constexpr int kFocusChannel = 0;

const char* LandmarkTitle(Scene::LandmarkKind kind) {
    switch (kind) {
    case Scene::LandmarkKind::LincolnMemorial: return "LINCOLN MEMORIAL";
    case Scene::LandmarkKind::ReflectingPool: return "REFLECTING POOL";
    case Scene::LandmarkKind::WashingtonMonument: return "WASHINGTON MONUMENT";
    }
    return "LANDMARK";
}

const char* ReasonText(Scene::ThoughtCommandReason reason) {
    switch (reason) {
    case Scene::ThoughtCommandReason::None: return "";
    case Scene::ThoughtCommandReason::Empty: return "EMPTY";
    case Scene::ThoughtCommandReason::Ambiguous: return "AMBIGUOUS";
    case Scene::ThoughtCommandReason::Unsupported: return "UNSUPPORTED";
    case Scene::ThoughtCommandReason::NoOp: return "NO OP";
    case Scene::ThoughtCommandReason::GuardedConflict: return "BLOCKED BY GUARD";
    case Scene::ThoughtCommandReason::Cooldown: return "COOLDOWN";
    case Scene::ThoughtCommandReason::InsufficientResource: return "NO RESOURCE";
    case Scene::ThoughtCommandReason::OutOfRange: return "OUT OF RANGE";
    case Scene::ThoughtCommandReason::TargetDefeated: return "TARGET DEFEATED";
    }
    return "UNKNOWN";
}

const char* ShadowResultText(Scene::ShadowActionResult result) {
    switch (result) {
    case Scene::ShadowActionResult::Cooldown: return "COOLDOWN";
    case Scene::ShadowActionResult::InsufficientResource: return "NO RESOURCE";
    case Scene::ShadowActionResult::OutOfRange: return "OUT OF RANGE";
    case Scene::ShadowActionResult::GuardedConflict: return "BLOCKED BY GUARD";
    case Scene::ShadowActionResult::TargetDefeated: return "TARGET DEFEATED";
    default: return "";
    }
}

bool Project(const Graphics::RenderView& view, Vec3 world, float& x, float& y) {
    const Vec4 clip = view.ViewProjection() * Vec4{world.x, world.y, world.z, 1.0f};
    if (clip.w <= view.nearPlane) return false;
    x = (clip.x / clip.w * 0.5f + 0.5f) * static_cast<float>(view.width);
    y = (0.5f - clip.y / clip.w * 0.5f) * static_cast<float>(view.height);
    return std::isfinite(x) && std::isfinite(y);
}

Animation::CharacterLook DummyLook() {
    Animation::CharacterLook look;
    look.skin = {0.22f, 0.2f, 0.26f};
    look.hair = {0.16f, 0.14f, 0.2f};
    look.eyes = {1.4f, 0.4f, 0.2f};
    look.coat = {0.3f, 0.26f, 0.3f};
    look.coatInner = {0.5f, 0.2f, 0.12f};
    look.pants = {0.2f, 0.18f, 0.22f};
    look.boots = {0.12f, 0.1f, 0.12f};
    look.scarf = {0.7f, 0.25f, 0.12f};
    look.glow = {1.0f, 0.42f, 0.3f};
    look.blade = {0.6f, 0.55f, 0.55f};
    look.spikyHair = false;
    look.longCoat = false;
    return look;
}

} // namespace

MallShowcase::MallShowcase(const Scene::WorldBlockout& blockout, Core::JobSystem* jobs, ShowcaseSettings settings)
    : settings_(settings), jobs_(jobs), renderer_(jobs), mixer_(48000, 32) {
    scene_ = std::make_unique<MallScene>(blockout, physics_);
    Physics::CharacterSettings character;
    character.groundAcceleration = 1000.0f; // gameplay already shapes movement; stay 1:1
    character.airAcceleration = 1000.0f;
    controller_ = std::make_unique<Physics::CharacterController>(physics_, character, Vec3{0, 0, 0});
    player_ = std::make_unique<CharacterPresenter>(Animation::CharacterLook{}, ObjectIds::Player, true);
    target_ = std::make_unique<CharacterPresenter>(DummyLook(), ObjectIds::Dummy, false);
    player_->Teleport(controller_->FootPosition());
    renderer_.Settings() = Graphics::RendererSettings::Preset(settings_.quality);

    frame_.sun.direction = Normalize(Vec3{-0.45f, -0.62f, 0.62f});
    frame_.sun.color = {1.0f, 0.93f, 0.84f};
    frame_.sun.intensity = 2.3f;
    frame_.ambientSky = {0.34f, 0.38f, 0.52f};
    frame_.ambientGround = {0.22f, 0.2f, 0.18f};
    frame_.sky.zenith = {0.16f, 0.34f, 0.82f};
    frame_.sky.horizon = {0.76f, 0.84f, 0.96f};
    frame_.fog.color = {0.72f, 0.8f, 0.93f};
    frame_.fog.density = 0.0045f;
    frame_.shadows.radius = 26.0f;
    frame_.shadows.resolution = 2048;
    frame_.post.exposure = 1.0f;
    frame_.post.bloomThreshold = 1.2f;
    frame_.post.bloomIntensity = 0.3f;

    swingClip_ = Audio::Synth::Whoosh(0.22f, 900.0f, 3200.0f, 3);
    heavyClip_ = Audio::Synth::Whoosh(0.4f, 300.0f, 1800.0f, 4);
    dashClip_ = Audio::Synth::Whoosh(0.3f, 2400.0f, 600.0f, 5);
    hitClip_ = Audio::Synth::Impact(0.25f, 140.0f, 6);
    impactClip_ = Audio::Synth::Impact(0.6f, 55.0f, 7);
    chimeClip_ = Audio::Synth::Chime(784.0f, 1.6f);
    droneClip_ = Audio::Synth::Drone(55.0f, 4.0f);
    stepClip_ = Audio::Synth::Footstep(8);
    crystalClip_ = Audio::Synth::Chime(1320.0f, 0.8f);
}

Vec3 MallShowcase::ResolveMovement(Vec3 from, Vec3 to, bool dash) {
    if (!settings_.collision || !IsFinite(from) || !IsFinite(to)) return to;
    Vec3 foot = controller_->FootPosition();
    if (std::fabs(foot.x - from.x) > 0.05f || std::fabs(foot.z - from.y) > 0.05f) {
        controller_->Teleport({from.x, foot.y, from.y});
        foot = controller_->FootPosition();
    }
    const Vec3 targetWorld{to.x, foot.y, to.y};
    if (dash) {
        if (settings_.dashPhasesThroughWalls) controller_->Teleport(targetWorld);
        else controller_->SweepTo(targetWorld);
    } else {
        const Vec3 delta = Horizontal(targetWorld - foot);
        if (LengthSquared(delta) > 1.0e-10f) {
            constexpr float kStep = 1.0f / 120.0f;
            controller_->Move(delta / kStep, false, kStep);
        } else {
            controller_->Move({}, false, 1.0f / 120.0f);
        }
    }
    const Vec3 resolved = controller_->FootPosition();
    return {resolved.x, resolved.z, 0.0f};
}

void MallShowcase::Cue(const Audio::AudioClip& clip, float volume, float pitch, Vec3 position, bool spatial) {
    Audio::PlayParams params;
    params.volume = volume;
    params.pitch = pitch;
    params.spatial = spatial;
    params.position = position;
    params.minDistance = 3.0f;
    params.maxDistance = 60.0f;
    mixer_.Play(&clip, params);
}

void MallShowcase::ApplyHitFeedback(const PendingHit& hit) {
    const Vec3 at = target_->Position() + Vec3{0, 1.1f, 0};
    const Vec3 direction = Normalize(Horizontal(target_->Position() - player_->Position()), {1, 0, 0});
    VFX::ParticleWorld& particles = scene_->Particles();
    particles.Spawn(VFX::Presets::SlashSparks(direction), at);
    particles.Spawn(VFX::Presets::HitBurst(), at);
    if (hit.fatal) particles.Spawn(VFX::Presets::ShadowSmoke(), at - Vec3{0, 0.8f, 0});
    time_.TriggerHitstop(hit.fatal ? 0.14f : (hit.heavy ? 0.09f : 0.05f), 0.02f);
    shake_ = std::min(1.0f, shake_ + (hit.fatal ? 0.9f : (hit.heavy ? 0.55f : 0.25f)));
    target_->FlashHit();
    target_->Anim().SetTrigger("Hit");
    Rgba8 color = hit.fatal ? Rgba8{200, 150, 255, 255} : (hit.heavy ? Rgba8{255, 190, 90, 255} : Rgba8{255, 240, 200, 255});
    floating_.push_back({std::to_string(hit.damage), at + Vec3{0, 0.6f, 0}, 0.0f, color, hit.fatal ? 5 : (hit.heavy ? 4 : 3)});
    Cue(hit.fatal || hit.heavy ? impactClip_ : hitClip_, hit.fatal ? 1.0f : 0.7f, 1.0f, at);
    if (hit.heavy || hit.fatal) {
        const int broken = scene_->ShatterCrystals(player_->Position(), hit.fatal ? 6.0f : 3.0f, direction, hit.fatal ? 100.0f : 40.0f);
        if (broken > 0) Cue(crystalClip_, 0.6f, 1.2f, player_->Position());
    }
}

void MallShowcase::HandleEvents(const FrameEvents& e) {
    VFX::ParticleWorld& particles = scene_->Particles();
    const Vec3 dummyWorld = target_->Position();
    auto nearDummy = [&] { return DistanceSquared(player_->Position(), dummyWorld) < 49.0f; };
    auto rejection = [&](const char* text) {
        if (text && *text) floating_.push_back({text, player_->Position() + Vec3{0, 2.2f, 0}, 0.0f, {170, 180, 200, 255}, 2});
    };
    if (e.lightAttack || e.heavyAttack) {
        player_->Anim().SetTrigger(e.heavyAttack ? "Heavy" : "Light");
        if (nearDummy()) player_->FaceTowards(dummyWorld);
        if (e.attack.result == Scene::AttackResult::Hit) {
            pendingHits_.push_back({e.attack.damageApplied, e.heavyAttack, false, 0.45f});
        } else if (e.attack.result == Scene::AttackResult::Cooldown) {
            rejection("COOLDOWN");
        } else if (e.attack.result == Scene::AttackResult::OutOfRange) {
            rejection("OUT OF RANGE");
        } else if (e.attack.result == Scene::AttackResult::TargetDefeated) {
            rejection("TARGET DEFEATED");
        }
    }
    if (e.dash) {
        if (e.dashReport.result == Scene::ShadowActionResult::Activated) {
            const Vec3 from = player_->Position();
            const Vec3 to = controller_ && settings_.collision ? controller_->FootPosition() : ToWorld(e.dashTo);
            player_->BeginDash(from, to, 0.22f);
            player_->Anim().SetTrigger("Dash");
            const Vec3 direction = Normalize(Horizontal(to - from), {0, 0, 1});
            particles.Spawn(VFX::Presets::DashStreaks(direction), from + Vec3{0, 1.0f, 0});
            particles.Spawn(VFX::Presets::ShadowSmoke(), from + Vec3{0, 0.4f, 0});
            particles.Spawn(VFX::Presets::ShadowSmoke(), to + Vec3{0, 0.4f, 0});
            Cue(dashClip_, 0.7f, 1.0f, from);
            fovKick_ = 8.0f;
        } else {
            rejection(ShadowResultText(e.dashReport.result));
        }
    }
    if (e.fatalStrike) {
        if (e.fatal.result == Scene::ShadowActionResult::Activated) {
            player_->Anim().SetTrigger("Fatal");
            player_->FaceTowards(dummyWorld);
            pendingHits_.push_back({e.fatal.damageApplied, false, true, 0.7f});
            floating_.push_back({"FATAL STRIKE", player_->Position() + Vec3{0, 2.6f, 0}, 0.0f, {220, 170, 255, 255}, 4});
        } else {
            rejection(ShadowResultText(e.fatal.result));
        }
    }
    if (e.thoughtCommand) {
        const bool accepted = e.thought.status == Scene::ThoughtCommandStatus::Accepted;
        std::string text = "THOUGHT: ";
        for (char c : e.thought.submitted) text.push_back(static_cast<char>(c >= 'a' && c <= 'z' ? c - 32 : c));
        if (!accepted) text += "  REJECTED " + std::string(ReasonText(e.thought.reason));
        else if (e.thought.type == Scene::ThoughtCommandType::Focus && view_.thought)
            text += view_.thought->IsFocusActive() ? " ON" : " OFF";
        floating_.push_back({text, player_->Position() + Vec3{0, 2.4f, 0}, 0.0f,
            accepted ? Rgba8{200, 170, 255, 255} : Rgba8{170, 170, 190, 255}, 2});
        if (accepted && e.thought.type == Scene::ThoughtCommandType::Fatal
            && e.thought.shadowAction.result == Scene::ShadowActionResult::Activated) {
            player_->Anim().SetTrigger("Fatal");
            pendingHits_.push_back({e.thought.shadowAction.damageApplied, false, true, 0.7f});
        }
    }
    if (e.interacted) {
        const auto result = e.interaction.result;
        if (result == Scene::LandmarkInteractionResult::Discovered || result == Scene::LandmarkInteractionResult::ObjectiveAdvanced) {
            ShowBanner(result == Scene::LandmarkInteractionResult::Discovered ? "DISCOVERED" : "OBJECTIVE",
                LandmarkTitle(e.interaction.landmark));
            VFX::EmitterSettings burst = VFX::Presets::HitBurst();
            burst.colors = {{0.0f, {0.6f, 2.4f, 3.0f, 1}}, {1.0f, {0.4f, 0.8f, 2.4f, 0}}};
            burst.burst = 60;
            burst.speedMax = 5.0f;
            particles.Spawn(burst, player_->Position() + Vec3{0, 1.2f, 0});
            Cue(chimeClip_, 0.8f, 1.0f, player_->Position(), false);
        } else if (result == Scene::LandmarkInteractionResult::AlreadyVisited) {
            rejection("ALREADY VISITED");
        }
    }
    if (e.encounterChanged && view_.encounter) {
        const auto state = view_.encounter->State();
        if (state == Scene::LandmarkEncounterState::Active) {
            ShowBanner("ENCOUNTER", "DEFEAT THE RIFT WRAITH AT THE TRAINING GROUND");
        } else if (state == Scene::LandmarkEncounterState::Completed) {
            ShowBanner("ENCOUNTER COMPLETE", "REWARD GRANTED");
            Cue(chimeClip_, 0.9f, 1.25f, player_->Position(), false);
        }
    }
}

void MallShowcase::Update(const GameplayView& view, const FrameEvents& events, Vec3 playerController, float realDelta) {
    ASTRAL_PROFILE_SCOPE("Showcase.Update");
    view_ = view;
    if (!std::isfinite(realDelta) || realDelta < 0.0f) realDelta = 0.0f;
    realDelta = std::min(realDelta, 0.1f);
    const bool focus = view.thought && view.thought->IsFocusActive();
    const bool guarding = view.shadowblade && view.shadowblade->IsGuarding();
    time_.SetDilation(kFocusChannel, focus ? Scene::ThoughtCommands::FocusTimeMultiplier : 1.0f, 0.18f);
    time_.Advance(realDelta);
    const float dt = time_.WorldDelta();
    const float now = static_cast<float>(time_.RealSeconds());
    focusBlend_ = Lerp(focusBlend_, focus ? 1.0f : 0.0f, DampFactor(8.0f, realDelta));

    // Player follows the resolved physical position (or raw gameplay position without collision).
    Vec3 playerWorld = ToWorld(playerController);
    if (settings_.collision) {
        const Vec3 foot = controller_->FootPosition();
        if (std::fabs(foot.x - playerWorld.x) < 0.1f && std::fabs(foot.z - playerWorld.z) < 0.1f) playerWorld.y = foot.y;
        else {
            controller_->Teleport(playerWorld);
            playerWorld = controller_->FootPosition();
        }
    }
    player_->SetTarget(playerWorld);
    HandleEvents(events);
    player_->Anim().SetBool("Guard", guarding);
    player_->Anim().SetBool("Focus", focus && player_->Speed() < 0.5f);

    // Training target mirrors the combat domain.
    if (view.combat && view.world) {
        const Scene::TrainingDummy& dummy = view.combat->Dummy();
        target_->SetTarget(view.world->GroundPosition(dummy.position));
        if (lastDummyHealth_ < 0) target_->Teleport(view.world->GroundPosition(dummy.position));
        target_->FaceTowards(player_->Position());
        const bool defeated = dummy.IsDefeated();
        target_->Anim().SetBool("Defeated", defeated);
        target_->Anim().SetBool("Guard", !defeated);
        if (dummyWasDefeated_ && !defeated) target_->Anim().ForceState("Locomotion");
        if (defeated && !dummyWasDefeated_) {
            floating_.push_back({"DEFEATED", target_->Position() + Vec3{0, 2.4f, 0}, 0.0f, {255, 120, 90, 255}, 4});
            Cue(impactClip_, 0.8f, 0.7f, target_->Position());
        }
        dummyWasDefeated_ = defeated;
        lastDummyHealth_ = dummy.health;
        Graphics::Color tint{1, 1, 1};
        if (view.encounter) {
            const auto state = view.encounter->State();
            if (state == Scene::LandmarkEncounterState::Active) tint = {1.25f, 0.9f, 0.8f};
            else if (state == Scene::LandmarkEncounterState::Completed) tint = {0.8f, 1.15f, 0.9f};
        }
        target_->SetTint(tint);
    }

    player_->Update(dt, now);
    target_->Update(dt, now);

    // Notify-driven feedback: trails on swings, hits land on the Hit frame.
    for (const Animation::AnimEvent& event : player_->Anim().Events()) {
        if (event.name == "Swing") {
            player_->StartTrail();
            Cue(event.state == "HeavyAttack" || event.state == "FatalStrike" ? heavyClip_ : swingClip_, 0.6f, 1.0f, player_->Position());
        } else if ((event.name == "Hit" || event.name == "Impact") && !pendingHits_.empty()) {
            ApplyHitFeedback(pendingHits_.front());
            pendingHits_.erase(pendingHits_.begin());
        } else if (event.name == "Footstep") {
            Cue(stepClip_, 0.25f, 0.9f + 0.2f * std::sin(now * 13.0f), player_->Position());
        }
    }
    const std::string& state = player_->Anim().CurrentStateName();
    if (state != "LightAttack" && state != "HeavyAttack" && state != "FatalStrike") player_->StopTrail();
    for (PendingHit& hit : pendingHits_) hit.timeout -= realDelta;
    while (!pendingHits_.empty() && pendingHits_.front().timeout <= 0.0f) {
        ApplyHitFeedback(pendingHits_.front());
        pendingHits_.erase(pendingHits_.begin());
    }

    // Persistent effects follow their owners.
    VFX::ParticleWorld& particles = scene_->Particles();
    if (focus && !focusAuraActive_) {
        focusAura_ = particles.Spawn(VFX::Presets::FocusAura(), player_->Position());
        focusAuraActive_ = true;
        Audio::PlayParams drone;
        drone.loop = true;
        drone.volume = 0.5f;
        drone.bus = Audio::Bus::Sfx;
        droneVoice_ = mixer_.Play(&droneClip_, drone);
        mixer_.Duck(Audio::Bus::Music, 0.6f, 1.0f);
    } else if (!focus && focusAuraActive_) {
        particles.Stop(focusAura_);
        focusAuraActive_ = false;
        mixer_.Stop(droneVoice_);
    }
    if (focusAuraActive_) {
        if (VFX::ParticleEmitter* aura = particles.Get(focusAura_)) aura->position = player_->Position();
    }
    if (guarding && !guardShimmerActive_) {
        guardShimmer_ = particles.Spawn(VFX::Presets::GuardShimmer(), player_->Position() + Vec3{0, 1, 0});
        guardShimmerActive_ = true;
    } else if (!guarding && guardShimmerActive_) {
        particles.Stop(guardShimmer_);
        guardShimmerActive_ = false;
    }
    if (guardShimmerActive_) {
        if (VFX::ParticleEmitter* shimmer = particles.Get(guardShimmer_)) shimmer->position = player_->Position() + Vec3{0, 1, 0};
    }

    scene_->Update(dt, now);
    for (int step = 0; step < time_.FixedStepsThisFrame(); ++step) physics_.Step(time_.FixedStepSeconds());

    for (FloatingText& text : floating_) {
        text.age += realDelta;
        text.position.y += realDelta * 0.8f;
    }
    floating_.erase(std::remove_if(floating_.begin(), floating_.end(), [](const FloatingText& t) { return t.age > 1.4f; }),
        floating_.end());
    bannerTime_ += realDelta;
    shake_ = std::max(0.0f, shake_ - realDelta * 2.2f);
    fovKick_ = std::max(0.0f, fovKick_ - realDelta * 40.0f);
    UpdateCamera(realDelta);
    mixer_.SetListener(cameraEye_, Normalize(cameraTarget_ - cameraEye_, {0, 0, 1}));
    stats_.particles = particles.ParticleCount();
    stats_.physicsBodies = physics_.BodyCount();
}

void MallShowcase::UpdateCamera(float dt) {
    const Vec3 focusPoint = player_->Position() + Vec3{0.0f, 1.35f, 0.0f};
    const Vec3 desiredEye = player_->Position() + Vec3{0.0f, 3.1f - focusBlend_ * 0.5f, -6.8f + focusBlend_ * 1.6f};
    // Spring arm: pull in when the Mall's geometry would block the view.
    Vec3 eye = desiredEye;
    Physics::ShapeCastHit hit;
    const Vec3 arm = desiredEye - focusPoint;
    if (physics_.ShapeCast(Physics::Shape::Sphere(0.25f), {focusPoint, {}}, arm, hit, CollisionLayers::World)
        && !hit.startPenetrating) {
        // Stop just short of the contact, never beyond it (UE-style spring arm).
        const float length = std::max(Length(arm), 1.0e-4f);
        eye = focusPoint + arm * (std::max(0.0f, hit.distance - 0.05f) / length);
    }
    cameraEye_ = Lerp(cameraEye_, eye, DampFactor(10.0f, dt));
    cameraTarget_ = Lerp(cameraTarget_, focusPoint + Vec3{0.0f, -0.2f, 3.0f}, DampFactor(12.0f, dt));
    cameraFov_ = 52.0f - 7.0f * focusBlend_ + fovKick_;
}

Graphics::RenderView MallShowcase::MakeView(int width, int height) const {
    Vec3 eye = cameraEye_, target = cameraTarget_;
    if (shake_ > 0.0f) {
        const float t = static_cast<float>(time_.RealSeconds());
        const float amount = shake_ * shake_ * 0.18f;
        const Vec3 jitter{std::sin(t * 61.0f) * amount, std::sin(t * 47.0f + 1.3f) * amount, std::sin(t * 53.0f + 2.1f) * amount * 0.5f};
        eye += jitter;
        target += jitter * 0.5f;
    }
    return Graphics::RenderView::Perspective(eye, target, cameraFov_, width, height, 0.2f, 400.0f);
}

void MallShowcase::CycleQuality() {
    settings_.quality = (settings_.quality + 1) % 4;
    renderer_.Settings() = Graphics::RendererSettings::Preset(settings_.quality);
}

const Graphics::ImageRgba8& MallShowcase::Render(int outputWidth, int outputHeight) {
    ASTRAL_PROFILE_SCOPE("Showcase.Render");
    outputWidth = std::max(64, outputWidth);
    outputHeight = std::max(36, outputHeight);
    const auto start = std::chrono::steady_clock::now();
    const float scale = settings_.dynamicResolution ? resolutionScale_ : Clamp(settings_.fixedResolutionScale, 0.25f, 1.0f);
    const int width = std::max(64, static_cast<int>(std::lround(static_cast<float>(outputWidth) * scale)));
    const int height = std::max(36, static_cast<int>(std::lround(static_cast<float>(outputHeight) * scale)));

    frame_.draws.clear();
    frame_.billboards.clear();
    frame_.lines.clear();
    frame_.time = static_cast<float>(time_.WorldSeconds());
    frame_.sky.cloudRotation = static_cast<float>(time_.RealSeconds()) * 0.004f;
    frame_.shadows.focus = player_->Position() + Vec3{0, 0, 8.0f};
    // Thought Focus grade: desaturated violet world, vignette, subtle fringing.
    frame_.post.saturation = Lerp(1.08f, 0.38f, focusBlend_);
    frame_.post.tint = Lerp(Vec3{1.0f, 1.0f, 1.0f}, Vec3{0.86f, 0.78f, 1.12f}, focusBlend_);
    frame_.post.vignette = Lerp(0.2f, 0.6f, focusBlend_);
    frame_.post.chromaticAberration = 2.5f * focusBlend_;
    frame_.post.contrast = Lerp(1.05f, 1.15f, focusBlend_);
    scene_->AppendDraws(frame_);
    const bool guarding = view_.shadowblade && view_.shadowblade->IsGuarding();
    player_->AppendDraws(frame_, guarding, static_cast<float>(time_.RealSeconds()));
    target_->AppendDraws(frame_, false, static_cast<float>(time_.RealSeconds()));

    Graphics::RenderView view3d = MakeView(width, height);
    view3d.occlusionFocus = player_->Position() + Vec3{0.0f, 1.0f, 0.0f};
    view3d.occlusionRadius = 1.6f;
    renderer_.Render(frame_, view3d, target3d_);

    // Upscale to the output resolution (bilinear), then draw the HUD crisply on top.
    const Graphics::ImageRgba8& source = target3d_.output;
    if (source.width == outputWidth && source.height == outputHeight) {
        output_ = source;
    } else {
        output_.Resize(outputWidth, outputHeight);
        auto rows = [&](std::size_t begin, std::size_t end) {
            for (std::size_t row = begin; row < end; ++row) {
                const int y = static_cast<int>(row);
                const float sy = Clamp((static_cast<float>(y) + 0.5f) * static_cast<float>(source.height) / static_cast<float>(outputHeight) - 0.5f,
                    0.0f, static_cast<float>(source.height - 1));
                const int y0 = static_cast<int>(sy), y1 = std::min(source.height - 1, y0 + 1);
                const float fy = sy - static_cast<float>(y0);
                for (int x = 0; x < outputWidth; ++x) {
                    const float sx = Clamp((static_cast<float>(x) + 0.5f) * static_cast<float>(source.width) / static_cast<float>(outputWidth) - 0.5f,
                        0.0f, static_cast<float>(source.width - 1));
                    const int x0 = static_cast<int>(sx), x1 = std::min(source.width - 1, x0 + 1);
                    const float fx = sx - static_cast<float>(x0);
                    const std::uint8_t* a = &source.pixels[(static_cast<std::size_t>(y0) * static_cast<std::size_t>(source.width) + static_cast<std::size_t>(x0)) * 4u];
                    const std::uint8_t* b = &source.pixels[(static_cast<std::size_t>(y0) * static_cast<std::size_t>(source.width) + static_cast<std::size_t>(x1)) * 4u];
                    const std::uint8_t* c = &source.pixels[(static_cast<std::size_t>(y1) * static_cast<std::size_t>(source.width) + static_cast<std::size_t>(x0)) * 4u];
                    const std::uint8_t* d = &source.pixels[(static_cast<std::size_t>(y1) * static_cast<std::size_t>(source.width) + static_cast<std::size_t>(x1)) * 4u];
                    std::uint8_t* out = &output_.pixels[(static_cast<std::size_t>(y) * static_cast<std::size_t>(outputWidth) + static_cast<std::size_t>(x)) * 4u];
                    for (int ch = 0; ch < 3; ++ch) {
                        const float top = a[ch] + (b[ch] - a[ch]) * fx;
                        const float bottom = c[ch] + (d[ch] - c[ch]) * fx;
                        out[ch] = static_cast<std::uint8_t>(std::lround(top + (bottom - top) * fy));
                    }
                    out[3] = 255;
                }
            }
        };
        if (jobs_) jobs_->ParallelFor(static_cast<std::size_t>(outputHeight), 8, rows);
        else rows(0, static_cast<std::size_t>(outputHeight));
    }
    DrawHud(output_, MakeView(outputWidth, outputHeight));

    const float renderMs = static_cast<float>(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count());
    smoothedRenderMs_ = smoothedRenderMs_ <= 0.0f ? renderMs : Lerp(smoothedRenderMs_, renderMs, 0.1f);
    if (settings_.dynamicResolution) {
        // Dynamic resolution: trade pixels for frame time around the budget.
        if (smoothedRenderMs_ > settings_.targetFrameMs * 1.08f) resolutionScale_ -= 0.04f;
        else if (smoothedRenderMs_ < settings_.targetFrameMs * 0.75f) resolutionScale_ += 0.02f;
        resolutionScale_ = Clamp(resolutionScale_, settings_.minResolutionScale, 1.0f);
    }
    stats_.renderMs = renderMs;
    stats_.resolutionScale = scale;
    stats_.internalWidth = width;
    stats_.internalHeight = height;
    stats_.render = renderer_.Stats();
    return output_;
}

void MallShowcase::DrawHud(Graphics::ImageRgba8& image, const Graphics::RenderView& view) {
    ASTRAL_PROFILE_SCOPE("Showcase.Hud");
    Canvas canvas(image);
    const float u = static_cast<float>(image.height) / 720.0f;
    const int font = std::max(1, static_cast<int>(std::lround(2.0f * u)));
    const int big = std::max(2, static_cast<int>(std::lround(4.0f * u)));
    auto px = [u](float value) { return static_cast<int>(std::lround(value * u)); };
    const Rgba8 panel{12, 10, 26, 255};
    const Rgba8 white{240, 240, 255, 255};
    const Rgba8 dim{160, 170, 200, 255};
    const bool focus = view_.thought && view_.thought->IsFocusActive();

    // Shadowblade panel.
    if (view_.shadowblade) {
        const Scene::ShadowbladeActions& s = *view_.shadowblade;
        canvas.FillRoundedRect(px(16), px(16), px(360), px(118), px(10), panel, 0.72f);
        canvas.Text(px(30), px(26), "SHADOWBLADE", {white, font, true});
        const float fraction = s.Resource() / Scene::ShadowbladeActions::MaximumResource;
        const Rgba8 barColor = focus ? Rgba8{150, 110, 255, 255} : Rgba8{70, 220, 235, 255};
        canvas.Bar(px(30), px(52), px(250), px(14), fraction, barColor, {30, 34, 60, 255});
        char resource[48];
        std::snprintf(resource, sizeof(resource), "%d/100", static_cast<int>(s.Resource()));
        canvas.Text(px(290), px(52), resource, {white, font, true});
        if (s.IsGuarding()) {
            canvas.FillRoundedRect(px(30), px(76), Canvas::MeasureText("GUARD ACTIVE", font) + px(20), px(22), px(8), {255, 210, 70, 255}, 0.9f);
            canvas.Text(px(40), px(80), "GUARD ACTIVE", {{40, 30, 10, 255}, font, false});
        } else {
            canvas.Text(px(30), px(80), "SHIFT GUARD", {dim, font, true});
        }
        // Cooldown rings for Q (dash) and L (fatal strike).
        const struct {
            const char* key;
            float remaining, total;
        } rings[2] = {{"Q", s.DashCooldownRemaining(), Scene::ShadowbladeActions::DashCooldownSeconds},
            {"L", s.FatalStrikeCooldownRemaining(), Scene::ShadowbladeActions::FatalStrikeCooldownSeconds}};
        for (int i = 0; i < 2; ++i) {
            const int cx = px(214 + 58 * static_cast<float>(i)), cy = px(94);
            const float r = 20.0f * u;
            canvas.FillCircle(cx, cy, r, {30, 34, 60, 255}, 0.9f);
            const float ready = rings[i].total > 0.0f ? 1.0f - Saturate(rings[i].remaining / rings[i].total) : 1.0f;
            canvas.Ring(cx, cy, r, 4.0f * u, ready, ready >= 1.0f ? Rgba8{150, 110, 255, 255} : Rgba8{90, 90, 130, 255});
            canvas.Text(cx - px(5), cy - px(7), rings[i].key, {white, font, true});
        }
    }
    // Thought Commands panel (bottom left); the typed prompt opens inside it.
    if (view_.thought) {
        const char* title = focus ? "THOUGHT FOCUS  LOCAL TIME x0.35" : "THOUGHT COMMANDS  1 DASH 2 FATAL 3/4 GUARD 5 FOCUS";
        const int x = px(16), y = image.height - px(196), h = px(74);
        const int w = std::max(px(560), Canvas::MeasureText(title, font) + px(28));
        canvas.FillRoundedRect(x, y, w, h, px(10), focus || promptOpen_ ? Rgba8{60, 30, 110, 255} : panel, promptOpen_ ? 0.9f : 0.72f);
        if (promptOpen_) canvas.StrokeRect(x, y, w, h, {170, 120, 255, 255}, std::max(1, px(2)));
        canvas.Text(x + px(14), y + px(10), title, {focus ? Rgba8{220, 200, 255, 255} : dim, font, true});
        if (promptOpen_) {
            const bool blink = std::fmod(time_.RealSeconds(), 1.0) < 0.5;
            canvas.Text(x + px(14), y + px(36), "> " + promptText_ + (blink ? "_" : ""), {{235, 225, 255, 255}, font + 1, true});
        } else {
            const Scene::ThoughtCommandReport& report = view_.thought->LastReport();
            std::string last = "ENTER  TYPE A THOUGHT";
            if (!report.submitted.empty()) {
                last = "LAST: ";
                for (char c : report.submitted) last.push_back(static_cast<char>(c >= 'a' && c <= 'z' ? c - 32 : c));
                last += report.status == Scene::ThoughtCommandStatus::Accepted ? "  ACCEPTED" : std::string("  REJECTED ") + ReasonText(report.reason);
            }
            canvas.Text(x + px(14), y + px(40), last, {white, font, true});
        }
    }
    // Landmark discovery prompt and objective count.
    if (view_.interaction) {
        const Scene::LandmarkInteraction& li = *view_.interaction;
        if (li.HasSelection()) {
            const bool visited = li.IsVisited(li.SelectedKind());
            const std::string text = std::string(visited ? "VISITED  " : "E  INTERACT  ") + LandmarkTitle(li.SelectedKind());
            const int w = Canvas::MeasureText(text, font + 1) + px(40);
            const int x = (image.width - w) / 2, y = image.height - px(110);
            canvas.FillRoundedRect(x, y, w, px(44), px(12), visited ? Rgba8{30, 90, 60, 255} : Rgba8{110, 30, 100, 255}, 0.82f);
            canvas.Text(x + px(20), y + px(12), text, {white, font + 1, true});
        }
    }
    // Discovery/encounter banner: top centre, between the left panels and the minimap.
    if (!bannerTitle_.empty() && bannerTime_ < 3.2f) {
        const float alpha = Saturate(std::min(bannerTime_ * 5.0f, (3.2f - bannerTime_) * 1.5f));
        const int available = image.width - 2 * px(420);
        int titleScale = big, subtitleScale = font;
        while (titleScale > 1 && Canvas::MeasureText(bannerTitle_, titleScale) > available) --titleScale;
        while (subtitleScale > 1 && Canvas::MeasureText(bannerSubtitle_, subtitleScale) > available) --subtitleScale;
        const int titleW = Canvas::MeasureText(bannerTitle_, titleScale);
        const int subtitleW = bannerSubtitle_.empty() ? 0 : Canvas::MeasureText(bannerSubtitle_, subtitleScale);
        const int w = std::max(titleW, subtitleW);
        const int padX = px(22), padY = px(12);
        const int h = titleScale * 7 + (subtitleW > 0 ? px(10) + subtitleScale * 7 : 0);
        const int x = (image.width - w) / 2, y = px(22);
        // Slide down as it appears.
        const int slide = static_cast<int>((1.0f - Saturate(bannerTime_ * 6.0f)) * static_cast<float>(px(16)));
        canvas.FillRoundedRect(x - padX, y - padY - slide, w + padX * 2, h + padY * 2, px(12), {24, 12, 48, 255}, 0.82f * alpha);
        canvas.FillRect(x - padX + px(12), y + h + padY - px(3) - slide, w + padX * 2 - px(24), std::max(1, px(2)),
            {190, 140, 255, 255}, alpha);
        const std::uint8_t a = static_cast<std::uint8_t>(255.0f * alpha);
        canvas.Text((image.width - titleW) / 2, y - slide, bannerTitle_, {{255, 240, 200, a}, titleScale, true});
        if (subtitleW > 0) {
            canvas.Text((image.width - subtitleW) / 2, y + titleScale * 7 + px(10) - slide, bannerSubtitle_,
                {{200, 185, 255, a}, subtitleScale, true});
        }
    }
    // Training target health above its head.
    if (view_.combat) {
        const Scene::TrainingDummy& dummy = view_.combat->Dummy();
        float sx, sy;
        if (Project(view, target_->Position() + Vec3{0, 2.25f, 0}, sx, sy)) {
            const int w = px(120), h = px(10);
            const int x = static_cast<int>(sx) - w / 2, y = static_cast<int>(sy);
            canvas.FillRoundedRect(x - px(3), y - px(3), w + px(6), h + px(6), px(4), {10, 8, 16, 255}, 0.75f);
            canvas.Bar(x, y, w, h, dummy.maximumHealth > 0 ? static_cast<float>(dummy.health) / static_cast<float>(dummy.maximumHealth) : 0.0f,
                dummy.IsDefeated() ? Rgba8{110, 110, 120, 255} : Rgba8{255, 105, 80, 255}, {50, 20, 26, 255});
            canvas.Text(x, y - px(22), "RIFT WRAITH", {{255, 200, 180, 255}, font, true});
        }
    }
    // Floating combat text, stacked upwards so simultaneous call-outs never overprint.
    struct Placed {
        int x0, y0, x1, y1;
    };
    std::vector<Placed> placed;
    for (const FloatingText& t : floating_) {
        float sx, sy;
        if (!Project(view, t.position, sx, sy)) continue;
        const int scale = std::max(1, static_cast<int>(std::lround(static_cast<float>(t.scale) * u)));
        const float pop = t.age < 0.1f ? 1.0f + (0.1f - t.age) * 4.0f : 1.0f;
        const int finalScale = std::max(1, static_cast<int>(std::lround(static_cast<float>(scale) * pop)));
        Rgba8 color = t.color;
        color.a = static_cast<std::uint8_t>(255.0f * Saturate((1.4f - t.age) / 0.5f));
        const int w = Canvas::MeasureText(t.text, finalScale), h = finalScale * 7;
        Placed box{static_cast<int>(sx) - w / 2, static_cast<int>(sy), static_cast<int>(sx) + w / 2, static_cast<int>(sy) + h};
        for (bool moved = true; moved;) {
            moved = false;
            for (const Placed& other : placed) {
                if (box.x0 < other.x1 && other.x0 < box.x1 && box.y0 < other.y1 && other.y0 < box.y1) {
                    const int shift = box.y1 - other.y0 + px(4);
                    box.y0 -= shift;
                    box.y1 -= shift;
                    moved = true;
                }
            }
        }
        placed.push_back(box);
        canvas.Text(box.x0, box.y0, t.text, {color, finalScale, true, {20, 8, 30, color.a}});
    }
    // Minimap of the playable Mall.
    if (view_.world) {
        const int mw = px(150), mh = px(260);
        const int mx = image.width - mw - px(18), my = px(18);
        canvas.FillRoundedRect(mx - px(6), my - px(6), mw + px(12), mh + px(12), px(10), panel, 0.72f);
        canvas.FillRect(mx, my, mw, mh, {60, 110, 60, 255}, 0.55f);
        auto mapX = [&](float x) { return mx + static_cast<int>((x + 20.0f) / 40.0f * static_cast<float>(mw)); };
        auto mapY = [&](float z) { return my + mh - static_cast<int>((z + 6.0f) / 80.0f * static_cast<float>(mh)); };
        for (std::size_t i = 0; i < view_.world->Landmarks().size(); ++i) {
            const Scene::LandmarkProxy& l = view_.world->Landmarks()[i];
            const bool visited = view_.interaction && view_.interaction->IsVisited(l.kind);
            const bool selected = view_.interaction && view_.interaction->HasSelection() && view_.interaction->SelectedIndex() == i;
            Rgba8 color = l.kind == Scene::LandmarkKind::ReflectingPool ? Rgba8{80, 150, 220, 255} : Rgba8{235, 235, 240, 255};
            if (visited) color = {80, 235, 125, 255};
            if (selected) color = {255, 90, 220, 255};
            const int x0 = mapX(l.position.x - l.dimensions.x * 0.5f), x1 = mapX(l.position.x + l.dimensions.x * 0.5f);
            const int y0 = mapY(l.position.z + l.dimensions.z * 0.5f), y1 = mapY(l.position.z - l.dimensions.z * 0.5f);
            canvas.FillRect(x0, y0, std::max(2, x1 - x0), std::max(2, y1 - y0), color, 0.9f);
        }
        if (view_.combat) {
            const Vec3 d = view_.world->GroundPosition(view_.combat->Dummy().position);
            canvas.FillCircle(mapX(d.x), mapY(d.z), 3.0f * u, {255, 105, 80, 255});
        }
        const Vec3 p = player_->Position();
        canvas.FillCircle(mapX(p.x), mapY(p.z), 4.5f * u, {170, 110, 255, 255});
        const Vec3 facing{std::sin(player_->Yaw()), 0, std::cos(player_->Yaw())};
        canvas.Line(mapX(p.x), mapY(p.z), mapX(p.x + facing.x * 4.0f), mapY(p.z + facing.z * 4.0f), {255, 255, 255, 255});
        if (view_.interaction) {
            char visited[32];
            std::snprintf(visited, sizeof(visited), "LANDMARKS %zu/3", view_.interaction->VisitedCount());
            canvas.Text(mx - px(4), my + mh + px(12), visited, {white, font, true});
        }
    }
    // Encounter status line (mirrors the GDI HUD states).
    if (view_.encounter) {
        const auto state = view_.encounter->State();
        const char* text = state == Scene::LandmarkEncounterState::Completed ? "TRAINING ENCOUNTER COMPLETED  REWARD GRANTED"
            : (state == Scene::LandmarkEncounterState::Active ? "TRAINING ENCOUNTER ACTIVE  DEFEAT DUMMY"
                                                             : "TRAINING ENCOUNTER LOCKED  DISCOVER LINCOLN");
        const Rgba8 color = state == Scene::LandmarkEncounterState::Completed ? Rgba8{80, 235, 125, 255}
            : (state == Scene::LandmarkEncounterState::Active ? Rgba8{255, 155, 60, 255} : Rgba8{120, 130, 160, 255});
        canvas.FillRoundedRect(px(16), px(142), Canvas::MeasureText(text, font) + px(30), px(28), px(8), panel, 0.7f);
        canvas.FillRect(px(16), px(142), px(6), px(28), color, 1.0f);
        canvas.Text(px(30), px(149), text, {color, font, true});
    }
    // Controls and optional engine statistics.
    canvas.Text(px(18), image.height - px(30), "F2 GDI  F3 STATS  F4 QUALITY  ENTER THOUGHT  WASD J K Q L E SHIFT", {dim, font, true});
    if (showStats_) {
        char lines[512];
        std::snprintf(lines, sizeof(lines),
            "ASTRAL RENDERER  PRESET %d  SCALE %.2f  %dx%d\nFRAME %.1f MS  SHADOW %.1f  RASTER %.1f  SHADE %.1f  POST %.1f\n"
            "DRAWS %zu/%zu  TRIS %zu  PARTICLES %zu  BODIES %zu",
            settings_.quality, static_cast<double>(stats_.resolutionScale), stats_.internalWidth, stats_.internalHeight,
            static_cast<double>(stats_.renderMs), stats_.render.shadowMs, stats_.render.rasterMs, stats_.render.shadeMs,
            stats_.render.postMs, stats_.render.drawsVisible, stats_.render.drawsSubmitted, stats_.render.trianglesRasterized,
            stats_.particles, stats_.physicsBodies);
        const int w = Canvas::MeasureText(lines, font) + px(24);
        const int x = image.width - w - px(16), y = image.height - px(140);
        canvas.FillRoundedRect(x, y, w, px(92), px(8), panel, 0.78f);
        canvas.Text(x + px(12), y + px(10), lines, {{200, 255, 200, 255}, font, true});
    }
}

} // namespace Astral::Showcase
