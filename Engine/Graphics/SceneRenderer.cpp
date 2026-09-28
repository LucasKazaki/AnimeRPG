#include "Engine/Graphics/SceneRenderer.h"

#include "Engine/Core/Profiler.h"
#include "Engine/Core/Random.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>

namespace Astral::Graphics {

using namespace Math;

namespace {

using SteadyClock = std::chrono::steady_clock;
double MillisecondsSince(SteadyClock::time_point start) {
    return std::chrono::duration<double, std::milli>(SteadyClock::now() - start).count();
}

const Material& DefaultMaterial() {
    static const Material material;
    return material;
}

float ValueNoise(float x, float y) {
    const float fx = std::floor(x), fy = std::floor(y);
    const int ix = static_cast<int>(fx), iy = static_cast<int>(fy);
    const float tx = x - fx, ty = y - fy;
    auto corner = [](int cx, int cy) {
        return static_cast<float>(Core::Hash32(static_cast<std::uint32_t>(cx) * 73856093u
            ^ static_cast<std::uint32_t>(cy) * 19349663u) & 0xFFFFu) / 65535.0f;
    };
    const float sx = tx * tx * (3.0f - 2.0f * tx);
    const float sy = ty * ty * (3.0f - 2.0f * ty);
    return Lerp(Lerp(corner(ix, iy), corner(ix + 1, iy), sx), Lerp(corner(ix, iy + 1), corner(ix + 1, iy + 1), sx), sy);
}

float Fbm(float x, float y, int octaves) {
    float sum = 0.0f, amplitude = 0.5f, frequency = 1.0f, norm = 0.0f;
    for (int i = 0; i < octaves; ++i) {
        sum += amplitude * ValueNoise(x * frequency, y * frequency);
        norm += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.03f;
    }
    return sum / norm;
}

Vec3 SafeNormal(Vec3 n) { return Normalize(n, {0.0f, 1.0f, 0.0f}); }

// GGX / Smith-Schlick specular with a Lambert diffuse lobe.
Color ShadePbr(Color albedo, float roughness, float metallic, Vec3 n, Vec3 v, Vec3 l, Color radiance) {
    const float nl = std::max(0.0f, Dot(n, l));
    if (nl <= 0.0f) return {};
    const Vec3 h = Normalize(l + v, n);
    const float nv = std::max(1.0e-4f, Dot(n, v));
    const float nh = std::max(0.0f, Dot(n, h));
    const float vh = std::max(0.0f, Dot(v, h));
    const float a = std::max(0.02f, roughness * roughness);
    const float a2 = a * a;
    const float denom = nh * nh * (a2 - 1.0f) + 1.0f;
    const float d = a2 / (kPi * denom * denom);
    const float k = (roughness + 1.0f) * (roughness + 1.0f) / 8.0f;
    const float g = (nv / (nv * (1.0f - k) + k)) * (nl / (nl * (1.0f - k) + k));
    const Color f0 = Lerp(Color{0.04f, 0.04f, 0.04f}, albedo, metallic);
    const float fw = std::pow(1.0f - vh, 5.0f);
    const Color f = f0 + (Color{1, 1, 1} - f0) * fw;
    const Color specular = f * (d * g / (4.0f * nv * nl + 1.0e-4f));
    const Color diffuse = Multiply(Color{1, 1, 1} - f, albedo) * ((1.0f - metallic) / kPi);
    return Multiply(diffuse * kPi + specular, radiance) * nl;
}

// Sum-of-sines height field gradient for the water surface.
Vec3 WaterNormal(Vec3 p, float time, float amplitude, float scale) {
    struct Wave { float dx, dz, frequency, speed, weight; };
    static const Wave waves[4] = {
        {1.0f, 0.3f, 0.9f, 1.1f, 1.0f}, {-0.4f, 1.0f, 1.7f, 1.6f, 0.6f},
        {0.7f, -0.8f, 3.1f, 2.3f, 0.35f}, {-1.0f, -0.2f, 5.3f, 3.0f, 0.2f}};
    float gx = 0.0f, gz = 0.0f;
    for (const Wave& w : waves) {
        const float length = std::sqrt(w.dx * w.dx + w.dz * w.dz);
        const float dx = w.dx / length, dz = w.dz / length;
        const float f = w.frequency * scale;
        const float phase = (p.x * dx + p.z * dz) * f + time * w.speed;
        const float c = std::cos(phase) * w.weight * f;
        gx += c * dx;
        gz += c * dz;
    }
    return Normalize(Vec3{-gx * amplitude, 1.0f, -gz * amplitude}, {0, 1, 0});
}

} // namespace

void SceneRenderer::ParallelFor(std::size_t count, std::size_t batch,
    const std::function<void(std::size_t, std::size_t)>& fn) {
    if (jobs_) jobs_->ParallelFor(count, batch, fn);
    else if (count > 0) fn(0, count);
}

Color SceneRenderer::SkyColor(const RenderScene& scene, Vec3 dir) const {
    const SkySettings& sky = scene.sky;
    Color color;
    if (dir.y >= 0.0f) {
        color = Lerp(sky.horizon, sky.zenith, std::pow(Saturate(dir.y), 0.55f));
        if (settings_.clouds && dir.y > 0.02f) {
            // Stylised cumulus on a plane 1 unit above the eye: crisp anime edges.
            const float scale = 1.0f / (dir.y + 0.08f);
            const float u = dir.x * scale * 1.6f + 3.7f;
            const float v = dir.z * scale * 1.6f + 1.3f;
            const float density = Fbm(u, v, 5);
            const float coverage = SmoothStep(0.52f, 0.56f, density);
            if (coverage > 0.0f) {
                const float shade = SmoothStep(0.52f, 0.75f, Fbm(u + 0.35f, v + 0.35f, 3));
                const Color lit = Color{1.0f, 0.98f, 0.96f} * 1.25f;
                const Color shadow = Color{0.62f, 0.66f, 0.82f};
                const float fade = SmoothStep(0.02f, 0.18f, dir.y);
                color = Lerp(color, Lerp(lit, shadow, shade * 0.8f), coverage * fade);
            }
        }
    } else {
        color = Lerp(sky.horizon, sky.ground, Saturate(-dir.y * 5.0f));
    }
    const Vec3 toSun = -Normalize(scene.sun.direction, {0, -1, 0});
    const float cosine = Dot(dir, toSun);
    const float size = std::cos(Radians(sky.sunSizeDegrees));
    const float disc = SmoothStep(size - 0.0004f, size + 0.0002f, cosine);
    const float glow = std::pow(std::max(0.0f, cosine), 48.0f) * 0.12f + std::pow(std::max(0.0f, cosine), 6.0f) * 0.05f;
    return color + sky.sunColor * (disc + glow * 0.2f);
}

void SceneRenderer::RenderShadowMap(const RenderScene& scene) {
    ASTRAL_PROFILE_SCOPE("Render.Shadows");
    const ShadowSettings& settings = scene.shadows;
    shadow_.valid = false;
    const int size = std::clamp(settings.resolution, 64, 8192);
    const float radius = std::max(1.0f, settings.radius);
    const Vec3 lightDir = Normalize(scene.sun.direction, {0, -1, 0});
    const Vec3 up = std::fabs(lightDir.y) > 0.99f ? Vec3{0, 0, 1} : Vec3{0, 1, 0};
    // Depth range covers casters well above and below the focus region.
    const float range = radius * 4.0f;
    Mat4 lightView = LookAtLH(settings.focus - lightDir * (range * 0.5f), settings.focus, up);
    // Snap the light-space origin to whole texels so the map does not shimmer.
    const float texel = 2.0f * radius / static_cast<float>(size);
    const Vec3 focusLight = TransformPoint(lightView, settings.focus);
    const float snapX = std::round(focusLight.x / texel) * texel - focusLight.x;
    const float snapY = std::round(focusLight.y / texel) * texel - focusLight.y;
    lightView = Translation({-snapX, -snapY, 0.0f}) * lightView;
    const Vec3 center = TransformPoint(lightView, settings.focus);
    const Mat4 projection = OrthographicOffCenterLH(center.x - radius, center.x + radius,
        center.y - radius, center.y + radius, 0.0f, range);
    shadow_.viewProjection = projection * lightView;
    shadow_.size = size;
    shadow_.texelWorld = texel;
    shadow_.depthRange = range;
    shadow_.depthBias = settings.depthBias;
    shadow_.normalBias = settings.normalBias;
    shadow_.depth.assign(static_cast<std::size_t>(size) * static_cast<std::size_t>(size), 1.0f);

    const Frustum lightFrustum = Frustum::FromViewProjection(shadow_.viewProjection);
    drawList_.clear();
    for (std::uint32_t i = 0; i < scene.draws.size(); ++i) {
        const DrawItem& draw = scene.draws[i];
        const Material& material = draw.material ? *draw.material : DefaultMaterial();
        if (!draw.mesh || !material.castShadows || material.blend != BlendMode::Opaque) continue;
        if (!lightFrustum.Intersects(TransformAABB(draw.world, draw.mesh->bounds))) continue;
        drawList_.push_back(i);
    }
    std::vector<SetupTriangle> shadowTriangles;
    BuildTriangles(scene, drawList_, shadow_.viewProjection, size, size, true, shadowTriangles);
    stats_.shadowTriangles = shadowTriangles.size();
    TileBins bins;
    bins.Build(shadowTriangles, size, size, 128);
    ParallelFor(static_cast<std::size_t>(bins.TileCount()), 1, [&](std::size_t begin, std::size_t end) {
        for (std::size_t tile = begin; tile < end; ++tile) {
            const int tx = static_cast<int>(tile) % bins.tilesX;
            const int ty = static_cast<int>(tile) / bins.tilesX;
            const int x0 = tx * bins.tileSize, y0 = ty * bins.tileSize;
            const int x1 = std::min(size, x0 + bins.tileSize), y1 = std::min(size, y0 + bins.tileSize);
            for (std::uint32_t index : bins.bins[tile]) {
                RasterizeTriangle(shadowTriangles[index], x0, y0, x1, y1,
                    [&](int x, int y, float z, float, float, float) {
                        float& stored = shadow_.depth[static_cast<std::size_t>(y) * static_cast<std::size_t>(size) + static_cast<std::size_t>(x)];
                        if (z < stored) stored = z;
                    });
            }
        }
    });
    shadow_.valid = true;
}

float SceneRenderer::ShadowFactor(Vec3 p, Vec3 n) const {
    if (!shadow_.valid) return 1.0f;
    const Vec3 offset = p + n * (shadow_.normalBias + shadow_.texelWorld * 1.5f);
    const Vec4 clip = shadow_.viewProjection * Vec4{offset.x, offset.y, offset.z, 1.0f};
    const float u = clip.x * 0.5f + 0.5f;
    const float v = 0.5f - clip.y * 0.5f;
    if (u <= 0.0f || u >= 1.0f || v <= 0.0f || v >= 1.0f || clip.z >= 1.0f) return 1.0f;
    const float z = clip.z - shadow_.depthBias;
    const float s = u * static_cast<float>(shadow_.size) - 0.5f;
    const float t = v * static_cast<float>(shadow_.size) - 0.5f;
    const float bs = std::floor(s), bt = std::floor(t);
    const float fs = s - bs, ft = t - bt;
    const int ix = static_cast<int>(bs), iy = static_cast<int>(bt);
    const float wx[4] = {1.0f - fs, 1.0f, 1.0f, fs};
    const float wy[4] = {1.0f - ft, 1.0f, 1.0f, ft};
    float lit = 0.0f;
    for (int j = 0; j < 4; ++j) {
        const int y = std::clamp(iy - 1 + j, 0, shadow_.size - 1);
        for (int i = 0; i < 4; ++i) {
            const int x = std::clamp(ix - 1 + i, 0, shadow_.size - 1);
            const float stored = shadow_.depth[static_cast<std::size_t>(y) * static_cast<std::size_t>(shadow_.size) + static_cast<std::size_t>(x)];
            if (z <= stored) lit += wx[i] * wy[j];
        }
    }
    return lit / 9.0f;
}

void SceneRenderer::BuildTriangles(const RenderScene& scene, const std::vector<std::uint32_t>& drawIndices,
    const Mat4& viewProjection, int width, int height, bool shadowPass, std::vector<SetupTriangle>& out) {
    out.clear();
    if (perDraw_.size() < drawIndices.size()) perDraw_.resize(drawIndices.size());
    ParallelFor(drawIndices.size(), 1, [&](std::size_t begin, std::size_t end) {
        std::vector<ClipVertex> transformed;
        for (std::size_t i = begin; i < end; ++i) {
            std::vector<SetupTriangle>& local = perDraw_[i];
            local.clear();
            const std::uint32_t drawIndex = drawIndices[i];
            const DrawItem& draw = scene.draws[drawIndex];
            const MeshData& mesh = *draw.mesh;
            const Material& material = draw.material ? *draw.material : DefaultMaterial();
            const Mat4 clipFromLocal = viewProjection * draw.world;
            Mat4 inverseWorld{};
            const Mat4 normalMatrix = Inverse(draw.world, inverseWorld) ? Transpose(inverseWorld) : Mat4::Identity();
            transformed.resize(mesh.vertices.size());
            for (std::size_t v = 0; v < mesh.vertices.size(); ++v) {
                const Vertex& source = mesh.vertices[v];
                ClipVertex& cv = transformed[v];
                cv.clip = clipFromLocal * Vec4{source.position.x, source.position.y, source.position.z, 1.0f};
                if (!shadowPass) {
                    cv.world = TransformPoint(draw.world, source.position);
                    cv.normal = TransformVector(normalMatrix, source.normal);
                    cv.uv = source.uv;
                    cv.color = source.color;
                }
            }
            const CullMode cull = (shadowPass || material.doubleSided) ? CullMode::None : CullMode::Back;
            const std::size_t triangleCount = mesh.indices.size() / 3;
            for (std::size_t tri = 0; tri < triangleCount; ++tri) {
                const std::uint32_t ia = mesh.indices[tri * 3], ib = mesh.indices[tri * 3 + 1], ic = mesh.indices[tri * 3 + 2];
                if (ia >= transformed.size() || ib >= transformed.size() || ic >= transformed.size()) continue;
                ClipAndSetupTriangle(transformed[ia], transformed[ib], transformed[ic], cull, width, height, drawIndex, local);
            }
        }
    });
    std::size_t total = 0;
    for (std::size_t i = 0; i < drawIndices.size(); ++i) total += perDraw_[i].size();
    out.reserve(total);
    for (std::size_t i = 0; i < drawIndices.size(); ++i) out.insert(out.end(), perDraw_[i].begin(), perDraw_[i].end());
}

void SceneRenderer::RasterizeVisibility(RenderTarget& target) {
    ASTRAL_PROFILE_SCOPE("Render.Visibility");
    const int width = target.Width(), height = target.Height();
    std::fill(target.depth.begin(), target.depth.end(), 1.0f);
    std::fill(target.visibility.begin(), target.visibility.end(), 0u);
    bins_.Build(triangles_, width, height, settings_.tileSize);
    ParallelFor(static_cast<std::size_t>(bins_.TileCount()), 1, [&](std::size_t begin, std::size_t end) {
        for (std::size_t tile = begin; tile < end; ++tile) {
            const int tx = static_cast<int>(tile) % bins_.tilesX;
            const int ty = static_cast<int>(tile) / bins_.tilesX;
            const int x0 = tx * bins_.tileSize, y0 = ty * bins_.tileSize;
            const int x1 = std::min(width, x0 + bins_.tileSize), y1 = std::min(height, y0 + bins_.tileSize);
            for (std::uint32_t index : bins_.bins[tile]) {
                RasterizeTriangle(triangles_[index], x0, y0, x1, y1,
                    [&](int x, int y, float z, float, float l1, float l2) {
                        const std::size_t p = target.Index(x, y);
                        if (z < target.depth[p]) {
                            target.depth[p] = z;
                            target.visibility[p] = index + 1u;
                            target.barycentric[p] = {l1, l2};
                        }
                    });
            }
        }
    });
}

void SceneRenderer::Shade(const RenderScene& scene, const RenderView& view, RenderTarget& target) {
    ASTRAL_PROFILE_SCOPE("Render.Shade");
    const int width = target.Width(), height = target.Height();
    const Vec3 lightDir = Normalize(scene.sun.direction, {0, -1, 0});
    const Vec3 toLight = -lightDir;
    const Color sunRadiance = scene.sun.color * scene.sun.intensity;
    const bool shadowsOn = shadow_.valid;
    std::vector<std::size_t> shadedCounts(static_cast<std::size_t>(height), 0);
    ParallelFor(static_cast<std::size_t>(height), 4, [&](std::size_t rowBegin, std::size_t rowEnd) {
        for (std::size_t row = rowBegin; row < rowEnd; ++row) {
            const int y = static_cast<int>(row);
            const float ndcY = 1.0f - (static_cast<float>(y) + 0.5f) / static_cast<float>(height) * 2.0f;
            for (int x = 0; x < width; ++x) {
                const std::size_t p = target.Index(x, y);
                const float ndcX = (static_cast<float>(x) + 0.5f) / static_cast<float>(width) * 2.0f - 1.0f;
                const Vec3 rayDir = Normalize(forward_ + right_ * (ndcX * tanHalfX_) + up_ * (ndcY * tanHalfY_), forward_);
                const std::uint32_t visibility = target.visibility[p];
                if (visibility == 0) {
                    target.hdr[p] = SkyColor(scene, rayDir);
                    target.normal[p] = {};
                    target.linearDepth[p] = 1.0e9f;
                    target.objectId[p] = 0;
                    target.flags[p] = kPixelSky;
                    target.reflectivity[p] = 0.0f;
                    continue;
                }
                ++shadedCounts[row];
                const SetupTriangle& tri = triangles_[visibility - 1];
                const DrawItem& draw = scene.draws[tri.drawIndex];
                const Material& material = draw.material ? *draw.material : DefaultMaterial();
                const Vec2 screenBary = target.barycentric[p];
                const float l1 = screenBary.x, l2 = screenBary.y, l0 = 1.0f - l1 - l2;
                float b0, b1, b2;
                PerspectiveCorrect(tri, l0, l1, l2, b0, b1, b2);
                const ClipVertex attributes = InterpolateVertex(tri, b0, b1, b2);
                const Vec3 world = attributes.world;
                Vec3 n = SafeNormal(attributes.normal);
                const Vec3 toEye = Normalize(view.eye - world, {0, 0, -1});
                if (material.doubleSided && Dot(n, toEye) < 0.0f) n = -n;

                Color albedo = Multiply(material.baseColor, draw.tint);
                if (material.vertexColor) albedo = Multiply(albedo, XYZ(attributes.color));
                if (material.baseTexture) {
                    const Vec2 uv{attributes.uv.x * material.uvScale.x, attributes.uv.y * material.uvScale.y};
                    // Analytic derivatives of the perspective-correct UV.
                    const float q = l0 * tri.invW[0] + l1 * tri.invW[1] + l2 * tri.invW[2];
                    float lod = 0.0f;
                    if (q > 0.0f) {
                        const float u = attributes.uv.x, v = attributes.uv.y;
                        const float sx = material.uvScale.x * static_cast<float>(material.baseTexture->Width());
                        const float sy = material.uvScale.y * static_cast<float>(material.baseTexture->Height());
                        const float dudx = (tri.uOverWdx - u * tri.invWdx) / q * sx;
                        const float dvdx = (tri.vOverWdx - v * tri.invWdx) / q * sy;
                        const float dudy = (tri.uOverWdy - u * tri.invWdy) / q * sx;
                        const float dvdy = (tri.vOverWdy - v * tri.invWdy) / q * sy;
                        const float rho = std::max(dudx * dudx + dvdx * dvdx, dudy * dudy + dvdy * dvdy);
                        lod = rho > 1.0f ? 0.5f * std::log2(rho) : 0.0f;
                    }
                    const Vec4 texel = material.baseTexture->Sample(uv, lod);
                    albedo = Multiply(albedo, XYZ(texel));
                }

                const Color ambient = Lerp(scene.ambientGround, scene.ambientSky, n.y * 0.5f + 0.5f);
                const float shadow = (shadowsOn && material.receiveShadows) ? ShadowFactor(world, n) : 1.0f;
                Color radiance{};
                std::uint8_t flags = material.outline ? kPixelOutline : 0;
                float reflect = 0.0f;
                switch (material.shading) {
                case ShadingModel::Unlit:
                    radiance = albedo;
                    break;
                case ShadingModel::Lit: {
                    radiance = Multiply(albedo, ambient)
                        + ShadePbr(albedo, material.roughness, material.metallic, n, toEye, toLight, sunRadiance * shadow);
                    for (const PointLight& light : scene.pointLights) {
                        const Vec3 d = light.position - world;
                        const float distance = Length(d);
                        if (distance >= light.radius || distance < 1.0e-4f) continue;
                        const float falloff = 1.0f - (distance / light.radius) * (distance / light.radius);
                        radiance += ShadePbr(albedo, material.roughness, material.metallic, n, toEye, d / distance,
                            light.color * (light.intensity * falloff * falloff));
                    }
                    break;
                }
                case ShadingModel::Water:
                case ShadingModel::Toon: {
                    Vec3 shadingNormal = n;
                    if (material.shading == ShadingModel::Water) {
                        shadingNormal = WaterNormal(world, scene.time, material.waveAmplitude, material.waveScale);
                        flags = static_cast<std::uint8_t>((flags & ~kPixelOutline) | kPixelWater);
                        reflect = material.reflectivity;
                    }
                    const float nl = Dot(shadingNormal, toLight);
                    const float band = SmoothStep(material.shadeThreshold - material.shadeSoftness,
                        material.shadeThreshold + material.shadeSoftness, nl);
                    const float shadowBand = SmoothStep(0.3f, 0.7f, shadow);
                    const float light = band * shadowBand;
                    const Color shade = Multiply(Multiply(albedo, material.shadeColor), ambient * 1.7f);
                    const Color lit = Multiply(albedo, sunRadiance * 0.55f + ambient);
                    radiance = Lerp(shade, lit, light);
                    for (const PointLight& pl : scene.pointLights) {
                        const Vec3 d = pl.position - world;
                        const float distance = Length(d);
                        if (distance >= pl.radius || distance < 1.0e-4f) continue;
                        const float falloff = 1.0f - (distance / pl.radius) * (distance / pl.radius);
                        const float plBand = SmoothStep(-0.05f, 0.1f, Dot(shadingNormal, d / distance));
                        radiance += Multiply(albedo, pl.color) * (pl.intensity * falloff * falloff * plBand);
                    }
                    const Vec3 h = Normalize(toLight + toEye, shadingNormal);
                    if (material.specularIntensity > 0.0f) {
                        const float spec = SmoothStep(material.specularThreshold - 0.005f, material.specularThreshold + 0.005f,
                            Dot(shadingNormal, h));
                        radiance += sunRadiance * (spec * material.specularIntensity * light);
                    }
                    if (material.rimIntensity > 0.0f) {
                        const float facing = 1.0f - Saturate(Dot(n, toEye));
                        const float rim = SmoothStep(1.0f - material.rimWidth, 1.0f - material.rimWidth + 0.04f, facing);
                        radiance += material.rimColor * (rim * material.rimIntensity * (0.3f + 0.7f * light));
                    }
                    if (material.shading == ShadingModel::Water) n = shadingNormal;
                    break;
                }
                }
                radiance += material.emissive + draw.emissiveBoost;

                const float viewZ = Dot(world - view.eye, forward_);
                if (scene.fog.enabled) {
                    const float distance = Length(world - view.eye);
                    const float heightTerm = std::exp(-scene.fog.heightFalloff * std::max(0.0f, world.y));
                    const float fog = std::min(scene.fog.maxOpacity,
                        1.0f - std::exp(-scene.fog.density * distance * heightTerm));
                    radiance = Lerp(radiance, scene.fog.color, fog);
                }
                target.hdr[p] = radiance;
                target.normal[p] = n;
                target.linearDepth[p] = viewZ;
                target.objectId[p] = draw.objectId != 0 ? draw.objectId : tri.drawIndex + 1u;
                target.flags[p] = flags;
                target.reflectivity[p] = reflect;
            }
        }
    });
    for (std::size_t count : shadedCounts) stats_.pixelsShaded += count;
}

void SceneRenderer::Reflections(const RenderScene& scene, const RenderView& view, RenderTarget& target) {
    ASTRAL_PROFILE_SCOPE("Render.Reflections");
    const int width = target.Width(), height = target.Height();
    const Mat4 viewProjection = view.ViewProjection();
    reflectionColor_.assign(target.PixelCount(), {});
    reflectionWeight_.assign(target.PixelCount(), 0.0f);
    std::vector<std::size_t> counts(static_cast<std::size_t>(height), 0);
    ParallelFor(static_cast<std::size_t>(height), 4, [&](std::size_t rowBegin, std::size_t rowEnd) {
        for (std::size_t row = rowBegin; row < rowEnd; ++row) {
            const int y = static_cast<int>(row);
            const float ndcY = 1.0f - (static_cast<float>(y) + 0.5f) / static_cast<float>(height) * 2.0f;
            for (int x = 0; x < width; ++x) {
                const std::size_t p = target.Index(x, y);
                if (!(target.flags[p] & kPixelWater)) continue;
                ++counts[row];
                const float ndcX = (static_cast<float>(x) + 0.5f) / static_cast<float>(width) * 2.0f - 1.0f;
                const Vec3 rayDir = Normalize(forward_ + right_ * (ndcX * tanHalfX_) + up_ * (ndcY * tanHalfY_), forward_);
                const float along = Dot(rayDir, forward_);
                const Vec3 world = view.eye + rayDir * (target.linearDepth[p] / std::max(along, 1.0e-4f));
                const Vec3 n = target.normal[p];
                const Vec3 r = Reflect(rayDir, n);
                const float cosine = Saturate(Dot(-rayDir, n));
                const float f0 = target.reflectivity[p];
                const float fresnel = f0 + (1.0f - f0) * std::pow(1.0f - cosine, 5.0f);
                Color reflection = SkyColor(scene, r.y < 0.02f ? Normalize(Vec3{r.x, 0.02f, r.z}) : r);
                const int steps = std::max(4, settings_.reflectionSteps);
                float t = 0.25f;
                float previous = 0.0f;
                const float growth = std::pow(settings_.reflectionMaxDistance / t, 1.0f / static_cast<float>(steps));
                for (int step = 0; step < steps; ++step) {
                    const Vec3 sample = world + r * t;
                    const Vec4 clip = viewProjection * Vec4{sample.x, sample.y, sample.z, 1.0f};
                    if (clip.w <= view.nearPlane) break;
                    const float u = clip.x / clip.w * 0.5f + 0.5f;
                    const float v = 0.5f - clip.y / clip.w * 0.5f;
                    if (u < 0.0f || u >= 1.0f || v < 0.0f || v >= 1.0f) break;
                    const int sx = static_cast<int>(u * static_cast<float>(width));
                    const int sy = static_cast<int>(v * static_cast<float>(height));
                    const std::size_t q = target.Index(sx, sy);
                    const float sceneZ = target.linearDepth[q];
                    const float sampleZ = clip.w;
                    const float thickness = std::max(0.6f, t * 0.12f);
                    if (!(target.flags[q] & (kPixelSky | kPixelWater)) && sampleZ > sceneZ && sampleZ - sceneZ < thickness) {
                        // Refine the intersection with a short binary search.
                        float lo = previous, hi = t;
                        std::size_t hit = q;
                        for (int i = 0; i < 5; ++i) {
                            const float mid = 0.5f * (lo + hi);
                            const Vec3 m = world + r * mid;
                            const Vec4 mc = viewProjection * Vec4{m.x, m.y, m.z, 1.0f};
                            const int mx = std::clamp(static_cast<int>((mc.x / mc.w * 0.5f + 0.5f) * static_cast<float>(width)), 0, width - 1);
                            const int my = std::clamp(static_cast<int>((0.5f - mc.y / mc.w * 0.5f) * static_cast<float>(height)), 0, height - 1);
                            const std::size_t mq = target.Index(mx, my);
                            if (mc.w > target.linearDepth[mq]) {
                                hi = mid;
                                hit = mq;
                            } else {
                                lo = mid;
                            }
                        }
                        const float edge = SmoothStep(0.0f, 0.08f, std::min(std::min(u, 1.0f - u), std::min(v, 1.0f - v)));
                        reflection = Lerp(reflection, target.hdr[hit], edge);
                        break;
                    }
                    previous = t;
                    t *= growth;
                }
                reflectionColor_[p] = reflection;
                reflectionWeight_[p] = Saturate(fresnel);
            }
        }
    });
    for (std::size_t p = 0; p < target.PixelCount(); ++p) {
        if (reflectionWeight_[p] > 0.0f) target.hdr[p] = Lerp(target.hdr[p], reflectionColor_[p], reflectionWeight_[p]);
    }
    for (std::size_t count : counts) stats_.reflectionPixels += count;
}

void SceneRenderer::Translucency(const RenderScene& scene, const RenderView& view, RenderTarget& target) {
    ASTRAL_PROFILE_SCOPE("Render.Translucency");
    translucentItems_.clear();
    const Frustum frustum = Frustum::FromViewProjection(view.ViewProjection());
    for (const DrawItem& draw : scene.draws) {
        if (!draw.mesh || !draw.material || draw.material->blend == BlendMode::Opaque) continue;
        const AABB bounds = TransformAABB(draw.world, draw.mesh->bounds);
        if (!frustum.Intersects(bounds)) continue;
        translucentItems_.push_back({&draw, nullptr, Dot(bounds.Center() - view.eye, forward_)});
    }
    for (const Billboard& billboard : scene.billboards) {
        if (!frustum.Intersects(Sphere{billboard.position, billboard.size * std::max(1.0f, billboard.stretch)})) continue;
        translucentItems_.push_back({nullptr, &billboard, Dot(billboard.position - view.eye, forward_)});
    }
    if (translucentItems_.empty()) return;
    std::stable_sort(translucentItems_.begin(), translucentItems_.end(),
        [](const TranslucentItem& a, const TranslucentItem& b) { return a.sortDepth > b.sortDepth; });

    const Mat4 viewProjection = view.ViewProjection();
    const int width = target.Width(), height = target.Height();
    translucentTriangles_.clear();
    std::vector<ClipVertex> transformed;
    for (std::uint32_t itemIndex = 0; itemIndex < translucentItems_.size(); ++itemIndex) {
        const TranslucentItem& item = translucentItems_[itemIndex];
        if (item.draw) {
            const DrawItem& draw = *item.draw;
            Mat4 inverseWorld{};
            const Mat4 normalMatrix = Inverse(draw.world, inverseWorld) ? Transpose(inverseWorld) : Mat4::Identity();
            const Mat4 clipFromLocal = viewProjection * draw.world;
            transformed.resize(draw.mesh->vertices.size());
            for (std::size_t v = 0; v < draw.mesh->vertices.size(); ++v) {
                const Vertex& s = draw.mesh->vertices[v];
                transformed[v] = {clipFromLocal * Vec4{s.position.x, s.position.y, s.position.z, 1.0f},
                    TransformPoint(draw.world, s.position), TransformVector(normalMatrix, s.normal), s.uv, s.color};
            }
            const CullMode cull = draw.material->doubleSided ? CullMode::None : CullMode::Back;
            for (std::size_t i = 0; i + 2 < draw.mesh->indices.size(); i += 3) {
                const std::uint32_t a = draw.mesh->indices[i], b = draw.mesh->indices[i + 1], c = draw.mesh->indices[i + 2];
                if (a >= transformed.size() || b >= transformed.size() || c >= transformed.size()) continue;
                ClipAndSetupTriangle(transformed[a], transformed[b], transformed[c], cull, width, height, itemIndex,
                    translucentTriangles_);
            }
        } else {
            const Billboard& b = *item.billboard;
            Vec3 axisX = right_, axisY = up_;
            const float speed = Length(b.velocity);
            if (b.stretch > 1.0f && speed > 1.0e-3f) {
                // Align the long axis with the on-screen velocity.
                const Vec3 dir = b.velocity / speed;
                axisY = Normalize(dir - forward_ * Dot(dir, forward_), up_);
                axisX = Normalize(Cross(forward_, axisY), right_);
            } else if (b.rotation != 0.0f) {
                const float c = std::cos(b.rotation), s = std::sin(b.rotation);
                axisX = right_ * c + up_ * s;
                axisY = up_ * c - right_ * s;
            }
            const Vec3 hx = axisX * b.size;
            const Vec3 hy = axisY * (b.size * std::max(1.0f, b.stretch));
            const Vec3 corners[4] = {b.position - hx - hy, b.position - hx + hy, b.position + hx + hy, b.position + hx - hy};
            const Vec2 uvs[4] = {{0, 1}, {0, 0}, {1, 0}, {1, 1}};
            ClipVertex quad[4];
            for (int i = 0; i < 4; ++i) {
                quad[i] = {viewProjection * Vec4{corners[i].x, corners[i].y, corners[i].z, 1.0f}, corners[i], -forward_, uvs[i], b.color};
            }
            ClipAndSetupTriangle(quad[0], quad[1], quad[2], CullMode::None, width, height, itemIndex, translucentTriangles_);
            ClipAndSetupTriangle(quad[0], quad[2], quad[3], CullMode::None, width, height, itemIndex, translucentTriangles_);
        }
    }
    stats_.transparentTriangles = translucentTriangles_.size();
    TileBins bins;
    bins.Build(translucentTriangles_, width, height, settings_.tileSize);
    ParallelFor(static_cast<std::size_t>(bins.TileCount()), 1, [&](std::size_t begin, std::size_t end) {
        for (std::size_t tile = begin; tile < end; ++tile) {
            const int tx = static_cast<int>(tile) % bins.tilesX;
            const int ty = static_cast<int>(tile) / bins.tilesX;
            const int x0 = tx * bins.tileSize, y0 = ty * bins.tileSize;
            const int x1 = std::min(width, x0 + bins.tileSize), y1 = std::min(height, y0 + bins.tileSize);
            for (std::uint32_t index : bins.bins[tile]) {
                const SetupTriangle& tri = translucentTriangles_[index];
                const TranslucentItem& item = translucentItems_[tri.drawIndex];
                RasterizeTriangle(tri, x0, y0, x1, y1, [&](int x, int y, float z, float l0, float l1, float l2) {
                    const std::size_t p = target.Index(x, y);
                    if (z >= target.depth[p]) return;
                    float b0, b1, b2;
                    PerspectiveCorrect(tri, l0, l1, l2, b0, b1, b2);
                    const ClipVertex a = InterpolateVertex(tri, b0, b1, b2);
                    const float fragmentZ = Dot(a.world - view.eye, forward_);
                    const float soft = Saturate((target.linearDepth[p] - fragmentZ) / 0.5f);
                    Color rgb;
                    float alpha;
                    BlendMode blend;
                    if (item.billboard) {
                        const Billboard& b = *item.billboard;
                        blend = b.blend;
                        rgb = XYZ(a.color);
                        alpha = a.color.w * soft;
                        if (b.texture) {
                            const Vec4 texel = b.texture->Sample(a.uv, 0.0f);
                            rgb = Multiply(rgb, XYZ(texel));
                            alpha *= texel.w;
                        } else {
                            const float du = a.uv.x - 0.5f, dv = a.uv.y - 0.5f;
                            const float disc = Saturate(1.0f - (du * du + dv * dv) * 4.0f);
                            alpha *= disc * disc;
                        }
                    } else {
                        const DrawItem& draw = *item.draw;
                        const Material& m = *draw.material;
                        blend = m.blend;
                        rgb = Multiply(m.baseColor, draw.tint);
                        alpha = m.opacity * draw.opacity * soft;
                        if (m.vertexColor) {
                            rgb = Multiply(rgb, XYZ(a.color));
                            alpha *= a.color.w;
                        }
                        if (m.baseTexture) {
                            const Vec4 texel = m.baseTexture->Sample({a.uv.x * m.uvScale.x, a.uv.y * m.uvScale.y}, 0.0f);
                            rgb = Multiply(rgb, XYZ(texel));
                            alpha *= texel.w;
                        }
                        rgb += m.emissive + draw.emissiveBoost;
                        if (m.rimIntensity > 0.0f) {
                            const Vec3 n = SafeNormal(a.normal);
                            const float facing = 1.0f - std::fabs(Dot(n, Normalize(view.eye - a.world, {0, 0, -1})));
                            const float rim = std::pow(facing, 3.0f) * m.rimIntensity;
                            rgb += m.rimColor * rim;
                            alpha = Saturate(alpha + rim * 0.5f * m.opacity * draw.opacity);
                        }
                    }
                    alpha = Saturate(alpha);
                    if (blend == BlendMode::Additive) target.hdr[p] += rgb * alpha;
                    else target.hdr[p] = Lerp(target.hdr[p], rgb, alpha);
                });
            }
        }
    });
}

void SceneRenderer::DrawDebugLines(const RenderScene& scene, const RenderView& view, RenderTarget& target) {
    if (scene.lines.empty()) return;
    const Mat4 viewProjection = view.ViewProjection();
    const int width = target.Width(), height = target.Height();
    for (const DebugLine& line : scene.lines) {
        Vec4 a = viewProjection * Vec4{line.start.x, line.start.y, line.start.z, 1.0f};
        Vec4 b = viewProjection * Vec4{line.end.x, line.end.y, line.end.z, 1.0f};
        if (!ClipSegment(a, b)) continue;
        const float ax = (a.x / a.w * 0.5f + 0.5f) * static_cast<float>(width);
        const float ay = (0.5f - a.y / a.w * 0.5f) * static_cast<float>(height);
        const float bx = (b.x / b.w * 0.5f + 0.5f) * static_cast<float>(width);
        const float by = (0.5f - b.y / b.w * 0.5f) * static_cast<float>(height);
        if (!std::isfinite(ax) || !std::isfinite(ay) || !std::isfinite(bx) || !std::isfinite(by)) continue;
        const float span = std::max(std::fabs(bx - ax), std::fabs(by - ay));
        const int steps = std::min(8192, std::max(1, static_cast<int>(std::ceil(span))));
        for (int i = 0; i <= steps; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(steps);
            const int x = static_cast<int>(std::floor(ax + (bx - ax) * t));
            const int y = static_cast<int>(std::floor(ay + (by - ay) * t));
            if (x < 0 || y < 0 || x >= width || y >= height) continue;
            if (line.depthTest) {
                // NDC depth is affine in screen space, so a plain lerp is exact.
                const float z = Lerp(a.z / a.w, b.z / b.w, t);
                if (z > target.depth[target.Index(x, y)] + 1.0e-4f) continue;
            }
            target.output.Set(x, y, line.color);
        }
    }
}

void SceneRenderer::Render(const RenderScene& scene, const RenderView& view, RenderTarget& target) {
    ASTRAL_PROFILE_SCOPE("Render.Frame");
    const auto frameStart = SteadyClock::now();
    stats_ = {};
    target.Resize(view.width, view.height);
    const int width = target.Width(), height = target.Height();
    right_ = Normalize(Vec3{view.view.m[0][0], view.view.m[0][1], view.view.m[0][2]}, {1, 0, 0});
    up_ = Normalize(Vec3{view.view.m[1][0], view.view.m[1][1], view.view.m[1][2]}, {0, 1, 0});
    forward_ = Normalize(Vec3{view.view.m[2][0], view.view.m[2][1], view.view.m[2][2]}, {0, 0, 1});
    tanHalfX_ = view.projection.m[0][0] != 0.0f ? 1.0f / view.projection.m[0][0] : 1.0f;
    tanHalfY_ = view.projection.m[1][1] != 0.0f ? 1.0f / view.projection.m[1][1] : 1.0f;

    auto phase = SteadyClock::now();
    if (scene.shadows.enabled && scene.sun.castShadows) RenderShadowMap(scene);
    else shadow_.valid = false;
    stats_.shadowMs = MillisecondsSince(phase);

    phase = SteadyClock::now();
    const Mat4 viewProjection = view.ViewProjection();
    const Frustum frustum = Frustum::FromViewProjection(viewProjection);
    drawList_.clear();
    stats_.drawsSubmitted = scene.draws.size();
    for (std::uint32_t i = 0; i < scene.draws.size(); ++i) {
        const DrawItem& draw = scene.draws[i];
        if (!draw.mesh || draw.mesh->indices.empty()) continue;
        if (draw.material && draw.material->blend != BlendMode::Opaque) continue;
        if (!frustum.Intersects(TransformAABB(draw.world, draw.mesh->bounds))) continue;
        drawList_.push_back(i);
        stats_.trianglesSubmitted += draw.mesh->indices.size() / 3;
    }
    stats_.drawsVisible = drawList_.size();
    {
        ASTRAL_PROFILE_SCOPE("Render.Geometry");
        BuildTriangles(scene, drawList_, viewProjection, width, height, false, triangles_);
    }
    stats_.trianglesRasterized = triangles_.size();
    stats_.geometryMs = MillisecondsSince(phase);

    phase = SteadyClock::now();
    RasterizeVisibility(target);
    stats_.rasterMs = MillisecondsSince(phase);

    phase = SteadyClock::now();
    Shade(scene, view, target);
    stats_.shadeMs = MillisecondsSince(phase);

    phase = SteadyClock::now();
    Reflections(scene, view, target);
    stats_.reflectionMs = MillisecondsSince(phase);

    phase = SteadyClock::now();
    Translucency(scene, view, target);
    stats_.translucencyMs = MillisecondsSince(phase);

    phase = SteadyClock::now();
    post_.Run(target, scene.post, jobs_);
    DrawDebugLines(scene, view, target);
    stats_.postMs = MillisecondsSince(phase);

    stats_.totalMs = MillisecondsSince(frameStart);
    stats_.memoryBytes = target.MemoryBytes() + shadow_.depth.size() * sizeof(float)
        + triangles_.capacity() * sizeof(SetupTriangle) + translucentTriangles_.capacity() * sizeof(SetupTriangle);
}

} // namespace Astral::Graphics
