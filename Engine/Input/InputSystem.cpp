#include "Engine/Input/InputSystem.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <utility>

namespace Astral::Input {

namespace {

struct NamedKey {
    const char* name;
    std::uint16_t key;
};

constexpr NamedKey kNamedKeys[] = {{"Space", Keys::Space}, {"Enter", Keys::Enter}, {"Escape", Keys::Escape},
    {"Tab", Keys::Tab}, {"Backspace", Keys::Backspace}, {"Shift", Keys::Shift}, {"LeftShift", Keys::LeftShift},
    {"Control", Keys::Control}, {"Left", Keys::Left}, {"Right", Keys::Right}, {"Up", Keys::Up}, {"Down", Keys::Down},
    {"MouseLeft", Keys::MouseLeft}, {"MouseRight", Keys::MouseRight}};

bool EqualsIgnoringCase(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        const char x = a[i] >= 'a' && a[i] <= 'z' ? static_cast<char>(a[i] - 32) : a[i];
        const char y = b[i] >= 'a' && b[i] <= 'z' ? static_cast<char>(b[i] - 32) : b[i];
        if (x != y) return false;
    }
    return true;
}

} // namespace

std::uint16_t KeyFromName(std::string_view name) {
    if (name.size() == 1) {
        const char c = name[0];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) return Keys::Letter(c);
        if (c >= '0' && c <= '9') return Keys::Digit(c - '0');
        return 0;
    }
    for (const NamedKey& named : kNamedKeys) {
        if (EqualsIgnoringCase(name, named.name)) return named.key;
    }
    if (name.size() >= 2 && name.size() <= 3 && (name[0] == 'F' || name[0] == 'f')) {
        int number = 0;
        for (std::size_t i = 1; i < name.size(); ++i) {
            if (name[i] < '0' || name[i] > '9') return 0;
            number = number * 10 + (name[i] - '0');
        }
        if (number >= 1 && number <= 12) return static_cast<std::uint16_t>(Keys::F1 + number - 1);
    }
    return 0;
}

