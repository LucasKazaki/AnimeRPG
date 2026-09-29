#include "Engine/Assets/Gltf.h"

#include "Engine/Core/Base64.h"
#include "Engine/Core/Json.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <map>
#include <utility>

namespace Astral::Assets {

using Core::JsonValue;
using Math::Mat4;
using Math::Quat;
using Math::TRS;
using Math::Vec2;
using Math::Vec3;
using Math::Vec4;

namespace {

// ---------------------------------------------------------------- Coordinate conversion
// glTF is right-handed (+Y up, +Z forward, -X right); the engine is left-handed
// (+X right). Mirroring X maps one onto the other and is its own inverse.

Vec3 FlipX(Vec3 v) { return {-v.x, v.y, v.z}; }
Quat FlipX(Quat q) { return {q.x, -q.y, -q.z, q.w}; }
Mat4 FlipX(Mat4 m) {
    for (int i = 1; i < 4; ++i) {
        m.m[0][i] = -m.m[0][i];
        m.m[i][0] = -m.m[i][0];
    }
    return m;
}
TRS FlipX(const TRS& t) { return {FlipX(t.translation), FlipX(t.rotation), t.scale}; }

constexpr int kByte = 5120, kUnsignedByte = 5121, kShort = 5122, kUnsignedShort = 5123, kUnsignedInt = 5125,
              kFloat = 5126;
constexpr int kArrayBuffer = 34962, kElementArrayBuffer = 34963;
constexpr std::uint32_t kGlbMagic = 0x46546C67, kJsonChunk = 0x4E4F534A, kBinChunk = 0x004E4942;

int ComponentSize(int type) {
    switch (type) {
    case kByte:
    case kUnsignedByte: return 1;
    case kShort:
    case kUnsignedShort: return 2;
    case kUnsignedInt:
    case kFloat: return 4;
    default: return 0;
    }
}

int ComponentCount(const std::string& type) {
    if (type == "SCALAR") return 1;
    if (type == "VEC2") return 2;
    if (type == "VEC3") return 3;
    if (type == "VEC4") return 4;
    if (type == "MAT2") return 4;
    if (type == "MAT3") return 9;
    if (type == "MAT4") return 16;
    return 0;
}

std::uint32_t ReadU32(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8)
        | (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
}

void WriteU32(std::vector<std::uint8_t>& out, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) out.push_back(static_cast<std::uint8_t>(v >> (8 * i)));
}

std::string PercentDecode(const std::string& text) {
    std::string out;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '%' && i + 2 < text.size() && std::isxdigit(static_cast<unsigned char>(text[i + 1]))
            && std::isxdigit(static_cast<unsigned char>(text[i + 2]))) {
            out.push_back(static_cast<char>(std::stoi(text.substr(i + 1, 2), nullptr, 16)));
            i += 2;
        } else {
            out.push_back(text[i]);
        }
    }
    return out;
}

bool ReadFile(const std::string& path, std::size_t maxBytes, std::vector<std::uint8_t>& out, std::string& error) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        error = "cannot open " + path;
        return false;
    }
    const std::streamoff size = file.tellg();
    if (size < 0 || static_cast<std::uint64_t>(size) > maxBytes) {
        error = path + " exceeds the size limit";
        return false;
    }
    out.resize(static_cast<std::size_t>(size));
    file.seekg(0);
    if (size > 0 && !file.read(reinterpret_cast<char*>(out.data()), size)) {
        error = "cannot read " + path;
        return false;
    }
    return true;
}

// ---------------------------------------------------------------- Import context

struct Importer {
    const JsonValue& json;
    std::string baseDirectory;
    const GltfImportOptions& options;
    std::vector<std::vector<std::uint8_t>> buffers;
    GltfDocument& doc;

    bool LoadUri(const std::string& uri, std::vector<std::uint8_t>& out, std::string& error) {
        if (uri.compare(0, 5, "data:") == 0) {
            const std::size_t comma = uri.find(',');
            if (comma == std::string::npos || uri.substr(0, comma).find(";base64") == std::string::npos) {
                error = "unsupported data URI";
                return false;
            }
            if (!Core::Base64Decode(std::string_view(uri).substr(comma + 1), out)) {
                error = "malformed base64 data URI";
                return false;
            }
            return true;
        }
        if (baseDirectory.empty()) {
            error = "external URI '" + uri + "' with no base directory";
            return false;
        }
        const std::string relative = PercentDecode(uri);
        if (relative.empty() || relative[0] == '/' || relative[0] == '\\' || relative.find(':') != std::string::npos) {
            error = "absolute or scheme URI '" + uri + "' is not allowed";
            return false;
        }
        return ReadFile(baseDirectory + "/" + relative, options.maxFileBytes, out, error);
    }

    bool LoadBuffers(const std::vector<std::uint8_t>* glbBin, std::string& error) {
        const JsonValue& list = json["buffers"];
        for (std::size_t i = 0; i < list.Size(); ++i) {
            const JsonValue& buffer = list[i];
            const std::int64_t byteLength = buffer.Int("byteLength", -1);
            if (byteLength < 0) {
                error = "buffer " + std::to_string(i) + " has no byteLength";
                return false;
            }
            std::vector<std::uint8_t> data;
            if (buffer.Has("uri")) {
                if (!LoadUri(buffer.String("uri"), data, error)) return false;
            } else if (i == 0 && glbBin) {
                data = *glbBin;
            } else {
                error = "buffer " + std::to_string(i) + " has no data";
                return false;
            }
            if (data.size() < static_cast<std::uint64_t>(byteLength)) {
                error = "buffer " + std::to_string(i) + " is shorter than byteLength";
                return false;
            }
            data.resize(static_cast<std::size_t>(byteLength));
            buffers.push_back(std::move(data));
        }
        return true;
    }

    // A validated view: [data, data + length) with an element stride.
    struct View {
        const std::uint8_t* data{};
        std::size_t length{};
        std::size_t stride{};
    };

    bool GetBufferView(int index, View& view, std::string& error) {
        const JsonValue& bv = json["bufferViews"][static_cast<std::size_t>(index)];
        if (!bv.IsObject()) {
            error = "bufferView " + std::to_string(index) + " is missing";
            return false;
        }
        const std::int64_t buffer = bv.Int("buffer", -1);
        const std::int64_t offset = bv.Int("byteOffset", 0);
        const std::int64_t length = bv.Int("byteLength", -1);
        const std::int64_t stride = bv.Int("byteStride", 0);
        if (buffer < 0 || static_cast<std::size_t>(buffer) >= buffers.size() || offset < 0 || length < 0
            || stride < 0 || stride > 252) {
            error = "bufferView " + std::to_string(index) + " is invalid";
            return false;
        }
        const std::vector<std::uint8_t>& data = buffers[static_cast<std::size_t>(buffer)];
        if (static_cast<std::uint64_t>(offset) + static_cast<std::uint64_t>(length) > data.size()) {
            error = "bufferView " + std::to_string(index) + " overruns its buffer";
            return false;
        }
        view = {data.data() + offset, static_cast<std::size_t>(length), static_cast<std::size_t>(stride)};
        return true;
    }

    static float Decode(const std::uint8_t* p, int componentType, bool normalized) {
        switch (componentType) {
        case kFloat: {
            float v;
            std::memcpy(&v, p, 4);
            return v;
        }
        case kUnsignedByte: return normalized ? p[0] / 255.0f : static_cast<float>(p[0]);
        case kByte: {
            const auto v = static_cast<std::int8_t>(p[0]);
            return normalized ? std::max(v / 127.0f, -1.0f) : static_cast<float>(v);
        }
        case kUnsignedShort: {
            const std::uint16_t v = static_cast<std::uint16_t>(p[0] | (p[1] << 8));
            return normalized ? v / 65535.0f : static_cast<float>(v);
        }
        case kShort: {
            const auto v = static_cast<std::int16_t>(static_cast<std::uint16_t>(p[0] | (p[1] << 8)));
            return normalized ? std::max(v / 32767.0f, -1.0f) : static_cast<float>(v);
        }
        case kUnsignedInt: return static_cast<float>(ReadU32(p));
        default: return 0.0f;
        }
    }

