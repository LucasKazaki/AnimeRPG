#pragma once

#include "Engine/Scene/ShadowbladeTrainingCoach.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace Astral::Scene {

enum class ShadowbladeTrainingHubState : std::uint8_t {
    Locked,
    Ready,
    Active,
    Debrief,
};

struct ShadowbladeTrainingDamageWindow {
    bool valid{};
    double windowSeconds{};
    std::int64_t damage{};
    int hits{};
};

struct ShadowbladeTrainingHubFeedback {
    ShadowbladeTrainingHubState state{ShadowbladeTrainingHubState::Locked};
    ShadowbladeTrainingFocus focus{ShadowbladeTrainingFocus::MixedDefense};
    DefensePracticePace pace{DefensePracticePace::Standard};
    int targetAttempts{};
    int resolvedAttempts{};
    bool paused{};
    ShadowbladeTrainingDebrief debrief{};
    ShadowbladeTrainingRecommendation recommendation{
        ShadowbladeTrainingRecommendation::NeedMoreData};
    std::array<ShadowbladeTrainingChallengeStatus, 3> challenges{};
    ShadowbladeTrainingTimingGuide timingGuide{};
    bool timingGuideValid{};
    ShadowbladeTrainingDamageWindow recentDamage{};
};

// Game-owned integration layer that attaches the pass-24 training coach to an
// existing gameplay owner. It deliberately reuses the authoritative combat,
// Shadowblade, and defense-practice domains. It owns no renderer, device input,
// platform loop, save backend, enemy definitions, or reward economy.
class ShadowbladeTrainingHub {
public:
    static constexpr int MinimumAttempts = 1;
    static constexpr int MaximumAttempts = 10;
    static constexpr int DefaultAttempts = 5;
    static constexpr double DamageWindowSeconds = 20.0;
    static constexpr double DamageSampleIntervalSeconds = 0.20;
    static constexpr std::size_t MaximumDamageSamples = 128;
    static constexpr std::size_t MaximumActivitiesPerSample = 16;

    bool Unlock() {
        if (state_ != ShadowbladeTrainingHubState::Locked) return false;
        ShadowbladeTrainingDrillPlan plan{};
        if (!ShadowbladeTrainingCoach::PlanForFocus(focus_, pace_, plan)) return false;
        state_ = ShadowbladeTrainingHubState::Ready;
        return true;
    }

    bool Configure(ShadowbladeTrainingFocus focus, DefensePracticePace pace,
        int targetAttempts) {
        if (state_ == ShadowbladeTrainingHubState::Locked
            || state_ == ShadowbladeTrainingHubState::Active
            || targetAttempts < MinimumAttempts || targetAttempts > MaximumAttempts) {
            return false;
        }
        ShadowbladeTrainingDrillPlan plan{};
        if (!ShadowbladeTrainingCoach::PlanForFocus(focus, pace, plan)) return false;
        if (focus_ == focus && pace_ == pace && targetAttempts_ == targetAttempts) {
            return false;
        }
        focus_ = focus;
        pace_ = pace;
        targetAttempts_ = targetAttempts;
        if (state_ == ShadowbladeTrainingHubState::Debrief) {
            state_ = ShadowbladeTrainingHubState::Ready;
        }
        return true;
    }

    bool Start(CombatSandbox& combat, ShadowbladeActions& actions) {
        if ((state_ != ShadowbladeTrainingHubState::Ready
                && state_ != ShadowbladeTrainingHubState::Debrief)
            || !AcceptsOwners(combat, actions)
            || combat.HasPendingEnemyAttack() || actions.HasIncomingAttack()) {
            return false;
        }

        DefensePracticeSession candidate;
        if (!ShadowbladeTrainingCoach::ApplyDrill(
                candidate, combat, actions, focus_, pace_)) {
            return false;
        }

        if (combat.TargetMode() != TrainingTargetMode::Endless) {
            if (!combat.SetTrainingTargetMode(TrainingTargetMode::Endless)) return false;
        } else {
            combat.ResetTrainingSession();
        }
        actions.ResetTransientStatePreservingLoadout();

        session_ = candidate;
        if (!session_.QueueNextAttack(combat, actions)) {
            state_ = ShadowbladeTrainingHubState::Ready;
            return false;
        }

        boundCombat_ = &combat;
        boundActions_ = &actions;
        state_ = ShadowbladeTrainingHubState::Active;
        ResetDamageWindow(combat);
        return true;
    }

