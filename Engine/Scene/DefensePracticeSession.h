#pragma once

#include "Engine/Scene/CombatDefenseTraining.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace Astral::Scene {

enum class DefensePracticePace {
    Learning,
    Standard,
    Expert,
};

struct DefensePracticeSequence {
    static constexpr std::size_t MaximumPatterns = 6;
    std::array<EnemyAttackPattern, MaximumPatterns> patterns{};
    std::size_t count{};
};

struct DefensePracticePatternStats {
    int attempts{};
    int perfectDefenses{};
    int ordinaryDefenses{};
    int hitsTaken{};
    int interruptions{};
    int damageTaken{};
};

struct DefensePracticeScoreReport {
    std::int64_t baseScore{};
    std::int64_t score{};
    std::int64_t resolvedAttempts{};
    int timeCoefficientPercent{};
    int bestAlternatingChain{};
};

// Game-specific practice-session policy layered over the already-authoritative
// combat/Shadowblade defense bridge. It owns no renderer, input device, or save
// behavior and intentionally delegates all attack values and defense results to
// the existing gameplay domains.
class DefensePracticeSession {
public:
    static constexpr int MinimumGoalTarget = 1;
    static constexpr int MaximumGoalTarget = 10;
    static constexpr int DefaultGoalTarget = CombatDefenseTraining::GoalTarget;
    static constexpr double AlternatingChainWindowSeconds = 2.5;

    bool SetPracticeSequence(const DefensePracticeSequence& sequence) {
        if (drill_.HasLinkedAttack() || currentAttackActive_) return false;
        if (sequence.count == 0 || sequence.count > DefensePracticeSequence::MaximumPatterns) {
            return false;
        }
        for (std::size_t index = 0; index < sequence.count; ++index) {
            if (!IsValidPattern(sequence.patterns[index])) return false;
        }

        bool changed = sequenceLength_ != sequence.count || sequenceCursor_ != 0;
        for (std::size_t index = 0; index < sequence.count; ++index) {
            if (practiceSequence_[index] != sequence.patterns[index]) changed = true;
        }
        if (!changed) return false;

        practiceSequence_ = sequence.patterns;
        sequenceLength_ = sequence.count;
        sequenceCursor_ = 0;
        return true;
    }

    bool ClearPracticeSequence() {
        if (drill_.HasLinkedAttack() || currentAttackActive_ || sequenceLength_ == 0) {
            return false;
        }
        practiceSequence_ = {};
        sequenceLength_ = 0;
        sequenceCursor_ = 0;
        return true;
    }

    std::size_t PracticeSequenceLength() const { return sequenceLength_; }
    std::size_t PracticeSequenceCursor() const { return sequenceCursor_; }

    bool SetPace(DefensePracticePace pace) {
        if (drill_.HasLinkedAttack() || currentAttackActive_ || !IsValidPace(pace)) {
            return false;
        }
        if (pace_ == pace) return false;
        pace_ = pace;
        return true;
    }

    DefensePracticePace Pace() const { return pace_; }

    bool SetGoal(DefenseTrainingGoal goal) { return drill_.SetGoal(goal); }
    DefenseTrainingGoal Goal() const { return drill_.Goal(); }

    bool SetGoalTarget(int target) {
        if (target < MinimumGoalTarget || target > MaximumGoalTarget
            || target == goalTarget_) {
            return false;
        }
        goalTarget_ = target;
        return true;
    }

    int GoalTarget() const { return goalTarget_; }

    DefenseTrainingGoalStatus GoalStatus() const {
        DefenseTrainingGoalStatus status{};
        status.goal = drill_.Goal();
        if (status.goal == DefenseTrainingGoal::FreePractice) return status;

        status.target = goalTarget_;
        switch (status.goal) {
        case DefenseTrainingGoal::PerfectDefense:
            status.current = std::min(goalTarget_, drill_.Stats().perfectDefenses);
            break;
        case DefenseTrainingGoal::GuardDiscipline:
            status.current = std::min(goalTarget_, drill_.Stats().guardDefenses);
            break;
        case DefenseTrainingGoal::DodgeDiscipline:
            status.current = std::min(goalTarget_, drill_.Stats().dodgeDefenses);
            break;
        case DefenseTrainingGoal::FreePractice:
        default:
            break;
        }
        status.complete = status.current >= status.target;
        return status;
    }

