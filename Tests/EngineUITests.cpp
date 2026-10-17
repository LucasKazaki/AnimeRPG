#include "Engine/Core/Console.h"
#include "Engine/Graphics/Canvas.h"
#include "Engine/Framework/GameHost.h"
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

std::unique_ptr<Framework::GameHost> OwnershipHost() {
    Framework::HostSettings settings;
    settings.workerThreads = 0;
    settings.audio = false;
    auto host = std::make_unique<Framework::GameHost>(settings);
    Input::InputContext context;
    context.name = "Neutral ownership fixture";
    context.actions = {
        Input::ActionDesc{"Attack", Input::ActionType::Button, {{Keys::MouseLeft, {}}}},
        Input::ActionDesc{"Jump", Input::ActionType::Button, {{Keys::Space, {}}}},
        Input::ActionDesc{"Move", Input::ActionType::Axis2D, {{'W', {0, 1}}}},
        Input::ActionDesc{"Cancel", Input::ActionType::Button, {{Keys::Escape, {}}}},
    };
    ASTRAL_CHECK(host->Input().AddContext(context));
    return host;
}

Framework::HostInput HostPointer(float x, float y, bool down = false) {
    Framework::HostInput input;
    input.ui = At(x, y, down);
    input.keys.keys.set(Keys::MouseLeft, down);
    return input;
}

Framework::HostInput HostKey(std::uint16_t key) {
    Framework::HostInput input;
    input.ui = Key(key);
    input.keys.keys.set(key);
    return input;
}

constexpr float kHostFrame = 1.0f / 60.0f;

} // namespace

ASTRAL_TEST(UIHostButtonPointerDoesNotReachGameplay) {
    auto host = OwnershipHost();
    Button& button = Placed(host->Hud().Add<Button>("Neutral action"), 50, 50, 100, 40);
    int clicks = 0;
    button.Clicked.Add([&] { ++clicks; });
    host->Update(kHostFrame, HostPointer(60, 60, true));
    ASTRAL_CHECK(button.Pressed() && !host->Input().Down("Attack"));
    host->Update(kHostFrame, HostPointer(60, 60));
    ASTRAL_CHECK(clicks == 1 && !host->Input().Pressed("Attack"));
    host->Update(kHostFrame, HostPointer(60, 60, true));
    Framework::HostInput held = HostPointer(800, 650, true);
    held.keys.keys.set('W');
    for (int i = 0; i < 3; ++i) {
        host->Update(kHostFrame, held);
        ASTRAL_CHECK(!host->Input().Down("Attack") && host->Input().Down("Move"));
    }
    host->Update(kHostFrame, HostPointer(800, 650));
    ASTRAL_CHECK(clicks == 1 && !host->Input().Released("Attack")); // outside release cancels UI click
    host->Update(kHostFrame, HostPointer(800, 650, true));
    ASTRAL_CHECK(host->Input().Pressed("Attack")); // fresh unhandled press is available
}

ASTRAL_TEST(UIHostButtonKeyboardDoesNotReachGameplay) {
    auto host = OwnershipHost();
    Button& button = Placed(host->Hud().Add<Button>("Neutral action"), 50, 50, 100, 40);
    int clicks = 0;
    button.Clicked.Add([&] { ++clicks; });
    host->UI().SetFocus(&button);
    host->Update(kHostFrame, HostKey(Keys::Space));
    ASTRAL_CHECK(clicks == 1 && !host->Input().Down("Jump"));
    Framework::HostInput held = HostKey(Keys::Space);
    held.ui.keysPressed.clear();
    host->UI().SetFocus(nullptr);
    for (int i = 0; i < 3; ++i) {
        host->Update(kHostFrame, held);
        ASTRAL_CHECK(!host->Input().Down("Jump") && clicks == 1);
    }
    host->Update(kHostFrame, {});
    ASTRAL_CHECK(!host->Input().Released("Jump"));
    host->Update(kHostFrame, HostKey(Keys::Space));
    ASTRAL_CHECK(host->Input().Pressed("Jump") && clicks == 1);
}

ASTRAL_TEST(UIHostConsoleOutsidePointerKeepsModalInput) {
    auto host = OwnershipHost();
    Button& behind = Placed(host->Hud().Add<Button>("Behind console"), 750, 600, 200, 80);
    int clicks = 0;
    behind.Clicked.Add([&] { ++clicks; });
    host->Update(kHostFrame, HostKey(host->Console().toggleKey));
    ASTRAL_CHECK(host->Console().IsOpen());
    Framework::HostInput outside = HostPointer(800, 650, true);
    outside.keys.keys.set('W');
    host->Update(kHostFrame, outside);
    ASTRAL_CHECK(host->Console().IsOpen() && !host->Input().Down("Move") && !host->Input().Down("Attack"));
    ASTRAL_CHECK(!behind.Pressed() && host->UI().Focused() == &host->Console().InputBox());
    host->Update(kHostFrame, HostPointer(800, 650));
    ASTRAL_CHECK(clicks == 0);
}

