#pragma once

#include "Engine/Scene/CombatSandbox.h"
#include "Engine/Scene/ShadowbladeActions.h"

#include <algorithm>
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
        SaturatingIncrement(stats_.attacksQueued);
        return true;
    }

    DefenseReport TryDefend(CombatSandbox& combat, ShadowbladeActions& actions,
        DefenseInput input) {
        const bool linkedBefore = linkedAttackActive_;
        const DefenseReport report = actions.TryDefend(input);
        if (!linkedBefore) return report;

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
        combat.AdvanceTime(deltaSeconds);
        actions.AdvanceTime(deltaSeconds);
        if (!linkedAttackActive_) return false;

        if (!combat.HasPendingEnemyAttack()) {
            if (actions.HasIncomingAttack()) actions.CancelIncomingAttack();
            linkedAttackActive_ = false;
            SaturatingIncrement(stats_.interruptions);
            stats_.currentPerfectStreak = 0;
            return true;
        }

        if (actions.HasIncomingAttack()) return false;

        const DefenseReport report = actions.LastDefense();
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

    DefenseTrainingCue Cue(const CombatSandbox& combat,
        const ShadowbladeActions& actions) const {
        DefenseTrainingCue cue{};
        if (!linkedAttackActive_ || !combat.HasPendingEnemyAttack()
            || !actions.HasIncomingAttack()) {
            return cue;
        }

        const EnemyAttackPlan& plan = combat.PendingEnemyAttack();
        cue.pattern = plan.pattern;
        cue.blockable = plan.blockable;
        cue.secondsToImpact = std::max(0.0f, actions.IncomingAttackRemaining());
        if (cue.secondsToImpact <= actions.PerfectDefenseWindowSeconds()) {
            cue.phase = DefenseTrainingCuePhase::PerfectWindow;
        } else if (cue.secondsToImpact <= ShadowbladeActions::DodgeWindowSeconds) {
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
        if (!linkedAttackActive_ || !combat.HasPendingEnemyAttack()) return false;
        if (!combat.ResolveEnemyAttack(outcome)) return false;

        linkedAttackActive_ = false;
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
    bool linkedAttackActive_{};
};

} // namespace Astral::Scene