    DefenseReport Defend(CombatSandbox& combat, ShadowbladeActions& actions,
        DefenseInput input) {
        if (state_ != ShadowbladeTrainingHubState::Active
            || !OwnsOwners(combat, actions)
            || (input != DefenseInput::Guard && input != DefenseInput::Dodge)) {
            return {DefenseResult::NoThreat, 0, 0, false, 0.0f};
        }
        const DefenseReport report = session_.TryDefend(combat, actions, input);
        ObserveCombat(combat);
        FinishIfRunEnded(actions);
        return report;
    }

    // This explicit domain step owns the combat/Shadowblade clocks for training.
    // Do not call it in addition to a platform loop that already advanced those
    // same owners for the frame. The current Win32 loop does not call this method.
    bool Advance(CombatSandbox& combat, ShadowbladeActions& actions,
        float deltaSeconds) {
        if (state_ != ShadowbladeTrainingHubState::Active
            || !OwnsOwners(combat, actions)) {
            return false;
        }
        const bool resolved = session_.AdvanceTime(combat, actions, deltaSeconds);
        ObserveCombat(combat);
        if (FinishIfRunEnded(actions)) return true;

        bool queued = false;
        if (!session_.Paused() && !combat.HasPendingEnemyAttack()
            && !actions.HasIncomingAttack()) {
            queued = session_.QueueNextAttack(combat, actions);
        }
        return resolved || queued;
    }

    bool SetPaused(bool paused) {
        if (state_ != ShadowbladeTrainingHubState::Active) return false;
        return session_.SetPaused(paused);
    }

    void ObserveCombat(const CombatSandbox& combat) {
        // Rolling damage telemetry belongs only to the active training run. The
        // final Defend/Advance call samples before transition to Debrief, after
        // which later encounter reuse of the same combat owner must not mutate
        // the completed run's feedback.
        if (state_ != ShadowbladeTrainingHubState::Active
            || (boundCombat_ && boundCombat_ != &combat)) {
            return;
        }
        const double now = combat.ElapsedSecondsPrecise();
        if (!std::isfinite(now) || now < 0.0) return;
        const TrainingStats& stats = combat.Stats();
        const std::int64_t safeDamage = std::max<std::int64_t>(0, stats.totalDamage);
        const int safeHits = std::max(0, stats.hitCount);
        if (sampleCount_ == 0 || now < latestObservedSeconds_
            || safeDamage < latestDamage_ || safeHits < latestHits_) {
            ResetDamageWindow(combat);
            return;
        }

        const std::int64_t damageDelta = safeDamage - latestDamage_;
        const int hitDelta = safeHits - latestHits_;
        if (damageDelta > 0 || hitDelta > 0) {
            if (pendingActivityCount_ < MaximumActivitiesPerSample) {
                pendingActivities_[pendingActivityCount_++] = {
                    now,
                    damageDelta,
                    hitDelta,
                };
            } else {
                // Do not silently smear more activity into an older timestamp.
                // A pathological caller that produces more than sixteen distinct
                // stat changes inside one 200 ms sample bucket makes this run's
                // telemetry invalid until the next training start/reset.
                damageTelemetryOverflow_ = true;
            }
        }

        latestObservedSeconds_ = now;
        latestDamage_ = safeDamage;
        latestHits_ = safeHits;
        if (now - lastStoredSampleSeconds_ >= DamageSampleIntervalSeconds) {
            AddDamageSample(now);
        }
    }

