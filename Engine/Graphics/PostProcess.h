#pragma once

// Post-processing chain for the software renderer: ink outlines from
// depth/normal/object discontinuities, multi-level bloom, Khronos PBR Neutral
// tone mapping, colour grading, FXAA, chromatic aberration, vignette and
// ordered-hash dithering into the final sRGB image.

#include "Engine/Core/JobSystem.h"
#include "Engine/Graphics/RenderScene.h"
#include "Engine/Graphics/RenderTarget.h"

#include <functional>
#include <vector>

namespace Astral::Graphics {

// Khronos PBR Neutral tone mapper (hue-preserving; suits stylised albedo).
Color ToneMapNeutral(Color color);

class PostProcessor {
public:
    void Run(RenderTarget& target, const PostSettings& settings, Core::JobSystem* jobs);

private:
    struct Level {
        int width{}, height{};
        std::vector<Color> pixels;
        std::vector<Color> scratch;
    };
    void ParallelRows(Core::JobSystem* jobs, int rows, const std::function<void(int, int)>& fn);
    void Outlines(RenderTarget& target, const PostSettings& settings, Core::JobSystem* jobs);
    void Bloom(RenderTarget& target, const PostSettings& settings, Core::JobSystem* jobs);
    Color SampleBloom(float u, float v) const;

    std::vector<Level> bloom_;
    std::vector<Color> ldr_;
    std::vector<Color> ldrScratch_;
    std::vector<Color> outlineScratch_;
};

} // namespace Astral::Graphics
