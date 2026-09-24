#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace Astral::Scene {

enum class ShadowCryptEnemyRole : std::uint8_t {
    RiftSkirmisher,
    VeilChanneler,
    GraveboundBulwark,
    Count,
};

enum class ShadowCryptThreatCue : std::uint8_t {
    None,
    Evade,
    Interrupt,
    Brace,
};

enum class ShadowCryptDefenseResponse : std::uint8_t {
    None,
    Dodge,
    Interrupt,
    Guard,
};

enum class ShadowCryptAttackStyle : std::uint8_t {
    Light,
    Heavy,
    Counter,
};

enum class ShadowCryptSkirmishTier : std::uint8_t {
    EntrySeal,
    ArchiveGallery,
    RiftNave,
    Count,
};

enum class ShadowCryptThreatVariant : std::uint8_t {
    RiftLunge,
    RiftCrosscut,
    VeilBind,
    VeilBurst,
    GraveHammer,
    GraveRush,
    Count,
};

enum class ShadowCryptStatusKind : std::uint8_t {
    RiftMark,
    VeilWard,
    BulwarkGuardLink,
    Count,
};

struct ShadowCryptEnemyState {
    ShadowCryptEnemyRole role{ShadowCryptEnemyRole::RiftSkirmisher};
    int health{};
    int maxHealth{};
    int posture{};
    int maxPosture{};
    bool staggered{};
    bool defeated{};
};

struct ShadowCryptThreatTelegraph {
    bool valid{};
    std::size_t enemyIndex{};
    ShadowCryptEnemyRole role{ShadowCryptEnemyRole::RiftSkirmisher};
    ShadowCryptThreatVariant variant{ShadowCryptThreatVariant::RiftLunge};
    ShadowCryptThreatCue cue{ShadowCryptThreatCue::None};
    ShadowCryptDefenseResponse expected{ShadowCryptDefenseResponse::None};
    double responseWindowSeconds{};
    int failureDamage{};
    bool defensiveCounterAllowed{};
    bool enraged{};
    std::uint8_t patternStep{};
};

struct ShadowCryptDefenseReport {
    bool accepted{};
    bool successful{};
    bool precision{};
    bool openedCounter{};
    bool interruptedChannel{};
    bool staggerOpened{};
    bool breachOpened{};
    bool pressureApplied{};
    bool pressurePurged{};
    int postureDamageApplied{};
    int damageTaken{};
    std::size_t enemyIndex{};
    std::uint8_t shadowFlow{};
};

struct ShadowCryptAttackReport {
    bool accepted{};
    std::size_t enemyIndex{};
    int healthDamage{};
    int postureDamage{};
    int wardAbsorbed{};
    int postureProtected{};
    bool staggerOpened{};
    bool staggerConsumed{};
    bool defeated{};
    bool encounterComplete{};
    bool breachConsumed{};
    bool weaknessExploited{};
    bool flowBreakConsumed{};
    bool pressurePurged{};
};

struct ShadowCryptTargetRecommendation {
    bool valid{};
    std::size_t enemyIndex{};
    ShadowCryptEnemyRole role{ShadowCryptEnemyRole::RiftSkirmisher};
    int priority{};
};

struct ShadowCryptThreatQueue {
    static constexpr std::size_t Capacity = 3;
    std::array<ShadowCryptThreatTelegraph, Capacity> threats{};
    std::size_t count{};
};

struct ShadowCryptRoleIntel {
    bool valid{};
    std::size_t enemyIndex{};
    ShadowCryptEnemyRole role{ShadowCryptEnemyRole::RiftSkirmisher};
    ShadowCryptAttackStyle recommendedAttack{ShadowCryptAttackStyle::Heavy};
    bool breachOpening{};
    bool enraged{};
};

struct ShadowCryptStatusEntry {
    bool active{};
    ShadowCryptStatusKind kind{ShadowCryptStatusKind::RiftMark};
    std::size_t sourceEnemyIndex{};
    std::size_t targetEnemyIndex{};
    int magnitude{};
};