    ShadowbladeTrainingHubFeedback Feedback(const CombatSandbox& combat) const {
        ShadowbladeTrainingHubFeedback feedback{};
        feedback.state = state_;
        feedback.focus = focus_;
        feedback.pace = pace_;
        feedback.targetAttempts = targetAttempts_;
        if (boundCombat_ && boundCombat_ != &combat) return feedback;

        // Run-derived metrics belong only to the currently active run or its
        // completed debrief. Reconfiguring a debrief transitions to Ready, so
        // stale results from the previous drill must not be presented alongside
        // the newly selected focus, pace, or attempt target.
        if (state_ == ShadowbladeTrainingHubState::Active
            || state_ == ShadowbladeTrainingHubState::Debrief) {
            feedback.resolvedAttempts = ResolvedAttempts();
            feedback.paused = session_.Paused();
            feedback.debrief = ShadowbladeTrainingCoach::Debrief(session_, combat);
            feedback.recommendation = ShadowbladeTrainingCoach::Recommendation(session_);
            feedback.challenges[0] = ShadowbladeTrainingCoach::ChallengeStatus(
                session_, ShadowbladeTrainingChallenge::PerfectStreak);
            feedback.challenges[1] = ShadowbladeTrainingCoach::ChallengeStatus(
                session_, ShadowbladeTrainingChallenge::NoHitSequence);
            feedback.challenges[2] = ShadowbladeTrainingCoach::ChallengeStatus(
                session_, ShadowbladeTrainingChallenge::PatternMastery);
            feedback.recentDamage = RecentDamage();
        }

        DefenseTimingPreset timingPreset = DefenseTimingPreset::Standard;
        if (boundActions_
            && boundActions_->PerfectDefenseWindowSeconds()
                == ShadowbladeActions::ForgivingPerfectDefenseWindowSeconds) {
            timingPreset = DefenseTimingPreset::Forgiving;
        }

        EnemyAttackPattern guidePattern{EnemyAttackPattern::QuickCut};
        bool guidePatternValid = false;
        if (state_ == ShadowbladeTrainingHubState::Active) {
            // The practice session's cue validates the exact combat-plan and
            // Shadowblade incoming-attack generations it owns. Never describe a
            // replacement planner threat that is not the attack approaching the
            // bound player action owner.
            if (boundActions_ && boundCombat_ == &combat) {
                const DefenseTrainingCue cue = session_.Cue(combat, *boundActions_);
                if (cue.phase != DefenseTrainingCuePhase::None) {
                    guidePattern = cue.pattern;
                    guidePatternValid = true;
                }
            }
        } else {
            ShadowbladeTrainingDrillPlan plan{};
            if (ShadowbladeTrainingCoach::PlanForFocus(focus_, pace_, plan)
                && plan.sequence.count > 0) {
                guidePattern = plan.sequence.patterns[0];
                guidePatternValid = true;
            }
        }
        if (guidePatternValid) {
            feedback.timingGuideValid = ShadowbladeTrainingCoach::TimingGuide(
                guidePattern, timingPreset, feedback.timingGuide);
        }
        return feedback;
    }

    ShadowbladeTrainingHubState State() const { return state_; }
    bool Unlocked() const { return state_ != ShadowbladeTrainingHubState::Locked; }
    bool Active() const { return state_ == ShadowbladeTrainingHubState::Active; }
    ShadowbladeTrainingFocus Focus() const { return focus_; }
    DefensePracticePace Pace() const { return pace_; }
    int TargetAttempts() const { return targetAttempts_; }
    const DefensePracticeSession& Session() const { return session_; }

    // Encounter transitions may reuse an unbound hub, but once a training run
    // establishes authoritative owners they must use that exact pair. This
    // exposes only the ownership predicate, never the stored pointers.
    bool AcceptsOwnerPair(const CombatSandbox& combat,
        const ShadowbladeActions& actions) const {
        return AcceptsOwners(combat, actions);
    }

private:
    struct DamageActivity {
        double activitySeconds{};
        std::int64_t damage{};
        int hits{};
    };

    struct DamageSample {
        double storedSeconds{};
        std::array<DamageActivity, MaximumActivitiesPerSample> activities{};
        std::size_t activityCount{};
    };

    bool AcceptsOwners(const CombatSandbox& combat,
        const ShadowbladeActions& actions) const {
        return (!boundCombat_ || boundCombat_ == &combat)
            && (!boundActions_ || boundActions_ == &actions);
    }

    bool OwnsOwners(const CombatSandbox& combat,
        const ShadowbladeActions& actions) const {
        return boundCombat_ == &combat && boundActions_ == &actions;
    }

    int ResolvedAttempts() const {
        const DefenseTrainingStats& stats = session_.Stats();
        const std::int64_t resolved = static_cast<std::int64_t>(std::max(0, stats.perfectDefenses))
            + static_cast<std::int64_t>(std::max(0, stats.ordinaryDefenses))
            + static_cast<std::int64_t>(std::max(0, stats.hitsTaken));
        if (resolved <= 0) return 0;
        return resolved > std::numeric_limits<int>::max()
            ? std::numeric_limits<int>::max()
            : static_cast<int>(resolved);
    }

    bool FinishIfRunEnded(const ShadowbladeActions& actions) {
        if (state_ != ShadowbladeTrainingHubState::Active) return false;
        if (ResolvedAttempts() < targetAttempts_ && actions.PlayerHealth() > 0) {
            return false;
        }
        state_ = ShadowbladeTrainingHubState::Debrief;
        return true;
    }

