#pragma once

#include "Engine/Scene/CombatSandbox.h"
#include "Engine/Scene/ShadowbladeActions.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace Astral::Scene {

enum class DefenseTrainingCuePhase {
    None,
    Approach,
    DodgeWindow,
    PerfectWindow,
};

enum class DefenseTrainingCueSymbol {
    None,
    Observe,
    Blockable,
    Unblockable,
    PerfectTiming,
};

enum class DefenseTrainingGrade {
    None,
    Bronze,
    Silver,
    Gold,
};

enum class DefenseTrainingGoal {
    FreePractice,
    PerfectDefense,
    GuardDiscipline,
    DodgeDiscipline,
};

enum class DefenseTrainingControlReminder {
    None,
    Guard,
    Dodge,
};

enum class DefenseTrainingTutorialHint {
    None,
    ReadTelegraph,
    GuardBlockable,
    DodgeUnblockable,
    AimPerfectTiming,
};

struct DefenseTrainingCue {
    DefenseTrainingCuePhase phase{DefenseTrainingCuePhase::None};
    DefenseTrainingCueSymbol symbol{DefenseTrainingCueSymbol::None};
    EnemyAttackPattern pattern{EnemyAttackPattern::QuickCut};
    float secondsToImpact{};
    bool blockable{};
};

struct DefenseTrainingStats {
    int attacksQueued{};
    int defenseInputs{};
    int perfectDefenses{};
    int ordinaryDefenses{};
    int guardDefenses{};
    int dodgeDefenses{};
    int hitsTaken{};
    int interruptions{};
    int damageTaken{};
    int currentPerfectStreak{};
    int bestPerfectStreak{};
    int consecutiveHits{};
};

struct DefenseTrainingGoalStatus {
    DefenseTrainingGoal goal{DefenseTrainingGoal::FreePractice};
    int current{};
    int target{};
    bool complete{};
};

class CombatDefenseTraining {
public:
    static constexpr int GoalTarget = 3;

    bool QueueNextAttack(CombatSandbox& combat, ShadowbladeActions& actions) {
        if (!CanQueue(combat, actions) || !combat.QueueNextEnemyAttack()) return false;
        return LinkQueuedAttack(combat, actions);
    }

    bool QueueAttackPattern(CombatSandbox& combat, ShadowbladeActions& actions,
        EnemyAttackPattern pattern) {
        if (!CanQueue(combat, actions) || !combat.QueueEnemyAttackPattern(pattern)) return false;
        return LinkQueuedAttack(combat, actions);
    }

    DefenseReport TryDefend(CombatSandbox& combat, ShadowbladeActions& actions,
        DefenseInput input) {
        if (paused_) {
            return {DefenseResult::Paused, 0, 0, false,
                actions.HasIncomingAttack() ? actions.IncomingAttackRemaining() : 0.0f};
        }
        if (!linkedAttackActive_) return actions.TryDefend(input);
        if (!OwnsObjects(combat, actions)) return NoLinkedThreatReport();

        if (!OwnsCombatPlan(combat)) {
            if (OwnsGeneration(actions) && actions.HasIncomingAttack()) {
                actions.CancelIncomingAttack();
            }
            InterruptLinked(combat, false);
            return NoLinkedThreatReport();
        }

        if (!OwnsGeneration(actions)) {
            InterruptLinked(combat, true);
            return NoLinkedThreatReport();
        }

        if (!actions.HasIncomingAttack()) {
            const DefenseReport report = actions.LastDefense();
            if (!ResolveTerminalReport(combat, report)) {
                InterruptLinked(combat, true);
            }
            return report;
        }

        const DefenseReport report = actions.TryDefend(input);
        if (report.result != DefenseResult::NoThreat
            && report.result != DefenseResult::InvalidThreat
            && report.result != DefenseResult::ThreatQueued
            && report.result != DefenseResult::Paused) {
            SaturatingIncrement(stats_.defenseInputs);
        }

        switch (report.result) {
        case DefenseResult::TooEarly:
            return report;
        case DefenseResult::PerfectGuard:
            ResolveLinked(combat, EnemyAttackOutcome::PerfectDefense, report,
                ResolutionKind::Perfect, DefenseKind::Guard);
            return report;
        case DefenseResult::PerfectDodge:
            ResolveLinked(combat, EnemyAttackOutcome::PerfectDefense, report,
                ResolutionKind::Perfect, DefenseKind::Dodge);
            return report;
        case DefenseResult::Guarded:
            ResolveLinked(combat, EnemyAttackOutcome::Guarded, report,
                ResolutionKind::OrdinaryDefense, DefenseKind::Guard);
            return report;
        case DefenseResult::Evaded:
            ResolveLinked(combat, EnemyAttackOutcome::Evaded, report,
                ResolutionKind::OrdinaryDefense, DefenseKind::Dodge);
            return report;
        case DefenseResult::GuardBroken:
        case DefenseResult::UnblockableHit:
        case DefenseResult::Hit:
            ResolveLinked(combat, EnemyAttackOutcome::Hit, report,
                ResolutionKind::Hit, DefenseKind::None);
            return report;
        case DefenseResult::None:
        case DefenseResult::ThreatQueued:
        case DefenseResult::NoThreat:
        case DefenseResult::InvalidThreat:
        case DefenseResult::Paused:
        default:
            return report;
        }
    }

