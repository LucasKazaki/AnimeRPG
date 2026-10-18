#pragma once

// Opt-in, portable reference component. Not connected to AstralGame yet.
// A production integration must give this component sole ownership of its actor's
// health; never mirror CombatSandbox/ShadowbladeActions health beside this state.
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace Astral::Gameplay::LocalizedDamage {

constexpr std::size_t MaxParts = 32;
constexpr int AmountLimit = 1000000;
constexpr int ScaleLimit = 8000;
using PartId = std::uint16_t;
using CapabilityMask = std::uint32_t;
enum class Capability : unsigned { Mobility, Melee, Ranged, Casting, Flight, Guard, Count };
constexpr CapabilityMask Bit(Capability c) {
    return static_cast<unsigned>(c) < static_cast<unsigned>(Capability::Count)
        ? (CapabilityMask{1} << static_cast<unsigned>(c)) : 0;
}
constexpr CapabilityMask AllCapabilities = (CapabilityMask{1} << static_cast<unsigned>(Capability::Count)) - 1;
enum class DamageKind : unsigned { Impact, Slash, Pierce, Solar, Umbral, Count };
enum class DefeatMode : unsigned { Vitality, VitalPart, Either };
enum class PartCondition : unsigned { Healthy, Impaired, Disabled };

struct PartDefinition {
    PartId id{}; // Stable authored id, never a skeleton-array index.
    PartId exposedAfterBreak{}; // Zero = exposed. Otherwise requires this part to be broken.
    int maximumIntegrity{100};
    int maximumArmor{};
    int integrityScale{1000}; // All scales are integer permille; 1000 = 1x.
    int vitalityScale{1000}; // Weak-point multiplier, independent of part durability.
    std::array<int, static_cast<std::size_t>(DamageKind::Count)> susceptibility{{1000,1000,1000,1000,1000}};
    int impairedAt{500}; // Remaining integrity fraction <= threshold is impaired.
    int impairedEfficiency{800};
    int disabledEfficiency{};
    CapabilityMask affectedCapabilities{};
    CapabilityMask blockedWhenDisabled{};
    bool vital{};
    bool removedWhenBroken{};
};

struct Profile {
    int maximumVitality{100};
    DefeatMode defeatMode{DefeatMode::Vitality};
    bool allowFriendlyFire{};
    std::vector<PartDefinition> parts;
};
struct PartState {
    int integrity{};
    int armor{};
    bool everBroken{};
};
struct Hit {
    std::uint64_t epoch{}; // World/encounter owner assigns a fresh identity on respawn.
    std::uint64_t sequence{}; // Strictly increasing PER TARGET, assigned by trusted owner.
    PartId part{}; // Must come from confirmed collision, not just a selected target.
    int rawDamage{};
    DamageKind kind{DamageKind::Impact};
    bool sameTeam{}; // Trusted faction-policy result, not client input.
};
enum class HitStatus : unsigned {
    Applied, Invalid, StaleEpoch, DuplicateOrOutOfOrder, FriendlyBlocked, Covered, Removed, Defeated
};
struct HitResult {
    HitStatus status{HitStatus::Invalid};
    int armorDamage{};
    int integrityDamage{};
    int vitalityDamage{};
    bool partBroken{}; // Transition event; repair and re-break can emit again.
    bool firstBreak{}; // Once per part per epoch; useful for reward deduplication.
    bool defeated{};
};

inline bool InRange(int x, int lo, int hi) { return x >= lo && x <= hi; }
inline bool ValidProfile(const Profile& p) {
    if (!InRange(p.maximumVitality, 1, AmountLimit) || p.parts.empty() || p.parts.size() > MaxParts
        || static_cast<unsigned>(p.defeatMode) > static_cast<unsigned>(DefeatMode::Either)) return false;
    bool hasVital = false;
    for (std::size_t i = 0; i < p.parts.size(); ++i) {
        const auto& d = p.parts[i];
        if (!d.id || !InRange(d.maximumIntegrity, 1, AmountLimit)
            || !InRange(d.maximumArmor, 0, AmountLimit) || !InRange(d.integrityScale, 0, ScaleLimit)
            || !InRange(d.vitalityScale, 0, ScaleLimit) || !InRange(d.impairedAt, 1, 999)
            || !InRange(d.impairedEfficiency, 0, 1000)
            || !InRange(d.disabledEfficiency, 0, d.impairedEfficiency)
            || (d.affectedCapabilities & ~AllCapabilities)
            || (d.blockedWhenDisabled & ~d.affectedCapabilities)) return false;
        for (int s : d.susceptibility) if (!InRange(s, 0, ScaleLimit)) return false;
        for (std::size_t j = 0; j < i; ++j) if (p.parts[j].id == d.id) return false;
        hasVital = hasVital || d.vital;
        // Follow the single cover edge; missing nodes and cycles fail closed.
        PartId next = d.exposedAfterBreak;
        std::size_t hops = 0;
        while (next) {
            if (++hops > p.parts.size()) return false;
            const PartDefinition* found = nullptr;
            for (const auto& other : p.parts) if (other.id == next) { found = &other; break; }
            if (!found) return false;
            next = found->exposedAfterBreak;
        }
    }
    return p.defeatMode == DefeatMode::Vitality || hasVital;
}

class Body {
public:
    // Reconfiguration is atomic for invalid profiles and rejects recycled epochs.
    // Copies authoring data at configuration time, never during hit resolution.
    bool Configure(const Profile& p, std::uint64_t epoch) {
        if (!epoch || epoch <= epoch_ || !ValidProfile(p)) return false;
        Body next;
        next.profile_ = p;
        next.epoch_ = epoch;
        next.vitality_ = p.maximumVitality;
        for (std::size_t i = 0; i < p.parts.size(); ++i)
            next.states_[i] = {p.parts[i].maximumIntegrity, p.parts[i].maximumArmor, false};
        *this = std::move(next);
        return true;
    }
    bool Configured() const { return epoch_ != 0; }
    std::uint64_t Epoch() const { return epoch_; }
    std::uint64_t LastSequence() const { return sequence_; }
    int Vitality() const { return vitality_; }
    std::size_t PartCount() const { return profile_.parts.size(); }
    const PartDefinition* Definition(PartId id) const {
        const auto i = Index(id);
        return i < PartCount() ? &profile_.parts[i] : nullptr;
    }
    const PartState* State(PartId id) const {
        const auto i = Index(id);
        return i < PartCount() ? &states_[i] : nullptr;
    }
    bool Exposed(PartId id) const {
        const auto* d = Definition(id);
        if (!d) return false;
        const auto* cover = State(d->exposedAfterBreak);
        return !d->exposedAfterBreak || (cover && cover->integrity == 0);
    }
    bool Defeated() const {
        if (!Configured()) return false;
        if (profile_.defeatMode != DefeatMode::VitalPart && vitality_ == 0) return true;
        if (profile_.defeatMode != DefeatMode::Vitality)
            for (std::size_t i = 0; i < PartCount(); ++i)
                if (profile_.parts[i].vital && states_[i].integrity == 0) return true;
        return false;
    }
    PartCondition Condition(PartId id) const {
        const auto* s = State(id);
        const auto* d = Definition(id);
        if (!s || !d || s->integrity == 0) return PartCondition::Disabled;
        return std::int64_t{s->integrity} * 1000 <= std::int64_t{d->maximumIntegrity} * d->impairedAt
            ? PartCondition::Impaired : PartCondition::Healthy;
    }
    CapabilityMask BlockedCapabilities() const {
        if (!Configured() || Defeated()) return AllCapabilities;
        CapabilityMask blocked = 0;
        for (std::size_t i = 0; i < PartCount(); ++i)
            if (states_[i].integrity == 0) blocked |= profile_.parts[i].blockedWhenDisabled;
        return blocked;
    }
    int Efficiency(Capability capability) const {
        const auto mask = Bit(capability);
        if (!mask || !Configured() || (BlockedCapabilities() & mask)) return 0;
        int efficiency = 1000;
        // Worst contributor wins, rather than exponential multiplicative stacking.
        for (const auto& d : profile_.parts) {
            if (!(d.affectedCapabilities & mask)) continue;
            switch (Condition(d.id)) {
            case PartCondition::Disabled: efficiency = std::min(efficiency, d.disabledEfficiency); break;
            case PartCondition::Impaired: efficiency = std::min(efficiency, d.impairedEfficiency); break;
            case PartCondition::Healthy: break;
            }
        }
        return efficiency;
    }
    HitResult Apply(const Hit& h) {
        HitResult r;
        const auto i = Index(h.part);
        if (!Configured() || i == PartCount() || !h.sequence
            || !InRange(h.rawDamage, 1, AmountLimit)
            || static_cast<unsigned>(h.kind) >= static_cast<unsigned>(DamageKind::Count)) return r;
        if (h.epoch != epoch_) { r.status = HitStatus::StaleEpoch; return r; }
        if (h.sequence <= sequence_) { r.status = HitStatus::DuplicateOrOutOfOrder; return r; }
        if (Defeated()) { r.status = HitStatus::Defeated; r.defeated = true; return r; }
        // Valid blocked contacts also consume their receipt, so replay after armor
        // removal/faction changes cannot retroactively turn a blocked hit into damage.
        sequence_ = h.sequence;
        if (h.sameTeam && !profile_.allowFriendlyFire) { r.status = HitStatus::FriendlyBlocked; return r; }
        const auto& d = profile_.parts[i];
        auto& s = states_[i];
        if (d.removedWhenBroken && s.integrity == 0) { r.status = HitStatus::Removed; return r; }
        if (!Exposed(h.part)) { r.status = HitStatus::Covered; return r; }
        const int incoming = Scale(h.rawDamage, d.susceptibility[static_cast<std::size_t>(h.kind)]);
        r.armorDamage = std::min(s.armor, incoming);
        const int transmitted = incoming - r.armorDamage;
        r.integrityDamage = std::min(s.integrity, Scale(transmitted, d.integrityScale));
        r.vitalityDamage = profile_.defeatMode == DefeatMode::VitalPart ? 0
            : std::min(vitality_, Scale(transmitted, d.vitalityScale));
        r.partBroken = s.integrity > 0 && r.integrityDamage == s.integrity;
        r.firstBreak = r.partBroken && !s.everBroken;
        s.armor -= r.armorDamage;
        s.integrity -= r.integrityDamage;
        s.everBroken = s.everBroken || r.partBroken;
        vitality_ -= r.vitalityDamage;
        r.status = HitStatus::Applied;
        r.defeated = Defeated();
        return r;
    }
    // Explicit recovery, not timers or revival. Armor/removed pieces need a later
    // equipment-repair policy; this restores integrity only, without resetting rewards.
    int RepairPart(PartId id, int amount) {
        const auto i = Index(id);
        if (!Configured() || Defeated() || i == PartCount() || !InRange(amount, 1, AmountLimit)) return 0;
        const auto& d = profile_.parts[i];
        auto& s = states_[i];
        if (d.removedWhenBroken && s.integrity == 0) return 0;
        const int restored = std::min(amount, d.maximumIntegrity - s.integrity);
        s.integrity += restored;
        return restored;
    }
    int HealVitality(int amount) {
        if (!Configured() || Defeated() || !InRange(amount, 1, AmountLimit)) return 0;
        const int restored = std::min(amount, profile_.maximumVitality - vitality_);
        vitality_ += restored;
        return restored;
    }
private:
    static int Scale(int amount, int permille) {
        return static_cast<int>(std::min<std::int64_t>(AmountLimit, std::int64_t{amount} * permille / 1000));
    }
    std::size_t Index(PartId id) const {
        for (std::size_t i = 0; i < PartCount(); ++i) if (profile_.parts[i].id == id) return i;
        return PartCount();
    }
    Profile profile_{};
    std::array<PartState, MaxParts> states_{};
    std::uint64_t epoch_{};
    std::uint64_t sequence_{};
    int vitality_{};
};

} // namespace Astral::Gameplay::LocalizedDamage
