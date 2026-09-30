#include "Engine/UI/Widgets.h"

#include "Engine/Input/InputSystem.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Astral::UI {

namespace Keys = Input::Keys;

namespace {

int Px(float value) { return static_cast<int>(std::lround(value)); }

bool IsActivateKey(std::uint16_t key) { return key == Keys::Enter || key == Keys::Space; }

int LineCount(const std::string& text) { return 1 + static_cast<int>(std::count(text.begin(), text.end(), '\n')); }

} // namespace

// ------------------------------------------------------------------ widget

void Widget::RemoveChild(Widget* child) {
    for (const auto& owned : children_) {
        if (owned.get() == child) owned->pendingRemoval_ = true;
    }
}

void Widget::RemoveFromParent() {
    if (parent_) parent_->RemoveChild(this);
}

void Widget::ClearChildren() {
    for (const auto& child : children_) child->pendingRemoval_ = true;
}

Widget* Widget::Find(std::string_view widgetName) {
    if (pendingRemoval_) return nullptr;
    if (name == widgetName) return this;
    for (const auto& child : children_) {
        if (Widget* found = child->Find(widgetName)) return found;
    }
    return nullptr;
}

bool Widget::EffectiveEnabled() const {
    for (const Widget* w = this; w; w = w->parent_) {
        if (!w->enabled || w->pendingRemoval_) return false;
    }
    return true;
}

bool Widget::EffectiveVisible() const {
    for (const Widget* w = this; w; w = w->parent_) {
        if (!w->visible || w->pendingRemoval_) return false;
    }
    return true;
}

Vec2 Widget::Measure(Vec2 available) {
    if (!visible || pendingRemoval_) {
        desired_ = {};
        return desired_;
    }
    Vec2 inner{std::max(0.0f, available.x - padding.Horizontal()), std::max(0.0f, available.y - padding.Vertical())};
    if (size.x >= 0.0f) inner.x = std::max(0.0f, size.x - padding.Horizontal());
    if (size.y >= 0.0f) inner.y = std::max(0.0f, size.y - padding.Vertical());
    const Vec2 content = MeasureContent(inner);
    Vec2 result{content.x + padding.Horizontal(), content.y + padding.Vertical()};
    if (size.x >= 0.0f) result.x = size.x;
    if (size.y >= 0.0f) result.y = size.y;
    desired_ = result;
    return result;
}

Vec2 Widget::MeasureContent(Vec2 available) {
    Vec2 result{};
    for (const auto& child : children_) {
        const Vec2 d = child->Measure({available.x - child->margin.Horizontal(), available.y - child->margin.Vertical()});
        if (!child->visible || child->pendingRemoval_) continue;
        result.x = std::max(result.x, d.x + child->margin.Horizontal());
        result.y = std::max(result.y, d.y + child->margin.Vertical());
    }
    return result;
}

Rect Widget::ContentRect() const {
    return {rect_.x + padding.left, rect_.y + padding.top, std::max(0.0f, rect_.width - padding.Horizontal()),
        std::max(0.0f, rect_.height - padding.Vertical())};
}

void Widget::Arrange(Rect slot) {
    rect_ = slot;
    ArrangeContent(ContentRect());
}

void Widget::ArrangeContent(Rect content) {
    for (const auto& child : children_) {
        if (child->visible && !child->pendingRemoval_) Place(*child, content);
    }
}

void Widget::Place(Widget& child, Rect slot) {
    const Rect inner{slot.x + child.margin.left, slot.y + child.margin.top, std::max(0.0f, slot.width - child.margin.Horizontal()),
        std::max(0.0f, slot.height - child.margin.Vertical())};
    const Vec2 desired = child.desired_;
    auto axis = [](float start, float extent, float wanted, bool stretch, int align, float& position, float& length) {
        length = stretch ? extent : std::min(wanted, extent);
        const float free = extent - length;
        position = start + (align == 0 ? 0.0f : (align == 2 ? free : free * 0.5f));
    };
    float x, y, w, h;
    const bool stretchX = child.hAlign == HAlign::Stretch && child.size.x < 0.0f;
    const bool stretchY = child.vAlign == VAlign::Stretch && child.size.y < 0.0f;
    const int alignX = child.hAlign == HAlign::Left ? 0 : (child.hAlign == HAlign::Right ? 2 : 1);
    const int alignY = child.vAlign == VAlign::Top ? 0 : (child.vAlign == VAlign::Bottom ? 2 : 1);
    axis(inner.x, inner.width, desired.x, stretchX, alignX, x, w);
    axis(inner.y, inner.height, desired.y, stretchY, alignY, y, h);
    child.Arrange({x, y, w, h});
}

