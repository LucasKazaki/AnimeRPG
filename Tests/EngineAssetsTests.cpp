// Asset pipeline: glTF 2.0 import/export, JPEG and WAV decoding, and the asset
// manager (caching, async loads, hot reload, collection).

#include "Engine/Animation/Skinning.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/Gltf.h"
#include "Engine/Audio/AudioMixer.h"
#include "Engine/Core/Base64.h"
#include "Engine/Core/JobSystem.h"
#include "Engine/Core/Json.h"
#include "Engine/Core/Random.h"
#include "Engine/Graphics/SceneRenderer.h"
#include "Engine/Graphics/Texture.h"
#include "Tests/EngineTestSupport.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace Astral;
using namespace Astral::Assets;
using Math::Mat4;
using Math::Quat;
using Math::Vec3;
using Math::Vec4;

namespace {

std::string TempDir(const char* name) {
    const std::filesystem::path path = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove_all(path);
    std::filesystem::create_directories(path);
    return path.string();
}

void WriteBytes(const std::string& path, const std::vector<std::uint8_t>& bytes) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

bool Near(Vec3 a, Vec3 b, float tolerance = 1e-5f) { return Math::Length(a - b) <= tolerance; }
bool NearQuat(Quat a, Quat b, float tolerance = 1e-5f) {
    // q and -q are the same rotation.
    return std::fabs(std::fabs(Math::Dot(a, b)) - 1.0f) <= tolerance;
}

// ---------------------------------------------------------------- Test-only baseline JPEG encoder
// Annex K tables; enough to produce standard files for the decoder to read.

const std::uint8_t kLumaQuant[64] = {16, 11, 10, 16, 24, 40, 51, 61, 12, 12, 14, 19, 26, 58, 60, 55, 14, 13, 16, 24, 40,
    57, 69, 56, 14, 17, 22, 29, 51, 87, 80, 62, 18, 22, 37, 56, 68, 109, 103, 77, 24, 35, 55, 64, 81, 104, 113, 92, 49,
    64, 78, 87, 103, 121, 120, 101, 72, 92, 95, 98, 112, 100, 103, 99};
const std::uint8_t kChromaQuant[64] = {17, 18, 24, 47, 99, 99, 99, 99, 18, 21, 26, 66, 99, 99, 99, 99, 24, 26, 56, 99,
    99, 99, 99, 99, 47, 66, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99};
const std::uint8_t kDcLumaBits[16] = {0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0};
const std::uint8_t kDcChromaBits[16] = {0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0};
const std::uint8_t kDcValues[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
const std::uint8_t kAcLumaBits[16] = {0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d};
const std::uint8_t kAcLumaValues[162] = {0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13,
    0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08, 0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
    0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x34,
    0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57,
    0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a,
    0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3,
    0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4,
    0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2, 0xe3, 0xe4,
    0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa};
const std::uint8_t kAcChromaBits[16] = {0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77};
const std::uint8_t kAcChromaValues[162] = {0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51,
    0x07, 0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
    0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26, 0x27, 0x28, 0x29,
    0x2a, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56,
    0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
    0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a,
    0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2,
    0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe2, 0xe3,
    0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa};
const int kZigZag[64] = {0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5, 12, 19, 26, 33, 40, 48, 41, 34, 27, 20,
    13, 6, 7, 14, 21, 28, 35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63};

struct HuffEncode {
    int code[256]{};
    int length[256]{};
    HuffEncode(const std::uint8_t bits[16], const std::uint8_t* values) {
        int c = 0, k = 0;
        for (int len = 1; len <= 16; ++len) {
            for (int i = 0; i < bits[len - 1]; ++i) {
                code[values[k]] = c++;
                length[values[k]] = len;
                ++k;
            }
            c <<= 1;
        }
    }
};

struct JpegWriter {
    std::vector<std::uint8_t> out;
    std::uint32_t buffer{};
    int count{};
    void Byte(int b) { out.push_back(static_cast<std::uint8_t>(b)); }
    void Word(int w) {
        Byte(w >> 8);
        Byte(w & 255);
    }
    void Bits(int value, int n) {
        for (int i = n - 1; i >= 0; --i) {
            buffer = (buffer << 1) | static_cast<std::uint32_t>((value >> i) & 1);
            if (++count == 8) {
                Byte(static_cast<int>(buffer & 255));
                if ((buffer & 255) == 0xFF) Byte(0);
                buffer = 0;
                count = 0;
            }
        }
    }
    void Flush() {
        while (count != 0) Bits(1, 1);
    }
};

void EncodeBlock(JpegWriter& w, const float pixels[64], const std::uint8_t* quant, int& predictor, const HuffEncode& dc,
    const HuffEncode& ac) {
    int coefficients[64];
    for (int v = 0; v < 8; ++v) {
        for (int u = 0; u < 8; ++u) {
            float sum = 0;
            for (int y = 0; y < 8; ++y) {
                for (int x = 0; x < 8; ++x) {
                    sum += (pixels[y * 8 + x] - 128.0f) * std::cos((2 * x + 1) * u * 3.14159265f / 16)
                        * std::cos((2 * y + 1) * v * 3.14159265f / 16);
                }
            }
            const float cu = u == 0 ? 1 / std::sqrt(2.0f) : 1.0f, cv = v == 0 ? 1 / std::sqrt(2.0f) : 1.0f;
            coefficients[v * 8 + u] = static_cast<int>(std::lround(0.25f * cu * cv * sum / quant[v * 8 + u]));
        }
    }
    auto category = [](int value) {
        int size = 0;
        for (int a = std::abs(value); a; a >>= 1) ++size;
        return size;
    };
    auto emit = [&](const HuffEncode& table, int symbol) {
        ASTRAL_CHECK(table.length[symbol] > 0);
        w.Bits(table.code[symbol], table.length[symbol]);
    };
    const int diff = coefficients[0] - predictor;
    predictor = coefficients[0];
    const int dcSize = category(diff);
    emit(dc, dcSize);
    if (dcSize) w.Bits(diff < 0 ? diff - 1 : diff, dcSize);
    int run = 0;
    for (int k = 1; k < 64; ++k) {
        const int value = coefficients[kZigZag[k]];
        if (value == 0) {
            ++run;
            continue;
        }
        while (run > 15) {
            emit(ac, 0xF0);
            run -= 16;
        }
        const int size = category(value);
        emit(ac, (run << 4) | size);
        w.Bits(value < 0 ? value - 1 : value, size);
        run = 0;
    }
    if (run > 0) emit(ac, 0x00);
}

// channels: 1 (grey) or 3 (YCbCr); subsample: 1 (4:4:4) or 2 (4:2:0).
std::vector<std::uint8_t> EncodeJpeg(const Graphics::ImageRgba8& image, int channels, int subsample, int restartInterval) {
    JpegWriter w;
    w.Word(0xFFD8);
    auto quantTable = [&](int id, const std::uint8_t* table) {
        w.Word(0xFFDB);
        w.Word(67);
        w.Byte(id);
        for (int k = 0; k < 64; ++k) w.Byte(table[kZigZag[k]]);
    };
    quantTable(0, kLumaQuant);
    if (channels == 3) quantTable(1, kChromaQuant);
    w.Word(0xFFC0);
    w.Word(8 + 3 * channels);
    w.Byte(8);
    w.Word(image.height);
    w.Word(image.width);
    w.Byte(channels);
    for (int c = 0; c < channels; ++c) {
        w.Byte(c + 1);
        w.Byte(c == 0 ? (subsample << 4) | subsample : 0x11);
        w.Byte(c == 0 ? 0 : 1);
    }
    auto huffman = [&](int classId, const std::uint8_t* bits, const std::uint8_t* values, int count) {
        w.Word(0xFFC4);
        w.Word(3 + 16 + count);
        w.Byte(classId);
        for (int i = 0; i < 16; ++i) w.Byte(bits[i]);
        for (int i = 0; i < count; ++i) w.Byte(values[i]);
    };
    huffman(0x00, kDcLumaBits, kDcValues, 12);
    huffman(0x10, kAcLumaBits, kAcLumaValues, 162);
    if (channels == 3) {
        huffman(0x01, kDcChromaBits, kDcValues, 12);
        huffman(0x11, kAcChromaBits, kAcChromaValues, 162);
    }
    if (restartInterval) {
        w.Word(0xFFDD);
        w.Word(4);
        w.Word(restartInterval);
    }
    w.Word(0xFFDA);
    w.Word(6 + 2 * channels);
    w.Byte(channels);
    for (int c = 0; c < channels; ++c) {
        w.Byte(c + 1);
        w.Byte(c == 0 ? 0x00 : 0x11);
    }
    w.Byte(0);
    w.Byte(63);
    w.Byte(0);
    const HuffEncode dcL(kDcLumaBits, kDcValues), acL(kAcLumaBits, kAcLumaValues);
    const HuffEncode dcC(kDcChromaBits, kDcValues), acC(kAcChromaBits, kAcChromaValues);
    auto channel = [&](int x, int y, int c) {
        x = std::min(x, image.width - 1);
        y = std::min(y, image.height - 1);
        const Graphics::Rgba8 p = image.Get(x, y);
        const float r = p.r, g = p.g, b = p.b;
        if (c == 0) return 0.299f * r + 0.587f * g + 0.114f * b;
        if (c == 1) return -0.168736f * r - 0.331264f * g + 0.5f * b + 128.0f;
        return 0.5f * r - 0.418688f * g - 0.081312f * b + 128.0f;
    };
    const int mcu = 8 * subsample;
    const int mcusX = (image.width + mcu - 1) / mcu, mcusY = (image.height + mcu - 1) / mcu;
    int predictors[3] = {0, 0, 0};
    int mcuCount = 0, restartIndex = 0;
    float block[64];
    for (int my = 0; my < mcusY; ++my) {
        for (int mx = 0; mx < mcusX; ++mx) {
            if (restartInterval && mcuCount > 0 && mcuCount % restartInterval == 0) {
                w.Flush();
                w.Word(0xFFD0 + (restartIndex++ & 7));
                predictors[0] = predictors[1] = predictors[2] = 0;
            }
            ++mcuCount;
            for (int by = 0; by < subsample; ++by) {
                for (int bx = 0; bx < subsample; ++bx) {
                    for (int y = 0; y < 8; ++y) {
                        for (int x = 0; x < 8; ++x) block[y * 8 + x] = channel(mx * mcu + bx * 8 + x, my * mcu + by * 8 + y, 0);
                    }
                    EncodeBlock(w, block, kLumaQuant, predictors[0], dcL, acL);
                }
            }
            for (int c = 1; c < channels; ++c) {
                for (int y = 0; y < 8; ++y) {
                    for (int x = 0; x < 8; ++x) {
                        float sum = 0;
                        for (int sy = 0; sy < subsample; ++sy) {
                            for (int sx = 0; sx < subsample; ++sx) {
                                sum += channel(mx * mcu + x * subsample + sx, my * mcu + y * subsample + sy, c);
                            }
                        }
                        block[y * 8 + x] = sum / static_cast<float>(subsample * subsample);
                    }
                }
                EncodeBlock(w, block, kChromaQuant, predictors[c], dcC, acC);
            }
        }
    }
    w.Flush();
    w.Word(0xFFD9);
    return w.out;
}

Graphics::ImageRgba8 TestPattern(int width, int height) {
    Graphics::ImageRgba8 image;
    image.Resize(width, height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            image.Set(x, y, {static_cast<std::uint8_t>(40 + x * 180 / width), static_cast<std::uint8_t>(60 + y * 150 / height),
                                static_cast<std::uint8_t>(120 + 60 * std::sin(x * 0.2f + y * 0.1f)), 255});
        }
    }
    return image;
}

double MeanAbsoluteError(const Graphics::ImageRgba8& a, const Graphics::ImageRgba8& b) {
    double sum = 0;
    for (std::size_t i = 0; i < a.pixels.size(); ++i) {
        if (i % 4 != 3) sum += std::abs(static_cast<int>(a.pixels[i]) - static_cast<int>(b.pixels[i]));
    }
    return sum / static_cast<double>(a.pixels.size() / 4 * 3);
}

// ---------------------------------------------------------------- glTF fixture

GltfDocument MakeFixture() {
    GltfDocument doc;
    doc.generator = "Astral test";
    GltfImage image;
    image.name = "checker";
    image.image.Resize(4, 4);
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            image.image.Set(x, y, {static_cast<std::uint8_t>(x * 60), static_cast<std::uint8_t>(y * 60), 200,
                                      static_cast<std::uint8_t>(255 - x * 30)});
        }
    }
    doc.images.push_back(image);
    doc.textures.push_back({"checkerTex", 0, true});
    GltfMaterial material;
    material.name = "Coat";
    material.baseColorFactor = {0.8f, 0.6f, 0.4f, 0.5f};
    material.baseColorTexture = 0;
    material.metallicFactor = 0.25f;
    material.roughnessFactor = 0.75f;
    material.emissiveFactor = {2.0f, 1.0f, 0.5f};
    material.alphaMode = GltfAlphaMode::Blend;
    material.doubleSided = true;
    doc.materials.push_back(material);
    GltfMaterial unlit;
    unlit.name = "Glow";
    unlit.unlit = true;
    doc.materials.push_back(unlit);

    // Nodes: 0 Root (armature) -> 1 Hips (joint) -> 2 Spine (joint); 3 Body (skinned mesh) under Root.
    doc.nodes.resize(4);
    doc.nodes[0].name = "Root";
    doc.nodes[0].local.translation = {1.0f, 0.0f, 0.5f};
    doc.nodes[0].local.rotation = Math::QuatFromYaw(Math::Radians(30.0f));
    doc.nodes[0].children = {1, 3};
    doc.nodes[1].name = "Hips";
    doc.nodes[1].parent = 0;
    doc.nodes[1].local.translation = {0.0f, 1.0f, 0.0f};
    doc.nodes[1].children = {2};
    doc.nodes[2].name = "Spine";
    doc.nodes[2].parent = 1;
    doc.nodes[2].local.translation = {0.0f, 0.5f, 0.0f};
    doc.nodes[2].local.scale = {1.0f, 1.0f, 1.0f};
    doc.nodes[3].name = "Body";
    doc.nodes[3].parent = 0;
    doc.nodes[3].mesh = 0;
    doc.nodes[3].skin = 0;
    doc.scenes.push_back({"Main", {0}});
    doc.scene = 0;

    GltfMesh mesh;
    mesh.name = "BodyMesh";
    GltfPrimitive primitive;
    Graphics::MeshBuilder builder(primitive.mesh);
    builder.SetColor(Math::Vec4{0.9f, 0.8f, 0.7f, 1.0f});
    builder.AddBox({1.0f, 1.5f, 0.5f}, {0.2f, 0.5f, 0.2f});
    primitive.mesh.skin.resize(primitive.mesh.vertices.size());
    for (std::size_t v = 0; v < primitive.mesh.vertices.size(); ++v) {
        const bool upper = primitive.mesh.vertices[v].position.y > 1.5f;
        primitive.mesh.skin[v].joints = {upper ? std::uint16_t{1} : std::uint16_t{0}, 0, 0, 0};
        primitive.mesh.skin[v].weights = {1.0f, 0.0f, 0.0f, 0.0f};
    }
    primitive.mesh.ComputeBounds();
    primitive.material = 0;
    mesh.primitives.push_back(primitive);
    doc.meshes.push_back(mesh);

    GltfSkin skin;
    skin.name = "Rig";
    skin.joints = {1, 2};
    for (int joint : skin.joints) {
        Mat4 inverse;
        Math::Inverse(doc.WorldMatrix(joint), inverse);
        skin.inverseBind.push_back(inverse);
    }
    doc.skins.push_back(skin);

    GltfAnimation animation;
    animation.name = "Wave";
    GltfChannel rotation;
    rotation.node = 2;
    rotation.path = GltfPath::Rotation;
    rotation.times = {0.0f, 0.5f, 1.0f};
    for (float degrees : {0.0f, 45.0f, 0.0f}) {
        const Quat q = Math::QuatFromAxisAngle({0, 0, 1}, Math::Radians(degrees));
        rotation.values.push_back({q.x, q.y, q.z, q.w});
    }
    animation.channels.push_back(rotation);
    GltfChannel step;
    step.node = 1;
    step.path = GltfPath::Translation;
    step.interpolation = GltfInterpolation::Step;
    step.times = {0.0f, 0.6f};
    step.values = {{0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 1.2f, 0.0f, 0.0f}};
    animation.channels.push_back(step);
    GltfChannel spline;
    spline.node = 2;
    spline.path = GltfPath::Scale;
    spline.interpolation = GltfInterpolation::CubicSpline;
    spline.times = {0.0f, 1.0f};
    spline.values = {{0, 0, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {2, 2, 2, 0}, {0, 0, 0, 0}};
    animation.channels.push_back(spline);
    doc.animations.push_back(animation);
    return doc;
}

