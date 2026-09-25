#include "Engine/Math/Math.h"
#include "Engine/Scene/Camera.h"
#include "Engine/Scene/PlayerController.h"
#include "Engine/Scene/CombatSandbox.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace M = Astral::Math;
namespace S = Astral::Scene;
namespace {
int cases = 0;
int failures = 0;
const float kNaN = std::numeric_limits<float>::quiet_NaN();
const float inf = std::numeric_limits<float>::infinity();
const float maxFloat = std::numeric_limits<float>::max();
#define REQUIRE(...) do { if (!(__VA_ARGS__)) throw std::runtime_error(std::string(#__VA_ARGS__) + " at line " + std::to_string(__LINE__)); } while (false)
bool Near(double a, double b, double tolerance = 0.001) { return std::abs(a - b) <= tolerance; }
bool Same(M::Vec3 a, M::Vec3 b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
bool Same(M::Vec2 a, M::Vec2 b) { return a.x == b.x && a.y == b.y; }
template<class F> void Case(const char* name, F fn) {
    ++cases;
    try { fn(); std::cout << "PASS " << name << '\n'; }
    catch (const std::exception& e) { ++failures; std::cerr << "FAIL " << name << ": " << e.what() << '\n'; }
}
template<class F> bool InvalidArgument(F fn) {
    try { fn(); } catch (const std::invalid_argument&) { return true; }
    return false;
}
}

int main() {
    Case("math ordinary norm", [] { REQUIRE(Near(M::Vec3{3,4,12}.Length(),13)); });
    Case("math large representable norm", [] {
        const float length = M::Vec3{1e20f,1e20f,0}.Length();
        REQUIRE(std::isfinite(length)); REQUIRE(Near(length / 1e20, std::sqrt(2.0), 1e-6));
    });
    Case("math tiny nonzero norm", [] { REQUIRE(M::Vec3{1e-30f,0,0}.Length() == 1e-30f); });
    Case("math finite vector predicate", [] {
        REQUIRE(M::IsFinite({maxFloat,0,-maxFloat}));
        REQUIRE(!M::IsFinite({kNaN,0,0}) && !M::IsFinite({0,inf,0}) && !M::IsFinite({0,0,-inf}));
    });
    Case("movement ordinary and diagonal", [] {
        S::PlayerController p(4,{ -2,2,-2,2 });
        p.Update({true,false,false,false},0.5f); REQUIRE(p.TransformState().localPosition.y == 2);
        p.SetPosition({}); p.Update({true,false,true,false},0.5f);
        REQUIRE(Near(p.TransformState().localPosition.x,-std::sqrt(2.0)));
        REQUIRE(Near(p.TransformState().localPosition.y,std::sqrt(2.0)));
    });
    Case("movement invalid deltas are atomic", [] {
        S::PlayerController p; p.SetPosition({1,2,3}); const auto before=p.TransformState().localPosition;
        for (float dt : {kNaN,inf,-inf,-1.0f,0.0f}) {
            p.Update({},dt); REQUIRE(Same(p.TransformState().localPosition,before));
            p.Update({true,false,false,true},dt); REQUIRE(Same(p.TransformState().localPosition,before));
        }
    });
    Case("movement nonfinite position rejected in every component", [] {
        S::PlayerController p; p.SetPosition({1,2,3}); const auto before=p.TransformState().localPosition;
        for (M::Vec3 bad : {M::Vec3{kNaN,0,0},M::Vec3{0,inf,0},M::Vec3{0,0,-inf}}) {
            p.SetPosition(bad); REQUIRE(Same(p.TransformState().localPosition,before)); REQUIRE(p.IsWithinBounds());
        }
    });
    Case("movement invalid speed rejected at construction", [] {
        for (float speed : {kNaN,inf,-inf,-1.0f}) REQUIRE(InvalidArgument([&] { S::PlayerController p(speed); }));
    });
    Case("movement reversed and nonfinite bounds rejected", [] {
        for (S::MovementBounds b : {S::MovementBounds{2,-2,-1,1},S::MovementBounds{-1,1,2,-2},
            S::MovementBounds{kNaN,1,-1,1},S::MovementBounds{-1,inf,-1,1},
            S::MovementBounds{-1,1,-inf,1},S::MovementBounds{-1,1,-1,kNaN}})
            REQUIRE(InvalidArgument([&] { S::PlayerController p(1,b); }));
    });
    Case("movement zero speed and degenerate bounds valid", [] {
        S::PlayerController p(0,{2,2,3,3}); p.Update({true,false,false,true},maxFloat);
        REQUIRE(Same(p.TransformState().localPosition,M::Vec3{2,3,0}));
    });
    Case("movement large finite midpoint does not overflow", [] {
        S::PlayerController p(1,{maxFloat/2,maxFloat,-maxFloat,-maxFloat/2});
        const float expected=static_cast<float>((static_cast<double>(maxFloat/2)+maxFloat)/2);
        REQUIRE(p.TransformState().localPosition.x==expected);
        REQUIRE(p.TransformState().localPosition.y==-expected && p.IsWithinBounds());
    });
    Case("movement huge finite displacement clamps without corrupting idle axis", [] {
        S::PlayerController p(maxFloat); p.Update({false,false,false,true},maxFloat);
        REQUIRE(Same(p.TransformState().localPosition,M::Vec3{9,0,0})); REQUIRE(p.IsWithinBounds());
    });
    Case("movement opposing keys cancel", [] {
        S::PlayerController p(maxFloat); p.SetPosition({1,2,3});p.Update({true,true,true,true},maxFloat);
        REQUIRE(Same(p.TransformState().localPosition,M::Vec3{1,2,3}));
    });
    Case("hierarchy preserves translation semantics", [] {
        S::Transform parent;parent.localPosition={3,4,5};
        S::Transform child;child.localPosition={1,-2,3};child.parent=&parent;
        M::Vec3 out{}; REQUIRE(child.TryWorldPosition(out)); REQUIRE(Same(out,M::Vec3{4,2,8}));
        REQUIRE(Same(child.WorldPosition(),out));
    });
    Case("hierarchy self cycle fails without mutating output", [] {
        S::Transform t;t.parent=&t;M::Vec3 out{1,2,3};
        REQUIRE(!t.TryWorldPosition(out));REQUIRE(Same(out,M::Vec3{1,2,3}));REQUIRE(!M::IsFinite(t.WorldPosition()));
    });
    Case("hierarchy multi-node cycle and prefix rejected", [] {
        S::Transform a,b,c; a.parent=&b;b.parent=&c;c.parent=&b;
        M::Vec3 out{1,2,3}; REQUIRE(!a.TryWorldPosition(out)); REQUIRE(Same(out,M::Vec3{1,2,3}));
    });
    Case("hierarchy deep valid chain is stack safe", [] {
        std::vector<S::Transform> chain(50000);
        for (std::size_t i=0;i<chain.size();++i) { chain[i].localPosition.x=1; if(i) chain[i].parent=&chain[i-1]; }
        REQUIRE(chain.back().WorldPosition().x==50000);
    });
    Case("hierarchy rejects invalid ancestor and overflow", [] {
        S::Transform a,b;b.parent=&a;M::Vec3 out{1,2,3};a.localPosition.y=kNaN;
        REQUIRE(!b.TryWorldPosition(out));a.localPosition={maxFloat,0,0};b.localPosition=a.localPosition;
        REQUIRE(!b.TryWorldPosition(out));REQUIRE(Same(out,M::Vec3{1,2,3}));
    });
    Case("perspective ordinary center depth and scale", [] {
        S::PerspectiveCamera c; const auto pos=c.Position();
        const auto at=[&](float x,float up,float depth) { return M::Vec3{x,pos.y-0.514495755f*depth+0.857492926f*up,
            pos.z+0.857492926f*depth+0.514495755f*up}; };
        M::Vec2 center{},near{},far{};
        REQUIRE(c.WorldToScreen(at(0,0,10),800,600,center));REQUIRE(Near(center.x,400)&&Near(center.y,300));
        REQUIRE(c.WorldToScreen(at(1,0,5),800,600,near));REQUIRE(c.WorldToScreen(at(1,0,10),800,600,far));
        REQUIRE(Near(near.x-400,2*(far.x-400)));
        REQUIRE(!c.WorldToScreen(at(0,0,-1),800,600,center));
        REQUIRE(!c.WorldToScreen(at(0,0,c.NearPlane()),800,600,center));
    });
    Case("perspective invalid viewport rejected atomically", [] {
        S::PerspectiveCamera c;M::Vec2 out{12,34};
        REQUIRE(!c.WorldToScreen({},0,600,out));REQUIRE(!c.WorldToScreen({},800,-1,out));REQUIRE(Same(out,M::Vec2{12,34}));
    });
    Case("perspective invalid field of view rejected", [] {
        S::PerspectiveCamera c;M::Vec2 out{12,34};
        for(float fov : {kNaN,inf,-inf,0.0f,-1.0f,180.0f}) {
            c.verticalFieldOfViewDegrees=fov;REQUIRE(!c.WorldToScreen({},800,600,out));REQUIRE(Same(out,M::Vec2{12,34}));
        }
    });
    Case("perspective invalid near plane rejected", [] {
        S::PerspectiveCamera c;M::Vec2 out{12,34};
        for(float near : {kNaN,inf,-inf,0.0f,-1.0f}) {
            c.nearPlane=near;REQUIRE(!c.WorldToScreen({},800,600,out));REQUIRE(Same(out,M::Vec2{12,34}));
        }
    });
    Case("perspective invalid points and unsafe raster values rejected", [] {
        S::PerspectiveCamera c;M::Vec2 out{12,34};
        for(M::Vec3 point : {M::Vec3{kNaN,0,0},M::Vec3{0,inf,0},M::Vec3{0,0,-inf},M::Vec3{1e20f,0,0}}) {
            REQUIRE(!c.WorldToScreen(point,800,600,out));REQUIRE(Same(out,M::Vec2{12,34}));
        }
    });
    Case("perspective offscreen finite points remain valid", [] {
        S::PerspectiveCamera c;M::Vec2 out{};REQUIRE(c.WorldToScreen({100,0,0},800,600,out));REQUIRE(out.x>800);
    });
    Case("perspective invalid follow preserves camera", [] {
        S::PerspectiveCamera c;S::Transform t;t.localPosition={4,7,0};c.Follow(t);const auto before=c.Position();
        REQUIRE(Same(before,M::Vec3{4,6,-3}));t.localPosition.x=kNaN;c.Follow(t);REQUIRE(Same(before,c.Position()));
        t.localPosition={};t.parent=&t;c.Follow(t);REQUIRE(Same(before,c.Position()));
    });
    Case("orthographic default and followed projection", [] {
        S::OrthographicCamera c;M::Vec2 out{};REQUIRE(c.TryWorldToScreen({},100,80,out));REQUIRE(Same(out,M::Vec2{50,40}));
        S::Transform t;t.localPosition={2,2,0};c.Follow(t,{1,-2,0});
        REQUIRE(Same(c.WorldToScreen({3,0,0},100,80),M::Vec2{50,40}));
    });
    Case("orthographic invalid projection fails and legacy wrapper stays finite", [] {
        S::OrthographicCamera c;M::Vec2 out{12,34};
        for(float width : {kNaN,inf,-inf,0.0f,-1.0f}) {
            c.worldWidth=width;REQUIRE(!c.TryWorldToScreen({},100,80,out));REQUIRE(Same(out,M::Vec2{12,34}));
            REQUIRE(Same(c.WorldToScreen({},100,80),M::Vec2{}));
        }
        c.worldWidth=20;c.worldHeight=0;REQUIRE(!c.TryWorldToScreen({},100,80,out));
        c.worldHeight=12;REQUIRE(!c.TryWorldToScreen({kNaN,0,0},100,80,out));
        REQUIRE(!c.TryWorldToScreen({},-1,80,out));REQUIRE(!c.TryWorldToScreen({},100,0,out));
    });
    Case("orthographic rejects impossible scale and invalid follow", [] {
        S::OrthographicCamera c;M::Vec2 out{12,34};c.worldWidth=std::numeric_limits<float>::denorm_min();
        REQUIRE(!c.TryWorldToScreen({1,0,0},100,80,out));REQUIRE(Same(out,M::Vec2{12,34}));
        c.worldWidth=20;S::Transform t;t.localPosition={1,2,3};c.Follow(t);const auto before=c.WorldToScreen({},100,80);
        c.Follow(t,{inf,0,0});REQUIRE(Same(before,c.WorldToScreen({},100,80)));
        t.parent=&t;c.Follow(t);REQUIRE(Same(before,c.WorldToScreen({},100,80)));
    });
    Case("combat valid attacks retain damage and cooldown", [] {
        S::CombatSandbox b;REQUIRE(b.TryAttack(S::AttackType::Light,{}).damageApplied==25);
        REQUIRE(b.TryAttack(S::AttackType::Heavy,{}).result==S::AttackResult::Cooldown);
        b.AdvanceTime(0.4f);REQUIRE(b.TryAttack(S::AttackType::Heavy,{}).damageApplied==60);
    });
    Case("combat invalid attack cannot mutate gameplay state", [] {
        S::CombatSandbox b;const auto r=b.TryAttack(static_cast<S::AttackType>(99),{});
        REQUIRE(r.result==S::AttackResult::OutOfRange && r.damageApplied==0);
        REQUIRE(b.Dummy().health==100 && b.Dummy().posture==0 && b.ComboCount()==0 && b.Stats().hitCount==0);
        REQUIRE(b.CooldownRemaining()==0 && b.TryAttack(S::AttackType::Light,{}).damageApplied==25);
    });
    Case("combat invalid attack definition is inert", [] {
        S::CombatSandbox b;const auto& d=b.Definition(static_cast<S::AttackType>(-1));
        REQUIRE(d.damage==0 && d.postureDamage==0 && d.range==0 && d.cooldownSeconds==0);
    });
    Case("combat invalid profile cannot reset health statistics or queued threat", [] {
        S::CombatSandbox b;b.SetTrainingEnemyProfile(S::TrainingEnemyProfile::Boss);b.ApplyDamage(80);b.AdvanceTime(2);
        REQUIRE(b.QueueNextEnemyAttack());const auto generation=b.EnemyAttackGeneration();
        REQUIRE(!b.SetTrainingEnemyProfile(static_cast<S::TrainingEnemyProfile>(99)));
        REQUIRE(b.EnemyProfile()==S::TrainingEnemyProfile::Boss && b.Dummy().health==240 && b.Stats().totalDamage==80);
        REQUIRE(b.HasPendingEnemyAttack() && b.EnemyAttackGeneration()==generation && b.ElapsedSecondsPrecise()==2);
    });
    Case("combat invalid target mode cannot reset health and statistics", [] {
        S::CombatSandbox b;b.ApplyDamage(25);REQUIRE(!b.SetTrainingTargetMode(static_cast<S::TrainingTargetMode>(99)));
        REQUIRE(b.TargetMode()==S::TrainingTargetMode::Standard && b.Dummy().health==75 && b.Stats().totalDamage==25);
    });
    Case("combat invalid affinity cannot create stored element", [] {
        S::CombatSandbox b;const auto r=b.ApplyManaAffinity(static_cast<S::ManaAffinity>(99));
        REQUIRE(r.reaction==S::ManaReaction::None && r.bonusDamage==0 && b.TargetAffinity()==S::ManaAffinity::None);
    });
    Case("combat invalid affinity cannot consume or trigger valid reaction", [] {
        S::CombatSandbox b;b.ApplyManaAffinity(S::ManaAffinity::Solar);
        const auto bad=b.ApplyManaAffinity(static_cast<S::ManaAffinity>(99));
        REQUIRE(bad.bonusDamage==0 && b.TargetAffinity()==S::ManaAffinity::Solar && b.Stats().reactionCount==0);
        const auto good=b.ApplyManaAffinity(S::ManaAffinity::Umbral);
        REQUIRE(good.reaction==S::ManaReaction::Eclipse && good.bonusDamage==20 && b.Stats().reactionCount==1);
    });
    Case("combat invalid assist cannot replace accessible preset", [] {
        S::CombatSandbox b;b.SetCombatAssistPreset(S::CombatAssistPreset::Accessible);
        b.SetCombatAssistPreset(static_cast<S::CombatAssistPreset>(99));
        REQUIRE(b.AssistPreset()==S::CombatAssistPreset::Accessible && b.ComboFinisherRequiredHits()==2);
    });
    Case("combat invalid and unrepresentable time steps are no-ops", [] {
        S::CombatSandbox b;b.AdvanceTime(2);b.TryAttack(S::AttackType::Light,{});
        const auto cooldown=b.CooldownRemaining();
        for(float dt : {kNaN,inf,-inf,-1.0f,0.0f,maxFloat}) {
            b.AdvanceTime(dt);REQUIRE(b.ElapsedSecondsPrecise()==2 && b.CooldownRemaining()==cooldown && b.Dummy().posture==25);
        }
    });
    Case("combat large representable time expires posture safely", [] {
        S::CombatSandbox b;b.TryAttack(S::AttackType::Light,{});b.AdvanceTime(100000000.0f);
        REQUIRE(b.Dummy().posture==0 && b.ComboCount()==0 && b.CooldownRemaining()==0 && !b.IsStaggered());
        REQUIRE(b.TryAttack(S::AttackType::Light,{}).damageApplied==25);
    });
    Case("combat horizon keeps new deadlines and reset valid", [] {
        S::CombatSandbox b;b.AdvanceTime(1e12f);REQUIRE(b.ElapsedSecondsPrecise()>0);
        REQUIRE(b.TryAttack(S::AttackType::Light,{}).damageApplied==25);
        REQUIRE(b.CooldownRemaining()>0 && std::isfinite(b.CooldownRemaining()));
        const auto time=b.ElapsedSecondsPrecise();b.AdvanceTime(1e15f);REQUIRE(b.ElapsedSecondsPrecise()==time);
        b.ResetTrainingSession();REQUIRE(b.ElapsedSecondsPrecise()==0 && b.Dummy().health==100);
    });
    Case("combat ordinary recovery boundary unchanged", [] {
        S::CombatSandbox b;b.TryAttack(S::AttackType::Light,{});b.AdvanceTime(1.5f);REQUIRE(b.Dummy().posture==25);
        b.AdvanceTime(0.1f);REQUIRE(b.Dummy().posture==22);b.AdvanceTime(1);REQUIRE(b.Dummy().posture==0);
    });
    Case("combat microstep DPS is finite", [] {
        S::CombatSandbox b;b.ApplyDamage(25);b.AdvanceTime(std::numeric_limits<float>::denorm_min());
        REQUIRE(std::isfinite(b.TrainingDps()) && b.TrainingDps()==maxFloat);
    });
    Case("combat valid endless mode and reaction stats unchanged", [] {
        S::CombatSandbox b;REQUIRE(b.SetTrainingTargetMode(S::TrainingTargetMode::Endless));
        REQUIRE(b.ApplyDamage(500)==500 && b.Dummy().health==100 && b.Stats().totalDamage==500);
        b.ApplyManaAffinity(S::ManaAffinity::Solar);b.ApplyManaAffinity(S::ManaAffinity::Umbral);
        REQUIRE(b.Stats().reactionCount==1 && b.Stats().totalDamage==520);
    });
    Case("combat existing malformed range and outcome rejection retained", [] {
        S::CombatSandbox b;REQUIRE(b.TryAttack(S::AttackType::Light,{kNaN,0,0}).damageApplied==0);
        REQUIRE(b.TryAttack(S::AttackType::Light,{100,0,0}).damageApplied==0);
        REQUIRE(b.QueueNextEnemyAttack());const auto gen=b.EnemyAttackGeneration();
        REQUIRE(!b.ResolveEnemyAttack(static_cast<S::EnemyAttackOutcome>(99)));
        REQUIRE(b.HasPendingEnemyAttack() && b.EnemyAttackGeneration()==gen);
    });
    Case("5000 fixed-seed movement updates stay finite and bounded", [] {
        std::mt19937 rng(20260925);S::PlayerController p;
        for(int i=0;i<5000;++i) {
            const auto bits=rng();const S::MovementInput input{(bits&1)!=0,(bits&2)!=0,(bits&4)!=0,(bits&8)!=0};
            p.Update(input,i%17==0?kNaN:static_cast<float>(rng()%3000)/1000);
            REQUIRE(p.IsWithinBounds() && M::IsFinite(p.TransformState().localPosition));
        }
    });
    Case("5000 fixed-seed finite norms match double reference", [] {
        std::mt19937 rng(25);
        for(int i=0;i<5000;++i) {
            const int exponent=static_cast<int>(rng()%201)-100;
            const float x=std::ldexp(static_cast<float>((rng()%1000)+1)/1000,exponent);
            const float y=x*0.5f,z=-x*0.25f;const double reference=std::sqrt(static_cast<double>(x)*x+static_cast<double>(y)*y+static_cast<double>(z)*z);
            const float actual=M::Vec3{x,y,z}.Length();REQUIRE(std::isfinite(actual) && actual>0);
            REQUIRE(std::abs(actual-reference)/reference<2e-6);
        }
    });
    std::cout << "RESULT " << (cases-failures) << '/' << cases << " named cases passed; 10000 fixed-seed trials included\n";
    return failures==0?0:1;
}