void Widget::Draw(Graphics::Canvas& canvas, const Theme& theme) const {
    if (!visible || pendingRemoval_) return;
    DrawSelf(canvas, theme);
    for (const auto& child : children_) child->Draw(canvas, theme);
}

bool Widget::OnKey(std::uint16_t, bool) { return false; }

// ------------------------------------------------------------------ panels

void Panel::DrawSelf(Graphics::Canvas& canvas, const Theme& theme) const {
    const Rgba8 color = background.a > 0 ? background : theme.panel;
    const Rect& r = Bounds();
    if (useTheme || background.a > 0) {
        canvas.FillRoundedRect(Px(r.x), Px(r.y), Px(r.width), Px(r.height), Px(cornerRadius >= 0.0f ? cornerRadius : theme.cornerRadius), color);
    }
    if (border) canvas.StrokeRect(Px(r.x), Px(r.y), Px(r.width), Px(r.height), theme.border);
}

Vec2 CanvasPanel::MeasureContent(Vec2 available) {
    for (const auto& child : Children()) child->Measure(available);
    return available;
}

void CanvasPanel::ArrangeContent(Rect content) {
    for (const auto& owned : Children()) {
        Widget& child = *owned;
        if (!child.visible || child.IsPendingRemoval()) continue;
        const Vec2 desired = child.Desired();
        auto axis = [](float start, float extent, float anchorMin, float anchorMax, float offset, float farOffset, float wanted,
                        float pivot, float& position, float& length) {
            if (anchorMin == anchorMax) {
                length = farOffset > 0.0f ? farOffset : wanted;
                position = start + extent * anchorMin + offset - pivot * length;
            } else {
                position = start + extent * anchorMin + offset;
                length = std::max(0.0f, start + extent * anchorMax - farOffset - position);
            }
        };
        float x, y, w, h;
        axis(content.x, content.width, child.anchorMin.x, child.anchorMax.x, child.offsets.x, child.offsets.z, desired.x,
            child.pivot.x, x, w);
        axis(content.y, content.height, child.anchorMin.y, child.anchorMax.y, child.offsets.y, child.offsets.w, desired.y,
            child.pivot.y, y, h);
        child.Arrange({x, y, w, h});
    }
}

Vec2 StackPanel::MeasureContent(Vec2 available) {
    Vec2 result{};
    int count = 0;
    for (const auto& child : Children()) {
        const Vec2 d = child->Measure({available.x - child->margin.Horizontal(), available.y - child->margin.Vertical()});
        if (!child->visible || child->IsPendingRemoval()) continue;
        const Vec2 outer{d.x + child->margin.Horizontal(), d.y + child->margin.Vertical()};
        if (orientation == Orientation::Vertical) {
            result.x = std::max(result.x, outer.x);
            result.y += outer.y;
        } else {
            result.y = std::max(result.y, outer.y);
            result.x += outer.x;
        }
        ++count;
    }
    const float gaps = spacing * static_cast<float>(std::max(0, count - 1));
    (orientation == Orientation::Vertical ? result.y : result.x) += gaps;
    return result;
}

void StackPanel::ArrangeContent(Rect content) {
    float cursor = orientation == Orientation::Vertical ? content.y : content.x;
    for (const auto& child : Children()) {
        if (!child->visible || child->IsPendingRemoval()) continue;
        const Vec2 d = child->Desired();
        if (orientation == Orientation::Vertical) {
            const float extent = d.y + child->margin.Vertical();
            Place(*child, {content.x, cursor, content.width, extent});
            cursor += extent + spacing;
        } else {
            const float extent = d.x + child->margin.Horizontal();
            Place(*child, {cursor, content.y, extent, content.height});
            cursor += extent + spacing;
        }
    }
}

// ------------------------------------------------------------------ label

Vec2 Label::MeasureContent(Vec2) {
    const int s = TextScale(scale);
    return {static_cast<float>(Graphics::Canvas::MeasureText(text, s)),
        static_cast<float>(Graphics::Canvas::LineHeight(s) * LineCount(text))};
}

