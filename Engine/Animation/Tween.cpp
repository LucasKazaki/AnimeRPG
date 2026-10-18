#include "Engine/Animation/Tween.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Astral::Animation {

namespace {

constexpr float kPiF = 3.14159265358979f;

const char* const kEaseNames[] = {"Linear", "InQuad", "OutQuad", "InOutQuad", "InCubic", "OutCubic", "InOutCubic",
    "InQuart", "OutQuart", "InOutQuart", "InQuint", "OutQuint", "InOutQuint", "InSine", "OutSine", "InOutSine",
    "InExpo", "OutExpo", "InOutExpo", "InCirc", "OutCirc", "InOutCirc", "InBack", "OutBack", "InOutBack",
    "InElastic", "OutElastic", "InOutElastic", "InBounce", "OutBounce", "InOutBounce"};
static_assert(sizeof(kEaseNames) / sizeof(kEaseNames[0]) == static_cast<std::size_t>(Ease::Count), "ease names");

float OutBounce(float t) {
    constexpr float n1 = 7.5625f, d1 = 2.75f;
    if (t < 1.0f / d1) return n1 * t * t;
    if (t < 2.0f / d1) {
        t -= 1.5f / d1;
        return n1 * t * t + 0.75f;
    }
    if (t < 2.5f / d1) {
        t -= 2.25f / d1;
        return n1 * t * t + 0.9375f;
    }
    t -= 2.625f / d1;
    return n1 * t * t + 0.984375f;
}

float InOutPower(float t, float power) {
    return t < 0.5f ? std::pow(2.0f, power - 1.0f) * std::pow(t, power) : 1.0f - std::pow(-2.0f * t + 2.0f, power) * 0.5f;
}

} // namespace

float EaseValue(Ease ease, float t) {
    if (!(t > 0.0f)) return 0.0f; // also maps NaN to the start
    if (t >= 1.0f) return 1.0f;
    constexpr float c1 = 1.70158f, c2 = c1 * 1.525f, c3 = c1 + 1.0f;
    constexpr float c4 = 2.0f * kPiF / 3.0f, c5 = 2.0f * kPiF / 4.5f;
    switch (ease) {
    case Ease::Linear: return t;
    case Ease::InQuad: return t * t;
    case Ease::OutQuad: return 1.0f - (1.0f - t) * (1.0f - t);
    case Ease::InOutQuad: return InOutPower(t, 2.0f);
    case Ease::InCubic: return t * t * t;
    case Ease::OutCubic: return 1.0f - std::pow(1.0f - t, 3.0f);
    case Ease::InOutCubic: return InOutPower(t, 3.0f);
    case Ease::InQuart: return std::pow(t, 4.0f);
    case Ease::OutQuart: return 1.0f - std::pow(1.0f - t, 4.0f);
    case Ease::InOutQuart: return InOutPower(t, 4.0f);
    case Ease::InQuint: return std::pow(t, 5.0f);
    case Ease::OutQuint: return 1.0f - std::pow(1.0f - t, 5.0f);
    case Ease::InOutQuint: return InOutPower(t, 5.0f);
    case Ease::InSine: return 1.0f - std::cos(t * kPiF * 0.5f);
    case Ease::OutSine: return std::sin(t * kPiF * 0.5f);
    case Ease::InOutSine: return -(std::cos(kPiF * t) - 1.0f) * 0.5f;
    case Ease::InExpo: return std::pow(2.0f, 10.0f * t - 10.0f);
    case Ease::OutExpo: return 1.0f - std::pow(2.0f, -10.0f * t);
    case Ease::InOutExpo:
        return t < 0.5f ? std::pow(2.0f, 20.0f * t - 10.0f) * 0.5f : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) * 0.5f;
    case Ease::InCirc: return 1.0f - std::sqrt(1.0f - t * t);
    case Ease::OutCirc: return std::sqrt(1.0f - (t - 1.0f) * (t - 1.0f));
    case Ease::InOutCirc:
        return t < 0.5f ? (1.0f - std::sqrt(1.0f - 4.0f * t * t)) * 0.5f
                        : (std::sqrt(1.0f - std::pow(-2.0f * t + 2.0f, 2.0f)) + 1.0f) * 0.5f;
    case Ease::InBack: return c3 * t * t * t - c1 * t * t;
    case Ease::OutBack: return 1.0f + c3 * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f);
    case Ease::InOutBack:
        return t < 0.5f ? (std::pow(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) * 0.5f
                        : (std::pow(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) * 0.5f;
    case Ease::InElastic: return -std::pow(2.0f, 10.0f * t - 10.0f) * std::sin((t * 10.0f - 10.75f) * c4);
    case Ease::OutElastic: return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
    case Ease::InOutElastic:
        return t < 0.5f ? -(std::pow(2.0f, 20.0f * t - 10.0f) * std::sin((20.0f * t - 11.125f) * c5)) * 0.5f
                        : (std::pow(2.0f, -20.0f * t + 10.0f) * std::sin((20.0f * t - 11.125f) * c5)) * 0.5f + 1.0f;
    case Ease::InBounce: return 1.0f - OutBounce(1.0f - t);
    case Ease::OutBounce: return OutBounce(t);
    case Ease::InOutBounce: return t < 0.5f ? (1.0f - OutBounce(1.0f - 2.0f * t)) * 0.5f : (1.0f + OutBounce(2.0f * t - 1.0f)) * 0.5f;
    case Ease::Count: break;
    }
    return t;
}