    bool AdvanceTime(CombatSandbox& combat, ShadowbladeActions& actions,
        float deltaSeconds) {
        // Preserve both underlying subsystems' no-op semantics for invalid or
        // nonpositive time. Reconciliation waits for the next valid tick.
        if (!(deltaSeconds > 0.0f) || !std::isfinite(deltaSeconds)) return false;
        if (paused_) return false;

        if (!linkedAttackActive_) {
            combat.AdvanceTime(deltaSeconds);
            actions.AdvanceTime(deltaSeconds);
            return false;
        }
        if (!OwnsObjects(combat, actions)) return false;

        // Reconcile an authoritative interruption or replacement before
        // advancing the linked Shadowblade clock. Planner-event identity keeps a
        // reset/requeued attack from being consumed as this coordinator's event.
        if (!OwnsCombatPlan(combat)) {
            if (OwnsGeneration(actions) && actions.HasIncomingAttack()) {
                actions.CancelIncomingAttack();
            }
            const bool interrupted = InterruptLinked(combat, false);
            // This was still a valid frame. Once the stale owned threat is
            // canceled, forward the full delta through the ordinary subsystem
            // clocks so stagger recovery, cooldowns, resource regeneration, and
            // any replacement planner event do not lose time.
            combat.AdvanceTime(deltaSeconds);
            actions.AdvanceTime(deltaSeconds);
            return interrupted;
        }

        // If the owned threat was canceled/replaced outside this coordinator,
        // close only the old combat plan. Never advance or cancel the replacement.
        if (!OwnsGeneration(actions)) {
            return InterruptLinked(combat, true);
        }

        if (!actions.HasIncomingAttack()) {
            const DefenseReport report = actions.LastDefense();
            if (ResolveTerminalReport(combat, report)) return true;
            return InterruptLinked(combat, true);
        }

        // If this frame spans impact, stop the first step at the threat deadline.
        // Resolve the planner at that impact-time clock, then carry the frame's
        // remainder through recovery. This keeps cadence invariant to whether the
        // same elapsed time arrived as one long frame or multiple shorter frames.
        float firstStep = deltaSeconds;
        float overflow = 0.0f;
        const float remaining = actions.IncomingAttackRemaining();
        if (remaining > 0.0f && remaining < deltaSeconds) {
            const float impactStep = std::nextafter(
                remaining, std::numeric_limits<float>::infinity());
            firstStep = std::min(deltaSeconds, impactStep);
            overflow = std::max(0.0f, deltaSeconds - firstStep);
        }

        combat.AdvanceTime(firstStep);
        if (!OwnsCombatPlan(combat)) {
            if (OwnsGeneration(actions) && actions.HasIncomingAttack()) {
                actions.CancelIncomingAttack();
            }
            return InterruptLinked(combat, false);
        }

        actions.AdvanceTime(firstStep);
        if (actions.HasIncomingAttack()) {
            if (overflow > 0.0f) {
                return AdvanceTime(combat, actions, overflow);
            }
            return false;
        }

        const DefenseReport report = actions.LastDefense();
        bool resolved = ResolveTerminalReport(combat, report);
        if (!resolved) resolved = InterruptLinked(combat, true);

        if (overflow > 0.0f) {
            // The linked event is terminal now, so overflow advances ordinary
            // post-impact recovery/cooldown clocks without another resolution.
            combat.AdvanceTime(overflow);
            actions.AdvanceTime(overflow);
        }
        return resolved;
    }

