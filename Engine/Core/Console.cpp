#include "Engine/Core/Console.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <limits>
#include <utility>

namespace Astral::Core {

namespace {

std::string Lower(std::string_view text) {
    std::string out(text);
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

std::string_view Trim(std::string_view text) {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) text.remove_prefix(1);
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) text.remove_suffix(1);
    return text;
}

bool ParseBool(std::string_view text, bool& value) {
    const std::string lower = Lower(Trim(text));
    if (lower == "1" || lower == "true" || lower == "on" || lower == "yes") {
        value = true;
        return true;
    }
    if (lower == "0" || lower == "false" || lower == "off" || lower == "no") {
        value = false;
        return true;
    }
    return false;
}

bool ParseInt(std::string_view text, long long& value) {
    text = Trim(text);
    if (!text.empty() && text.front() == '+') text.remove_prefix(1);
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    return result.ec == std::errc() && result.ptr == text.data() + text.size() && !text.empty();
}

bool ParseDouble(std::string_view text, double& value) {
    text = Trim(text);
    if (!text.empty() && text.front() == '+') text.remove_prefix(1);
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    return result.ec == std::errc() && result.ptr == text.data() + text.size() && !text.empty() && std::isfinite(value);
}

std::string FormatNumber(CVarType type, double value) {
    char buffer[64];
    switch (type) {
    case CVarType::Bool: return value != 0.0 ? "true" : "false";
    case CVarType::Int: std::snprintf(buffer, sizeof(buffer), "%lld", static_cast<long long>(value)); return buffer;
    default: {
        std::snprintf(buffer, sizeof(buffer), "%.9g", value);
        double rounded = 0.0;
        // Nine digits normally preserve Float display. At the finite boundary
        // they can round the decimal beyond FLT_MAX, making our own config
        // unsafe to reload. Preserve enough of the stored double in that case.
        if (!ParseDouble(buffer, rounded)
            || rounded < -static_cast<double>(std::numeric_limits<float>::max())
            || rounded > static_cast<double>(std::numeric_limits<float>::max())) {
            std::snprintf(buffer, sizeof(buffer), "%.17g", value);
        }
        return buffer;
    }
    }
}

const char* TypeName(CVarType type) {
    switch (type) {
    case CVarType::Bool: return "bool";
    case CVarType::Int: return "int";
    case CVarType::Float: return "float";
    case CVarType::String: return "string";
    }
    return "?";
}

} // namespace

std::vector<std::string> TokenizeCommandLine(std::string_view line) {
    std::vector<std::string> tokens;
    std::string current;
    bool inQuotes = false, hasToken = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (inQuotes) {
            if (c == '\\' && i + 1 < line.size() && (line[i + 1] == '"' || line[i + 1] == '\\')) {
                current.push_back(line[++i]);
            } else if (c == '"') {
                inQuotes = false;
            } else {
                current.push_back(c);
            }
        } else if (c == '"') {
            inQuotes = true;
            hasToken = true;
        } else if (std::isspace(static_cast<unsigned char>(c))) {
            if (hasToken) tokens.push_back(current);
            current.clear();
            hasToken = false;
        } else {
            current.push_back(c);
            hasToken = true;
        }
    }
    if (hasToken) tokens.push_back(current);
    return tokens;
}

// ------------------------------------------------------------------ ConsoleVariable

std::string ConsoleVariable::GetString() const {
    return type_ == CVarType::String ? text_ : FormatNumber(type_, number_);
}

std::string ConsoleVariable::DefaultString() const {
    return type_ == CVarType::String ? defaultText_ : FormatNumber(type_, defaultNumber_);
}

bool ConsoleVariable::SetFromString(std::string_view text, std::string& error) {
    switch (type_) {
    case CVarType::String:
        Set(std::string(text));
        return true;
    case CVarType::Bool: {
        bool value = false;
        if (!ParseBool(text, value)) {
            error = name_ + " expects a bool (0/1, true/false, on/off)";
            return false;
        }
        Set(value ? 1.0 : 0.0);
        return true;
    }
    case CVarType::Int: {
        long long integer = 0;
        double value = 0.0;
        if (ParseInt(text, integer)) {
            value = static_cast<double>(integer);
        } else {
            // Accept an integral float spelling ("2.0").
            if (!ParseDouble(text, value) || std::floor(value) != value) {
                error = name_ + " expects an integer";
                return false;
            }
        }
        double normalized = 0.0;
        if (!NormalizeNumber(value, minimum_, maximum_, normalized)) {
            error = name_ + " is out of the int range";
            return false;
        }
        Set(normalized);
        return true;
    }
    case CVarType::Float: {
        double value = 0.0;
        if (!ParseDouble(text, value)) {
            error = name_ + " expects a number";
            return false;
        }
        double normalized = 0.0;
        if (!NormalizeNumber(value, minimum_, maximum_, normalized)) {
            error = name_ + " is out of the float range";
            return false;
        }
        Set(normalized);
        return true;
    }
    }
    return false;
}