    bool QueueNextAttack(CombatSandbox& combat, ShadowbladeActions& actions) {
        if (drill_.HasLinkedAttack() || currentAttackActive_ || !ObjectsAvailable(combat, actions)) {
            return false;
        }

        const EnemyAggressionPreset previousPreset = combat.AggressionPreset();
        const EnemyAggressionPreset desiredPreset = PresetForPace(pace_);
        combat.SetEnemyAggressionPreset(desiredPreset);

        bool queued = false;
        if (sequenceLength_ > 0) {
            queued = drill_.QueueAttackPattern(
                combat, actions, practiceSequence_[sequenceCursor_]);
        } else {
            queued = drill_.QueueNextAttack(combat, actions);
        }
        if (!queued) {
            if (previousPreset != desiredPreset) {
                combat.SetEnemyAggressionPreset(previousPreset);
            }
            return false;
        }

        sessionCombat_ = &combat;
        sessionActions_ = &actions;
        currentPattern_ = combat.PendingEnemyAttack().pattern;
        currentAttackActive_ = true;
        if (!sessionStarted_) {
            sessionStarted_ = true;
            sessionStartSeconds_ = combat.ElapsedSecondsPrecise();
        }
        Increment(PatternStatsMutable(currentPattern_).attempts);

        if (sequenceLength_ > 0) {
            sequenceCursor_ = (sequenceCursor_ + 1) % sequenceLength_;
        }
        return true;
    }

    DefenseReport TryDefend(CombatSandbox& combat, ShadowbladeActions& actions,
        DefenseInput input) {
        if (!ObjectsOwned(combat, actions)) return NoSessionThreatReport();

        const DefenseReport report = drill_.TryDefend(combat, actions, input);
        if (currentAttackActive_ && IsTerminal(report.result)) {
            RecordTerminalReport(combat, report, input);
        }
        return report;
    }

    bool AdvanceTime(CombatSandbox& combat, ShadowbladeActions& actions,
        float deltaSeconds) {
        if (sessionStarted_ && !ObjectsOwned(combat, actions)) return false;

        const DefenseTrainingStats before = drill_.Stats();
        const bool resolved = drill_.AdvanceTime(combat, actions, deltaSeconds);
        if (!resolved || !currentAttackActive_) return resolved;

        const DefenseTrainingStats after = drill_.Stats();
        DefensePracticePatternStats& pattern = PatternStatsMutable(currentPattern_);
        if (after.hitsTaken > before.hitsTaken) {
            Increment(pattern.hitsTaken);
            AddDamage(pattern.damageTaken, after.damageTaken - before.damageTaken);
            ResetAlternatingChain();
        } else if (after.interruptions > before.interruptions) {
            Increment(pattern.interruptions);
            ResetAlternatingChain();
        } else if (after.perfectDefenses > before.perfectDefenses) {
            Increment(pattern.perfectDefenses);
            ResetAlternatingChain();
        } else if (after.ordinaryDefenses > before.ordinaryDefenses) {
            Increment(pattern.ordinaryDefenses);
            ResetAlternatingChain();
        }
        currentAttackActive_ = false;
        return true;
    }

    bool SetPaused(bool paused) { return drill_.SetPaused(paused); }
    bool Paused() const { return drill_.Paused(); }

    DefenseTrainingCue Cue(const CombatSandbox& combat,
        const ShadowbladeActions& actions) const {
        return drill_.Cue(combat, actions);
    }

    DefenseTrainingControlReminder ControlReminder(const CombatSandbox& combat,
        const ShadowbladeActions& actions) const {
        return drill_.ControlReminder(combat, actions);
    }

    DefenseTrainingTutorialHint TutorialHint(const CombatSandbox& combat,
        const ShadowbladeActions& actions) const {
        return drill_.TutorialHint(combat, actions);
    }

    DefenseTrainingGrade Grade() const { return drill_.Grade(); }
    const DefenseTrainingStats& Stats() const { return drill_.Stats(); }

    int CurrentAlternatingChain() const { return currentAlternatingChain_; }
    int BestAlternatingChain() const { return bestAlternatingChain_; }