struct ShadowCryptStatusLedger {
    static constexpr std::size_t Capacity = 5;
    std::array<ShadowCryptStatusEntry, Capacity> entries{};
    std::size_t count{};
};

// Game-domain Shadow Crypt room combat. This models original enemy roles and
// readable counterplay without taking renderer, input, animation, audio, or
// generic engine ownership.
class ShadowCryptSkirmish {
public:
    static constexpr std::size_t EnemyCapacity = 3;
    static constexpr std::uint8_t MaxShadowFlow = 3;

    bool Begin(ShadowCryptSkirmishTier tier) {
        if (active_ || !ValidTier(tier)) return false;
        Reset();
        tier_ = tier;
        active_ = true;
        switch (tier) {
        case ShadowCryptSkirmishTier::EntrySeal:
            enemyCount_ = 2;
            enemies_[0] = MakeEnemy(ShadowCryptEnemyRole::RiftSkirmisher, 80, 40);
            enemies_[1] = MakeEnemy(ShadowCryptEnemyRole::GraveboundBulwark, 120, 70);
            break;
        case ShadowCryptSkirmishTier::ArchiveGallery:
            enemyCount_ = 2;
            enemies_[0] = MakeEnemy(ShadowCryptEnemyRole::VeilChanneler, 90, 45);
            enemies_[1] = MakeEnemy(ShadowCryptEnemyRole::RiftSkirmisher, 95, 45);
            break;
        case ShadowCryptSkirmishTier::RiftNave:
            enemyCount_ = 3;
            enemies_[0] = MakeEnemy(ShadowCryptEnemyRole::VeilChanneler, 110, 55);
            enemies_[1] = MakeEnemy(ShadowCryptEnemyRole::GraveboundBulwark, 150, 85);
            enemies_[2] = MakeEnemy(ShadowCryptEnemyRole::RiftSkirmisher, 110, 50);
            break;
        case ShadowCryptSkirmishTier::Count:
            return false;
        }
        RefreshThreat();
        return true;
    }

    bool Replay() {
        if (active_ || !complete_ || !ValidTier(tier_)) return false;
        const ShadowCryptSkirmishTier replayTier = tier_;
        if (!Begin(replayTier)) return false;
        replayMode_ = true;
        return true;
    }

    bool Cancel() {
        if (!active_) return false;
        Reset();
        return true;
    }

    bool Active() const { return active_; }
    bool Complete() const { return complete_; }
    bool ReplayMode() const { return replayMode_; }
    ShadowCryptSkirmishTier Tier() const { return tier_; }
    std::size_t EnemyCount() const { return enemyCount_; }
    std::uint8_t ShadowFlow() const { return shadowFlow_; }
    bool ShadowFlowReady() const { return shadowFlow_ == MaxShadowFlow; }

    ShadowCryptEnemyState Enemy(std::size_t index) const {
        return index < enemyCount_ ? enemies_[index] : ShadowCryptEnemyState{};
    }

    ShadowCryptRoleIntel EnemyIntel(std::size_t index) const {
        ShadowCryptRoleIntel intel{};
        if (index >= enemyCount_) return intel;
        intel.valid = true;
        intel.enemyIndex = index;
        intel.role = enemies_[index].role;
        intel.recommendedAttack = RecommendedAttack(enemies_[index].role);
        intel.breachOpening = breachOpenings_[index] && !enemies_[index].defeated;
        intel.enraged = enemies_[index].health > 0
            && enemies_[index].health * 2 <= enemies_[index].maxHealth;
        return intel;
    }

