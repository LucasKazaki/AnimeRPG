#include "Engine/Gameplay/LocalizedDamage.h"
#include "Game/Combat/LocalizedDamageProfiles.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace D = Astral::Gameplay::LocalizedDamage;
namespace G = AnimeRPG::Combat;
static int cases = 0;
#define CHECK(expression) do { if (!(expression)) { std::cerr << __LINE__ << ": " << #expression << '\n'; std::exit(1); } } while(false)
template<class F> void Case(const char* name, F test) { test(); ++cases; std::cout << "PASS " << name << '\n'; }
D::Profile One() { D::Profile p; p.maximumVitality = 1000; D::PartDefinition d; d.id = 1; p.parts = {d}; return p; }
D::Hit Hit(std::uint64_t sequence, D::PartId part, int damage, std::uint64_t epoch = 1) { return {epoch,sequence,part,damage,D::DamageKind::Impact,false}; }
D::Body Make(const D::Profile& p) { D::Body b; CHECK(b.Configure(p,1)); return b; }

int main() {
    Case("all authored profiles validate", [] {
        CHECK(D::ValidProfile(G::MallSentinelProfile())); CHECK(D::ValidProfile(G::AdventurerProfile()));
        CHECK(D::ValidProfile(G::CompanionProfile())); CHECK(D::ValidProfile(G::MallSentinelProfile(D::DefeatMode::VitalPart)));
    });
    Case("unconfigured component fails closed", [] {
        D::Body b; CHECK(b.Apply(Hit(1,1,10)).status == D::HitStatus::Invalid);
        CHECK(b.Efficiency(D::Capability::Melee) == 0); CHECK(b.RepairPart(1,10) == 0);
        CHECK(G::ChooseSentinelAction(b) == G::SentinelAction::None);
    });
    Case("profile structural validation", [] {
        auto p=One(); p.parts.clear(); CHECK(!D::ValidProfile(p));
        p=One(); p.parts.push_back(p.parts[0]); CHECK(!D::ValidProfile(p));
        p=One(); p.parts[0].id=0; CHECK(!D::ValidProfile(p));
        p=One(); for (unsigned i=2;i<=33;++i) { auto d=p.parts[0]; d.id=static_cast<D::PartId>(i); p.parts.push_back(d); }
        CHECK(!D::ValidProfile(p)); p.parts.pop_back(); CHECK(D::ValidProfile(p));
    });
    Case("profile numeric bounds", [] {
        auto p=One(); p.maximumVitality=0; CHECK(!D::ValidProfile(p));
        p=One(); p.parts[0].maximumIntegrity=-1; CHECK(!D::ValidProfile(p));
        p=One(); p.parts[0].maximumArmor=-1; CHECK(!D::ValidProfile(p));
        p=One(); p.parts[0].vitalityScale=8001; CHECK(!D::ValidProfile(p));
        p=One(); p.parts[0].susceptibility[0]=-1; CHECK(!D::ValidProfile(p));
        p=One(); p.parts[0].impairedAt=1000; CHECK(!D::ValidProfile(p));
        p=One(); p.parts[0].disabledEfficiency=901; CHECK(!D::ValidProfile(p));
    });
    Case("cover references and cycles rejected", [] {
        auto p=One(); p.parts[0].exposedAfterBreak=2; CHECK(!D::ValidProfile(p));
        p.parts.push_back(p.parts[0]); p.parts[1].id=2; p.parts[1].exposedAfterBreak=1; CHECK(!D::ValidProfile(p));
        p=One(); p.parts[0].exposedAfterBreak=1; CHECK(!D::ValidProfile(p));
    });
    Case("defeat and capability enums validated", [] {
        auto p=One(); p.defeatMode=static_cast<D::DefeatMode>(99); CHECK(!D::ValidProfile(p));
        p=One(); p.defeatMode=D::DefeatMode::VitalPart; CHECK(!D::ValidProfile(p));
        p.parts[0].vital=true; CHECK(D::ValidProfile(p));
        p=One(); p.parts[0].affectedCapabilities=0x80000000u; CHECK(!D::ValidProfile(p));
        p=One(); p.parts[0].blockedWhenDisabled=D::Bit(D::Capability::Casting); CHECK(!D::ValidProfile(p));
    });
    Case("configuration is atomic and epochs cannot recycle", [] {
        auto b=Make(One()); b.Apply(Hit(1,1,20)); auto invalid=One(); invalid.maximumVitality=0;
        CHECK(!b.Configure(invalid,2)); CHECK(b.Vitality()==980); CHECK(b.State(1)->integrity==80);
        CHECK(!b.Configure(One(),1)); CHECK(!b.Configure(One(),0)); CHECK(b.LastSequence()==1);
        CHECK(b.Configure(One(),2)); CHECK(b.Vitality()==1000); CHECK(b.LastSequence()==0);
        CHECK(b.Apply(Hit(2,1,20,1)).status==D::HitStatus::StaleEpoch);
    });
    Case("armor absorbs before integrity and vitality", [] {
        auto p=One(); p.parts[0].maximumArmor=30; auto b=Make(p);
        auto r=b.Apply(Hit(1,1,20)); CHECK(r.armorDamage==20 && r.integrityDamage==0 && r.vitalityDamage==0);
        r=b.Apply(Hit(2,1,25)); CHECK(r.armorDamage==10 && r.integrityDamage==15 && r.vitalityDamage==15);
        CHECK(b.State(1)->armor==0 && b.State(1)->integrity==85 && b.Vitality()==985);
    });
    Case("weak damage and part integrity are separate", [] {
        auto p=One(); p.parts[0].integrityScale=500; p.parts[0].vitalityScale=2500; auto b=Make(p);
        auto r=b.Apply(Hit(1,1,40)); CHECK(r.integrityDamage==20 && r.vitalityDamage==100);
    });
    Case("typed susceptibility before armor", [] {
        auto p=One(); p.parts[0].maximumArmor=10; p.parts[0].susceptibility[3]=1500;
        auto b=Make(p); auto h=Hit(1,1,20); h.kind=D::DamageKind::Solar;
        auto r=b.Apply(h); CHECK(r.armorDamage==10 && r.integrityDamage==20 && r.vitalityDamage==20);
    });
    Case("immunity does not damage or break", [] {
        auto p=One(); p.parts[0].susceptibility[0]=0; auto b=Make(p);
        auto r=b.Apply(Hit(1,1,100)); CHECK(r.status==D::HitStatus::Applied);
        CHECK(r.integrityDamage==0 && r.vitalityDamage==0 && !r.partBroken); CHECK(b.LastSequence()==1);
    });
    Case("covered core rejects and consumes contact", [] {
        auto b=Make(G::MallSentinelProfile()); auto r=b.Apply(Hit(1,2,100));
        CHECK(r.status==D::HitStatus::Covered && b.Vitality()==1200 && b.LastSequence()==1);
        CHECK(!b.Exposed(2)); CHECK(b.Apply(Hit(2,1,150)).partBroken); CHECK(b.Exposed(2));
        CHECK(b.Apply(Hit(1,2,100)).status==D::HitStatus::DuplicateOrOutOfOrder);
        CHECK(b.Apply(Hit(3,2,40)).vitalityDamage==100);
    });
    Case("broken removable plate has no repeat damage or reward", [] {
        auto b=Make(G::MallSentinelProfile()); auto r=b.Apply(Hit(1,1,150)); CHECK(r.firstBreak);
        r=b.Apply(Hit(2,1,150)); CHECK(r.status==D::HitStatus::Removed && !r.firstBreak);
        CHECK(b.RepairPart(1,100)==0);
    });
    Case("disable weapon arm changes proposed boss action", [] {
        auto b=Make(G::MallSentinelProfile()); CHECK(G::ChooseSentinelAction(b)==G::SentinelAction::Cleave);
        b.Apply(Hit(1,3,100)); CHECK(b.Efficiency(D::Capability::Melee)==0);
        CHECK(G::ChooseSentinelAction(b)==G::SentinelAction::RuneBurst);
        b.Apply(Hit(2,5,80)); CHECK(G::ChooseSentinelAction(b)==G::SentinelAction::DesperationPulse);
    });
    Case("leg impairment keeps a movement floor", [] {
        auto b=Make(G::MallSentinelProfile()); b.Apply(Hit(1,4,50)); CHECK(b.Efficiency(D::Capability::Mobility)==800);
        b.Apply(Hit(2,4,50)); CHECK(b.Efficiency(D::Capability::Mobility)==550);
        CHECK(!(b.BlockedCapabilities() & D::Bit(D::Capability::Mobility)));
    });
    Case("impairment threshold exactness", [] {
        auto b=Make(G::MallSentinelProfile()); b.Apply(Hit(1,4,49)); CHECK(b.Condition(4)==D::PartCondition::Healthy);
        b.Apply(Hit(2,4,1)); CHECK(b.Condition(4)==D::PartCondition::Impaired);
        CHECK(b.RepairPart(4,1)==1 && b.Condition(4)==D::PartCondition::Healthy);
    });
    Case("friendly fire disabled and replay safe", [] {
        auto b=Make(G::CompanionProfile()); auto h=Hit(1,3,20); h.sameTeam=true;
        CHECK(b.Apply(h).status==D::HitStatus::FriendlyBlocked); CHECK(b.Vitality()==150 && b.LastSequence()==1);
        h.sameTeam=false; CHECK(b.Apply(h).status==D::HitStatus::DuplicateOrOutOfOrder);
    });
    Case("friendly fire is explicit opt-in", [] {
        auto p=One(); p.allowFriendlyFire=true; auto b=Make(p); auto h=Hit(1,1,20); h.sameTeam=true;
        CHECK(b.Apply(h).vitalityDamage==20);
    });
    Case("invalid hits do not consume a sequence", [] {
        auto b=Make(One()); CHECK(b.Apply(Hit(1,99,10)).status==D::HitStatus::Invalid);
        CHECK(b.Apply(Hit(1,1,-1)).status==D::HitStatus::Invalid);
        CHECK(b.Apply(Hit(1,1,0)).status==D::HitStatus::Invalid);
        CHECK(b.Apply(Hit(1,1,D::AmountLimit+1)).status==D::HitStatus::Invalid);
        CHECK(b.Apply(Hit(0,1,10)).status==D::HitStatus::Invalid);
        auto h=Hit(1,1,10); h.kind=static_cast<D::DamageKind>(99); CHECK(b.Apply(h).status==D::HitStatus::Invalid);
        CHECK(b.LastSequence()==0 && b.Vitality()==1000 && b.State(1)->integrity==100);
    });
    Case("duplicates and older receipts do not double damage", [] {
        auto b=Make(One()); CHECK(b.Apply(Hit(4,1,10)).vitalityDamage==10);
        CHECK(b.Apply(Hit(4,1,10)).status==D::HitStatus::DuplicateOrOutOfOrder);
        CHECK(b.Apply(Hit(3,1,10)).status==D::HitStatus::DuplicateOrOutOfOrder); CHECK(b.Vitality()==990);
    });
    Case("maximum receipt cannot wrap inside component", [] {
        auto b=Make(One()); auto h=Hit(std::numeric_limits<std::uint64_t>::max(),1,1);
        CHECK(b.Apply(h).status==D::HitStatus::Applied);
        CHECK(b.Apply(Hit(1,1,1)).status==D::HitStatus::DuplicateOrOutOfOrder);
        CHECK(b.Configure(One(),2)); CHECK(b.Apply(Hit(1,1,1,2)).status==D::HitStatus::Applied);
    });
    Case("integer scaling saturates safely", [] {
        auto p=One(); p.maximumVitality=D::AmountLimit; p.parts[0].maximumIntegrity=D::AmountLimit;
        p.parts[0].susceptibility[0]=8000; p.parts[0].integrityScale=8000; p.parts[0].vitalityScale=8000;
        auto b=Make(p); auto r=b.Apply(Hit(1,1,D::AmountLimit));
        CHECK(r.integrityDamage==D::AmountLimit && r.vitalityDamage==D::AmountLimit && r.defeated);
    });
    Case("vitality-only defeat ignores nonlethal broken parts", [] {
        auto p=One(); p.parts[0].vital=true; auto b=Make(p);
        CHECK(!b.Apply(Hit(1,1,100)).defeated); CHECK(b.Apply(Hit(2,1,900)).defeated);
    });
    Case("module-only defeat does not use a global HP pool", [] {
        auto b=Make(G::MallSentinelProfile(D::DefeatMode::VitalPart));
        auto r=b.Apply(Hit(1,3,1000)); CHECK(r.vitalityDamage==0 && !r.defeated);
        b.Apply(Hit(2,1,150)); CHECK(b.Apply(Hit(3,2,120)).defeated); CHECK(b.Vitality()==1200);
    });
    Case("hybrid supports vital-part finish", [] {
        auto b=Make(G::MallSentinelProfile()); b.Apply(Hit(1,1,150));
        CHECK(b.Apply(Hit(2,2,120)).defeated); CHECK(b.Vitality()==900);
        CHECK(G::ChooseSentinelAction(b)==G::SentinelAction::None);
    });
    Case("defeated actors do not attack heal or mutate", [] {
        auto b=Make(One()); b.Apply(Hit(1,1,1000));
        CHECK(b.Apply(Hit(2,1,1)).status==D::HitStatus::Defeated);
        CHECK(b.HealVitality(10)==0 && b.RepairPart(1,10)==0 && b.LastSequence()==1);
        CHECK(b.Efficiency(D::Capability::Melee)==0);
    });
    Case("repair restores function without reward farming", [] {
        auto b=Make(G::MallSentinelProfile()); CHECK(b.Apply(Hit(1,3,100)).firstBreak);
        CHECK(b.RepairPart(3,100)==100 && b.Efficiency(D::Capability::Melee)==1000);
        auto r=b.Apply(Hit(2,3,100)); CHECK(r.partBroken && !r.firstBreak);
        CHECK(b.RepairPart(3,-1)==0 && b.RepairPart(99,10)==0);
    });
    Case("vitality healing does not repair limbs or armor", [] {
        auto b=Make(G::AdventurerProfile()); b.Apply(Hit(1,3,85));
        CHECK(b.State(3)->integrity==0 && b.State(3)->armor==0); CHECK(b.HealVitality(100)==75);
        CHECK(b.State(3)->integrity==0 && b.State(3)->armor==0 && b.Vitality()==100);
    });
    Case("player and companion floors use same engine rules", [] {
        auto player=Make(G::AdventurerProfile()); player.Apply(Hit(1,3,85));
        CHECK(player.Efficiency(D::Capability::Melee)==800);
        auto ally=Make(G::CompanionProfile()); ally.Apply(Hit(1,3,85)); CHECK(ally.Efficiency(D::Capability::Melee)==900);
        CHECK(ally.BlockedCapabilities()==0 && player.BlockedCapabilities()==0);
    });
    Case("two injured legs do not multiply penalties", [] {
        auto b=Make(G::AdventurerProfile()); b.Apply(Hit(1,5,48)); b.Apply(Hit(2,6,48));
        CHECK(b.Efficiency(D::Capability::Mobility)==900 && !b.Defeated());
    });
    Case("unknown part and capability cannot be targeted or enabled", [] {
        auto b=Make(One()); CHECK(!b.Exposed(99) && b.State(99)==nullptr && b.Definition(99)==nullptr);
        CHECK(b.Efficiency(static_cast<D::Capability>(99))==0);
    });
    Case("2000 fixed-seed independent damage arithmetic cases", [] {
        std::mt19937 random(20260924);
        for (int i=0;i<2000;++i) {
            auto p=One(); const int armor=static_cast<int>(random()%80);
            const int raw=static_cast<int>(random()%200)+1;
            const int scale=static_cast<int>(random()%3001);
            p.parts[0].maximumArmor=armor; p.parts[0].vitalityScale=scale;
            auto b=Make(p); const auto r=b.Apply(Hit(1,1,raw));
            const int absorbed=raw<armor?raw:armor; const int transmitted=raw-absorbed;
            CHECK(r.armorDamage==absorbed);
            CHECK(r.integrityDamage==(transmitted<100?transmitted:100));
            const int expected=transmitted*scale/1000;
            CHECK(r.vitalityDamage==expected && b.Vitality()==1000-expected);
            CHECK(b.State(1)->integrity>=0 && b.State(1)->armor>=0);
        }
    });
    std::cout << "PASS: " << cases << " named cases (including 2000 arithmetic trials)\n";
}