const char* EaseName(Ease ease) {
    const auto index = static_cast<std::size_t>(ease);
    return index < static_cast<std::size_t>(Ease::Count) ? kEaseNames[index] : "Linear";
}

bool ParseEase(std::string_view name, Ease& out) {
    for (std::size_t i = 0; i < static_cast<std::size_t>(Ease::Count); ++i) {
        if (name == kEaseNames[i]) {
            out = static_cast<Ease>(i);
            return true;
        }
    }
    return false;
}

// ------------------------------------------------------------------ creation

TweenHandle TweenManager::Add(std::shared_ptr<Tween> tween) {
    tween->id = nextId_++;
    const TweenHandle handle{tween->id};
    tweens_.emplace(tween->id, std::move(tween));
    return handle;
}

TweenHandle TweenManager::Custom(float duration, std::function<void(float)> apply, Ease ease) {
    auto tween = std::make_shared<Tween>();
    tween->duration = std::isfinite(duration) ? std::max(0.0f, duration) : 0.0f;
    tween->apply = std::move(apply);
    tween->ease = ease;
    return Add(std::move(tween));
}

TweenHandle TweenManager::Float(float from, float to, float duration, std::function<void(float)> setter, Ease ease) {
    if (!setter) return {};
    return Custom(duration, [from, to, setter = std::move(setter)](float e) { setter(from + (to - from) * e); }, ease);
}

TweenHandle TweenManager::Vector(Math::Vec3 from, Math::Vec3 to, float duration, std::function<void(Math::Vec3)> setter,
    Ease ease) {
    if (!setter) return {};
    return Custom(duration, [from, to, setter = std::move(setter)](float e) { setter(from + (to - from) * e); }, ease);
}

TweenHandle TweenManager::Rotation(Math::Quat from, Math::Quat to, float duration, std::function<void(Math::Quat)> setter,
    Ease ease) {
    if (!setter) return {};
    return Custom(duration, [from, to, setter = std::move(setter)](float e) { setter(Math::Slerp(from, to, e)); }, ease);
}

TweenHandle TweenManager::Sequence() {
    auto tween = std::make_shared<Tween>();
    tween->isSequence = true;
    tween->ease = Ease::Linear;
    return Add(std::move(tween));
}

std::shared_ptr<TweenManager::Tween> TweenManager::Find(TweenHandle handle) const {
    const auto found = tweens_.find(handle.id);
    return found == tweens_.end() ? nullptr : found->second;
}

bool TweenManager::SetDelay(TweenHandle handle, float seconds) {
    auto tween = Find(handle);
    if (!tween || !std::isfinite(seconds)) return false;
    tween->delay = std::max(0.0f, seconds);
    return true;
}

bool TweenManager::SetLoops(TweenHandle handle, int loops, LoopMode mode) {
    auto tween = Find(handle);
    if (!tween || loops == 0 || (loops < 0 && tween->parented)) return false;
    tween->loops = loops;
    tween->loopMode = mode;
    return true;
}

bool TweenManager::SetTimeScale(TweenHandle handle, float scale) {
    auto tween = Find(handle);
    if (!tween || !std::isfinite(scale) || scale < 0.0f) return false;
    tween->timeScale = scale;
    return true;
}

bool TweenManager::SetEase(TweenHandle handle, Ease ease) {
    auto tween = Find(handle);
    if (!tween) return false;
    tween->ease = ease;
    return true;
}

bool TweenManager::OnComplete(TweenHandle handle, std::function<void()> callback) {
    auto tween = Find(handle);
    if (!tween) return false;
    tween->onComplete = std::move(callback);
    return true;
}

bool TweenManager::SetOwner(TweenHandle handle, std::uint64_t owner) {
    auto tween = Find(handle);
    if (!tween) return false;
    tween->owner = owner;
    return true;
}