    // Reads an accessor as floats (`components` per element; `count` out).
    bool ReadAccessor(int index, int expectedComponents, std::vector<float>& out, std::size_t& count,
        std::string& error, std::vector<std::uint32_t>* integers = nullptr) {
        const JsonValue& accessor = json["accessors"][static_cast<std::size_t>(index)];
        if (!accessor.IsObject()) {
            error = "accessor " + std::to_string(index) + " is missing";
            return false;
        }
        const int componentType = static_cast<int>(accessor.Int("componentType", 0));
        const int components = ComponentCount(accessor.String("type"));
        const int componentSize = ComponentSize(componentType);
        const std::int64_t elementCount = accessor.Int("count", -1);
        const bool normalized = accessor.Bool("normalized");
        if (components == 0 || componentSize == 0 || elementCount < 0
            || static_cast<std::uint64_t>(elementCount) > options.maxElements
            || (expectedComponents > 0 && components != expectedComponents)) {
            error = "accessor " + std::to_string(index) + " has an unexpected layout";
            return false;
        }
        count = static_cast<std::size_t>(elementCount);
        const std::size_t total = count * static_cast<std::size_t>(components);
        out.assign(total, 0.0f);
        if (integers) integers->assign(total, 0u);
        if (accessor.Has("bufferView")) {
            View view;
            if (!GetBufferView(static_cast<int>(accessor.Int("bufferView", -1)), view, error)) return false;
            const std::int64_t offset = accessor.Int("byteOffset", 0);
            const std::size_t elementSize = static_cast<std::size_t>(components * componentSize);
            const std::size_t stride = view.stride ? view.stride : elementSize;
            if (offset < 0 || stride < elementSize) {
                error = "accessor " + std::to_string(index) + " has a bad offset or stride";
                return false;
            }
            if (count > 0) {
                const std::uint64_t needed = static_cast<std::uint64_t>(offset)
                    + static_cast<std::uint64_t>(count - 1) * stride + elementSize;
                if (needed > view.length) {
                    error = "accessor " + std::to_string(index) + " overruns its bufferView";
                    return false;
                }
            }
            for (std::size_t e = 0; e < count; ++e) {
                const std::uint8_t* element = view.data + offset + e * stride;
                for (int c = 0; c < components; ++c) {
                    const std::uint8_t* p = element + c * componentSize;
                    const std::size_t slot = e * static_cast<std::size_t>(components) + static_cast<std::size_t>(c);
                    out[slot] = Decode(p, componentType, normalized);
                    if (integers) {
                        integers->at(slot) = componentType == kUnsignedInt ? ReadU32(p)
                            : componentType == kUnsignedShort ? static_cast<std::uint32_t>(p[0] | (p[1] << 8))
                                                              : static_cast<std::uint32_t>(p[0]);
                    }
                }
            }
        }
        // Sparse substitution.
        const JsonValue& sparse = accessor["sparse"];
        if (sparse.IsObject()) {
            const std::int64_t sparseCount = sparse.Int("count", -1);
            if (sparseCount < 0 || static_cast<std::uint64_t>(sparseCount) > count) {
                error = "accessor " + std::to_string(index) + " has a bad sparse count";
                return false;
            }
            const JsonValue& indices = sparse["indices"];
            const JsonValue& values = sparse["values"];
            View indexView, valueView;
            if (!GetBufferView(static_cast<int>(indices.Int("bufferView", -1)), indexView, error)
                || !GetBufferView(static_cast<int>(values.Int("bufferView", -1)), valueView, error)) {
                return false;
            }
            const int indexType = static_cast<int>(indices.Int("componentType", 0));
            const int indexSize = ComponentSize(indexType);
            const std::int64_t indexOffset = indices.Int("byteOffset", 0);
            const std::int64_t valueOffset = values.Int("byteOffset", 0);
            const std::size_t elementSize = static_cast<std::size_t>(components * componentSize);
            const std::size_t n = static_cast<std::size_t>(sparseCount);
            if (indexSize == 0 || indexType == kFloat || indexOffset < 0 || valueOffset < 0
                || static_cast<std::uint64_t>(indexOffset) + n * static_cast<std::uint64_t>(indexSize) > indexView.length
                || static_cast<std::uint64_t>(valueOffset) + n * elementSize > valueView.length) {
                error = "accessor " + std::to_string(index) + " has out-of-range sparse data";
                return false;
            }
            for (std::size_t s = 0; s < n; ++s) {
                const std::uint8_t* ip = indexView.data + indexOffset + s * static_cast<std::size_t>(indexSize);
                const std::uint32_t target = indexType == kUnsignedInt ? ReadU32(ip)
                    : indexType == kUnsignedShort ? static_cast<std::uint32_t>(ip[0] | (ip[1] << 8))
                                                  : static_cast<std::uint32_t>(ip[0]);
                if (target >= count) {
                    error = "sparse index out of range";
                    return false;
                }
                for (int c = 0; c < components; ++c) {
                    const std::uint8_t* vp = valueView.data + valueOffset + s * elementSize
                        + static_cast<std::size_t>(c * componentSize);
                    out[target * static_cast<std::size_t>(components) + static_cast<std::size_t>(c)]
                        = Decode(vp, componentType, normalized);
                }
            }
        }
        for (float v : out) {
            if (!std::isfinite(v)) {
                error = "accessor " + std::to_string(index) + " contains non-finite values";
                return false;
            }
        }
        return true;
    }

    bool LoadImages(std::string& error) {
        const JsonValue& list = json["images"];
        for (std::size_t i = 0; i < list.Size(); ++i) {
            const JsonValue& image = list[i];
            GltfImage out;
            out.name = image.String("name");
            out.uri = image.String("uri");
            out.mimeType = image.String("mimeType");
            if (options.decodeImages) {
                std::vector<std::uint8_t> bytes;
                std::string imageError;
                bool loaded = false;
                if (image.Has("uri")) {
                    loaded = LoadUri(out.uri, bytes, imageError);
                } else if (image.Has("bufferView")) {
                    View view;
                    loaded = GetBufferView(static_cast<int>(image.Int("bufferView", -1)), view, imageError);
                    if (loaded) bytes.assign(view.data, view.data + view.length);
                } else {
                    imageError = "no source";
                }
                if (loaded) {
                    if (bytes.size() >= 8 && bytes[0] == 0x89 && bytes[1] == 'P' && bytes[2] == 'N' && bytes[3] == 'G') {
                        loaded = Graphics::DecodePng(bytes.data(), bytes.size(), out.image, imageError);
                    } else if (bytes.size() >= 3 && bytes[0] == 0xFF && bytes[1] == 0xD8) {
                        loaded = DecodeJpeg(bytes.data(), bytes.size(), out.image, imageError);
                    } else {
                        loaded = false;
                        imageError = "unsupported image format";
                    }
                }
                if (!loaded) {
                    out.image = {};
                    doc.warnings.push_back("image " + std::to_string(i) + ": " + imageError);
                }
            }
            doc.images.push_back(std::move(out));
        }
        (void)error;
        return true;
    }

    void LoadTexturesAndMaterials() {
        const JsonValue& samplers = json["samplers"];
        const JsonValue& textures = json["textures"];
        for (std::size_t i = 0; i < textures.Size(); ++i) {
            GltfTexture texture;
            texture.name = textures[i].String("name");
            const std::int64_t source = textures[i].Int("source", -1);
            texture.image = source >= 0 && static_cast<std::size_t>(source) < doc.images.size() ? static_cast<int>(source) : -1;
            const std::int64_t sampler = textures[i].Int("sampler", -1);
            if (sampler >= 0) {
                const JsonValue& s = samplers[static_cast<std::size_t>(sampler)];
                texture.clamp = s.Int("wrapS", 10497) == 33071 && s.Int("wrapT", 10497) == 33071;
            }
            doc.textures.push_back(texture);
        }
        auto textureIndex = [&](const JsonValue& info) {
            const std::int64_t index = info.Int("index", -1);
            return index >= 0 && static_cast<std::size_t>(index) < doc.textures.size() ? static_cast<int>(index) : -1;
        };
        const JsonValue& materials = json["materials"];
        for (std::size_t i = 0; i < materials.Size(); ++i) {
            const JsonValue& m = materials[i];
            GltfMaterial material;
            material.name = m.String("name");
            const JsonValue& pbr = m["pbrMetallicRoughness"];
            const JsonValue& factor = pbr["baseColorFactor"];
            if (factor.Size() == 4) {
                material.baseColorFactor = {factor[0].AsFloat(1), factor[1].AsFloat(1), factor[2].AsFloat(1), factor[3].AsFloat(1)};
            }
            material.baseColorTexture = textureIndex(pbr["baseColorTexture"]);
            material.metallicFactor = Math::Saturate(pbr.Float("metallicFactor", 1.0f));
            material.roughnessFactor = Math::Saturate(pbr.Float("roughnessFactor", 1.0f));
            const JsonValue& emissive = m["emissiveFactor"];
            if (emissive.Size() == 3) material.emissiveFactor = {emissive[0].AsFloat(), emissive[1].AsFloat(), emissive[2].AsFloat()};
            if (const JsonValue* strength = m["extensions"]["KHR_materials_emissive_strength"].Find("emissiveStrength")) {
                material.emissiveFactor = material.emissiveFactor * strength->AsFloat(1.0f);
            }
            material.emissiveTexture = textureIndex(m["emissiveTexture"]);
            const std::string alphaMode = m.String("alphaMode", "OPAQUE");
            material.alphaMode = alphaMode == "BLEND" ? GltfAlphaMode::Blend
                : alphaMode == "MASK"                  ? GltfAlphaMode::Mask
                                                       : GltfAlphaMode::Opaque;
            material.alphaCutoff = m.Float("alphaCutoff", 0.5f);
            material.doubleSided = m.Bool("doubleSided");
            material.unlit = m["extensions"].Has("KHR_materials_unlit");
            doc.materials.push_back(material);
        }
    }

