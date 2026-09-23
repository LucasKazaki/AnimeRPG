#pragma once

#include "Engine/Scene/DefensePracticeSession.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace Astral::Scene {

enum class ShadowbladeTrainingFocus : std::uint8_t {
    QuickCut,
    GuardBreaker,
    RiftBurst,
    MixedDefense,
    BossCycle,
};

enum class ShadowbladeTrainingChallenge : std::uint8_t {
    PerfectStreak,
    NoHitSequence,
    PatternMastery,
};

enum class ShadowbladeTrainingRecommendation : std::uint8_t {
    NeedMoreData,
    PracticeQuickCut,
    PracticeGuardBreaker,
    PracticeRiftBurst,
    ImprovePerfectTiming,
    MaintainForm,
};

struct ShadowbladeTrainingDrillPlan {
    ShadowbladeTrainingFocus focus{ShadowbladeTrainingFocus::MixedDefense};
    DefensePracticeSequence sequence{};
    DefensePracticePace pace{DefensePracticePace::Standard};
    DefenseTrainingGoal goal{DefenseTrainingGoal::PerfectDefense};
    int goalTarget{DefensePracticeSession::DefaultGoalTarget};
};

struct ShadowbladeTrainingDebrief {
    bool valid{};
    int resolvedAttempts{};
    int successfulDefenses{};
    int perfectDefenses{};
    int ordinaryDefenses{};
    int hitsTaken{};
    int damageTaken{};
    int accuracyPercent{};
    int perfectPercent{};
    int bestPerfectStreak{};
    int bestAlternatingChain{};
    std::int64_t baseScore{};
    int timeCoefficientPercent{};
    std::int64_t finalScore{};
    DefenseTrainingGrade grade{DefenseTrainingGrade::None};
};

struct ShadowbladeTrainingChallengeStatus {
    ShadowbladeTrainingChallenge challenge{ShadowbladeTrainingChallenge::PerfectStreak};
    int current{};
    int target{};
    bool complete{};
};

struct ShadowbladeTrainingTimingGuide {
    EnemyAttackPattern pattern{EnemyAttackPattern::QuickCut};
    float windupSeconds{};
    int damage{};
    int guardDamage{};
    bool blockable{};
    DefenseInput recommendedInput{DefenseInput::Guard};
    float selectedPerfectWindowSeconds{};
    float standardPerfectWindowSeconds{};
    float forgivingPerfectWindowSeconds{};
    float dodgeWindowSeconds{};
};

// Game-owned training policy layered over the authoritative defense-practice
// session. It owns no renderer, input device, enemy data, save backend, or
// reward economy. All mutations go through DefensePracticeSession.
class ShadowbladeTrainingCoach {
public:
    static bool PlanForFocus(ShadowbladeTrainingFocus focus,
        DefensePracticePace pace, ShadowbladeTrainingDrillPlan& outPlan) {
        if (!ValidPace(pace)) return false;

        ShadowbladeTrainingDrillPlan plan{};
        plan.focus = focus;
        plan.pace = pace;
        switch (focus) {
        case ShadowbladeTrainingFocus::QuickCut:
            plan.sequence.patterns[0] = EnemyAttackPattern::QuickCut;
            plan.sequence.count = 1;
            plan.goal = DefenseTrainingGoal::GuardDiscipline;
            break;
        case ShadowbladeTrainingFocus::GuardBreaker:
            plan.sequence.patterns[0] = EnemyAttackPattern::GuardBreaker;
            plan.sequence.count = 1;
            plan.goal = DefenseTrainingGoal::GuardDiscipline;
            break;
        case ShadowbladeTrainingFocus::RiftBurst:
            plan.sequence.patterns[0] = EnemyAttackPattern::RiftBurst;
            plan.sequence.count = 1;
            plan.goal = DefenseTrainingGoal::DodgeDiscipline;
            break;
        case ShadowbladeTrainingFocus::MixedDefense:
            plan.sequence.patterns[0] = EnemyAttackPattern::QuickCut;
            plan.sequence.patterns[1] = EnemyAttackPattern::RiftBurst;
            plan.sequence.count = 2;
            plan.goal = DefenseTrainingGoal::PerfectDefense;
            break;
        case ShadowbladeTrainingFocus::BossCycle:
            plan.sequence.patterns[0] = EnemyAttackPattern::QuickCut;
            plan.sequence.patterns[1] = EnemyAttackPattern::GuardBreaker;
            plan.sequence.patterns[2] = EnemyAttackPattern::RiftBurst;
            plan.sequence.count = 3;
            plan.goal = DefenseTrainingGoal::FreePractice;
            break;
        default:
            return false;
        }

        outPlan = plan;
        return true;
    }

