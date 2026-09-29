#include "Engine/Core/Console.h"
#include "Engine/Graphics/Canvas.h"
#include "Engine/Input/InputSystem.h"
#include "Engine/UI/ConsoleOverlay.h"
#include "Engine/UI/Widgets.h"
#include "Tests/EngineTestSupport.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

using namespace Astral;
using namespace Astral::UI;
namespace Keys = Input::Keys;

namespace {

bool RectIs(const Rect& r, float x, float y, float w, float h) {
    return std::fabs(r.x - x) < 1e-3f && std::fabs(r.y - y) < 1e-3f && std::fabs(r.width - w) < 1e-3f
        && std::fabs(r.height - h) < 1e-3f;
}

UIInput At(float x, float y, bool down = false) {
    UIInput input;
    input.pointer = {x, y};
    input.pointerDown = down;
    return input;
}

UIInput Key(std::uint16_t key, bool shift = false) {
    UIInput input;
    input.pointer = {-100.0f, -100.0f};
    input.keysPressed.push_back(key);
    input.shift = shift;
    return input;
}

UIInput Typed(const std::string& text) {
    UIInput input;
    input.pointer = {-100.0f, -100.0f};
    input.text = text;
    return input;
}

// Places a widget at an absolute rect inside the root canvas.
template <typename T>
T& Placed(T& widget, float x, float y, float w, float h) {
    widget.anchorMin = widget.anchorMax = {0.0f, 0.0f};
    widget.offsets = {x, y, w, h};
    return widget;
}

const Vec2 kViewport{400.0f, 300.0f};

} // namespace

ASTRAL_TEST(LayoutMeasuresAndArrangesPanels) {
    UIRoot ui;
    StackPanel& stack = ui.Root().Add<StackPanel>("stack");
    stack.offsets = {10.0f, 20.0f, 0.0f, 0.0f}; // point anchor, measured size
    stack.padding = Thickness::All(4.0f);
    stack.spacing = 6.0f;
    Label& title = stack.Add<Label>("Hello");   // 5 glyphs * 12 px, 18 px tall
    Button& ok = stack.Add<Button>("OK");       // 24 + 24 padding wide, 18 + 12 tall
    ok.margin = {0.0f, 2.0f, 0.0f, 2.0f};
    Label& hidden = stack.Add<Label>("Hidden");
    hidden.visible = false;
    Panel& dialog = ui.Root().Add<Panel>("dialog");
    dialog.anchorMin = dialog.anchorMax = {0.5f, 0.5f};
    dialog.pivot = {0.5f, 0.5f};
    dialog.offsets = {0.0f, 0.0f, 200.0f, 100.0f};
    dialog.padding = Thickness::All(10.0f);
    Label& corner = dialog.Add<Label>("Hi");
    corner.hAlign = HAlign::Right;
    corner.vAlign = VAlign::Bottom;
    Label& centred = dialog.Add<Label>("Mid");
    centred.hAlign = HAlign::Center;
    centred.vAlign = VAlign::Center;
    Panel& bar = ui.Root().Add<Panel>("bar");
    bar.anchorMin = {0.0f, 1.0f};
    bar.anchorMax = {1.0f, 1.0f};
    bar.offsets = {16.0f, -40.0f, 16.0f, 24.0f}; // stretched x (insets), point y (offset, height)
    Panel& fixed = ui.Root().Add<Panel>("fixed");
    fixed.anchorMin = {0.0f, 0.0f};
    fixed.anchorMax = {1.0f, 1.0f};
    fixed.offsets = {5.0f, 5.0f, 5.0f, 5.0f};
    Button& small = fixed.Add<Button>("x");
    small.size = {40.0f, 20.0f};

    ui.Update({}, kViewport);
    ASTRAL_CHECK(RectIs(stack.Bounds(), 10, 20, 68, 66));
    ASTRAL_CHECK(RectIs(title.Bounds(), 14, 24, 60, 18));
    ASTRAL_CHECK(RectIs(ok.Bounds(), 14, 50, 60, 30));
    ASTRAL_CHECK(hidden.Desired().x == 0.0f);
    ASTRAL_CHECK(RectIs(dialog.Bounds(), 100, 100, 200, 100));
    ASTRAL_CHECK(RectIs(corner.Bounds(), 100 + 200 - 10 - 24, 100 + 100 - 10 - 18, 24, 18));
    ASTRAL_CHECK(RectIs(centred.Bounds(), 100 + (200 - 36) * 0.5f, 100 + (100 - 18) * 0.5f, 36, 18));
    ASTRAL_CHECK(RectIs(bar.Bounds(), 16, 260, 368, 24));
    ASTRAL_CHECK(RectIs(fixed.Bounds(), 5, 5, 390, 290));
    ASTRAL_CHECK(RectIs(small.Bounds(), 5 + (390 - 40) * 0.5f, 5 + (290 - 20) * 0.5f, 40, 20)); // fixed size centres
    // Layout follows the viewport and theme text scale.
    ui.theme.textScale = 3;
    ui.Update({}, {800.0f, 600.0f});
    ASTRAL_CHECK(RectIs(dialog.Bounds(), 300, 250, 200, 100));
    ASTRAL_CHECK(title.Bounds().width == 90.0f && title.Bounds().height == 27.0f);
    ASTRAL_CHECK(ui.Root().Find("bar") == &bar && ui.Root().FindAs<Button>("dialog") == nullptr);
}