bool ConsoleVariable::NormalizeNumber(double value, double minimum, double maximum, double& normalized) const {
    if (type_ == CVarType::String || !std::isfinite(value)) return false;
    value = std::clamp(value, minimum, maximum);
    if (type_ == CVarType::Int) value = std::round(value);
    if (type_ == CVarType::Bool) value = value != 0.0 ? 1.0 : 0.0;
    if (type_ == CVarType::Int
        && (value < static_cast<double>(std::numeric_limits<int>::lowest())
            || value > static_cast<double>(std::numeric_limits<int>::max()))) return false;
    if (type_ == CVarType::Float
        && (value < -static_cast<double>(std::numeric_limits<float>::max())
            || value > static_cast<double>(std::numeric_limits<float>::max()))) return false;
    normalized = value;
    return true;
}

void ConsoleVariable::Set(double value) {
    double normalized = 0.0;
    if (!NormalizeNumber(value, minimum_, maximum_, normalized)) return;
    value = normalized;
    if (value == number_) return;
    number_ = value;
    Notify();
}

void ConsoleVariable::Set(std::string value) {
    if (type_ != CVarType::String) {
        std::string error;
        SetFromString(value, error);
        return;
    }
    if (value == text_) return;
    text_ = std::move(value);
    Notify();
}

void ConsoleVariable::Reset() {
    if (type_ == CVarType::String) Set(defaultText_);
    else Set(defaultNumber_);
}

void ConsoleVariable::SetRange(double minimum, double maximum) {
    if (!std::isfinite(minimum) || !std::isfinite(maximum) || !(minimum <= maximum)) return;
    double normalized = 0.0;
    if (type_ != CVarType::String && !NormalizeNumber(number_, minimum, maximum, normalized)) return;
    minimum_ = minimum;
    maximum_ = maximum;
    if (type_ != CVarType::String && normalized != number_) {
        number_ = normalized;
        Notify();
    }
}

void ConsoleVariable::Notify() {
    // Copy so a callback may register further callbacks safely.
    const auto callbacks = callbacks_;
    for (const auto& callback : callbacks) callback(*this);
}

// ------------------------------------------------------------------ ConsoleRegistry

ConsoleRegistry::ConsoleRegistry() = default;

ConsoleRegistry& ConsoleRegistry::Global() {
    static ConsoleRegistry registry;
    return registry;
}

ConsoleVariable* ConsoleRegistry::Register(std::string name, CVarType type, double number, std::string text,
    std::string help, std::uint32_t flags) {
    if (ConsoleVariable* existing = Find(name)) return existing->type_ == type ? existing : nullptr;
    if (type == CVarType::Float && (!std::isfinite(number)
        || number < -static_cast<double>(std::numeric_limits<float>::max())
        || number > static_cast<double>(std::numeric_limits<float>::max()))) return nullptr;
    auto variable = std::make_unique<ConsoleVariable>();
    variable->name_ = name;
    variable->help_ = std::move(help);
    variable->type_ = type;
    variable->flags_ = flags;
    variable->number_ = variable->defaultNumber_ = number;
    variable->text_ = variable->defaultText_ = std::move(text);
    ConsoleVariable* raw = variable.get();
    variables_.emplace(name, std::move(variable));
    // Apply a config value that arrived before registration.
    const auto pending = pending_.find(name);
    if (pending != pending_.end()) {
        std::string error;
        raw->SetFromString(pending->second, error);
        pending_.erase(pending);
    }
    return raw;
}

ConsoleVariable* ConsoleRegistry::RegisterBool(std::string name, bool value, std::string help, std::uint32_t flags) {
    return Register(std::move(name), CVarType::Bool, value ? 1.0 : 0.0, {}, std::move(help), flags);
}

ConsoleVariable* ConsoleRegistry::RegisterInt(std::string name, int value, std::string help, std::uint32_t flags) {
    return Register(std::move(name), CVarType::Int, value, {}, std::move(help), flags);
}

ConsoleVariable* ConsoleRegistry::RegisterFloat(std::string name, float value, std::string help, std::uint32_t flags) {
    return Register(std::move(name), CVarType::Float, static_cast<double>(value), {}, std::move(help), flags);
}

ConsoleVariable* ConsoleRegistry::RegisterString(std::string name, std::string value, std::string help,
    std::uint32_t flags) {
    return Register(std::move(name), CVarType::String, 0.0, std::move(value), std::move(help), flags);
}

void ConsoleRegistry::RegisterCommand(std::string name, std::string help, ConsoleCommand command) {
    commands_[std::move(name)] = {std::move(help), std::move(command)};
}