    bool LoadPrimitive(const JsonValue& primitive, std::size_t meshIndex, GltfPrimitive& out, bool& skip,
        std::string& error) {
        skip = false;
        const std::int64_t mode = primitive.Int("mode", 4);
        if (mode < 4 || mode > 6) {
            doc.warnings.push_back("mesh " + std::to_string(meshIndex) + ": points/lines primitive skipped");
            skip = true;
            return true;
        }
        const JsonValue& attributes = primitive["attributes"];
        if (!attributes.Has("POSITION")) {
            error = "mesh " + std::to_string(meshIndex) + " primitive has no POSITION";
            return false;
        }
        std::vector<float> positions, normals, uvs, colors, joints, weights;
        std::size_t count = 0, n = 0;
        if (!ReadAccessor(static_cast<int>(attributes.Int("POSITION", -1)), 3, positions, count, error)) return false;
        Graphics::MeshData& mesh = out.mesh;
        mesh.vertices.resize(count);
        for (std::size_t v = 0; v < count; ++v) {
            mesh.vertices[v].position = FlipX(Vec3{positions[v * 3], positions[v * 3 + 1], positions[v * 3 + 2]});
        }
        const bool hasNormals = attributes.Has("NORMAL");
        if (hasNormals) {
            if (!ReadAccessor(static_cast<int>(attributes.Int("NORMAL", -1)), 3, normals, n, error)) return false;
            if (n != count) {
                error = "NORMAL count differs from POSITION";
                return false;
            }
            for (std::size_t v = 0; v < count; ++v) {
                mesh.vertices[v].normal = Math::Normalize(FlipX(Vec3{normals[v * 3], normals[v * 3 + 1], normals[v * 3 + 2]}), {0, 1, 0});
            }
        }
        if (attributes.Has("TEXCOORD_0")) {
            if (!ReadAccessor(static_cast<int>(attributes.Int("TEXCOORD_0", -1)), 2, uvs, n, error)) return false;
            if (n != count) {
                error = "TEXCOORD_0 count differs from POSITION";
                return false;
            }
            for (std::size_t v = 0; v < count; ++v) mesh.vertices[v].uv = {uvs[v * 2], uvs[v * 2 + 1]};
        }
        if (attributes.Has("COLOR_0")) {
            const int colorAccessor = static_cast<int>(attributes.Int("COLOR_0", -1));
            const int components = ComponentCount(json["accessors"][static_cast<std::size_t>(colorAccessor)].String("type"));
            if (components != 3 && components != 4) {
                error = "COLOR_0 must be VEC3 or VEC4";
                return false;
            }
            if (!ReadAccessor(colorAccessor, components, colors, n, error)) return false;
            if (n != count) {
                error = "COLOR_0 count differs from POSITION";
                return false;
            }
            for (std::size_t v = 0; v < count; ++v) {
                const float* c = &colors[v * static_cast<std::size_t>(components)];
                mesh.vertices[v].color = {c[0], c[1], c[2], components == 4 ? c[3] : 1.0f};
            }
        }
        if (attributes.Has("JOINTS_0") && attributes.Has("WEIGHTS_0")) {
            std::vector<std::uint32_t> jointIndices;
            if (!ReadAccessor(static_cast<int>(attributes.Int("JOINTS_0", -1)), 4, joints, n, error, &jointIndices)) return false;
            if (n != count) {
                error = "JOINTS_0 count differs from POSITION";
                return false;
            }
            if (!ReadAccessor(static_cast<int>(attributes.Int("WEIGHTS_0", -1)), 4, weights, n, error)) return false;
            if (n != count) {
                error = "WEIGHTS_0 count differs from POSITION";
                return false;
            }
            mesh.skin.resize(count);
            for (std::size_t v = 0; v < count; ++v) {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    const std::size_t slot = v * 4 + static_cast<std::size_t>(k);
                    const std::uint32_t joint = jointIndices[slot];
                    if (joint > 65535u) {
                        error = "joint index out of range";
                        return false;
                    }
                    mesh.skin[v].joints[static_cast<std::size_t>(k)] = static_cast<std::uint16_t>(joint);
                    mesh.skin[v].weights[static_cast<std::size_t>(k)] = std::max(0.0f, weights[slot]);
                    sum += mesh.skin[v].weights[static_cast<std::size_t>(k)];
                }
                if (sum > 1.0e-6f) {
                    for (float& w : mesh.skin[v].weights) w /= sum;
                } else {
                    mesh.skin[v].weights = {1.0f, 0.0f, 0.0f, 0.0f};
                }
            }
        }
        std::vector<std::uint32_t> indices;
        if (primitive.Has("indices")) {
            std::vector<float> ignored;
            if (!ReadAccessor(static_cast<int>(primitive.Int("indices", -1)), 1, ignored, n, error, &indices)) return false;
            const int type = static_cast<int>(json["accessors"][static_cast<std::size_t>(primitive.Int("indices", -1))].Int("componentType", 0));
            if (type != kUnsignedByte && type != kUnsignedShort && type != kUnsignedInt) {
                error = "indices must be unsigned integers";
                return false;
            }
        } else {
            indices.resize(count);
            for (std::size_t i = 0; i < count; ++i) indices[i] = static_cast<std::uint32_t>(i);
        }
        for (std::uint32_t index : indices) {
            if (index >= count) {
                error = "index out of range in mesh " + std::to_string(meshIndex);
                return false;
            }
        }
        // Triangulate strips/fans, then reverse winding (mirroring flips handedness).
        std::vector<std::uint32_t> triangles;
        if (mode == 4) {
            triangles.assign(indices.begin(), indices.begin() + static_cast<std::ptrdiff_t>(indices.size() / 3 * 3));
        } else if (mode == 5) {
            for (std::size_t i = 2; i < indices.size(); ++i) {
                if (i % 2 == 0) triangles.insert(triangles.end(), {indices[i - 2], indices[i - 1], indices[i]});
                else triangles.insert(triangles.end(), {indices[i - 1], indices[i - 2], indices[i]});
            }
        } else {
            for (std::size_t i = 2; i < indices.size(); ++i) triangles.insert(triangles.end(), {indices[0], indices[i - 1], indices[i]});
        }
        mesh.indices.reserve(triangles.size());
        for (std::size_t t = 0; t + 2 < triangles.size(); t += 3) {
            const std::uint32_t a = triangles[t], b = triangles[t + 1], c = triangles[t + 2];
            if (a == b || b == c || a == c) continue; // degenerate
            mesh.indices.insert(mesh.indices.end(), {a, c, b});
        }
        if (!hasNormals) mesh.RecomputeNormals();
        mesh.ComputeBounds();
        const std::int64_t material = primitive.Int("material", -1);
        out.material = material >= 0 && static_cast<std::size_t>(material) < doc.materials.size() ? static_cast<int>(material) : -1;
        return true;
    }

    bool LoadMeshes(std::string& error) {
        const JsonValue& meshes = json["meshes"];
        for (std::size_t i = 0; i < meshes.Size(); ++i) {
            GltfMesh mesh;
            mesh.name = meshes[i].String("name");
            const JsonValue& primitives = meshes[i]["primitives"];
            for (std::size_t p = 0; p < primitives.Size(); ++p) {
                GltfPrimitive primitive;
                bool skip = false;
                if (!LoadPrimitive(primitives[p], i, primitive, skip, error)) return false;
                if (skip) continue;
                primitive.mesh.name = mesh.name.empty() ? "mesh" + std::to_string(i) : mesh.name;
                if (primitives.Size() > 1) primitive.mesh.name += "#" + std::to_string(p);
                mesh.primitives.push_back(std::move(primitive));
            }
            doc.meshes.push_back(std::move(mesh));
        }
        return true;
    }

    bool LoadNodes(std::string& error) {
        const JsonValue& nodes = json["nodes"];
        doc.nodes.resize(nodes.Size());
        for (std::size_t i = 0; i < nodes.Size(); ++i) {
            const JsonValue& n = nodes[i];
            GltfNode& node = doc.nodes[i];
            node.name = n.String("name");
            TRS local;
            const JsonValue& matrix = n["matrix"];
            if (matrix.Size() == 16) {
                Mat4 m = Mat4::Identity();
                for (int c = 0; c < 4; ++c) {
                    for (int r = 0; r < 4; ++r) m.m[r][c] = matrix[static_cast<std::size_t>(c * 4 + r)].AsFloat();
                }
                local = Math::Decompose(m);
            } else {
                const JsonValue& t = n["translation"];
                const JsonValue& r = n["rotation"];
                const JsonValue& s = n["scale"];
                if (t.Size() == 3) local.translation = {t[0].AsFloat(), t[1].AsFloat(), t[2].AsFloat()};
                if (r.Size() == 4) local.rotation = Math::Normalize(Quat{r[0].AsFloat(), r[1].AsFloat(), r[2].AsFloat(), r[3].AsFloat(1)});
                if (s.Size() == 3) local.scale = {s[0].AsFloat(1), s[1].AsFloat(1), s[2].AsFloat(1)};
            }
            if (!Math::IsFinite(local.translation) || !Math::IsFinite(local.scale)) {
                error = "node " + std::to_string(i) + " has a non-finite transform";
                return false;
            }
            node.local = FlipX(local);
            const std::int64_t mesh = n.Int("mesh", -1);
            node.mesh = mesh >= 0 && static_cast<std::size_t>(mesh) < doc.meshes.size() ? static_cast<int>(mesh) : -1;
            const std::int64_t skin = n.Int("skin", -1);
            node.skin = skin >= 0 && static_cast<std::size_t>(skin) < json["skins"].Size() ? static_cast<int>(skin) : -1;
            const JsonValue& children = n["children"];
            for (std::size_t c = 0; c < children.Size(); ++c) {
                const std::int64_t child = children[c].AsInt(-1);
                if (child < 0 || static_cast<std::size_t>(child) >= nodes.Size() || static_cast<std::size_t>(child) == i) {
                    error = "node " + std::to_string(i) + " has an invalid child";
                    return false;
                }
                node.children.push_back(static_cast<int>(child));
            }
        }
        for (std::size_t i = 0; i < doc.nodes.size(); ++i) {
            for (int child : doc.nodes[i].children) {
                GltfNode& c = doc.nodes[static_cast<std::size_t>(child)];
                if (c.parent >= 0) {
                    error = "node " + std::to_string(child) + " has two parents";
                    return false;
                }
                c.parent = static_cast<int>(i);
            }
        }
        // Reject cycles (every node must reach a root).
        for (std::size_t i = 0; i < doc.nodes.size(); ++i) {
            int current = static_cast<int>(i);
            std::size_t guard = 0;
            while (current >= 0) {
                current = doc.nodes[static_cast<std::size_t>(current)].parent;
                if (++guard > doc.nodes.size()) {
                    error = "node hierarchy contains a cycle";
                    return false;
                }
            }
        }
        return true;
    }

    bool LoadSkins(std::string& error) {
        const JsonValue& skins = json["skins"];
        for (std::size_t i = 0; i < skins.Size(); ++i) {
            GltfSkin skin;
            skin.name = skins[i].String("name");
            const JsonValue& joints = skins[i]["joints"];
            for (std::size_t j = 0; j < joints.Size(); ++j) {
                const std::int64_t node = joints[j].AsInt(-1);
                if (node < 0 || static_cast<std::size_t>(node) >= doc.nodes.size()) {
                    error = "skin " + std::to_string(i) + " references a missing node";
                    return false;
                }
                skin.joints.push_back(static_cast<int>(node));
            }
            if (skins[i].Has("inverseBindMatrices")) {
                std::vector<float> values;
                std::size_t count = 0;
                if (!ReadAccessor(static_cast<int>(skins[i].Int("inverseBindMatrices", -1)), 16, values, count, error)) return false;
                if (count != skin.joints.size()) {
                    error = "skin " + std::to_string(i) + " has the wrong number of inverse bind matrices";
                    return false;
                }
                for (std::size_t j = 0; j < count; ++j) {
                    Mat4 m = Mat4::Identity();
                    for (int c = 0; c < 4; ++c) {
                        for (int r = 0; r < 4; ++r) m.m[r][c] = values[j * 16 + static_cast<std::size_t>(c * 4 + r)];
                    }
                    skin.inverseBind.push_back(FlipX(m));
                }
            } else {
                skin.inverseBind.assign(skin.joints.size(), Mat4::Identity());
            }
            doc.skins.push_back(std::move(skin));
        }
        return true;
    }

    bool LoadAnimations(std::string& error) {
        const JsonValue& animations = json["animations"];
        for (std::size_t a = 0; a < animations.Size(); ++a) {
            const JsonValue& anim = animations[a];
            GltfAnimation animation;
            animation.name = anim.String("name", "animation" + std::to_string(a));
            const JsonValue& samplers = anim["samplers"];
            const JsonValue& channels = anim["channels"];
            for (std::size_t c = 0; c < channels.Size(); ++c) {
                const JsonValue& target = channels[c]["target"];
                const std::string path = target.String("path");
                if (path == "weights") continue; // morph targets are not supported
                GltfChannel channel;
                const std::int64_t node = target.Int("node", -1);
                if (node < 0 || static_cast<std::size_t>(node) >= doc.nodes.size()) continue; // extensions may target others
                channel.node = static_cast<int>(node);
                if (path == "translation") channel.path = GltfPath::Translation;
                else if (path == "rotation") channel.path = GltfPath::Rotation;
                else if (path == "scale") channel.path = GltfPath::Scale;
                else continue;
                const JsonValue& sampler = samplers[static_cast<std::size_t>(channels[c].Int("sampler", -1))];
                if (!sampler.IsObject()) {
                    error = "animation " + std::to_string(a) + " references a missing sampler";
                    return false;
                }
                const std::string interpolation = sampler.String("interpolation", "LINEAR");
                channel.interpolation = interpolation == "STEP" ? GltfInterpolation::Step
                    : interpolation == "CUBICSPLINE"         ? GltfInterpolation::CubicSpline
                                                             : GltfInterpolation::Linear;
                std::size_t keyCount = 0, valueCount = 0;
                if (!ReadAccessor(static_cast<int>(sampler.Int("input", -1)), 1, channel.times, keyCount, error)) return false;
                for (std::size_t k = 1; k < channel.times.size(); ++k) {
                    if (channel.times[k] < channel.times[k - 1]) {
                        error = "animation " + std::to_string(a) + " has decreasing key times";
                        return false;
                    }
                }
                const int components = channel.path == GltfPath::Rotation ? 4 : 3;
                std::vector<float> values;
                if (!ReadAccessor(static_cast<int>(sampler.Int("output", -1)), components, values, valueCount, error)) return false;
                const std::size_t expected = keyCount * (channel.interpolation == GltfInterpolation::CubicSpline ? 3u : 1u);
                if (valueCount != expected) {
                    error = "animation " + std::to_string(a) + " has mismatched sampler lengths";
                    return false;
                }
                channel.values.resize(valueCount);
                for (std::size_t v = 0; v < valueCount; ++v) {
                    const float* p = &values[v * static_cast<std::size_t>(components)];
                    if (channel.path == GltfPath::Rotation) {
                        const Quat q = FlipX(Quat{p[0], p[1], p[2], p[3]});
                        channel.values[v] = {q.x, q.y, q.z, q.w};
                    } else if (channel.path == GltfPath::Translation) {
                        channel.values[v] = {-p[0], p[1], p[2], 0.0f};
                    } else {
                        channel.values[v] = {p[0], p[1], p[2], 0.0f};
                    }
                }
                if (!channel.times.empty()) animation.channels.push_back(std::move(channel));
            }
            doc.animations.push_back(std::move(animation));
        }
        return true;
    }

    bool LoadScenes() {
        const JsonValue& scenes = json["scenes"];
        for (std::size_t i = 0; i < scenes.Size(); ++i) {
            GltfScene scene;
            scene.name = scenes[i].String("name");
            const JsonValue& nodes = scenes[i]["nodes"];
            for (std::size_t n = 0; n < nodes.Size(); ++n) {
                const std::int64_t node = nodes[n].AsInt(-1);
                if (node >= 0 && static_cast<std::size_t>(node) < doc.nodes.size()) scene.nodes.push_back(static_cast<int>(node));
            }
            doc.scenes.push_back(std::move(scene));
        }
        const std::int64_t scene = json.Int("scene", doc.scenes.empty() ? -1 : 0);
        doc.scene = scene >= 0 && static_cast<std::size_t>(scene) < doc.scenes.size() ? static_cast<int>(scene) : -1;
        return true;
    }
};