ASTRAL_TEST(PointerHoverPressCaptureAndClicks) {
    UIRoot ui;
    Button& button = Placed(ui.Root().Add<Button>("Go", "go"), 50, 50, 100, 40);
    int clicks = 0;
    button.Clicked.Add([&] { ++clicks; });
    Slider& slider = Placed(ui.Root().Add<Slider>(0.0f, 10.0f, 2.0f, "slider"), 50, 120, 200, 20);
    std::vector<float> values;
    slider.ValueChanged.Add([&](float v) { values.push_back(v); });
    Toggle& toggle = Placed(ui.Root().Add<Toggle>("Sound", false, "sound"), 50, 160, 150, 22);
    bool toggled = false;
    toggle.Toggled.Add([&](bool on) { toggled = on; });

    ui.Update(At(60, 60), kViewport);
    ASTRAL_CHECK(ui.Hovered() == &button && button.Hovered() && ui.PointerOverUI());
    ui.Update(At(60, 60, true), kViewport);
    ASTRAL_CHECK(button.Pressed() && ui.Focused() == &button && clicks == 0);
    ui.Update(At(61, 61), kViewport);
    ASTRAL_CHECK(clicks == 1 && !button.Pressed());
    // Press inside, release outside: no click.
    ui.Update(At(60, 60, true), kViewport);
    ui.Update(At(300, 20, true), kViewport);
    ui.Update(At(300, 20), kViewport);
    ASTRAL_CHECK(clicks == 1 && !ui.PointerOverUI());
    // Slider: press sets the value; the capture keeps dragging outside its bounds.
    ui.Update(At(150, 130, true), kViewport);
    ASTRAL_CHECK_NEAR(slider.Value(), 5.0f, 1e-4);
    ui.Update(At(390, 290, true), kViewport);
    ASTRAL_CHECK(slider.Value() == 10.0f);
    ui.Update(At(390, 290), kViewport);
    ASTRAL_CHECK(values.size() == 2 && values.back() == 10.0f);
    slider.step = 2.5f;
    slider.SetValue(3.4f);
    ASTRAL_CHECK(slider.Value() == 2.5f);
    // Toggle.
    ui.Update(At(60, 170, true), kViewport);
    ui.Update(At(60, 170), kViewport);
    ASTRAL_CHECK(toggle.checked && toggled);
    // A modal panel on top swallows clicks meant for widgets below it.
    Panel& modal = Placed(ui.Root().Add<Panel>("modal"), 0, 0, 400, 300);
    modal.useTheme = true;
    ui.Update(At(60, 60, true), kViewport);
    ui.Update(At(60, 60), kViewport);
    ASTRAL_CHECK(clicks == 1 && ui.Hovered() == &modal);
    modal.visible = false;
    ui.Update(At(60, 60, true), kViewport);
    ui.Update(At(60, 60), kViewport);
    ASTRAL_CHECK(clicks == 2);
    // Disabled buttons neither hover nor click.
    button.enabled = false;
    ui.Update(At(60, 60, true), kViewport);
    ui.Update(At(60, 60), kViewport);
    ASTRAL_CHECK(clicks == 2 && ui.Hovered() != &button);
}

