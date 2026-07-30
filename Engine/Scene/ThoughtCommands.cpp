#include "Engine/Scene/ThoughtCommands.h"

#include <cctype>
#include <cmath>
#include <vector>

namespace {

std::string Normalize(const std::string& input) {
    std::string normalized;
    bool pendingSpace = false;
    for (unsigned char character : input) {
        if (std::isspace(character)) {
            pendingSpace = !normalized.empty();
            continue;
        }
        if (pendingSpace) normalized.push_back(' ');
        normalized.push_back(static_cast<char>(std::tolower(character)));
        pendingSpace = false;
    }
    return normalized;
}

int RecognizedCommandCount(const std::string& normalized) {
    std::vector<std::string> words;
    std::string word;
    for (char character : normalized) {
        if (character == ' ') {
            words.push_back(word);
            word.clear();
        } else {
            word.push_back(character);
        }
    }
    if (!word.empty()) words.push_back(word);

    int count = 0;
    for (std::size_t index = 0; index < words.size(); ++index) {
        if (words[index] == "dash" || words[index] == "fatal" || words[index] == "focus") {
            ++count;
        } else if (words[index] == "guard" && index + 1 < words.size()
            && (words[index + 1] == "on" || words[index + 1] == "off")) {
            ++count;
            ++index;
        }
    }
    return count;
}

Astral::Scene::ThoughtCommandReason ReasonFor(
    Astral::Scene::ShadowActionResult result) {
    using Astral::Scene::ShadowActionResult;
    using Astral::Scene::ThoughtCommandReason;
    switch (result) {
    case ShadowActionResult::Activated:
    case ShadowActionResult::Guarding:
    case ShadowActionResult::Ready: return ThoughtCommandReason::None;
    case ShadowActionResult::GuardedConflict: return ThoughtCommandReason::GuardedConflict;
    case ShadowActionResult::Cooldown: return ThoughtCommandReason::Cooldown;
    case ShadowActionResult::InsufficientResource:
        return ThoughtCommandReason::InsufficientResource;
    case ShadowActionResult::OutOfRange: return ThoughtCommandReason::OutOfRange;
    case ShadowActionResult::TargetDefeated: return ThoughtCommandReason::TargetDefeated;
    }
    return ThoughtCommandReason::Unsupported;
}

} // namespace

namespace Astral::Scene {

ParsedThoughtCommand ThoughtCommands::Parse(const std::string& input) {
    ParsedThoughtCommand parsed;
    parsed.normalized = Normalize(input);
    if (parsed.normalized.empty()) {
        parsed.reason = ThoughtCommandReason::Empty;
    } else if (parsed.normalized == "dash") {
        parsed.type = ThoughtCommandType::Dash;
        parsed.reason = ThoughtCommandReason::None;
    } else if (parsed.normalized == "fatal") {
        parsed.type = ThoughtCommandType::Fatal;
        parsed.reason = ThoughtCommandReason::None;
    } else if (parsed.normalized == "guard on") {
        parsed.type = ThoughtCommandType::GuardOn;
        parsed.reason = ThoughtCommandReason::None;
    } else if (parsed.normalized == "guard off") {
        parsed.type = ThoughtCommandType::GuardOff;
        parsed.reason = ThoughtCommandReason::None;
    } else if (parsed.normalized == "focus") {
        parsed.type = ThoughtCommandType::Focus;
        parsed.reason = ThoughtCommandReason::None;
    } else {
        parsed.reason = RecognizedCommandCount(parsed.normalized) > 1
            ? ThoughtCommandReason::Ambiguous : ThoughtCommandReason::Unsupported;
    }
    return parsed;
}

ThoughtCommandReport ThoughtCommands::Submit(const std::string& input,
    const Math::Vec3& playerPosition, ShadowbladeActions& shadowbladeActions,
    CombatSandbox& combatSandbox) {
    const ParsedThoughtCommand parsed = Parse(input);
    lastReport_ = {};
    lastReport_.type = parsed.type;
    lastReport_.submitted = parsed.normalized;
    lastReport_.reason = parsed.reason;
    if (parsed.reason != ThoughtCommandReason::None) return lastReport_;

    switch (parsed.type) {
    case ThoughtCommandType::Dash:
        lastReport_.shadowAction = shadowbladeActions.TryDash(playerPosition);
        lastReport_.reason = ReasonFor(lastReport_.shadowAction.result);
        break;
    case ThoughtCommandType::Fatal:
        lastReport_.shadowAction = shadowbladeActions.TryFatalStrike(
            playerPosition, combatSandbox);
        lastReport_.reason = ReasonFor(lastReport_.shadowAction.result);
        break;
    case ThoughtCommandType::GuardOn:
        if (commandGuardActive_) {
            lastReport_.reason = ThoughtCommandReason::NoOp;
        } else {
            commandGuardActive_ = true;
            shadowbladeActions.SetGuarding(true);
            lastReport_.shadowAction = shadowbladeActions.LastAction();
        }
        break;
    case ThoughtCommandType::GuardOff:
        if (!commandGuardActive_) {
            lastReport_.reason = ThoughtCommandReason::NoOp;
        } else {
            commandGuardActive_ = false;
            shadowbladeActions.SetGuarding(false);
            lastReport_.shadowAction = shadowbladeActions.LastAction();
        }
        break;
    case ThoughtCommandType::Focus:
        focusActive_ = !focusActive_;
        break;
    case ThoughtCommandType::None:
        lastReport_.reason = ThoughtCommandReason::Unsupported;
        break;
    }

    lastReport_.status = lastReport_.reason == ThoughtCommandReason::None
        ? ThoughtCommandStatus::Accepted : ThoughtCommandStatus::Rejected;
    return lastReport_;
}

void ThoughtCommands::ApplyGuardState(bool physicalGuarding,
    ShadowbladeActions& shadowbladeActions) const {
    shadowbladeActions.SetGuarding(physicalGuarding || commandGuardActive_);
}

float ThoughtCommands::ScaleDelta(float deltaSeconds) const {
    if (deltaSeconds <= 0.0f || !std::isfinite(deltaSeconds)) return 0.0f;
    return deltaSeconds * TimeMultiplier();
}

} // namespace Astral::Scene
