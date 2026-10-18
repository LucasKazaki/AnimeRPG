#include "Engine/Core/Json.h"

#include <charconv>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <sstream>
#include <system_error>

namespace Astral::Core {

// ------------------------------------------------------------------ JsonValue

JsonValue JsonValue::MakeArray() {
    JsonValue value;
    value.type_ = Type::Array;
    return value;
}

JsonValue JsonValue::MakeObject() {
    JsonValue value;
    value.type_ = Type::Object;
    return value;
}

const JsonValue& JsonValue::NullValue() {
    static const JsonValue null;
    return null;
}

float JsonValue::AsFloat(float fallback) const {
    if (type_ != Type::Number) return fallback;
    const double clamped = std::fabs(number_) > std::numeric_limits<float>::max()
        ? std::copysign(static_cast<double>(std::numeric_limits<float>::max()), number_)
        : number_;
    return static_cast<float>(clamped);
}

std::int64_t JsonValue::AsInt(std::int64_t fallback) const {
    if (type_ != Type::Number || !std::isfinite(number_) || std::floor(number_) != number_) return fallback;
    // 2^63 is exactly representable; anything at or above it overflows int64.
    if (number_ < -9223372036854775808.0 || number_ >= 9223372036854775808.0) return fallback;
    return static_cast<std::int64_t>(number_);
}

const std::string& JsonValue::AsString() const {
    static const std::string empty;
    return type_ == Type::String ? string_ : empty;
}

std::string JsonValue::AsString(std::string_view fallback) const {
    return type_ == Type::String ? string_ : std::string(fallback);
}

std::size_t JsonValue::Size() const {
    return type_ == Type::Array || type_ == Type::Object ? items_.size() : 0;
}

const JsonValue& JsonValue::operator[](std::size_t index) const {
    return type_ == Type::Array && index < items_.size() ? items_[index] : NullValue();
}

JsonValue& JsonValue::Append(JsonValue value) {
    if (type_ != Type::Array) {
        *this = MakeArray();
    }
    items_.push_back(std::move(value));
    return items_.back();
}

const JsonValue* JsonValue::Find(std::string_view key) const {
    if (type_ != Type::Object) return nullptr;
    for (std::size_t i = 0; i < keys_.size(); ++i) {
        if (keys_[i] == key) return &items_[i];
    }
    return nullptr;
}

JsonValue* JsonValue::Find(std::string_view key) {
    return const_cast<JsonValue*>(static_cast<const JsonValue&>(*this).Find(key));
}

const JsonValue& JsonValue::operator[](std::string_view key) const {
    const JsonValue* found = Find(key);
    return found ? *found : NullValue();
}

JsonValue& JsonValue::Set(std::string_view key, JsonValue value) {
    if (type_ != Type::Object) *this = MakeObject();
    if (JsonValue* existing = Find(key)) {
        *existing = std::move(value);
        return *existing;
    }
    keys_.emplace_back(key);
    items_.push_back(std::move(value));
    return items_.back();
}

bool JsonValue::Erase(std::string_view key) {
    if (type_ != Type::Object) return false;
    for (std::size_t i = 0; i < keys_.size(); ++i) {
        if (keys_[i] == key) {
            keys_.erase(keys_.begin() + static_cast<std::ptrdiff_t>(i));
            items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(i));
            return true;
        }
    }
    return false;
}

double JsonValue::Number(std::string_view key, double fallback) const { return (*this)[key].AsNumber(fallback); }
float JsonValue::Float(std::string_view key, float fallback) const { return (*this)[key].AsFloat(fallback); }
std::int64_t JsonValue::Int(std::string_view key, std::int64_t fallback) const { return (*this)[key].AsInt(fallback); }
bool JsonValue::Bool(std::string_view key, bool fallback) const { return (*this)[key].AsBool(fallback); }
std::string JsonValue::String(std::string_view key, std::string_view fallback) const {
    return (*this)[key].AsString(fallback);
}

bool JsonValue::operator==(const JsonValue& other) const {
    if (type_ != other.type_) return false;
    switch (type_) {
    case Type::Null: return true;
    case Type::Bool: return bool_ == other.bool_;
    case Type::Number: return number_ == other.number_;
    case Type::String: return string_ == other.string_;
    case Type::Array: return items_ == other.items_;
    case Type::Object: return keys_ == other.keys_ && items_ == other.items_;
    }
    return false;
}

// ------------------------------------------------------------------ Parser

namespace {

class Parser {
public:
    Parser(std::string_view text, const JsonParseOptions& options) : text_(text), options_(options) {}