// ---------------------------------------------------------------- Export helpers

struct Exporter {
    std::vector<std::uint8_t> bin;
    JsonValue bufferViews = JsonValue::MakeArray();
    JsonValue accessors = JsonValue::MakeArray();

    void Align() {
        while (bin.size() % 4) bin.push_back(0);
    }

    int AddBufferView(const void* data, std::size_t size, int target) {
        Align();
        const std::size_t offset = bin.size();
        const auto* bytes = static_cast<const std::uint8_t*>(data);
        bin.insert(bin.end(), bytes, bytes + size);
        JsonValue view = JsonValue::MakeObject();
        view.Set("buffer", 0);
        view.Set("byteOffset", offset);
        view.Set("byteLength", size);
        if (target) view.Set("target", target);
        bufferViews.Append(std::move(view));
        return static_cast<int>(bufferViews.Size() - 1);
    }

    int AddFloatAccessor(const std::vector<float>& values, int components, const char* type, int target,
        bool withBounds = false) {
        const int view = AddBufferView(values.data(), values.size() * sizeof(float), target);
        JsonValue accessor = JsonValue::MakeObject();
        accessor.Set("bufferView", view);
        accessor.Set("componentType", kFloat);
        accessor.Set("count", values.size() / static_cast<std::size_t>(components));
        accessor.Set("type", type);
        if (withBounds && !values.empty()) {
            JsonValue minimum = JsonValue::MakeArray(), maximum = JsonValue::MakeArray();
            for (int c = 0; c < components; ++c) {
                float lo = std::numeric_limits<float>::max(), hi = -std::numeric_limits<float>::max();
                for (std::size_t i = static_cast<std::size_t>(c); i < values.size(); i += static_cast<std::size_t>(components)) {
                    lo = std::min(lo, values[i]);
                    hi = std::max(hi, values[i]);
                }
                minimum.Append(lo);
                maximum.Append(hi);
            }
            accessor.Set("min", std::move(minimum));
            accessor.Set("max", std::move(maximum));
        }
        accessors.Append(std::move(accessor));
        return static_cast<int>(accessors.Size() - 1);
    }