void CheckSameDocument(const GltfDocument& a, const GltfDocument& b) {
    ASTRAL_CHECK(a.nodes.size() == b.nodes.size() && a.meshes.size() == b.meshes.size());
    for (std::size_t i = 0; i < a.nodes.size(); ++i) {
        ASTRAL_CHECK(a.nodes[i].name == b.nodes[i].name && a.nodes[i].parent == b.nodes[i].parent);
        ASTRAL_CHECK(a.nodes[i].children == b.nodes[i].children && a.nodes[i].mesh == b.nodes[i].mesh);
        ASTRAL_CHECK(Near(a.nodes[i].local.translation, b.nodes[i].local.translation));
        ASTRAL_CHECK(NearQuat(a.nodes[i].local.rotation, b.nodes[i].local.rotation));
    }
    const Graphics::MeshData& ma = a.meshes[0].primitives[0].mesh;
    const Graphics::MeshData& mb = b.meshes[0].primitives[0].mesh;
    ASTRAL_CHECK(ma.vertices.size() == mb.vertices.size() && ma.indices == mb.indices);
    for (std::size_t v = 0; v < ma.vertices.size(); ++v) {
        ASTRAL_CHECK(Near(ma.vertices[v].position, mb.vertices[v].position, 0.0f));
        ASTRAL_CHECK(Near(ma.vertices[v].normal, mb.vertices[v].normal));
        ASTRAL_CHECK(ma.vertices[v].uv.x == mb.vertices[v].uv.x && ma.vertices[v].color.x == mb.vertices[v].color.x);
        ASTRAL_CHECK(ma.skin[v].joints == mb.skin[v].joints && ma.skin[v].weights == mb.skin[v].weights);
    }
    ASTRAL_CHECK(b.materials.size() == 2 && b.materials[0].name == "Coat" && b.materials[1].unlit);
    const GltfMaterial& m = b.materials[0];
    ASTRAL_CHECK(m.alphaMode == GltfAlphaMode::Blend && m.doubleSided && m.baseColorTexture == 0);
    ASTRAL_CHECK(std::fabs(m.baseColorFactor.w - 0.5f) < 1e-6f && std::fabs(m.metallicFactor - 0.25f) < 1e-6f);
    ASTRAL_CHECK(Near(m.emissiveFactor, {2.0f, 1.0f, 0.5f})); // HDR survives via KHR_materials_emissive_strength
    ASTRAL_CHECK(b.textures.size() == 1 && b.textures[0].clamp && b.textures[0].image == 0);
    ASTRAL_CHECK(b.images[0].image.pixels == a.images[0].image.pixels); // PNG with alpha is lossless
    ASTRAL_CHECK(b.skins.size() == 1 && b.skins[0].joints == a.skins[0].joints);
    for (std::size_t j = 0; j < a.skins[0].inverseBind.size(); ++j) {
        ASTRAL_CHECK(Math::NearlyEqual(a.skins[0].inverseBind[j], b.skins[0].inverseBind[j], 1e-5f));
    }
    ASTRAL_CHECK(b.animations.size() == 1 && b.animations[0].channels.size() == 3);
    for (std::size_t c = 0; c < 3; ++c) {
        const GltfChannel& ca = a.animations[0].channels[c];
        const GltfChannel& cb = b.animations[0].channels[c];
        ASTRAL_CHECK(ca.node == cb.node && ca.path == cb.path && ca.interpolation == cb.interpolation);
        ASTRAL_CHECK(ca.times == cb.times && ca.values.size() == cb.values.size());
        for (std::size_t k = 0; k < ca.values.size(); ++k) {
            ASTRAL_CHECK(std::fabs(ca.values[k].x - cb.values[k].x) < 1e-6f && std::fabs(ca.values[k].w - cb.values[k].w) < 1e-6f);
        }
    }
}

} // namespace