ASTRAL_TEST(UIHostConsoleEscapeClosesWithoutTextFocus) {
    auto host = OwnershipHost();
    host->Update(kHostFrame, HostKey(host->Console().toggleKey));
    host->Update(kHostFrame, HostPointer(800, 650, true));
    host->Update(kHostFrame, HostPointer(800, 650));
    host->UI().SetFocus(nullptr); // Escape routing must not rely on the TextBox
    host->Update(kHostFrame, HostKey(Keys::Escape));
    ASTRAL_CHECK(!host->Console().IsOpen() && !host->Input().Down("Cancel"));
    Framework::HostInput held = HostKey(Keys::Escape);
    held.ui.keysPressed.clear();
    host->Update(kHostFrame, held);
    ASTRAL_CHECK(!host->Input().Down("Cancel"));
    host->Update(kHostFrame, {});
    host->Update(kHostFrame, HostKey(Keys::Escape));
    ASTRAL_CHECK(host->Input().Pressed("Cancel"));
}

ASTRAL_TEST(UIHostPointerCancellationRetainsOwnershipUntilRelease) {
    for (int cancellation = 0; cancellation < 3; ++cancellation) {
        auto host = OwnershipHost();
        Button& button = Placed(host->Hud().Add<Button>("Cancel fixture", "fixture"), 50, 50, 100, 40);
        int clicks = 0;
        button.Clicked.Add([&] { ++clicks; });
        host->Update(kHostFrame, HostPointer(60, 60, true));
        if (cancellation == 0) button.RemoveFromParent();
        if (cancellation == 1) button.enabled = false;
        if (cancellation == 2) button.visible = false;
        host->Update(kHostFrame, HostPointer(60, 60, true));
        ASTRAL_CHECK(!host->Input().Down("Attack") && host->UI().Focused() == nullptr);
        if (cancellation != 0) ASTRAL_CHECK(!button.Pressed());
        host->Update(kHostFrame, HostPointer(60, 60));
        ASTRAL_CHECK(clicks == 0 && !host->Input().Down("Attack"));
        host->Update(kHostFrame, HostPointer(800, 650, true));
        ASTRAL_CHECK(host->Input().Pressed("Attack"));
    }
}

ASTRAL_TEST(UIHostKeyboardCancellationRetainsOwnershipUntilRelease) {
    for (int cancellation = 0; cancellation < 3; ++cancellation) {
        auto host = OwnershipHost();
        Button& button = Placed(host->Hud().Add<Button>("Cancel fixture"), 50, 50, 100, 40);
        button.Clicked.Add([&] {
            if (cancellation == 0) button.RemoveFromParent();
            if (cancellation == 1) button.enabled = false;
            if (cancellation == 2) host->UI().SetFocus(nullptr);
        });
        host->UI().SetFocus(&button);
        host->Update(kHostFrame, HostKey(Keys::Space));
        ASTRAL_CHECK(!host->Input().Down("Jump") && host->UI().Focused() == nullptr);
        Framework::HostInput held = HostKey(Keys::Space);
        held.ui.keysPressed.clear();
        host->Update(kHostFrame, held);
        ASTRAL_CHECK(!host->Input().Down("Jump"));
        host->Update(kHostFrame, {});
        host->Update(kHostFrame, HostKey(Keys::Space));
        ASTRAL_CHECK(host->Input().Pressed("Jump"));
    }
}