    bool Parse(JsonValue& out, std::string& error) {
        if (text_.size() > options_.maxBytes) return Fail(error, "document exceeds the size limit");
        // Tolerate a UTF-8 byte-order mark.
        if (text_.size() >= 3 && static_cast<unsigned char>(text_[0]) == 0xEF
            && static_cast<unsigned char>(text_[1]) == 0xBB && static_cast<unsigned char>(text_[2]) == 0xBF) {
            position_ = 3;
        }
        JsonValue value;
        if (!SkipSpace(error) || !ParseValue(value, 0, error)) return false;
        if (!SkipSpace(error)) return false;
        if (position_ != text_.size()) return Fail(error, "unexpected content after the document");
        out = std::move(value);
        return true;
    }

private:
    bool Fail(std::string& error, const char* message) const {
        std::size_t line = 1, column = 1;
        for (std::size_t i = 0; i < position_ && i < text_.size(); ++i) {
            if (text_[i] == '\n') {
                ++line;
                column = 1;
            } else {
                ++column;
            }
        }
        error = std::to_string(line) + ":" + std::to_string(column) + ": " + message;
        return false;
    }

    bool AtEnd() const { return position_ >= text_.size(); }
    char Peek() const { return AtEnd() ? '\0' : text_[position_]; }

    bool SkipSpace(std::string& error) {
        while (!AtEnd()) {
            const char c = text_[position_];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                ++position_;
            } else if (c == '/' && options_.allowComments && position_ + 1 < text_.size()) {
                if (text_[position_ + 1] == '/') {
                    while (!AtEnd() && text_[position_] != '\n') ++position_;
                } else if (text_[position_ + 1] == '*') {
                    const std::size_t close = text_.find("*/", position_ + 2);
                    if (close == std::string_view::npos) return Fail(error, "unterminated comment");
                    position_ = close + 2;
                } else {
                    return true;
                }
            } else {
                return true;
            }
        }
        return true;
    }

    bool Expect(std::string_view word, std::string& error) {
        if (text_.substr(position_, word.size()) != word) return Fail(error, "invalid literal");
        position_ += word.size();
        return true;
    }

    bool ParseValue(JsonValue& out, std::size_t depth, std::string& error) {
        if (depth > options_.maxDepth) return Fail(error, "nesting exceeds the depth limit");
        switch (Peek()) {
        case '{': return ParseObject(out, depth, error);
        case '[': return ParseArray(out, depth, error);
        case '"': {
            std::string text;
            if (!ParseString(text, error)) return false;
            out = JsonValue(std::move(text));
            return true;
        }
        case 't':
            if (!Expect("true", error)) return false;
            out = JsonValue(true);
            return true;
        case 'f':
            if (!Expect("false", error)) return false;
            out = JsonValue(false);
            return true;
        case 'n':
            if (!Expect("null", error)) return false;
            out = JsonValue();
            return true;
        default: return ParseNumber(out, error);
        }
    }

    bool ParseNumber(JsonValue& out, std::string& error) {
        const std::size_t start = position_;
        if (Peek() == '-') ++position_;
        if (Peek() == '0') {
            ++position_;
        } else if (Peek() >= '1' && Peek() <= '9') {
            while (Peek() >= '0' && Peek() <= '9') ++position_;
        } else {
            return Fail(error, "expected a value");
        }
        if (Peek() == '.') {
            ++position_;
            if (!(Peek() >= '0' && Peek() <= '9')) return Fail(error, "expected digits after the decimal point");
            while (Peek() >= '0' && Peek() <= '9') ++position_;
        }
        if (Peek() == 'e' || Peek() == 'E') {
            ++position_;
            if (Peek() == '+' || Peek() == '-') ++position_;
            if (!(Peek() >= '0' && Peek() <= '9')) return Fail(error, "expected exponent digits");
            while (Peek() >= '0' && Peek() <= '9') ++position_;
        }
        double value = 0.0;
        const char* first = text_.data() + start;
        const char* last = text_.data() + position_;
        const auto result = std::from_chars(first, last, value);
        if (result.ec == std::errc::result_out_of_range && result.ptr == last) {
            // Tiny magnitudes (negative exponent) flush to zero; huge ones saturate.
            const std::string_view literal(first, static_cast<std::size_t>(last - first));
            const std::size_t e = literal.find_first_of("eE");
            const bool tiny = e != std::string_view::npos && e + 1 < literal.size() && literal[e + 1] == '-';
            const double magnitude = tiny ? 0.0 : std::numeric_limits<double>::max();
            value = *first == '-' ? -magnitude : magnitude;
        } else if (result.ec != std::errc() || result.ptr != last) {
            return Fail(error, "malformed number");
        }
        out = JsonValue(value);
        return true;
    }