// ---------------------------------------------------------------- Math helpers

ASTRAL_TEST(QuatFromMatrixAndDecomposeInvertComposition) {
    Core::Random rng(9);
    for (int i = 0; i < 200; ++i) {
        const Vec3 axis = Math::Normalize(Vec3{rng.Range(-1, 1), rng.Range(-1, 1), rng.Range(-1, 1)}, {0, 1, 0});
        const Quat q = Math::QuatFromAxisAngle(axis, rng.Range(-3.1f, 3.1f));
        ASTRAL_CHECK(NearQuat(Math::QuatFromMatrix(Math::ToMat4(q)), q, 1e-4f));
        Math::TRS trs{{rng.Range(-5, 5), rng.Range(-5, 5), rng.Range(-5, 5)}, q,
            {rng.Range(0.2f, 3.0f), rng.Range(0.2f, 3.0f), rng.Range(0.2f, 3.0f)}};
        const Math::TRS back = Math::Decompose(Math::ToMat4(trs));
        ASTRAL_CHECK(Math::NearlyEqual(Math::ToMat4(back), Math::ToMat4(trs), 1e-4f));
    }
}

ASTRAL_TEST(Base64RoundTripsAndRejectsGarbage) {
    for (std::size_t n = 0; n < 20; ++n) {
        std::vector<std::uint8_t> bytes(n);
        for (std::size_t i = 0; i < n; ++i) bytes[i] = static_cast<std::uint8_t>(i * 37 + 11);
        const std::string text = Core::Base64Encode(bytes.data(), bytes.size());
        std::vector<std::uint8_t> decoded;
        ASTRAL_CHECK(Core::Base64Decode(text, decoded) && decoded == bytes);
    }
    std::vector<std::uint8_t> out;
    ASTRAL_CHECK(Core::Base64Decode("SGVs\nbG8=", out) && std::string(out.begin(), out.end()) == "Hello");
    ASTRAL_CHECK(!Core::Base64Decode("SGVs*G8=", out));
    ASTRAL_CHECK(!Core::Base64Decode("SG=Vs", out));
}

