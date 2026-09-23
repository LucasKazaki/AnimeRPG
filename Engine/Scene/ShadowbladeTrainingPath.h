#pragma once

#include "Engine/Scene/DefensePracticeSession.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace Astral::Scene {

enum class ShadowbladeTrainingLesson : std::uint8_t {
    GuardFundamentals,
    DodgeFundamentals,
    MixedTiming,
    PressureHandling,
    BossRehearsal,
    Complete,
};

enum class ShadowbladeTrainingMedal : std::uint8_t {
    None,
    Bronze,
    Silver,
    Gold,
};

struct ShadowbladeTrainingLessonPlan {
    ShadowbladeTrainingLesson lesson{ShadowbladeTrainingLesson::GuardFundamentals};
    DefensePracticeSequence sequence{};
    DefensePracticePace pace{DefensePracticePace::Learning};
    DefenseTrainingGoal goal{DefenseTrainingGoal::GuardDiscipline};
    int goalTarget{DefensePracticeSession::DefaultGoalTarget};
    bool bossRehearsal{};
};

struct ShadowbladeTrainingCheckpoint {
    static constexpr std::uint32_t SchemaVersion = 1;

    std::uint32_t schemaVersion{SchemaVersion};
    ShadowbladeTrainingLesson currentLesson{ShadowbladeTrainingLesson::GuardFundamentals};
    std::uint8_t completedMask{};
    std::array<ShadowbladeTrainingMedal, 5> medals{};
    bool forgivingTimingAssist{};
};

// Game-owned onboarding/training policy layered over the existing authoritative
// defense-practice bridge. It owns no renderer, device input, persistence backend,
// enemy stats, or reward economy. Checkpoints are in-memory state contracts only.
class ShadowbladeTrainingPath {
public:
    static constexpr std::size_t LessonCount = 5;
    static constexpr std::uint8_t AllLessonsMask =
        static_cast<std::uint8_t>((1u << LessonCount) - 1u);

    ShadowbladeTrainingLesson CurrentLesson() const { return currentLesson_; }
    bool Complete() const { return currentLesson_ == ShadowbladeTrainingLesson::Complete; }
    bool ForgivingTimingAssistEnabled() const { return forgivingTimingAssist_; }

    bool LessonUnlocked(ShadowbladeTrainingLesson lesson) const {
        const std::size_t index = LessonIndex(lesson);
        if (index >= LessonCount) return lesson == ShadowbladeTrainingLesson::Complete
            && completedMask_ == AllLessonsMask;
        const std::uint8_t requiredMask = index == 0
            ? 0
            : static_cast<std::uint8_t>((1u << index) - 1u);
        return (completedMask_ & requiredMask) == requiredMask;
    }

    int CompletedLessonCount() const {
        int count = 0;
        for (std::size_t index = 0; index < LessonCount; ++index) {
            if ((completedMask_ & LessonBit(index)) != 0) ++count;
        }
        return count;
    }

    ShadowbladeTrainingMedal MedalFor(ShadowbladeTrainingLesson lesson) const {
        const std::size_t index = LessonIndex(lesson);
        return index < LessonCount ? medals_[index] : ShadowbladeTrainingMedal::None;
    }

