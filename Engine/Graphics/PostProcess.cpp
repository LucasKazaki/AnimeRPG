#include "Engine/Graphics/PostProcess.h"

#include "Engine/Core/Profiler.h"
#include "Engine/Core/Random.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace Astral::Graphics {

using namespace Math;

Color ToneMapNeutral(Color color) {
    // Khronos PBR Neutral (2024): linear below ~0.76, smooth highlight roll-off,
    // mild desaturation of the brightest values only.
    constexpr float startCompression = 0.8f - 0.04f;
    constexpr float desaturation = 0.15f;
    const float x = std::min(color.x, std::min(color.y, color.z));
    const float offset = x < 0.08f ? x - 6.25f * x * x : 0.04f;
    color = color - Color{offset, offset, offset};
    const float peak = std::max(color.x, std::max(color.y, color.z));
    if (peak < startCompression) return color;
    const float d = 1.0f - startCompression;
    const float newPeak = 1.0f - d * d / (peak + d - startCompression);
    color = color * (newPeak / peak);
    const float g = 1.0f - 1.0f / (desaturation * (peak - newPeak) + 1.0f);
    return Lerp(color, Color{newPeak, newPeak, newPeak}, g);
}

namespace {

float PerceptualLuma(Color c) { return std::sqrt(std::max(0.0f, Luminance(c))); }

// Linear [0,1] -> sRGB value in [0,255] as float, via a 4096-entry table.
const std::array<float, 4097>& EncodeTable() {
    static const std::array<float, 4097> table = [] {
        std::array<float, 4097> values{};
        for (int i = 0; i <= 4096; ++i) values[static_cast<std::size_t>(i)] = LinearToSrgb(i / 4096.0f) * 255.0f;
        return values;
    }();
    return table;
}

float EncodeChannel(float linear) {
    if (!(linear > 0.0f)) return 0.0f;
    if (linear >= 1.0f) return 255.0f;
    const float scaled = linear * 4096.0f;
    const int index = static_cast<int>(scaled);
    const float fraction = scaled - static_cast<float>(index);
    const auto& table = EncodeTable();
    return Lerp(table[static_cast<std::size_t>(index)], table[static_cast<std::size_t>(std::min(index + 1, 4096))], fraction);
}

} // namespace

void PostProcessor::ParallelRows(Core::JobSystem* jobs, int rows, const std::function<void(int, int)>& fn) {
    auto adapter = [&fn](std::size_t begin, std::size_t end) { fn(static_cast<int>(begin), static_cast<int>(end)); };
    if (jobs) jobs->ParallelFor(static_cast<std::size_t>(std::max(0, rows)), 8, adapter);
    else if (rows > 0) adapter(0, static_cast<std::size_t>(rows));
}

void PostProcessor::Outlines(RenderTarget& target, const PostSettings& settings, Core::JobSystem* jobs) {
    ASTRAL_PROFILE_SCOPE("Post.Outlines");
    const int width = target.Width(), height = target.Height();
    const int reach = std::max(1, static_cast<int>(std::lround(static_cast<float>(height) / 720.0f)));
    outlineScratch_.assign(target.hdr.begin(), target.hdr.end());
    ParallelRows(jobs, height, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            for (int x = 0; x < width; ++x) {
                const std::size_t p = target.Index(x, y);
                if (!(target.flags[p] & kPixelOutline)) continue;
                const float depth = target.linearDepth[p];
                const std::uint32_t id = target.objectId[p];
                const Vec3 n = target.normal[p];
                float edge = 0.0f;
                const int offsets[4][2] = {{reach, 0}, {-reach, 0}, {0, reach}, {0, -reach}};
                for (const auto& offset : offsets) {
                    const int nx = x + offset[0], ny = y + offset[1];
                    if (nx < 0 || ny < 0 || nx >= width || ny >= height) continue;
                    const std::size_t q = target.Index(nx, ny);
                    const float otherDepth = target.linearDepth[q];
                    // Lines belong to the nearer surface, giving crisp silhouettes.
                    if (otherDepth > depth) {
                        const float relative = (otherDepth - depth) / std::max(depth, 1.0e-3f);
                        if (relative > settings.outlineDepthThreshold) edge = 1.0f;
                        else if (target.objectId[q] != id && !(target.flags[q] & kPixelWater)) edge = std::max(edge, 0.85f);
                    }
                    if ((offset[0] > 0 || offset[1] > 0) && target.objectId[q] == id
                        && 1.0f - Dot(n, target.normal[q]) > settings.outlineNormalThreshold) {
                        edge = std::max(edge, 0.7f);
                    }
                }
                if (edge > 0.0f) {
                    const Color base = target.hdr[p];
                    const Color ink = Multiply(base, Color{settings.outlineDarkness, settings.outlineDarkness, settings.outlineDarkness})
                        + settings.outlineTint * 0.05f;
                    outlineScratch_[p] = Lerp(base, ink, edge);
                }
            }
        }
    });
    target.hdr.swap(outlineScratch_);
}

