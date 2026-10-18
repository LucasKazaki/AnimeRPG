#pragma once

// Easing curves and tweens (the role of DOTween/LeanTween in Unity and
// timeline curves in Unreal): the standard Penner easing set, value tweens
// driven through setters, loops (restart or yoyo), delays, per-tween time scale,
// completion callbacks, owner tags for bulk cancellation, and sequences that
// append, join or insert tweens, intervals and callbacks. Every tween is a pure
// function of its elapsed time, so sequences can scrub, rewind and loop them
// exactly.

#include "Engine/Math/VectorMath.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Astral::Animation {

enum class Ease : std::uint8_t {
    Linear,
    InQuad, OutQuad, InOutQuad,
    InCubic, OutCubic, InOutCubic,
    InQuart, OutQuart, InOutQuart,
    InQuint, OutQuint, InOutQuint,
    InSine, OutSine, InOutSine,
    InExpo, OutExpo, InOutExpo,
    InCirc, OutCirc, InOutCirc,
    InBack, OutBack, InOutBack,
    InElastic, OutElastic, InOutElastic,
    InBounce, OutBounce, InOutBounce,
    Count
};

// Eased progress for t in [0, 1] (clamped). Back and elastic overshoot.
// Every curve maps 0 to 0 and 1 to 1.
float EaseValue(Ease ease, float t);
const char* EaseName(Ease ease);
bool ParseEase(std::string_view name, Ease& out);

enum class LoopMode : std::uint8_t { Restart, Yoyo };

struct TweenHandle {
    std::uint64_t id{};
    bool IsNull() const { return id == 0; }
    friend bool operator==(TweenHandle a, TweenHandle b) { return a.id == b.id; }
    friend bool operator!=(TweenHandle a, TweenHandle b) { return a.id != b.id; }
};

class TweenManager {
public:
    // Calls apply(easedProgress) as time advances; the building block of the
    // typed tweens below. A duration <= 0 completes on the first update.
    TweenHandle Custom(float duration, std::function<void(float)> apply, Ease ease = Ease::OutQuad);
    TweenHandle Float(float from, float to, float duration, std::function<void(float)> setter, Ease ease = Ease::OutQuad);
    TweenHandle Vector(Math::Vec3 from, Math::Vec3 to, float duration, std::function<void(Math::Vec3)> setter,
        Ease ease = Ease::OutQuad);
    TweenHandle Rotation(Math::Quat from, Math::Quat to, float duration, std::function<void(Math::Quat)> setter,
        Ease ease = Ease::OutQuad);
    // A container whose children play on its own timeline.
    TweenHandle Sequence();

    // Options (return false for an unknown handle).
    bool SetDelay(TweenHandle tween, float seconds);
    // loops < 0 repeats forever.
    bool SetLoops(TweenHandle tween, int loops, LoopMode mode = LoopMode::Restart);
    bool SetTimeScale(TweenHandle tween, float scale);
    bool SetEase(TweenHandle tween, Ease ease);
    bool OnComplete(TweenHandle tween, std::function<void()> callback);
    // Owner tag for KillOwner (e.g. an entity), 0 = none.
    bool SetOwner(TweenHandle tween, std::uint64_t owner);

    // Sequences: the child tween is moved into the sequence. Append starts it
    // when everything so far ends; Join starts it with the previous append;
    // Insert places it at an absolute time. Children must be fresh tweens.
    bool Append(TweenHandle sequence, TweenHandle child);
    bool Join(TweenHandle sequence, TweenHandle child);
    bool Insert(TweenHandle sequence, float time, TweenHandle child);
    bool AppendInterval(TweenHandle sequence, float seconds);
    bool AppendCallback(TweenHandle sequence, std::function<void()> callback);
    bool InsertCallback(TweenHandle sequence, float time, std::function<void()> callback);

    bool Pause(TweenHandle tween, bool paused);
    // Kills a tween; with complete=true it first jumps to its end state and
    // runs its completion callback (infinite loops end on their current cycle).
    bool Kill(TweenHandle tween, bool complete = false);
    std::size_t KillOwner(std::uint64_t owner, bool complete = false);
    void KillAll();

    // Advances every active top-level tween; completed tweens run their
    // callbacks and are removed. Returns how many tweens remain.
    std::size_t Update(float dt);
    bool IsActive(TweenHandle tween) const;
    // Seconds of the whole tween including delay and loops (infinite -> -1).
    float TotalDuration(TweenHandle tween) const;
    float Elapsed(TweenHandle tween) const;
    std::size_t Count() const { return tweens_.size(); }

private:
    struct Tween;
    struct Child {
        std::shared_ptr<Tween> tween;
        float start{};
    };
    struct Callback {
        float time{};
        std::function<void()> callback;
    };
    struct Tween {
        std::uint64_t id{};
        float duration{};
        float delay{};
        Ease ease{Ease::OutQuad};
        int loops{1};
        LoopMode loopMode{LoopMode::Restart};
        float timeScale{1.0f};
        bool paused{};
        std::uint64_t owner{};
        std::function<void(float)> apply;
        std::function<void()> onComplete;
        bool isSequence{};
        std::vector<Child> children;
        std::vector<Callback> callbacks;
        float lastAppendStart{};
        // Runtime.
        float elapsed{};
        float sampledLocal{-1.0f}; // last sampled local (per-cycle) time, for sequence callbacks
        int sampledCycle{-1};
        bool touched{};
        bool completed{};
        bool parented{};
    };

    TweenHandle Add(std::shared_ptr<Tween> tween);
    std::shared_ptr<Tween> Find(TweenHandle handle) const;
    bool Adopt(const std::shared_ptr<Tween>& sequence, TweenHandle child, float start);
    static float CycleDuration(const Tween& tween);
    static float Total(const Tween& tween);
    // Applies the tween's state at `elapsed` seconds (delay included).
    void Sample(Tween& tween, float elapsed);
    void SampleCycle(Tween& tween, int cycle, float local);
    void Finish(const std::shared_ptr<Tween>& tween);
    void CompleteChildren(Tween& tween);
    void Remove(std::uint64_t id);

    std::map<std::uint64_t, std::shared_ptr<Tween>> tweens_;
    std::uint64_t nextId_{1};
};

} // namespace Astral::Animation