    ShadowCryptStatusLedger StatusLedger() const {
        ShadowCryptStatusLedger ledger{};
        auto append = [&](ShadowCryptStatusKind kind, std::size_t source,
                          std::size_t target, int magnitude) {
            if (ledger.count >= ShadowCryptStatusLedger::Capacity) return;
            ledger.entries[ledger.count++] = {true, kind, source, target, magnitude};
        };
        if (RiftMarkActive()) {
            append(ShadowCryptStatusKind::RiftMark, riftMarkSource_, EnemyCapacity,
                RiftMarkDamageBonus);
        }
        for (std::size_t i = 0; i < enemyCount_; ++i) {
            if (VeilWardActive(i)) {
                append(ShadowCryptStatusKind::VeilWard, veilWardSource_[i], i,
                    veilWard_[i]);
            }
        }
        if (GuardLinkActive()) {
            append(ShadowCryptStatusKind::BulwarkGuardLink, guardLinkSource_,
                guardLinkedTarget_, BulwarkGuardPostureReduction);
        }
        return ledger;
    }

    ShadowCryptThreatTelegraph CurrentThreat() const {
        if (!active_ || complete_ || threatIndex_ >= enemyCount_
            || enemies_[threatIndex_].defeated) return {};
        return ThreatForIndex(threatIndex_);
    }

    ShadowCryptThreatQueue ThreatQueue(
        std::size_t maxCount = ShadowCryptThreatQueue::Capacity) const {
        ShadowCryptThreatQueue queue{};
        if (!active_ || complete_ || enemyCount_ == 0 || maxCount == 0) return queue;
        const std::size_t wanted = std::min(maxCount, ShadowCryptThreatQueue::Capacity);
        for (std::size_t offset = 0; offset < enemyCount_ && queue.count < wanted; ++offset) {
            const std::size_t index = (threatIndex_ + offset) % enemyCount_;
            if (enemies_[index].defeated) continue;
            const ShadowCryptThreatTelegraph threat = ThreatForIndex(index);
            if (threat.valid) queue.threats[queue.count++] = threat;
        }
        return queue;
    }

    ShadowCryptDefenseReport ResolveThreat(ShadowCryptDefenseResponse response,
        double reactionSeconds) {
        ShadowCryptDefenseReport report{};
        const auto threat = CurrentThreat();
        if (!threat.valid || !ValidResponse(response)
            || !std::isfinite(reactionSeconds) || reactionSeconds < 0.0) return report;
        report.accepted = true;
        report.enemyIndex = threat.enemyIndex;
        report.successful = response == threat.expected
            && reactionSeconds <= threat.responseWindowSeconds;
        if (report.successful) {
            report.precision = reactionSeconds
                <= threat.responseWindowSeconds * PrecisionResponseFraction;
            report.openedCounter = threat.defensiveCounterAllowed;
            counterTarget_ = report.openedCounter ? threat.enemyIndex : EnemyCapacity;
            precisionCounterTarget_ =
                report.openedCounter && report.precision ? threat.enemyIndex : EnemyCapacity;
            report.interruptedChannel =
                threat.role == ShadowCryptEnemyRole::VeilChanneler
                && response == ShadowCryptDefenseResponse::Interrupt;
            breachOpenings_[threat.enemyIndex] = true;
            report.breachOpened = true;
            shadowFlow_ = std::min<std::uint8_t>(MaxShadowFlow,
                static_cast<std::uint8_t>(shadowFlow_ + 1));
            if (report.precision) {
                report.pressurePurged = ClearPressureFromSource(threat.enemyIndex);
            }
            if (threat.role == ShadowCryptEnemyRole::GraveboundBulwark
                && response == ShadowCryptDefenseResponse::Guard) {
                auto& enemy = enemies_[threat.enemyIndex];
                if (!enemy.staggered && !enemy.defeated) {
                    const int postureBefore = enemy.posture;
                    enemy.posture = std::max(0, enemy.posture - BracePostureDamage);
                    report.postureDamageApplied = postureBefore - enemy.posture;
                    if (enemy.posture == 0) {
                        enemy.staggered = true;
                        report.staggerOpened = true;
                        ClearGuardLinkFromSource(threat.enemyIndex);
                    }
                }
            }
        } else {
            const bool consumedMark = RiftMarkActive();
            const int damageBefore = damageTaken_;
            damageTaken_ = std::min(MaxTrackedDamage, damageTaken_ + threat.failureDamage);
            report.damageTaken = damageTaken_ - damageBefore;
            if (consumedMark) ClearRiftMark();
            counterTarget_ = EnemyCapacity;
            precisionCounterTarget_ = EnemyCapacity;
            breachOpenings_[threat.enemyIndex] = false;
            shadowFlow_ = 0;
            report.pressureApplied = ApplyPressure(threat.enemyIndex, threat.role);
        }
        report.shadowFlow = shadowFlow_;
        AdvanceThreatPattern(threat.enemyIndex);
        AdvanceThreatCursor();
        return report;
    }

