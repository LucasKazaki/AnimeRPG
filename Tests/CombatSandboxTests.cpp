#include "Engine/Scene/CombatSandbox.h"

#include <cmath>
#include <cstdlib>
#include <limits>

#ifdef ASTRAL_RUNTIME_SMOKE
#include <windows.h>

#include <iostream>
#include <string>

namespace {
struct WindowSearch {
    DWORD processId{};
    HWND window{};
};

BOOL CALLBACK FindProcessWindow(HWND window, LPARAM parameter) {
    auto& search = *reinterpret_cast<WindowSearch*>(parameter);
    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
    if (processId == search.processId && IsWindowVisible(window)) {
        search.window = window;
        return FALSE;
    }
    return TRUE;
}

HWND WaitForWindow(DWORD processId) {
    for (int attempt = 0; attempt < 100; ++attempt) {
        WindowSearch search{processId, nullptr};
        EnumWindows(FindProcessWindow, reinterpret_cast<LPARAM>(&search));
        if (search.window) return search.window;
        Sleep(50);
    }
    return nullptr;
}

std::wstring WindowTitle(HWND window) {
    wchar_t title[512]{};
    GetWindowTextW(window, title, 512);
    return title;
}

bool SendKey(WORD key) {
    INPUT input{};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = key;
    if (SendInput(1, &input, sizeof(INPUT)) != 1) return false;
    Sleep(100);
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    return SendInput(1, &input, sizeof(INPUT)) == 1;
}

bool WaitForTitle(HWND window, const std::wstring& marker, std::wstring& observed) {
    for (int attempt = 0; attempt < 100; ++attempt) {
        observed = WindowTitle(window);
        if (observed.find(marker) != std::wstring::npos) return true;
        Sleep(50);
    }
    return false;
}
}

int wmain(int argc, wchar_t** argv) {
    if (argc != 3) {
        std::cerr << "usage: M4RuntimeSmoke <AstralGame> <working-directory>\n";
        return 1;
    }

    std::wstring command = L"\"" + std::wstring(argv[1]) + L"\"";
    STARTUPINFOW startup{sizeof(startup)};
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, 0, nullptr, argv[2],
            &startup, &process)) {
        std::cerr << "M4 AUTOMATED NATIVE RUNTIME SMOKE: FAIL (launch)\n";
        return 1;
    }

    bool passed = false;
    HWND window = WaitForWindow(process.dwProcessId);
    std::wstring initialTitle;
    std::wstring lightTitle;
    std::wstring cooldownTitle;
    std::wstring heavyTitle;
    std::wstring defeatedTitle;
    std::wstring postDefeatTitle;
    if (window && WaitForTitle(window, L"Dummy: Alive HP: 100/100", initialTitle)) {
        SetForegroundWindow(window);
        Sleep(100);
        const bool lightHit = SendKey('J')
            && WaitForTitle(window, L"Last: Light Hit -25", lightTitle)
            && lightTitle.find(L"Dummy: Alive HP: 75/100") != std::wstring::npos;
        const bool cooldownRejected = lightHit && SendKey('K')
            && WaitForTitle(window, L"Last: Heavy Cooldown", cooldownTitle)
            && cooldownTitle.find(L"Dummy: Alive HP: 75/100") != std::wstring::npos;
        Sleep(450);
        const bool heavyHit = cooldownRejected && SendKey('K')
            && WaitForTitle(window, L"Last: Heavy Hit -60", heavyTitle)
            && heavyTitle.find(L"Dummy: Alive HP: 15/100") != std::wstring::npos;
        Sleep(1050);
        const bool defeated = heavyHit && SendKey('J')
            && WaitForTitle(window, L"Last: Light Hit -15", defeatedTitle)
            && defeatedTitle.find(L"Dummy: Defeated HP: 0/100") != std::wstring::npos;
        Sleep(100);
        const bool postDefeatRejected = defeated && SendKey('J')
            && WaitForTitle(window, L"Last: Light Already Defeated", postDefeatTitle)
            && postDefeatTitle.find(L"Dummy: Defeated HP: 0/100") != std::wstring::npos;
        if (postDefeatRejected) {
            SendKey(VK_ESCAPE);
            passed = WaitForSingleObject(process.hProcess, 5000) == WAIT_OBJECT_0;
        }
    }

    if (!passed) {
        PostMessageW(window, WM_CLOSE, 0, 0);
        if (WaitForSingleObject(process.hProcess, 2000) != WAIT_OBJECT_0) {
            TerminateProcess(process.hProcess, 2);
            WaitForSingleObject(process.hProcess, 2000);
        }
    }

    DWORD exitCode = 1;
    GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);

    if (!passed || exitCode != 0) {
        std::wcerr << L"M4 AUTOMATED NATIVE RUNTIME SMOKE: FAIL\nInitial: " << initialTitle
                   << L"\nLight: " << lightTitle << L"\nCooldown: " << cooldownTitle
                   << L"\nHeavy: " << heavyTitle << L"\nDefeated: " << defeatedTitle
                   << L"\nPost-defeat: " << postDefeatTitle << L"\nExit: " << exitCode << L"\n";
        return 1;
    }
    std::wcout << L"M4 AUTOMATED NATIVE RUNTIME SMOKE: PASS\nInitial: " << initialTitle
               << L"\nLight: " << lightTitle << L"\nCooldown: " << cooldownTitle
               << L"\nHeavy: " << heavyTitle << L"\nDefeated: " << defeatedTitle
               << L"\nPost-defeat: " << postDefeatTitle
               << L"\nClean Escape exit code: " << exitCode << L"\n";
    return 0;
}