// ---------------------------------------------------------------- JPEG and WAV

ASTRAL_TEST(JpegDecodesBaselineVariants) {
    const Graphics::ImageRgba8 source = TestPattern(37, 21); // partial MCUs on both axes
    struct Case {
        int channels, subsample, restart;
        double tolerance;
    };
    for (const Case& c : {Case{3, 1, 0, 6.0}, Case{3, 2, 0, 8.0}, Case{3, 2, 2, 8.0}, Case{3, 1, 1, 6.0}, Case{1, 1, 0, 60.0}}) {
        const std::vector<std::uint8_t> jpeg = EncodeJpeg(source, c.channels, c.subsample, c.restart);
        Graphics::ImageRgba8 decoded;
        std::string error;
        ASTRAL_CHECK(DecodeJpeg(jpeg.data(), jpeg.size(), decoded, error));
        ASTRAL_CHECK(decoded.width == 37 && decoded.height == 21);
        ASTRAL_CHECK(MeanAbsoluteError(source, decoded) < c.tolerance);
        if (c.channels == 1) {
            const Graphics::Rgba8 p = decoded.Get(10, 10);
            ASTRAL_CHECK(p.r == p.g && p.g == p.b);
        }
    }
    // Rejections never crash: progressive, truncated, garbage.
    std::vector<std::uint8_t> jpeg = EncodeJpeg(source, 3, 1, 0);
    Graphics::ImageRgba8 decoded;
    std::string error;
    std::vector<std::uint8_t> progressive = jpeg;
    for (std::size_t i = 0; i + 1 < progressive.size(); ++i) {
        if (progressive[i] == 0xFF && progressive[i + 1] == 0xC0) {
            progressive[i + 1] = 0xC2;
            break;
        }
    }
    ASTRAL_CHECK(!DecodeJpeg(progressive.data(), progressive.size(), decoded, error) && error.find("progressive") != std::string::npos);
    for (std::size_t cut : {std::size_t{3}, std::size_t{40}, jpeg.size() / 2}) {
        std::vector<std::uint8_t> truncated(jpeg.begin(), jpeg.begin() + static_cast<std::ptrdiff_t>(cut));
        DecodeJpeg(truncated.data(), truncated.size(), decoded, error); // result irrelevant; must be memory-safe
    }
    Core::Random rng(4);
    for (int trial = 0; trial < 60; ++trial) {
        std::vector<std::uint8_t> mutated = jpeg;
        for (int k = 0; k < 6; ++k) mutated[rng.NextBounded(static_cast<std::uint32_t>(mutated.size()))] = static_cast<std::uint8_t>(rng.NextU32());
        DecodeJpeg(mutated.data(), mutated.size(), decoded, error);
    }
    const std::uint8_t garbage[] = {0xFF, 0xD8, 0x00, 0x11};
    ASTRAL_CHECK(!DecodeJpeg(garbage, sizeof(garbage), decoded, error));
}