void Label::DrawSelf(Graphics::Canvas& canvas, const Theme& theme) const {
    const Rect content = ContentRect();
    Graphics::TextStyle style;
    style.scale = TextScale(scale);
    style.color = color.a > 0 ? color : (EffectiveEnabled() ? theme.text : theme.textDisabled);
    const float width = static_cast<float>(Graphics::Canvas::MeasureText(text, style.scale));
    const float height = static_cast<float>(Graphics::Canvas::LineHeight(style.scale) * LineCount(text));
    float x = content.x;
    if (textAlign == HAlign::Center) x += (content.width - width) * 0.5f;
    if (textAlign == HAlign::Right) x += content.width - width;
    canvas.Text(Px(x), Px(content.y + (content.height - height) * 0.5f), text, style);
}

// ------------------------------------------------------------------ button

Button::Button(std::string text, std::string name) : Widget(std::move(name)), text(std::move(text)) {
    padding = {12.0f, 6.0f, 12.0f, 6.0f};
}

void Button::Click() {
    if (EffectiveEnabled()) Clicked.Broadcast();
}

Vec2 Button::MeasureContent(Vec2) {
    return {static_cast<float>(Graphics::Canvas::MeasureText(text, TextScale())),
        static_cast<float>(Graphics::Canvas::LineHeight(TextScale()))};
}

void Button::DrawSelf(Graphics::Canvas& canvas, const Theme& theme) const {
    const Rect& r = Bounds();
    const bool active = EffectiveEnabled();
    Rgba8 fill = theme.button;
    if (!active) fill = theme.track;
    else if (Pressed()) fill = theme.buttonPressed;
    else if (Hovered()) fill = theme.buttonHover;
    canvas.FillRoundedRect(Px(r.x), Px(r.y), Px(r.width), Px(r.height), Px(theme.cornerRadius), fill);
    if (Focused()) canvas.StrokeRect(Px(r.x) - 1, Px(r.y) - 1, Px(r.width) + 2, Px(r.height) + 2, theme.focus, 2);
    Graphics::TextStyle style;
    style.scale = theme.textScale;
    style.color = active ? theme.text : theme.textDisabled;
    const float width = static_cast<float>(Graphics::Canvas::MeasureText(text, style.scale));
    const float height = static_cast<float>(Graphics::Canvas::LineHeight(style.scale));
    canvas.Text(Px(r.x + (r.width - width) * 0.5f), Px(r.y + (r.height - height) * 0.5f), text, style);
}

void Button::OnPointerUp(Vec2, bool inside) {
    if (inside) Click();
}

bool Button::OnKey(std::uint16_t key, bool) {
    if (!IsActivateKey(key)) return false;
    Click();
    return true;
}

// ------------------------------------------------------------------ toggle

Toggle::Toggle(std::string text, bool checked, std::string name) : Widget(std::move(name)), text(std::move(text)), checked(checked) {
    padding = {2.0f, 2.0f, 2.0f, 2.0f};
}

void Toggle::Set(bool value) {
    if (value == checked) return;
    checked = value;
    Toggled.Broadcast(checked);
}

Vec2 Toggle::MeasureContent(Vec2) {
    const float line = static_cast<float>(Graphics::Canvas::LineHeight(TextScale()));
    return {line + 8.0f + static_cast<float>(Graphics::Canvas::MeasureText(text, TextScale())), line};
}

void Toggle::DrawSelf(Graphics::Canvas& canvas, const Theme& theme) const {
    const Rect content = ContentRect();
    const int box = Graphics::Canvas::LineHeight(theme.textScale);
    const bool active = EffectiveEnabled();
    canvas.FillRoundedRect(Px(content.x), Px(content.y), box, box, 3, Hovered() && active ? theme.buttonHover : theme.button);
    if (checked) canvas.FillRoundedRect(Px(content.x) + 4, Px(content.y) + 4, box - 8, box - 8, 2, theme.accent);
    if (Focused()) canvas.StrokeRect(Px(content.x) - 1, Px(content.y) - 1, box + 2, box + 2, theme.focus, 2);
    Graphics::TextStyle style;
    style.scale = theme.textScale;
    style.color = active ? theme.text : theme.textDisabled;
    canvas.Text(Px(content.x) + box + 8, Px(content.y), text, style);
}

