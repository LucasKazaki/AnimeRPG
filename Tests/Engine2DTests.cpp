#include "Engine/Core/JobSystem.h"
#include "Engine/Graphics/Canvas.h"
#include "Engine/Graphics/ProceduralTextures.h"
#include "Engine/Graphics/Sprite2D.h"
#include "Tests/EngineTestSupport.h"

#include <cstdlib>
#include <string>

using namespace Astral;
using namespace Astral::Graphics;
using Math::Vec2;

namespace {
void MaybeCapture(const ImageRgba8& image, const char* name) {
    const char* directory = std::getenv("ASTRAL_CAPTURE_DIR");
    if (!directory || !*directory) return;
    std::string error;
    ASTRAL_CHECK(WritePng(std::string(directory) + "/" + name + ".png", image, error));
}
int CountColor(const ImageRgba8& image, Rgba8 color) {
    int count = 0;
    for (int y = 0; y < image.height; ++y)
        for (int x = 0; x < image.width; ++x) {
            const Rgba8 p = image.Get(x, y);
            count += p.r == color.r && p.g == color.g && p.b == color.b;
        }
    return count;
}
} // namespace

ASTRAL_TEST(FontCoversPrintableAscii) {
    for (int c = 32; c <= 126; ++c) ASTRAL_CHECK(FontGlyph(static_cast<char>(c)) != nullptr);
    ASTRAL_CHECK(FontGlyph('\t') == nullptr);
    // 'A' has a solid crossbar on row 3 and a blank space glyph has no ink.
    ASTRAL_CHECK(FontGlyph('A')[3] == 0x1F);
    for (int row = 0; row < 7; ++row) ASTRAL_CHECK(FontGlyph(' ')[row] == 0);
    // Every non-space glyph has ink.
    for (int c = 33; c <= 126; ++c) {
        int ink = 0;
        for (int row = 0; row < 7; ++row) ink |= FontGlyph(static_cast<char>(c))[row];
        ASTRAL_CHECK(ink != 0);
    }
}

ASTRAL_TEST(CanvasTextAndHudPrimitives) {
    ImageRgba8 image;
    image.Resize(200, 80, {0, 0, 0, 255});
    Canvas canvas(image);
    TextStyle style;
    style.color = {255, 255, 0, 255};
    style.scale = 2;
    style.shadow = false;
    const int end = canvas.Text(4, 4, "HP 100", style);
    ASTRAL_CHECK(end == 4 + 6 * 2 * 6);
    ASTRAL_CHECK(Canvas::MeasureText("HP 100", 2) == 72);
    ASTRAL_CHECK(Canvas::MeasureText("AB\nABCD", 1) == 24);
    ASTRAL_CHECK(CountColor(image, {255, 255, 0, 255}) > 60);
    canvas.Bar(4, 40, 100, 6, 0.25f, {0, 255, 0, 255}, {40, 40, 40, 255});
    ASTRAL_CHECK(image.Get(10, 42).g == 255);
    ASTRAL_CHECK(image.Get(60, 42).g == 40);
    canvas.Bar(4, 50, 100, 6, std::nanf(""), {0, 255, 0, 255}, {40, 40, 40, 255});
    ASTRAL_CHECK(image.Get(5, 52).g == 40);
    canvas.Ring(160, 40, 20.0f, 5.0f, 0.25f, {255, 0, 0, 255});
    // Quarter ring: present at 1-2 o'clock, absent at 7 o'clock.
    ASTRAL_CHECK(image.Get(172, 27).r > 128);
    ASTRAL_CHECK(image.Get(147, 52).r == 0);
    canvas.FillRoundedRect(-10, -10, 20, 20, 5, {0, 0, 255, 255}, 0.5f); // clipped, no crash
    MaybeCapture(image, "canvas_hud");
}

