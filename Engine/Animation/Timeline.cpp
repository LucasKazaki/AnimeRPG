#include "Engine/Animation/Timeline.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <utility>

namespace Astral::Animation {

using Core::JsonValue;
using namespace Math;

namespace {

const char* const kKindNames[] = {"float", "vector", "rotation", "event", "activation"};
const char* const kInterpolationNames[] = {"constant", "linear", "cubic"};

bool Fail(std::string& error, const std::string& path, const std::string& message) {
    error = path + ": " + message;
    return false;
}

// Index of the key segment containing `time` (keys sorted, time strictly inside).
template <typename T>
std::size_t Segment(const std::vector<TimelineKey<T>>& keys, float time) {
    const auto next = std::upper_bound(keys.begin(), keys.end(), time,
        [](float t, const TimelineKey<T>& key) { return t < key.time; });
    return static_cast<std::size_t>(next - keys.begin()) - 1;
}

// Auto tangent (value per second) at key i: centred difference, flat at the ends.
template <typename T>
T Tangent(const std::vector<TimelineKey<T>>& keys, std::size_t i) {
    if (i == 0 || i + 1 >= keys.size()) return T{};
    const float span = keys[i + 1].time - keys[i - 1].time;
    return span > 0.0f ? (keys[i + 1].value - keys[i - 1].value) * (1.0f / span) : T{};
}

template <typename T>
T EvaluateCurve(const std::vector<TimelineKey<T>>& keys, float time) {
    if (keys.empty()) return T{};
    if (!(time > keys.front().time)) return keys.front().value;
    if (time >= keys.back().time) return keys.back().value;
    const std::size_t i = Segment(keys, time);
    const TimelineKey<T>& a = keys[i];
    const TimelineKey<T>& b = keys[i + 1];
    const float span = b.time - a.time;
    if (!(span > 0.0f)) return b.value;
    const float u = EaseValue(a.ease, (time - a.time) / span);
    switch (a.interpolation) {
    case KeyInterpolation::Constant: return a.value;
    case KeyInterpolation::Linear: return a.value + (b.value - a.value) * u;
    case KeyInterpolation::Cubic: {
        const float u2 = u * u, u3 = u2 * u;
        const float h00 = 2.0f * u3 - 3.0f * u2 + 1.0f, h10 = u3 - 2.0f * u2 + u;
        const float h01 = -2.0f * u3 + 3.0f * u2, h11 = u3 - u2;
        return a.value * h00 + Tangent(keys, i) * (h10 * span) + b.value * h01 + Tangent(keys, i + 1) * (h11 * span);
    }
    }
    return a.value;
}

bool ReadNumber(const JsonValue& json, float& out) {
    if (!json.IsNumber() || !std::isfinite(json.AsNumber())) return false;
    out = json.AsFloat();
    return true;
}

bool ReadVec3(const JsonValue& json, Vec3& out) {
    if (!json.IsArray() || json.Size() != 3) return false;
    return ReadNumber(json[0], out.x) && ReadNumber(json[1], out.y) && ReadNumber(json[2], out.z);
}

bool ReadQuat(const JsonValue& json, Quat& out) {
    if (json.IsArray() && json.Size() == 4) {
        Quat q;
        if (!ReadNumber(json[0], q.x) || !ReadNumber(json[1], q.y) || !ReadNumber(json[2], q.z) || !ReadNumber(json[3], q.w)) {
            return false;
        }
        const float lengthSquared = Dot(q, q);
        if (!(lengthSquared > 1.0e-12f)) return false;
        out = std::fabs(lengthSquared - 1.0f) <= 1.0e-5f ? q : Normalize(q); // unit input stays bit-exact
        return true;
    }
    Vec3 euler;
    if (json.IsObject() && json.Find("euler") && ReadVec3(json["euler"], euler)) {
        out = QuatFromEuler(Radians(euler.y), Radians(euler.x), Radians(euler.z));
        return true;
    }
    return false;
}

JsonValue Array(std::initializer_list<float> values) {
    JsonValue array = JsonValue::MakeArray();
    for (float v : values) array.Append(v);
    return array;
}

template <typename T, typename Read>
bool ReadKeys(const JsonValue& json, const std::string& path, std::vector<TimelineKey<T>>& out, Read read, std::string& error) {
    if (!json.IsArray()) return Fail(error, path, "must be an array of keys");
    for (std::size_t i = 0; i < json.Size(); ++i) {
        const JsonValue& key = json[i];
        const std::string keyPath = path + "[" + std::to_string(i) + "]";
        if (!key.IsObject()) return Fail(error, keyPath, "must be an object");
        for (const std::string& name : key.Keys()) {
            if (name != "t" && name != "v" && name != "interp" && name != "ease") return Fail(error, keyPath + "." + name, "unknown key");
        }
        TimelineKey<T> parsed;
        if (!key.Find("t") || !ReadNumber(key["t"], parsed.time)) return Fail(error, keyPath + ".t", "must be a number");
        if (!key.Find("v") || !read(key["v"], parsed.value)) return Fail(error, keyPath + ".v", "has the wrong type");
        if (const JsonValue* interp = key.Find("interp")) {
            bool found = false;
            for (int k = 0; k < 3; ++k) {
                if (interp->IsString() && interp->AsString() == kInterpolationNames[k]) {
                    parsed.interpolation = static_cast<KeyInterpolation>(k);
                    found = true;
                }
            }
            if (!found) return Fail(error, keyPath + ".interp", "must be constant, linear or cubic");
        }
        if (const JsonValue* ease = key.Find("ease")) {
            if (!ease->IsString() || !ParseEase(ease->AsString(), parsed.ease)) return Fail(error, keyPath + ".ease", "unknown easing");
        }
        out.push_back(parsed);
    }
    return true;
}

template <typename T, typename Write>
JsonValue WriteKeys(const std::vector<TimelineKey<T>>& keys, Write write) {
    JsonValue array = JsonValue::MakeArray();
    for (const TimelineKey<T>& key : keys) {
        JsonValue item = JsonValue::MakeObject();
        item.Set("t", key.time);
        item.Set("v", write(key.value));
        item.Set("interp", kInterpolationNames[static_cast<int>(key.interpolation)]);
        item.Set("ease", EaseName(key.ease));
        array.Append(item);
    }
    return array;
}

template <typename T>
void SortKeys(std::vector<TimelineKey<T>>& keys) {
    std::stable_sort(keys.begin(), keys.end(), [](const TimelineKey<T>& a, const TimelineKey<T>& b) { return a.time < b.time; });
}

} // namespace

// ------------------------------------------------------------------ tracks

float TimelineTrack::EvaluateFloat(float time) const { return EvaluateCurve(floatKeys, time); }

Vec3 TimelineTrack::EvaluateVector(float time) const { return EvaluateCurve(vectorKeys, time); }

Quat TimelineTrack::EvaluateRotation(float time) const {
    const auto& keys = rotationKeys;
    if (keys.empty()) return {};
    if (!(time > keys.front().time)) return keys.front().value;
    if (time >= keys.back().time) return keys.back().value;
    const std::size_t i = Segment(keys, time);
    const float span = keys[i + 1].time - keys[i].time;
    if (!(span > 0.0f)) return keys[i + 1].value;
    float u = EaseValue(keys[i].ease, (time - keys[i].time) / span);
    switch (keys[i].interpolation) {
    case KeyInterpolation::Constant: return keys[i].value;
    case KeyInterpolation::Linear: break;
    case KeyInterpolation::Cubic: u = u * u * (3.0f - 2.0f * u); break; // eased slerp
    }
    return Slerp(keys[i].value, keys[i + 1].value, u);
}

bool TimelineTrack::IsActive(float time) const {
    for (const auto& range : ranges) {
        if (time >= range.first && time < range.second) return true;
    }
    return false;
}

// ------------------------------------------------------------------ asset

bool TimelineAsset::Validate(std::string& error) {
    if (!std::isfinite(duration) || !(duration > 0.0f)) return Fail(error, "duration", "must be a positive number");
    for (std::size_t i = 0; i < tracks.size(); ++i) {
        TimelineTrack& track = tracks[i];
        const std::string path = "tracks[" + std::to_string(i) + "]";
        const bool curve = track.kind == TrackKind::Float || track.kind == TrackKind::Vector || track.kind == TrackKind::Rotation;
        if (curve && track.property.empty()) return Fail(error, path, "curve tracks need a property");
        for (const auto& key : track.floatKeys) {
            if (!std::isfinite(key.time) || !std::isfinite(key.value)) return Fail(error, path, "non-finite key");
        }
        for (const auto& key : track.vectorKeys) {
            if (!std::isfinite(key.time) || !IsFinite(key.value)) return Fail(error, path, "non-finite key");
        }
        for (auto& key : track.rotationKeys) {
            if (!std::isfinite(key.time) || !std::isfinite(key.value.x) || !std::isfinite(key.value.y)
                || !std::isfinite(key.value.z) || !std::isfinite(key.value.w) || !(Dot(key.value, key.value) > 1.0e-12f)) {
                return Fail(error, path, "invalid rotation key");
            }
            if (std::fabs(Dot(key.value, key.value) - 1.0f) > 1.0e-5f) key.value = Normalize(key.value);
        }
        for (const auto& event : track.events) {
            if (!std::isfinite(event.time)) return Fail(error, path, "non-finite event time");
        }
        for (const auto& range : track.ranges) {
            if (!std::isfinite(range.first) || !std::isfinite(range.second) || !(range.first < range.second)) {
                return Fail(error, path, "activation ranges need start < end");
            }
        }
        SortKeys(track.floatKeys);
        SortKeys(track.vectorKeys);
        SortKeys(track.rotationKeys);
        std::stable_sort(track.events.begin(), track.events.end(),
            [](const TimelineEvent& a, const TimelineEvent& b) { return a.time < b.time; });
    }
    return true;
}

bool TimelineAsset::FromJson(const JsonValue& json, std::string& error) {
    TimelineAsset parsed;
    if (!json.IsObject()) return Fail(error, "timeline", "must be an object");
    for (const std::string& key : json.Keys()) {
        if (key != "name" && key != "duration" && key != "tracks") return Fail(error, key, "unknown key");
    }
    parsed.name = json.String("name");
    if (!json.Find("duration") || !ReadNumber(json["duration"], parsed.duration)) return Fail(error, "duration", "must be a number");
    if (const JsonValue* tracks = json.Find("tracks")) {
        if (!tracks->IsArray()) return Fail(error, "tracks", "must be an array");
        for (std::size_t i = 0; i < tracks->Size(); ++i) {
            const JsonValue& source = (*tracks)[i];
            const std::string path = "tracks[" + std::to_string(i) + "]";
            if (!source.IsObject()) return Fail(error, path, "must be an object");
            for (const std::string& key : source.Keys()) {
                if (key != "type" && key != "binding" && key != "property" && key != "keys" && key != "events"
                    && key != "ranges" && key != "muted") {
                    return Fail(error, path + "." + key, "unknown key");
                }
            }
            TimelineTrack track;
            const std::string type = source.String("type");
            bool known = false;
            for (int k = 0; k < 5; ++k) {
                if (type == kKindNames[k]) {
                    track.kind = static_cast<TrackKind>(k);
                    known = true;
                }
            }
            if (!known) return Fail(error, path + ".type", "must be float, vector, rotation, event or activation");
            track.binding = source.String("binding");
            track.property = source.String("property");
            track.muted = source.Bool("muted");
            const JsonValue& keys = source["keys"];
            bool ok = true;
            switch (track.kind) {
            case TrackKind::Float: ok = ReadKeys(keys, path + ".keys", track.floatKeys, ReadNumber, error); break;
            case TrackKind::Vector: ok = ReadKeys(keys, path + ".keys", track.vectorKeys, ReadVec3, error); break;
            case TrackKind::Rotation: ok = ReadKeys(keys, path + ".keys", track.rotationKeys, ReadQuat, error); break;
            case TrackKind::Event: {
                const JsonValue& events = source["events"];
                if (!events.IsArray()) return Fail(error, path + ".events", "must be an array");
                for (std::size_t e = 0; e < events.Size(); ++e) {
                    const JsonValue& item = events[e];
                    TimelineEvent event;
                    if (!item.IsObject() || !item.Find("t") || !ReadNumber(item["t"], event.time) || !item["name"].IsString()) {
                        return Fail(error, path + ".events[" + std::to_string(e) + "]", "needs a numeric \"t\" and a \"name\"");
                    }
                    event.name = item["name"].AsString();
                    event.payload = item.String("payload");
                    track.events.push_back(std::move(event));
                }
                break;
            }
            case TrackKind::Activation: {
                const JsonValue& ranges = source["ranges"];
                if (!ranges.IsArray()) return Fail(error, path + ".ranges", "must be an array");
                for (std::size_t r = 0; r < ranges.Size(); ++r) {
                    float start = 0.0f, end = 0.0f;
                    if (!ranges[r].IsArray() || ranges[r].Size() != 2 || !ReadNumber(ranges[r][0], start)
                        || !ReadNumber(ranges[r][1], end)) {
                        return Fail(error, path + ".ranges[" + std::to_string(r) + "]", "must be [start, end]");
                    }
                    track.ranges.emplace_back(start, end);
                }
                break;
            }
            }
            if (!ok) return false;
            parsed.tracks.push_back(std::move(track));
        }
    }
    if (!parsed.Validate(error)) return false;
    *this = std::move(parsed);
    return true;
}

JsonValue TimelineAsset::ToJson() const {
    JsonValue json = JsonValue::MakeObject();
    json.Set("name", name);
    json.Set("duration", duration);
    JsonValue list = JsonValue::MakeArray();
    for (const TimelineTrack& track : tracks) {
        JsonValue item = JsonValue::MakeObject();
        item.Set("type", kKindNames[static_cast<int>(track.kind)]);
        item.Set("binding", track.binding);
        if (!track.property.empty()) item.Set("property", track.property);
        if (track.muted) item.Set("muted", true);
        switch (track.kind) {
        case TrackKind::Float: item.Set("keys", WriteKeys(track.floatKeys, [](float v) { return JsonValue(v); })); break;
        case TrackKind::Vector:
            item.Set("keys", WriteKeys(track.vectorKeys, [](Vec3 v) { return Array({v.x, v.y, v.z}); }));
            break;
        case TrackKind::Rotation:
            item.Set("keys", WriteKeys(track.rotationKeys, [](Quat q) { return Array({q.x, q.y, q.z, q.w}); }));
            break;
        case TrackKind::Event: {
            JsonValue events = JsonValue::MakeArray();
            for (const TimelineEvent& event : track.events) {
                JsonValue e = JsonValue::MakeObject();
                e.Set("t", event.time);
                e.Set("name", event.name);
                if (!event.payload.empty()) e.Set("payload", event.payload);
                events.Append(e);
            }
            item.Set("events", events);
            break;
        }
        case TrackKind::Activation: {
            JsonValue ranges = JsonValue::MakeArray();
            for (const auto& range : track.ranges) ranges.Append(Array({range.first, range.second}));
            item.Set("ranges", ranges);
            break;
        }
        }
        list.Append(item);
    }
    json.Set("tracks", list);
    return json;
}

// ------------------------------------------------------------------ player

TimelinePlayer::TimelinePlayer(std::shared_ptr<const TimelineAsset> asset, TimelineBinder* binder)
    : asset_(std::move(asset)), binder_(binder) {
    if (!asset_) asset_ = std::make_shared<TimelineAsset>();
    activeState_.assign(asset_->tracks.size(), -1);
}

void TimelinePlayer::Play() {
    const float duration = asset_->duration;
    if (finished_) {
        // Replaying a finished Once/Hold timeline starts over.
        time_ = speed >= 0.0f ? 0.0f : duration;
        direction_ = 1.0f;
        finished_ = false;
        Evaluate();
    }
    if (!playing_ && speed >= 0.0f && time_ <= 0.0f) FireEvents(-1.0f, 0.0f); // events at the very start
    playing_ = true;
}

void TimelinePlayer::Stop() {
    playing_ = false;
    finished_ = false;
    direction_ = 1.0f;
    time_ = 0.0f;
    Evaluate();
}

void TimelinePlayer::Seek(float time, bool fireEvents) {
    if (!std::isfinite(time)) return;
    time = std::clamp(time, 0.0f, asset_->duration);
    if (fireEvents) FireEvents(time_, time);
    time_ = time;
    finished_ = false;
    Evaluate();
}

void TimelinePlayer::Update(float dt) {
    if (!playing_ || !std::isfinite(dt) || dt <= 0.0f || !std::isfinite(speed)) return;
    const float duration = asset_->duration;
    float remaining = dt * std::fabs(speed);
    float heading = (speed >= 0.0f ? 1.0f : -1.0f) * direction_;
    // Walk the (possibly wrapping) path so events on every lap fire in order.
    for (int guard = 0; guard < 1000 && remaining > 0.0f; ++guard) {
        const float target = heading > 0.0f ? duration : 0.0f;
        const float room = std::fabs(target - time_);
        if (remaining < room) {
            const float next = time_ + heading * remaining;
            FireEvents(time_, next);
            time_ = next;
            remaining = 0.0f;
            break;
        }
        FireEvents(time_, target);
        time_ = target;
        remaining -= room;
        switch (wrap) {
        case TimelineWrap::Once:
            playing_ = false;
            finished_ = true;
            remaining = 0.0f;
            break;
        case TimelineWrap::Hold:
            finished_ = true;
            remaining = 0.0f;
            break;
        case TimelineWrap::Loop:
            time_ = heading > 0.0f ? 0.0f : duration;
            FireEvents(heading > 0.0f ? -1.0f : duration + 1.0f, time_); // the new lap's first instant
            break;
        case TimelineWrap::PingPong:
            direction_ = -direction_;
            heading = -heading;
            break;
        }
    }
    Evaluate();
}

void TimelinePlayer::FireEvents(float from, float to) {
    if (!binder_ || from == to) return;
    for (const TimelineTrack& track : asset_->tracks) {
        if (track.kind != TrackKind::Event || track.muted) continue;
        if (to > from) {
            for (const TimelineEvent& event : track.events) {
                if (event.time > from && event.time <= to) binder_->OnEvent(track.binding, event);
            }
        } else {
            for (auto it = track.events.rbegin(); it != track.events.rend(); ++it) {
                if (it->time < from && it->time >= to) binder_->OnEvent(track.binding, *it);
            }
        }
    }
}

void TimelinePlayer::Evaluate() {
    if (!binder_) return;
    const auto& tracks = asset_->tracks;
    if (activeState_.size() != tracks.size()) activeState_.assign(tracks.size(), -1);
    for (std::size_t i = 0; i < tracks.size(); ++i) {
        const TimelineTrack& track = tracks[i];
        if (track.muted) continue;
        switch (track.kind) {
        case TrackKind::Float:
            if (!track.floatKeys.empty()) binder_->SetFloat(track.binding, track.property, track.EvaluateFloat(time_));
            break;
        case TrackKind::Vector:
            if (!track.vectorKeys.empty()) binder_->SetVector(track.binding, track.property, track.EvaluateVector(time_));
            break;
        case TrackKind::Rotation:
            if (!track.rotationKeys.empty()) binder_->SetRotation(track.binding, track.property, track.EvaluateRotation(time_));
            break;
        case TrackKind::Activation: {
            const int active = track.IsActive(time_) ? 1 : 0;
            if (activeState_[i] != active) {
                activeState_[i] = active;
                binder_->SetActive(track.binding, active == 1);
            }
            break;
        }
        case TrackKind::Event: break;
        }
    }
}

} // namespace Astral::Animation
