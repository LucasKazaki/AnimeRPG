#include "Engine/Framework/Timers.h"

#include <cmath>
#include <utility>

namespace Astral::Framework {

namespace {
// Looping intervals are clamped so a zero interval cannot spin forever.
constexpr float kMinInterval = 1.0e-4f;
} // namespace

TimerHandle TimerManager::Set(float delay, std::function<void()> callback, float interval, World::Entity owner) {
    if (!callback || !std::isfinite(delay) || !std::isfinite(interval)) return {};
    Timer timer;
    timer.due = now_ + static_cast<double>(delay < 0.0f ? 0.0f : delay);
    timer.interval = interval > 0.0f ? (interval < kMinInterval ? kMinInterval : interval) : 0.0f;
    timer.callback = std::move(callback);
    timer.owner = owner;
    const std::uint64_t id = nextId_++;
    timers_.emplace(id, std::move(timer));
    return {id};
}

bool TimerManager::Clear(TimerHandle handle) { return timers_.erase(handle.id) > 0; }

std::size_t TimerManager::ClearOwner(World::Entity owner) {
    if (owner.IsNull()) return 0;
    std::size_t removed = 0;
    for (auto it = timers_.begin(); it != timers_.end();) {
        if (it->second.owner == owner) {
            it = timers_.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }
    return removed;
}

void TimerManager::ClearAll() { timers_.clear(); }

bool TimerManager::IsActive(TimerHandle handle) const { return timers_.count(handle.id) > 0; }

float TimerManager::Remaining(TimerHandle handle) const {
    const auto it = timers_.find(handle.id);
    if (it == timers_.end()) return -1.0f;
    const double remaining = it->second.paused ? it->second.pausedRemaining : it->second.due - now_;
    return static_cast<float>(remaining < 0.0 ? 0.0 : remaining);
}

bool TimerManager::SetPaused(TimerHandle handle, bool paused) {
    const auto it = timers_.find(handle.id);
    if (it == timers_.end()) return false;
    Timer& timer = it->second;
    if (paused == timer.paused) return true;
    if (paused) {
        timer.pausedRemaining = timer.due - now_;
    } else {
        timer.due = now_ + timer.pausedRemaining;
    }
    timer.paused = paused;
    return true;
}

std::size_t TimerManager::Tick(float dt) {
    if (std::isfinite(dt) && dt > 0.0f) now_ += static_cast<double>(dt);
    std::size_t fired = 0;
    while (fired < maxFiresPerTick) {
        // Earliest due timer (ties by creation order) that has not been paused.
        auto next = timers_.end();
        for (auto it = timers_.begin(); it != timers_.end(); ++it) {
            if (it->second.paused || it->second.due > now_) continue;
            if (next == timers_.end() || it->second.due < next->second.due) next = it;
        }
        if (next == timers_.end()) break;
        std::function<void()> callback;
        if (next->second.interval > 0.0f) {
            next->second.due += static_cast<double>(next->second.interval);
            callback = next->second.callback; // the timer stays; the callback may clear it
        } else {
            callback = std::move(next->second.callback);
            timers_.erase(next);
        }
        ++fired;
        callback();
    }
    return fired;
}

} // namespace Astral::Framework
