#pragma once

// Console variables and commands, after Unreal's IConsoleManager/CVars and
// Unity's debug console: typed, range-checked settings addressable by name
// ("r.Quality 2"), change callbacks, config files (Archive-flagged variables
// are saved), command registration, tab completion and a small line history.

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Astral::Core {

enum class CVarType : std::uint8_t { Bool, Int, Float, String };

enum CVarFlags : std::uint32_t {
    CVarNone = 0,
    CVarReadOnly = 1u << 0, // only code (SetDefault/ForceSet) may change it
    CVarCheat = 1u << 1,    // rejected unless cheats are enabled
    CVarArchive = 1u << 2,  // written by SaveConfig
};

class ConsoleVariable {
public:
    const std::string& Name() const { return name_; }
    const std::string& Help() const { return help_; }
    CVarType Type() const { return type_; }
    std::uint32_t Flags() const { return flags_; }

    bool GetBool() const { return number_ != 0.0; }
    int GetInt() const { return static_cast<int>(number_); }
    float GetFloat() const { return static_cast<float>(number_); }
    std::string GetString() const; // any type, formatted
    std::string DefaultString() const;

    // Parses by type and applies the range; returns false (unchanged) on bad input.
    bool SetFromString(std::string_view text, std::string& error);
    void Set(double value);
    void Set(std::string value);
    void Reset();
    void SetRange(double minimum, double maximum);
    // Called after every successful change with the variable itself.
    void OnChanged(std::function<void(const ConsoleVariable&)> callback) { callbacks_.push_back(std::move(callback)); }

private:
    friend class ConsoleRegistry;
    void Notify();
    std::string name_, help_;
    CVarType type_{CVarType::Int};
    std::uint32_t flags_{};
    double number_{}, defaultNumber_{};
    std::string text_, defaultText_;
    double minimum_{-1.0e300}, maximum_{1.0e300};
    std::vector<std::function<void(const ConsoleVariable&)>> callbacks_;
};

using ConsoleCommand = std::function<std::string(const std::vector<std::string>& arguments)>;

class ConsoleRegistry {
public:
    ConsoleRegistry();
    // Process-wide registry used by AutoConsoleVariable and the in-game console.
    static ConsoleRegistry& Global();

    // Registering an existing name returns the existing variable (types must match).
    ConsoleVariable* RegisterBool(std::string name, bool value, std::string help, std::uint32_t flags = CVarNone);
    ConsoleVariable* RegisterInt(std::string name, int value, std::string help, std::uint32_t flags = CVarNone);
    ConsoleVariable* RegisterFloat(std::string name, float value, std::string help, std::uint32_t flags = CVarNone);
    ConsoleVariable* RegisterString(std::string name, std::string value, std::string help, std::uint32_t flags = CVarNone);
    void RegisterCommand(std::string name, std::string help, ConsoleCommand command);

    ConsoleVariable* Find(std::string_view name);
    const ConsoleVariable* Find(std::string_view name) const;
    bool HasCommand(std::string_view name) const;
    bool Set(std::string_view name, std::string_view value, std::string& error);

    // Runs one console line: "name" prints a variable, "name value" sets it,
    // "command args..." runs a command. Built-ins: help, list [prefix], reset <name>.
    // Arguments may be double-quoted. Returns the text to show.
    std::string Execute(std::string_view line);
    std::vector<std::string> Complete(std::string_view prefix) const;
    const std::vector<std::string>& History() const { return history_; }

    // "name = value" lines; '#' and ';' comments; [Section] headers prefix names
    // ("[r]" + "Quality = 2" -> "r.Quality"). Unknown names are kept and applied
    // when the variable registers later (like Unreal's deferred ini values).
    bool LoadConfig(std::string_view text, std::string& error);
    std::string SaveConfig() const; // Archive variables that differ from default

    void EnableCheats(bool enabled) { cheats_ = enabled; }

private:
    ConsoleVariable* Register(std::string name, CVarType type, double number, std::string text, std::string help,
        std::uint32_t flags);
    std::map<std::string, std::unique_ptr<ConsoleVariable>, std::less<>> variables_;
    struct CommandEntry {
        std::string help;
        ConsoleCommand command;
    };
    std::map<std::string, CommandEntry, std::less<>> commands_;
    std::map<std::string, std::string, std::less<>> pending_; // config values for not-yet-registered names
    std::vector<std::string> history_;
    bool cheats_{};
};

// Static-registration helper: `static AutoConsoleVariable<int> q("r.Quality", 2, "...");`
template <typename T>
class AutoConsoleVariable;

template <>
class AutoConsoleVariable<bool> {
public:
    AutoConsoleVariable(std::string name, bool value, std::string help, std::uint32_t flags = CVarNone)
        : variable_(ConsoleRegistry::Global().RegisterBool(std::move(name), value, std::move(help), flags)) {}
    bool Get() const { return variable_->GetBool(); }
    ConsoleVariable* operator->() const { return variable_; }

private:
    ConsoleVariable* variable_;
};

template <>
class AutoConsoleVariable<int> {
public:
    AutoConsoleVariable(std::string name, int value, std::string help, std::uint32_t flags = CVarNone)
        : variable_(ConsoleRegistry::Global().RegisterInt(std::move(name), value, std::move(help), flags)) {}
    int Get() const { return variable_->GetInt(); }
    ConsoleVariable* operator->() const { return variable_; }

private:
    ConsoleVariable* variable_;
};

template <>
class AutoConsoleVariable<float> {
public:
    AutoConsoleVariable(std::string name, float value, std::string help, std::uint32_t flags = CVarNone)
        : variable_(ConsoleRegistry::Global().RegisterFloat(std::move(name), value, std::move(help), flags)) {}
    float Get() const { return variable_->GetFloat(); }
    ConsoleVariable* operator->() const { return variable_; }

private:
    ConsoleVariable* variable_;
};

// Splits a console line into arguments, honouring double quotes.
std::vector<std::string> TokenizeCommandLine(std::string_view line);

} // namespace Astral::Core