ASTRAL_TEST(WavDecodesPcmFloatAndDownmixes) {
    const std::string dir = TempDir("astral_wav_test");
    std::vector<float> stereo;
    for (int i = 0; i < 480; ++i) {
        stereo.push_back(0.5f * std::sin(i * 0.1f));
        stereo.push_back(-0.25f);
    }
    std::string error;
    ASTRAL_CHECK(Audio::WriteWav(dir + "/tone.wav", stereo, 48000, error));
    Audio::AudioClip clip;
    ASTRAL_CHECK(Audio::ReadWav(dir + "/tone.wav", clip, error));
    ASTRAL_CHECK(clip.sampleRate == 48000 && clip.samples.size() == 480);
    ASTRAL_CHECK_NEAR(clip.samples[20], (0.5f * std::sin(2.0f) - 0.25f) * 0.5f, 1e-3);
    // Hand-built mono 24-bit and float WAVs.
    auto build = [](std::uint16_t format, std::uint16_t bits, const std::vector<std::uint8_t>& payload) {
        std::vector<std::uint8_t> b = {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ', 16, 0, 0, 0};
        auto u16 = [&](std::uint32_t v) {
            b.push_back(static_cast<std::uint8_t>(v));
            b.push_back(static_cast<std::uint8_t>(v >> 8));
        };
        auto u32 = [&](std::uint32_t v) {
            u16(v & 0xFFFF);
            u16(v >> 16);
        };
        u16(format);
        u16(1);
        u32(22050);
        u32(22050u * bits / 8);
        u16(bits / 8);
        u16(bits);
        b.insert(b.end(), {'L', 'I', 'S', 'T', 3, 0, 0, 0, 'x', 'y', 'z', 0}); // odd chunk + pad byte
        b.insert(b.end(), {'d', 'a', 't', 'a'});
        u32(static_cast<std::uint32_t>(payload.size()));
        b.insert(b.end(), payload.begin(), payload.end());
        return b;
    };
    const std::vector<std::uint8_t> pcm24 = build(1, 24, {0x00, 0x00, 0x40, 0x00, 0x00, 0xC0}); // +0.5, -0.5
    ASTRAL_CHECK(Audio::DecodeWav(pcm24.data(), pcm24.size(), clip, error) && clip.samples.size() == 2);
    ASTRAL_CHECK_NEAR(clip.samples[0], 0.5f, 1e-6);
    ASTRAL_CHECK_NEAR(clip.samples[1], -0.5f, 1e-6);
    float f = 0.75f;
    std::vector<std::uint8_t> floatBytes(4);
    std::memcpy(floatBytes.data(), &f, 4);
    const std::vector<std::uint8_t> ieee = build(3, 32, floatBytes);
    ASTRAL_CHECK(Audio::DecodeWav(ieee.data(), ieee.size(), clip, error) && clip.samples[0] == 0.75f && clip.sampleRate == 22050);
    std::vector<std::uint8_t> bad = ieee;
    bad[20] = 2; // ADPCM
    ASTRAL_CHECK(!Audio::DecodeWav(bad.data(), bad.size(), clip, error));
    std::filesystem::remove_all(dir);
}

// ---------------------------------------------------------------- glTF

ASTRAL_TEST(GltfRoundTripsThroughEmbeddedAndBinaryContainers) {
    const GltfDocument source = MakeFixture();
    for (GltfContainer container : {GltfContainer::Embedded, GltfContainer::Binary}) {
        std::string error;
        const std::vector<std::uint8_t> bytes = ExportGltfToMemory(source, container, error);
        ASTRAL_CHECK(!bytes.empty());
        ASTRAL_CHECK((container == GltfContainer::Binary) == (bytes.size() > 4 && std::memcmp(bytes.data(), "glTF", 4) == 0));
        GltfDocument loaded;
        ASTRAL_CHECK(ImportGltfFromMemory(bytes.data(), bytes.size(), "", loaded, error));
        ASTRAL_CHECK(loaded.warnings.empty());
        CheckSameDocument(source, loaded);
        ASTRAL_CHECK(loaded.RootNodes() == std::vector<int>({0}) && loaded.generator == "Astral test");
    }
    // Files on disk with a relative external reference.
    const std::string dir = TempDir("astral_gltf_test");
    std::string error;
    ASTRAL_CHECK(ExportGltf(source, dir + "/fixture.glb", GltfContainer::Binary, error));
    GltfDocument fromFile;
    ASTRAL_CHECK(ImportGltf(dir + "/fixture.glb", fromFile, error));
    CheckSameDocument(source, fromFile);
    std::filesystem::remove_all(dir);
}

ASTRAL_TEST(GltfConvertsRightHandedDataToEngineSpace) {
    // One triangle facing glTF +Z (counter-clockwise), in a data-URI buffer, with STRIP and a fan copy.
    std::vector<std::uint8_t> buffer;
    auto put = [&](const void* p, std::size_t n) {
        const auto* b = static_cast<const std::uint8_t*>(p);
        buffer.insert(buffer.end(), b, b + n);
    };
    const float positions[] = {0, 0, 0, 1, 0, 0, 0, 1, 0};
    put(positions, sizeof(positions));
    const std::uint16_t indices[] = {0, 1, 2, 0};
    put(indices, sizeof(indices));
    const std::string uri = "data:application/octet-stream;base64," + Core::Base64Encode(buffer.data(), buffer.size());
    const std::string json = R"({"asset":{"version":"2.0"},"scene":0,"scenes":[{"nodes":[0,1]}],
      "nodes":[{"mesh":0,"translation":[2,0,0],"rotation":[0,0.3826834,0,0.9238795]},{"mesh":1}],
      "meshes":[{"primitives":[{"attributes":{"POSITION":0},"indices":1}]},
                {"primitives":[{"attributes":{"POSITION":0},"indices":1,"mode":5},{"attributes":{"POSITION":0},"mode":1}]}],
      "buffers":[{"byteLength":44,"uri":")" + uri + R"("}],
      "bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":36},{"buffer":0,"byteOffset":36,"byteLength":8}],
      "accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3"},
                   {"bufferView":1,"componentType":5123,"count":3,"type":"SCALAR"}]})";
    GltfDocument doc;
    std::string error;
    ASTRAL_CHECK(ImportGltfFromMemory(reinterpret_cast<const std::uint8_t*>(json.data()), json.size(), "", doc, error));
    const Graphics::MeshData& mesh = doc.meshes[0].primitives[0].mesh;
    ASTRAL_CHECK(mesh.vertices[1].position.x == -1.0f);                     // X mirrored
    ASTRAL_CHECK(mesh.indices == std::vector<std::uint32_t>({0, 2, 1}));    // winding reversed
    ASTRAL_CHECK(Near(mesh.vertices[0].normal, {0, 0, 1}));                  // still faces +Z
    ASTRAL_CHECK(doc.nodes[0].local.translation.x == -2.0f);
    // +45 degrees about glTF +Y becomes -45 degrees about engine +Y (mirror).
    ASTRAL_CHECK(NearQuat(doc.nodes[0].local.rotation, Math::QuatFromYaw(Math::Radians(-45.0f)), 1e-5f));
    ASTRAL_CHECK(doc.meshes[1].primitives.size() == 1 && doc.warnings.size() == 1); // lines primitive skipped
    ASTRAL_CHECK(doc.meshes[1].primitives[0].mesh.indices.size() == 3);

    // Rendered from the front (+Z side) the imported face is visible; from behind it is culled.
    Graphics::Material material;
    Graphics::MeshData visible = mesh;
    visible.ComputeBounds();
    Graphics::RenderScene scene;
    scene.draws.push_back({&visible, &material, Math::Mat4::Identity(), 7});
    Graphics::SceneRenderer renderer(nullptr);
    Graphics::RenderTarget target;
    auto centreId = [&](Vec3 eye) {
        const Graphics::RenderView view = Graphics::RenderView::Perspective(eye, {-0.3f, 0.3f, 0.0f}, 40.0f, 64, 64);
        renderer.Render(scene, view, target);
        return target.objectId[target.Index(32, 32)];
    };
    ASTRAL_CHECK(centreId({-0.3f, 0.3f, 3.0f}) == 7);
    ASTRAL_CHECK(centreId({-0.3f, 0.3f, -3.0f}) == 0);
}