#else

namespace {
bool NearlyEqual(float left, float right) {
    return std::fabs(left - right) < 0.001f;
}

void Check(bool condition) {
    if (!condition) std::abort();
}

void TestComboStaggerAndTrainingMetrics() {
    using namespace Astral::Scene;

    CombatSandbox sandbox;
    Check(sandbox.Stats().totalDamage == 0 && sandbox.Stats().hitCount == 0
        && sandbox.Stats().peakHit == 0 && sandbox.Stats().bestCombo == 0);

    const AttackReport light = sandbox.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f});
    Check(light.result == AttackResult::Hit && light.comboCount == 1);
    Check(sandbox.ComboCount() == 1 && sandbox.Dummy().posture == 25);
    Check(sandbox.Stats().totalDamage == 25 && sandbox.Stats().hitCount == 1
        && sandbox.Stats().peakHit == 25 && sandbox.Stats().bestCombo == 1);

    sandbox.AdvanceTime(0.4f);
    const AttackReport heavy = sandbox.TryAttack(AttackType::Heavy, {0.0f, 0.0f, 0.0f});
    Check(heavy.result == AttackResult::Hit && heavy.comboCount == 2
        && heavy.staggerTriggered);
    Check(sandbox.IsStaggered() && sandbox.Dummy().posture == sandbox.Dummy().maximumPosture);
    Check(sandbox.Stats().totalDamage == 85 && sandbox.Stats().hitCount == 2
        && sandbox.Stats().peakHit == 60 && sandbox.Stats().bestCombo == 2);

    sandbox.AdvanceTime(0.5f);
    Check(sandbox.IsStaggered() && sandbox.StaggerRemaining() > 0.0f);
    Check(sandbox.ConsumeStaggerOpening());
    Check(!sandbox.IsStaggered() && sandbox.Dummy().posture == 0);
    Check(!sandbox.ConsumeStaggerOpening());

    sandbox.ResetTrainingSession();
    Check(sandbox.Dummy().health == sandbox.Dummy().maximumHealth
        && sandbox.Dummy().posture == 0 && sandbox.ComboCount() == 0);
    Check(sandbox.Stats().totalDamage == 0 && sandbox.Stats().hitCount == 0
        && sandbox.Stats().peakHit == 0 && sandbox.Stats().bestCombo == 0);
    Check(NearlyEqual(sandbox.ElapsedSeconds(), 0.0f)
        && NearlyEqual(sandbox.CooldownRemaining(), 0.0f));
}

