#pragma once

// Action-mapped input (the role of UE Enhanced Input): named actions bound to
// keys through prioritised, toggleable contexts; press/release edges, hold
// durations, taps, double taps and 2D axes; a per-action input buffer so an
// attack pressed slightly before a recovery ends still fires (fighting-game
// style); runtime rebinding with text save/load; and frame-exact recording and
// replay for deterministic gameplay tests.
//
// Key codes are Windows virtual-key values (ASCII for letters and digits), so
// the Win32 layer feeds them directly and other platforms map onto them.

#include "Engine/Math/VectorMath.h"

#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace Astral::Input {

namespace Keys {
constexpr std::uint16_t Backspace = 0x08, Tab = 0x09, Enter = 0x0D, Shift = 0x10, Control = 0x11, Escape = 0x1B,
    Space = 0x20, Left = 0x25, Up = 0x26, Right = 0x27, Down = 0x28, LeftShift = 0xA0, F1 = 0x70, F2 = 0x71,
    F3 = 0x72, F4 = 0x73, MouseLeft = 0x01, MouseRight = 0x02;
constexpr std::uint16_t Letter(char c) { return static_cast<std::uint16_t>(c >= 'a' && c <= 'z' ? c - 32 : c); }
constexpr std::uint16_t Digit(int d) { return static_cast<std::uint16_t>('0' + d); }
} // namespace Keys

struct InputSnapshot {
    std::bitset<256> keys;
    Math::Vec2 mouseDelta{};
    float dt{};
    bool Down(std::uint16_t key) const { return key < 256 && keys.test(key); }
};

enum class ActionType : std::uint8_t { Button, Axis2D };

struct Binding {
    std::uint16_t key{};
    Math::Vec2 axisContribution{}; // Axis2D: e.g. W = (0, 1), A = (-1, 0)
};

struct ActionDesc {
    std::string name;
    ActionType type{ActionType::Button};
    std::vector<Binding> bindings;
    float bufferSeconds{0.0f};  // keep a press available this long for ConsumeBuffered
    float doubleTapSeconds{0.3f};
    float tapMaxSeconds{0.2f};
};

struct InputContext {
    std::string name;
    int priority{0};
    bool enabled{true};
    bool consumesKeys{false}; // keys bound here are hidden from lower-priority contexts
    std::vector<ActionDesc> actions;
};

struct ActionState {
    bool down{};
    bool pressed{};   // went down this frame
    bool released{};  // went up this frame
    bool tapped{};    // released within tapMaxSeconds of pressing
    bool doubleTapped{};
    float heldSeconds{};
    float bufferRemaining{};
    float sinceLastPress{1.0e9f};
    Math::Vec2 axis{};
};

class InputSystem {
public:
    void AddContext(const InputContext& context);
    bool SetContextEnabled(const std::string& name, bool enabled);
    void Update(const InputSnapshot& snapshot);

    bool Down(const std::string& action) const;
    bool Pressed(const std::string& action) const;
    bool Released(const std::string& action) const;
    bool Tapped(const std::string& action) const;
    bool DoubleTapped(const std::string& action) const;
    float HeldSeconds(const std::string& action) const;
    Math::Vec2 Axis(const std::string& action) const;
    // True once per buffered press, even if the press happened a few frames ago.
    bool ConsumeBuffered(const std::string& action);

    // Replaces `oldKey` with `newKey` for the action in every context.
    bool Rebind(const std::string& action, std::uint16_t oldKey, std::uint16_t newKey);
    std::string SaveBindings() const;
    bool LoadBindings(const std::string& text, std::string& error);

private:
    const ActionState* Find(const std::string& action) const;
    ActionState* FindMutable(const std::string& action);

    std::vector<InputContext> contexts_;
    std::vector<std::pair<std::string, ActionState>> states_;
};

// Frame-exact capture and playback of snapshots.
class InputRecording {
public:
    void Record(const InputSnapshot& snapshot) { frames_.push_back(snapshot); }
    std::size_t FrameCount() const { return frames_.size(); }
    const InputSnapshot& Frame(std::size_t index) const { return frames_[index]; }
    std::string Serialize() const;
    bool Deserialize(const std::string& text, std::string& error);

private:
    std::vector<InputSnapshot> frames_;
};

} // namespace Astral::Input
