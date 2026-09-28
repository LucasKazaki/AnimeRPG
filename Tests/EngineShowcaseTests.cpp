// Astral-mode showcase: the presentation must never change gameplay rules.
// Scenarios mirror the Win32 frame order through ShowcaseSession and the M10
// native smoke route, then check what the player would see and hear.

#include "Engine/Graphics/Image.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Game/Showcase/ShowcaseSession.h"
#include "Tests/EngineTestSupport.h"

#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

using namespace Astral;
using Showcase::SessionInput;
using Showcase::ShowcaseSession;

namespace {

constexpr float kDt = 1.0f / 60.0f;

Showcase::ShowcaseSettings TestSettings() {
    Showcase::ShowcaseSettings settings;
    settings.quality = 1;
    settings.dynamicResolution = false;
    return settings;
}

void Run(ShowcaseSession& session, int frames, const SessionInput& held = {}) {
    for (int i = 0; i < frames; ++i) session.Step(held, kDt);
}

template <typename Press>
Showcase::FrameEvents Tap(ShowcaseSession& session, Press press) {
    SessionInput input;
    press(input);
    const Showcase::FrameEvents events = session.Step(input, kDt);
    session.Step({}, kDt);
    return events;
}

void Thought(ShowcaseSession& session, const std::string& text) {
    SessionInput input;
    input.typedThought = text;
    session.Step(input, kDt);
}

template <typename Done>
bool HoldUntil(ShowcaseSession& session, const SessionInput& held, Done done, int maxFrames) {
    for (int i = 0; i < maxFrames && !done(); ++i) session.Step(held, kDt);
    return done();
}

Math::Vec3 Position(const ShowcaseSession& session) { return session.Player().TransformState().WorldPosition(); }

// Same tolerance idea as the M10 native smoke's CountEncounterColor.
int CountColor(const Graphics::ImageRgba8& image, int r, int g, int b) {
    int count = 0;
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            const Graphics::Rgba8 p = image.Get(x, y);
            if (std::abs(p.r - r) <= 12 && std::abs(p.g - g) <= 12 && std::abs(p.b - b) <= 12) ++count;
        }
    }
    return count;
}

void MaybeCapture(const Graphics::ImageRgba8& image, const char* name) {
    const char* directory = std::getenv("ASTRAL_CAPTURE_DIR");
    if (!directory || !*directory) return;
    std::string error;
    ASTRAL_CHECK(Graphics::WritePng(std::string(directory) + "/" + name + ".png", image, error));
}

// The M10 native smoke route: fatal + dash thoughts, walk to Lincoln, discover,
// return and finish the encounter with a Fatal Strike.
void PlayM10Route(ShowcaseSession& session, bool render, int* activePixels, int* completedPixels) {
    Thought(session, "fatal");
    Thought(session, "dash");
    SessionInput north;
    north.forward = true;
    ASTRAL_CHECK(HoldUntil(session, north, [&] { return session.Interaction().HasSelection(); }, 600));
    ASTRAL_CHECK(session.Interaction().SelectedKind() == Scene::LandmarkKind::LincolnMemorial);
    Tap(session, [](SessionInput& in) { in.interact = true; });
    ASTRAL_CHECK(session.Encounter().State() == Scene::LandmarkEncounterState::Active);
    ASTRAL_CHECK(session.Interaction().VisitedCount() == 1);
    Run(session, 10);
    if (render) {
        const Graphics::ImageRgba8& frame = session.Showcase().Render(480, 270);
        MaybeCapture(frame, "showcase_encounter_active");
        *activePixels = CountColor(frame, 255, 155, 60);
    }
    SessionInput south;
    south.backward = true;
    Run(session, 132, south); // HoldKey('S', 2200)
    Tap(session, [](SessionInput& in) { in.fatal = true; });
    Run(session, 30);
    ASTRAL_CHECK(session.Combat().Dummy().IsDefeated());
    ASTRAL_CHECK(session.Encounter().State() == Scene::LandmarkEncounterState::Completed);
    ASTRAL_CHECK(session.Encounter().CompletionRewardGranted());
    // Repeat input after completion is safe (M10 repeatSafe).
    Tap(session, [](SessionInput& in) { in.light = true; });
    ASTRAL_CHECK(session.Encounter().State() == Scene::LandmarkEncounterState::Completed);
    if (render) {
        const Graphics::ImageRgba8& frame = session.Showcase().Render(480, 270);
        MaybeCapture(frame, "showcase_encounter_completed");
        *completedPixels = CountColor(frame, 80, 235, 125);
    }
}

} // namespace

