#pragma once

// In-game developer console (Unreal's ~ console, Unity's dev console) built
// from UI widgets over a Core::ConsoleRegistry: toggled with the backquote key,
// it executes commands and CVar assignments, keeps a scrollback log, walks the
// registry's history with Up/Down, completes names with Tab and closes with
// Escape. The toggle key never reaches the text box.

#include "Engine/Core/Console.h"
#include "Engine/UI/Widgets.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Astral::UI {

class ConsoleOverlay {
public:
    ConsoleOverlay(UIRoot& ui, Core::ConsoleRegistry& console, int visibleLines = 12);

    void Open();
    void Close();
    void Toggle() { IsOpen() ? Close() : Open(); }
    bool IsOpen() const { return panel_->visible; }

    // Call before UIRoot::Update: handles the toggle key and strips it (and
    // its typed character) from the input.
    void PreprocessInput(UIInput& input);

    // Appends text to the log (split into lines).
    void Print(const std::string& text);
    // Runs a line as if typed and submitted.
    void Submit(const std::string& line);
    const std::vector<std::string>& Lines() const { return lines_; }
    TextBox& InputBox() { return *input_; }

    std::uint16_t toggleKey{0xC0}; // VK_OEM_3, the `~ key
    std::size_t maxLines{256};

private:
    bool HandleKey(std::uint16_t key);
    void Complete();
    void Refresh();

    UIRoot& ui_;
    Core::ConsoleRegistry& console_;
    Panel* panel_{};
    TextBox* input_{};
    std::vector<Label*> lineLabels_;
    std::vector<std::string> lines_;
    int historyIndex_{-1}; // -1 = editing a new line
    std::string draft_;
};

} // namespace Astral::UI