ConsoleVariable* ConsoleRegistry::Find(std::string_view name) {
    return const_cast<ConsoleVariable*>(static_cast<const ConsoleRegistry&>(*this).Find(name));
}

const ConsoleVariable* ConsoleRegistry::Find(std::string_view name) const {
    const auto exact = variables_.find(name);
    if (exact != variables_.end()) return exact->second.get();
    // Console names are case-insensitive, like Unreal's.
    const std::string lower = Lower(name);
    for (const auto& [key, variable] : variables_) {
        if (Lower(key) == lower) return variable.get();
    }
    return nullptr;
}

bool ConsoleRegistry::HasCommand(std::string_view name) const {
    if (commands_.count(name)) return true;
    const std::string lower = Lower(name);
    for (const auto& entry : commands_) {
        if (Lower(entry.first) == lower) return true;
    }
    return false;
}

bool ConsoleRegistry::Set(std::string_view name, std::string_view value, std::string& error) {
    ConsoleVariable* variable = Find(name);
    if (!variable) {
        error = "unknown variable " + std::string(name);
        return false;
    }
    if (variable->flags_ & CVarReadOnly) {
        error = variable->name_ + " is read-only";
        return false;
    }
    if ((variable->flags_ & CVarCheat) && !cheats_) {
        error = variable->name_ + " requires cheats";
        return false;
    }
    return variable->SetFromString(value, error);
}

bool ConsoleRegistry::PrepareBatch(const std::vector<std::pair<std::string, std::string>>& values,
    std::vector<PreparedChange>& prepared, std::string& error) const {
    for (const auto& [name, value] : values) {
        const ConsoleVariable* variable = Find(name);
        if (!variable) {
            error = name + ": unknown variable " + name;
            return false;
        }
        if ((variable->flags_ & CVarReadOnly) || ((variable->flags_ & CVarCheat) && !cheats_)) {
            error = name + ": " + variable->name_ + ((variable->flags_ & CVarReadOnly) ? " is read-only" : " requires cheats");
            return false;
        }
        auto change = std::find_if(prepared.begin(), prepared.end(),
            [&](const PreparedChange& entry) { return entry.first == variable; });
        if (change == prepared.end()) {
            ConsoleVariable probe = *variable;
            probe.callbacks_.clear(); // copied callbacks can still capture live host services
            prepared.emplace_back(variable, std::move(probe));
            change = prepared.end() - 1;
        }
        std::string fieldError;
        if (!change->second.SetFromString(value, fieldError)) {
            error = name + ": " + fieldError;
            return false;
        }
    }
    error.clear();
    return true;
}

bool ConsoleRegistry::ValidateBatch(const std::vector<std::pair<std::string, std::string>>& values,
    std::string& error) const {
    std::vector<PreparedChange> prepared;
    return PrepareBatch(values, prepared, error);
}

bool ConsoleRegistry::SetBatch(const std::vector<std::pair<std::string, std::string>>& values, std::string& error) {
    std::vector<PreparedChange> prepared;
    if (!PrepareBatch(values, prepared, error)) return false;
    std::vector<ConsoleVariable*> changed;
    changed.reserve(prepared.size());
    // No validation or allocating value copies follow the first live write.
    // Notification is a second phase so callbacks see all committed settings.
    for (auto& entry : prepared) {
        auto* variable = const_cast<ConsoleVariable*>(entry.first);
        ConsoleVariable& value = entry.second;
        if (variable->type_ == CVarType::String) {
            if (variable->text_ == value.text_) continue;
            variable->text_.swap(value.text_);
        } else {
            if (variable->number_ == value.number_) continue;
            variable->number_ = value.number_;
        }
        changed.push_back(variable);
    }
    for (ConsoleVariable* variable : changed) variable->Notify();
    return true;
}

