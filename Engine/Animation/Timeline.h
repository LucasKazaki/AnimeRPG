#pragma once

// Sequencer timelines (the role of Unreal's Level Sequencer and Unity's
// Timeline): an asset of tracks bound by name to scene objects, and a player
// that evaluates them.
//
//   Float / Vector / Rotation tracks - keyed curves with constant, linear or
//       cubic (auto-tangent Hermite) segments and per-key easing, applied to a
//       named property of the binding ("position", "light.intensity", ...).
//   Event tracks      - named events (with a payload) fired when playback
//                       crosses them, forwards or backwards.
//   Activation tracks - time ranges during which the binding is active.
//
// The player supports play/pause/stop, speed (negative plays in reverse),
// once/hold/loop/ping-pong wrapping, and scrubbing (Seek never fires events
// unless asked). Assets load from and save to JSON; bindings are resolved by a
// TimelineBinder, so the same asset drives any world.

#include "Engine/Animation/Tween.h"
#include "Engine/Core/Json.h"
#include "Engine/Math/VectorMath.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Astral::Animation {

enum class KeyInterpolation : std::uint8_t { Constant, Linear, Cubic };

template <typename T>
struct TimelineKey {
    float time{};
    T value{};
    KeyInterpolation interpolation{KeyInterpolation::Linear}; // of the segment to the next key
    Ease ease{Ease::Linear};                                  // applied to that segment's progress
};

enum class TrackKind : std::uint8_t { Float, Vector, Rotation, Event, Activation };

struct TimelineEvent {
    float time{};
    std::string name;
    std::string payload;
};

struct TimelineTrack {
    TrackKind kind{TrackKind::Float};
    std::string binding;  // scene object name ("" = the timeline itself, for events)
    std::string property; // curve tracks
    std::vector<TimelineKey<float>> floatKeys;
    std::vector<TimelineKey<Math::Vec3>> vectorKeys;
    std::vector<TimelineKey<Math::Quat>> rotationKeys;
    std::vector<TimelineEvent> events;
    std::vector<std::pair<float, float>> ranges; // activation [start, end)
    bool muted{};

    float EvaluateFloat(float time) const;
    Math::Vec3 EvaluateVector(float time) const;
    Math::Quat EvaluateRotation(float time) const;
    bool IsActive(float time) const;
};

class TimelineAsset {
public:
    std::string name;
    float duration{1.0f};
    std::vector<TimelineTrack> tracks;

    // Sorts keys/events by time and checks values (finite, known kinds, ranges).
    bool Validate(std::string& error);
    bool FromJson(const Core::JsonValue& json, std::string& error);
    Core::JsonValue ToJson() const;
};

// Applies evaluated tracks to a scene. Unhandled bindings/properties are ignored.
class TimelineBinder {
public:
    virtual ~TimelineBinder() = default;
    virtual void SetFloat(const std::string& binding, const std::string& property, float value) = 0;
    virtual void SetVector(const std::string& binding, const std::string& property, Math::Vec3 value) = 0;
    virtual void SetRotation(const std::string& binding, const std::string& property, Math::Quat value) = 0;
    virtual void SetActive(const std::string& binding, bool active) = 0;
    virtual void OnEvent(const std::string& binding, const TimelineEvent& event) = 0;
};

enum class TimelineWrap : std::uint8_t { Once, Hold, Loop, PingPong };

class TimelinePlayer {
public:
    TimelinePlayer(std::shared_ptr<const TimelineAsset> asset, TimelineBinder* binder);

    void Play();
    void Pause() { playing_ = false; }
    // Back to time 0 (evaluated) and paused.
    void Stop();
    // Jumps to `time` and evaluates; events are only fired when requested.
    void Seek(float time, bool fireEvents = false);
    // Advances playback by dt * speed and evaluates.
    void Update(float dt);

    float Time() const { return time_; }
    bool IsPlaying() const { return playing_; }
    // Once mode: reached the end (the player stops); Hold keeps evaluating the end.
    bool Finished() const { return finished_; }
    const TimelineAsset& Asset() const { return *asset_; }

    TimelineWrap wrap{TimelineWrap::Once};
    float speed{1.0f};

private:
    void Evaluate();
    void FireEvents(float from, float to);

    std::shared_ptr<const TimelineAsset> asset_;
    TimelineBinder* binder_;
    float time_{};
    float direction_{1.0f}; // ping-pong
    bool playing_{};
    bool finished_{};
    std::vector<int> activeState_; // -1 unknown, 0/1 last applied, per track
};

} // namespace Astral::Animation