    static ShadowbladeTrainingLessonPlan PlanForLesson(ShadowbladeTrainingLesson lesson) {
        ShadowbladeTrainingLessonPlan plan{};
        plan.lesson = lesson;
        switch (lesson) {
        case ShadowbladeTrainingLesson::GuardFundamentals:
            plan.sequence.patterns[0] = EnemyAttackPattern::QuickCut;
            plan.sequence.count = 1;
            plan.pace = DefensePracticePace::Learning;
            plan.goal = DefenseTrainingGoal::GuardDiscipline;
            plan.goalTarget = 3;
            return plan;
        case ShadowbladeTrainingLesson::DodgeFundamentals:
            plan.sequence.patterns[0] = EnemyAttackPattern::RiftBurst;
            plan.sequence.count = 1;
            plan.pace = DefensePracticePace::Learning;
            plan.goal = DefenseTrainingGoal::DodgeDiscipline;
            plan.goalTarget = 3;
            return plan;
        case ShadowbladeTrainingLesson::MixedTiming:
            plan.sequence.patterns[0] = EnemyAttackPattern::QuickCut;
            plan.sequence.patterns[1] = EnemyAttackPattern::RiftBurst;
            plan.sequence.count = 2;
            plan.pace = DefensePracticePace::Standard;
            plan.goal = DefenseTrainingGoal::PerfectDefense;
            plan.goalTarget = 3;
            return plan;
        case ShadowbladeTrainingLesson::PressureHandling:
            plan.sequence.patterns[0] = EnemyAttackPattern::QuickCut;
            plan.sequence.patterns[1] = EnemyAttackPattern::GuardBreaker;
            plan.sequence.patterns[2] = EnemyAttackPattern::RiftBurst;
            plan.sequence.count = 3;
            plan.pace = DefensePracticePace::Expert;
            plan.goal = DefenseTrainingGoal::PerfectDefense;
            plan.goalTarget = 3;
            return plan;
        case ShadowbladeTrainingLesson::BossRehearsal:
            plan.sequence.patterns[0] = EnemyAttackPattern::QuickCut;
            plan.sequence.patterns[1] = EnemyAttackPattern::GuardBreaker;
            plan.sequence.patterns[2] = EnemyAttackPattern::RiftBurst;
            plan.sequence.count = 3;
            plan.pace = DefensePracticePace::Standard;
            plan.goal = DefenseTrainingGoal::FreePractice;
            plan.goalTarget = 3;
            plan.bossRehearsal = true;
            return plan;
        case ShadowbladeTrainingLesson::Complete:
        default:
            plan.sequence.count = 0;
            return plan;
        }
    }

    bool ApplyCurrentLesson(DefensePracticeSession& session,
        ShadowbladeActions& actions) {
        if (Complete() || !LessonUnlocked(currentLesson_) || actions.HasIncomingAttack()) {
            return false;
        }
        const ShadowbladeTrainingLessonPlan plan = PlanForLesson(currentLesson_);
        if (plan.sequence.count == 0) return false;

        // Configure a copy first. This makes lesson setup all-or-nothing even if
        // an owned threat is active or a future policy setter rejects a value.
        DefensePracticeSession candidate = session;
        if (!candidate.ResetMetrics()) return false;
        if (candidate.Paused() && !candidate.SetPaused(false)) return false;
        if (candidate.PracticeSequenceLength() > 0 && !candidate.ClearPracticeSequence()) {
            return false;
        }
        if (!candidate.SetPracticeSequence(plan.sequence)) return false;
        if (candidate.Pace() != plan.pace && !candidate.SetPace(plan.pace)) return false;
        if (candidate.Goal() != plan.goal && !candidate.SetGoal(plan.goal)) return false;
        if (candidate.GoalTarget() != plan.goalTarget
            && !candidate.SetGoalTarget(plan.goalTarget)) {
            return false;
        }

        session = candidate;
        actions.SetDefenseTimingPreset(forgivingTimingAssist_
            ? DefenseTimingPreset::Forgiving
            : DefenseTimingPreset::Standard);
        appliedLesson_ = currentLesson_;
        return true;
    }

    bool SetForgivingTimingAssist(bool enabled, ShadowbladeActions& actions) {
        if (forgivingTimingAssist_ == enabled || actions.HasIncomingAttack()) return false;
        forgivingTimingAssist_ = enabled;
        actions.SetDefenseTimingPreset(enabled
            ? DefenseTimingPreset::Forgiving
            : DefenseTimingPreset::Standard);
        return true;
    }

    bool CurrentLessonComplete(const DefensePracticeSession& session) const {
        if (Complete() || !SessionMatchesCurrentLesson(session)) return false;
        if (currentLesson_ != ShadowbladeTrainingLesson::BossRehearsal) {
            return session.GoalStatus().complete;
        }

        for (EnemyAttackPattern pattern : BossPatterns()) {
            const DefensePracticePatternStats stats = session.PatternStats(pattern);
            const bool resolved = stats.perfectDefenses > 0
                || stats.ordinaryDefenses > 0
                || stats.hitsTaken > 0;
            if (stats.attempts < 1 || !resolved) return false;
        }
        return true;
    }