    static void AppendUtf8(std::string& out, std::uint32_t codepoint) {
        if (codepoint < 0x80) {
            out.push_back(static_cast<char>(codepoint));
        } else if (codepoint < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else if (codepoint < 0x10000) {
            out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
    }

    bool ParseHex4(std::uint32_t& value, std::string& error) {
        value = 0;
        for (int i = 0; i < 4; ++i) {
            const char c = Peek();
            std::uint32_t digit = 0;
            if (c >= '0' && c <= '9') digit = static_cast<std::uint32_t>(c - '0');
            else if (c >= 'a' && c <= 'f') digit = static_cast<std::uint32_t>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') digit = static_cast<std::uint32_t>(c - 'A' + 10);
            else return Fail(error, "invalid \\u escape");
            value = value * 16u + digit;
            ++position_;
        }
        return true;
    }

    bool ParseString(std::string& out, std::string& error) {
        ++position_; // opening quote
        while (true) {
            if (AtEnd()) return Fail(error, "unterminated string");
            const char c = text_[position_];
            if (c == '"') {
                ++position_;
                return true;
            }
            if (static_cast<unsigned char>(c) < 0x20) return Fail(error, "control character in string");
            if (c != '\\') {
                out.push_back(c);
                ++position_;
                continue;
            }
            ++position_;
            if (AtEnd()) return Fail(error, "unterminated escape");
            const char escape = text_[position_++];
            switch (escape) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'u': {
                std::uint32_t codepoint = 0;
                if (!ParseHex4(codepoint, error)) return false;
                if (codepoint >= 0xD800 && codepoint <= 0xDBFF) {
                    if (text_.substr(position_, 2) != "\\u") return Fail(error, "unpaired surrogate");
                    position_ += 2;
                    std::uint32_t low = 0;
                    if (!ParseHex4(low, error)) return false;
                    if (low < 0xDC00 || low > 0xDFFF) return Fail(error, "invalid low surrogate");
                    codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
                } else if (codepoint >= 0xDC00 && codepoint <= 0xDFFF) {
                    return Fail(error, "unpaired surrogate");
                }
                AppendUtf8(out, codepoint);
                break;
            }
            default: return Fail(error, "invalid escape");
            }
        }
    }

    bool ParseArray(JsonValue& out, std::size_t depth, std::string& error) {
        ++position_;
        JsonValue array = JsonValue::MakeArray();
        if (!SkipSpace(error)) return false;
        if (Peek() == ']') {
            ++position_;
            out = std::move(array);
            return true;
        }
        while (true) {
            JsonValue item;
            if (!SkipSpace(error) || !ParseValue(item, depth + 1, error)) return false;
            array.Append(std::move(item));
            if (!SkipSpace(error)) return false;
            if (Peek() == ',') {
                ++position_;
                if (!SkipSpace(error)) return false;
                if (options_.allowTrailingCommas && Peek() == ']') {
                    ++position_;
                    break;
                }
                continue;
            }
            if (Peek() == ']') {
                ++position_;
                break;
            }
            return Fail(error, "expected ',' or ']'");
        }
        out = std::move(array);
        return true;
    }

    bool ParseObject(JsonValue& out, std::size_t depth, std::string& error) {
        ++position_;
        JsonValue object = JsonValue::MakeObject();
        if (!SkipSpace(error)) return false;
        if (Peek() == '}') {
            ++position_;
            out = std::move(object);
            return true;
        }
        while (true) {
            if (!SkipSpace(error)) return false;
            if (Peek() != '"') return Fail(error, "expected a string key");
            std::string key;
            if (!ParseString(key, error)) return false;
            if (!SkipSpace(error)) return false;
            if (Peek() != ':') return Fail(error, "expected ':'");
            ++position_;
            JsonValue value;
            if (!SkipSpace(error) || !ParseValue(value, depth + 1, error)) return false;
            if (object.Has(key)) return Fail(error, "duplicate key");
            object.Set(key, std::move(value));
            if (!SkipSpace(error)) return false;
            if (Peek() == ',') {
                ++position_;
                if (!SkipSpace(error)) return false;
                if (options_.allowTrailingCommas && Peek() == '}') {
                    ++position_;
                    break;
                }
                continue;
            }
            if (Peek() == '}') {
                ++position_;
                break;
            }
            return Fail(error, "expected ',' or '}'");
        }
        out = std::move(object);
        return true;
    }

    std::string_view text_;
    const JsonParseOptions& options_;
    std::size_t position_{};
};

// ------------------------------------------------------------------ Writer

void WriteString(std::string& out, const std::string& text) {
    out.push_back('"');
    for (const char c : text) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                char buffer[8];
                std::snprintf(buffer, sizeof(buffer), "\\u%04x", static_cast<unsigned>(static_cast<unsigned char>(c)));
                out += buffer;
            } else {
                out.push_back(c);
            }
        }
    }
    out.push_back('"');
}

