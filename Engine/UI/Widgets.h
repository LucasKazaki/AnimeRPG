#pragma once

// Retained-mode UI (the role of Unreal's UMG/Slate and Unity's UI Toolkit),
// drawn with the engine's own Canvas so HUDs look the same in the Win32 host,
// headless captures and tests.
//
//   Widget tree   - panels (Canvas with anchors, Stack, Panel/overlay) and
//                   controls (Label, Button, Toggle, Slider, ProgressBar,
//                   TextBox), each with margin, padding, alignment, fixed or
//                   measured size, visibility and enabled state.
//   Layout        - two passes: Measure (desired size, bottom-up) and Arrange
//                   (final rects, top-down), rerun every Update.
//   Input         - pointer hover/press/capture/click, keyboard focus with
//                   Tab/Shift+Tab and arrow-key spatial navigation, Enter/Space
//                   activation, text entry for the focused TextBox; unhandled
//                   keys bubble to parents.
//   Safety        - removals requested during callbacks (a button closing its
//                   own dialog) are deferred to the end of the update.

#include "Engine/Core/Delegate.h"
#include "Engine/Graphics/Canvas.h"
#include "Engine/Math/VectorMath.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Astral::UI {

using Math::Vec2;
using Graphics::Rgba8;

struct Rect {
    float x{}, y{}, width{}, height{};
    bool Contains(Vec2 p) const { return p.x >= x && p.y >= y && p.x < x + width && p.y < y + height; }
    Vec2 Center() const { return {x + width * 0.5f, y + height * 0.5f}; }
};

struct Thickness {
    float left{}, top{}, right{}, bottom{};
    static Thickness All(float v) { return {v, v, v, v}; }
    float Horizontal() const { return left + right; }
    float Vertical() const { return top + bottom; }
};

enum class HAlign : std::uint8_t { Left, Center, Right, Stretch };
enum class VAlign : std::uint8_t { Top, Center, Bottom, Stretch };
enum class Orientation : std::uint8_t { Vertical, Horizontal };

// One frame of UI input, fed by the host (Win32 messages, tests, replays).
struct UIInput {
    Vec2 pointer{};
    bool pointerDown{};
    std::vector<std::uint16_t> keysPressed; // virtual keys pressed this frame (Input::Keys)
    bool shift{};
    std::string text;                       // printable characters typed this frame
};

struct Theme {
    Rgba8 text{235, 232, 248, 255};
    Rgba8 textDisabled{140, 136, 160, 255};
    Rgba8 panel{24, 20, 44, 220};
    Rgba8 border{120, 110, 190, 255};
    Rgba8 button{58, 48, 110, 255};
    Rgba8 buttonHover{84, 70, 150, 255};
    Rgba8 buttonPressed{40, 32, 84, 255};
    Rgba8 accent{255, 170, 90, 255};
    Rgba8 focus{255, 220, 120, 255};
    Rgba8 track{30, 26, 54, 255};
    int textScale{2};
    float cornerRadius{6.0f};
};

class UIRoot;

class Widget {
public:
    explicit Widget(std::string name = {}) : name(std::move(name)) {}
    virtual ~Widget() = default;
    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

    // ---------------------------------------------------------------- tree
    template <typename T, typename... Args>
    T& Add(Args&&... args) {
        auto child = std::make_unique<T>(std::forward<Args>(args)...);
        T& reference = *child;
        child->parent_ = this;
        children_.push_back(std::move(child));
        return reference;
    }
    // Deferred to the end of the current UIRoot::Update (safe inside callbacks).
    void RemoveChild(Widget* child);
    void RemoveFromParent();
    void ClearChildren();
    Widget* Parent() const { return parent_; }
    const std::vector<std::unique_ptr<Widget>>& Children() const { return children_; }
    // Depth-first search by name (this widget included).
    Widget* Find(std::string_view widgetName);
    template <typename T>
    T* FindAs(std::string_view widgetName) { return dynamic_cast<T*>(Find(widgetName)); }
    bool IsPendingRemoval() const { return pendingRemoval_; }

    // ---------------------------------------------------------------- layout
    std::string name;
    Thickness margin;
    Thickness padding;
    HAlign hAlign{HAlign::Stretch};
    VAlign vAlign{VAlign::Stretch};
    Vec2 size{-1.0f, -1.0f}; // fixed size per axis; < 0 = measured
    bool visible{true};
    bool enabled{true};
    // Placement inside a CanvasPanel: anchors in [0, 1] of the parent. With a
    // point anchor (min == max) offsets are (x, y, width, height) around the
    // pivot; with stretched anchors they are (left, top, right, bottom) insets.
    Vec2 anchorMin{0.0f, 0.0f};
    Vec2 anchorMax{0.0f, 0.0f};
    Math::Vec4 offsets{0.0f, 0.0f, 0.0f, 0.0f};
    Vec2 pivot{0.0f, 0.0f};

