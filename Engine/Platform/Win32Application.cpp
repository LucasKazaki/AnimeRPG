#include "Engine/Platform/Win32Application.h"

#include "Engine/Core/Clock.h"
#include "Engine/Core/FramePhaseTimingCapture.h"
#include "Engine/Core/Logger.h"
#include "Engine/Renderer/Renderer.h"

#include <chrono>
#include <cstdio>
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

const wchar_t* CommandReasonName(Astral::Scene::ThoughtCommandReason reason) {
    switch (reason) {
    case Astral::Scene::ThoughtCommandReason::None: return L"None";
    case Astral::Scene::ThoughtCommandReason::Empty: return L"Empty";
    case Astral::Scene::ThoughtCommandReason::Ambiguous: return L"Ambiguous";
    case Astral::Scene::ThoughtCommandReason::Unsupported: return L"Unsupported";
    case Astral::Scene::ThoughtCommandReason::NoOp: return L"No Op";
    case Astral::Scene::ThoughtCommandReason::GuardedConflict: return L"Blocked by Guard";
    case Astral::Scene::ThoughtCommandReason::Cooldown: return L"Cooldown";
    case Astral::Scene::ThoughtCommandReason::InsufficientResource: return L"No Resource";
    case Astral::Scene::ThoughtCommandReason::OutOfRange: return L"Out of Range";
    case Astral::Scene::ThoughtCommandReason::TargetDefeated: return L"Target Defeated";
    }
    return L"Unknown";
}

const wchar_t* LandmarkName(Astral::Scene::LandmarkKind kind) {
    switch (kind) {
    case Astral::Scene::LandmarkKind::LincolnMemorial: return L"Lincoln";
    case Astral::Scene::LandmarkKind::ReflectingPool: return L"Pool";
    case Astral::Scene::LandmarkKind::WashingtonMonument: return L"Monument";
    }
    return L"None";
}

const wchar_t* InteractionResultName(Astral::Scene::LandmarkInteractionResult result) {
    switch (result) {
    case Astral::Scene::LandmarkInteractionResult::None: return L"None";
    case Astral::Scene::LandmarkInteractionResult::OutOfRange: return L"Out of Range";
    case Astral::Scene::LandmarkInteractionResult::Discovered: return L"Discovered";
    case Astral::Scene::LandmarkInteractionResult::AlreadyVisited: return L"Already Visited";
    }
    return L"Unknown";
}

const wchar_t* EncounterStateName(Astral::Scene::LandmarkEncounterState state) {
    switch (state) {
    case Astral::Scene::LandmarkEncounterState::Locked: return L"Locked";
    case Astral::Scene::LandmarkEncounterState::Active: return L"Active";
    case Astral::Scene::LandmarkEncounterState::Completed: return L"Completed";
    }
    return L"Unknown";
}

std::wstring WidenAscii(const std::string& text) {
    return std::wstring(text.begin(), text.end());
}