ASTRAL_TEST(AstralModeKeepsGameplayRulesOnTheM10Route) {
    ShowcaseSession astral(nullptr, TestSettings(), true);
    ShowcaseSession gdi(nullptr, TestSettings(), false); // raw gameplay positions, as the GDI renderer uses
    int activePixels = 0, completedPixels = 0;
    PlayM10Route(astral, true, &activePixels, &completedPixels);
    PlayM10Route(gdi, false, nullptr, nullptr);
    // Presentation colours mirror the GDI HUD states the native smoke looks for.
    ASTRAL_CHECK(activePixels > 5);
    ASTRAL_CHECK(completedPixels > 5);
    // Identical rules: same damage, resource, rewards and route outcome.
    ASTRAL_CHECK(astral.Combat().Dummy().health == gdi.Combat().Dummy().health);
    ASTRAL_CHECK(astral.Shadowblade().Resource() == gdi.Shadowblade().Resource());
    ASTRAL_CHECK(astral.Encounter().LastReport().rewardApplied == gdi.Encounter().LastReport().rewardApplied);
    ASTRAL_CHECK(astral.Encounter().LastReport().rewardApplied == Scene::LandmarkEncounter::CompletionReward);
    ASTRAL_CHECK(astral.Interaction().VisitedCount() == gdi.Interaction().VisitedCount());
    ASTRAL_CHECK_NEAR(Position(astral).x, Position(gdi).x, 0.05f);
    ASTRAL_CHECK_NEAR(Position(astral).y, Position(gdi).y, 0.05f);
}

ASTRAL_TEST(ThoughtFocusDrivesPresentationTimeAtGameplayMultiplier) {
    ShowcaseSession session(nullptr, TestSettings());
    Thought(session, "focus");
    ASTRAL_CHECK(session.Thought().IsFocusActive());
    ASTRAL_CHECK_NEAR(session.Thought().ScaleDelta(1.0f), Scene::ThoughtCommands::FocusTimeMultiplier, 1.0e-6f);
    Run(session, 60);
    // Once blended in, animation/VFX time runs at exactly the gameplay multiplier.
    ASTRAL_CHECK_NEAR(session.Showcase().Time().WorldDelta(), kDt * Scene::ThoughtCommands::FocusTimeMultiplier, 1.0e-4f);
    const Graphics::ImageRgba8 focused = session.Showcase().Render(320, 180); // copy: Render reuses its buffer
    MaybeCapture(focused, "showcase_focus");
    Thought(session, "focus");
    ASTRAL_CHECK(!session.Thought().IsFocusActive());
    Run(session, 60);
    ASTRAL_CHECK_NEAR(session.Showcase().Time().WorldDelta(), kDt, 1.0e-4f);
    // The focus grade drains colour from the world: the Mall's lawns stop reading green.
    const Graphics::ImageRgba8& normal = session.Showcase().Render(320, 180);
    MaybeCapture(normal, "showcase_normal");
    auto greenLawn = [](const Graphics::ImageRgba8& image) {
        int count = 0;
        // 3D band only: HUD panels (the green minimap) sit above, below and to the right.
        for (int y = image.height * 3 / 10; y < image.height * 7 / 10; ++y) {
            for (int x = 0; x < image.width * 3 / 4; ++x) {
                const Graphics::Rgba8 p = image.Get(x, y);
                if (p.g > p.r + 25 && p.g > p.b + 10) ++count;
            }
        }
        return count;
    };
    ASTRAL_CHECK(greenLawn(normal) > 1500);
    ASTRAL_CHECK(greenLawn(focused) * 10 < greenLawn(normal));
}