    // Desired size including padding (not margin). Called by the parent.
    Vec2 Measure(Vec2 available);
    // Final rect (margin already removed by the parent's placement helper).
    void Arrange(Rect slot);
    const Rect& Bounds() const { return rect_; }
    Vec2 Desired() const { return desired_; }
    // Visible and enabled up the whole parent chain.
    bool EffectiveEnabled() const;
    bool EffectiveVisible() const;

    // ---------------------------------------------------------------- drawing
    void Draw(Graphics::Canvas& canvas, const Theme& theme) const;

    // ---------------------------------------------------------------- interaction
    virtual bool Focusable() const { return false; }
    // Receives pointer presses (the pressed widget captures the pointer).
    virtual bool HitTestable() const { return false; }
    bool Hovered() const { return hovered_; }
    bool Pressed() const { return pressed_; }
    bool Focused() const { return focused_; }
    // Optional key hook, consulted before OnKey (e.g. console history keys).
    std::function<bool(std::uint16_t key, bool shift)> keyHandler;

protected:
    virtual Vec2 MeasureContent(Vec2 available);         // content size, padding excluded
    virtual void ArrangeContent(Rect content);           // place children in the content rect
    virtual void DrawSelf(Graphics::Canvas&, const Theme&) const {}
    virtual void OnPointerDown(Vec2) {}
    virtual void OnPointerDrag(Vec2) {}
    virtual void OnPointerUp(Vec2, bool inside) { (void)inside; }
    virtual bool OnKey(std::uint16_t key, bool shift);
    virtual bool OnText(const std::string&) { return false; }
    virtual void OnFocusChanged(bool) {}
    // Places `child` in `slot` honouring its margin, alignment and desired size.
    static void Place(Widget& child, Rect slot);
    Rect ContentRect() const;
    // Text scale for measuring and drawing: explicit, else the root's theme.
    int TextScale(int requested = 0) const { return requested > 0 ? requested : (theme_ ? theme_->textScale : 2); }

private:
    friend class UIRoot;
    Widget* parent_{};
    std::vector<std::unique_ptr<Widget>> children_;
    Rect rect_;
    Vec2 desired_;
    const Theme* theme_{};
    bool hovered_{}, pressed_{}, focused_{};
    bool pendingRemoval_{};
};

// Children overlap, each aligned inside the content area; optional background.
class Panel : public Widget {
public:
    explicit Panel(std::string name = {}) : Widget(std::move(name)) {}
    Rgba8 background{0, 0, 0, 0}; // alpha 0 = none (theme panel when useTheme)
    bool useTheme{};
    bool border{};
    float cornerRadius{-1.0f}; // < 0 = theme
    bool HitTestable() const override { return useTheme || background.a > 0; } // blocks clicks through dialogs

protected:
    void DrawSelf(Graphics::Canvas& canvas, const Theme& theme) const override;
};

// Children positioned by anchors and offsets (UMG's Canvas Panel).
class CanvasPanel : public Widget {
public:
    explicit CanvasPanel(std::string name = {}) : Widget(std::move(name)) {}

protected:
    Vec2 MeasureContent(Vec2 available) override;
    void ArrangeContent(Rect content) override;
};

class StackPanel : public Widget {
public:
    explicit StackPanel(std::string name = {}, Orientation orientation = Orientation::Vertical)
        : Widget(std::move(name)), orientation(orientation) {}
    Orientation orientation{Orientation::Vertical};
    float spacing{4.0f};

protected:
    Vec2 MeasureContent(Vec2 available) override;
    void ArrangeContent(Rect content) override;
};

class Label : public Widget {
public:
    explicit Label(std::string text = {}, std::string name = {}) : Widget(std::move(name)), text(std::move(text)) {}
    std::string text;
    int scale{0};             // 0 = theme
    Rgba8 color{0, 0, 0, 0};  // alpha 0 = theme text colour
    HAlign textAlign{HAlign::Left};

protected:
    Vec2 MeasureContent(Vec2 available) override;
    void DrawSelf(Graphics::Canvas& canvas, const Theme& theme) const override;
};