    DefensePracticePatternStats PatternStats(EnemyAttackPattern pattern) const {
        if (!IsValidPattern(pattern)) return {};
        return patternStats_[PatternIndex(pattern)];
    }

    DefensePracticeScoreReport Score(const CombatSandbox& combat) const {
        DefensePracticeScoreReport report{};
        if (!sessionStarted_ || sessionCombat_ != &combat) return report;

        const DefenseTrainingStats& stats = drill_.Stats();
        const std::int64_t perfect = std::max(0, stats.perfectDefenses);
        const std::int64_t ordinary = std::max(0, stats.ordinaryDefenses);
        const std::int64_t hits = std::max(0, stats.hitsTaken);
        const std::int64_t successes = perfect + ordinary;
        report.resolvedAttempts = successes + hits;
        report.bestAlternatingChain = bestAlternatingChain_;
        report.baseScore = perfect * 300 + ordinary * 150
            + static_cast<std::int64_t>(bestAlternatingChain_) * 50;
        if (report.baseScore <= 0 || successes <= 0) {
            report.baseScore = std::max<std::int64_t>(0, report.baseScore);
            return report;
        }

        const double activeSeconds = std::max(
            0.0, combat.ElapsedSecondsPrecise() - sessionStartSeconds_);
        const double secondsPerSuccess = activeSeconds / static_cast<double>(successes);
        report.timeCoefficientPercent = secondsPerSuccess <= 1.5
            ? 125
            : (secondsPerSuccess <= 3.0 ? 100 : 75);
        report.score = report.baseScore * report.timeCoefficientPercent / 100;
        return report;
    }

    bool ResetMetrics() {
        if (drill_.HasLinkedAttack() || currentAttackActive_) return false;
        drill_.ResetStats();
        patternStats_ = {};
        currentAlternatingChain_ = 0;
        bestAlternatingChain_ = 0;
        lastSuccessfulDefense_ = SuccessfulDefenseKind::None;
        lastSuccessfulDefenseSeconds_ = 0.0;
        sessionStarted_ = false;
        sessionStartSeconds_ = 0.0;
        sessionCombat_ = nullptr;
        sessionActions_ = nullptr;
        sequenceCursor_ = 0;
        return true;
    }

private:
    enum class SuccessfulDefenseKind {
        None,
        Guard,
        Dodge,
    };

    static bool IsValidPattern(EnemyAttackPattern pattern) {
        switch (pattern) {
        case EnemyAttackPattern::QuickCut:
        case EnemyAttackPattern::GuardBreaker:
        case EnemyAttackPattern::RiftBurst:
            return true;
        default:
            return false;
        }
    }

    static bool IsValidPace(DefensePracticePace pace) {
        switch (pace) {
        case DefensePracticePace::Learning:
        case DefensePracticePace::Standard:
        case DefensePracticePace::Expert:
            return true;
        default:
            return false;
        }
    }

    static EnemyAggressionPreset PresetForPace(DefensePracticePace pace) {
        switch (pace) {
        case DefensePracticePace::Learning:
            return EnemyAggressionPreset::Relaxed;
        case DefensePracticePace::Expert:
            return EnemyAggressionPreset::Aggressive;
        case DefensePracticePace::Standard:
        default:
            return EnemyAggressionPreset::Standard;
        }
    }

    static std::size_t PatternIndex(EnemyAttackPattern pattern) {
        switch (pattern) {
        case EnemyAttackPattern::GuardBreaker: return 1;
        case EnemyAttackPattern::RiftBurst: return 2;
        case EnemyAttackPattern::QuickCut:
        default: return 0;
        }
    }

    static bool IsTerminal(DefenseResult result) {
        switch (result) {
        case DefenseResult::PerfectGuard:
        case DefenseResult::PerfectDodge:
        case DefenseResult::Guarded:
        case DefenseResult::Evaded:
        case DefenseResult::GuardBroken:
        case DefenseResult::UnblockableHit:
        case DefenseResult::Hit:
            return true;
        default:
            return false;
        }
    }

    static bool IsPerfect(DefenseResult result) {
        return result == DefenseResult::PerfectGuard
            || result == DefenseResult::PerfectDodge;
    }

    static bool IsOrdinaryDefense(DefenseResult result) {
        return result == DefenseResult::Guarded || result == DefenseResult::Evaded;
    }