void TestComboExpiryPostureRecoveryAndInvalidInputs() {
    using namespace Astral::Scene;

    CombatSandbox sandbox;
    sandbox.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f});
    sandbox.AdvanceTime(CombatSandbox::ComboWindowSeconds + 0.01f);
    Check(sandbox.ComboCount() == 0);
    Check(sandbox.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f}).comboCount == 1);

    CombatSandbox recovery;
    recovery.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f});
    recovery.AdvanceTime(CombatSandbox::PostureRecoveryDelaySeconds - 0.01f);
    Check(recovery.Dummy().posture == 25);
    recovery.AdvanceTime(0.04f);
    Check(recovery.Dummy().posture == 24);

    CombatSandbox oneStep;
    CombatSandbox splitSteps;
    oneStep.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f});
    splitSteps.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f});
    oneStep.AdvanceTime(CombatSandbox::PostureRecoveryDelaySeconds + 0.5f);
    splitSteps.AdvanceTime(CombatSandbox::PostureRecoveryDelaySeconds);
    for (int step = 0; step < 50; ++step) splitSteps.AdvanceTime(0.01f);
    Check(oneStep.Dummy().posture == splitSteps.Dummy().posture);

    CombatSandbox exactOneStep;
    CombatSandbox exact30Hz;
    CombatSandbox exact60Hz;
    CombatSandbox exact90Hz;
    exactOneStep.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f});
    exact30Hz.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f});
    exact60Hz.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f});
    exact90Hz.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f});
    exactOneStep.AdvanceTime(CombatSandbox::PostureRecoveryDelaySeconds + 0.2f);
    exact30Hz.AdvanceTime(CombatSandbox::PostureRecoveryDelaySeconds);
    exact60Hz.AdvanceTime(CombatSandbox::PostureRecoveryDelaySeconds);
    exact90Hz.AdvanceTime(CombatSandbox::PostureRecoveryDelaySeconds);
    for (int step = 0; step < 6; ++step) exact30Hz.AdvanceTime(1.0f / 30.0f);
    for (int step = 0; step < 12; ++step) exact60Hz.AdvanceTime(1.0f / 60.0f);
    for (int step = 0; step < 18; ++step) exact90Hz.AdvanceTime(1.0f / 90.0f);
    Check(exactOneStep.Dummy().posture == 18);
    Check(exact30Hz.Dummy().posture == exactOneStep.Dummy().posture);
    Check(exact60Hz.Dummy().posture == exactOneStep.Dummy().posture);
    Check(exact90Hz.Dummy().posture == exactOneStep.Dummy().posture);

    const float elapsed = recovery.ElapsedSeconds();
    const int posture = recovery.Dummy().posture;
    recovery.AdvanceTime(0.0f);
    recovery.AdvanceTime(-1.0f);
    recovery.AdvanceTime(std::numeric_limits<float>::quiet_NaN());
    recovery.AdvanceTime(std::numeric_limits<float>::infinity());
    Check(NearlyEqual(recovery.ElapsedSeconds(), elapsed) && recovery.Dummy().posture == posture);

    const TrainingStats before = recovery.Stats();
    Check(recovery.ApplyDamage(0) == 0 && recovery.ApplyDamage(-5) == 0);
    Check(recovery.Stats().totalDamage == before.totalDamage
        && recovery.Stats().hitCount == before.hitCount
        && recovery.Stats().peakHit == before.peakHit);
}