class Button : public Widget {
public:
    explicit Button(std::string text = {}, std::string name = {});
    std::string text;
    Core::MulticastDelegate<> Clicked;
    bool Focusable() const override { return true; }
    bool HitTestable() const override { return true; }
    // Programmatic press (Enter/Space do this when focused).
    void Click();

protected:
    Vec2 MeasureContent(Vec2 available) override;
    void DrawSelf(Graphics::Canvas& canvas, const Theme& theme) const override;
    void OnPointerUp(Vec2, bool inside) override;
    bool OnKey(std::uint16_t key, bool shift) override;
};

class Toggle : public Widget {
public:
    explicit Toggle(std::string text = {}, bool checked = false, std::string name = {});
    std::string text;
    bool checked{};
    Core::MulticastDelegate<bool> Toggled;
    bool Focusable() const override { return true; }
    bool HitTestable() const override { return true; }
    void Set(bool value);

protected:
    Vec2 MeasureContent(Vec2 available) override;
    void DrawSelf(Graphics::Canvas& canvas, const Theme& theme) const override;
    void OnPointerUp(Vec2, bool inside) override;
    bool OnKey(std::uint16_t key, bool shift) override;
};

class Slider : public Widget {
public:
    explicit Slider(float minimum = 0.0f, float maximum = 1.0f, float value = 0.0f, std::string name = {});
    float minimum{0.0f}, maximum{1.0f};
    float step{0.0f}; // 0 = continuous; keyboard uses step or 5% of the range
    Core::MulticastDelegate<float> ValueChanged;
    float Value() const { return value_; }
    void SetValue(float value);
    bool Focusable() const override { return true; }
    bool HitTestable() const override { return true; }

protected:
    Vec2 MeasureContent(Vec2 available) override;
    void DrawSelf(Graphics::Canvas& canvas, const Theme& theme) const override;
    void OnPointerDown(Vec2 p) override { OnPointerDrag(p); }
    void OnPointerDrag(Vec2 p) override;
    bool OnKey(std::uint16_t key, bool shift) override;

private:
    float value_{};
};

class ProgressBar : public Widget {
public:
    explicit ProgressBar(float value = 0.0f, std::string name = {}) : Widget(std::move(name)), value(value) {}
    float value{};             // [0, 1]
    Rgba8 fill{0, 0, 0, 0};    // alpha 0 = theme accent

protected:
    Vec2 MeasureContent(Vec2 available) override;
    void DrawSelf(Graphics::Canvas& canvas, const Theme& theme) const override;
};

// Single-line text entry: printable ASCII, Backspace, Enter submits.
class TextBox : public Widget {
public:
    explicit TextBox(std::string name = {}) : Widget(std::move(name)) {}
    std::string text;
    std::string placeholder;
    std::size_t maxLength{256};
    Core::MulticastDelegate<const std::string&> Submitted;
    Core::MulticastDelegate<const std::string&> TextChanged;
    bool Focusable() const override { return true; }
    bool HitTestable() const override { return true; }
    void SetText(std::string value);

protected:
    Vec2 MeasureContent(Vec2 available) override;
    void DrawSelf(Graphics::Canvas& canvas, const Theme& theme) const override;
    bool OnKey(std::uint16_t key, bool shift) override;
    bool OnText(const std::string& typed) override;
};

// Owns the widget tree and routes input to it.
class UIRoot {
public:
    UIRoot();
    CanvasPanel& Root() { return *root_; }
    Theme theme;

    // Lays out for `viewport`, dispatches input, then applies deferred removals.
    void Update(const UIInput& input, Vec2 viewport);
    void Draw(Graphics::Canvas& canvas) const;

    Widget* Focused() const { return focused_; }
    Widget* Hovered() const { return hovered_; }
    void SetFocus(Widget* widget);
    // Moves focus to the next/previous focusable widget in tree order.
    void FocusNext(bool backwards = false);
    // True while typing into a TextBox: the game should ignore keyboard input.
    bool WantsKeyboard() const;
    // True when the pointer is over a hit-testable widget.
    bool PointerOverUI() const { return hovered_ != nullptr; }

private:
    void Layout(Vec2 viewport);
    void ShareTheme(Widget& widget) const;
    Widget* HitTest(Widget& widget, Vec2 point);
    void Collect(Widget& widget, std::vector<Widget*>& out) const;
    bool Contains(const Widget* widget) const;
    void Validate();
    bool Navigate(std::uint16_t key);
    void PurgeRemoved(Widget& widget);

    std::unique_ptr<CanvasPanel> root_;
    Widget* hovered_{};
    Widget* focused_{};
    Widget* captured_{};
    bool pointerWasDown_{};
};

} // namespace Astral::UI
