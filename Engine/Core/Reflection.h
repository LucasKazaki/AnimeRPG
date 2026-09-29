#pragma once

// Lightweight runtime reflection for data-driven content (Unreal UPROPERTY,
// Unity serialized fields): types register their editable fields once, and the
// registry then serialises them to/from JSON, validates ranges and enums, and
// lets tools enumerate and edit fields by name without per-type code.
//
//   TypeRegistry::Global().Register<Light>("Light")
//       .Field("intensity", &Light::intensity).Range(0, 100)
//       .Field("color", &Light::color)
//       .EnumField("kind", &Light::kind, {"Directional", "Point"});

#include "Engine/Core/Json.h"
#include "Engine/Math/VectorMath.h"

#include <cmath>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <utility>
#include <vector>

namespace Astral::Core {

enum class FieldKind : std::uint8_t { Bool, Int, Float, String, Vec2, Vec3, Vec4, Quat, Enum };

struct FieldInfo {
    std::string name;
    FieldKind kind{FieldKind::Float};
    std::string tooltip;
    double minimum{-std::numeric_limits<double>::infinity()};
    double maximum{std::numeric_limits<double>::infinity()};
    std::vector<std::string> enumNames;
    std::function<JsonValue(const void*)> read;
    // Returns false (object unchanged for this field) on a type mismatch or bad enum name.
    std::function<bool(void*, const JsonValue&, const FieldInfo&, std::string&)> write;
};

struct TypeInfo {
    std::string name;
    std::size_t size{};
    std::vector<FieldInfo> fields;
    std::function<void*()> create;      // new default instance (owned by the caller)
    std::function<void(void*)> destroy; // deletes an instance from create()

    const FieldInfo* Field(std::string_view fieldName) const;
    JsonValue ToJson(const void* object) const;
    // Missing keys keep their current values. Unknown keys are reported in
    // `error` but do not fail unless `strict`.
    bool FromJson(void* object, const JsonValue& json, std::string& error, bool strict = false) const;
    bool SetField(void* object, std::string_view fieldName, const JsonValue& value, std::string& error) const;
    JsonValue GetField(const void* object, std::string_view fieldName) const;
};

namespace Detail {

inline JsonValue ToJsonArray(std::initializer_list<float> values) {
    JsonValue array = JsonValue::MakeArray();
    for (float v : values) array.Append(v);
    return array;
}

inline bool ReadFloats(const JsonValue& json, float* out, std::size_t count) {
    if (!json.IsArray() || json.Size() != count) return false;
    for (std::size_t i = 0; i < count; ++i) {
        if (!json[i].IsNumber() || !std::isfinite(json[i].AsNumber())) return false;
        out[i] = json[i].AsFloat();
    }
    return true;
}

template <typename M, typename = void>
struct FieldTraits; // unsupported member types fail to compile here

template <>
struct FieldTraits<bool> {
    static constexpr FieldKind kind = FieldKind::Bool;
    static JsonValue Read(const bool& v) { return JsonValue(v); }
    static bool Write(bool& v, const JsonValue& j, const FieldInfo&) {
        if (!j.IsBool()) return false;
        v = j.AsBool();
        return true;
    }
};

template <typename M>
struct FieldTraits<M, std::enable_if_t<std::is_integral_v<M> && !std::is_same_v<M, bool>>> {
    static constexpr FieldKind kind = FieldKind::Int;
    static JsonValue Read(const M& v) { return JsonValue(static_cast<double>(v)); }
    static bool Write(M& v, const JsonValue& j, const FieldInfo& field) {
        if (!j.IsNumber() || !std::isfinite(j.AsNumber())) return false;
        double value = std::round(j.AsNumber());
        value = std::fmax(value, std::fmax(field.minimum, static_cast<double>(std::numeric_limits<M>::lowest())));
        value = std::fmin(value, std::fmin(field.maximum, static_cast<double>(std::numeric_limits<M>::max())));
        v = static_cast<M>(value);
        return true;
    }
};

template <typename M>
struct FieldTraits<M, std::enable_if_t<std::is_floating_point_v<M>>> {
    static constexpr FieldKind kind = FieldKind::Float;
    static JsonValue Read(const M& v) { return JsonValue(static_cast<double>(v)); }
    static bool Write(M& v, const JsonValue& j, const FieldInfo& field) {
        if (!j.IsNumber() || !std::isfinite(j.AsNumber())) return false;
        v = static_cast<M>(std::fmin(std::fmax(j.AsNumber(), field.minimum), field.maximum));
        return true;
    }
};

template <>
struct FieldTraits<std::string> {
    static constexpr FieldKind kind = FieldKind::String;
    static JsonValue Read(const std::string& v) { return JsonValue(v); }
    static bool Write(std::string& v, const JsonValue& j, const FieldInfo&) {
        if (!j.IsString()) return false;
        v = j.AsString();
        return true;
    }
};

template <>
struct FieldTraits<Math::Vec2> {
    static constexpr FieldKind kind = FieldKind::Vec2;
    static JsonValue Read(const Math::Vec2& v) { return ToJsonArray({v.x, v.y}); }
    static bool Write(Math::Vec2& v, const JsonValue& j, const FieldInfo&) {
        float f[2];
        if (!ReadFloats(j, f, 2)) return false;
        v = {f[0], f[1]};
        return true;
    }
};

template <>
struct FieldTraits<Math::Vec3> {
    static constexpr FieldKind kind = FieldKind::Vec3;
    static JsonValue Read(const Math::Vec3& v) { return ToJsonArray({v.x, v.y, v.z}); }
    static bool Write(Math::Vec3& v, const JsonValue& j, const FieldInfo&) {
        float f[3];
        if (!ReadFloats(j, f, 3)) return false;
        v = {f[0], f[1], f[2]};
        return true;
    }
};

template <>
struct FieldTraits<Math::Vec4> {
    static constexpr FieldKind kind = FieldKind::Vec4;
    static JsonValue Read(const Math::Vec4& v) { return ToJsonArray({v.x, v.y, v.z, v.w}); }
    static bool Write(Math::Vec4& v, const JsonValue& j, const FieldInfo&) {
        float f[4];
        if (!ReadFloats(j, f, 4)) return false;
        v = {f[0], f[1], f[2], f[3]};
        return true;
    }
};

template <>
struct FieldTraits<Math::Quat> {
    static constexpr FieldKind kind = FieldKind::Quat;
    static JsonValue Read(const Math::Quat& v) { return ToJsonArray({v.x, v.y, v.z, v.w}); }
    // Accepts [x, y, z, w] or Unity-style Euler degrees {"euler": [x(pitch), y(yaw), z(roll)]}.
    static bool Write(Math::Quat& v, const JsonValue& j, const FieldInfo&) {
        float f[4];
        if (ReadFloats(j, f, 4)) {
            v = Math::Normalize(Math::Quat{f[0], f[1], f[2], f[3]});
            return true;
        }
        if (j.IsObject() && ReadFloats(j["euler"], f, 3)) {
            v = Math::QuatFromEuler(Math::Radians(f[1]), Math::Radians(f[0]), Math::Radians(f[2]));
            return true;
        }
        return false;
    }
};

} // namespace Detail

template <typename T>
class TypeBuilder {
public:
    explicit TypeBuilder(TypeInfo& info) : info_(&info) {}

