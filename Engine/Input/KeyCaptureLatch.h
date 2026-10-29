#pragma once

#include <array>
#include <cstddef>

namespace Astral::Input {

// Physical Win32 virtual-key ownership across a modal capture boundary. This
// owns no gameplay actions and does not reset persistent command/domain state.
class KeyCaptureLatch {
public:
    // Closing may happen between polls, so quarantine every binding, including
    // keys not sampled while captured. A physical up observation rearms each
    // key separately. Keep this object alive across renderer enable/disable.
    void QuarantineAllUntilRelease() { blocked_.fill(true); }

    bool Suppress(int virtualKey, bool physicallyDown, bool captured) {
        if (virtualKey < 0 || static_cast<std::size_t>(virtualKey) >= blocked_.size()) return true;
        bool& blocked = blocked_[static_cast<std::size_t>(virtualKey)];
        if (!physicallyDown) blocked = false;
        else if (captured) blocked = true;
        return captured || blocked;
    }

private:
    std::array<bool, 256> blocked_{};
};

} // namespace Astral::Input