void PostProcessor::Bloom(RenderTarget& target, const PostSettings& settings, Core::JobSystem* jobs) {
    ASTRAL_PROFILE_SCOPE("Post.Bloom");
    const int levels = 5;
    bloom_.resize(levels);
    int w = std::max(1, target.Width() / 2), h = std::max(1, target.Height() / 2);
    for (Level& level : bloom_) {
        level.width = w;
        level.height = h;
        level.pixels.assign(static_cast<std::size_t>(w) * static_cast<std::size_t>(h), {});
        level.scratch.assign(level.pixels.size(), {});
        w = std::max(1, w / 2);
        h = std::max(1, h / 2);
    }
    // Threshold + 2x2 downsample into level 0 (soft knee keeps edges smooth).
    const float threshold = settings.bloomThreshold;
    const float knee = threshold * 0.5f;
    Level& first = bloom_[0];
    ParallelRows(jobs, first.height, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            for (int x = 0; x < first.width; ++x) {
                Color sum{};
                for (int dy = 0; dy < 2; ++dy)
                    for (int dx = 0; dx < 2; ++dx)
                        sum += target.hdr[target.Index(std::min(target.Width() - 1, x * 2 + dx), std::min(target.Height() - 1, y * 2 + dy))];
                sum *= 0.25f;
                const float brightness = std::max(sum.x, std::max(sum.y, sum.z));
                float soft = brightness - threshold + knee;
                soft = Clamp(soft, 0.0f, 2.0f * knee);
                soft = soft * soft / (4.0f * knee + 1.0e-5f);
                const float contribution = std::max(soft, brightness - threshold) / std::max(brightness, 1.0e-5f);
                first.pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(first.width) + static_cast<std::size_t>(x)] = sum * contribution;
            }
        }
    });
    for (std::size_t i = 1; i < bloom_.size(); ++i) {
        const Level& source = bloom_[i - 1];
        Level& level = bloom_[i];
        for (int y = 0; y < level.height; ++y) {
            for (int x = 0; x < level.width; ++x) {
                Color sum{};
                for (int dy = 0; dy < 2; ++dy)
                    for (int dx = 0; dx < 2; ++dx)
                        sum += source.pixels[static_cast<std::size_t>(std::min(source.height - 1, y * 2 + dy)) * static_cast<std::size_t>(source.width)
                            + static_cast<std::size_t>(std::min(source.width - 1, x * 2 + dx))];
                level.pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(level.width) + static_cast<std::size_t>(x)] = sum * 0.25f;
            }
        }
    }
    // Separable [1 4 6 4 1] blur on every level.
    const float weights[5] = {1.0f / 16.0f, 4.0f / 16.0f, 6.0f / 16.0f, 4.0f / 16.0f, 1.0f / 16.0f};
    for (Level& level : bloom_) {
        for (int pass = 0; pass < 2; ++pass) {
            const std::vector<Color>& in = pass == 0 ? level.pixels : level.scratch;
            std::vector<Color>& out = pass == 0 ? level.scratch : level.pixels;
            ParallelRows(jobs, level.height, [&](int begin, int end) {
                for (int y = begin; y < end; ++y) {
                    for (int x = 0; x < level.width; ++x) {
                        Color sum{};
                        for (int k = -2; k <= 2; ++k) {
                            const int sx = pass == 0 ? std::clamp(x + k, 0, level.width - 1) : x;
                            const int sy = pass == 1 ? std::clamp(y + k, 0, level.height - 1) : y;
                            sum += in[static_cast<std::size_t>(sy) * static_cast<std::size_t>(level.width) + static_cast<std::size_t>(sx)] * weights[k + 2];
                        }
                        out[static_cast<std::size_t>(y) * static_cast<std::size_t>(level.width) + static_cast<std::size_t>(x)] = sum;
                    }
                }
            });
        }
    }
    // Upsample-accumulate from the smallest level into level 0.
    for (std::size_t i = bloom_.size() - 1; i > 0; --i) {
        const Level& small = bloom_[i];
        Level& large = bloom_[i - 1];
        for (int y = 0; y < large.height; ++y) {
            for (int x = 0; x < large.width; ++x) {
                const float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(large.width) * static_cast<float>(small.width) - 0.5f;
                const float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(large.height) * static_cast<float>(small.height) - 0.5f;
                const int x0 = std::clamp(static_cast<int>(std::floor(u)), 0, small.width - 1);
                const int y0 = std::clamp(static_cast<int>(std::floor(v)), 0, small.height - 1);
                const int x1 = std::min(small.width - 1, x0 + 1), y1 = std::min(small.height - 1, y0 + 1);
                const float fx = Saturate(u - std::floor(u)), fy = Saturate(v - std::floor(v));
                auto at = [&small](int sx, int sy) {
                    return small.pixels[static_cast<std::size_t>(sy) * static_cast<std::size_t>(small.width) + static_cast<std::size_t>(sx)];
                };
                const Color up = Lerp(Lerp(at(x0, y0), at(x1, y0), fx), Lerp(at(x0, y1), at(x1, y1), fx), fy);
                large.pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(large.width) + static_cast<std::size_t>(x)] += up;
            }
        }
    }
}