ASTRAL_TEST(KeyboardFocusNavigationAndTextEntry) {
    UIRoot ui;
    StackPanel& column = Placed(ui.Root().Add<StackPanel>("column"), 20, 20, 200, 260);
    Button& a = column.Add<Button>("A", "a");
    Toggle& b = column.Add<Toggle>("B", false, "b");
    Slider& c = column.Add<Slider>(0.0f, 1.0f, 0.5f, "c");
    TextBox& d = column.Add<TextBox>("d");
    Button& e = column.Add<Button>("E", "e");
    e.enabled = false;
    int clicks = 0;
    a.Clicked.Add([&] { ++clicks; });
    ui.Update({}, kViewport);
    std::vector<const Widget*> order;
    for (int i = 0; i < 5; ++i) {
        ui.Update(Key(Keys::Tab), kViewport);
        order.push_back(ui.Focused());
    }
    ASTRAL_CHECK((order == std::vector<const Widget*>{&a, &b, &c, &d, &a})); // E is disabled
    ui.Update(Key(Keys::Tab, true), kViewport);
    ASTRAL_CHECK(ui.Focused() == &d);
    ui.SetFocus(&a);
    ui.Update(Key(Keys::Enter), kViewport);
    ASTRAL_CHECK(clicks == 1 && a.Focused());
    ui.SetFocus(&b);
    ui.Update(Key(Keys::Space), kViewport);
    ASTRAL_CHECK(b.checked);
    ui.SetFocus(&c);
    ui.Update(Key(Keys::Right), kViewport);
    ASTRAL_CHECK_NEAR(c.Value(), 0.55f, 1e-5);
    // Text entry.
    std::string submitted;
    d.Submitted.Add([&](const std::string& text) { submitted = text; });
    d.maxLength = 8;
    ui.SetFocus(&d);
    ASTRAL_CHECK(ui.WantsKeyboard());
    ui.Update(Typed("hello\tworld!"), kViewport); // non-printable dropped, length capped
    ASTRAL_CHECK(d.text == "hellowor");
    ui.Update(Key(Keys::Backspace), kViewport);
    ui.Update(Key(Keys::Enter), kViewport);
    ASTRAL_CHECK(submitted == "hellowo");
    ui.Update(Key(Keys::Escape), kViewport);
    ASTRAL_CHECK(ui.Focused() == nullptr && !ui.WantsKeyboard());

    // Arrow keys move focus spatially when the focused widget does not use them.
    UIRoot grid;
    StackPanel& row = Placed(grid.Root().Add<StackPanel>("row", Orientation::Horizontal), 20, 20, 360, 40);
    Button& left = row.Add<Button>("L");
    Button& middle = row.Add<Button>("M");
    Button& right = row.Add<Button>("R");
    Button& below = Placed(grid.Root().Add<Button>("Below"), 60, 200, 80, 30);
    grid.Update({}, kViewport);
    grid.SetFocus(&middle);
    grid.Update(Key(Keys::Right), kViewport);
    ASTRAL_CHECK(grid.Focused() == &right);
    grid.Update(Key(Keys::Left), kViewport);
    grid.Update(Key(Keys::Left), kViewport);
    ASTRAL_CHECK(grid.Focused() == &left);
    grid.Update(Key(Keys::Down), kViewport);
    ASTRAL_CHECK(grid.Focused() == &below);
    grid.Update(Key(Keys::Down), kViewport); // nothing further down: focus stays
    ASTRAL_CHECK(grid.Focused() == &below);
}

