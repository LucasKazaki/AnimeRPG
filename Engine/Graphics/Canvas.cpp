#include "Engine/Graphics/Canvas.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace Astral::Graphics {

namespace {

// 5x7 glyphs for ASCII 32..126 as row art ('#' = ink). Authored for this engine.
constexpr const char* kGlyphArt[95][7] = {
    {".....", ".....", ".....", ".....", ".....", ".....", "....."}, // ' '
    {"..#..", "..#..", "..#..", "..#..", "..#..", ".....", "..#.."}, // !
    {".#.#.", ".#.#.", ".....", ".....", ".....", ".....", "....."}, // "
    {".#.#.", ".#.#.", "#####", ".#.#.", "#####", ".#.#.", ".#.#."}, // #
    {"..#..", ".####", "#.#..", ".###.", "..#.#", "####.", "..#.."}, // $
    {"##...", "##..#", "...#.", "..#..", ".#...", "#..##", "...##"}, // %
    {".##..", "#..#.", "#.#..", ".#...", "#.#.#", "#..#.", ".##.#"}, // &
    {"..#..", "..#..", ".....", ".....", ".....", ".....", "....."}, // '
    {"...#.", "..#..", ".#...", ".#...", ".#...", "..#..", "...#."}, // (
    {".#...", "..#..", "...#.", "...#.", "...#.", "..#..", ".#..."}, // )
    {".....", "..#..", "#.#.#", ".###.", "#.#.#", "..#..", "....."}, // *
    {".....", "..#..", "..#..", "#####", "..#..", "..#..", "....."}, // +
    {".....", ".....", ".....", ".....", ".##..", "..#..", ".#..."}, // ,
    {".....", ".....", ".....", "#####", ".....", ".....", "....."}, // -
    {".....", ".....", ".....", ".....", ".....", ".##..", ".##.."}, // .
    {".....", "....#", "...#.", "..#..", ".#...", "#....", "....."}, // /
    {".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###."}, // 0
    {"..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###."}, // 1
    {".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####"}, // 2
    {"#####", "...#.", "..#..", "...#.", "....#", "#...#", ".###."}, // 3
    {"...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#."}, // 4
    {"#####", "#....", "####.", "....#", "....#", "#...#", ".###."}, // 5
    {"..##.", ".#...", "#....", "####.", "#...#", "#...#", ".###."}, // 6
    {"#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..."}, // 7
    {".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."}, // 8
    {".###.", "#...#", "#...#", ".####", "....#", "...#.", ".##.."}, // 9
    {".....", ".##..", ".##..", ".....", ".##..", ".##..", "....."}, // :
    {".....", ".##..", ".##..", ".....", ".##..", "..#..", ".#..."}, // ;
    {"...#.", "..#..", ".#...", "#....", ".#...", "..#..", "...#."}, // <
    {".....", ".....", "#####", ".....", "#####", ".....", "....."}, // =
    {".#...", "..#..", "...#.", "....#", "...#.", "..#..", ".#..."}, // >
    {".###.", "#...#", "....#", "...#.", "..#..", ".....", "..#.."}, // ?
    {".###.", "#...#", "....#", ".##.#", "#.#.#", "#.#.#", ".###."}, // @
    {".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}, // A
    {"####.", "#...#", "#...#", "####.", "#...#", "#...#", "####."}, // B
    {".###.", "#...#", "#....", "#....", "#....", "#...#", ".###."}, // C
    {"###..", "#..#.", "#...#", "#...#", "#...#", "#..#.", "###.."}, // D
    {"#####", "#....", "#....", "####.", "#....", "#....", "#####"}, // E
    {"#####", "#....", "#....", "####.", "#....", "#....", "#...."}, // F
    {".###.", "#...#", "#....", "#.###", "#...#", "#...#", ".####"}, // G
    {"#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}, // H
    {".###.", "..#..", "..#..", "..#..", "..#..", "..#..", ".###."}, // I
    {"..###", "...#.", "...#.", "...#.", "...#.", "#..#.", ".##.."}, // J
    {"#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#"}, // K
    {"#....", "#....", "#....", "#....", "#....", "#....", "#####"}, // L
    {"#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#"}, // M
    {"#...#", "#...#", "##..#", "#.#.#", "#..##", "#...#", "#...#"}, // N
    {".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}, // O
    {"####.", "#...#", "#...#", "####.", "#....", "#....", "#...."}, // P
    {".###.", "#...#", "#...#", "#...#", "#.#.#", "#..#.", ".##.#"}, // Q
    {"####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#"}, // R
    {".####", "#....", "#....", ".###.", "....#", "....#", "####."}, // S
    {"#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.."}, // T
    {"#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}, // U
    {"#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", "..#.."}, // V
    {"#...#", "#...#", "#...#", "#.#.#", "#.#.#", "#.#.#", ".#.#."}, // W
    {"#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#"}, // X
    {"#...#", "#...#", "#...#", ".#.#.", "..#..", "..#..", "..#.."}, // Y
    {"#####", "....#", "...#.", "..#..", ".#...", "#....", "#####"}, // Z
    {".###.", ".#...", ".#...", ".#...", ".#...", ".#...", ".###."}, // [
    {".....", "#....", ".#...", "..#..", "...#.", "....#", "....."}, // backslash
    {".###.", "...#.", "...#.", "...#.", "...#.", "...#.", ".###."}, // ]
    {"..#..", ".#.#.", "#...#", ".....", ".....", ".....", "....."}, // ^
    {".....", ".....", ".....", ".....", ".....", ".....", "#####"}, // _
    {".#...", "..#..", ".....", ".....", ".....", ".....", "....."}, // `
    {".....", ".....", ".###.", "....#", ".####", "#...#", ".####"}, // a
    {"#....", "#....", "#.##.", "##..#", "#...#", "#...#", "####."}, // b
    {".....", ".....", ".###.", "#....", "#....", "#...#", ".###."}, // c
    {"....#", "....#", ".##.#", "#..##", "#...#", "#...#", ".####"}, // d
    {".....", ".....", ".###.", "#...#", "#####", "#....", ".###."}, // e
    {"..##.", ".#..#", ".#...", "###..", ".#...", ".#...", ".#..."}, // f
    {".....", ".####", "#...#", "#...#", ".####", "....#", ".###."}, // g
    {"#....", "#....", "#.##.", "##..#", "#...#", "#...#", "#...#"}, // h
    {"..#..", ".....", ".##..", "..#..", "..#..", "..#..", ".###."}, // i
    {"...#.", ".....", "..##.", "...#.", "...#.", "#..#.", ".##.."}, // j
    {"#....", "#....", "#..#.", "#.#..", "##...", "#.#..", "#..#."}, // k
    {".##..", "..#..", "..#..", "..#..", "..#..", "..#..", ".###."}, // l
    {".....", ".....", "##.#.", "#.#.#", "#.#.#", "#...#", "#...#"}, // m
    {".....", ".....", "#.##.", "##..#", "#...#", "#...#", "#...#"}, // n
    {".....", ".....", ".###.", "#...#", "#...#", "#...#", ".###."}, // o
    {".....", ".....", "####.", "#...#", "####.", "#....", "#...."}, // p
    {".....", ".....", ".##.#", "#..##", ".####", "....#", "....#"}, // q
    {".....", ".....", "#.##.", "##..#", "#....", "#....", "#...."}, // r
    {".....", ".....", ".###.", "#....", ".###.", "....#", "####."}, // s
    {".#...", ".#...", "###..", ".#...", ".#...", ".#..#", "..##."}, // t
    {".....", ".....", "#...#", "#...#", "#...#", "#..##", ".##.#"}, // u
    {".....", ".....", "#...#", "#...#", "#...#", ".#.#.", "..#.."}, // v
    {".....", ".....", "#...#", "#...#", "#.#.#", "#.#.#", ".#.#."}, // w
    {".....", ".....", "#...#", ".#.#.", "..#..", ".#.#.", "#...#"}, // x
    {".....", ".....", "#...#", "#...#", ".####", "....#", ".###."}, // y
    {".....", ".....", "#####", "...#.", "..#..", ".#...", "#####"}, // z
    {"...#.", "..#..", "..#..", ".#...", "..#..", "..#..", "...#."}, // {
    {"..#..", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.."}, // |
    {".#...", "..#..", "..#..", "...#.", "..#..", "..#..", ".#..."}, // }
    {".....", ".....", ".#...", "#.#.#", "...#.", ".....", "....."}, // ~
};

const std::array<std::array<unsigned char, 7>, 95>& GlyphTable() {
    static const std::array<std::array<unsigned char, 7>, 95> table = [] {
        std::array<std::array<unsigned char, 7>, 95> rows{};
        for (std::size_t g = 0; g < 95; ++g) {
            for (std::size_t r = 0; r < 7; ++r) {
                unsigned char bits = 0;
                for (int c = 0; c < 5; ++c) {
                    if (kGlyphArt[g][r][c] == '#') bits = static_cast<unsigned char>(bits | (1u << (4 - c)));
                }
                rows[g][r] = bits;
            }
        }
        return rows;
    }();
    return table;
}

} // namespace

const unsigned char* FontGlyph(char c) {
    const int code = static_cast<unsigned char>(c);
    if (code < 32 || code > 126) return nullptr;
    return GlyphTable()[static_cast<std::size_t>(code - 32)].data();
}

void Canvas::FillRect(int x, int y, int width, int height, Rgba8 color, float alpha) {
    const int x0 = std::max(0, x), y0 = std::max(0, y);
    const int x1 = std::min(target_.width, x + width), y1 = std::min(target_.height, y + height);
    for (int py = y0; py < y1; ++py)
        for (int px = x0; px < x1; ++px) target_.Blend(px, py, color, alpha);
}

void Canvas::FillGradient(int x, int y, int width, int height, Rgba8 top, Rgba8 bottom, float alpha) {
    if (height <= 0) return;
    for (int row = 0; row < height; ++row) {
        const float t = height > 1 ? static_cast<float>(row) / static_cast<float>(height - 1) : 0.0f;
        const Rgba8 c{static_cast<std::uint8_t>(std::lround(top.r + (bottom.r - top.r) * t)),
            static_cast<std::uint8_t>(std::lround(top.g + (bottom.g - top.g) * t)),
            static_cast<std::uint8_t>(std::lround(top.b + (bottom.b - top.b) * t)),
            static_cast<std::uint8_t>(std::lround(top.a + (bottom.a - top.a) * t))};
        FillRect(x, y + row, width, 1, c, alpha);
    }
}

void Canvas::FillRoundedRect(int x, int y, int width, int height, int radius, Rgba8 color, float alpha) {
    radius = std::max(0, std::min(radius, std::min(width, height) / 2));
    const int x0 = std::max(0, x), y0 = std::max(0, y);
    const int x1 = std::min(target_.width, x + width), y1 = std::min(target_.height, y + height);
    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            // Distance to the rounded corner arc, for a one-pixel anti-aliased edge.
            const float cx = std::clamp(static_cast<float>(px) + 0.5f, static_cast<float>(x + radius), static_cast<float>(x + width - radius));
            const float cy = std::clamp(static_cast<float>(py) + 0.5f, static_cast<float>(y + radius), static_cast<float>(y + height - radius));
            const float dx = static_cast<float>(px) + 0.5f - cx, dy = static_cast<float>(py) + 0.5f - cy;
            const float distance = std::sqrt(dx * dx + dy * dy);
            const float coverage = std::clamp(static_cast<float>(radius) - distance + 0.5f, 0.0f, 1.0f);
            if (coverage > 0.0f) target_.Blend(px, py, color, alpha * (radius > 0 ? coverage : 1.0f));
        }
    }
}

void Canvas::StrokeRect(int x, int y, int width, int height, Rgba8 color, int thickness, float alpha) {
    FillRect(x, y, width, thickness, color, alpha);
    FillRect(x, y + height - thickness, width, thickness, color, alpha);
    FillRect(x, y + thickness, thickness, height - 2 * thickness, color, alpha);
    FillRect(x + width - thickness, y + thickness, thickness, height - 2 * thickness, color, alpha);
}

void Canvas::Bar(int x, int y, int width, int height, float fraction, Rgba8 fill, Rgba8 back, float alpha) {
    if (!std::isfinite(fraction)) fraction = 0.0f;
    fraction = std::clamp(fraction, 0.0f, 1.0f);
    FillRect(x, y, width, height, back, alpha);
    FillRect(x, y, static_cast<int>(std::lround(static_cast<float>(width) * fraction)), height, fill, alpha);
}

void Canvas::Ring(int centerX, int centerY, float outerRadius, float thickness, float fraction, Rgba8 color, float alpha) {
    if (!std::isfinite(fraction) || fraction <= 0.0f || outerRadius <= 0.0f) return;
    fraction = std::min(fraction, 1.0f);
    const float innerRadius = std::max(0.0f, outerRadius - thickness);
    const int r = static_cast<int>(std::ceil(outerRadius)) + 1;
    constexpr float kTwoPi = 6.28318530718f;
    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            const float fx = static_cast<float>(dx) + 0.5f, fy = static_cast<float>(dy) + 0.5f;
            const float distance = std::sqrt(fx * fx + fy * fy);
            const float coverage = std::clamp(outerRadius - distance + 0.5f, 0.0f, 1.0f)
                * std::clamp(distance - innerRadius + 0.5f, 0.0f, 1.0f);
            if (coverage <= 0.0f) continue;
            float angle = std::atan2(fx, -fy); // 0 at 12 o'clock, clockwise positive
            if (angle < 0.0f) angle += kTwoPi;
            if (angle / kTwoPi > fraction) continue;
            target_.Blend(centerX + dx, centerY + dy, color, alpha * coverage);
        }
    }
}

void Canvas::FillCircle(int centerX, int centerY, float radius, Rgba8 color, float alpha) {
    Ring(centerX, centerY, radius, radius + 1.0f, 1.0f, color, alpha);
}

void Canvas::Line(int x0, int y0, int x1, int y1, Rgba8 color, float alpha) {
    const int steps = std::max(std::abs(x1 - x0), std::abs(y1 - y0));
    for (int i = 0; i <= steps; ++i) {
        const float t = steps > 0 ? static_cast<float>(i) / static_cast<float>(steps) : 0.0f;
        target_.Blend(static_cast<int>(std::lround(x0 + (x1 - x0) * t)), static_cast<int>(std::lround(y0 + (y1 - y0) * t)), color, alpha);
    }
}

void Canvas::Glyph(int x, int y, char c, Rgba8 color, int scale) {
    const unsigned char* rows = FontGlyph(c);
    if (!rows) rows = FontGlyph('?');
    for (int row = 0; row < 7; ++row) {
        for (int column = 0; column < 5; ++column) {
            if (!(rows[row] & (1u << (4 - column)))) continue;
            FillRect(x + column * scale, y + row * scale, scale, scale, color, 1.0f);
        }
    }
}

int Canvas::Text(int x, int y, const std::string& text, const TextStyle& style) {
    const int scale = std::max(1, style.scale);
    int penX = x, penY = y;
    for (char c : text) {
        if (c == '\n') {
            penX = x;
            penY += LineHeight(scale);
            continue;
        }
        if (style.shadow) Glyph(penX + scale, penY + scale, c, style.shadowColor, scale);
        Glyph(penX, penY, c, style.color, scale);
        penX += 6 * scale;
    }
    return penX;
}

int Canvas::MeasureText(const std::string& text, int scale) {
    scale = std::max(1, scale);
    int longest = 0, current = 0;
    for (char c : text) {
        if (c == '\n') {
            longest = std::max(longest, current);
            current = 0;
        } else {
            current += 6 * scale;
        }
    }
    return std::max(longest, current);
}

} // namespace Astral::Graphics
