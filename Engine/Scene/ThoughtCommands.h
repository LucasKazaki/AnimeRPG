#pragma once

#include "Engine/Math/Math.h"
#include "Engine/Scene/CombatSandbox.h"
#include "Engine/Scene/ShadowbladeActions.h"

#include <string>

namespace Astral::Scene {

enum class ThoughtCommandType {
    None,
    Dash,
    Fatal,
    GuardOn,
    GuardOff,
    Focus,
};

enum class ThoughtCommandStatus {
    Accepted,
    Rejected,
};

enum class ThoughtCommandReason {
    None,
    Empty,
    Ambiguous,
    Unsupported,
    NoOp,
    GuardedConflict,
    Cooldown,
    InsufficientResource,
    OutOfRange,
    TargetDefeated,
};

struct ParsedThoughtCommand {
    ThoughtCommandType type{ThoughtCommandType::None};
    ThoughtCommandReason reason{ThoughtCommandReason::Unsupported};
    std::string normalized;
};

struct ThoughtCommandReport {
    ThoughtCommandType type{ThoughtCommandType::None};
    ThoughtCommandStatus status{ThoughtCommandStatus::Rejected};
    ThoughtCommandReason reason{ThoughtCommandReason::Unsupported};
    std::string submitted;
    ShadowActionReport shadowAction{};
};

class ThoughtCommands {
public:
    static constexpr float NormalTimeMultiplier = 1.0f;
    static constexpr float FocusTimeMultiplier = 0.35f;

    static ParsedThoughtCommand Parse(const std::string& input);

    ThoughtCommandReport Submit(const std::string& input, const Math::Vec3& playerPosition,
        ShadowbladeActions& shadowbladeActions, CombatSandbox& combatSandbox);
    void ApplyGuardState(bool physicalGuarding, ShadowbladeActions& shadowbladeActions) const;
    float ScaleDelta(float deltaSeconds) const;

    bool IsFocusActive() const { return focusActive_; }
    bool IsCommandGuardActive() const { return commandGuardActive_; }
    float TimeMultiplier() const {
        return focusActive_ ? FocusTimeMultiplier : NormalTimeMultiplier;
    }
    const ThoughtCommandReport& LastReport() const { return lastReport_; }

private:
    bool focusActive_{};
    bool commandGuardActive_{};
    ThoughtCommandReport lastReport_{};
};

} // namespace Astral::Scene