void Toggle::OnPointerUp(Vec2, bool inside) {
    if (inside && EffectiveEnabled()) Set(!checked);
}

bool Toggle::OnKey(std::uint16_t key, bool) {
    if (!IsActivateKey(key)) return false;
    if (EffectiveEnabled()) Set(!checked);
    return true;
}

// ------------------------------------------------------------------ slider

Slider::Slider(float minimum, float maximum, float value, std::string name)
    : Widget(std::move(name)), minimum(minimum), maximum(maximum) {
    value_ = std::clamp(value, std::min(minimum, maximum), std::max(minimum, maximum));
}

void Slider::SetValue(float value) {
    if (!std::isfinite(value)) return;
    const float low = std::min(minimum, maximum), high = std::max(minimum, maximum);
    value = std::clamp(value, low, high);
    if (step > 0.0f) value = std::clamp(low + std::round((value - low) / step) * step, low, high);
    if (value == value_) return;
    value_ = value;
    ValueChanged.Broadcast(value_);
}

Vec2 Slider::MeasureContent(Vec2) { return {160.0f, 18.0f}; }

void Slider::DrawSelf(Graphics::Canvas& canvas, const Theme& theme) const {
    const Rect content = ContentRect();
    const float range = maximum - minimum;
    const float fraction = range != 0.0f ? (value_ - minimum) / range : 0.0f;
    const int trackY = Px(content.y + content.height * 0.5f) - 3;
    canvas.Bar(Px(content.x), trackY, Px(content.width), 6, fraction, EffectiveEnabled() ? theme.accent : theme.textDisabled, theme.track);
    const float knobX = content.x + content.width * fraction;
    canvas.FillCircle(Px(knobX), Px(content.y + content.height * 0.5f), content.height * 0.45f, Hovered() || Pressed() ? theme.focus : theme.text);
    if (Focused()) canvas.StrokeRect(Px(content.x) - 2, Px(content.y) - 2, Px(content.width) + 4, Px(content.height) + 4, theme.focus);
}

void Slider::OnPointerDrag(Vec2 p) {
    if (!EffectiveEnabled()) return;
    const Rect content = ContentRect();
    const float fraction = content.width > 0.0f ? std::clamp((p.x - content.x) / content.width, 0.0f, 1.0f) : 0.0f;
    SetValue(minimum + (maximum - minimum) * fraction);
}

bool Slider::OnKey(std::uint16_t key, bool) {
    if (key != Keys::Left && key != Keys::Right) return false;
    if (!EffectiveEnabled()) return true;
    const float increment = step > 0.0f ? step : (maximum - minimum) * 0.05f;
    SetValue(value_ + (key == Keys::Right ? increment : -increment));
    return true;
}

// ------------------------------------------------------------------ progress bar

Vec2 ProgressBar::MeasureContent(Vec2) { return {120.0f, 12.0f}; }

void ProgressBar::DrawSelf(Graphics::Canvas& canvas, const Theme& theme) const {
    const Rect content = ContentRect();
    canvas.Bar(Px(content.x), Px(content.y), Px(content.width), Px(content.height), value, fill.a > 0 ? fill : theme.accent, theme.track);
}

// ------------------------------------------------------------------ text box

void TextBox::SetText(std::string value) {
    if (value.size() > maxLength) value.resize(maxLength);
    if (value == text) return;
    text = std::move(value);
    TextChanged.Broadcast(text);
}

Vec2 TextBox::MeasureContent(Vec2) {
    const float line = static_cast<float>(Graphics::Canvas::LineHeight(TextScale()));
    const float width = static_cast<float>(Graphics::Canvas::MeasureText(text.empty() ? placeholder : text, TextScale()));
    return {std::max(160.0f, width + 16.0f), line + 6.0f};
}