std::string ConsoleRegistry::Execute(std::string_view line) {
    const std::vector<std::string> tokens = TokenizeCommandLine(line);
    if (tokens.empty()) return {};
    history_.emplace_back(Trim(line));
    if (history_.size() > 64) history_.erase(history_.begin());

    const std::string& head = tokens[0];
    const std::string lowerHead = Lower(head);
    if (lowerHead == "help") {
        if (tokens.size() > 1) {
            if (const ConsoleVariable* variable = Find(tokens[1])) {
                return variable->name_ + " (" + TypeName(variable->type_) + ", default " + variable->DefaultString()
                    + "): " + variable->help_;
            }
            for (const auto& [name, entry] : commands_) {
                if (Lower(name) == Lower(tokens[1])) return name + ": " + entry.help;
            }
            return "No help for '" + tokens[1] + "'";
        }
        return std::to_string(variables_.size()) + " variables, " + std::to_string(commands_.size())
            + " commands. 'list [prefix]' shows them; 'help <name>' explains one; 'reset <name>' restores a default.";
    }
    if (lowerHead == "list") {
        const std::string prefix = tokens.size() > 1 ? Lower(tokens[1]) : std::string();
        std::string out;
        for (const auto& [name, variable] : variables_) {
            if (Lower(name).compare(0, prefix.size(), prefix) == 0) out += name + " = " + variable->GetString() + "\n";
        }
        for (const auto& entry : commands_) {
            if (Lower(entry.first).compare(0, prefix.size(), prefix) == 0) out += entry.first + " (command)\n";
        }
        if (!out.empty()) out.pop_back();
        return out;
    }
    if (lowerHead == "reset" && tokens.size() > 1) {
        ConsoleVariable* variable = Find(tokens[1]);
        if (!variable) return "Unknown variable '" + tokens[1] + "'";
        if (variable->flags_ & CVarReadOnly) return variable->name_ + " is read-only";
        variable->Reset();
        return variable->name_ + " = " + variable->GetString();
    }
    for (auto& [name, entry] : commands_) {
        if (Lower(name) == lowerHead) {
            return entry.command(std::vector<std::string>(tokens.begin() + 1, tokens.end()));
        }
    }
    if (ConsoleVariable* variable = Find(head)) {
        if (tokens.size() == 1) return variable->name_ + " = " + variable->GetString();
        std::string value = tokens[1];
        if (variable->type_ == CVarType::String) {
            for (std::size_t i = 2; i < tokens.size(); ++i) value += " " + tokens[i];
        }
        std::string error;
        if (!Set(variable->name_, value, error)) return "Error: " + error;
        return variable->name_ + " = " + variable->GetString();
    }
    return "Unknown command '" + head + "'";
}

std::vector<std::string> ConsoleRegistry::Complete(std::string_view prefix) const {
    const std::string lower = Lower(prefix);
    std::vector<std::string> matches;
    for (const auto& entry : variables_) {
        if (Lower(entry.first).compare(0, lower.size(), lower) == 0) matches.push_back(entry.first);
    }
    for (const auto& entry : commands_) {
        if (Lower(entry.first).compare(0, lower.size(), lower) == 0) matches.push_back(entry.first);
    }
    for (const char* builtin : {"help", "list", "reset"}) {
        if (std::string_view(builtin).compare(0, lower.size(), lower) == 0) matches.emplace_back(builtin);
    }
    std::sort(matches.begin(), matches.end());
    matches.erase(std::unique(matches.begin(), matches.end()), matches.end());
    return matches;
}

bool ConsoleRegistry::LoadConfig(std::string_view text, std::string& error) {
    struct Entry {
        std::string name, value;
    };
    std::vector<Entry> entries;
    std::string section;
    std::size_t lineNumber = 0;
    while (!text.empty()) {
        const std::size_t newline = text.find('\n');
        std::string_view line = text.substr(0, newline);
        text = newline == std::string_view::npos ? std::string_view() : text.substr(newline + 1);
        ++lineNumber;
        line = Trim(line);
        if (line.empty() || line.front() == '#' || line.front() == ';') continue;
        if (line.front() == '[') {
            if (line.back() != ']') {
                error = "line " + std::to_string(lineNumber) + ": malformed section";
                return false;
            }
            section = std::string(Trim(line.substr(1, line.size() - 2)));
            continue;
        }
        const std::size_t equals = line.find('=');
        if (equals == std::string_view::npos || Trim(line.substr(0, equals)).empty()) {
            error = "line " + std::to_string(lineNumber) + ": expected name = value";
            return false;
        }
        std::string name(Trim(line.substr(0, equals)));
        if (!section.empty()) name = section + "." + name;
        std::string_view value = Trim(line.substr(equals + 1));
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') value = value.substr(1, value.size() - 2);
        entries.push_back({std::move(name), std::string(value)});
    }
    // Validate every known value before changing anything (all-or-nothing).
    for (const Entry& entry : entries) {
        if (const ConsoleVariable* variable = Find(entry.name)) {
            ConsoleVariable probe = *variable;
            probe.callbacks_.clear();
            if (!probe.SetFromString(entry.value, error)) return false;
        }
    }
    for (const Entry& entry : entries) {
        if (ConsoleVariable* variable = Find(entry.name)) {
            variable->SetFromString(entry.value, error);
        } else {
            pending_[entry.name] = entry.value;
        }
    }
    error.clear();
    return true;
}

std::string ConsoleRegistry::SaveConfig() const {
    std::string out;
    for (const auto& [name, variable] : variables_) {
        if (!(variable->flags_ & CVarArchive)) continue;
        if (variable->GetString() == variable->DefaultString()) continue;
        const std::string value = variable->GetString();
        const bool quote = variable->type_ == CVarType::String;
        out += name + " = " + (quote ? "\"" + value + "\"" : value) + "\n";
    }
    return out;
}

} // namespace Astral::Core