ASTRAL_TEST(RemovingWidgetsInsideCallbacksIsSafe) {
    UIRoot ui;
    Panel& dialog = Placed(ui.Root().Add<Panel>("dialog"), 50, 50, 200, 120);
    Button& close = Placed(dialog.Add<Button>("Close", "close"), 0, 0, 0, 0);
    close.hAlign = HAlign::Center;
    close.vAlign = VAlign::Center;
    close.Clicked.Add([&] { dialog.RemoveFromParent(); });
    ui.Update({}, kViewport);
    ui.SetFocus(&close);
    ui.Update(Key(Keys::Enter), kViewport); // removes its own dialog while handling the key
    ASTRAL_CHECK(ui.Root().Find("dialog") == nullptr && ui.Focused() == nullptr);
    ASTRAL_CHECK(ui.Root().Children().empty());
    // Same through the pointer path, with the button deleting itself.
    Button& self = Placed(ui.Root().Add<Button>("Bye", "bye"), 10, 10, 80, 30);
    Button* selfPointer = &self;
    self.Clicked.Add([selfPointer] { selfPointer->RemoveFromParent(); });
    ui.Update(At(20, 20, true), kViewport);
    ui.Update(At(20, 20), kViewport);
    ASTRAL_CHECK(ui.Root().Find("bye") == nullptr && ui.Hovered() == nullptr);
    ui.Update(At(20, 20, true), kViewport);
    ui.Update(At(20, 20), kViewport);
    // ClearChildren defers too.
    StackPanel& list = ui.Root().Add<StackPanel>("list");
    for (int i = 0; i < 3; ++i) list.Add<Label>("row");
    list.ClearChildren();
    ASTRAL_CHECK(list.Children().size() == 3 && list.Find("row") == nullptr);
    ui.Update({}, kViewport);
    ASTRAL_CHECK(list.Children().empty());
}

ASTRAL_TEST(WidgetsDrawOntoTheCanvas) {
    Graphics::ImageRgba8 image;
    image.Resize(200, 100, {0, 0, 0, 255});
    Graphics::Canvas canvas(image);
    UIRoot ui;
    Placed(ui.Root().Add<Button>("OK"), 20, 20, 100, 30);
    Label& hidden = Placed(ui.Root().Add<Label>("secret"), 130, 60, 60, 20);
    hidden.visible = false;
    ProgressBar& progress = Placed(ui.Root().Add<ProgressBar>(0.5f), 20, 70, 100, 10);
    TextBox& box = Placed(ui.Root().Add<TextBox>("box"), 130, 10, 60, 30);
    box.text = "a very long line that scrolls";
    ui.Update({}, {200.0f, 100.0f});
    ui.SetFocus(&box);
    ui.Draw(canvas);
    auto isBlack = [&](int x, int y) {
        const Graphics::Rgba8 p = image.Get(x, y);
        return p.r == 0 && p.g == 0 && p.b == 0;
    };
    ASTRAL_CHECK(!isBlack(25, 22));   // button face
    ASTRAL_CHECK(isBlack(5, 5));      // background untouched
    ASTRAL_CHECK(!isBlack(30, 75));   // progress fill
    ASTRAL_CHECK(!isBlack(110, 75));  // progress track
    bool hiddenDrawn = false;
    for (int y = 60; y < 80; ++y) {
        for (int x = 130; x < 190; ++x) hiddenDrawn = hiddenDrawn || !isBlack(x, y);
    }
    ASTRAL_CHECK(!hiddenDrawn);
    (void)progress;
}