    template <typename T>
    int AddIntegerAccessor(const std::vector<T>& values, int componentType, int components, const char* type, int target) {
        const int view = AddBufferView(values.data(), values.size() * sizeof(T), target);
        JsonValue accessor = JsonValue::MakeObject();
        accessor.Set("bufferView", view);
        accessor.Set("componentType", componentType);
        accessor.Set("count", values.size() / static_cast<std::size_t>(components));
        accessor.Set("type", type);
        accessors.Append(std::move(accessor));
        return static_cast<int>(accessors.Size() - 1);
    }
};

JsonValue ArrayOf(std::initializer_list<float> values) {
    JsonValue array = JsonValue::MakeArray();
    for (float v : values) array.Append(v);
    return array;
}

// Samples one channel at `time` (engine-space value).
Vec4 SampleChannel(const GltfChannel& channel, float time) {
    const std::vector<float>& t = channel.times;
    const bool cubic = channel.interpolation == GltfInterpolation::CubicSpline;
    auto valueAt = [&](std::size_t key) { return cubic ? channel.values[key * 3 + 1] : channel.values[key]; };
    if (t.empty()) return {};
    if (time <= t.front()) return valueAt(0);
    if (time >= t.back()) return valueAt(t.size() - 1);
    const std::size_t next = static_cast<std::size_t>(std::upper_bound(t.begin(), t.end(), time) - t.begin());
    const std::size_t prev = next - 1;
    const float span = t[next] - t[prev];
    const float u = span > 0.0f ? (time - t[prev]) / span : 0.0f;
    const bool rotation = channel.path == GltfPath::Rotation;
    switch (channel.interpolation) {
    case GltfInterpolation::Step: return valueAt(prev);
    case GltfInterpolation::Linear: {
        const Vec4 a = valueAt(prev), b = valueAt(next);
        if (rotation) {
            const Quat q = Math::Slerp(Quat{a.x, a.y, a.z, a.w}, Quat{b.x, b.y, b.z, b.w}, u);
            return {q.x, q.y, q.z, q.w};
        }
        return Math::Lerp(a, b, u);
    }
    case GltfInterpolation::CubicSpline: {
        // Hermite: p(u) = (2u^3-3u^2+1)p0 + (u^3-2u^2+u)*span*m0 + (-2u^3+3u^2)p1 + (u^3-u^2)*span*m1
        const Vec4 p0 = channel.values[prev * 3 + 1], m0 = channel.values[prev * 3 + 2];
        const Vec4 p1 = channel.values[next * 3 + 1], m1 = channel.values[next * 3];
        const float u2 = u * u, u3 = u2 * u;
        Vec4 v = p0 * (2 * u3 - 3 * u2 + 1) + m0 * ((u3 - 2 * u2 + u) * span) + p1 * (-2 * u3 + 3 * u2) + m1 * ((u3 - u2) * span);
        if (rotation) {
            const Quat q = Math::Normalize(Quat{v.x, v.y, v.z, v.w});
            v = {q.x, q.y, q.z, q.w};
        }
        return v;
    }
    }
    return {};
}

} // namespace

// ---------------------------------------------------------------- GltfDocument

float GltfAnimation::Duration() const {
    float duration = 0.0f;
    for (const GltfChannel& channel : channels) {
        if (!channel.times.empty()) duration = std::max(duration, channel.times.back());
    }
    return duration;
}

std::vector<int> GltfDocument::RootNodes() const {
    if (scene >= 0 && static_cast<std::size_t>(scene) < scenes.size()) return scenes[static_cast<std::size_t>(scene)].nodes;
    std::vector<int> roots;
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].parent < 0) roots.push_back(static_cast<int>(i));
    }
    return roots;
}

Mat4 GltfDocument::WorldMatrix(int node) const {
    Mat4 world = Mat4::Identity();
    std::size_t guard = 0;
    while (node >= 0 && static_cast<std::size_t>(node) < nodes.size() && guard++ <= nodes.size()) {
        world = Math::ToMat4(nodes[static_cast<std::size_t>(node)].local) * world;
        node = nodes[static_cast<std::size_t>(node)].parent;
    }
    return world;
}

TRS GltfDocument::WorldTrs(int node) const {
    TRS world;
    std::size_t guard = 0;
    while (node >= 0 && static_cast<std::size_t>(node) < nodes.size() && guard++ <= nodes.size()) {
        world = Math::Combine(nodes[static_cast<std::size_t>(node)].local, world);
        node = nodes[static_cast<std::size_t>(node)].parent;
    }
    return world;
}

// ---------------------------------------------------------------- Import

