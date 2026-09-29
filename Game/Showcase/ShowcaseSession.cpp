#include "Game/Showcase/ShowcaseSession.h"

#include "Game/Showcase/GameplayBridge.h"

namespace Astral::Showcase {

ShowcaseSession::ShowcaseSession(Core::JobSystem* jobs, ShowcaseSettings settings, bool astralMovement)
    : showcase_(world_, jobs, settings), astralMovement_(astralMovement) {
    player_.SetPosition({0.0f, 0.0f, 0.0f});
    interaction_.UpdateSelection(player_.TransformState().WorldPosition(), world_);
    showcase_.Update(View(), {}, player_.TransformState().WorldPosition(), 0.0f);
}

GameplayView ShowcaseSession::View() const {
    return {&world_, &combat_, &shadowblade_, &thought_, &interaction_, &encounter_};
}

void ShowcaseSession::Place(Math::Vec3 controllerPosition) {
    player_.SetPosition(controllerPosition);
    interaction_.UpdateSelection(player_.TransformState().WorldPosition(), world_);
}

void ShowcaseSession::ApplyDash(const Scene::ShadowActionReport& report, FrameEvents& events) {
    const Math::Vec3 from = player_.TransformState().WorldPosition();
    player_.SetPosition(report.dashDestination); // bounds clamp, exactly as Win32
    ResolveDash(player_, from, report, SweepTarget(), events);
}

const FrameEvents& ShowcaseSession::Step(const SessionInput& input, float deltaSeconds) {
    FrameEvents& events = events_;
    events = {};

    const float simulationDelta = thought_.ScaleDelta(deltaSeconds);
    const Scene::MovementInput movement{input.forward, input.backward, input.left, input.right};
    const Math::Vec3 before = player_.TransformState().WorldPosition();
    player_.Update(movement, simulationDelta);
    if (astralMovement_) ResolveWalk(player_, before, showcase_);
    events.moveInput = {(input.right ? 1.0f : 0.0f) - (input.left ? 1.0f : 0.0f),
        (input.forward ? 1.0f : 0.0f) - (input.backward ? 1.0f : 0.0f)};
    combat_.AdvanceTime(simulationDelta);
    shadowblade_.AdvanceTime(simulationDelta);

    thought_.ApplyGuardState(input.guard, shadowblade_);
    const bool guarding = shadowblade_.IsGuarding();

    const Math::Vec3 position = player_.TransformState().WorldPosition();
    if (!guarding && input.light && !previous_.light) {
        events.lightAttack = true;
        events.attack = combat_.TryAttack(Scene::AttackType::Light, position);
    } else if (!guarding && input.heavy && !previous_.heavy) {
        events.heavyAttack = true;
        events.attack = combat_.TryAttack(Scene::AttackType::Heavy, position);
    }
    if (input.dash && !previous_.dash) {
        const Scene::ShadowActionReport report = shadowblade_.TryDash(player_.TransformState().WorldPosition());
        if (report.result == Scene::ShadowActionResult::Activated) {
            ApplyDash(report, events);
        } else {
            events.dash = true;
            events.dashReport = report;
        }
    } else if (input.fatal && !previous_.fatal) {
        events.fatalStrike = true;
        events.fatal = shadowblade_.TryFatalStrike(player_.TransformState().WorldPosition(), combat_);
    }

    static const char* const kCommandInputs[6]{"unsupported", "dash", "fatal", "guard on", "guard off", "focus"};
    std::string submitted;
    for (int index = 0; index < 6; ++index) {
        if (input.command[static_cast<std::size_t>(index)] && !previous_.command[static_cast<std::size_t>(index)]) {
            submitted = kCommandInputs[index];
            break;
        }
    }
    if (submitted.empty() && !input.typedThought.empty()) submitted = input.typedThought;
    if (!submitted.empty()) {
        const Scene::ThoughtCommandReport report =
            thought_.Submit(submitted, player_.TransformState().WorldPosition(), shadowblade_, combat_);
        if (report.type == Scene::ThoughtCommandType::Dash && report.status == Scene::ThoughtCommandStatus::Accepted) {
            ApplyDash(report.shadowAction, events);
        }
        thought_.ApplyGuardState(input.guard, shadowblade_);
        events.thoughtCommand = true;
        events.thought = report;
    }

    interaction_.UpdateSelection(player_.TransformState().WorldPosition(), world_);
    if (input.interact && !previous_.interact) {
        const Scene::LandmarkInteractionReport report =
            interaction_.TryInteract(player_.TransformState().WorldPosition(), world_, shadowblade_);
        if (report.result == Scene::LandmarkInteractionResult::Discovered) {
            events.encounterChanged =
                encounter_.TryActivate(report, combat_).result == Scene::LandmarkEncounterResult::Activated;
        }
        events.interacted = true;
        events.interaction = report;
    }
    events.encounterChanged = encounter_.Update(combat_, shadowblade_) || events.encounterChanged;
    previous_ = input;
    previous_.typedThought.clear();

    showcase_.Update(View(), events, player_.TransformState().WorldPosition(), deltaSeconds);
    return events;
}

} // namespace Astral::Showcase