void UpdateTitle(HWND window, float fps, const Astral::Scene::Transform& state,
    const Astral::Scene::CombatSandbox& combatSandbox,
    const Astral::Scene::ShadowbladeActions& shadowbladeActions,
    const Astral::Scene::ThoughtCommands& thoughtCommands,
    const Astral::Scene::LandmarkInteraction& landmarkInteraction,
    const Astral::Scene::LandmarkEncounter& landmarkEncounter) {
    const Astral::Math::Vec3 position = state.WorldPosition();
    const Astral::Scene::TrainingDummy& dummy = combatSandbox.Dummy();
    const Astral::Scene::AttackReport& attack = combatSandbox.LastAttack();
    const Astral::Scene::ShadowActionReport& shadow = shadowbladeActions.LastAction();
    const Astral::Scene::ThoughtCommandReport& command = thoughtCommands.LastReport();
    const std::wstring submitted = command.submitted.empty()
        ? L"none" : WidenAscii(command.submitted);
    const std::wstring commandOutcome = command.status
            == Astral::Scene::ThoughtCommandStatus::Accepted
        ? L"ACCEPTED" : L"REJECTED: " + std::wstring(CommandReasonName(command.reason));
    const Astral::Scene::LandmarkInteractionReport& interaction = landmarkInteraction.LastReport();
    const std::wstring selected = landmarkInteraction.HasSelection()
        ? LandmarkName(landmarkInteraction.SelectedKind()) : L"None";
    const std::wstring reward = interaction.rewardApplied > 0.0f
        ? L" Reward Shadow Restored" : L"";
    const std::wstring title = L"Astral | M10 Landmark Encounter | Encounter: "
        + std::wstring(EncounterStateName(landmarkEncounter.State()))
        + L" | Encounter Reward: "
        + std::to_wstring(static_cast<int>(landmarkEncounter.LastReport().rewardApplied))
        + L" | M5 Perspective Mall | Pos: (" + std::to_wstring(position.x) + L", "
        + std::to_wstring(position.y) + L") | Dummy: "
        + std::wstring(dummy.IsDefeated() ? L"Defeated" : L"Alive") + L" HP: "
        + std::to_wstring(dummy.health) + L"/" + std::to_wstring(dummy.maximumHealth)
        + L" | Last: " + AttackName(attack.type) + L" " + ResultName(attack.result)
        + (attack.damageApplied > 0 ? L" -" + std::to_wstring(attack.damageApplied) : L"")
        + L" | M7 Shadowblade | Shadow: "
        + std::to_wstring(static_cast<int>(shadowbladeActions.Resource())) + L"/100 | Guard: "
        + (shadowbladeActions.IsGuarding() ? L"ON" : L"OFF")
        + L" | Shadow Last: " + ShadowActionName(shadow.type) + L" "
        + ShadowResultName(shadow.result)
        + (shadow.damageApplied > 0 ? L" -" + std::to_wstring(shadow.damageApplied) : L"")
        + L" | M8 Thought Commands | Command: " + submitted + L" | " + commandOutcome
        + L" | Focus: " + (thoughtCommands.IsFocusActive() ? L"ON x0.35" : L"OFF x1.00")
        + L" | M9 Landmark Interaction | Selected: " + selected + L" | Visited: "
        + std::to_wstring(landmarkInteraction.VisitedCount())
        + L"/3 | Interaction Last: " + InteractionResultName(interaction.result) + reward;
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
    landmarkInteraction_.UpdateSelection(playerController_.TransformState().WorldPosition(), world_);
    camera_.Follow(playerController_.TransformState());
    UpdateTitle(window_, 0.0f, playerController_.TransformState(), combatSandbox_,
        shadowbladeActions_, thoughtCommands_, landmarkInteraction_, landmarkEncounter_);
    g_logger.Info("Window created; M10 landmark encounter active; E discovers and Escape exits");
    return true;
}

