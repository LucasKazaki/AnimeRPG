#pragma once

// Astral-mode presentation of the existing game: the gameplay domains in
// Engine/Scene stay authoritative; this layer turns their state and per-frame
// reports into a rendered, animated, audible anime scene with an engine HUD.
//
// Special features presented (rules unchanged):
//   Shadowblade  - dash (afterimages, streaks, swept against the world), light/
//                  heavy attacks and Fatal Strike (trails, sparks, hitstop,
//                  camera shake, crystal shattering), guard bubble, resource HUD.
//   Thought      - focus slow-time visualised with smooth time dilation, a
//   Commands       desaturated violet grade and aura; typed command prompt.
//   National Mall- landmark discovery prompts, encounter banner, minimap.

#include "Engine/Audio/AudioMixer.h"
#include "Engine/Core/GameTime.h"
#include "Engine/Core/JobSystem.h"
#include "Engine/Graphics/Canvas.h"
#include "Engine/Graphics/SceneRenderer.h"
#include "Engine/Physics/CharacterController.h"
#include "Engine/Scene/CombatSandbox.h"
#include "Engine/Scene/LandmarkEncounter.h"
#include "Engine/Scene/LandmarkInteraction.h"
#include "Engine/Scene/ShadowbladeActions.h"
#include "Engine/Scene/ThoughtCommands.h"
#include "Engine/Scene/WorldBlockout.h"
#include "Game/Showcase/CharacterPresenter.h"
#include "Game/Showcase/MallScene.h"

#include <memory>
#include <string>
#include <vector>

namespace Astral::Showcase {

struct ShowcaseSettings {
    int quality{2};               // RendererSettings::Preset level
    bool collision{true};         // resolve movement against the physical Mall
    bool dashPhasesThroughWalls{false};
    bool dynamicResolution{true};
    float targetFrameMs{16.7f};
    float minResolutionScale{0.5f};
    float fixedResolutionScale{1.0f}; // used when dynamic resolution is off
};

struct GameplayView {
    const Scene::WorldBlockout* world{};
    const Scene::CombatSandbox* combat{};
    const Scene::ShadowbladeActions* shadowblade{};
    const Scene::ThoughtCommands* thought{};
    const Scene::LandmarkInteraction* interaction{};
    const Scene::LandmarkEncounter* encounter{};
};

// What the gameplay loop did this frame (filled at each action site).
struct FrameEvents {
    Math::Vec2 moveInput{};               // controller space: x right, y forward
    bool lightAttack{}, heavyAttack{};
    Scene::AttackReport attack{};
    bool dash{};
    Scene::ShadowActionReport dashReport{};
    Math::Vec3 dashFrom{}, dashTo{};       // controller space
    bool fatalStrike{};
    Scene::ShadowActionReport fatal{};
    bool thoughtCommand{};
    Scene::ThoughtCommandReport thought{};
    bool interacted{};
    Scene::LandmarkInteractionReport interaction{};
    bool encounterChanged{};
};

struct ShowcaseStats {
    float frameMs{};
    float renderMs{};
    float resolutionScale{1.0f};
    int internalWidth{}, internalHeight{};
    std::size_t particles{};
    std::size_t physicsBodies{};
    Graphics::RenderStats render;
};

class MallShowcase {
public:
    MallShowcase(const Scene::WorldBlockout& blockout, Core::JobSystem* jobs, ShowcaseSettings settings = {});

    // Controller-space in/out. Sweeps the player capsule from `from` to `to`
    // against the Mall so walking and dashing respect walls.
    Math::Vec3 ResolveMovement(Math::Vec3 fromController, Math::Vec3 toController, bool dash);
    void Update(const GameplayView& view, const FrameEvents& events, Math::Vec3 playerController, float realDelta);
    // Renders the frame (3D at the dynamic internal resolution, HUD at output resolution).
    const Graphics::ImageRgba8& Render(int outputWidth, int outputHeight);

    void SetPrompt(bool open, const std::string& text) {
        promptOpen_ = open;
        promptText_ = text;
    }
    void ToggleStats() { showStats_ = !showStats_; }
    void CycleQuality();
    int Quality() const { return settings_.quality; }
    Audio::AudioMixer& Mixer() { return mixer_; }
    const ShowcaseStats& Stats() const { return stats_; }
    const CharacterPresenter& Player() const { return *player_; }
    const CharacterPresenter& Target() const { return *target_; }
    MallScene& Mall() { return *scene_; }
    Physics::PhysicsWorld& PhysicsScene() { return physics_; }
    Math::Vec3 CameraEye() const { return cameraEye_; }
    const Core::GameTime& Time() const { return time_; }

private:
    struct FloatingText {
        std::string text;
        Math::Vec3 position;
        float age{};
        Graphics::Rgba8 color;
        int scale{3};
    };
    struct PendingHit {
        int damage{};
        bool heavy{};
        bool fatal{};
        float timeout{};
    };
    Math::Vec3 ToWorld(Math::Vec3 controller) const { return {controller.x, 0.0f, controller.y}; }
    void HandleEvents(const FrameEvents& events);
    void ApplyHitFeedback(const PendingHit& hit);
    void UpdateCamera(float dt);
    void DrawHud(Graphics::ImageRgba8& image, const Graphics::RenderView& view);
    void ShowBanner(std::string title, std::string subtitle) {
        bannerTitle_ = std::move(title);
        bannerSubtitle_ = std::move(subtitle);
        bannerTime_ = 0.0f;
    }
    void Cue(const Audio::AudioClip& clip, float volume, float pitch, Math::Vec3 position, bool spatial = true);
    Graphics::RenderView MakeView(int width, int height) const;

    ShowcaseSettings settings_;
    Core::JobSystem* jobs_;
    Physics::PhysicsWorld physics_;
    std::unique_ptr<MallScene> scene_;
    std::unique_ptr<Physics::CharacterController> controller_;
    std::unique_ptr<CharacterPresenter> player_;
    std::unique_ptr<CharacterPresenter> target_;
    Graphics::SceneRenderer renderer_;
    Graphics::RenderTarget target3d_;
    Graphics::ImageRgba8 output_;
    Graphics::RenderScene frame_;
    Core::GameTime time_;
    Audio::AudioMixer mixer_;
    Audio::AudioClip swingClip_, heavyClip_, dashClip_, hitClip_, impactClip_, chimeClip_, droneClip_, stepClip_, crystalClip_;
    Audio::VoiceId droneVoice_{};
    GameplayView view_{};
    std::vector<FloatingText> floating_;
    std::vector<PendingHit> pendingHits_;
    VFX::EmitterId focusAura_{}, guardShimmer_{};
    bool focusAuraActive_{}, guardShimmerActive_{};
    Math::Vec3 cameraEye_{0, 4, -8}, cameraTarget_{0, 1, 0};
    float cameraFov_{50.0f};
    float shake_{};
    float fovKick_{};
    float focusBlend_{};
    float resolutionScale_{1.0f};
    float smoothedRenderMs_{};
    bool promptOpen_{};
    std::string promptText_;
    bool showStats_{};
    bool dummyWasDefeated_{};
    int lastDummyHealth_{-1};
    std::string bannerTitle_, bannerSubtitle_;
    float bannerTime_{10.0f};
    ShowcaseStats stats_;
};

} // namespace Astral::Showcase