    ShadowbladeTrainingMedal CurrentMedal(const DefensePracticeSession& session) const {
        if (!CurrentLessonComplete(session)) return ShadowbladeTrainingMedal::None;

        if (currentLesson_ == ShadowbladeTrainingLesson::BossRehearsal) {
            bool everySuccess = true;
            bool everyPerfect = true;
            bool anyHit = false;
            for (EnemyAttackPattern pattern : BossPatterns()) {
                const DefensePracticePatternStats stats = session.PatternStats(pattern);
                everySuccess = everySuccess
                    && (stats.perfectDefenses > 0 || stats.ordinaryDefenses > 0);
                everyPerfect = everyPerfect
                    && stats.perfectDefenses > 0
                    && stats.ordinaryDefenses == 0;
                anyHit = anyHit || stats.hitsTaken > 0;
            }
            if (anyHit) return ShadowbladeTrainingMedal::Bronze;
            if (everyPerfect) return ShadowbladeTrainingMedal::Gold;
            if (everySuccess) return ShadowbladeTrainingMedal::Silver;
            return ShadowbladeTrainingMedal::Bronze;
        }

        const DefenseTrainingStats& stats = session.Stats();
        if (stats.hitsTaken == 0 && stats.ordinaryDefenses == 0
            && stats.perfectDefenses >= session.GoalTarget()) {
            return ShadowbladeTrainingMedal::Gold;
        }
        if (stats.hitsTaken == 0) return ShadowbladeTrainingMedal::Silver;
        return ShadowbladeTrainingMedal::Bronze;
    }

    bool CommitCurrentLesson(const DefensePracticeSession& session) {
        if (Complete() || !CurrentLessonComplete(session)) return false;
        const std::size_t index = LessonIndex(currentLesson_);
        if (index >= LessonCount) return false;

        const ShadowbladeTrainingMedal medal = CurrentMedal(session);
        if (medal == ShadowbladeTrainingMedal::None) return false;
        if (MedalRank(medal) > MedalRank(medals_[index])) medals_[index] = medal;
        completedMask_ = static_cast<std::uint8_t>(completedMask_ | LessonBit(index));
        currentLesson_ = LessonAtIndex(FirstIncompleteIndex(completedMask_));
        appliedLesson_ = ShadowbladeTrainingLesson::Complete;
        return true;
    }

    ShadowbladeTrainingCheckpoint Checkpoint() const {
        ShadowbladeTrainingCheckpoint checkpoint{};
        checkpoint.currentLesson = currentLesson_;
        checkpoint.completedMask = completedMask_;
        checkpoint.medals = medals_;
        checkpoint.forgivingTimingAssist = forgivingTimingAssist_;
        return checkpoint;
    }

    bool RestoreCheckpoint(const ShadowbladeTrainingCheckpoint& checkpoint) {
        if (!CheckpointValid(checkpoint)) return false;

        // A checkpoint can resume equal or newer progress, never roll the local
        // training path backward or reduce an already-earned medal.
        if ((checkpoint.completedMask & completedMask_) != completedMask_) return false;
        for (std::size_t index = 0; index < LessonCount; ++index) {
            if (MedalRank(checkpoint.medals[index]) < MedalRank(medals_[index])) return false;
        }

        currentLesson_ = checkpoint.currentLesson;
        completedMask_ = checkpoint.completedMask;
        medals_ = checkpoint.medals;
        forgivingTimingAssist_ = checkpoint.forgivingTimingAssist;
        appliedLesson_ = ShadowbladeTrainingLesson::Complete;
        return true;
    }

private:
    static constexpr std::uint8_t LessonBit(std::size_t index) {
        return static_cast<std::uint8_t>(1u << index);
    }

    static constexpr std::size_t LessonIndex(ShadowbladeTrainingLesson lesson) {
        switch (lesson) {
        case ShadowbladeTrainingLesson::GuardFundamentals: return 0;
        case ShadowbladeTrainingLesson::DodgeFundamentals: return 1;
        case ShadowbladeTrainingLesson::MixedTiming: return 2;
        case ShadowbladeTrainingLesson::PressureHandling: return 3;
        case ShadowbladeTrainingLesson::BossRehearsal: return 4;
        case ShadowbladeTrainingLesson::Complete: return LessonCount;
        default: return LessonCount + 1;
        }
    }