ASTRAL_TEST(DashIsSweptAgainstTheMallWithoutChangingItsCost) {
    ShowcaseSession swept(nullptr, TestSettings(), true);
    ShowcaseSession raw(nullptr, TestSettings(), false);
    for (ShowcaseSession* session : {&swept, &raw}) session->Place({-8.0f, 9.0f, 0.0f}); // facing the Lincoln stairs
    const Showcase::FrameEvents sweptEvents = Tap(swept, [](SessionInput& in) { in.dash = true; });
    const Showcase::FrameEvents rawEvents = Tap(raw, [](SessionInput& in) { in.dash = true; });
    ASTRAL_CHECK(sweptEvents.dashReport.result == Scene::ShadowActionResult::Activated);
    ASTRAL_CHECK(rawEvents.dashReport.result == Scene::ShadowActionResult::Activated);
    // Raw gameplay goes straight into the memorial's footprint; the swept dash stops at its face.
    ASTRAL_CHECK_NEAR(Position(raw).y, rawEvents.dashReport.dashDestination.y, 1.0e-4f);
    ASTRAL_CHECK(Position(raw).y > 14.5f);
    ASTRAL_CHECK(Position(swept).y < 14.5f);
    ASTRAL_CHECK(Position(swept).y > 10.0f);
    // Cost, cooldown and report are the gameplay's own.
    ASTRAL_CHECK(swept.Shadowblade().Resource() == raw.Shadowblade().Resource());
    ASTRAL_CHECK(swept.Shadowblade().DashCooldownRemaining() == raw.Shadowblade().DashCooldownRemaining());
}

ASTRAL_TEST(SpringArmCameraNeverEntersSolidLandmarks) {
    ShowcaseSession session(nullptr, TestSettings());
    session.Place({5.0f, 71.5f, 0.0f}); // north of the Washington Monument: the arm points into the shaft
    Run(session, 90);
    const Math::Vec3 eye = session.Showcase().CameraEye();
    std::vector<Physics::BodyId> hits;
    session.Showcase().PhysicsScene().Overlap(Physics::Shape::Sphere(0.1f), {eye, {}}, hits, Showcase::CollisionLayers::World);
    ASTRAL_CHECK(hits.empty());
    ASTRAL_CHECK(Math::Distance(eye, session.Showcase().Player().Position()) > 0.5f);
}

ASTRAL_TEST(HeavyHitsShatterCrystalsThatRespawn) {
    ShowcaseSession session(nullptr, TestSettings());
    Showcase::MallScene& mall = session.Showcase().Mall();
    ASTRAL_CHECK(!mall.Crystals().empty());
    const Math::Vec3 at = mall.Crystals().front().position;
    ASTRAL_CHECK(mall.ShatterCrystals(at, 1.0f, {1, 0, 0}, 100.0f) >= 1);
    ASTRAL_CHECK(mall.Crystals().front().destructible->Broken());
    Run(session, 60 * 11);
    ASTRAL_CHECK(!mall.Crystals().front().destructible->Broken());
}

ASTRAL_TEST(PresentationIsDeterministicForIdenticalInput) {
    auto play = [](ShowcaseSession& session) {
        Tap(session, [](SessionInput& in) { in.light = true; });
        Run(session, 20);
        Tap(session, [](SessionInput& in) { in.dash = true; });
        Run(session, 6);
        Thought(session, "focus");
        Run(session, 12);
    };
    ShowcaseSession a(nullptr, TestSettings()), b(nullptr, TestSettings());
    play(a);
    play(b);
    const std::uint64_t hashA = Graphics::HashImage(a.Showcase().Render(256, 144));
    const std::uint64_t hashB = Graphics::HashImage(b.Showcase().Render(256, 144));
    ASTRAL_CHECK(hashA == hashB);
    std::vector<float> audioA(2 * 4096), audioB(2 * 4096);
    a.Showcase().Mixer().Mix(audioA.data(), 4096);
    b.Showcase().Mixer().Mix(audioB.data(), 4096);
    ASTRAL_CHECK(audioA == audioB);
    float peak = 0.0f;
    for (float sample : audioA) peak = std::max(peak, std::fabs(sample));
    ASTRAL_CHECK(peak > 0.0f && peak <= 1.0f);
}

ASTRAL_TEST(RenderSurvivesDegenerateSizesAndTimeSteps) {
    ShowcaseSession session(nullptr, TestSettings());
    session.Showcase().Update(session.View(), {}, {0, 0, 0}, std::nanf(""));
    session.Showcase().Update(session.View(), {}, {0, 0, 0}, -1.0f);
    session.Showcase().Update(session.View(), {}, {0, 0, 0}, 10.0f);
    const Graphics::ImageRgba8& tiny = session.Showcase().Render(1, 1);
    ASTRAL_CHECK(tiny.width >= 64 && tiny.height >= 36);
    Math::Vec3 p = Position(session);
    ASTRAL_CHECK(std::isfinite(p.x) && std::isfinite(p.y));
}

ASTRAL_TEST_MAIN("EngineShowcaseTests")