    DefenseTrainingCue Cue(const CombatSandbox& combat,
        const ShadowbladeActions& actions) const {
        DefenseTrainingCue cue{};
        if (!linkedAttackActive_ || !OwnsObjects(combat, actions)
            || !OwnsCombatPlan(combat) || !actions.HasIncomingAttack()
            || !OwnsGeneration(actions)) {
            return cue;
        }

        const EnemyAttackPlan& plan = combat.PendingEnemyAttack();
        cue.pattern = plan.pattern;
        cue.blockable = plan.blockable;
        cue.secondsToImpact = std::max(0.0f, actions.IncomingAttackRemaining());
        if (actions.PerfectDefenseWindowOpen()) {
            cue.phase = DefenseTrainingCuePhase::PerfectWindow;
            cue.symbol = DefenseTrainingCueSymbol::PerfectTiming;
        } else if (actions.DodgeWindowOpen()) {
            cue.phase = DefenseTrainingCuePhase::DodgeWindow;
            cue.symbol = plan.blockable
                ? DefenseTrainingCueSymbol::Blockable
                : DefenseTrainingCueSymbol::Unblockable;
        } else {
            cue.phase = DefenseTrainingCuePhase::Approach;
            cue.symbol = plan.blockable
                ? DefenseTrainingCueSymbol::Blockable
                : DefenseTrainingCueSymbol::Unblockable;
        }
        return cue;
    }

    DefenseTrainingControlReminder ControlReminder(const CombatSandbox& combat,
        const ShadowbladeActions& actions) const {
        const DefenseTrainingCue cue = Cue(combat, actions);
        if (cue.phase == DefenseTrainingCuePhase::None) {
            return DefenseTrainingControlReminder::None;
        }
        if (!cue.blockable || goal_ == DefenseTrainingGoal::DodgeDiscipline) {
            return DefenseTrainingControlReminder::Dodge;
        }
        return DefenseTrainingControlReminder::Guard;
    }

    DefenseTrainingTutorialHint TutorialHint(const CombatSandbox& combat,
        const ShadowbladeActions& actions) const {
        if (stats_.consecutiveHits >= 2) {
            const DefenseTrainingCue cue = Cue(combat, actions);
            if (cue.phase == DefenseTrainingCuePhase::None) {
                return DefenseTrainingTutorialHint::ReadTelegraph;
            }
            return cue.blockable
                ? DefenseTrainingTutorialHint::GuardBlockable
                : DefenseTrainingTutorialHint::DodgeUnblockable;
        }
        if (stats_.consecutiveHits == 1) {
            return DefenseTrainingTutorialHint::ReadTelegraph;
        }
        const int successful = stats_.perfectDefenses + stats_.ordinaryDefenses;
        if (successful >= GoalTarget && stats_.perfectDefenses == 0) {
            return DefenseTrainingTutorialHint::AimPerfectTiming;
        }
        return DefenseTrainingTutorialHint::None;
    }

    bool SetGoal(DefenseTrainingGoal goal) {
        switch (goal) {
        case DefenseTrainingGoal::FreePractice:
        case DefenseTrainingGoal::PerfectDefense:
        case DefenseTrainingGoal::GuardDiscipline:
        case DefenseTrainingGoal::DodgeDiscipline:
            break;
        default:
            return false;
        }
        if (goal_ == goal) return false;
        goal_ = goal;
        return true;
    }

    DefenseTrainingGoal Goal() const { return goal_; }

