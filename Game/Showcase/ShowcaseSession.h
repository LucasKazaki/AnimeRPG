#pragma once

// Headless mirror of the Win32Application gameplay frame (same domains, same
// PlayerController tuning, same update order and edge-triggered keys) driving
// the Astral showcase. Used by Tools/AstralCapture and the showcase suite so
// captures and tests exercise the real, unchanged gameplay rules. The Win32
// loop keeps its own copy of this order; keep the two in step.

#include "Engine/Scene/PlayerController.h"
#include "Game/Showcase/MallShowcase.h"

#include <array>
#include <string>

namespace Astral::Showcase {

// Held state of the keys the Win32 loop polls, plus a typed thought submitted
// through the Enter prompt this frame.
struct SessionInput {
    bool forward{}, backward{}, left{}, right{}; // W S A D
    bool guard{};                                // Left Shift
    bool light{}, heavy{}, dash{}, fatal{}, interact{}; // J K Q L E
    std::array<bool, 6> command{};               // number keys 0..5
    std::string typedThought;                    // non-empty: submit once
};

class ShowcaseSession {
public:
    explicit ShowcaseSession(Core::JobSystem* jobs, ShowcaseSettings settings = {}, bool astralMovement = true);

    // One Win32-equivalent frame: gameplay update then showcase update.
    const FrameEvents& Step(const SessionInput& input, float deltaSeconds);

    MallShowcase& Showcase() { return showcase_; }
    const Scene::PlayerController& Player() const { return player_; }
    const Scene::CombatSandbox& Combat() const { return combat_; }
    const Scene::ShadowbladeActions& Shadowblade() const { return shadowblade_; }
    const Scene::ThoughtCommands& Thought() const { return thought_; }
    const Scene::LandmarkInteraction& Interaction() const { return interaction_; }
    const Scene::LandmarkEncounter& Encounter() const { return encounter_; }
    const Scene::WorldBlockout& World() const { return world_; }
    GameplayView View() const;
    // Diagnostic teleport (controller space), e.g. to stage a capture.
    void Place(Math::Vec3 controllerPosition);

private:
    void ApplyDash(const Scene::ShadowActionReport& report, FrameEvents& events);
    MallShowcase* SweepTarget() { return astralMovement_ ? &showcase_ : nullptr; }

    Scene::WorldBlockout world_;
    // Same tuning as Win32Application::playerController_.
    Scene::PlayerController player_{6.0f, {-18.0f, 18.0f, -4.0f, 72.0f}};
    Scene::CombatSandbox combat_;
    Scene::LandmarkEncounter encounter_;
    Scene::LandmarkInteraction interaction_;
    Scene::ShadowbladeActions shadowblade_;
    Scene::ThoughtCommands thought_;
    MallShowcase showcase_;
    bool astralMovement_;
    SessionInput previous_{};
    FrameEvents events_{};
};

} // namespace Astral::Showcase