    ShadowCryptAttackReport AttackTarget(std::size_t index, ShadowCryptAttackStyle style) {
        ShadowCryptAttackReport report{};
        if (!active_ || complete_ || index >= enemyCount_ || !ValidAttackStyle(style)
            || enemies_[index].defeated) return report;
        auto& enemy = enemies_[index];
        report.accepted = true;
        report.enemyIndex = index;

        int healthDamage = 0;
        int postureDamage = 0;
        switch (style) {
        case ShadowCryptAttackStyle::Light:
            healthDamage = enemy.role == ShadowCryptEnemyRole::GraveboundBulwark
                && !enemy.staggered ? 8 : 20;
            postureDamage = 12;
            break;
        case ShadowCryptAttackStyle::Heavy:
            healthDamage = enemy.role == ShadowCryptEnemyRole::GraveboundBulwark
                && !enemy.staggered ? 14 : 28;
            postureDamage = 28;
            break;
        case ShadowCryptAttackStyle::Counter:
            if (counterTarget_ != index) {
                report.accepted = false;
                return report;
            }
            healthDamage = 36;
            postureDamage = 24;
            if (precisionCounterTarget_ == index) {
                healthDamage += PrecisionCounterHealthBonus;
                postureDamage += PrecisionCounterPostureBonus;
            }
            counterTarget_ = EnemyCapacity;
            precisionCounterTarget_ = EnemyCapacity;
            break;
        }

        if (breachOpenings_[index]) {
            report.breachConsumed = true;
            breachOpenings_[index] = false;
            if (style == RecommendedAttack(enemy.role)) {
                report.weaknessExploited = true;
                healthDamage += WeaknessHealthBonus;
                postureDamage += WeaknessPostureBonus;
                report.pressurePurged = ClearPressureFromSource(index);
            }
        }
        if (style == ShadowCryptAttackStyle::Heavy && ShadowFlowReady()) {
            report.flowBreakConsumed = true;
            shadowFlow_ = 0;
            healthDamage += FlowBreakHealthBonus;
            postureDamage += FlowBreakPostureBonus;
        }

        if (GuardLinkActive() && guardLinkedTarget_ == index) {
            const int protectedPosture = std::min(BulwarkGuardPostureReduction, postureDamage);
            postureDamage -= protectedPosture;
            report.postureProtected = protectedPosture;
            ClearGuardLink();
        }

        if (enemy.staggered) {
            healthDamage += 20;
            enemy.staggered = false;
            enemy.posture = enemy.maxPosture;
            report.staggerConsumed = true;
        } else {
            const int postureBefore = enemy.posture;
            enemy.posture = std::max(0, enemy.posture - postureDamage);
            report.postureDamage = postureBefore - enemy.posture;
            if (enemy.posture == 0) {
                enemy.staggered = true;
                report.staggerOpened = true;
                if (enemy.role == ShadowCryptEnemyRole::GraveboundBulwark) {
                    ClearGuardLinkFromSource(index);
                }
            }
        }

        if (VeilWardActive(index)) {
            report.wardAbsorbed = std::min(veilWard_[index], healthDamage);
            veilWard_[index] -= report.wardAbsorbed;
            healthDamage -= report.wardAbsorbed;
            if (veilWard_[index] == 0) veilWardSource_[index] = EnemyCapacity;
        }
        const int beforeHealth = enemy.health;
        enemy.health = std::max(0, enemy.health - healthDamage);
        report.healthDamage = beforeHealth - enemy.health;
        if (enemy.health == 0) {
            enemy.defeated = true;
            enemy.staggered = false;
            report.staggerOpened = false;
            report.defeated = true;
            breachOpenings_[index] = false;
            veilWard_[index] = 0;
            veilWardSource_[index] = EnemyCapacity;
            ClearPressureFromSource(index);
            if (guardLinkedTarget_ == index) ClearGuardLink();
            if (lockedTarget_ == index) lockedTarget_ = EnemyCapacity;
            if (counterTarget_ == index) counterTarget_ = EnemyCapacity;
            if (precisionCounterTarget_ == index) precisionCounterTarget_ = EnemyCapacity;
        }
        complete_ = AllDefeated();
        if (complete_) {
            active_ = false;
        } else if (threatIndex_ >= enemyCount_ || enemies_[threatIndex_].defeated) {
            RefreshThreat();
        }
        report.encounterComplete = complete_;
        return report;
    }