    void ResetDamageWindow(const CombatSandbox& combat) {
        damageSamples_ = {};
        sampleCount_ = 0;
        nextSample_ = 0;
        latestObservedSeconds_ = 0.0;
        lastStoredSampleSeconds_ = 0.0;
        damageWindowStartSeconds_ = 0.0;
        latestDamage_ = 0;
        latestHits_ = 0;
        pendingActivities_ = {};
        pendingActivityCount_ = 0;
        damageTelemetryOverflow_ = false;
        const double now = combat.ElapsedSecondsPrecise();
        if (!std::isfinite(now) || now < 0.0) return;
        const TrainingStats& stats = combat.Stats();
        latestObservedSeconds_ = now;
        lastStoredSampleSeconds_ = now;
        damageWindowStartSeconds_ = now;
        latestDamage_ = std::max<std::int64_t>(0, stats.totalDamage);
        latestHits_ = std::max(0, stats.hitCount);
        AddDamageSample(now);
    }

    void AddDamageSample(double storedSeconds) {
        DamageSample sample{};
        sample.storedSeconds = storedSeconds;
        sample.activities = pendingActivities_;
        sample.activityCount = pendingActivityCount_;
        damageSamples_[nextSample_] = sample;
        nextSample_ = (nextSample_ + 1) % MaximumDamageSamples;
        if (sampleCount_ < MaximumDamageSamples) ++sampleCount_;
        lastStoredSampleSeconds_ = storedSeconds;
        pendingActivities_ = {};
        pendingActivityCount_ = 0;
    }

    ShadowbladeTrainingDamageWindow RecentDamage() const {
        ShadowbladeTrainingDamageWindow window{};
        if (sampleCount_ == 0 || damageTelemetryOverflow_
            || !std::isfinite(latestObservedSeconds_)
            || !std::isfinite(damageWindowStartSeconds_)) {
            return window;
        }

        const double cutoff = std::max(damageWindowStartSeconds_,
            latestObservedSeconds_ - DamageWindowSeconds);
        std::int64_t damage = 0;
        int hits = 0;
        const auto accumulateActivity = [&](const DamageActivity& activity) {
            if (!std::isfinite(activity.activitySeconds)
                || activity.activitySeconds < cutoff
                || activity.activitySeconds > latestObservedSeconds_) {
                return;
            }
            damage += std::max<std::int64_t>(0, activity.damage);
            hits += std::max(0, activity.hits);
        };

        for (std::size_t index = 0; index < sampleCount_; ++index) {
            const DamageSample& sample = damageSamples_[index];
            const std::size_t count = std::min(
                sample.activityCount, MaximumActivitiesPerSample);
            for (std::size_t activityIndex = 0; activityIndex < count; ++activityIndex) {
                accumulateActivity(sample.activities[activityIndex]);
            }
        }
        const std::size_t pendingCount = std::min(
            pendingActivityCount_, MaximumActivitiesPerSample);
        for (std::size_t activityIndex = 0; activityIndex < pendingCount; ++activityIndex) {
            accumulateActivity(pendingActivities_[activityIndex]);
        }

        window.valid = true;
        window.windowSeconds = std::min(DamageWindowSeconds,
            std::max(0.0, latestObservedSeconds_ - damageWindowStartSeconds_));
        window.damage = std::max<std::int64_t>(0, damage);
        window.hits = std::max(0, hits);
        return window;
    }

    ShadowbladeTrainingHubState state_{ShadowbladeTrainingHubState::Locked};
    ShadowbladeTrainingFocus focus_{ShadowbladeTrainingFocus::MixedDefense};
    DefensePracticePace pace_{DefensePracticePace::Standard};
    int targetAttempts_{DefaultAttempts};
    DefensePracticeSession session_{};
    const CombatSandbox* boundCombat_{};
    const ShadowbladeActions* boundActions_{};
    std::array<DamageSample, MaximumDamageSamples> damageSamples_{};
    std::size_t sampleCount_{};
    std::size_t nextSample_{};
    double latestObservedSeconds_{};
    double lastStoredSampleSeconds_{};
    double damageWindowStartSeconds_{};
    std::int64_t latestDamage_{};
    int latestHits_{};
    std::array<DamageActivity, MaximumActivitiesPerSample> pendingActivities_{};
    std::size_t pendingActivityCount_{};
    bool damageTelemetryOverflow_{};
};

} // namespace Astral::Scene