    static constexpr ShadowbladeTrainingLesson LessonAtIndex(std::size_t index) {
        switch (index) {
        case 0: return ShadowbladeTrainingLesson::GuardFundamentals;
        case 1: return ShadowbladeTrainingLesson::DodgeFundamentals;
        case 2: return ShadowbladeTrainingLesson::MixedTiming;
        case 3: return ShadowbladeTrainingLesson::PressureHandling;
        case 4: return ShadowbladeTrainingLesson::BossRehearsal;
        default: return ShadowbladeTrainingLesson::Complete;
        }
    }

    static constexpr std::size_t FirstIncompleteIndex(std::uint8_t mask) {
        for (std::size_t index = 0; index < LessonCount; ++index) {
            if ((mask & LessonBit(index)) == 0) return index;
        }
        return LessonCount;
    }

    static constexpr bool PrefixMask(std::uint8_t mask) {
        return mask == 0 || mask == 1 || mask == 3 || mask == 7
            || mask == 15 || mask == AllLessonsMask;
    }

    static constexpr int MedalRank(ShadowbladeTrainingMedal medal) {
        switch (medal) {
        case ShadowbladeTrainingMedal::None: return 0;
        case ShadowbladeTrainingMedal::Bronze: return 1;
        case ShadowbladeTrainingMedal::Silver: return 2;
        case ShadowbladeTrainingMedal::Gold: return 3;
        default: return -1;
        }
    }

    static constexpr std::array<EnemyAttackPattern, 3> BossPatterns() {
        return {EnemyAttackPattern::QuickCut,
            EnemyAttackPattern::GuardBreaker,
            EnemyAttackPattern::RiftBurst};
    }

    static bool PlanContainsPattern(const ShadowbladeTrainingLessonPlan& plan,
        EnemyAttackPattern pattern) {
        for (std::size_t index = 0; index < plan.sequence.count; ++index) {
            if (plan.sequence.patterns[index] == pattern) return true;
        }
        return false;
    }

    static bool PatternProvenanceMatchesPlan(const DefensePracticeSession& session,
        const ShadowbladeTrainingLessonPlan& plan) {
        for (EnemyAttackPattern pattern : BossPatterns()) {
            const int attempts = session.PatternStats(pattern).attempts;
            if (PlanContainsPattern(plan, pattern)) {
                if (attempts < 1) return false;
            } else if (attempts != 0) {
                return false;
            }
        }
        return true;
    }

    bool SessionMatchesCurrentLesson(const DefensePracticeSession& session) const {
        const ShadowbladeTrainingLessonPlan plan = PlanForLesson(currentLesson_);
        return appliedLesson_ == currentLesson_
            && plan.sequence.count > 0
            && session.PracticeSequenceLength() == plan.sequence.count
            && session.Pace() == plan.pace
            && session.Goal() == plan.goal
            && session.GoalTarget() == plan.goalTarget
            && PatternProvenanceMatchesPlan(session, plan);
    }

    static bool CheckpointValid(const ShadowbladeTrainingCheckpoint& checkpoint) {
        if (checkpoint.schemaVersion != ShadowbladeTrainingCheckpoint::SchemaVersion
            || (checkpoint.completedMask & static_cast<std::uint8_t>(~AllLessonsMask)) != 0
            || !PrefixMask(checkpoint.completedMask)) {
            return false;
        }

        const ShadowbladeTrainingLesson expectedLesson =
            LessonAtIndex(FirstIncompleteIndex(checkpoint.completedMask));
        if (checkpoint.currentLesson != expectedLesson) return false;

        for (std::size_t index = 0; index < LessonCount; ++index) {
            const int rank = MedalRank(checkpoint.medals[index]);
            if (rank < 0) return false;
            const bool completed = (checkpoint.completedMask & LessonBit(index)) != 0;
            if (completed != (checkpoint.medals[index] != ShadowbladeTrainingMedal::None)) {
                return false;
            }
        }
        return true;
    }

    ShadowbladeTrainingLesson currentLesson_{ShadowbladeTrainingLesson::GuardFundamentals};
    ShadowbladeTrainingLesson appliedLesson_{ShadowbladeTrainingLesson::Complete};
    std::uint8_t completedMask_{};
    std::array<ShadowbladeTrainingMedal, LessonCount> medals_{};
    bool forgivingTimingAssist_{};
};

} // namespace Astral::Scene