ASTRAL_TEST(GltfSkinsAndClipsDriveEngineSkinning) {
    const GltfDocument doc = MakeFixture();
    GltfSkeletonBinding binding;
    std::string error;
    ASTRAL_CHECK(BuildSkeleton(doc, 0, binding, error));
    ASTRAL_CHECK(binding.skeleton.JointCount() == 2 && binding.skeleton.GetJoint(1).parent == 0);
    ASTRAL_CHECK(binding.nodeToJoint[1] == 0 && binding.nodeToJoint[2] == 1 && binding.nodeToJoint[3] == -1);
    // Bind pose reproduces the mesh exactly (skin matrices are identity).
    const Graphics::MeshData& bindMesh = doc.meshes[0].primitives[0].mesh;
    Animation::Pose pose = Animation::Pose::Bind(binding.skeleton);
    std::vector<Mat4> skin;
    Animation::ComputeSkinMatrices(binding.skeleton, pose, skin);
    Graphics::MeshData skinned;
    Animation::SkinMesh(bindMesh, skin, skinned);
    for (std::size_t v = 0; v < bindMesh.vertices.size(); ++v) {
        ASTRAL_CHECK(Near(skinned.vertices[v].position, bindMesh.vertices[v].position, 1e-4f));
    }
    // Animated: upper vertices follow Spine through 45 degrees at t = 0.5.
    const Animation::AnimationClip clip = BuildClip(doc, 0, binding);
    std::string clipError;
    ASTRAL_CHECK(clip.name == "Wave" && std::fabs(clip.duration - 1.0f) < 1e-6f && clip.Validate(binding.skeleton, clipError));
    clip.Sample(0.5f, pose);
    const Quat spineRotation = pose.local[1].rotation;
    ASTRAL_CHECK(NearQuat(spineRotation, Math::QuatFromAxisAngle({0, 0, 1}, Math::Radians(45.0f)), 1e-3f));
    ASTRAL_CHECK_NEAR(pose.local[1].scale.x, 1.5f, 1e-3); // cubic spline midpoint with zero tangents
    ASTRAL_CHECK_NEAR(pose.local[0].translation.y, 1.0f, 1e-4); // STEP holds until 0.6
    clip.Sample(0.7f, pose);
    ASTRAL_CHECK_NEAR(pose.local[0].translation.y, 1.2f, 1e-4);
    clip.Sample(0.5f, pose);
    Animation::ComputeSkinMatrices(binding.skeleton, pose, skin);
    Animation::SkinMesh(bindMesh, skin, skinned);
    // Expected: glTF joint global at t (Root * Hips * Spine(t)) * inverseBind * v.
    Mat4 spineWorld = doc.WorldMatrix(1) * Math::ToMat4(pose.local[1]);
    for (std::size_t v = 0; v < bindMesh.vertices.size(); ++v) {
        if (bindMesh.skin[v].joints[0] != 1) continue;
        const Vec3 expected = Math::TransformPoint(spineWorld * doc.skins[0].inverseBind[1], bindMesh.vertices[v].position);
        ASTRAL_CHECK(Near(skinned.vertices[v].position, expected, 1e-3f));
    }
    GltfSkeletonBinding bad;
    ASTRAL_CHECK(!BuildSkeleton(doc, 3, bad, error));
}