// ------------------------------------------------------------------ sequences

float TweenManager::CycleDuration(const Tween& tween) {
    if (!tween.isSequence) return tween.duration;
    float end = 0.0f;
    for (const Child& child : tween.children) end = std::max(end, child.start + Total(*child.tween));
    for (const Callback& callback : tween.callbacks) end = std::max(end, callback.time);
    return end;
}

float TweenManager::Total(const Tween& tween) {
    if (tween.loops < 0) return -1.0f;
    return tween.delay + CycleDuration(tween) * static_cast<float>(tween.loops);
}

bool TweenManager::Adopt(const std::shared_ptr<Tween>& sequence, TweenHandle handle, float start) {
    auto child = Find(handle);
    if (!sequence || !sequence->isSequence || !child || child == sequence || child->parented || child->loops < 0
        || child->touched || !std::isfinite(start)) {
        return false;
    }
    child->parented = true;
    sequence->children.push_back({child, std::max(0.0f, start)});
    return true;
}

bool TweenManager::Append(TweenHandle sequence, TweenHandle child) {
    auto owner = Find(sequence);
    if (!owner) return false;
    const float start = CycleDuration(*owner);
    if (!Adopt(owner, child, start)) return false;
    owner->lastAppendStart = start;
    return true;
}

bool TweenManager::Join(TweenHandle sequence, TweenHandle child) {
    auto owner = Find(sequence);
    return owner && Adopt(owner, child, owner->lastAppendStart);
}

bool TweenManager::Insert(TweenHandle sequence, float time, TweenHandle child) { return Adopt(Find(sequence), child, time); }

bool TweenManager::AppendInterval(TweenHandle sequence, float seconds) {
    auto owner = Find(sequence);
    if (!owner || !owner->isSequence || !std::isfinite(seconds) || seconds < 0.0f) return false;
    const TweenHandle gap = Custom(seconds, nullptr, Ease::Linear);
    return Append(sequence, gap);
}

bool TweenManager::AppendCallback(TweenHandle sequence, std::function<void()> callback) {
    auto owner = Find(sequence);
    if (!owner || !owner->isSequence || !callback) return false;
    owner->callbacks.push_back({CycleDuration(*owner), std::move(callback)});
    return true;
}

bool TweenManager::InsertCallback(TweenHandle sequence, float time, std::function<void()> callback) {
    auto owner = Find(sequence);
    if (!owner || !owner->isSequence || !callback || !std::isfinite(time)) return false;
    owner->callbacks.push_back({std::max(0.0f, time), std::move(callback)});
    return true;
}

// ------------------------------------------------------------------ sampling

void TweenManager::Sample(Tween& tween, float elapsed) {
    const float t = elapsed - tween.delay;
    const float cycle = CycleDuration(tween);
    if (t < 0.0f) {
        if (tween.touched) SampleCycle(tween, 0, 0.0f); // rewound to before the start
        return;
    }
    if (cycle <= 0.0f) {
        SampleCycle(tween, 0, 0.0f);
        return;
    }
    int index = static_cast<int>(std::floor(t / cycle));
    float local = t - static_cast<float>(index) * cycle;
    if (tween.loops >= 0 && index >= tween.loops) {
        index = tween.loops - 1;
        local = cycle;
    }
    SampleCycle(tween, index, std::min(local, cycle));
}