void TestSplitStableTimingBoundaries() {
    using namespace Astral::Scene;

    CombatSandbox comboOneStep;
    CombatSandbox comboSplit;
    comboOneStep.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f});
    comboSplit.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f});
    comboOneStep.AdvanceTime(CombatSandbox::ComboWindowSeconds);
    for (int step = 0; step < 7; ++step) {
        comboSplit.AdvanceTime(CombatSandbox::ComboWindowSeconds / 7.0f);
    }
    comboOneStep.RegisterSuccessfulAttackHit();
    comboSplit.RegisterSuccessfulAttackHit();
    Check(comboOneStep.ComboCount() == 2 && comboSplit.ComboCount() == 2);

    CombatSandbox staggerOneStep;
    CombatSandbox staggerSplit;
    staggerOneStep.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f});
    staggerSplit.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f});
    staggerOneStep.AdvanceTime(0.4f);
    staggerSplit.AdvanceTime(0.4f);
    Check(staggerOneStep.TryAttack(AttackType::Heavy, {0.0f, 0.0f, 0.0f}).staggerTriggered);
    Check(staggerSplit.TryAttack(AttackType::Heavy, {0.0f, 0.0f, 0.0f}).staggerTriggered);
    staggerOneStep.AdvanceTime(CombatSandbox::StaggerDurationSeconds);
    for (int step = 0; step < 30; ++step) staggerSplit.AdvanceTime(0.05f);
    Check(!staggerOneStep.IsStaggered() && !staggerSplit.IsStaggered());
    Check(staggerOneStep.Dummy().posture == 0 && staggerSplit.Dummy().posture == 0);

    CombatSandbox longRecoveryOneStep;
    CombatSandbox longRecoverySplit;
    longRecoveryOneStep.TryAttack(AttackType::Heavy, {0.0f, 0.0f, 0.0f});
    longRecoverySplit.TryAttack(AttackType::Heavy, {0.0f, 0.0f, 0.0f});
    longRecoveryOneStep.AdvanceTime(CombatSandbox::PostureRecoveryDelaySeconds + 1.4f);
    longRecoverySplit.AdvanceTime(CombatSandbox::PostureRecoveryDelaySeconds);
    for (int step = 0; step < 35; ++step) longRecoverySplit.AdvanceTime(1.0f / 25.0f);
    Check(longRecoveryOneStep.Dummy().posture == 21);
    Check(longRecoverySplit.Dummy().posture == longRecoveryOneStep.Dummy().posture);

    CombatSandbox cooldownOneStep;
    CombatSandbox cooldownSplit;
    cooldownOneStep.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f});
    cooldownSplit.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f});
    cooldownOneStep.AdvanceTime(0.4f);
    for (int step = 0; step < 4; ++step) cooldownSplit.AdvanceTime(0.1f);
    Check(cooldownOneStep.TryAttack(AttackType::Heavy, {0.0f, 0.0f, 0.0f}).result
        == AttackResult::Hit);
    Check(cooldownSplit.TryAttack(AttackType::Heavy, {0.0f, 0.0f, 0.0f}).result
        == AttackResult::Hit);
}

void TestProfileResistanceAndStaggerVulnerability() {
    using namespace Astral::Scene;

    CombatSandbox standard;
    const AttackReport standardLight = standard.TryAttack(AttackType::Light, {});
    Check(standardLight.damageApplied == 25 && !standardLight.resistanceApplied
        && !standardLight.staggerBonusApplied);

    CombatSandbox vanguard;
    Check(vanguard.SetTrainingEnemyProfile(TrainingEnemyProfile::Vanguard));
    Check(vanguard.CurrentEnemyDefinition().resistance == AttackResistance::Heavy);
    const AttackReport resistedHeavy = vanguard.TryAttack(AttackType::Heavy, {});
    Check(resistedHeavy.result == AttackResult::Hit && resistedHeavy.damageApplied == 48
        && resistedHeavy.resistanceApplied && !resistedHeavy.staggerBonusApplied
        && resistedHeavy.staggerTriggered && vanguard.IsStaggered());
    vanguard.AdvanceTime(1.0f);
    const AttackReport staggeredLight = vanguard.TryAttack(AttackType::Light, {});
    Check(staggeredLight.result == AttackResult::Hit && staggeredLight.damageApplied == 31
        && !staggeredLight.resistanceApplied && staggeredLight.staggerBonusApplied
        && vanguard.Dummy().health == 11);

    CombatSandbox boss;
    Check(boss.SetTrainingEnemyProfile(TrainingEnemyProfile::Boss));
    Check(boss.CurrentEnemyDefinition().resistance == AttackResistance::Light);
    const AttackReport resistedLight = boss.TryAttack(AttackType::Light, {});
    Check(resistedLight.damageApplied == 20 && resistedLight.resistanceApplied
        && !resistedLight.staggerBonusApplied);
}

