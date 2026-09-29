#include "Engine/UI/ConsoleOverlay.h"

#include "Engine/Input/InputSystem.h"

#include <algorithm>
#include <cctype>
#include <cstddef>

namespace Astral::UI {

namespace Keys = Input::Keys;

ConsoleOverlay::ConsoleOverlay(UIRoot& ui, Core::ConsoleRegistry& console, int visibleLines) : ui_(ui), console_(console) {
    visibleLines = std::max(1, visibleLines);
    Panel& panel = ui.Root().Add<Panel>("console");
    panel.useTheme = true;
    panel.border = true;
    panel.cornerRadius = 0.0f;
    panel.padding = Thickness::All(8.0f);
    // Full width, anchored to the top; height fits the log and the input line.
    panel.anchorMin = {0.0f, 0.0f};
    panel.anchorMax = {1.0f, 0.0f};
    panel.visible = false;
    panel.keyHandler = [this](std::uint16_t key, bool) { return HandleKey(key); };
    panel_ = &panel;
    StackPanel& stack = panel.Add<StackPanel>("console.stack");
    stack.spacing = 2.0f;
    for (int i = 0; i < visibleLines; ++i) {
        Label& line = stack.Add<Label>("", "console.line" + std::to_string(i));
        line.scale = 1;
        lineLabels_.push_back(&line);
    }
    TextBox& input = stack.Add<TextBox>("console.input");
    input.placeholder = "command or cvar (Tab completes)";
    input.Submitted.Add([this](const std::string& line) { Submit(line); });
    input_ = &input;
    const float lineHeight = static_cast<float>(Graphics::Canvas::LineHeight(1));
    panel.offsets = {0.0f, 0.0f, 0.0f,
        static_cast<float>(visibleLines) * (lineHeight + 2.0f) + static_cast<float>(Graphics::Canvas::LineHeight(2)) + 30.0f};
}

void ConsoleOverlay::Open() {
    panel_->visible = true;
    historyIndex_ = -1;
    Refresh();
    // Focus lands once the panel is laid out; SetFocus checks visibility only.
    ui_.SetFocus(input_);
}

void ConsoleOverlay::Close() {
    panel_->visible = false;
    if (ui_.Focused() == input_) ui_.SetFocus(nullptr);
}

void ConsoleOverlay::PreprocessInput(UIInput& input) {
    const auto toggle = std::find(input.keysPressed.begin(), input.keysPressed.end(), toggleKey);
    if (toggle == input.keysPressed.end()) return;
    input.keysPressed.erase(toggle);
    input.text.erase(std::remove_if(input.text.begin(), input.text.end(), [](char c) { return c == '`' || c == '~'; }),
        input.text.end());
    Toggle();
}

void ConsoleOverlay::Print(const std::string& text) {
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t end = text.find('\n', start);
        lines_.push_back(text.substr(start, end == std::string::npos ? std::string::npos : end - start));
        if (end == std::string::npos) break;
        start = end + 1;
    }
    if (lines_.size() > maxLines) lines_.erase(lines_.begin(), lines_.begin() + static_cast<std::ptrdiff_t>(lines_.size() - maxLines));
    Refresh();
}

void ConsoleOverlay::Submit(const std::string& line) {
    const std::size_t first = line.find_first_not_of(" \t");
    input_->SetText("");
    historyIndex_ = -1;
    draft_.clear();
    if (first == std::string::npos) return;
    const std::string command = line.substr(first);
    Print("> " + command);
    const std::string output = console_.Execute(command);
    if (!output.empty()) Print(output);
}

bool ConsoleOverlay::HandleKey(std::uint16_t key) {
    if (!IsOpen()) return false;
    const std::vector<std::string>& history = console_.History();
    if (key == Keys::Escape) {
        Close();
        return true;
    }
    if (key == Keys::Tab) {
        Complete();
        return true;
    }
    if (key == Keys::Up && !history.empty()) {
        if (historyIndex_ < 0) {
            draft_ = input_->text;
            historyIndex_ = static_cast<int>(history.size()) - 1;
        } else if (historyIndex_ > 0) {
            --historyIndex_;
        }
        input_->SetText(history[static_cast<std::size_t>(historyIndex_)]);
        return true;
    }
    if (key == Keys::Down) {
        if (historyIndex_ >= 0) {
            ++historyIndex_;
            if (historyIndex_ >= static_cast<int>(history.size())) {
                historyIndex_ = -1;
                input_->SetText(draft_);
            } else {
                input_->SetText(history[static_cast<std::size_t>(historyIndex_)]);
            }
        }
        return true;
    }
    return false;
}

void ConsoleOverlay::Complete() {
    const std::string& text = input_->text;
    if (text.find(' ') != std::string::npos) return; // completes the command name only
    const std::vector<std::string> matches = console_.Complete(text);
    if (matches.empty()) return;
    if (matches.size() == 1) {
        input_->SetText(matches.front() + " ");
        return;
    }
    // Several: list them and extend to the longest common prefix.
    std::string common = matches.front();
    for (const std::string& match : matches) {
        std::size_t n = 0;
        while (n < common.size() && n < match.size()
            && std::tolower(static_cast<unsigned char>(common[n])) == std::tolower(static_cast<unsigned char>(match[n]))) {
            ++n;
        }
        common.resize(n);
    }
    std::string listing;
    for (const std::string& match : matches) listing += (listing.empty() ? "" : "  ") + match;
    Print(listing);
    if (common.size() > text.size()) input_->SetText(common);
}

void ConsoleOverlay::Refresh() {
    const std::size_t shown = lineLabels_.size();
    const std::size_t first = lines_.size() > shown ? lines_.size() - shown : 0;
    for (std::size_t i = 0; i < shown; ++i) {
        const std::size_t index = first + i;
        lineLabels_[i]->text = index < lines_.size() ? lines_[index] : std::string();
    }
}

} // namespace Astral::UI