int Win32Application::Run() {
    Astral::Core::Clock clock;
    Astral::Core::FramePhaseTimingCapture phaseTimingCapture;
    std::string phaseTimingError;
    const auto phaseTimingStatus = phaseTimingCapture.ConfigureFromEnvironment(phaseTimingError);
    if (phaseTimingStatus == Astral::Core::FramePhaseTimingEnvironmentStatus::Invalid) {
        std::fprintf(stderr, "Astral frame phase timing capture configuration rejected: %s\n",
            phaseTimingError.c_str());
    }

    MSG message{};
    double fpsAccumulator = 0.0;
    int frameCount = 0;
    std::uint64_t phaseFrameIndex = 0;
    using PhaseClock = std::chrono::steady_clock;
    const auto toMilliseconds = [](PhaseClock::duration duration) {
        return std::chrono::duration<double, std::milli>(duration).count();
    };

    while (message.message != WM_QUIT) {
        PhaseClock::time_point phaseStart{};
        PhaseClock::time_point afterMessages{};
        PhaseClock::time_point afterUpdate{};
        PhaseClock::time_point afterRender{};
        if (phaseTimingCapture.Enabled()) {
            phaseStart = PhaseClock::now();
        }

        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        if (phaseTimingCapture.Enabled()) {
            afterMessages = PhaseClock::now();
        }

        const float deltaSeconds = clock.Tick();
        fpsAccumulator += deltaSeconds;
        ++frameCount;
        if (fpsAccumulator >= 1.0) {
            UpdateTitle(window_, static_cast<float>(frameCount / fpsAccumulator),
                playerController_.TransformState(), combatSandbox_, shadowbladeActions_,
                thoughtCommands_, landmarkInteraction_, landmarkEncounter_);
            g_logger.Info("Frame timing active");
            fpsAccumulator = 0.0;
            frameCount = 0;
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            PostMessageW(window_, WM_CLOSE, 0, 0);
        }

        const float simulationDelta = thoughtCommands_.ScaleDelta(deltaSeconds);
        const Scene::MovementInput input{
            (GetAsyncKeyState('W') & 0x8000) != 0,
            (GetAsyncKeyState('S') & 0x8000) != 0,
            (GetAsyncKeyState('A') & 0x8000) != 0,
            (GetAsyncKeyState('D') & 0x8000) != 0,
        };
        playerController_.Update(input, simulationDelta);
        camera_.Follow(playerController_.TransformState());
        combatSandbox_.AdvanceTime(simulationDelta);
        shadowbladeActions_.AdvanceTime(simulationDelta);

        const bool physicalGuarding = (GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0;
        const bool effectiveGuarding = physicalGuarding
            || thoughtCommands_.IsCommandGuardActive();
        const bool guardChanged = effectiveGuarding != shadowbladeActions_.IsGuarding();
        thoughtCommands_.ApplyGuardState(physicalGuarding, shadowbladeActions_);
        const bool guarding = shadowbladeActions_.IsGuarding();

        const bool lightAttackDown = (GetAsyncKeyState('J') & 0x8000) != 0;
        const bool heavyAttackDown = (GetAsyncKeyState('K') & 0x8000) != 0;
        const bool dashDown = (GetAsyncKeyState('Q') & 0x8000) != 0;
        const bool fatalStrikeDown = (GetAsyncKeyState('L') & 0x8000) != 0;
        const bool interactDown = (GetAsyncKeyState('E') & 0x8000) != 0;
        bool commandDown[6]{};
        for (int index = 0; index < 6; ++index) {
            commandDown[index] = (GetAsyncKeyState('0' + index) & 0x8000) != 0;
        }
        bool attacked = false;
        bool shadowAction = false;
        bool thoughtCommand = false;
        bool interacted = false;
        bool encounterChanged = false;
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
        const char* commandInputs[6]{"unsupported", "dash", "fatal", "guard on",
            "guard off", "focus"};
        for (int index = 0; index < 6; ++index) {
            if (commandDown[index] && !commandPressed_[index]) {
                const Scene::ThoughtCommandReport report = thoughtCommands_.Submit(
                    commandInputs[index], playerController_.TransformState().WorldPosition(),
                    shadowbladeActions_, combatSandbox_);
                if (report.type == Scene::ThoughtCommandType::Dash
                    && report.status == Scene::ThoughtCommandStatus::Accepted) {
                    playerController_.SetPosition(report.shadowAction.dashDestination);
                    camera_.Follow(playerController_.TransformState());
                }
                thoughtCommands_.ApplyGuardState(physicalGuarding, shadowbladeActions_);
                thoughtCommand = true;
                break;
            }
        }
        const bool selectionChanged = landmarkInteraction_.UpdateSelection(
            playerController_.TransformState().WorldPosition(), world_);
        if (interactDown && !interactPressed_) {
            const Scene::LandmarkInteractionReport report = landmarkInteraction_.TryInteract(
                playerController_.TransformState().WorldPosition(),
                world_, shadowbladeActions_);
            if (report.result == Scene::LandmarkInteractionResult::Discovered) {
                encounterChanged = landmarkEncounter_.TryActivate(report, combatSandbox_).result
                    == Scene::LandmarkEncounterResult::Activated;
            }
            interacted = true;
        }
        encounterChanged = landmarkEncounter_.Update(combatSandbox_, shadowbladeActions_)
            || encounterChanged;
        lightAttackPressed_ = lightAttackDown;
        heavyAttackPressed_ = heavyAttackDown;
        dashPressed_ = dashDown;
        fatalStrikePressed_ = fatalStrikeDown;
        interactPressed_ = interactDown;
        for (int index = 0; index < 6; ++index) commandPressed_[index] = commandDown[index];
        if (attacked || shadowAction || thoughtCommand || interacted || selectionChanged
            || encounterChanged || guardChanged || input.forward || input.backward
            || input.left || input.right) {
            UpdateTitle(window_, frameCount > 0 && fpsAccumulator > 0.0
                    ? static_cast<float>(frameCount / fpsAccumulator) : 0.0f,
                playerController_.TransformState(), combatSandbox_, shadowbladeActions_,
                thoughtCommands_, landmarkInteraction_, landmarkEncounter_);
        }
        if (phaseTimingCapture.Enabled()) {
            afterUpdate = PhaseClock::now();
        }

        HDC deviceContext = GetDC(window_);
        RECT viewport{};
        GetClientRect(window_, &viewport);
        g_renderer.Clear(deviceContext, viewport);
        g_renderer.RenderWorld(deviceContext, viewport, camera_, world_,
            playerController_.TransformState(), combatSandbox_, shadowbladeActions_,
            thoughtCommands_, landmarkInteraction_, landmarkEncounter_);
        ReleaseDC(window_, deviceContext);
        if (phaseTimingCapture.Enabled()) {
            afterRender = PhaseClock::now();
        }
        Sleep(1);

        if (phaseTimingCapture.Enabled()) {
            const auto afterWait = PhaseClock::now();
            phaseTimingCapture.Record(phaseFrameIndex,
                toMilliseconds(afterMessages - phaseStart),
                toMilliseconds(afterUpdate - afterMessages),
                toMilliseconds(afterRender - afterUpdate),
                toMilliseconds(afterWait - afterRender));
        }
        ++phaseFrameIndex;
    }

    if (phaseTimingCapture.Enabled()) {
        std::string error;
        if (!phaseTimingCapture.Flush(error)) {
            std::fprintf(stderr, "Astral frame phase timing capture was not published: %s\n",
                error.c_str());
        }
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