void TextBox::DrawSelf(Graphics::Canvas& canvas, const Theme& theme) const {
    const Rect& r = Bounds();
    canvas.FillRoundedRect(Px(r.x), Px(r.y), Px(r.width), Px(r.height), 4, theme.track);
    canvas.StrokeRect(Px(r.x), Px(r.y), Px(r.width), Px(r.height), Focused() ? theme.focus : theme.border, Focused() ? 2 : 1);
    const Rect content = ContentRect();
    Graphics::TextStyle style;
    style.scale = theme.textScale;
    style.shadow = false;
    const bool showPlaceholder = text.empty() && !Focused();
    style.color = showPlaceholder || !EffectiveEnabled() ? theme.textDisabled : theme.text;
    const float height = static_cast<float>(Graphics::Canvas::LineHeight(style.scale));
    const int y = Px(content.y + (content.height - height) * 0.5f);
    int x = Px(content.x) + 4;
    // Keep the end of long text (and the caret) in view.
    std::string shown = showPlaceholder ? placeholder : text;
    const int room = Px(content.width) - 16;
    while (!shown.empty() && Graphics::Canvas::MeasureText(shown, style.scale) > room) shown.erase(0, 1);
    x = canvas.Text(x, y, shown, style);
    if (Focused()) canvas.FillRect(x + 1, y, 2 * style.scale, Px(height) - style.scale, theme.focus);
}

bool TextBox::OnKey(std::uint16_t key, bool) {
    if (key == Keys::Backspace) {
        if (!text.empty()) {
            text.pop_back();
            TextChanged.Broadcast(text);
        }
        return true;
    }
    if (key == Keys::Enter) {
        const std::string submitted = text;
        Submitted.Broadcast(submitted);
        return true;
    }
    return false;
}

bool TextBox::OnText(const std::string& typed) {
    if (!EffectiveEnabled()) return false;
    bool changed = false;
    for (char c : typed) {
        if (c < 32 || c > 126 || text.size() >= maxLength) continue;
        text.push_back(c);
        changed = true;
    }
    if (changed) TextChanged.Broadcast(text);
    return true;
}

// ------------------------------------------------------------------ root

UIRoot::UIRoot() : root_(std::make_unique<CanvasPanel>("root")) {}

void UIRoot::ShareTheme(Widget& widget) const {
    widget.theme_ = &theme;
    for (const auto& child : widget.children_) ShareTheme(*child);
}

void UIRoot::Layout(Vec2 viewport) {
    ShareTheme(*root_);
    root_->Measure(viewport);
    root_->Arrange({0.0f, 0.0f, viewport.x, viewport.y});
}

void UIRoot::Collect(Widget& widget, std::vector<Widget*>& out) const {
    if (widget.pendingRemoval_) return;
    out.push_back(&widget);
    for (const auto& child : widget.children_) Collect(*child, out);
}

bool UIRoot::Contains(const Widget* widget) const {
    if (!widget) return false;
    std::vector<Widget*> all;
    Collect(*root_, all);
    return std::find(all.begin(), all.end(), widget) != all.end();
}

void UIRoot::Validate() {
    if (hovered_ && (!Contains(hovered_) || !hovered_->EffectiveVisible() || !hovered_->EffectiveEnabled())) {
        if (Contains(hovered_)) hovered_->hovered_ = false;
        hovered_ = nullptr;
    }
    if (captured_ && (!Contains(captured_) || !captured_->EffectiveVisible())) {
        if (Contains(captured_)) captured_->pressed_ = false;
        captured_ = nullptr;
    }
    if (focused_ && (!Contains(focused_) || !focused_->EffectiveVisible() || !focused_->EffectiveEnabled())) {
        Widget* lost = focused_;
        focused_ = nullptr;
        if (Contains(lost)) {
            lost->focused_ = false;
            lost->OnFocusChanged(false);
        }
    }
}

Widget* UIRoot::HitTest(Widget& widget, Vec2 point) {
    if (!widget.visible || !widget.enabled || widget.pendingRemoval_) return nullptr;
    for (auto it = widget.children_.rbegin(); it != widget.children_.rend(); ++it) {
        if (Widget* hit = HitTest(**it, point)) return hit;
    }
    return widget.HitTestable() && widget.rect_.Contains(point) ? &widget : nullptr;
}

void UIRoot::SetFocus(Widget* widget) {
    if (widget && (!widget->Focusable() || !Contains(widget) || !widget->EffectiveVisible() || !widget->EffectiveEnabled())) {
        widget = nullptr;
    }
    if (widget == focused_) return;
    Widget* previous = focused_;
    focused_ = widget;
    if (previous && Contains(previous)) {
        previous->focused_ = false;
        previous->OnFocusChanged(false);
    }
    if (focused_) {
        focused_->focused_ = true;
        focused_->OnFocusChanged(true);
    }
}