ASTRAL_TEST(UIHostModalClosingFrameCannotActivateHudOrGameplay) {
    auto host = OwnershipHost();
    Button& behind = Placed(host->Hud().Add<Button>("Behind console"), 750, 600, 200, 80);
    int clicks = 0;
    behind.Clicked.Add([&] { ++clicks; });
    host->Update(kHostFrame, HostKey(host->Console().toggleKey));
    Framework::HostInput closing = HostPointer(800, 650, true);
    closing.ui.keysPressed = {Keys::Escape, Keys::Space};
    closing.ui.text = "ignored on close";
    closing.keys.keys.set(Keys::Escape);
    closing.keys.keys.set(Keys::Space);
    closing.keys.keys.set('W');
    host->Update(kHostFrame, closing);
    ASTRAL_CHECK(!host->Console().IsOpen() && !behind.Pressed() && clicks == 0);
    closing.ui.keysPressed.clear();
    closing.ui.text.clear();
    for (int i = 0; i < 3; ++i) {
        host->Update(kHostFrame, closing);
        for (const char* action : {"Attack", "Jump", "Move", "Cancel"}) ASTRAL_CHECK(!host->Input().Down(action));
    }
    host->Update(kHostFrame, HostPointer(1100, 680));
    Framework::HostInput fresh = HostPointer(1100, 680, true);
    fresh.keys.keys.set('W');
    fresh.keys.keys.set(Keys::Space);
    fresh.ui.keysPressed = {Keys::Space};
    host->Update(kHostFrame, fresh);
    ASTRAL_CHECK(host->Input().Pressed("Attack") && host->Input().Pressed("Jump") && host->Input().Pressed("Move"));
    ASTRAL_CHECK(clicks == 0);
}

ASTRAL_TEST(UIHostTextEntryAndUnhandledInputsRemainDistinct) {
    auto host = OwnershipHost();
    Button& button = Placed(host->Hud().Add<Button>("Ordinary button"), 50, 50, 100, 40);
    host->UI().SetFocus(&button);
    host->Update(kHostFrame, HostKey('W'));
    ASTRAL_CHECK(host->Input().Pressed("Move")); // unhandled key on a button reaches the game
    host->Update(kHostFrame, {});
    TextBox& box = Placed(host->Hud().Add<TextBox>(), 50, 120, 200, 30);
    host->UI().SetFocus(&box);
    Framework::HostInput typing = HostKey('W');
    typing.ui.text = "w";
    host->Update(kHostFrame, typing);
    ASTRAL_CHECK(box.text == "w" && !host->Input().Down("Move"));
    box.enabled = false;
    typing.ui.keysPressed.clear();
    typing.ui.text.clear();
    host->Update(kHostFrame, typing);
    ASTRAL_CHECK(host->UI().Focused() == nullptr && !host->Input().Down("Move"));
    host->Update(kHostFrame, {});
    host->Update(kHostFrame, HostKey('W'));
    ASTRAL_CHECK(host->Input().Pressed("Move"));
}

ASTRAL_TEST(UIConsumptionReportsActualHandlersAndCapture) {
    UIRoot ui;
    Button& button = Placed(ui.Root().Add<Button>("Button"), 50, 50, 100, 40);
    ui.SetFocus(&button);
    UIInputResult result = ui.Update(Key(Keys::Space), kViewport);
    ASTRAL_CHECK(result.consumedKeys.test(Keys::Space) && !result.keyboardCaptured && !result.modal);
    result = ui.Update(Key('W'), kViewport);
    ASTRAL_CHECK(!result.consumedKeys.test('W'));
    result = ui.Update(At(60, 60, true), kViewport);
    ASTRAL_CHECK(result.pointerConsumed);
    button.enabled = false;
    result = ui.Update(At(300, 280, true), kViewport);
    ASTRAL_CHECK(result.pointerConsumed && !button.Pressed());
    result = ui.Update(At(300, 280), kViewport);
    ASTRAL_CHECK(result.pointerConsumed);
    result = ui.Update(At(300, 280), kViewport);
    ASTRAL_CHECK(!result.pointerConsumed);
}

ASTRAL_TEST(UIFocusCallbackCanCancelPointerTargetBeforeDispatch) {
    class BlurButton : public Button {
    public:
        std::function<void()> onBlur;
    protected:
        void OnFocusChanged(bool focused) override { if (!focused && onBlur) onBlur(); }
    };
    class PointerProbe : public Button {
    public:
        int downs{};
    protected:
        void OnPointerDown(Vec2) override { ++downs; }
    };
    UIRoot ui;
    Panel& modal = Placed(ui.Root().Add<Panel>("new modal"), 200, 180, 150, 80);
    modal.useTheme = true;
    BlurButton& previous = Placed(ui.Root().Add<BlurButton>(), 10, 10, 100, 30);
    PointerProbe& target = Placed(ui.Root().Add<PointerProbe>(), 50, 80, 100, 40);
    previous.onBlur = [&] { ui.SetModal(&modal); };
    ui.Update({}, kViewport);
    ui.SetFocus(&previous);
    const UIInputResult result = ui.Update(At(60, 90, true), kViewport);
    ASTRAL_CHECK(ui.Modal() == &modal && target.downs == 0 && !target.Pressed());
    ASTRAL_CHECK(result.modal && result.pointerConsumed);
    ui.Update(At(60, 90), kViewport);
}

