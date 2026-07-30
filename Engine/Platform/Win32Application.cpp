#include "Engine/Platform/Win32Application.h"

#include "Engine/Core/Clock.h"
#include "Engine/Core/Logger.h"
#include "Engine/Renderer/Renderer.h"

#include <string>

namespace {
constexpr wchar_t kWindowClass[] = L"AstralEngineWindow";
Astral::Renderer::Renderer g_renderer;
Astral::Core::Logger g_logger("astral.log");

const wchar_t* AttackName(Astral::Scene::AttackType type) {
    return type == Astral::Scene::AttackType::Light ? L"Light" : L"Heavy";
}

const wchar_t* ResultName(Astral::Scene::AttackResult result) {
    switch (result) {
    case Astral::Scene::AttackResult::Ready: return L"Ready";
    case Astral::Scene::AttackResult::Hit: return L"Hit";
    case Astral::Scene::AttackResult::Cooldown: return L"Cooldown";
    case Astral::Scene::AttackResult::OutOfRange: return L"Out of Range";
    case Astral::Scene::AttackResult::TargetDefeated: return L"Already Defeated";
    }
    return L"Unknown";
}

const wchar_t* ShadowActionName(Astral::Scene::ShadowActionType type) {
    switch (type) {
    case Astral::Scene::ShadowActionType::None: return L"None";
    case Astral::Scene::ShadowActionType::Dash: return L"Dash";
    case Astral::Scene::ShadowActionType::FatalStrike: return L"Fatal";
    case Astral::Scene::ShadowActionType::Guard: return L"Guard";
    }
    return L"Unknown";
}

const wchar_t* ShadowResultName(Astral::Scene::ShadowActionResult result) {
    switch (result) {
    case Astral::Scene::ShadowActionResult::Ready: return L"Ready";
    case Astral::Scene::ShadowActionResult::Activated: return L"Activated";
    case Astral::Scene::ShadowActionResult::Guarding: return L"Active";
    case Astral::Scene::ShadowActionResult::GuardedConflict: return L"Blocked by Guard";
    case Astral::Scene::ShadowActionResult::Cooldown: return L"Cooldown";
    case Astral::Scene::ShadowActionResult::InsufficientResource: return L"No Resource";
    case Astral::Scene::ShadowActionResult::OutOfRange: return L"Out of Range";
    case Astral::Scene::ShadowActionResult::TargetDefeated: return L"Target Defeated";
    }
    return L"Unknown";
}

void UpdateTitle(HWND window, float fps, const Astral::Scene::Transform& state,
    const Astral::Scene::CombatSandbox& combatSandbox,
    const Astral::Scene::ShadowbladeActions& shadowbladeActions) {
    const Astral::Math::Vec3 position = state.WorldPosition();
    const Astral::Scene::TrainingDummy& dummy = combatSandbox.Dummy();
    const Astral::Scene::AttackReport& attack = combatSandbox.LastAttack();
    const Astral::Scene::ShadowActionReport& shadow = shadowbladeActions.LastAction();
    const std::wstring title = L"Astral Engine | M7 Shadowblade | Q Dash L Fatal Shift Guard | Shadow: "
        + std::to_wstring(static_cast<int>(shadowbladeActions.Resource())) + L"/100 | Guard: "
        + (shadowbladeActions.IsGuarding() ? L"ON" : L"OFF")
        + L" | DashCD: " + std::to_wstring(static_cast<int>(shadowbladeActions.DashCooldownRemaining() * 10.0f))
        + L" | FatalCD: " + std::to_wstring(static_cast<int>(shadowbladeActions.FatalStrikeCooldownRemaining() * 10.0f))
        + L" | Shadow Last: " + ShadowActionName(shadow.type) + L" " + ShadowResultName(shadow.result)
        + (shadow.damageApplied > 0 ? L" -" + std::to_wstring(shadow.damageApplied) : L"")
        + L" | M5 Perspective Mall | WASD Traverse | J Light K Heavy | Dummy: "
        + std::wstring(dummy.IsDefeated() ? L"Defeated" : L"Alive") + L" HP: "
        + std::to_wstring(dummy.health) + L"/" + std::to_wstring(dummy.maximumHealth)
        + L" | Last: " + AttackName(attack.type) + L" " + ResultName(attack.result)
        + (attack.damageApplied > 0 ? L" -" + std::to_wstring(attack.damageApplied) : L"")
        + L" | FPS: " + std::to_wstring(static_cast<int>(fps)) + L" | Pos: ("
        + std::to_wstring(position.x) + L", "
        + std::to_wstring(position.y) + L")";
    SetWindowTextW(window, title.c_str());
}
} // namespace