    ShadowCryptTargetRecommendation RecommendedTarget() const {
        ShadowCryptTargetRecommendation best{};
        for (std::size_t i = 0; i < enemyCount_; ++i) {
            if (enemies_[i].defeated) continue;
            int priority = BasePriority(enemies_[i].role);
            if (enemies_[i].staggered) priority += 100;
            if (counterTarget_ == i) priority += 80;
            if (threatIndex_ == i && enemies_[i].role == ShadowCryptEnemyRole::VeilChanneler)
                priority += 40;
            if (SourcesActivePressure(i)) priority += 60;
            if (!best.valid || priority > best.priority
                || (priority == best.priority && i < best.enemyIndex)) {
                best.valid = true;
                best.enemyIndex = i;
                best.role = enemies_[i].role;
                best.priority = priority;
            }
        }
        return best;
    }

    bool LockTarget(std::size_t index) {
        if (!active_ || complete_ || index >= enemyCount_ || enemies_[index].defeated
            || lockedTarget_ == index) return false;
        lockedTarget_ = index;
        return true;
    }

    bool ClearTargetLock() {
        if (lockedTarget_ >= enemyCount_) return false;
        lockedTarget_ = EnemyCapacity;
        return true;
    }

    bool HasTargetLock() const {
        return lockedTarget_ < enemyCount_ && !enemies_[lockedTarget_].defeated;
    }

    std::size_t SelectedTarget() const {
        if (HasTargetLock()) return lockedTarget_;
        const auto recommendation = RecommendedTarget();
        return recommendation.valid ? recommendation.enemyIndex : EnemyCapacity;
    }

    int DamageTaken() const { return damageTaken_; }

private:
    static constexpr int MaxTrackedDamage = 9999;
    static constexpr int BracePostureDamage = 10;
    static constexpr int PrecisionCounterHealthBonus = 8;
    static constexpr int PrecisionCounterPostureBonus = 6;
    static constexpr int WeaknessHealthBonus = 10;
    static constexpr int WeaknessPostureBonus = 8;
    static constexpr int FlowBreakHealthBonus = 12;
    static constexpr int FlowBreakPostureBonus = 10;
    static constexpr int RiftMarkDamageBonus = 6;
    static constexpr int VeilWardDurability = 14;
    static constexpr int BulwarkGuardPostureReduction = 10;
    static constexpr double PrecisionResponseFraction = 0.30;