void WriteNumber(std::string& out, double value) {
    if (!std::isfinite(value)) {
        out += "null";
        return;
    }
    if (value == 0.0) {
        out += "0"; // also normalises -0
        return;
    }
    char buffer[64];
    // Integers print without an exponent or fraction; others use the shortest round-trip form.
    if (std::fabs(value) < 1e15 && std::floor(value) == value) {
        std::snprintf(buffer, sizeof(buffer), "%.0f", value);
        out += buffer;
        return;
    }
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    out.append(buffer, result.ptr);
}

void WriteValue(std::string& out, const JsonValue& value, const JsonWriteOptions& options, int depth) {
    const bool pretty = options.pretty;
    auto newline = [&](int level) {
        if (!pretty) return;
        out.push_back('\n');
        out.append(static_cast<std::size_t>(level * options.indent), ' ');
    };
    switch (value.GetType()) {
    case JsonValue::Type::Null: out += "null"; break;
    case JsonValue::Type::Bool: out += value.AsBool() ? "true" : "false"; break;
    case JsonValue::Type::Number: WriteNumber(out, value.AsNumber()); break;
    case JsonValue::Type::String: WriteString(out, value.AsString()); break;
    case JsonValue::Type::Array: {
        const auto& items = value.Items();
        if (items.empty()) {
            out += "[]";
            break;
        }
        // Short arrays of numbers stay on one line (vectors, colours, matrices).
        bool inlineArray = items.size() <= 16;
        for (const JsonValue& item : items) inlineArray = inlineArray && item.IsNumber();
        out.push_back('[');
        for (std::size_t i = 0; i < items.size(); ++i) {
            if (i > 0) out += (pretty && inlineArray) ? ", " : ",";
            if (!inlineArray) newline(depth + 1);
            WriteValue(out, items[i], options, depth + 1);
        }
        if (!inlineArray) newline(depth);
        out.push_back(']');
        break;
    }
    case JsonValue::Type::Object: {
        const auto& keys = value.Keys();
        const auto& values = value.Values();
        if (keys.empty()) {
            out += "{}";
            break;
        }
        out.push_back('{');
        for (std::size_t i = 0; i < keys.size(); ++i) {
            if (i > 0) out.push_back(',');
            newline(depth + 1);
            WriteString(out, keys[i]);
            out += pretty ? ": " : ":";
            WriteValue(out, values[i], options, depth + 1);
        }
        newline(depth);
        out.push_back('}');
        break;
    }
    }
}

} // namespace

bool ParseJson(std::string_view text, JsonValue& out, std::string& error, const JsonParseOptions& options) {
    Parser parser(text, options);
    return parser.Parse(out, error);
}

std::string WriteJson(const JsonValue& value, const JsonWriteOptions& options) {
    std::string out;
    WriteValue(out, value, options, 0);
    if (options.pretty) out.push_back('\n');
    return out;
}

bool ReadJsonFile(const std::string& path, JsonValue& out, std::string& error, const JsonParseOptions& options) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        error = "cannot open " + path;
        return false;
    }
    std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (!ParseJson(text, out, error, options)) {
        error = path + ":" + error;
        return false;
    }
    return true;
}

bool WriteJsonFile(const std::string& path, const JsonValue& value, std::string& error, const JsonWriteOptions& options) {
    const std::string text = WriteJson(value, options);
    const std::string temporary = path + ".tmp";
    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        if (!file) {
            error = "cannot write " + temporary;
            return false;
        }
        file.write(text.data(), static_cast<std::streamsize>(text.size()));
        if (!file) {
            error = "short write to " + temporary;
            return false;
        }
    }
    // std::filesystem::rename replaces an existing target in one step on every
    // platform (POSIX rename, MoveFileEx with MOVEFILE_REPLACE_EXISTING), so
    // readers never observe a missing or half-written file.
    std::error_code ec;
    std::filesystem::rename(temporary, path, ec);
    if (ec) {
        std::remove(temporary.c_str());
        error = "cannot move " + temporary + " into place: " + ec.message();
        return false;
    }
    return true;
}

} // namespace Astral::Core
