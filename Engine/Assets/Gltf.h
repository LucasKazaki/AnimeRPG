#pragma once

// glTF 2.0 import and export (.gltf with external or data-URI buffers, and
// binary .glb). This is the engine's interchange format, the one Blender, Maya,
// Unreal and Unity all read and write, covering:
//   meshes (positions, normals, UVs, colours, skin joints/weights; triangle
//   lists, strips and fans; sparse accessors), PBR metallic-roughness materials
//   (+ KHR_materials_unlit), PNG and baseline JPEG textures, node hierarchies,
//   skins with inverse bind matrices, and animations (LINEAR, STEP and
//   CUBICSPLINE translation/rotation/scale channels).
//
// Everything is converted to engine conventions on import (left-handed, +X
// right, +Y up, +Z forward, clockwise front faces) by mirroring X, and back on
// export. Files are untrusted: every index, offset and size is range-checked.

#include "Engine/Animation/Skeleton.h"
#include "Engine/Graphics/Image.h"
#include "Engine/Graphics/Material.h"
#include "Engine/Graphics/Mesh.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Astral::Assets {

struct GltfImage {
    std::string name;
    std::string uri; // original reference (informational)
    std::string mimeType;
    Graphics::ImageRgba8 image; // decoded; empty when decoding failed or was disabled
};

struct GltfTexture {
    std::string name;
    int image{-1};
    bool clamp{}; // sampler wraps with CLAMP_TO_EDGE
};

enum class GltfAlphaMode : std::uint8_t { Opaque, Mask, Blend };

struct GltfMaterial {
    std::string name;
    Math::Vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f}; // linear
    int baseColorTexture{-1};
    float metallicFactor{1.0f};
    float roughnessFactor{1.0f};
    Math::Vec3 emissiveFactor{};
    int emissiveTexture{-1};
    GltfAlphaMode alphaMode{GltfAlphaMode::Opaque};
    float alphaCutoff{0.5f};
    bool doubleSided{};
    bool unlit{}; // KHR_materials_unlit
};

struct GltfPrimitive {
    Graphics::MeshData mesh; // engine space; skin filled when JOINTS_0/WEIGHTS_0 exist
    int material{-1};
};

struct GltfMesh {
    std::string name;
    std::vector<GltfPrimitive> primitives;
};

struct GltfNode {
    std::string name;
    Math::TRS local; // engine space
    int mesh{-1};
    int skin{-1};
    int parent{-1};
    std::vector<int> children;
};

struct GltfSkin {
    std::string name;
    std::vector<int> joints;               // node indices
    std::vector<Math::Mat4> inverseBind;   // engine space, one per joint
};

enum class GltfPath : std::uint8_t { Translation, Rotation, Scale };
enum class GltfInterpolation : std::uint8_t { Linear, Step, CubicSpline };

struct GltfChannel {
    int node{-1};
    GltfPath path{GltfPath::Translation};
    GltfInterpolation interpolation{GltfInterpolation::Linear};
    std::vector<float> times;
    // xyz (translation/scale) or xyzw (rotation) in engine space. CUBICSPLINE
    // stores three per key: in-tangent, value, out-tangent.
    std::vector<Math::Vec4> values;
};

struct GltfAnimation {
    std::string name;
    std::vector<GltfChannel> channels;
    float Duration() const;
};

struct GltfScene {
    std::string name;
    std::vector<int> nodes;
};

struct GltfDocument {
    std::vector<GltfNode> nodes;
    std::vector<GltfMesh> meshes;
    std::vector<GltfMaterial> materials;
    std::vector<GltfTexture> textures;
    std::vector<GltfImage> images;
    std::vector<GltfSkin> skins;
    std::vector<GltfAnimation> animations;
    std::vector<GltfScene> scenes;
    int scene{-1};
    std::string generator;
    std::vector<std::string> warnings; // non-fatal skips (unsupported modes, undecodable images)

    // Roots of the default scene (or every parentless node when there is none).
    std::vector<int> RootNodes() const;
    Math::Mat4 WorldMatrix(int node) const;
    Math::TRS WorldTrs(int node) const;
};

struct GltfImportOptions {
    std::size_t maxFileBytes{512u * 1024u * 1024u};
    std::size_t maxElements{64u * 1024u * 1024u}; // per accessor
    bool decodeImages{true};
};

bool ImportGltf(const std::string& path, GltfDocument& out, std::string& error, const GltfImportOptions& options = {});
// `baseDirectory` resolves relative buffer/image URIs ("" disallows external files).
bool ImportGltfFromMemory(const std::uint8_t* data, std::size_t size, const std::string& baseDirectory,
    GltfDocument& out, std::string& error, const GltfImportOptions& options = {});

enum class GltfContainer : std::uint8_t { Embedded, Binary }; // .gltf with data URIs, or .glb
std::vector<std::uint8_t> ExportGltfToMemory(const GltfDocument& document, GltfContainer container, std::string& error);
bool ExportGltf(const GltfDocument& document, const std::string& path, GltfContainer container, std::string& error);

// ---------------------------------------------------------------- Engine conversion

struct GltfSkeletonBinding {
    Animation::Skeleton skeleton;
    std::vector<int> nodeToJoint;   // per document node; -1 when not a joint
    std::vector<int> jointToNode;
    std::vector<Math::TRS> prefix;  // transform from the parent joint (or world) to the joint's parent node
};

// Joint order satisfies the engine's parent-before-child rule; the skin's
// inverse bind matrices are kept exactly.
bool BuildSkeleton(const GltfDocument& document, int skin, GltfSkeletonBinding& out, std::string& error);
// Samples every channel that targets a joint (CUBICSPLINE is resampled at
// `resampleRate` Hz; STEP holds values) into an engine clip.
Animation::AnimationClip BuildClip(const GltfDocument& document, int animation, const GltfSkeletonBinding& binding,
    float resampleRate = 30.0f);
// Maps PBR metallic-roughness onto the engine material. `baseTexture` may be null.
Graphics::Material BuildMaterial(const GltfMaterial& material, const Graphics::Texture2D* baseTexture,
    Graphics::ShadingModel shading = Graphics::ShadingModel::Lit);

// Baseline (sequential Huffman) JPEG decoder: 8-bit greyscale or YCbCr with
// any 1-2x chroma subsampling and restart markers. Progressive files are
// rejected with an error.
bool DecodeJpeg(const std::uint8_t* data, std::size_t size, Graphics::ImageRgba8& out, std::string& error);

} // namespace Astral::Assets