    static constexpr bool ValidTier(ShadowCryptSkirmishTier tier) {
        return static_cast<std::uint8_t>(tier)
            < static_cast<std::uint8_t>(ShadowCryptSkirmishTier::Count);
    }
    static constexpr bool ValidResponse(ShadowCryptDefenseResponse response) {
        return static_cast<std::uint8_t>(response)
            <= static_cast<std::uint8_t>(ShadowCryptDefenseResponse::Guard);
    }
    static constexpr bool ValidAttackStyle(ShadowCryptAttackStyle style) {
        return static_cast<std::uint8_t>(style)
            <= static_cast<std::uint8_t>(ShadowCryptAttackStyle::Counter);
    }
    static constexpr ShadowCryptEnemyState MakeEnemy(ShadowCryptEnemyRole role,
        int health, int posture) {
        return {role, health, health, posture, posture, false, false};
    }
    static constexpr int BasePriority(ShadowCryptEnemyRole role) {
        switch (role) {
        case ShadowCryptEnemyRole::VeilChanneler: return 30;
        case ShadowCryptEnemyRole::RiftSkirmisher: return 20;
        case ShadowCryptEnemyRole::GraveboundBulwark: return 10;
        case ShadowCryptEnemyRole::Count: return 0;
        }
        return 0;
    }
    static constexpr ShadowCryptAttackStyle RecommendedAttack(ShadowCryptEnemyRole role) {
        switch (role) {
        case ShadowCryptEnemyRole::VeilChanneler: return ShadowCryptAttackStyle::Light;
        case ShadowCryptEnemyRole::RiftSkirmisher: return ShadowCryptAttackStyle::Heavy;
        case ShadowCryptEnemyRole::GraveboundBulwark: return ShadowCryptAttackStyle::Heavy;
        case ShadowCryptEnemyRole::Count: return ShadowCryptAttackStyle::Heavy;
        }
        return ShadowCryptAttackStyle::Heavy;
    }

    ShadowCryptThreatTelegraph ThreatForIndex(std::size_t index) const {
        if (!active_ || complete_ || index >= enemyCount_ || enemies_[index].defeated) return {};
        const auto role = enemies_[index].role;
        const std::uint8_t patternStep = static_cast<std::uint8_t>(patternSteps_[index] % 2);
        ShadowCryptThreatTelegraph telegraph{};
        telegraph.valid = true;
        telegraph.enemyIndex = index;
        telegraph.role = role;
        telegraph.patternStep = patternStep;
        telegraph.enraged = enemies_[index].health > 0
            && enemies_[index].health * 2 <= enemies_[index].maxHealth;
        switch (role) {
        case ShadowCryptEnemyRole::RiftSkirmisher:
            if (patternStep == 0) {
                telegraph.variant = ShadowCryptThreatVariant::RiftLunge;
                telegraph.cue = ShadowCryptThreatCue::Evade;
                telegraph.expected = ShadowCryptDefenseResponse::Dodge;
                telegraph.responseWindowSeconds = 0.42;
                telegraph.failureDamage = 18;
                telegraph.defensiveCounterAllowed = true;
            } else {
                telegraph.variant = ShadowCryptThreatVariant::RiftCrosscut;
                telegraph.cue = ShadowCryptThreatCue::Evade;
                telegraph.expected = ShadowCryptDefenseResponse::Dodge;
                telegraph.responseWindowSeconds = 0.34;
                telegraph.failureDamage = 22;
                telegraph.defensiveCounterAllowed = true;
            }
            break;
        case ShadowCryptEnemyRole::VeilChanneler:
            if (patternStep == 0) {
                telegraph.variant = ShadowCryptThreatVariant::VeilBind;
                telegraph.cue = ShadowCryptThreatCue::Interrupt;
                telegraph.expected = ShadowCryptDefenseResponse::Interrupt;
                telegraph.responseWindowSeconds = 0.70;
                telegraph.failureDamage = 24;
                telegraph.defensiveCounterAllowed = true;
            } else {
                telegraph.variant = ShadowCryptThreatVariant::VeilBurst;
                telegraph.cue = ShadowCryptThreatCue::Interrupt;
                telegraph.expected = ShadowCryptDefenseResponse::Interrupt;
                telegraph.responseWindowSeconds = 0.52;
                telegraph.failureDamage = 28;
                telegraph.defensiveCounterAllowed = true;
            }
            break;
        case ShadowCryptEnemyRole::GraveboundBulwark:
            if (patternStep == 0) {
                telegraph.variant = ShadowCryptThreatVariant::GraveHammer;
                telegraph.cue = ShadowCryptThreatCue::Brace;
                telegraph.expected = ShadowCryptDefenseResponse::Guard;
                telegraph.responseWindowSeconds = 0.55;
                telegraph.failureDamage = 30;
                telegraph.defensiveCounterAllowed = false;
            } else {
                telegraph.variant = ShadowCryptThreatVariant::GraveRush;
                telegraph.cue = ShadowCryptThreatCue::Brace;
                telegraph.expected = ShadowCryptDefenseResponse::Guard;
                telegraph.responseWindowSeconds = 0.42;
                telegraph.failureDamage = 34;
                telegraph.defensiveCounterAllowed = false;
            }
            break;
        case ShadowCryptEnemyRole::Count:
            return {};
        }
        if (telegraph.enraged) {
            telegraph.responseWindowSeconds =
                std::max(0.20, telegraph.responseWindowSeconds * 0.80);
            telegraph.failureDamage = std::min(40, telegraph.failureDamage + 6);
        }
        if (RiftMarkActive()) {
            telegraph.failureDamage = std::min(40,
                telegraph.failureDamage + RiftMarkDamageBonus);
        }
        return telegraph;
    }

