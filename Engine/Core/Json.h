#pragma once

// JSON document model, parser and writer (RFC 8259) for scenes, prefabs,
// asset metadata, glTF and configuration. Objects keep insertion order so
// written files diff cleanly. The parser is bounded (depth, size), reports
// line:column errors and accepts // and /* */ comments when asked.

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace Astral::Core {

class JsonValue {
public:
    enum class Type : std::uint8_t { Null, Bool, Number, String, Array, Object };

    JsonValue() = default;
    JsonValue(std::nullptr_t) {}
    JsonValue(bool value) : type_(Type::Bool), bool_(value) {}
    // Any integer or floating-point type (bool has its own overload).
    template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> && !std::is_same_v<T, bool>>>
    JsonValue(T value) : type_(Type::Number), number_(static_cast<double>(value)) {}
    JsonValue(const char* value) : type_(Type::String), string_(value ? value : "") {}
    JsonValue(std::string value) : type_(Type::String), string_(std::move(value)) {}
    JsonValue(std::string_view value) : type_(Type::String), string_(value) {}

    static JsonValue MakeArray();
    static JsonValue MakeObject();

    Type GetType() const { return type_; }
    bool IsNull() const { return type_ == Type::Null; }
    bool IsBool() const { return type_ == Type::Bool; }
    bool IsNumber() const { return type_ == Type::Number; }
    bool IsString() const { return type_ == Type::String; }
    bool IsArray() const { return type_ == Type::Array; }
    bool IsObject() const { return type_ == Type::Object; }

    bool AsBool(bool fallback = false) const { return type_ == Type::Bool ? bool_ : fallback; }
    double AsNumber(double fallback = 0.0) const { return type_ == Type::Number ? number_ : fallback; }
    float AsFloat(float fallback = 0.0f) const;
    // Integral view: fallback unless the number is finite, integral and in range.
    std::int64_t AsInt(std::int64_t fallback = 0) const;
    const std::string& AsString() const;
    std::string AsString(std::string_view fallback) const;

    // Arrays.
    std::size_t Size() const;
    const JsonValue& operator[](std::size_t index) const;
    JsonValue& Append(JsonValue value);
    const std::vector<JsonValue>& Items() const { return items_; }
    std::vector<JsonValue>& Items() { return items_; }

    // Objects (insertion-ordered; Set replaces an existing key in place).
    bool Has(std::string_view key) const { return Find(key) != nullptr; }
    const JsonValue* Find(std::string_view key) const;
    JsonValue* Find(std::string_view key);
    const JsonValue& operator[](std::string_view key) const;
    const JsonValue& operator[](const char* key) const { return (*this)[std::string_view(key)]; }
    JsonValue& Set(std::string_view key, JsonValue value);
    bool Erase(std::string_view key);
    const std::vector<std::string>& Keys() const { return keys_; }
    // Object values in key order (parallel to Keys()).
    const std::vector<JsonValue>& Values() const { return items_; }

    // Convenience typed lookups with defaults.
    double Number(std::string_view key, double fallback = 0.0) const;
    float Float(std::string_view key, float fallback = 0.0f) const;
    std::int64_t Int(std::string_view key, std::int64_t fallback = 0) const;
    bool Bool(std::string_view key, bool fallback = false) const;
    std::string String(std::string_view key, std::string_view fallback = {}) const;

    bool operator==(const JsonValue& other) const;
    bool operator!=(const JsonValue& other) const { return !(*this == other); }

    static const JsonValue& NullValue();

private:
    Type type_{Type::Null};
    bool bool_{};
    double number_{};
    std::string string_;
    std::vector<JsonValue> items_; // array items or object values
    std::vector<std::string> keys_; // object keys
};

struct JsonParseOptions {
    std::size_t maxDepth{256};
    std::size_t maxBytes{256u * 1024u * 1024u};
    bool allowComments{true};
    bool allowTrailingCommas{false};
};

// On failure `out` is untouched and `error` holds "line:column: message".
bool ParseJson(std::string_view text, JsonValue& out, std::string& error, const JsonParseOptions& options = {});

struct JsonWriteOptions {
    bool pretty{true};
    int indent{2};
};
// Non-finite numbers are written as null (JSON has no NaN/Infinity).
std::string WriteJson(const JsonValue& value, const JsonWriteOptions& options = {});

bool ReadJsonFile(const std::string& path, JsonValue& out, std::string& error, const JsonParseOptions& options = {});
bool WriteJsonFile(const std::string& path, const JsonValue& value, std::string& error, const JsonWriteOptions& options = {});

} // namespace Astral::Core
