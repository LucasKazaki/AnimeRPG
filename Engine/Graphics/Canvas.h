#pragma once

// Immediate-mode 2D drawing onto the final sRGB image: HUD panels, bars,
// radial cooldown rings and text from a built-in 5x7 bitmap font (ASCII 32-126).
// This gives the engine its own UI/text path that does not depend on GDI, so
// headless captures and future GPU backends show the same HUD.

#include "Engine/Graphics/Image.h"

#include <string>

namespace Astral::Graphics {

struct TextStyle {
    Rgba8 color{255, 255, 255, 255};
    int scale{2};
    bool shadow{true};
    Rgba8 shadowColor{10, 8, 24, 200};
};

class Canvas {
public:
    explicit Canvas(ImageRgba8& target) : target_(target) {}

    void FillRect(int x, int y, int width, int height, Rgba8 color, float alpha = 1.0f);
    // Vertical gradient from top colour to bottom colour.
    void FillGradient(int x, int y, int width, int height, Rgba8 top, Rgba8 bottom, float alpha = 1.0f);
    void FillRoundedRect(int x, int y, int width, int height, int radius, Rgba8 color, float alpha = 1.0f);
    void StrokeRect(int x, int y, int width, int height, Rgba8 color, int thickness = 1, float alpha = 1.0f);
    // Horizontal fill bar; fraction is clamped to [0, 1].
    void Bar(int x, int y, int width, int height, float fraction, Rgba8 fill, Rgba8 back, float alpha = 1.0f);
    // Anti-aliased ring arc starting at 12 o'clock, clockwise, covering `fraction`.
    void Ring(int centerX, int centerY, float outerRadius, float thickness, float fraction, Rgba8 color, float alpha = 1.0f);
    void FillCircle(int centerX, int centerY, float radius, Rgba8 color, float alpha = 1.0f);
    void Line(int x0, int y0, int x1, int y1, Rgba8 color, float alpha = 1.0f);

    // Returns the pen x after the text. '\n' starts a new line.
    int Text(int x, int y, const std::string& text, const TextStyle& style = {});
    static int MeasureText(const std::string& text, int scale);
    static int LineHeight(int scale) { return 9 * scale; }

    ImageRgba8& Target() { return target_; }

private:
    void Glyph(int x, int y, char c, Rgba8 color, int scale);
    ImageRgba8& target_;
};

// Returns the 7 row bitmasks (bit 4 = leftmost column) of a glyph, or nullptr.
const unsigned char* FontGlyph(char c);

} // namespace Astral::Graphics