    DefenseTrainingGoalStatus GoalStatus() const {
        DefenseTrainingGoalStatus status{};
        status.goal = goal_;
        if (goal_ == DefenseTrainingGoal::FreePractice) return status;

        status.target = GoalTarget;
        switch (goal_) {
        case DefenseTrainingGoal::PerfectDefense:
            status.current = std::min(GoalTarget, stats_.perfectDefenses);
            break;
        case DefenseTrainingGoal::GuardDiscipline:
            status.current = std::min(GoalTarget, stats_.guardDefenses);
            break;
        case DefenseTrainingGoal::DodgeDiscipline:
            status.current = std::min(GoalTarget, stats_.dodgeDefenses);
            break;
        case DefenseTrainingGoal::FreePractice:
        default:
            break;
        }
        status.complete = status.current >= status.target;
        return status;
    }

    bool SetPaused(bool paused) {
        if (paused_ == paused) return false;
        paused_ = paused;
        return true;
    }

    bool Paused() const { return paused_; }

    DefenseTrainingGrade Grade() const {
        const std::int64_t resolved = static_cast<std::int64_t>(stats_.perfectDefenses)
            + static_cast<std::int64_t>(stats_.ordinaryDefenses)
            + static_cast<std::int64_t>(stats_.hitsTaken);
        if (resolved < 3) return DefenseTrainingGrade::None;

        const std::int64_t perfect = stats_.perfectDefenses;
        const std::int64_t successful = perfect + stats_.ordinaryDefenses;
        if (stats_.damageTaken == 0 && perfect * 3 >= resolved * 2) {
            return DefenseTrainingGrade::Gold;
        }
        if (stats_.damageTaken <= 20 && successful * 2 >= resolved) {
            return DefenseTrainingGrade::Silver;
        }
        return DefenseTrainingGrade::Bronze;
    }

    const DefenseTrainingStats& Stats() const { return stats_; }
    bool HasLinkedAttack() const { return linkedAttackActive_; }

    void ResetStats() {
        stats_ = {};
    }

private:
    enum class ResolutionKind {
        Perfect,
        OrdinaryDefense,
        Hit,
    };

    enum class DefenseKind {
        None,
        Guard,
        Dodge,
    };

    static DefenseReport NoLinkedThreatReport() {
        return {DefenseResult::NoThreat, 0, 0, false, 0.0f};
    }

    bool CanQueue(const CombatSandbox& combat, const ShadowbladeActions& actions) const {
        return !paused_ && !linkedAttackActive_ && !combat.HasPendingEnemyAttack()
            && !actions.HasIncomingAttack() && actions.PlayerHealth() > 0;
    }

    bool LinkQueuedAttack(CombatSandbox& combat, ShadowbladeActions& actions) {
        const EnemyAttackPlan plan = combat.PendingEnemyAttack();
        const IncomingAttackDefinition incoming{
            plan.windupSeconds, plan.damage, plan.guardDamage, plan.blockable};
        if (!actions.BeginIncomingAttack(incoming)) {
            combat.ResolveEnemyAttack(EnemyAttackOutcome::Interrupted);
            return false;
        }

        linkedAttackActive_ = true;
        linkedCombat_ = &combat;
        linkedActions_ = &actions;
        linkedCombatAttackGeneration_ = combat.EnemyAttackGeneration();
        linkedAttackGeneration_ = actions.IncomingAttackGeneration();
        SaturatingIncrement(stats_.attacksQueued);
        return true;
    }

    bool OwnsObjects(const CombatSandbox& combat,
        const ShadowbladeActions& actions) const {
        return linkedAttackActive_ && linkedCombat_ == &combat && linkedActions_ == &actions;
    }

    bool OwnsCombatPlan(const CombatSandbox& combat) const {
        return linkedAttackActive_ && linkedCombat_ == &combat
            && linkedCombatAttackGeneration_ != 0
            && combat.HasPendingEnemyAttack()
            && combat.EnemyAttackGeneration() == linkedCombatAttackGeneration_;
    }

    bool OwnsGeneration(const ShadowbladeActions& actions) const {
        return linkedAttackActive_ && linkedActions_ == &actions
            && linkedAttackGeneration_ != 0
            && actions.IncomingAttackGeneration() == linkedAttackGeneration_;
    }

    void ClearLink() {
        linkedAttackActive_ = false;
        linkedCombat_ = nullptr;
        linkedActions_ = nullptr;
        linkedCombatAttackGeneration_ = 0;
        linkedAttackGeneration_ = 0;
    }