bool ImportGltfFromMemory(const std::uint8_t* data, std::size_t size, const std::string& baseDirectory,
    GltfDocument& out, std::string& error, const GltfImportOptions& options) {
    if (!data || size == 0) {
        error = "empty glTF";
        return false;
    }
    if (size > options.maxFileBytes) {
        error = "glTF exceeds the size limit";
        return false;
    }
    std::string_view jsonText;
    std::vector<std::uint8_t> glbBin;
    bool haveBin = false;
    if (size >= 12 && ReadU32(data) == kGlbMagic) {
        const std::uint32_t version = ReadU32(data + 4);
        const std::uint32_t length = ReadU32(data + 8);
        if (version != 2 || length > size || length < 20) {
            error = "unsupported or truncated GLB header";
            return false;
        }
        std::size_t offset = 12;
        while (offset + 8 <= length) {
            const std::uint32_t chunkLength = ReadU32(data + offset);
            const std::uint32_t chunkType = ReadU32(data + offset + 4);
            offset += 8;
            if (chunkLength > length - offset) {
                error = "GLB chunk overruns the file";
                return false;
            }
            if (chunkType == kJsonChunk && jsonText.empty()) {
                jsonText = std::string_view(reinterpret_cast<const char*>(data + offset), chunkLength);
            } else if (chunkType == kBinChunk && !haveBin) {
                glbBin.assign(data + offset, data + offset + chunkLength);
                haveBin = true;
            }
            offset += (chunkLength + 3u) & ~3u;
        }
        if (jsonText.empty()) {
            error = "GLB has no JSON chunk";
            return false;
        }
    } else {
        jsonText = std::string_view(reinterpret_cast<const char*>(data), size);
    }
    JsonValue json;
    Core::JsonParseOptions parseOptions;
    parseOptions.allowComments = false;
    parseOptions.maxDepth = 64;
    if (!Core::ParseJson(jsonText, json, error, parseOptions)) {
        error = "glTF JSON: " + error;
        return false;
    }
    const std::string version = json["asset"].String("version");
    if (version.empty() || version[0] != '2') {
        error = "not a glTF 2.x asset (asset.version '" + version + "')";
        return false;
    }
    const JsonValue& required = json["extensionsRequired"];
    for (std::size_t i = 0; i < required.Size(); ++i) {
        const std::string name = required[i].AsString();
        if (name != "KHR_materials_unlit" && name != "KHR_materials_emissive_strength") {
            error = "required extension " + name + " is not supported";
            return false;
        }
    }
    GltfDocument doc;
    doc.generator = json["asset"].String("generator");
    Importer importer{json, baseDirectory, options, {}, doc};
    if (!importer.LoadBuffers(haveBin ? &glbBin : nullptr, error) || !importer.LoadImages(error)) return false;
    importer.LoadTexturesAndMaterials();
    if (!importer.LoadMeshes(error) || !importer.LoadNodes(error) || !importer.LoadSkins(error)
        || !importer.LoadAnimations(error) || !importer.LoadScenes()) {
        return false;
    }
    out = std::move(doc);
    return true;
}

bool ImportGltf(const std::string& path, GltfDocument& out, std::string& error, const GltfImportOptions& options) {
    std::vector<std::uint8_t> bytes;
    if (!ReadFile(path, options.maxFileBytes, bytes, error)) return false;
    const std::size_t slash = path.find_last_of("/\\");
    const std::string base = slash == std::string::npos ? "." : path.substr(0, slash);
    if (!ImportGltfFromMemory(bytes.data(), bytes.size(), base, out, error, options)) {
        error = path + ": " + error;
        return false;
    }
    return true;
}

// ---------------------------------------------------------------- Export