void UIRoot::FocusNext(bool backwards) {
    std::vector<Widget*> all, focusable;
    Collect(*root_, all);
    for (Widget* w : all) {
        if (w->Focusable() && w->EffectiveVisible() && w->EffectiveEnabled()) focusable.push_back(w);
    }
    if (focusable.empty()) {
        SetFocus(nullptr);
        return;
    }
    const auto current = std::find(focusable.begin(), focusable.end(), focused_);
    const auto count = static_cast<std::ptrdiff_t>(focusable.size());
    std::ptrdiff_t index;
    if (current == focusable.end()) {
        index = backwards ? count - 1 : 0;
    } else {
        index = (current - focusable.begin() + (backwards ? count - 1 : 1)) % count;
    }
    SetFocus(focusable[static_cast<std::size_t>(index)]);
}

bool UIRoot::Navigate(std::uint16_t key) {
    Vec2 direction{};
    if (key == Keys::Left) direction = {-1, 0};
    else if (key == Keys::Right) direction = {1, 0};
    else if (key == Keys::Up) direction = {0, -1};
    else if (key == Keys::Down) direction = {0, 1};
    else return false;
    if (!focused_) {
        FocusNext(false);
        return focused_ != nullptr;
    }
    std::vector<Widget*> all;
    Collect(*root_, all);
    const Vec2 from = focused_->rect_.Center();
    Widget* best = nullptr;
    float bestScore = std::numeric_limits<float>::max();
    for (Widget* w : all) {
        if (w == focused_ || !w->Focusable() || !w->EffectiveVisible() || !w->EffectiveEnabled()) continue;
        const Vec2 delta = w->rect_.Center() - from;
        const float along = delta.x * direction.x + delta.y * direction.y;
        if (along <= 1.0f) continue;
        const float across = std::fabs(delta.x * direction.y - delta.y * direction.x);
        const float score = along + 2.0f * across;
        if (score < bestScore) {
            bestScore = score;
            best = w;
        }
    }
    if (best) SetFocus(best);
    return best != nullptr;
}

void UIRoot::PurgeRemoved(Widget& widget) {
    auto& children = widget.children_;
    children.erase(std::remove_if(children.begin(), children.end(),
                       [](const std::unique_ptr<Widget>& child) { return child->pendingRemoval_; }),
        children.end());
    for (const auto& child : children) PurgeRemoved(*child);
}

void UIRoot::Update(const UIInput& input, Vec2 viewport) {
    Validate();
    Layout(viewport);
    // Hover.
    Widget* hit = HitTest(*root_, input.pointer);
    if (hit != hovered_) {
        if (hovered_) hovered_->hovered_ = false;
        hovered_ = hit;
        if (hovered_) hovered_->hovered_ = true;
    }
    // Press, drag, release (the pressed widget captures the pointer).
    const bool down = input.pointerDown;
    if (down && !pointerWasDown_) {
        if (hovered_) {
            captured_ = hovered_;
            captured_->pressed_ = true;
            if (captured_->Focusable()) SetFocus(captured_);
            captured_->OnPointerDown(input.pointer);
        } else {
            SetFocus(nullptr);
        }
    } else if (down && captured_) {
        captured_->OnPointerDrag(input.pointer);
    } else if (!down && pointerWasDown_ && captured_) {
        Widget* released = captured_;
        captured_ = nullptr;
        released->pressed_ = false;
        released->OnPointerUp(input.pointer, released->rect_.Contains(input.pointer));
    }
    pointerWasDown_ = down;
    Validate();
    // Keys go to the focused widget, bubbling to its parents; leftovers navigate.
    for (std::uint16_t key : input.keysPressed) {
        bool handled = false;
        for (Widget* w = focused_; w && !handled; w = w->parent_) {
            handled = (w->keyHandler && w->keyHandler(key, input.shift)) || w->OnKey(key, input.shift);
        }
        if (!handled) {
            if (key == Keys::Tab) FocusNext(input.shift);
            else if (key == Keys::Escape) SetFocus(nullptr);
            else Navigate(key);
        }
        Validate();
    }
    if (!input.text.empty() && focused_) focused_->OnText(input.text);
    Validate();
    PurgeRemoved(*root_);
}

void UIRoot::Draw(Graphics::Canvas& canvas) const { root_->Draw(canvas, theme); }

bool UIRoot::WantsKeyboard() const { return dynamic_cast<const TextBox*>(focused_) != nullptr; }

} // namespace Astral::UI