namespace Astral::Platform {

bool Win32Application::Create(HINSTANCE instance, int showCommand) {
    WNDCLASSW windowClass{};
    windowClass.hInstance = instance;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.lpszClassName = kWindowClass;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    if (!RegisterClassW(&windowClass)) {
        g_logger.Info("RegisterClassW failed");
        return false;
    }

    window_ = CreateWindowExW(0, kWindowClass, L"Astral Engine", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, nullptr, nullptr, instance, nullptr);
    if (!window_) {
        g_logger.Info("CreateWindowExW failed");
        return false;
    }

    ShowWindow(window_, showCommand);
    UpdateWindow(window_);
    playerController_.SetPosition({0.0f, 0.0f, 0.0f});
    camera_.Follow(playerController_.TransformState());
    UpdateTitle(window_, 0.0f, playerController_.TransformState(), combatSandbox_,
        shadowbladeActions_);
    g_logger.Info("Window created; M7 Shadowblade active; Q dash, L fatal, Shift guard, Escape exits");
    return true;
}

int Win32Application::Run() {
    Astral::Core::Clock clock;
    MSG message{};
    double fpsAccumulator = 0.0;
    int frameCount = 0;

    while (message.message != WM_QUIT) {
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }

        const float deltaSeconds = clock.Tick();
        fpsAccumulator += deltaSeconds;
        ++frameCount;
        if (fpsAccumulator >= 1.0) {
            UpdateTitle(window_, static_cast<float>(frameCount / fpsAccumulator),
                playerController_.TransformState(), combatSandbox_, shadowbladeActions_);
            g_logger.Info("Frame timing active");
            fpsAccumulator = 0.0;
            frameCount = 0;
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            PostMessageW(window_, WM_CLOSE, 0, 0);
        }

        const Scene::MovementInput input{
            (GetAsyncKeyState('W') & 0x8000) != 0,
            (GetAsyncKeyState('S') & 0x8000) != 0,
            (GetAsyncKeyState('A') & 0x8000) != 0,
            (GetAsyncKeyState('D') & 0x8000) != 0,
        };
        playerController_.Update(input, deltaSeconds);
        camera_.Follow(playerController_.TransformState());
        combatSandbox_.AdvanceTime(deltaSeconds);
        shadowbladeActions_.AdvanceTime(deltaSeconds);

        const bool guarding = (GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0;
        const bool guardChanged = guarding != shadowbladeActions_.IsGuarding();
        shadowbladeActions_.SetGuarding(guarding);

        const bool lightAttackDown = (GetAsyncKeyState('J') & 0x8000) != 0;
        const bool heavyAttackDown = (GetAsyncKeyState('K') & 0x8000) != 0;
        const bool dashDown = (GetAsyncKeyState('Q') & 0x8000) != 0;
        const bool fatalStrikeDown = (GetAsyncKeyState('L') & 0x8000) != 0;
        bool attacked = false;
        bool shadowAction = false;
        if (!guarding && lightAttackDown && !lightAttackPressed_) {
            combatSandbox_.TryAttack(Scene::AttackType::Light,
                playerController_.TransformState().WorldPosition());
            attacked = true;
        } else if (!guarding && heavyAttackDown && !heavyAttackPressed_) {
            combatSandbox_.TryAttack(Scene::AttackType::Heavy,
                playerController_.TransformState().WorldPosition());
            attacked = true;
        }
        if (dashDown && !dashPressed_) {
            const Scene::ShadowActionReport report = shadowbladeActions_.TryDash(
                playerController_.TransformState().WorldPosition());
            if (report.result == Scene::ShadowActionResult::Activated) {
                playerController_.SetPosition(report.dashDestination);
                camera_.Follow(playerController_.TransformState());
            }
            shadowAction = true;
        } else if (fatalStrikeDown && !fatalStrikePressed_) {
            shadowbladeActions_.TryFatalStrike(
                playerController_.TransformState().WorldPosition(), combatSandbox_);
            shadowAction = true;
        }
        lightAttackPressed_ = lightAttackDown;
        heavyAttackPressed_ = heavyAttackDown;
        dashPressed_ = dashDown;
        fatalStrikePressed_ = fatalStrikeDown;
        if (attacked || shadowAction || guardChanged || input.forward || input.backward
            || input.left || input.right) {
            UpdateTitle(window_, frameCount > 0 && fpsAccumulator > 0.0
                    ? static_cast<float>(frameCount / fpsAccumulator) : 0.0f,
                playerController_.TransformState(), combatSandbox_, shadowbladeActions_);
        }

        HDC deviceContext = GetDC(window_);
        RECT viewport{};
        GetClientRect(window_, &viewport);
        g_renderer.Clear(deviceContext, viewport);
        g_renderer.RenderWorld(deviceContext, viewport, camera_, world_,
            playerController_.TransformState(), combatSandbox_, shadowbladeActions_);
        ReleaseDC(window_, deviceContext);
        Sleep(1);
    }

    return static_cast<int>(message.wParam);
}

LRESULT CALLBACK Win32Application::WindowProc(HWND window, UINT message, WPARAM wParam,
    LPARAM lParam) {
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        BeginPaint(window, &paint);
        EndPaint(window, &paint);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(window, message, wParam, lParam);
    }
}

} // namespace Astral::Platform