std::vector<std::uint8_t> ExportGltfToMemory(const GltfDocument& doc, GltfContainer container, std::string& error) {
    Exporter ex;
    JsonValue root = JsonValue::MakeObject();
    JsonValue asset = JsonValue::MakeObject();
    asset.Set("version", "2.0");
    asset.Set("generator", doc.generator.empty() ? "Astral Engine" : doc.generator);
    root.Set("asset", std::move(asset));

    // Images are embedded as PNG.
    JsonValue images = JsonValue::MakeArray();
    for (const GltfImage& image : doc.images) {
        JsonValue entry = JsonValue::MakeObject();
        if (!image.name.empty()) entry.Set("name", image.name);
        const std::vector<std::uint8_t> png = Graphics::EncodePng(image.image, true);
        if (container == GltfContainer::Binary) {
            entry.Set("bufferView", ex.AddBufferView(png.data(), png.size(), 0));
            entry.Set("mimeType", "image/png");
        } else {
            entry.Set("uri", "data:image/png;base64," + Core::Base64Encode(png.data(), png.size()));
        }
        images.Append(std::move(entry));
    }
    JsonValue textures = JsonValue::MakeArray();
    JsonValue samplers = JsonValue::MakeArray();
    bool needClampSampler = false;
    for (const GltfTexture& texture : doc.textures) needClampSampler = needClampSampler || texture.clamp;
    if (needClampSampler) {
        JsonValue clamp = JsonValue::MakeObject();
        clamp.Set("wrapS", 33071);
        clamp.Set("wrapT", 33071);
        samplers.Append(std::move(clamp));
    }
    for (const GltfTexture& texture : doc.textures) {
        JsonValue entry = JsonValue::MakeObject();
        if (!texture.name.empty()) entry.Set("name", texture.name);
        if (texture.image >= 0) entry.Set("source", texture.image);
        if (texture.clamp) entry.Set("sampler", 0);
        textures.Append(std::move(entry));
    }
    JsonValue materials = JsonValue::MakeArray();
    bool usesUnlit = false;
    for (const GltfMaterial& m : doc.materials) {
        JsonValue entry = JsonValue::MakeObject();
        if (!m.name.empty()) entry.Set("name", m.name);
        JsonValue pbr = JsonValue::MakeObject();
        pbr.Set("baseColorFactor", ArrayOf({m.baseColorFactor.x, m.baseColorFactor.y, m.baseColorFactor.z, m.baseColorFactor.w}));
        if (m.baseColorTexture >= 0) {
            JsonValue info = JsonValue::MakeObject();
            info.Set("index", m.baseColorTexture);
            pbr.Set("baseColorTexture", std::move(info));
        }
        pbr.Set("metallicFactor", m.metallicFactor);
        pbr.Set("roughnessFactor", m.roughnessFactor);
        entry.Set("pbrMetallicRoughness", std::move(pbr));
        if (m.emissiveFactor.x > 0 || m.emissiveFactor.y > 0 || m.emissiveFactor.z > 0) {
            const float peak = std::max(1.0f, Math::MaxComponent(m.emissiveFactor));
            entry.Set("emissiveFactor", ArrayOf({m.emissiveFactor.x / peak, m.emissiveFactor.y / peak, m.emissiveFactor.z / peak}));
            if (peak > 1.0f) {
                JsonValue strength = JsonValue::MakeObject();
                strength.Set("emissiveStrength", peak);
                JsonValue extensions = entry.Has("extensions") ? *entry.Find("extensions") : JsonValue::MakeObject();
                extensions.Set("KHR_materials_emissive_strength", std::move(strength));
                entry.Set("extensions", std::move(extensions));
            }
        }
        if (m.emissiveTexture >= 0) {
            JsonValue info = JsonValue::MakeObject();
            info.Set("index", m.emissiveTexture);
            entry.Set("emissiveTexture", std::move(info));
        }
        if (m.alphaMode != GltfAlphaMode::Opaque) {
            entry.Set("alphaMode", m.alphaMode == GltfAlphaMode::Blend ? "BLEND" : "MASK");
            if (m.alphaMode == GltfAlphaMode::Mask) entry.Set("alphaCutoff", m.alphaCutoff);
        }
        if (m.doubleSided) entry.Set("doubleSided", true);
        if (m.unlit) {
            usesUnlit = true;
            JsonValue extensions = entry.Has("extensions") ? *entry.Find("extensions") : JsonValue::MakeObject();
            extensions.Set("KHR_materials_unlit", JsonValue::MakeObject());
            entry.Set("extensions", std::move(extensions));
        }
        materials.Append(std::move(entry));
    }

    JsonValue meshes = JsonValue::MakeArray();
    for (const GltfMesh& mesh : doc.meshes) {
        JsonValue entry = JsonValue::MakeObject();
        if (!mesh.name.empty()) entry.Set("name", mesh.name);
        JsonValue primitives = JsonValue::MakeArray();
        for (const GltfPrimitive& primitive : mesh.primitives) {
            const Graphics::MeshData& m = primitive.mesh;
            std::string validation;
            if (!m.Validate(validation)) {
                error = "mesh '" + mesh.name + "': " + validation;
                return {};
            }
            std::vector<float> positions, normals, uvs, colors, weights;
            std::vector<std::uint16_t> joints;
            for (const Graphics::Vertex& v : m.vertices) {
                const Vec3 p = FlipX(v.position), n = FlipX(v.normal);
                positions.insert(positions.end(), {p.x, p.y, p.z});
                normals.insert(normals.end(), {n.x, n.y, n.z});
                uvs.insert(uvs.end(), {v.uv.x, v.uv.y});
                colors.insert(colors.end(), {v.color.x, v.color.y, v.color.z, v.color.w});
            }
            JsonValue attributes = JsonValue::MakeObject();
            attributes.Set("POSITION", ex.AddFloatAccessor(positions, 3, "VEC3", kArrayBuffer, true));
            attributes.Set("NORMAL", ex.AddFloatAccessor(normals, 3, "VEC3", kArrayBuffer));
            attributes.Set("TEXCOORD_0", ex.AddFloatAccessor(uvs, 2, "VEC2", kArrayBuffer));
            attributes.Set("COLOR_0", ex.AddFloatAccessor(colors, 4, "VEC4", kArrayBuffer));
            if (m.skin.size() == m.vertices.size() && !m.skin.empty()) {
                for (const Graphics::SkinInfluence& influence : m.skin) {
                    joints.insert(joints.end(), influence.joints.begin(), influence.joints.end());
                    weights.insert(weights.end(), influence.weights.begin(), influence.weights.end());
                }
                attributes.Set("JOINTS_0", ex.AddIntegerAccessor(joints, kUnsignedShort, 4, "VEC4", kArrayBuffer));
                attributes.Set("WEIGHTS_0", ex.AddFloatAccessor(weights, 4, "VEC4", kArrayBuffer));
            }
            std::vector<std::uint32_t> indices;
            indices.reserve(m.indices.size());
            for (std::size_t t = 0; t + 2 < m.indices.size(); t += 3) {
                indices.insert(indices.end(), {m.indices[t], m.indices[t + 2], m.indices[t + 1]}); // back to CCW
            }
            JsonValue prim = JsonValue::MakeObject();
            prim.Set("attributes", std::move(attributes));
            prim.Set("indices", ex.AddIntegerAccessor(indices, kUnsignedInt, 1, "SCALAR", kElementArrayBuffer));
            if (primitive.material >= 0) prim.Set("material", primitive.material);
            primitives.Append(std::move(prim));
        }
        entry.Set("primitives", std::move(primitives));
        meshes.Append(std::move(entry));
    }

    JsonValue nodes = JsonValue::MakeArray();
    for (const GltfNode& node : doc.nodes) {
        JsonValue entry = JsonValue::MakeObject();
        if (!node.name.empty()) entry.Set("name", node.name);
        const TRS local = FlipX(node.local);
        if (Math::LengthSquared(local.translation) > 0.0f) {
            entry.Set("translation", ArrayOf({local.translation.x, local.translation.y, local.translation.z}));
        }
        const Quat& q = local.rotation;
        if (q.x != 0.0f || q.y != 0.0f || q.z != 0.0f || q.w != 1.0f) entry.Set("rotation", ArrayOf({q.x, q.y, q.z, q.w}));
        if (local.scale.x != 1.0f || local.scale.y != 1.0f || local.scale.z != 1.0f) {
            entry.Set("scale", ArrayOf({local.scale.x, local.scale.y, local.scale.z}));
        }
        if (node.mesh >= 0) entry.Set("mesh", node.mesh);
        if (node.skin >= 0) entry.Set("skin", node.skin);
        if (!node.children.empty()) {
            JsonValue children = JsonValue::MakeArray();
            for (int child : node.children) children.Append(child);
            entry.Set("children", std::move(children));
        }
        nodes.Append(std::move(entry));
    }

    JsonValue skins = JsonValue::MakeArray();
    for (const GltfSkin& skin : doc.skins) {
        JsonValue entry = JsonValue::MakeObject();
        if (!skin.name.empty()) entry.Set("name", skin.name);
        JsonValue joints = JsonValue::MakeArray();
        for (int joint : skin.joints) joints.Append(joint);
        entry.Set("joints", std::move(joints));
        std::vector<float> matrices;
        for (const Mat4& matrix : skin.inverseBind) {
            const Mat4 m = FlipX(matrix);
            for (int c = 0; c < 4; ++c) {
                for (int r = 0; r < 4; ++r) matrices.push_back(m.m[r][c]);
            }
        }
        if (!matrices.empty()) entry.Set("inverseBindMatrices", ex.AddFloatAccessor(matrices, 16, "MAT4", 0));
        skins.Append(std::move(entry));
    }

    JsonValue animations = JsonValue::MakeArray();
    for (const GltfAnimation& animation : doc.animations) {
        JsonValue entry = JsonValue::MakeObject();
        if (!animation.name.empty()) entry.Set("name", animation.name);
        JsonValue channelList = JsonValue::MakeArray(), samplerList = JsonValue::MakeArray();
        for (const GltfChannel& channel : animation.channels) {
            std::vector<float> values;
            const int components = channel.path == GltfPath::Rotation ? 4 : 3;
            for (const Vec4& v : channel.values) {
                if (channel.path == GltfPath::Rotation) {
                    const Quat q = FlipX(Quat{v.x, v.y, v.z, v.w});
                    values.insert(values.end(), {q.x, q.y, q.z, q.w});
                } else if (channel.path == GltfPath::Translation) {
                    values.insert(values.end(), {-v.x, v.y, v.z});
                } else {
                    values.insert(values.end(), {v.x, v.y, v.z});
                }
            }
            JsonValue sampler = JsonValue::MakeObject();
            sampler.Set("input", ex.AddFloatAccessor(channel.times, 1, "SCALAR", 0, true));
            sampler.Set("output", ex.AddFloatAccessor(values, components, components == 4 ? "VEC4" : "VEC3", 0));
            sampler.Set("interpolation", channel.interpolation == GltfInterpolation::Step ? "STEP"
                    : channel.interpolation == GltfInterpolation::CubicSpline               ? "CUBICSPLINE"
                                                                                            : "LINEAR");
            samplerList.Append(std::move(sampler));
            JsonValue target = JsonValue::MakeObject();
            target.Set("node", channel.node);
            target.Set("path", channel.path == GltfPath::Rotation ? "rotation"
                    : channel.path == GltfPath::Scale              ? "scale"
                                                                   : "translation");
            JsonValue ch = JsonValue::MakeObject();
            ch.Set("sampler", samplerList.Size() - 1);
            ch.Set("target", std::move(target));
            channelList.Append(std::move(ch));
        }
        entry.Set("channels", std::move(channelList));
        entry.Set("samplers", std::move(samplerList));
        animations.Append(std::move(entry));
    }

    JsonValue scenes = JsonValue::MakeArray();
    const std::vector<GltfScene> sceneList = doc.scenes.empty() ? std::vector<GltfScene>{GltfScene{"Scene", doc.RootNodes()}} : doc.scenes;
    for (const GltfScene& scene : sceneList) {
        JsonValue entry = JsonValue::MakeObject();
        if (!scene.name.empty()) entry.Set("name", scene.name);
        JsonValue list = JsonValue::MakeArray();
        for (int node : scene.nodes) list.Append(node);
        entry.Set("nodes", std::move(list));
        scenes.Append(std::move(entry));
    }
    root.Set("scene", doc.scene >= 0 ? doc.scene : 0);
    root.Set("scenes", std::move(scenes));
    if (nodes.Size()) root.Set("nodes", std::move(nodes));
    if (meshes.Size()) root.Set("meshes", std::move(meshes));
    if (materials.Size()) root.Set("materials", std::move(materials));
    if (textures.Size()) root.Set("textures", std::move(textures));
    if (samplers.Size()) root.Set("samplers", std::move(samplers));
    if (images.Size()) root.Set("images", std::move(images));
    if (skins.Size()) root.Set("skins", std::move(skins));
    if (animations.Size()) root.Set("animations", std::move(animations));
    if (usesUnlit) {
        JsonValue used = JsonValue::MakeArray();
        used.Append("KHR_materials_unlit");
        root.Set("extensionsUsed", std::move(used));
    }
    ex.Align();
    if (!ex.bin.empty()) {
        JsonValue buffer = JsonValue::MakeObject();
        buffer.Set("byteLength", ex.bin.size());
        if (container == GltfContainer::Embedded) {
            buffer.Set("uri", "data:application/octet-stream;base64," + Core::Base64Encode(ex.bin.data(), ex.bin.size()));
        }
        JsonValue buffers = JsonValue::MakeArray();
        buffers.Append(std::move(buffer));
        root.Set("buffers", std::move(buffers));
        root.Set("bufferViews", std::move(ex.bufferViews));
        root.Set("accessors", std::move(ex.accessors));
    }
    Core::JsonWriteOptions writeOptions;
    writeOptions.pretty = container == GltfContainer::Embedded;
    std::string text = Core::WriteJson(root, writeOptions);
    if (container == GltfContainer::Embedded) return std::vector<std::uint8_t>(text.begin(), text.end());

    while (text.size() % 4) text.push_back(' ');
    std::vector<std::uint8_t> glb;
    const std::size_t total = 12 + 8 + text.size() + (ex.bin.empty() ? 0 : 8 + ex.bin.size());
    WriteU32(glb, kGlbMagic);
    WriteU32(glb, 2);
    WriteU32(glb, static_cast<std::uint32_t>(total));
    WriteU32(glb, static_cast<std::uint32_t>(text.size()));
    WriteU32(glb, kJsonChunk);
    glb.insert(glb.end(), text.begin(), text.end());
    if (!ex.bin.empty()) {
        WriteU32(glb, static_cast<std::uint32_t>(ex.bin.size()));
        WriteU32(glb, kBinChunk);
        glb.insert(glb.end(), ex.bin.begin(), ex.bin.end());
    }
    return glb;
}