Color PostProcessor::SampleBloom(float u, float v) const {
    const Level& level = bloom_[0];
    const float fx = u * static_cast<float>(level.width) - 0.5f;
    const float fy = v * static_cast<float>(level.height) - 0.5f;
    const int x0 = std::clamp(static_cast<int>(std::floor(fx)), 0, level.width - 1);
    const int y0 = std::clamp(static_cast<int>(std::floor(fy)), 0, level.height - 1);
    const int x1 = std::min(level.width - 1, x0 + 1), y1 = std::min(level.height - 1, y0 + 1);
    const float tx = Saturate(fx - std::floor(fx)), ty = Saturate(fy - std::floor(fy));
    auto at = [&level](int x, int y) {
        return level.pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(level.width) + static_cast<std::size_t>(x)];
    };
    return Lerp(Lerp(at(x0, y0), at(x1, y0), tx), Lerp(at(x0, y1), at(x1, y1), tx), ty);
}

void PostProcessor::Run(RenderTarget& target, const PostSettings& settings, Core::JobSystem* jobs) {
    ASTRAL_PROFILE_SCOPE("Post");
    const int width = target.Width(), height = target.Height();
    if (settings.outlines) Outlines(target, settings, jobs);
    const bool bloom = settings.bloom && settings.bloomIntensity > 0.0f;
    if (bloom) Bloom(target, settings, jobs);

    // Exposure, bloom composite, tone map and grade into display-linear LDR.
    ldr_.resize(target.PixelCount());
    {
    ASTRAL_PROFILE_SCOPE("Post.ToneMap");
    ParallelRows(jobs, height, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            for (int x = 0; x < width; ++x) {
                const std::size_t p = target.Index(x, y);
                Color c = target.hdr[p];
                if (!IsFinite(c)) c = {1.0f, 0.0f, 1.0f};
                if (bloom) {
                    c += SampleBloom((static_cast<float>(x) + 0.5f) / static_cast<float>(width),
                             (static_cast<float>(y) + 0.5f) / static_cast<float>(height)) * settings.bloomIntensity;
                }
                c = ToneMapNeutral(Max(c * settings.exposure, Color{}));
                c = Saturation(c, settings.saturation);
                c = Multiply(c, settings.tint);
                // Contrast around mid grey in a perceptual (sqrt) space.
                const Color perceptual{std::sqrt(std::max(0.0f, c.x)), std::sqrt(std::max(0.0f, c.y)), std::sqrt(std::max(0.0f, c.z))};
                const Color contrasted = (perceptual - Color{0.5f, 0.5f, 0.5f}) * settings.contrast + Color{0.5f, 0.5f, 0.5f};
                const Color clamped = Max(contrasted, Color{});
                ldr_[p] = Min(Multiply(clamped, clamped), Color{1.0f, 1.0f, 1.0f});
            }
        }
    });
    }

    if (settings.fxaa) {
        ASTRAL_PROFILE_SCOPE("Post.FXAA");
        ldrScratch_ = ldr_;
        ParallelRows(jobs, height, [&](int begin, int end) {
            auto luma = [&](int x, int y) {
                x = std::clamp(x, 0, width - 1);
                y = std::clamp(y, 0, height - 1);
                return PerceptualLuma(ldrScratch_[target.Index(x, y)]);
            };
            for (int y = begin; y < end; ++y) {
                for (int x = 0; x < width; ++x) {
                    const float m = luma(x, y), n = luma(x, y - 1), s = luma(x, y + 1), e = luma(x + 1, y), w = luma(x - 1, y);
                    const float maxLuma = std::max(m, std::max(std::max(n, s), std::max(e, w)));
                    const float minLuma = std::min(m, std::min(std::min(n, s), std::min(e, w)));
                    const float range = maxLuma - minLuma;
                    if (range < std::max(0.0312f, maxLuma * 0.125f)) continue;
                    const float nw = luma(x - 1, y - 1), ne = luma(x + 1, y - 1), sw = luma(x - 1, y + 1), se = luma(x + 1, y + 1);
                    const float horizontal = std::fabs(n + s - 2.0f * m) * 2.0f + std::fabs(ne + se - 2.0f * e) + std::fabs(nw + sw - 2.0f * w);
                    const float vertical = std::fabs(e + w - 2.0f * m) * 2.0f + std::fabs(ne + nw - 2.0f * n) + std::fabs(se + sw - 2.0f * s);
                    const bool isHorizontal = horizontal >= vertical;
                    const float l1 = isHorizontal ? n : w;
                    const float l2 = isHorizontal ? s : e;
                    const bool towardFirst = std::fabs(l1 - m) >= std::fabs(l2 - m);
                    const float average = (2.0f * (n + s + e + w) + nw + ne + sw + se) / 12.0f;
                    float blend = Saturate(std::fabs(average - m) / range);
                    blend = SmoothStep(0.0f, 1.0f, blend);
                    blend = blend * blend * 0.75f;
                    // Edge blend: at least a quarter-pixel step toward the steeper side.
                    blend = std::max(blend, 0.25f);
                    const int ox = isHorizontal ? 0 : (towardFirst ? -1 : 1);
                    const int oy = isHorizontal ? (towardFirst ? -1 : 1) : 0;
                    const Color neighbour = ldrScratch_[target.Index(std::clamp(x + ox, 0, width - 1), std::clamp(y + oy, 0, height - 1))];
                    ldr_[target.Index(x, y)] = Lerp(ldrScratch_[target.Index(x, y)], neighbour, blend);
                }
            }
        });
    }

    // Chromatic aberration, vignette and dithered sRGB encode.
    ASTRAL_PROFILE_SCOPE("Post.Encode");
    const float cx = static_cast<float>(width) * 0.5f, cy = static_cast<float>(height) * 0.5f;
    const float inverseRadius = 1.0f / std::sqrt(cx * cx + cy * cy);
    ParallelRows(jobs, height, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            for (int x = 0; x < width; ++x) {
                const std::size_t p = target.Index(x, y);
                const float dx = (static_cast<float>(x) + 0.5f - cx) * inverseRadius;
                const float dy = (static_cast<float>(y) + 0.5f - cy) * inverseRadius;
                const float r2 = dx * dx + dy * dy;
                Color c = ldr_[p];
                if (settings.chromaticAberration > 0.0f) {
                    const float shift = settings.chromaticAberration * r2;
                    const int rx = std::clamp(static_cast<int>(std::lround(static_cast<float>(x) + dx * shift)), 0, width - 1);
                    const int ry = std::clamp(static_cast<int>(std::lround(static_cast<float>(y) + dy * shift)), 0, height - 1);
                    const int bx = std::clamp(static_cast<int>(std::lround(static_cast<float>(x) - dx * shift)), 0, width - 1);
                    const int by = std::clamp(static_cast<int>(std::lround(static_cast<float>(y) - dy * shift)), 0, height - 1);
                    c.x = ldr_[target.Index(rx, ry)].x;
                    c.z = ldr_[target.Index(bx, by)].z;
                }
                const float vignette = 1.0f - settings.vignette * SmoothStep(0.15f, 1.0f, r2 * 1.6f);
                c = c * vignette;
                const float dither = settings.dither
                    ? (static_cast<float>(Core::Hash32(static_cast<std::uint32_t>(x) * 1973u + static_cast<std::uint32_t>(y) * 9277u) & 1023u) / 1023.0f - 0.5f)
                    : 0.0f;
                auto encode = [dither](float linear) {
                    return static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::lround(EncodeChannel(linear) + dither)), 0, 255));
                };
                target.output.Set(x, y, {encode(c.x), encode(c.y), encode(c.z), 255});
            }
        }
    });
}

} // namespace Astral::Graphics