ASTRAL_TEST(ImageBlendComposesSourceOverAlpha) {
    ImageRgba8 image;
    image.Resize(2, 1, {0, 0, 0, 0});
    image.Blend(0, 0, {255, 0, 0, 255}, 0.5f);
    const Rgba8 onTransparent = image.Get(0, 0);
    ASTRAL_CHECK(onTransparent.r == 255 && onTransparent.g == 0 && onTransparent.a == 128);
    image.Blend(0, 0, {0, 0, 255, 255}, 0.5f); // 50% blue over 50% red
    const Rgba8 layered = image.Get(0, 0);
    ASTRAL_CHECK(layered.a == 191 || layered.a == 192);
    ASTRAL_CHECK(layered.b > layered.r && layered.r > 60);
    // Opaque destinations keep the exact lerp every render target relies on.
    image.Set(1, 0, {100, 100, 100, 255});
    image.Blend(1, 0, {200, 0, 0, 255}, 0.25f);
    const Rgba8 opaque = image.Get(1, 0);
    ASTRAL_CHECK(opaque.r == 125 && opaque.g == 75 && opaque.a == 255);
}

ASTRAL_TEST(CameraRoundTripsAndBounds) {
    Camera2D camera;
    camera.position = {10.0f, -3.0f};
    camera.zoom = 24.0f;
    camera.rotation = 0.4f;
    camera.viewportWidth = 320;
    camera.viewportHeight = 200;
    const Vec2 world{12.5f, 1.0f};
    const Vec2 back = camera.ScreenToWorld(camera.WorldToScreen(world));
    ASTRAL_CHECK_NEAR(back.x, world.x, 1e-4);
    ASTRAL_CHECK_NEAR(back.y, world.y, 1e-4);
    const Vec2 centre = camera.WorldToScreen(camera.position);
    ASTRAL_CHECK_NEAR(centre.x, 160.0f, 1e-4);
    ASTRAL_CHECK_NEAR(centre.y, 100.0f, 1e-4);
    Vec2 minimum, maximum;
    camera.VisibleBounds(minimum, maximum);
    ASTRAL_CHECK(minimum.x < camera.position.x && maximum.x > camera.position.x);
}

ASTRAL_TEST(SpriteBatchLayersPivotsAndDeterminism) {
    Camera2D camera;
    camera.zoom = 10.0f;
    camera.viewportWidth = 100;
    camera.viewportHeight = 100;
    Texture2D checker = Procedural::Checker(8, 2, {1, 0, 0}, {0, 0, 1});
    auto drawAll = [&](Core::JobSystem* jobs) {
        ImageRgba8 image;
        image.Resize(100, 100, {0, 0, 0, 255});
        SpriteBatch batch;
        batch.Begin(camera);
        Sprite top;
        top.tint = {0, 1, 0, 1};
        top.size = {2, 2};
        top.layer = 5;
        batch.Draw(top);
        Sprite bottom;
        bottom.texture = &checker;
        bottom.size = {4, 4};
        bottom.layer = 1;
        batch.Draw(bottom);
        Sprite rotated;
        rotated.tint = {1, 1, 1, 0.5f};
        rotated.position = {3, 3};
        rotated.rotation = 0.7f;
        rotated.pivot = {0, 0};
        batch.Draw(rotated);
        batch.End(image, jobs);
        ASTRAL_CHECK(batch.LastDrawnCount() == 3);
        return image;
    };
    const ImageRgba8 serial = drawAll(nullptr);
    Core::JobSystem jobs(3);
    const ImageRgba8 parallel = drawAll(&jobs);
    ASTRAL_CHECK(HashImage(serial) == HashImage(parallel));
    // Higher layer (green) covers the centre even though it was submitted first.
    const Rgba8 centre = serial.Get(50, 50);
    ASTRAL_CHECK(centre.g == 255 && centre.r == 0);
    // Checker visible outside the green square but inside the 4x4 sprite.
    const Rgba8 checkerPixel = serial.Get(32, 50);
    ASTRAL_CHECK(checkerPixel.r == 255 || checkerPixel.b == 255);
    // Half-transparent white sprite with a bottom-left pivot at world (3,3): its
    // centre lands at screen (80.6, 13) after the 0.7 rad rotation.
    const Rgba8 blended = serial.Get(80, 13);
    ASTRAL_CHECK(blended.r > 100 && blended.r < 255);
    MaybeCapture(serial, "sprite_batch");
}