void TestTechniqueVarietyChain() {
    using namespace Astral::Scene;

    CombatSandbox repeated;
    repeated.ApplyManaAffinity(ManaAffinity::Solar);
    repeated.ApplyManaAffinity(ManaAffinity::Umbral);
    Check(repeated.TechniqueChain() == 1
        && repeated.LastTechniqueType() == TechniqueType::Reaction
        && repeated.Stats().techniqueScore == CombatSandbox::ReactionTechniquePoints);
    repeated.ApplyManaAffinity(ManaAffinity::Solar);
    repeated.ApplyManaAffinity(ManaAffinity::Umbral);
    Check(repeated.TechniqueChain() == 1
        && repeated.Stats().bestTechniqueChain == 1
        && repeated.Stats().techniqueScore == CombatSandbox::ReactionTechniquePoints * 2);

    CombatSandbox varied;
    varied.SetTrainingEnemyProfile(TrainingEnemyProfile::Bulwark);
    varied.ApplyManaAffinity(ManaAffinity::Solar);
    varied.ApplyManaAffinity(ManaAffinity::Umbral);
    varied.TryAttack(AttackType::Heavy, {});
    varied.AdvanceTime(1.0f);
    const AttackReport stagger = varied.TryAttack(AttackType::Heavy, {});
    Check(stagger.staggerTriggered && varied.TechniqueChain() == 2
        && varied.LastTechniqueType() == TechniqueType::Stagger
        && varied.Stats().bestTechniqueChain == 2);
    Check(varied.Stats().techniqueScore
        == CombatSandbox::ReactionTechniquePoints
            + CombatSandbox::StaggerTechniquePoints * 2);
    varied.AdvanceTime(CombatSandbox::TechniqueChainWindowSeconds + 0.01f);
    Check(varied.TechniqueChain() == 0
        && varied.LastTechniqueType() == TechniqueType::None
        && varied.Stats().bestTechniqueChain == 2);
}