    bool RiftMarkActive() const {
        return riftMarked_ && riftMarkSource_ < enemyCount_
            && !enemies_[riftMarkSource_].defeated;
    }

    bool VeilWardActive(std::size_t target) const {
        return target < enemyCount_ && veilWard_[target] > 0
            && veilWardSource_[target] < enemyCount_
            && !enemies_[veilWardSource_[target]].defeated
            && !enemies_[target].defeated;
    }

    bool GuardLinkActive() const {
        return guardLinkedTarget_ < enemyCount_ && guardLinkSource_ < enemyCount_
            && !enemies_[guardLinkedTarget_].defeated
            && !enemies_[guardLinkSource_].defeated
            && !enemies_[guardLinkSource_].staggered;
    }

    bool SourcesActivePressure(std::size_t source) const {
        if (RiftMarkActive() && riftMarkSource_ == source) return true;
        for (std::size_t i = 0; i < enemyCount_; ++i) {
            if (VeilWardActive(i) && veilWardSource_[i] == source) return true;
        }
        return GuardLinkActive() && guardLinkSource_ == source;
    }

    void ClearRiftMark() {
        riftMarked_ = false;
        riftMarkSource_ = EnemyCapacity;
    }

    void ClearGuardLink() {
        guardLinkedTarget_ = EnemyCapacity;
        guardLinkSource_ = EnemyCapacity;
    }

    void ClearGuardLinkFromSource(std::size_t source) {
        if (guardLinkSource_ == source) ClearGuardLink();
    }

    bool ClearPressureFromSource(std::size_t source) {
        bool cleared = false;
        if (riftMarked_ && riftMarkSource_ == source) {
            ClearRiftMark();
            cleared = true;
        }
        for (std::size_t i = 0; i < enemyCount_; ++i) {
            if (veilWard_[i] > 0 && veilWardSource_[i] == source) {
                veilWard_[i] = 0;
                veilWardSource_[i] = EnemyCapacity;
                cleared = true;
            }
        }
        if (guardLinkSource_ == source && guardLinkedTarget_ < enemyCount_) {
            ClearGuardLink();
            cleared = true;
        }
        return cleared;
    }

    std::size_t LowestHealthAlly(std::size_t source) const {
        std::size_t best = EnemyCapacity;
        for (std::size_t i = 0; i < enemyCount_; ++i) {
            if (i == source || enemies_[i].defeated) continue;
            if (best == EnemyCapacity
                || enemies_[i].health * enemies_[best].maxHealth
                    < enemies_[best].health * enemies_[i].maxHealth
                || (enemies_[i].health * enemies_[best].maxHealth
                        == enemies_[best].health * enemies_[i].maxHealth && i < best)) {
                best = i;
            }
        }
        return best == EnemyCapacity ? source : best;
    }

    std::size_t HighestPriorityAlly(std::size_t source) const {
        std::size_t best = EnemyCapacity;
        int bestPriority = -1;
        for (std::size_t i = 0; i < enemyCount_; ++i) {
            if (i == source || enemies_[i].defeated) continue;
            const int priority = BasePriority(enemies_[i].role);
            if (best == EnemyCapacity || priority > bestPriority
                || (priority == bestPriority && i < best)) {
                best = i;
                bestPriority = priority;
            }
        }
        return best;
    }