    bool InterruptLinked(CombatSandbox& combat, bool resolvePlanner) {
        if (!linkedAttackActive_ || linkedCombat_ != &combat) return false;
        if (resolvePlanner && OwnsCombatPlan(combat)
            && !combat.ResolveEnemyAttack(EnemyAttackOutcome::Interrupted)) {
            return false;
        }
        ClearLink();
        SaturatingIncrement(stats_.interruptions);
        stats_.currentPerfectStreak = 0;
        return true;
    }

    bool ResolveTerminalReport(CombatSandbox& combat, const DefenseReport& report) {
        switch (report.result) {
        case DefenseResult::PerfectGuard:
            return ResolveLinked(combat, EnemyAttackOutcome::PerfectDefense, report,
                ResolutionKind::Perfect, DefenseKind::Guard);
        case DefenseResult::PerfectDodge:
            return ResolveLinked(combat, EnemyAttackOutcome::PerfectDefense, report,
                ResolutionKind::Perfect, DefenseKind::Dodge);
        case DefenseResult::Guarded:
            return ResolveLinked(combat, EnemyAttackOutcome::Guarded, report,
                ResolutionKind::OrdinaryDefense, DefenseKind::Guard);
        case DefenseResult::Evaded:
            return ResolveLinked(combat, EnemyAttackOutcome::Evaded, report,
                ResolutionKind::OrdinaryDefense, DefenseKind::Dodge);
        case DefenseResult::GuardBroken:
        case DefenseResult::UnblockableHit:
        case DefenseResult::Hit:
            return ResolveLinked(combat, EnemyAttackOutcome::Hit, report,
                ResolutionKind::Hit, DefenseKind::None);
        default:
            return false;
        }
    }

    static void SaturatingIncrement(int& value) {
        if (value < std::numeric_limits<int>::max()) ++value;
    }

    static void SaturatingAddDamage(int& value, int amount) {
        if (amount <= 0) return;
        const int maximum = std::numeric_limits<int>::max();
        value = value > maximum - amount ? maximum : value + amount;
    }

    void RecordSuccessfulDefense(DefenseKind kind) {
        if (kind == DefenseKind::Guard) {
            SaturatingIncrement(stats_.guardDefenses);
        } else if (kind == DefenseKind::Dodge) {
            SaturatingIncrement(stats_.dodgeDefenses);
        }
        stats_.consecutiveHits = 0;
    }

    bool ResolveLinked(CombatSandbox& combat, EnemyAttackOutcome outcome,
        const DefenseReport& report, ResolutionKind kind, DefenseKind defenseKind) {
        if (!linkedAttackActive_ || linkedCombat_ != &combat
            || !OwnsCombatPlan(combat)) {
            return false;
        }
        if (!combat.ResolveEnemyAttack(outcome)) return false;

        ClearLink();
        if (kind == ResolutionKind::Perfect) {
            SaturatingIncrement(stats_.perfectDefenses);
            SaturatingIncrement(stats_.currentPerfectStreak);
            stats_.bestPerfectStreak = std::max(
                stats_.bestPerfectStreak, stats_.currentPerfectStreak);
            RecordSuccessfulDefense(defenseKind);
        } else if (kind == ResolutionKind::OrdinaryDefense) {
            SaturatingIncrement(stats_.ordinaryDefenses);
            stats_.currentPerfectStreak = 0;
            RecordSuccessfulDefense(defenseKind);
        } else {
            SaturatingIncrement(stats_.hitsTaken);
            SaturatingAddDamage(stats_.damageTaken, report.damageTaken);
            stats_.currentPerfectStreak = 0;
            SaturatingIncrement(stats_.consecutiveHits);
        }
        return true;
    }

    DefenseTrainingStats stats_{};
    DefenseTrainingGoal goal_{DefenseTrainingGoal::FreePractice};
    CombatSandbox* linkedCombat_{};
    ShadowbladeActions* linkedActions_{};
    std::uint64_t linkedCombatAttackGeneration_{};
    std::uint64_t linkedAttackGeneration_{};
    bool linkedAttackActive_{};
    bool paused_{};
};

} // namespace Astral::Scene