ASTRAL_TEST(UIModalKeyboardBubblingStopsAtModalRoot) {
    UIRoot ui;
    Panel& outer = Placed(ui.Root().Add<Panel>("outer"), 0, 0, 400, 300);
    Panel& modal = Placed(outer.Add<Panel>("modal"), 20, 20, 200, 100);
    TextBox& input = Placed(modal.Add<TextBox>(), 0, 0, 150, 30);
    int outsideCalls = 0;
    outer.keyHandler = [&](std::uint16_t, bool) { ++outsideCalls; return true; };
    ui.SetModal(&modal);
    ui.SetFocus(&input);
    const UIInputResult result = ui.Update(Key(Keys::Space), kViewport);
    if (outsideCalls) std::fprintf(stderr, "modal key bubbled to outer handler %d time(s)\n", outsideCalls);
    ASTRAL_CHECK(outsideCalls == 0 && result.modal && result.keyboardCaptured);
}

ASTRAL_TEST(UIModalOpenedAndClosedDuringDispatchOwnsRemainingFrame) {
    UIRoot ui;
    Button& launcher = Placed(ui.Root().Add<Button>("Launcher"), 50, 50, 100, 40);
    Panel& modal = Placed(ui.Root().Add<Panel>("modal"), 200, 100, 150, 100);
    modal.visible = false;
    Button& close = Placed(modal.Add<Button>("Close"), 0, 0, 80, 30);
    int launches = 0, closes = 0;
    launcher.Clicked.Add([&] { ++launches; modal.visible = true; ui.SetModal(&modal); ui.SetFocus(&close); });
    close.Clicked.Add([&] { ++closes; modal.visible = false; ui.SetModal(nullptr); ui.SetFocus(&launcher); });
    ui.Update(At(60, 60, true), kViewport);
    UIInput release = At(60, 60);
    release.keysPressed = {Keys::Enter, Keys::Space};
    const UIInputResult result = ui.Update(release, kViewport);
    if (launches != 1) std::fprintf(stderr, "queued key escaped transient modal: launches=%d closes=%d\n", launches, closes);
    ASTRAL_CHECK(launches == 1 && closes == 1 && ui.Modal() == nullptr);
    ASTRAL_CHECK(result.modal && result.keyboardCaptured && result.pointerConsumed);

    // Even a modal entirely opened and closed in one callback owns the frame.
    UIRoot transient;
    Button& activate = Placed(transient.Root().Add<Button>("Activate"), 50, 50, 100, 40);
    Panel& brief = Placed(transient.Root().Add<Panel>("brief modal"), 200, 100, 150, 100);
    int activations = 0;
    activate.Clicked.Add([&] { ++activations; transient.SetModal(&brief); transient.SetModal(nullptr); transient.SetFocus(&activate); });
    transient.SetFocus(&activate);
    UIInput queued;
    queued.keysPressed = {Keys::Enter, Keys::Space};
    const UIInputResult briefResult = transient.Update(queued, kViewport);
    ASTRAL_CHECK(activations == 1 && transient.Modal() == nullptr);
    ASTRAL_CHECK(briefResult.modal && briefResult.keyboardCaptured && briefResult.pointerConsumed);
}

ASTRAL_TEST(UIKeyHookScopeChangesCancelOldRoute) {
    for (int change = 0; change != 3; ++change) {
        UIRoot ui;
        Panel& parent = Placed(ui.Root().Add<Panel>("parent"), 0, 0, 150, 100);
        Button& target = Placed(parent.Add<Button>("target"), 10, 10, 100, 30);
        Panel& modal = Placed(ui.Root().Add<Panel>("modal"), 200, 100, 150, 100);
        int clicks = 0, bubbled = 0;
        target.Clicked.Add([&] { ++clicks; });
        parent.keyHandler = [&](std::uint16_t, bool) { ++bubbled; return true; };
        target.keyHandler = [&](std::uint16_t, bool) {
            if (change == 0) target.enabled = false;
            else if (change == 1) target.RemoveFromParent();
            else ui.SetModal(&modal);
            return false;
        };
        ui.SetFocus(&target);
        const UIInputResult result = ui.Update(Key(Keys::Space), kViewport);
        ASTRAL_CHECK(clicks == 0 && bubbled == 0 && result.consumedKeys.test(Keys::Space));
        ASTRAL_CHECK(ui.Focused() == nullptr);
    }
}

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