bool ExportGltf(const GltfDocument& doc, const std::string& path, GltfContainer container, std::string& error) {
    const std::vector<std::uint8_t> bytes = ExportGltfToMemory(doc, container, error);
    if (bytes.empty()) return false;
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file || !file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()))) {
        error = "cannot write " + path;
        return false;
    }
    return true;
}

// ---------------------------------------------------------------- Engine conversion

bool BuildSkeleton(const GltfDocument& doc, int skinIndex, GltfSkeletonBinding& out, std::string& error) {
    if (skinIndex < 0 || static_cast<std::size_t>(skinIndex) >= doc.skins.size()) {
        error = "skin index out of range";
        return false;
    }
    const GltfSkin& skin = doc.skins[static_cast<std::size_t>(skinIndex)];
    GltfSkeletonBinding binding;
    binding.nodeToJoint.assign(doc.nodes.size(), -1);
    std::vector<char> isJoint(doc.nodes.size(), 0);
    for (int node : skin.joints) isJoint[static_cast<std::size_t>(node)] = 1;
    // Depth of each joint node (parents always shallower) gives a parent-first order.
    auto depthOf = [&](int node) {
        int depth = 0;
        for (int p = doc.nodes[static_cast<std::size_t>(node)].parent; p >= 0; p = doc.nodes[static_cast<std::size_t>(p)].parent) ++depth;
        return depth;
    };
    std::vector<int> order(skin.joints.size());
    for (std::size_t i = 0; i < order.size(); ++i) order[i] = static_cast<int>(i);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        return depthOf(skin.joints[static_cast<std::size_t>(a)]) < depthOf(skin.joints[static_cast<std::size_t>(b)]);
    });
    std::vector<int> skinSlotToJoint(skin.joints.size(), -1);
    std::vector<Mat4> inverseBind(skin.joints.size());
    for (int slot : order) {
        const int node = skin.joints[static_cast<std::size_t>(slot)];
        if (binding.nodeToJoint[static_cast<std::size_t>(node)] >= 0) {
            error = "skin lists a joint twice";
            return false;
        }
        // Nearest joint ancestor; transforms of non-joint nodes in between fold into a prefix.
        TRS prefix;
        int parentJoint = -1;
        for (int p = doc.nodes[static_cast<std::size_t>(node)].parent; p >= 0; p = doc.nodes[static_cast<std::size_t>(p)].parent) {
            if (isJoint[static_cast<std::size_t>(p)]) {
                parentJoint = binding.nodeToJoint[static_cast<std::size_t>(p)];
                break;
            }
            prefix = Math::Combine(doc.nodes[static_cast<std::size_t>(p)].local, prefix);
        }
        const TRS bindLocal = Math::Combine(prefix, doc.nodes[static_cast<std::size_t>(node)].local);
        const std::string name = doc.nodes[static_cast<std::size_t>(node)].name.empty()
            ? "joint" + std::to_string(slot) : doc.nodes[static_cast<std::size_t>(node)].name;
        const int joint = binding.skeleton.AddJoint(name, parentJoint, bindLocal);
        if (joint < 0) {
            error = "could not order skin joints parent-first";
            return false;
        }
        binding.nodeToJoint[static_cast<std::size_t>(node)] = joint;
        binding.jointToNode.push_back(node);
        binding.prefix.push_back(prefix);
        skinSlotToJoint[static_cast<std::size_t>(slot)] = joint;
        inverseBind[static_cast<std::size_t>(joint)] = skin.inverseBind[static_cast<std::size_t>(slot)];
    }
    binding.skeleton.ComputeBindMatrices();
    if (!binding.skeleton.SetInverseBindMatrices(std::move(inverseBind))) {
        error = "skin has a singular inverse bind matrix";
        return false;
    }
    out = std::move(binding);
    return true;
}

Animation::AnimationClip BuildClip(const GltfDocument& doc, int animationIndex, const GltfSkeletonBinding& binding,
    float resampleRate) {
    Animation::AnimationClip clip;
    if (animationIndex < 0 || static_cast<std::size_t>(animationIndex) >= doc.animations.size()) return clip;
    const GltfAnimation& animation = doc.animations[static_cast<std::size_t>(animationIndex)];
    clip.name = animation.name;
    clip.duration = std::max(animation.Duration(), 1.0e-3f);
    resampleRate = std::max(1.0f, resampleRate);
    for (int joint = 0; joint < binding.skeleton.JointCount(); ++joint) {
        const int node = binding.jointToNode[static_cast<std::size_t>(joint)];
        const GltfChannel* channels[3] = {nullptr, nullptr, nullptr};
        for (const GltfChannel& channel : animation.channels) {
            if (channel.node == node) channels[static_cast<int>(channel.path)] = &channel;
        }
        if (!channels[0] && !channels[1] && !channels[2]) continue;
        // Union of key times (plus resampling for splines and duplicated keys for steps).
        std::vector<float> times;
        for (const GltfChannel* channel : channels) {
            if (!channel) continue;
            times.insert(times.end(), channel->times.begin(), channel->times.end());
            if (channel->interpolation == GltfInterpolation::CubicSpline) {
                for (float t = channel->times.front(); t < channel->times.back(); t += 1.0f / resampleRate) times.push_back(t);
            } else if (channel->interpolation == GltfInterpolation::Step) {
                for (std::size_t k = 1; k < channel->times.size(); ++k) {
                    times.push_back(std::max(channel->times[k - 1], channel->times[k] - 1.0e-4f));
                }
            }
        }
        // glTF clamps outside a channel's keys; pin every track to [0, duration]
        // so the engine's loop-seam blend never mixes in values glTF would hold.
        times.push_back(0.0f);
        times.push_back(clip.duration);
        std::sort(times.begin(), times.end());
        times.erase(std::unique(times.begin(), times.end(), [](float a, float b) { return std::fabs(a - b) < 1.0e-6f; }), times.end());
        const TRS rest = doc.nodes[static_cast<std::size_t>(node)].local;
        const TRS& prefix = binding.prefix[static_cast<std::size_t>(joint)];
        for (float t : times) {
            TRS local = rest;
            if (channels[0]) {
                const Vec4 v = SampleChannel(*channels[0], t);
                local.translation = {v.x, v.y, v.z};
            }
            if (channels[1]) {
                const Vec4 v = SampleChannel(*channels[1], t);
                local.rotation = Math::Normalize(Quat{v.x, v.y, v.z, v.w});
            }
            if (channels[2]) {
                const Vec4 v = SampleChannel(*channels[2], t);
                local.scale = {v.x, v.y, v.z};
            }
            clip.AddKey(joint, Math::Clamp(t, 0.0f, clip.duration), Math::Combine(prefix, local));
        }
    }
    return clip;
}

Graphics::Material BuildMaterial(const GltfMaterial& m, const Graphics::Texture2D* baseTexture, Graphics::ShadingModel shading) {
    Graphics::Material material;
    material.name = m.name;
    material.shading = m.unlit ? Graphics::ShadingModel::Unlit : shading;
    material.baseColor = {m.baseColorFactor.x, m.baseColorFactor.y, m.baseColorFactor.z};
    material.opacity = m.baseColorFactor.w;
    material.blend = m.alphaMode == GltfAlphaMode::Blend ? Graphics::BlendMode::AlphaBlend : Graphics::BlendMode::Opaque;
    material.baseTexture = baseTexture;
    material.emissive = m.emissiveFactor;
    material.metallic = m.metallicFactor;
    material.roughness = std::max(0.04f, m.roughnessFactor);
    material.doubleSided = m.doubleSided;
    material.vertexColor = true;
    return material;
}

} // namespace Astral::Assets
