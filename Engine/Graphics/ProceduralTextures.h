#pragma once

// Deterministic, tileable procedural textures. They give the engine fixtures
// and the National Mall blockout real surface detail without external assets
// or licensing questions.

#include "Engine/Graphics/Texture.h"

namespace Astral::Graphics::Procedural {

Texture2D Checker(int size, int cells, Color a, Color b);
// White marble with soft grey veins (Lincoln Memorial, Washington Monument).
Texture2D Marble(int size, Color base, Color vein, std::uint32_t seed = 1);
// Mown lawn with subtle stripes and clover noise.
Texture2D Grass(int size, Color light, Color dark, std::uint32_t seed = 2);
// Compacted gravel walkway.
Texture2D Gravel(int size, Color base, std::uint32_t seed = 3);
// Rectangular stone paving with grout lines.
Texture2D Paving(int size, int tilesX, int tilesY, Color stone, Color grout, std::uint32_t seed = 4);
// Soft radial falloff for particles (alpha channel carries the shape).
Texture2D SoftDisc(int size);
// Horizontal colour ramp, e.g. for toon lookups or UI gradients.
Texture2D Gradient(int width, Color left, Color right);

} // namespace Astral::Graphics::Procedural
