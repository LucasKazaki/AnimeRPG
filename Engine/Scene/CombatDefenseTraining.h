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

enum class DefenseTrainingGrade {
    None,
    Bronze,
    Silver,
    Gold,
};

struct DefenseTrainingCue {
    DefenseTrainingCuePhase phase{DefenseTrainingCuePhase::None};
    EnemyAttackPattern pattern{EnemyAttackPattern::QuickCut};
    float secondsToImpact{};
    bool blockable{};
};

struct DefenseTrainingStats {
    int attacksQueued{};
    int defenseInputs{};
    int perfectDefenses{};
    int ordinaryDefenses{};
    int hitsTaken{};
    int interruptions{};
    int damageTaken{};
    int currentPerfectStreak{};
    int bestPerfectStreak{};
};

class CombatDefenseTraining {
public:
    bool QueueNextAttack(CombatSandbox& combat, ShadowbladeActions& actions) {
        if (linkedAttackActive_ || combat.HasPendingEnemyAttack()
            || actions.HasIncomingAttack() || actions.PlayerHealth() <= 0) {
            return false;
        }
        if (!combat.QueueNextEnemyAttack()) return false;

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
        linkedAttackGeneration_ = actions.IncomingAttackGeneration();
        SaturatingIncrement(stats_.attacksQueued);
        return true;
    }

    DefenseReport TryDefend(CombatSandbox& combat, ShadowbladeActions& actions,
        DefenseInput input) {
        if (!linkedAttackActive_) return actions.TryDefend(input);
        if (!OwnsObjects(combat, actions)) return NoLinkedThreatReport();

        if (!combat.HasPendingEnemyAttack()) {
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
            && report.result != DefenseResult::ThreatQueued) {
            SaturatingIncrement(stats_.defenseInputs);
        }

        switch (report.result) {
        case DefenseResult::TooEarly:
            return report;
        case DefenseResult::PerfectGuard:
        case DefenseResult::PerfectDodge:
            ResolveLinked(combat, EnemyAttackOutcome::PerfectDefense, report,
                ResolutionKind::Perfect);
            return report;
        case DefenseResult::Guarded:
            ResolveLinked(combat, EnemyAttackOutcome::Guarded, report,
                ResolutionKind::OrdinaryDefense);
            return report;
        case DefenseResult::Evaded:
            ResolveLinked(combat, EnemyAttackOutcome::Evaded, report,
                ResolutionKind::OrdinaryDefense);
            return report;
        case DefenseResult::GuardBroken:
        case DefenseResult::UnblockableHit:
        case DefenseResult::Hit:
            ResolveLinked(combat, EnemyAttackOutcome::Hit, report,
                ResolutionKind::Hit);
            return report;
        case DefenseResult::None:
        case DefenseResult::ThreatQueued:
        case DefenseResult::NoThreat:
        case DefenseResult::InvalidThreat:
        default:
            return report;
        }
    }