void TestBossPhaseWeaknessAndEndlessPractice() {
    using namespace Astral::Scene;

    CombatSandbox boss;
    boss.SetCombatAssistPreset(CombatAssistPreset::Accessible);
    Check(boss.SetTrainingEnemyProfile(TrainingEnemyProfile::Boss));
    TrainingEnemyDefinition definition = boss.CurrentEnemyDefinition();
    Check(boss.Dummy().maximumHealth == 320 && boss.Dummy().maximumPosture == 160
        && definition.weakness == ManaAffinity::Solar
        && boss.CurrentEnemyPhase() == EnemyPhase::Normal);

    boss.ApplyManaAffinity(ManaAffinity::Umbral);
    const ManaReactionReport firstPhase = boss.ApplyManaAffinity(ManaAffinity::Solar);
    Check(firstPhase.weaknessExploited && firstPhase.bonusDamage == 30
        && boss.Dummy().health == 290);
    Check(boss.ApplyDamage(129) == 129 && boss.Dummy().health == 161
        && boss.CurrentEnemyPhase() == EnemyPhase::Normal);
    Check(boss.ApplyDamage(1) == 1 && boss.Dummy().health == 160
        && boss.CurrentEnemyPhase() == EnemyPhase::Pressure
        && boss.CurrentEnemyDefinition().weakness == ManaAffinity::Umbral);
    boss.ApplyManaAffinity(ManaAffinity::Solar);
    const ManaReactionReport secondPhase = boss.ApplyManaAffinity(ManaAffinity::Umbral);
    Check(secondPhase.weaknessExploited && secondPhase.bonusDamage == 30);

    boss.ResetTrainingSession();
    Check(boss.Dummy().health == 320 && boss.CurrentEnemyPhase() == EnemyPhase::Normal
        && boss.CurrentEnemyDefinition().weakness == ManaAffinity::Solar
        && boss.AssistPreset() == CombatAssistPreset::Accessible);

    Check(boss.SetTrainingTargetMode(TrainingTargetMode::Endless));
    Check(boss.TargetMode() == TrainingTargetMode::Endless
        && boss.Dummy().health == 320 && boss.Stats().totalDamage == 0
        && boss.AssistPreset() == CombatAssistPreset::Accessible);
    Check(boss.ApplyDamage(999) == 999 && boss.Dummy().health == 320
        && !boss.Dummy().IsDefeated() && boss.Stats().totalDamage == 999
        && boss.Stats().peakHit == 999);
    Check(!boss.SetTrainingTargetMode(TrainingTargetMode::Endless)
        && boss.Stats().totalDamage == 999);
    Check(boss.SetTrainingTargetMode(TrainingTargetMode::Standard)
        && boss.TargetMode() == TrainingTargetMode::Standard
        && boss.EnemyProfile() == TrainingEnemyProfile::Boss
        && boss.Dummy().health == 320 && boss.Stats().totalDamage == 0
        && boss.AssistPreset() == CombatAssistPreset::Accessible);
}