    static bool ApplyDrill(DefensePracticeSession& session,
        const CombatSandbox& combat, const ShadowbladeActions& actions,
        ShadowbladeTrainingFocus focus, DefensePracticePace pace) {
        // A directly queued replacement can exist outside the practice-session
        // coordinator. First require the exact owners already bound by any prior
        // practice attack, then reject a live threat on either authoritative owner.
        if (!session.AcceptsPracticeObjects(combat, actions)
            || combat.HasPendingEnemyAttack() || actions.HasIncomingAttack()) {
            return false;
        }

        ShadowbladeTrainingDrillPlan plan{};
        if (!PlanForFocus(focus, pace, plan)) return false;

        DefensePracticeSession candidate = session;
        if (!candidate.ResetMetrics()) return false;
        if (candidate.Paused() && !candidate.SetPaused(false)) return false;
        if (candidate.PracticeSequenceLength() > 0
            && !candidate.ClearPracticeSequence()) {
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
        return true;
    }

    static ShadowbladeTrainingDebrief Debrief(const DefensePracticeSession& session,
        const CombatSandbox& combat) {
        ShadowbladeTrainingDebrief report{};
        const DefenseTrainingStats& stats = session.Stats();
        const std::int64_t perfect = std::max(0, stats.perfectDefenses);
        const std::int64_t ordinary = std::max(0, stats.ordinaryDefenses);
        const std::int64_t hits = std::max(0, stats.hitsTaken);
        const std::int64_t resolved = perfect + ordinary + hits;
        const std::int64_t successful = perfect + ordinary;
        const DefensePracticeScoreReport score = session.Score(combat);

        // Score() is deliberately ownership-aware. Require its resolved-attempt
        // witness to match the session metrics before composing a debrief, so a
        // caller cannot mix one session's stats with another combat instance.
        if (resolved <= 0 || score.resolvedAttempts != resolved) return report;

        report.valid = true;
        report.resolvedAttempts = ClampToInt(resolved);
        report.successfulDefenses = ClampToInt(successful);
        report.perfectDefenses = ClampToInt(perfect);
        report.ordinaryDefenses = ClampToInt(ordinary);
        report.hitsTaken = ClampToInt(hits);
        report.damageTaken = std::max(0, stats.damageTaken);
        report.bestPerfectStreak = std::max(0, stats.bestPerfectStreak);
        report.bestAlternatingChain = std::max(0, session.BestAlternatingChain());
        report.accuracyPercent = static_cast<int>(successful * 100 / resolved);
        report.perfectPercent = static_cast<int>(perfect * 100 / resolved);
        report.baseScore = score.baseScore;
        report.timeCoefficientPercent = score.timeCoefficientPercent;
        report.finalScore = score.score;
        report.grade = session.Grade();
        return report;
    }

    static ShadowbladeTrainingChallengeStatus ChallengeStatus(
        const DefensePracticeSession& session, ShadowbladeTrainingChallenge challenge) {
        ShadowbladeTrainingChallengeStatus status{};
        status.challenge = challenge;
        const DefenseTrainingStats& stats = session.Stats();

        switch (challenge) {
        case ShadowbladeTrainingChallenge::PerfectStreak:
            status.target = 3;
            status.current = std::min(status.target, std::max(0, stats.bestPerfectStreak));
            break;
        case ShadowbladeTrainingChallenge::NoHitSequence: {
            status.target = 5;
            const std::int64_t resolved = static_cast<std::int64_t>(std::max(0, stats.perfectDefenses))
                + static_cast<std::int64_t>(std::max(0, stats.ordinaryDefenses))
                + static_cast<std::int64_t>(std::max(0, stats.hitsTaken));
            status.current = stats.hitsTaken == 0
                ? std::min(status.target, ClampToInt(resolved))
                : 0;
            break;
        }
        case ShadowbladeTrainingChallenge::PatternMastery:
            status.target = 3;
            status.current = 0;
            for (EnemyAttackPattern pattern : CanonicalPatterns()) {
                if (session.PatternStats(pattern).perfectDefenses > 0) ++status.current;
            }
            break;
        default:
            return {};
        }

        status.complete = status.current >= status.target;
        return status;
    }

    static ShadowbladeTrainingRecommendation Recommendation(
        const DefensePracticeSession& session) {
        const DefenseTrainingStats& stats = session.Stats();
        const int resolved = SafeResolved(stats);
        if (resolved == 0) return ShadowbladeTrainingRecommendation::NeedMoreData;

        EnemyAttackPattern worstPattern = EnemyAttackPattern::QuickCut;
        bool foundPatternFailure = false;
        DefensePracticePatternStats worstStats{};
        for (EnemyAttackPattern pattern : CanonicalPatterns()) {
            const DefensePracticePatternStats current = session.PatternStats(pattern);
            if (current.hitsTaken <= 0 || current.attempts <= 0) continue;
            if (!foundPatternFailure
                || FailureRatioGreater(current, worstStats)) {
                foundPatternFailure = true;
                worstPattern = pattern;
                worstStats = current;
            }
        }
        if (foundPatternFailure) return RecommendationForPattern(worstPattern);

        if (stats.ordinaryDefenses > stats.perfectDefenses) {
            return ShadowbladeTrainingRecommendation::ImprovePerfectTiming;
        }
        return ShadowbladeTrainingRecommendation::MaintainForm;
    }

    static bool RetryDrill(DefensePracticeSession& session,
        const CombatSandbox& combat, const ShadowbladeActions& actions) {
        // ResetMetrics() can only see threats coordinated by this session. Require
        // the exact previously bound owners as well as idle authoritative owners
        // before clearing metrics, so an unrelated idle pair cannot bypass a live
        // replacement on the session's real pair.
        if (!session.AcceptsPracticeObjects(combat, actions)
            || combat.HasPendingEnemyAttack() || actions.HasIncomingAttack()) {
            return false;
        }
        return session.ResetMetrics();
    }

    static bool TimingGuide(EnemyAttackPattern pattern, DefenseTimingPreset preset,
        ShadowbladeTrainingTimingGuide& outGuide) {
        switch (preset) {
        case DefenseTimingPreset::Standard:
        case DefenseTimingPreset::Forgiving:
            break;
        default:
            return false;
        }

        CombatSandbox sandbox;
        if (!sandbox.QueueEnemyAttackPattern(pattern)) return false;
        const EnemyAttackPlan plan = sandbox.PendingEnemyAttack();

        ShadowbladeTrainingTimingGuide guide{};
        guide.pattern = plan.pattern;
        guide.windupSeconds = plan.windupSeconds;
        guide.damage = plan.damage;
        guide.guardDamage = plan.guardDamage;
        guide.blockable = plan.blockable;
        guide.recommendedInput = plan.blockable ? DefenseInput::Guard : DefenseInput::Dodge;
        guide.standardPerfectWindowSeconds =
            ShadowbladeActions::StandardPerfectDefenseWindowSeconds;
        guide.forgivingPerfectWindowSeconds =
            ShadowbladeActions::ForgivingPerfectDefenseWindowSeconds;
        guide.dodgeWindowSeconds = ShadowbladeActions::DodgeWindowSeconds;
        guide.selectedPerfectWindowSeconds = preset == DefenseTimingPreset::Forgiving
            ? guide.forgivingPerfectWindowSeconds
            : guide.standardPerfectWindowSeconds;
        outGuide = guide;
        return true;
    }

private:
    static constexpr std::array<EnemyAttackPattern, 3> CanonicalPatterns() {
        return {EnemyAttackPattern::QuickCut,
            EnemyAttackPattern::GuardBreaker,
            EnemyAttackPattern::RiftBurst};
    }

    static bool ValidPace(DefensePracticePace pace) {
        switch (pace) {
        case DefensePracticePace::Learning:
        case DefensePracticePace::Standard:
        case DefensePracticePace::Expert:
            return true;
        default:
            return false;
        }
    }

    static int ClampToInt(std::int64_t value) {
        if (value <= 0) return 0;
        const std::int64_t maximum = static_cast<std::int64_t>(
            std::numeric_limits<int>::max());
        return static_cast<int>(std::min(value, maximum));
    }

    static int SafeResolved(const DefenseTrainingStats& stats) {
        const std::int64_t resolved = static_cast<std::int64_t>(std::max(0, stats.perfectDefenses))
            + static_cast<std::int64_t>(std::max(0, stats.ordinaryDefenses))
            + static_cast<std::int64_t>(std::max(0, stats.hitsTaken));
        return ClampToInt(resolved);
    }

    static bool FailureRatioGreater(const DefensePracticePatternStats& lhs,
        const DefensePracticePatternStats& rhs) {
        if (rhs.attempts <= 0) return true;
        return static_cast<std::int64_t>(lhs.hitsTaken) * rhs.attempts
            > static_cast<std::int64_t>(rhs.hitsTaken) * lhs.attempts;
    }

    static ShadowbladeTrainingRecommendation RecommendationForPattern(
        EnemyAttackPattern pattern) {
        switch (pattern) {
        case EnemyAttackPattern::QuickCut:
            return ShadowbladeTrainingRecommendation::PracticeQuickCut;
        case EnemyAttackPattern::GuardBreaker:
            return ShadowbladeTrainingRecommendation::PracticeGuardBreaker;
        case EnemyAttackPattern::RiftBurst:
            return ShadowbladeTrainingRecommendation::PracticeRiftBurst;
        default:
            return ShadowbladeTrainingRecommendation::NeedMoreData;
        }
    }
};

} // namespace Astral::Scene