ASTRAL_TEST(GltfRejectsMalformedFilesSafely) {
    const GltfDocument source = MakeFixture();
    std::string error;
    const std::vector<std::uint8_t> glb = ExportGltfToMemory(source, GltfContainer::Binary, error);
    GltfDocument doc;
    const char* cases[] = {
        R"({"asset":{"version":"1.0"}})",
        R"({"asset":{"version":"2.0"},"extensionsRequired":["KHR_draco_mesh_compression"]})",
        R"({"asset":{"version":"2.0"},"buffers":[{"byteLength":8,"uri":"data:application/octet-stream;base64,AAAA"}]})",
        R"({"asset":{"version":"2.0"},"buffers":[{"byteLength":4,"uri":"../../etc/passwd"}]})",
        R"({"asset":{"version":"2.0"},"nodes":[{"children":[1]},{"children":[0]}]})",
        R"({"asset":{"version":"2.0"},"nodes":[{"children":[0]}]})",
        R"({"asset":{"version":"2.0"},"buffers":[{"byteLength":12,"uri":"data:application/octet-stream;base64,AAAAAAAAAAAAAAAA"}],
            "bufferViews":[{"buffer":0,"byteLength":12}],
            "accessors":[{"bufferView":0,"componentType":5126,"count":2,"type":"VEC3"}],
            "meshes":[{"primitives":[{"attributes":{"POSITION":0}}]}]})",
        R"({"asset":{"version":"2.0"},"buffers":[{"byteLength":12,"uri":"data:application/octet-stream;base64,AAAAAAAAAAAAAAAA"}],
            "bufferViews":[{"buffer":0,"byteLength":12}],
            "accessors":[{"bufferView":0,"componentType":5126,"count":1,"type":"VEC3"},{"bufferView":0,"componentType":5123,"count":1,"type":"SCALAR","byteOffset":0}],
            "meshes":[{"primitives":[{"attributes":{"POSITION":0},"indices":1}]}]})",
    };
    for (const char* text : cases) {
        const std::string s(text);
        const bool ok = ImportGltfFromMemory(reinterpret_cast<const std::uint8_t*>(s.data()), s.size(), "", doc, error);
        // The last case is valid except index 0 == count 1 -> in range; everything else must fail.
        if (s.find("\"indices\":1") == std::string::npos) ASTRAL_CHECK(!ok && !error.empty());
    }
    // Random corruption of a real GLB never crashes.
    Core::Random rng(77);
    std::size_t failures = 0;
    for (int trial = 0; trial < 150; ++trial) {
        std::vector<std::uint8_t> mutated = glb;
        const int flips = 1 + static_cast<int>(rng.NextBounded(8));
        for (int k = 0; k < flips; ++k) mutated[rng.NextBounded(static_cast<std::uint32_t>(mutated.size()))] = static_cast<std::uint8_t>(rng.NextU32());
        if (!ImportGltfFromMemory(mutated.data(), mutated.size(), "", doc, error)) ++failures;
    }
    ASTRAL_CHECK(failures > 0);
    std::vector<std::uint8_t> truncated(glb.begin(), glb.begin() + 30);
    ASTRAL_CHECK(!ImportGltfFromMemory(truncated.data(), truncated.size(), "", doc, error));
}