    template <typename M>
    TypeBuilder& Field(std::string name, M T::*member, std::string tooltip = {}) {
        using Traits = Detail::FieldTraits<M>;
        FieldInfo field;
        field.name = std::move(name);
        field.kind = Traits::kind;
        field.tooltip = std::move(tooltip);
        field.read = [member](const void* object) { return Traits::Read(static_cast<const T*>(object)->*member); };
        field.write = [member](void* object, const JsonValue& json, const FieldInfo& info, std::string& error) {
            if (Traits::Write(static_cast<T*>(object)->*member, json, info)) return true;
            error = "field '" + info.name + "' has the wrong type";
            return false;
        };
        info_->fields.push_back(std::move(field));
        return *this;
    }

    // Enum (or integral) stored in JSON by name.
    template <typename E>
    TypeBuilder& EnumField(std::string name, E T::*member, std::vector<std::string> names, std::string tooltip = {}) {
        FieldInfo field;
        field.name = std::move(name);
        field.kind = FieldKind::Enum;
        field.tooltip = std::move(tooltip);
        field.enumNames = std::move(names);
        field.read = [member, table = field.enumNames](const void* object) {
            const auto index = static_cast<std::size_t>(static_cast<std::int64_t>(static_cast<const T*>(object)->*member));
            return index < table.size() ? JsonValue(table[index]) : JsonValue(static_cast<double>(index));
        };
        field.write = [member](void* object, const JsonValue& json, const FieldInfo& info, std::string& error) {
            for (std::size_t i = 0; i < info.enumNames.size(); ++i) {
                if (json.IsString() && json.AsString() == info.enumNames[i]) {
                    static_cast<T*>(object)->*member = static_cast<E>(i);
                    return true;
                }
            }
            error = "field '" + info.name + "' has no enumerator '" + json.AsString() + "'";
            return false;
        };
        info_->fields.push_back(std::move(field));
        return *this;
    }

    // Clamp range for the most recently added numeric field.
    TypeBuilder& Range(double minimum, double maximum) {
        if (!info_->fields.empty()) {
            info_->fields.back().minimum = minimum;
            info_->fields.back().maximum = maximum;
        }
        return *this;
    }

private:
    TypeInfo* info_;
};

class TypeRegistry {
public:
    static TypeRegistry& Global();

    // Re-registering a name replaces its fields (hot-reload friendly).
    template <typename T>
    TypeBuilder<T> Register(std::string name) {
        static_assert(std::is_default_constructible_v<T>, "reflected types need a default constructor");
        auto info = std::make_unique<TypeInfo>();
        info->name = name;
        info->size = sizeof(T);
        info->create = [] { return static_cast<void*>(new T()); };
        info->destroy = [](void* object) { delete static_cast<T*>(object); };
        TypeInfo& stored = *info;
        const auto previous = byName_.find(name);
        if (previous != byName_.end()) {
            for (auto it = byType_.begin(); it != byType_.end();) {
                it = it->second == previous->second.get() ? byType_.erase(it) : std::next(it);
            }
        }
        byName_[name] = std::move(info);
        byType_[std::type_index(typeid(T))] = &stored;
        return TypeBuilder<T>(stored);
    }
    const TypeInfo* Find(std::string_view name) const;
    template <typename T>
    const TypeInfo* Get() const {
        const auto it = byType_.find(std::type_index(typeid(T)));
        return it == byType_.end() ? nullptr : it->second;
    }
    std::vector<std::string> Names() const;

private:
    std::map<std::string, std::unique_ptr<TypeInfo>, std::less<>> byName_;
    std::map<std::type_index, TypeInfo*> byType_;
};

} // namespace Astral::Core