void TestEnemyAttackPatternsAggressionBossPracticeAndPunish() {
    using namespace Astral::Scene;

    CombatSandbox standard;
    Check(standard.QueueNextEnemyAttack());
    const EnemyAttackPlan standardPlan = standard.PendingEnemyAttack();
    Check(standardPlan.pattern == EnemyAttackPattern::QuickCut
        && NearlyEqual(standardPlan.windupSeconds, 0.55f)
        && standardPlan.damage == 18 && standardPlan.guardDamage == 20
        && standardPlan.blockable);
    Check(!standard.QueueNextEnemyAttack());
    Check(standard.ResolveEnemyAttack(EnemyAttackOutcome::Hit));
    Check(!standard.ResolveEnemyAttack(EnemyAttackOutcome::Hit));
    Check(!standard.QueueNextEnemyAttack());
    standard.AdvanceTime(standardPlan.recoverySeconds);
    Check(standard.QueueNextEnemyAttack());

    CombatSandbox aggression;
    Check(aggression.SetEnemyAggressionPreset(EnemyAggressionPreset::Relaxed));
    Check(aggression.QueueNextEnemyAttack());
    const EnemyAttackPlan relaxedPlan = aggression.PendingEnemyAttack();
    Check(NearlyEqual(relaxedPlan.recoverySeconds,
        CombatSandbox::RelaxedEnemyRecoverySeconds));
    Check(aggression.SetEnemyAggressionPreset(EnemyAggressionPreset::Aggressive));
    Check(NearlyEqual(aggression.PendingEnemyAttack().recoverySeconds,
        CombatSandbox::RelaxedEnemyRecoverySeconds));
    Check(aggression.ResolveEnemyAttack(EnemyAttackOutcome::Guarded));
    aggression.AdvanceTime(relaxedPlan.recoverySeconds);
    Check(aggression.QueueNextEnemyAttack());
    Check(NearlyEqual(aggression.PendingEnemyAttack().recoverySeconds,
        CombatSandbox::AggressiveEnemyRecoverySeconds));
    Check(!aggression.SetEnemyAggressionPreset(
        static_cast<EnemyAggressionPreset>(99)));

    CombatSandbox boss;
    Check(boss.SetTrainingEnemyProfile(TrainingEnemyProfile::Boss));
    Check(boss.QueueNextEnemyAttack());
    const EnemyAttackPlan normalFirst = boss.PendingEnemyAttack();
    Check(normalFirst.pattern == EnemyAttackPattern::QuickCut);
    Check(boss.ResolveEnemyAttack(EnemyAttackOutcome::Evaded));
    boss.AdvanceTime(normalFirst.recoverySeconds);
    Check(boss.QueueNextEnemyAttack());
    Check(boss.PendingEnemyAttack().pattern == EnemyAttackPattern::GuardBreaker);

    Check(boss.SetBossPracticePhase(EnemyPhase::Pressure));
    Check(boss.BossPracticePhaseLocked()
        && boss.CurrentEnemyPhase() == EnemyPhase::Pressure
        && boss.Dummy().health == boss.Dummy().maximumHealth
        && boss.CurrentEnemyDefinition().weakness == ManaAffinity::Umbral);
    Check(boss.QueueNextEnemyAttack());
    const EnemyAttackPlan pressureFirst = boss.PendingEnemyAttack();
    Check(pressureFirst.pattern == EnemyAttackPattern::QuickCut);
    Check(boss.ResolveEnemyAttack(EnemyAttackOutcome::Evaded));
    boss.AdvanceTime(pressureFirst.recoverySeconds);
    Check(boss.QueueNextEnemyAttack());
    Check(boss.PendingEnemyAttack().pattern == EnemyAttackPattern::RiftBurst
        && !boss.PendingEnemyAttack().blockable);
    boss.ResetTrainingSession();
    Check(boss.CurrentEnemyPhase() == EnemyPhase::Pressure
        && boss.BossPracticePhaseLocked());
    Check(!boss.SetBossPracticePhase(static_cast<EnemyPhase>(99)));
    Check(boss.ClearBossPracticePhase()
        && boss.CurrentEnemyPhase() == EnemyPhase::Normal
        && !boss.BossPracticePhaseLocked());

    CombatSandbox bulwark;
    Check(bulwark.SetTrainingEnemyProfile(TrainingEnemyProfile::Bulwark));
    Check(bulwark.QueueNextEnemyAttack() && bulwark.HasPendingEnemyAttack());
    const AttackReport firstHeavy = bulwark.TryAttack(AttackType::Heavy, {});
    Check(firstHeavy.result == AttackResult::Hit && !firstHeavy.staggerTriggered);
    bulwark.AdvanceTime(1.0f);
    const AttackReport secondHeavy = bulwark.TryAttack(AttackType::Heavy, {});
    Check(secondHeavy.staggerTriggered && bulwark.IsStaggered()
        && !bulwark.HasPendingEnemyAttack());
    Check(!bulwark.QueueNextEnemyAttack());
    bulwark.AdvanceTime(CombatSandbox::StaggerDurationSeconds);
    Check(!bulwark.IsStaggered() && bulwark.QueueNextEnemyAttack());

    CombatSandbox punish;
    Check(punish.QueueNextEnemyAttack());
    Check(punish.ResolveEnemyAttack(EnemyAttackOutcome::PerfectDefense)
        && punish.DefensePunishOpeningReady());
    const AttackReport miss = punish.TryAttack(AttackType::Light, {-10.0f, 0.0f, 0.0f});
    Check(miss.result == AttackResult::OutOfRange && punish.DefensePunishOpeningReady());
    const AttackReport punishHit = punish.TryAttack(AttackType::Light, {});
    Check(punishHit.result == AttackResult::Hit && punishHit.defensePunishBonusApplied
        && punishHit.damageApplied == 31 && !punish.DefensePunishOpeningReady());
    punish.AdvanceTime(0.4f);
    const AttackReport ordinary = punish.TryAttack(AttackType::Light, {});
    Check(ordinary.result == AttackResult::Hit && !ordinary.defensePunishBonusApplied
        && ordinary.damageApplied == 25);

    CombatSandbox expired;
    Check(expired.QueueNextEnemyAttack());
    Check(expired.ResolveEnemyAttack(EnemyAttackOutcome::PerfectDefense));
    expired.AdvanceTime(CombatSandbox::DefensePunishWindowSeconds + 0.01f);
    Check(!expired.DefensePunishOpeningReady());
    const AttackReport afterExpiry = expired.TryAttack(AttackType::Light, {});
    Check(afterExpiry.damageApplied == 25 && !afterExpiry.defensePunishBonusApplied);

    CombatSandbox invalid;
    Check(!invalid.SetBossPracticePhase(EnemyPhase::Pressure));
    Check(!invalid.ClearBossPracticePhase());
    Check(invalid.QueueNextEnemyAttack());
    Check(!invalid.ResolveEnemyAttack(static_cast<EnemyAttackOutcome>(99))
        && invalid.HasPendingEnemyAttack());
    Check(invalid.ResolveEnemyAttack(EnemyAttackOutcome::Hit));
}
}