    static bool IsHit(DefenseResult result) {
        return result == DefenseResult::GuardBroken
            || result == DefenseResult::UnblockableHit
            || result == DefenseResult::Hit;
    }

    static SuccessfulDefenseKind KindForInput(DefenseInput input) {
        return input == DefenseInput::Guard
            ? SuccessfulDefenseKind::Guard
            : SuccessfulDefenseKind::Dodge;
    }

    static DefenseReport NoSessionThreatReport() {
        return {DefenseResult::NoThreat, 0, 0, false, 0.0f};
    }

    bool ObjectsAvailable(const CombatSandbox& combat,
        const ShadowbladeActions& actions) const {
        return (!sessionCombat_ || sessionCombat_ == &combat)
            && (!sessionActions_ || sessionActions_ == &actions);
    }

    bool ObjectsOwned(const CombatSandbox& combat,
        const ShadowbladeActions& actions) const {
        return sessionCombat_ == &combat && sessionActions_ == &actions;
    }

    DefensePracticePatternStats& PatternStatsMutable(EnemyAttackPattern pattern) {
        return patternStats_[PatternIndex(pattern)];
    }

    void RecordTerminalReport(CombatSandbox& combat, const DefenseReport& report,
        DefenseInput input) {
        DefensePracticePatternStats& pattern = PatternStatsMutable(currentPattern_);
        if (IsPerfect(report.result)) {
            Increment(pattern.perfectDefenses);
            RecordSuccessfulDefense(combat, input);
        } else if (IsOrdinaryDefense(report.result)) {
            Increment(pattern.ordinaryDefenses);
            RecordSuccessfulDefense(combat, input);
        } else if (IsHit(report.result)) {
            Increment(pattern.hitsTaken);
            AddDamage(pattern.damageTaken, report.damageTaken);
            ResetAlternatingChain();
        }
        currentAttackActive_ = false;
    }

    void RecordSuccessfulDefense(const CombatSandbox& combat, DefenseInput input) {
        const SuccessfulDefenseKind kind = KindForInput(input);
        const double now = combat.ElapsedSecondsPrecise();
        const bool alternates = currentAlternatingChain_ > 0
            && kind != lastSuccessfulDefense_
            && now >= lastSuccessfulDefenseSeconds_
            && now - lastSuccessfulDefenseSeconds_ <= AlternatingChainWindowSeconds;
        if (alternates) {
            Increment(currentAlternatingChain_);
        } else {
            currentAlternatingChain_ = 1;
        }
        bestAlternatingChain_ = std::max(bestAlternatingChain_, currentAlternatingChain_);
        lastSuccessfulDefense_ = kind;
        lastSuccessfulDefenseSeconds_ = now;
    }

    void ResetAlternatingChain() {
        currentAlternatingChain_ = 0;
        lastSuccessfulDefense_ = SuccessfulDefenseKind::None;
        lastSuccessfulDefenseSeconds_ = 0.0;
    }

    static void Increment(int& value) {
        if (value < std::numeric_limits<int>::max()) ++value;
    }

    static void AddDamage(int& value, int amount) {
        if (amount <= 0) return;
        const int maximum = std::numeric_limits<int>::max();
        value = value > maximum - amount ? maximum : value + amount;
    }

    CombatDefenseTraining drill_{};
    std::array<EnemyAttackPattern, DefensePracticeSequence::MaximumPatterns> practiceSequence_{};
    std::array<DefensePracticePatternStats, 3> patternStats_{};
    std::size_t sequenceLength_{};
    std::size_t sequenceCursor_{};
    DefensePracticePace pace_{DefensePracticePace::Standard};
    int goalTarget_{DefaultGoalTarget};
    CombatSandbox* sessionCombat_{};
    ShadowbladeActions* sessionActions_{};
    EnemyAttackPattern currentPattern_{EnemyAttackPattern::QuickCut};
    bool currentAttackActive_{};
    int currentAlternatingChain_{};
    int bestAlternatingChain_{};
    SuccessfulDefenseKind lastSuccessfulDefense_{SuccessfulDefenseKind::None};
    double lastSuccessfulDefenseSeconds_{};
    bool sessionStarted_{};
    double sessionStartSeconds_{};
};

} // namespace Astral::Scene