ASTRAL_TEST(GltfMaterialsBecomeEngineMaterials) {
    const GltfDocument doc = MakeFixture();
    Graphics::Texture2D texture;
    texture.FromImage(doc.images[0].image);
    const Graphics::Material coat = BuildMaterial(doc.materials[0], &texture);
    ASTRAL_CHECK(coat.shading == Graphics::ShadingModel::Lit && coat.blend == Graphics::BlendMode::AlphaBlend);
    ASTRAL_CHECK(coat.baseTexture == &texture && coat.doubleSided && std::fabs(coat.opacity - 0.5f) < 1e-6f);
    ASTRAL_CHECK(std::fabs(coat.metallic - 0.25f) < 1e-6f && Near(coat.emissive, {2.0f, 1.0f, 0.5f}));
    ASTRAL_CHECK(BuildMaterial(doc.materials[1], nullptr).shading == Graphics::ShadingModel::Unlit);
    ASTRAL_CHECK(BuildMaterial(doc.materials[0], nullptr, Graphics::ShadingModel::Toon).shading == Graphics::ShadingModel::Toon);
}

// ---------------------------------------------------------------- Asset manager

ASTRAL_TEST(AssetManagerCachesLoadsAsyncAndHotReloads) {
    const std::string dir = TempDir("astral_asset_manager");
    std::filesystem::create_directories(dir + "/Config");
    {
        std::ofstream(dir + "/Config/game.json") << R"({"gravity": -9.8})";
        std::ofstream(dir + "/readme.txt") << "hello";
        std::string error;
        Graphics::ImageRgba8 image = TestPattern(8, 8);
        ASTRAL_CHECK(Graphics::WritePng(dir + "/tex.png", image, error));
        WriteBytes(dir + "/photo.jpg", EncodeJpeg(image, 3, 2, 0));
        ASTRAL_CHECK(ExportGltf(MakeFixture(), dir + "/hero.glb", GltfContainer::Binary, error));
        ASTRAL_CHECK(Audio::WriteWav(dir + "/blip.wav", std::vector<float>(200, 0.1f), 48000, error));
    }
    Core::JobSystem jobs(2);
    AssetManager assets(dir, &jobs);
    assets.RegisterDefaultLoaders();

    AssetHandle<Core::JsonValue> config = assets.Load<Core::JsonValue>("Config/game.json");
    ASTRAL_CHECK(config.Ready() && config->Number("gravity") == -9.8 && config.Version() == 1);
    ASTRAL_CHECK(assets.Load<Core::JsonValue>("Config/game.json") == config); // cached
    ASTRAL_CHECK(assets.Load<std::string>("readme.txt").Get() && *assets.Load<std::string>("readme.txt").Get() == "hello");

    AssetHandle<GltfDocument> hero = assets.LoadAsync<GltfDocument>("hero.glb");
    AssetHandle<Graphics::Texture2D> png = assets.LoadAsync<Graphics::Texture2D>("tex.png");
    AssetHandle<Graphics::Texture2D> jpg = assets.LoadAsync<Graphics::Texture2D>("photo.jpg");
    AssetHandle<Audio::AudioClip> blip = assets.LoadAsync<Audio::AudioClip>("blip.wav");
    int readyCallbacks = 0;
    assets.OnReady(hero, [&] { ++readyCallbacks; });
    assets.WaitForAll();
    ASTRAL_CHECK(readyCallbacks == 1);
    ASTRAL_CHECK(hero.Ready() && hero->skins.size() == 1 && hero->meshes.size() == 1);
    ASTRAL_CHECK(png.Ready() && png->Width() == 8 && png->LevelCount() == 4);
    ASTRAL_CHECK(jpg.Ready() && jpg->Height() == 8);
    ASTRAL_CHECK(blip.Ready() && blip->samples.size() == 100);

    AssetHandle<Core::JsonValue> missing = assets.Load<Core::JsonValue>("nope.json");
    ASTRAL_CHECK(missing.State() == AssetState::Failed && !missing.Error().empty() && !missing.Get());
    ASTRAL_CHECK(assets.Load<Core::JsonValue>("../outside.json").State() == AssetState::Failed);
    ASTRAL_CHECK(assets.Load<int>("readme.txt").State() == AssetState::Failed); // no loader

    // Hot reload: new content and a newer timestamp.
    {
        std::ofstream(dir + "/Config/game.json") << R"({"gravity": -3.7})";
    }
    std::filesystem::last_write_time(dir + "/Config/game.json",
        std::filesystem::last_write_time(dir + "/Config/game.json") + std::chrono::seconds(5));
    ASTRAL_CHECK(assets.PollChanges() == 1);
    ASTRAL_CHECK(config->Number("gravity") == -3.7 && config.Version() == 2);
    ASTRAL_CHECK(assets.PollChanges() == 0);
    // A broken edit keeps the last good data and reports the error.
    {
        std::ofstream(dir + "/Config/game.json") << "{ broken";
    }
    std::filesystem::last_write_time(dir + "/Config/game.json",
        std::filesystem::last_write_time(dir + "/Config/game.json") + std::chrono::seconds(10));
    ASTRAL_CHECK(assets.PollChanges() == 1);
    ASTRAL_CHECK(config.Ready() && config->Number("gravity") == -3.7 && !config.Error().empty());

    const AssetManagerStats before = assets.Stats();
    ASTRAL_CHECK(before.ready >= 6 && before.failed >= 3 && before.reloads == 2);
    png = {};
    missing = {};
    ASTRAL_CHECK(assets.Collect() >= 2);
    ASTRAL_CHECK(assets.Stats().cached < before.cached);
    ASTRAL_CHECK(config.Ready()); // still referenced, still cached
    std::filesystem::remove_all(dir);
}

ASTRAL_TEST_MAIN("EngineAssetsTests")