void TweenManager::SampleCycle(Tween& tween, int cycle, float local) {
    tween.touched = true;
    const float length = CycleDuration(tween);
    const float progress = length > 0.0f ? local / length : 1.0f;
    const bool reverse = tween.loopMode == LoopMode::Yoyo && (cycle % 2) == 1;
    const float eased = EaseValue(tween.ease, reverse ? 1.0f - progress : progress);
    if (!tween.isSequence) {
        if (tween.apply) tween.apply(eased);
        return;
    }
    const float time = eased * length;
    // Callbacks fire as the sequence time sweeps across them, including the
    // rest of any cycles completed since the last sample.
    auto fireBetween = [&](float from, float to) {
        for (const Callback& callback : tween.callbacks) {
            const bool crossed = from < to ? (callback.time > from && callback.time <= to)
                                           : (callback.time < from && callback.time >= to);
            if (crossed && callback.callback) callback.callback();
        }
    };
    auto cycleStart = [&](int index) {
        const bool backwards = tween.loopMode == LoopMode::Yoyo && (index % 2) == 1;
        return backwards ? length + 1.0f : -1.0f;
    };
    auto cycleEnd = [&](int index) {
        const bool backwards = tween.loopMode == LoopMode::Yoyo && (index % 2) == 1;
        return backwards ? 0.0f : length;
    };
    // Never sampled: playback began at the start of the first cycle.
    const int previousCycle = tween.sampledCycle < 0 ? 0 : tween.sampledCycle;
    const float previousTime = tween.sampledCycle < 0 ? cycleStart(0) : tween.sampledLocal;
    if (cycle == previousCycle) {
        fireBetween(previousTime, time);
    } else if (cycle > previousCycle) {
        fireBetween(previousTime, cycleEnd(previousCycle));
        for (int skipped = previousCycle + 1; skipped < cycle && skipped - previousCycle < 1000; ++skipped) {
            fireBetween(cycleStart(skipped), cycleEnd(skipped));
        }
        fireBetween(cycleStart(cycle), time);
    }
    // (Rewinds to an earlier cycle, e.g. a parent sequence restarting, fire nothing.)
    tween.sampledCycle = cycle;
    tween.sampledLocal = time;
    for (Child& child : tween.children) {
        const float childTime = time - child.start;
        Tween& inner = *child.tween;
        if (childTime < 0.0f) {
            if (inner.touched) Sample(inner, 0.0f);
            inner.completed = false;
            continue;
        }
        const float total = Total(inner);
        Sample(inner, std::min(childTime, total));
        if (childTime >= total && !inner.completed) {
            inner.completed = true;
            if (inner.onComplete) inner.onComplete();
        } else if (childTime < total) {
            inner.completed = false;
        }
    }
}

// ------------------------------------------------------------------ control

void TweenManager::Remove(std::uint64_t id) {
    const auto found = tweens_.find(id);
    if (found == tweens_.end()) return;
    const std::shared_ptr<Tween> tween = found->second;
    tweens_.erase(found);
    for (const Child& child : tween->children) Remove(child.tween->id);
}

void TweenManager::Finish(const std::shared_ptr<Tween>& tween) {
    // Removed first, so the callback may start new tweens and IsActive is false.
    Remove(tween->id);
    if (tween->onComplete) tween->onComplete();
}

bool TweenManager::Pause(TweenHandle handle, bool paused) {
    auto tween = Find(handle);
    if (!tween || tween->parented) return false;
    tween->paused = paused;
    return true;
}

bool TweenManager::Kill(TweenHandle handle, bool complete) {
    auto tween = Find(handle);
    if (!tween || tween->parented) return false;
    if (!complete) {
        Remove(tween->id);
        return true;
    }
    const float total = Total(*tween);
    if (total >= 0.0f) {
        Sample(*tween, total);
    } else {
        // Infinite loops end on their current cycle.
        const float cycle = CycleDuration(*tween);
        const float t = std::max(0.0f, tween->elapsed - tween->delay);
        const int index = cycle > 0.0f ? static_cast<int>(std::floor(t / cycle)) : 0;
        SampleCycle(*tween, index, cycle);
    }
    Finish(tween);
    return true;
}

std::size_t TweenManager::KillOwner(std::uint64_t owner, bool complete) {
    if (owner == 0) return 0;
    std::vector<TweenHandle> matches;
    for (const auto& entry : tweens_) {
        if (entry.second->owner == owner && !entry.second->parented) matches.push_back({entry.first});
    }
    std::size_t killed = 0;
    for (TweenHandle handle : matches) killed += Kill(handle, complete) ? 1u : 0u;
    return killed;
}

void TweenManager::KillAll() { tweens_.clear(); }

std::size_t TweenManager::Update(float dt) {
    if (!std::isfinite(dt) || dt < 0.0f) dt = 0.0f;
    std::vector<std::shared_ptr<Tween>> active;
    for (const auto& entry : tweens_) {
        if (!entry.second->parented && !entry.second->paused) active.push_back(entry.second);
    }
    for (const auto& tween : active) {
        if (!tweens_.count(tween->id) || tween->paused) continue; // killed or paused by a callback
        tween->elapsed += dt * tween->timeScale;
        const float total = Total(*tween);
        const bool done = total >= 0.0f && tween->elapsed >= total;
        Sample(*tween, done ? total : tween->elapsed);
        if (done && tweens_.count(tween->id)) Finish(tween);
    }
    return tweens_.size();
}

bool TweenManager::IsActive(TweenHandle handle) const { return tweens_.count(handle.id) > 0; }

float TweenManager::TotalDuration(TweenHandle handle) const {
    auto tween = Find(handle);
    return tween ? Total(*tween) : 0.0f;
}

float TweenManager::Elapsed(TweenHandle handle) const {
    auto tween = Find(handle);
    return tween ? tween->elapsed : 0.0f;
}

} // namespace Astral::Animation