    bool ApplyPressure(std::size_t source, ShadowCryptEnemyRole role) {
        if (source >= enemyCount_ || enemies_[source].defeated) return false;
        switch (role) {
        case ShadowCryptEnemyRole::RiftSkirmisher:
            riftMarked_ = true;
            riftMarkSource_ = source;
            return true;
        case ShadowCryptEnemyRole::VeilChanneler: {
            const std::size_t target = LowestHealthAlly(source);
            if (target >= enemyCount_ || enemies_[target].defeated) return false;
            veilWard_[target] = VeilWardDurability;
            veilWardSource_[target] = source;
            return true;
        }
        case ShadowCryptEnemyRole::GraveboundBulwark: {
            const std::size_t target = HighestPriorityAlly(source);
            if (target >= enemyCount_) return false;
            guardLinkedTarget_ = target;
            guardLinkSource_ = source;
            return true;
        }
        case ShadowCryptEnemyRole::Count:
            return false;
        }
        return false;
    }

    void Reset() {
        enemies_ = {};
        patternSteps_ = {};
        breachOpenings_ = {};
        veilWard_ = {};
        veilWardSource_.fill(EnemyCapacity);
        enemyCount_ = 0;
        threatIndex_ = 0;
        lockedTarget_ = EnemyCapacity;
        counterTarget_ = EnemyCapacity;
        precisionCounterTarget_ = EnemyCapacity;
        riftMarkSource_ = EnemyCapacity;
        guardLinkedTarget_ = EnemyCapacity;
        guardLinkSource_ = EnemyCapacity;
        riftMarked_ = false;
        damageTaken_ = 0;
        shadowFlow_ = 0;
        replayMode_ = false;
        complete_ = false;
        active_ = false;
    }

    bool AllDefeated() const {
        if (enemyCount_ == 0) return false;
        for (std::size_t i = 0; i < enemyCount_; ++i) {
            if (!enemies_[i].defeated) return false;
        }
        return true;
    }

    void RefreshThreat() {
        if (!active_ || complete_ || enemyCount_ == 0) return;
        for (std::size_t offset = 0; offset < enemyCount_; ++offset) {
            const std::size_t index = (threatIndex_ + offset) % enemyCount_;
            if (!enemies_[index].defeated) {
                threatIndex_ = index;
                return;
            }
        }
    }

    void AdvanceThreatPattern(std::size_t index) {
        if (index >= enemyCount_) return;
        patternSteps_[index] = static_cast<std::uint8_t>((patternSteps_[index] + 1) % 2);
    }

    void AdvanceThreatCursor() {
        if (enemyCount_ == 0) return;
        threatIndex_ = (threatIndex_ + 1) % enemyCount_;
        RefreshThreat();
    }

    std::array<ShadowCryptEnemyState, EnemyCapacity> enemies_{};
    std::array<std::uint8_t, EnemyCapacity> patternSteps_{};
    std::array<bool, EnemyCapacity> breachOpenings_{};
    std::array<int, EnemyCapacity> veilWard_{};
    std::array<std::size_t, EnemyCapacity> veilWardSource_{};
    std::size_t enemyCount_{};
    std::size_t threatIndex_{};
    std::size_t lockedTarget_{EnemyCapacity};
    std::size_t counterTarget_{EnemyCapacity};
    std::size_t precisionCounterTarget_{EnemyCapacity};
    std::size_t riftMarkSource_{EnemyCapacity};
    std::size_t guardLinkedTarget_{EnemyCapacity};
    std::size_t guardLinkSource_{EnemyCapacity};
    int damageTaken_{};
    std::uint8_t shadowFlow_{};
    ShadowCryptSkirmishTier tier_{ShadowCryptSkirmishTier::EntrySeal};
    bool riftMarked_{};
    bool replayMode_{};
    bool active_{};
    bool complete_{};
};

} // namespace Astral::Scene