int main() {
    using namespace Astral::Scene;

    CombatSandbox lightSandbox;
    AttackReport report = lightSandbox.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f});
    Check(report.result == AttackResult::Hit);
    Check(report.damageApplied == 25);
    Check(lightSandbox.Dummy().health == 75);
    Check(NearlyEqual(lightSandbox.CooldownRemaining(), 0.4f));

    CombatSandbox heavySandbox;
    report = heavySandbox.TryAttack(AttackType::Heavy, {0.0f, 0.0f, 0.0f});
    Check(report.result == AttackResult::Hit);
    Check(report.damageApplied == 60);
    Check(heavySandbox.Dummy().health == 40);
    Check(NearlyEqual(heavySandbox.CooldownRemaining(), 1.0f));

    CombatSandbox rangeSandbox;
    report = rangeSandbox.TryAttack(AttackType::Light, {-1.0f, 0.0f, 0.0f});
    Check(report.result == AttackResult::OutOfRange);
    Check(rangeSandbox.Dummy().health == 100);

    CombatSandbox cooldownSandbox;
    cooldownSandbox.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f});
    report = cooldownSandbox.TryAttack(AttackType::Heavy, {0.0f, 0.0f, 0.0f});
    Check(report.result == AttackResult::Cooldown);
    Check(cooldownSandbox.Dummy().health == 75);
    cooldownSandbox.AdvanceTime(0.4f);
    report = cooldownSandbox.TryAttack(AttackType::Heavy, {0.0f, 0.0f, 0.0f});
    Check(report.result == AttackResult::Hit);
    Check(cooldownSandbox.Dummy().health == 15);
    cooldownSandbox.AdvanceTime(0.99f);
    report = cooldownSandbox.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f});
    Check(report.result == AttackResult::Cooldown);
    cooldownSandbox.AdvanceTime(0.01f);
    Check(cooldownSandbox.TryAttack(AttackType::Light, {0.0f, 0.0f, 0.0f}).result
        == AttackResult::Hit);
    Check(cooldownSandbox.Dummy().IsDefeated());
    Check(cooldownSandbox.Dummy().health == 0);
    cooldownSandbox.AdvanceTime(10.0f);
    report = cooldownSandbox.TryAttack(AttackType::Heavy, {0.0f, 0.0f, 0.0f});
    Check(report.result == AttackResult::TargetDefeated);
    Check(report.damageApplied == 0);
    Check(cooldownSandbox.Dummy().health == 0);

    TestComboStaggerAndTrainingMetrics();
    TestComboExpiryPostureRecoveryAndInvalidInputs();
    TestSplitStableTimingBoundaries();
    TestProfileResistanceAndStaggerVulnerability();
    TestTechniqueVarietyChain();
    TestBossPhaseWeaknessAndEndlessPractice();
    TestEnemyAttackPatternsAggressionBossPracticeAndPunish();
    return 0;
}
#endif