    bool AdvanceTime(CombatSandbox& combat, ShadowbladeActions& actions,
        float deltaSeconds) {
        // Preserve both underlying subsystems' no-op semantics for invalid or
        // nonpositive time. Reconciliation waits for the next valid tick.
        if (!(deltaSeconds > 0.0f) || !std::isfinite(deltaSeconds)) return false;

        if (!linkedAttackActive_) {
            combat.AdvanceTime(deltaSeconds);
            actions.AdvanceTime(deltaSeconds);
            return false;
        }
        if (!OwnsObjects(combat, actions)) return false;

        // Reconcile an authoritative interruption before advancing the linked
        // Shadowblade clock. A delayed coordinator tick must never resurrect a
        // staggered/defeated enemy's already-cleared attack as stale damage.
        if (!combat.HasPendingEnemyAttack()) {
            if (OwnsGeneration(actions) && actions.HasIncomingAttack()) {
                actions.CancelIncomingAttack();
            }
            return InterruptLinked(combat, false);
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

        combat.AdvanceTime(deltaSeconds);
        if (!combat.HasPendingEnemyAttack()) {
            if (OwnsGeneration(actions) && actions.HasIncomingAttack()) {
                actions.CancelIncomingAttack();
            }
            return InterruptLinked(combat, false);
        }

        actions.AdvanceTime(deltaSeconds);
        if (actions.HasIncomingAttack()) return false;

        const DefenseReport report = actions.LastDefense();
        if (ResolveTerminalReport(combat, report)) return true;
        return InterruptLinked(combat, true);
    }

    DefenseTrainingCue Cue(const CombatSandbox& combat,
        const ShadowbladeActions& actions) const {
        DefenseTrainingCue cue{};
        if (!linkedAttackActive_ || !OwnsObjects(combat, actions)
            || !combat.HasPendingEnemyAttack() || !actions.HasIncomingAttack()
            || !OwnsGeneration(actions)) {
            return cue;
        }

        const EnemyAttackPlan& plan = combat.PendingEnemyAttack();
        cue.pattern = plan.pattern;
        cue.blockable = plan.blockable;
        cue.secondsToImpact = std::max(0.0f, actions.IncomingAttackRemaining());
        if (actions.PerfectDefenseWindowOpen()) {
            cue.phase = DefenseTrainingCuePhase::PerfectWindow;
        } else if (actions.DodgeWindowOpen()) {
            cue.phase = DefenseTrainingCuePhase::DodgeWindow;
        } else {
            cue.phase = DefenseTrainingCuePhase::Approach;
        }
        return cue;
    }

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

    static DefenseReport NoLinkedThreatReport() {
        return {DefenseResult::NoThreat, 0, 0, false, 0.0f};
    }

    bool OwnsObjects(const CombatSandbox& combat,
        const ShadowbladeActions& actions) const {
        return linkedAttackActive_ && linkedCombat_ == &combat && linkedActions_ == &actions;
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
        linkedAttackGeneration_ = 0;
    }

    bool InterruptLinked(CombatSandbox& combat, bool resolvePlanner) {
        if (!linkedAttackActive_ || linkedCombat_ != &combat) return false;
        if (resolvePlanner && combat.HasPendingEnemyAttack()
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
        case DefenseResult::PerfectDodge:
            return ResolveLinked(combat, EnemyAttackOutcome::PerfectDefense, report,
                ResolutionKind::Perfect);
        case DefenseResult::Guarded:
            return ResolveLinked(combat, EnemyAttackOutcome::Guarded, report,
                ResolutionKind::OrdinaryDefense);
        case DefenseResult::Evaded:
            return ResolveLinked(combat, EnemyAttackOutcome::Evaded, report,
                ResolutionKind::OrdinaryDefense);
        case DefenseResult::GuardBroken:
        case DefenseResult::UnblockableHit:
        case DefenseResult::Hit:
            return ResolveLinked(combat, EnemyAttackOutcome::Hit, report,
                ResolutionKind::Hit);
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

    bool ResolveLinked(CombatSandbox& combat, EnemyAttackOutcome outcome,
        const DefenseReport& report, ResolutionKind kind) {
        if (!linkedAttackActive_ || linkedCombat_ != &combat
            || !combat.HasPendingEnemyAttack()) {
            return false;
        }
        if (!combat.ResolveEnemyAttack(outcome)) return false;

        ClearLink();
        if (kind == ResolutionKind::Perfect) {
            SaturatingIncrement(stats_.perfectDefenses);
            SaturatingIncrement(stats_.currentPerfectStreak);
            stats_.bestPerfectStreak = std::max(
                stats_.bestPerfectStreak, stats_.currentPerfectStreak);
        } else if (kind == ResolutionKind::OrdinaryDefense) {
            SaturatingIncrement(stats_.ordinaryDefenses);
            stats_.currentPerfectStreak = 0;
        } else {
            SaturatingIncrement(stats_.hitsTaken);
            SaturatingAddDamage(stats_.damageTaken, report.damageTaken);
            stats_.currentPerfectStreak = 0;
        }
        return true;
    }

    DefenseTrainingStats stats_{};
    CombatSandbox* linkedCombat_{};
    ShadowbladeActions* linkedActions_{};
    std::uint64_t linkedAttackGeneration_{};
    bool linkedAttackActive_{};
};

} // namespace Astral::Scene