ASTRAL_TEST(ConsoleOverlayRunsCommandsWithHistoryAndCompletion) {
    Core::ConsoleRegistry console;
    console.RegisterInt("r.Quality", 1, "Renderer preset", Core::CVarArchive)->SetRange(0, 3);
    console.RegisterFloat("r.Gamma", 2.2f, "Display gamma");
    console.RegisterFloat("r.GammaCurve", 1.0f, "Tone curve");
    console.RegisterCommand("say", "Echo the arguments", [](const std::vector<std::string>& args) {
        std::string joined;
        for (const std::string& arg : args) joined += (joined.empty() ? "" : " ") + arg;
        return joined;
    });
    UIRoot ui;
    ConsoleOverlay overlay(ui, console, 6);
    ASTRAL_CHECK(!overlay.IsOpen());
    // The toggle key opens it and never reaches the text box.
    UIInput toggle = Key(overlay.toggleKey);
    toggle.text = "`";
    overlay.PreprocessInput(toggle);
    ASTRAL_CHECK(overlay.IsOpen() && toggle.keysPressed.empty() && toggle.text.empty());
    ui.Update(toggle, {640.0f, 360.0f});
    ASTRAL_CHECK(ui.Focused() == &overlay.InputBox() && ui.WantsKeyboard());
    auto type = [&](const std::string& text) {
        UIInput input = Typed(text);
        overlay.PreprocessInput(input);
        ui.Update(input, {640.0f, 360.0f});
    };
    auto press = [&](std::uint16_t key) {
        UIInput input = Key(key);
        overlay.PreprocessInput(input);
        ui.Update(input, {640.0f, 360.0f});
    };
    type("r.Quality 2");
    press(Keys::Enter);
    ASTRAL_CHECK(console.Find("r.Quality")->GetInt() == 2);
    ASTRAL_CHECK(overlay.InputBox().text.empty());
    const auto& lines = overlay.Lines();
    ASTRAL_CHECK(std::find(lines.begin(), lines.end(), "> r.Quality 2") != lines.end());
    ASTRAL_CHECK(lines.back().find("r.Quality") != std::string::npos);
    type("say hello   world");
    press(Keys::Enter);
    ASTRAL_CHECK(lines.back() == "hello world");
    // History: Up walks back, Down returns to the draft.
    type("draft");
    press(Keys::Up);
    ASTRAL_CHECK(overlay.InputBox().text == "say hello   world");
    press(Keys::Up);
    ASTRAL_CHECK(overlay.InputBox().text == "r.Quality 2");
    press(Keys::Down);
    press(Keys::Down);
    ASTRAL_CHECK(overlay.InputBox().text == "draft");
    // Tab completion: unique match completes; several list and extend the prefix.
    overlay.InputBox().SetText("r.Q");
    press(Keys::Tab);
    ASTRAL_CHECK(overlay.InputBox().text == "r.Quality ");
    ASTRAL_CHECK(ui.Focused() == &overlay.InputBox()); // Tab did not move focus
    overlay.InputBox().SetText("r.G");
    press(Keys::Tab);
    ASTRAL_CHECK(overlay.InputBox().text == "r.Gamma");
    ASTRAL_CHECK(lines.back().find("r.GammaCurve") != std::string::npos && lines.back().find("r.Quality") == std::string::npos);
    // Drawing the open console works; Escape closes it and releases focus.
    Graphics::ImageRgba8 image;
    image.Resize(640, 360, {0, 0, 0, 255});
    Graphics::Canvas canvas(image);
    ui.Draw(canvas);
    press(Keys::Escape);
    ASTRAL_CHECK(!overlay.IsOpen() && ui.Focused() == nullptr);
    // The scrollback is bounded.
    overlay.maxLines = 10;
    for (int i = 0; i < 30; ++i) overlay.Print("line " + std::to_string(i));
    ASTRAL_CHECK(lines.size() == 10 && lines.back() == "line 29");
}

ASTRAL_TEST_MAIN("EngineUITests")