std::string KeyName(std::uint16_t key) {
    if ((key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9')) return std::string(1, static_cast<char>(key));
    for (const NamedKey& named : kNamedKeys) {
        if (named.key == key) return named.name;
    }
    if (key >= Keys::F1 && key < Keys::F1 + 12) return "F" + std::to_string(key - Keys::F1 + 1);
    return {};
}

void InputSystem::AddContext(const InputContext& context) {
    contexts_.push_back(context);
    std::stable_sort(contexts_.begin(), contexts_.end(),
        [](const InputContext& a, const InputContext& b) { return a.priority > b.priority; });
    for (const ActionDesc& action : context.actions) {
        if (!Find(action.name)) states_.push_back({action.name, ActionState{}});
    }
}

bool InputSystem::SetContextEnabled(const std::string& name, bool enabled) {
    for (InputContext& context : contexts_) {
        if (context.name == name) {
            context.enabled = enabled;
            return true;
        }
    }
    return false;
}

const ActionState* InputSystem::Find(const std::string& action) const {
    for (const auto& entry : states_)
        if (entry.first == action) return &entry.second;
    return nullptr;
}

ActionState* InputSystem::FindMutable(const std::string& action) {
    for (auto& entry : states_)
        if (entry.first == action) return &entry.second;
    return nullptr;
}

void InputSystem::Update(const InputSnapshot& snapshot) {
    const float dt = std::isfinite(snapshot.dt) && snapshot.dt > 0.0f ? snapshot.dt : 0.0f;
    std::bitset<256> available = snapshot.keys;
    // Resolve each action once, from the highest-priority enabled context that defines it.
    std::vector<std::string> resolved;
    for (auto& entry : states_) {
        ActionState& state = entry.second;
        state.pressed = state.released = state.tapped = state.doubleTapped = false;
    }
    for (const InputContext& context : contexts_) {
        if (!context.enabled) continue;
        std::bitset<256> consumed;
        for (const ActionDesc& action : context.actions) {
            if (std::find(resolved.begin(), resolved.end(), action.name) != resolved.end()) continue;
            resolved.push_back(action.name);
            ActionState* state = FindMutable(action.name);
            if (!state) continue;
            bool down = false;
            Math::Vec2 axis{};
            for (const Binding& binding : action.bindings) {
                if (binding.key >= 256 || !available.test(binding.key)) continue;
                down = true;
                axis += binding.axisContribution;
                consumed.set(binding.key);
            }
            if (action.type == ActionType::Axis2D) {
                const float length = Math::Length(axis);
                state->axis = length > 1.0f ? axis / length : axis;
                down = length > 0.0f;
            }
            const bool wasDown = state->down;
            state->down = down;
            state->sinceLastPress += dt;
            state->bufferRemaining = std::max(0.0f, state->bufferRemaining - dt);
            if (down && !wasDown) {
                state->pressed = true;
                state->doubleTapped = state->sinceLastPress <= action.doubleTapSeconds;
                state->sinceLastPress = 0.0f;
                state->heldSeconds = 0.0f;
                state->bufferRemaining = std::max(action.bufferSeconds, 1.0e-4f);
            } else if (down) {
                state->heldSeconds += dt;
            }
            if (!down && wasDown) {
                state->released = true;
                state->tapped = state->heldSeconds <= action.tapMaxSeconds;
            }
        }
        if (context.consumesKeys) available &= ~consumed;
    }
    // Actions not defined by any enabled context read as released.
    for (auto& entry : states_) {
        if (std::find(resolved.begin(), resolved.end(), entry.first) != resolved.end()) continue;
        ActionState& state = entry.second;
        state.released = state.down;
        state.down = false;
        state.axis = {};
        state.bufferRemaining = 0.0f;
    }
}

bool InputSystem::Down(const std::string& a) const { const ActionState* s = Find(a); return s && s->down; }
bool InputSystem::Pressed(const std::string& a) const { const ActionState* s = Find(a); return s && s->pressed; }
bool InputSystem::Released(const std::string& a) const { const ActionState* s = Find(a); return s && s->released; }
bool InputSystem::Tapped(const std::string& a) const { const ActionState* s = Find(a); return s && s->tapped; }
bool InputSystem::DoubleTapped(const std::string& a) const { const ActionState* s = Find(a); return s && s->doubleTapped; }
float InputSystem::HeldSeconds(const std::string& a) const { const ActionState* s = Find(a); return s ? s->heldSeconds : 0.0f; }
Math::Vec2 InputSystem::Axis(const std::string& a) const { const ActionState* s = Find(a); return s ? s->axis : Math::Vec2{}; }

bool InputSystem::ConsumeBuffered(const std::string& action) {
    ActionState* state = FindMutable(action);
    if (!state || state->bufferRemaining <= 0.0f) return false;
    state->bufferRemaining = 0.0f;
    return true;
}

bool InputSystem::Rebind(const std::string& action, std::uint16_t oldKey, std::uint16_t newKey) {
    if (newKey >= 256) return false;
    bool changed = false;
    for (InputContext& context : contexts_) {
        for (ActionDesc& desc : context.actions) {
            if (desc.name != action) continue;
            for (Binding& binding : desc.bindings) {
                if (binding.key == oldKey) {
                    binding.key = newKey;
                    changed = true;
                }
            }
        }
    }
    return changed;
}

std::string InputSystem::SaveBindings() const {
    std::ostringstream out;
    out << "ASTRAL_BINDINGS 1\n";
    for (const InputContext& context : contexts_) {
        for (const ActionDesc& action : context.actions) {
            for (std::size_t i = 0; i < action.bindings.size(); ++i) {
                out << context.name << ' ' << action.name << ' ' << i << ' ' << action.bindings[i].key << '\n';
            }
        }
    }
    return out.str();
}

bool InputSystem::LoadBindings(const std::string& text, std::string& error) {
    std::istringstream in(text);
    std::string header;
    int version = 0;
    if (!(in >> header >> version) || header != "ASTRAL_BINDINGS" || version != 1) {
        error = "missing ASTRAL_BINDINGS 1 header";
        return false;
    }
    std::vector<InputContext> updated = contexts_;
    std::string contextName, actionName;
    long index = 0, key = 0;
    while (in >> contextName >> actionName >> index >> key) {
        if (key < 0 || key >= 256 || index < 0) {
            error = "binding value out of range";
            return false;
        }
        bool applied = false;
        for (InputContext& context : updated) {
            if (context.name != contextName) continue;
            for (ActionDesc& action : context.actions) {
                if (action.name == actionName && static_cast<std::size_t>(index) < action.bindings.size()) {
                    action.bindings[static_cast<std::size_t>(index)].key = static_cast<std::uint16_t>(key);
                    applied = true;
                }
            }
        }
        if (!applied) {
            error = "unknown binding " + contextName + "/" + actionName;
            return false;
        }
    }
    if (!in.eof()) {
        error = "malformed binding line";
        return false;
    }
    contexts_ = updated;
    return true;
}

std::string InputRecording::Serialize() const {
    std::ostringstream out;
    out << "ASTRAL_INPUT 2 " << frames_.size() << '\n';
    for (const InputSnapshot& frame : frames_) {
        char fields[96];
        std::snprintf(fields, sizeof(fields), "%.9g %.9g %.9g", static_cast<double>(frame.dt),
            static_cast<double>(frame.mouseDelta.x), static_cast<double>(frame.mouseDelta.y));
        out << fields;
        for (int key = 0; key < 256; ++key)
            if (frame.keys.test(static_cast<std::size_t>(key))) out << ' ' << key;
        out << '\n';
    }
    return out.str();
}

bool InputRecording::Deserialize(const std::string& text, std::string& error) {
    std::istringstream in(text);
    std::string header;
    int version = 0;
    std::size_t count = 0;
    if (!(in >> header >> version >> count) || header != "ASTRAL_INPUT" || (version != 1 && version != 2)
        || count > 10000000) {
        error = "missing ASTRAL_INPUT 1/2 header";
        return false;
    }
    std::string line;
    std::getline(in, line);
    std::vector<InputSnapshot> frames;
    frames.reserve(count);
    while (frames.size() < count && std::getline(in, line)) {
        std::istringstream row(line);
        InputSnapshot snapshot;
        std::string dt;
        if (!(row >> dt)) {
            error = "empty frame";
            return false;
        }
        char* end = nullptr;
        snapshot.dt = std::strtof(dt.c_str(), &end);
        if (!end || *end != '\0' || !std::isfinite(snapshot.dt) || snapshot.dt < 0.0f) {
            error = "bad frame delta";
            return false;
        }
        if (version >= 2) { // v2 adds the mouse delta after the frame delta
            std::string mx, my;
            if (!(row >> mx >> my)) {
                error = "missing mouse delta";
                return false;
            }
            char* endX = nullptr;
            char* endY = nullptr;
            snapshot.mouseDelta = {std::strtof(mx.c_str(), &endX), std::strtof(my.c_str(), &endY)};
            if (!endX || *endX != '\0' || !endY || *endY != '\0' || !std::isfinite(snapshot.mouseDelta.x)
                || !std::isfinite(snapshot.mouseDelta.y)) {
                error = "bad mouse delta";
                return false;
            }
        }
        int key = 0;
        while (row >> key) {
            if (key < 0 || key >= 256) {
                error = "key out of range";
                return false;
            }
            snapshot.keys.set(static_cast<std::size_t>(key));
        }
        if (!row.eof()) {
            error = "malformed frame";
            return false;
        }
        frames.push_back(snapshot);
    }
    if (frames.size() != count) {
        error = "truncated recording";
        return false;
    }
    frames_ = std::move(frames);
    return true;
}

} // namespace Astral::Input