ASTRAL_TEST(TilemapCollisionNeverTunnels) {
    TileSet tiles;
    tiles.columns = 2;
    tiles.rows = 1;
    tiles.solid = {false, true};
    Tilemap map(20, 10, 1.0f);
    for (int x = 0; x < 20; ++x) map.Set(x, 0, 1); // floor
    for (int y = 1; y < 6; ++y) map.Set(12, y, 1);  // wall
    ASTRAL_CHECK(map.IsSolid(12, 3, tiles));
    ASTRAL_CHECK(!map.IsSolid(5, 3, tiles));
    ASTRAL_CHECK(!map.IsSolid(-1, 0, tiles));
    const Vec2 half{0.4f, 0.4f};
    // Fall onto the floor.
    auto fall = map.MoveBox({3.0f, 5.0f}, half, {0.0f, -20.0f}, tiles);
    ASTRAL_CHECK(fall.hitY && fall.grounded);
    ASTRAL_CHECK_NEAR(fall.position.y, 1.4f, 1e-3);
    // Run into the wall at high speed: stop flush, never pass through.
    auto run = map.MoveBox({3.0f, 1.5f}, half, {50.0f, 0.0f}, tiles);
    ASTRAL_CHECK(run.hitX);
    ASTRAL_CHECK(run.position.x < 12.0f - 0.39f && run.position.x > 11.5f);
    // Free movement is exact.
    auto free = map.MoveBox({3.0f, 3.0f}, half, {1.25f, 0.5f}, tiles);
    ASTRAL_CHECK(!free.hitX && !free.hitY);
    ASTRAL_CHECK_NEAR(free.position.x, 4.25f, 1e-5);
    // Non-finite input is ignored.
    auto nan = map.MoveBox({3.0f, 3.0f}, half, {std::nanf(""), 0.0f}, tiles);
    ASTRAL_CHECK_NEAR(nan.position.x, 3.0f, 1e-6);
    // Emission culls to the camera.
    Camera2D camera;
    camera.position = {3.0f, 3.0f};
    camera.zoom = 50.0f;
    camera.viewportWidth = 200;
    camera.viewportHeight = 200;
    SpriteBatch batch;
    batch.Begin(camera);
    const std::size_t emitted = map.Emit(batch, camera, tiles, 0);
    ASTRAL_CHECK(emitted > 0 && emitted < 25);
}

ASTRAL_TEST(ProceduralTexturesAreTileableAndDeterministic) {
    const Texture2D a = Procedural::Marble(64, {0.9f, 0.9f, 0.88f}, {0.5f, 0.5f, 0.55f}, 7);
    const Texture2D b = Procedural::Marble(64, {0.9f, 0.9f, 0.88f}, {0.5f, 0.5f, 0.55f}, 7);
    ASTRAL_CHECK(a.Valid() && a.LevelCount() == 7);
    for (int i = 0; i < 64; i += 7) {
        const Math::Vec4 pa = a.Fetch(0, i, 13), pb = b.Fetch(0, i, 13);
        ASTRAL_CHECK(pa.x == pb.x && pa.y == pb.y && pa.z == pb.z);
    }
    // Tileable: opposite edges are close in value (continuity across the seam).
    const Texture2D grass = Procedural::Grass(64, {0.4f, 0.7f, 0.3f}, {0.2f, 0.4f, 0.15f});
    float seam = 0.0f;
    for (int y = 0; y < 64; ++y) seam += std::fabs(grass.Fetch(0, 0, y).y - grass.Fetch(0, 63, y).y);
    ASTRAL_CHECK(seam / 64.0f < 0.12f);
    const Texture2D disc = Procedural::SoftDisc(32);
    ASTRAL_CHECK(disc.Fetch(0, 16, 16).w > 0.9f);
    ASTRAL_CHECK(disc.Fetch(0, 0, 0).w < 0.01f);
    const Texture2D paving = Procedural::Paving(64, 4, 4, {0.7f, 0.7f, 0.7f}, {0.3f, 0.3f, 0.3f});
    ASTRAL_CHECK(paving.Fetch(0, 0, 8).x < paving.Fetch(0, 8, 8).x); // grout darker than stone
}

ASTRAL_TEST_MAIN("Engine2DTests")
