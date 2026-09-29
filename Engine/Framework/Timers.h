#pragma once

// Game-time timers (the role of Unreal's FTimerManager and Unity's
// Invoke/InvokeRepeating): one-shot and looping callbacks on scaled game time,
// pausable, owned by an entity so destroying it cancels its timers. Callbacks
// fire in deadline order and may freely set or clear timers (including their
// own). A looping timer whose interval is shorter than the frame fires once per
// elapsed interval, bounded per tick.

#include "Engine/World/Registry.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>

namespace Astral::Framework {

struct TimerHandle {
    std::uint64_t id{};
    bool IsNull() const { return id == 0; }
    friend bool operator==(TimerHandle a, TimerHandle b) { return a.id == b.id; }
    friend bool operator!=(TimerHandle a, TimerHandle b) { return a.id != b.id; }
};

class TimerManager {
public:
    // Fires `callback` after `delay` seconds; with interval > 0 it then repeats
    // every `interval` seconds until cleared. Returns a null handle for a
    // non-finite delay/interval or an empty callback.
    TimerHandle Set(float delay, std::function<void()> callback, float interval = 0.0f, World::Entity owner = {});
    bool Clear(TimerHandle handle);
    // Cancels every timer owned by `owner` (entity destruction).
    std::size_t ClearOwner(World::Entity owner);
    void ClearAll();
    bool IsActive(TimerHandle handle) const;
    // Seconds until the next firing (-1 when not active).
    float Remaining(TimerHandle handle) const;
    bool SetPaused(TimerHandle handle, bool paused);

    // Advances game time and fires due timers. Returns the number fired.
    std::size_t Tick(float dt);
    double Now() const { return now_; }
    std::size_t Count() const { return timers_.size(); }

    // Upper bound on callbacks per Tick (protects against runaway loops).
    std::size_t maxFiresPerTick{10000};

private:
    struct Timer {
        double due{};
        float interval{};
        std::function<void()> callback;
        World::Entity owner{};
        bool paused{};
        double pausedRemaining{};
    };
    std::map<std::uint64_t, Timer> timers_;
    std::uint64_t nextId_{1};
    double now_{};
};

} // namespace Astral::Framework
