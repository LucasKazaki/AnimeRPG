#include "Engine/Graphics/D3D11/D3D11Renderer.h"
#include "Engine/Graphics/D3D11/BuiltinShaders.h"
#include "Engine/Graphics/D3D11/ShaderCompiler.h"

#include <d3d11.h>
#include <d3d11sdklayers.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstring>
#include <deque>
#include <exception>
#include <iomanip>
#include <iterator>
#include <limits>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <utility>

namespace Astral::Graphics::D3D11 {

namespace {
using Microsoft::WRL::ComPtr;
using Clock = std::chrono::steady_clock;

double Milliseconds(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}
std::string Hex(HRESULT hr) {
    std::ostringstream stream;
    stream << "0x" << std::hex << static_cast<unsigned long>(hr);
    return stream.str();
}
std::string Utf8(const wchar_t* text) {
    const int count = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (count <= 1) return {};
    std::string result(static_cast<std::size_t>(count), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), count, nullptr, nullptr);
    result.pop_back();
    return result;
}
bool Finite(const Math::Mat4& matrix) {
    for (const auto& row : matrix.m)
        for (float value : row)
            if (!std::isfinite(value)) return false;
    return true;
}
bool Finite(Math::Vec4 value) {
    return std::isfinite(value.x) && std::isfinite(value.y)
        && std::isfinite(value.z) && std::isfinite(value.w);
}
bool AddBytes(std::size_t amount, std::size_t limit, std::size_t& used) {
    if (used > limit || amount > limit - used) return false;
    used += amount;
    return true;
}
bool Bytes(std::size_t count, std::size_t stride, std::size_t& result) {
    if (count > std::numeric_limits<std::size_t>::max() / stride) return false;
    result = count * stride;
    return true;
}
const char* ShadingLabel(ShadingModel shading) {
    switch (shading) {
    case ShadingModel::Toon: return "Toon";
    case ShadingModel::Lit: return "Lit";
    case ShadingModel::Unlit: return "Unlit";
    case ShadingModel::Water: return "Water";
    default: return "Unknown";
    }
}
const char* BlendLabel(BlendMode blend) {
    switch (blend) {
    case BlendMode::Opaque: return "Opaque";
    case BlendMode::AlphaBlend: return "AlphaBlend";
    case BlendMode::Additive: return "Additive";
    default: return "Unknown";
    }
}

struct alignas(16) Constants {
    Math::Mat4 clip;
    Math::Vec4 baseTint;
    Math::Vec4 emissive;
    Math::Vec4 uvSize;
    std::uint32_t flags[4]{};
};
static_assert(sizeof(Constants) == 128, "HLSL constant buffer packing");
static_assert(sizeof(Math::Vec4) == 16, "float texture layout");

struct MeshUpload {
    const MeshData* source{};
    UINT vertexBytes{}, indexBytes{};
    ComPtr<ID3D11Buffer> vertices, indices;
};
struct TextureUpload {
    const Texture2D* source{};
    ComPtr<ID3D11Texture2D> texture;
    ComPtr<ID3D11ShaderResourceView> view;
};
struct PreparedDraw {
    Constants constants{};
    std::size_t mesh{}, texture{};
    bool textured{}, doubleSided{};
};
struct Targets {
    ComPtr<ID3D11Texture2D> color, ids, depth, colorRead, idsRead;
    ComPtr<ID3D11RenderTargetView> colorView, idView;
    ComPtr<ID3D11DepthStencilView> depthView;
    ComPtr<ID3D11ShaderResourceView> colorResource;
};

struct Submission {
    std::vector<MeshUpload> meshes;
    std::vector<TextureUpload> textures;
    ComPtr<ID3D11Query> disjoint, begin, uploaded, sceneEnd, end, complete;
    FrameStats stats;
    std::size_t payloadBytes{}, payloadObjects{};
};
} // namespace

struct D3D11Renderer::Impl {
    Limits limits;
    DeviceInfo info;
    std::thread::id owner{std::this_thread::get_id()};
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<ID3D11VertexShader> vertexShader;
    ComPtr<ID3D11PixelShader> pixelShader;
    ComPtr<ID3D11InputLayout> inputLayout;
    ComPtr<ID3D11Buffer> constants;
    ComPtr<ID3D11RasterizerState> cullBack, cullNone;
    ComPtr<ID3D11DepthStencilState> depthState;
    ComPtr<ID3D11SamplerState> repeatSampler, clampSampler;
    HWND nativeWindow{};
    int nativeWidth{}, nativeHeight{};
    ComPtr<IDXGISwapChain1> swapChain;
    ComPtr<ID3D11RenderTargetView> backbufferView;
    Targets nativeTargets;
    ComPtr<ID3D11Texture2D> uiTexture;
    ComPtr<ID3D11ShaderResourceView> uiResource;
    ComPtr<ID3D11VertexShader> displayVertex;
    ComPtr<ID3D11PixelShader> displayPixel;
    ComPtr<ID3D11Buffer> displayConstants;
    ComPtr<ID3D11DepthStencilState> displayDepth;
    std::deque<Submission> inFlight;
    std::vector<FrameStats> completedFrames;
    std::uint64_t nextFrameId{1};

    bool Thread(std::string& error) const {
        if (owner == std::this_thread::get_id()) return true;
        error = "D3D11 renderer called outside its owner thread";
        return false;
    }
    bool Check(HRESULT hr, const char* operation, std::string& error) {
        if (SUCCEEDED(hr)) return true;
        const HRESULT reason = device ? device->GetDeviceRemovedReason() : S_OK;
        error = std::string(operation) + " HRESULT=" + Hex(hr) + " removalReason=" + Hex(reason);
        if (FAILED(reason)) {
            // Drop every device-dependent object, including context bindings.
            Reset();
        }
        return false;
    }
    void Reset() {
        if (context) { context->ClearState(); context->Flush(); }
        ResetPresentation();
        clampSampler.Reset(); repeatSampler.Reset(); depthState.Reset();
        cullNone.Reset(); cullBack.Reset(); constants.Reset(); inputLayout.Reset();
        pixelShader.Reset(); vertexShader.Reset(); context.Reset(); device.Reset(); adapter.Reset();
    }
    void ResetPresentation() {
        inFlight.clear(); completedFrames.clear(); nativeTargets = {};
        displayDepth.Reset(); displayConstants.Reset(); displayPixel.Reset(); displayVertex.Reset();
        uiResource.Reset(); uiTexture.Reset(); backbufferView.Reset(); swapChain.Reset();
        nativeWindow = nullptr; nativeWidth = nativeHeight = 0;
    }
    bool Format(DXGI_FORMAT format, UINT required, std::string& error) {
        UINT support = 0;
        if (!Check(device->CheckFormatSupport(format, &support), "CheckFormatSupport", error)) return false;
        if ((support & required) == required) return true;
        error = "Required D3D11 format capabilities unavailable for format " + std::to_string(format);
        return false;
    }
    bool Create(std::string& error, bool debug) {
        ComPtr<IDXGIFactory1> factory;
        if (!Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)), "CreateDXGIFactory1", error)) return false;
        HRESULT creation = E_FAIL;
        bool debugMissing = false;
        // Explicitly exclude software adapters; never fall back to WARP.
        for (UINT index = 0;; ++index) {
            ComPtr<IDXGIAdapter1> candidate;
            const HRESULT enumeration = factory->EnumAdapters1(index, &candidate);
            if (enumeration == DXGI_ERROR_NOT_FOUND) break;
            if (!Check(enumeration, "EnumAdapters1", error)) return false;
            DXGI_ADAPTER_DESC1 description{};
            if (!Check(candidate->GetDesc1(&description), "GetDesc1", error)) return false;
            if (description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
            const D3D_FEATURE_LEVEL requested = D3D_FEATURE_LEVEL_11_0;
            D3D_FEATURE_LEVEL actual{};
            UINT flags = debug ? D3D11_CREATE_DEVICE_DEBUG : 0;
            creation = D3D11CreateDevice(candidate.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags,
                &requested, 1, D3D11_SDK_VERSION, &device, &actual, &context);
            if (creation == DXGI_ERROR_SDK_COMPONENT_MISSING && debug) {
                device.Reset(); context.Reset();
                debugMissing = true;
                flags = 0;
                creation = D3D11CreateDevice(candidate.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags,
                    &requested, 1, D3D11_SDK_VERSION, &device, &actual, &context);
            }
            if (FAILED(creation)) { device.Reset(); context.Reset(); continue; }
            adapter = candidate;
            info.adapter = Utf8(description.Description);
            info.vendorId = description.VendorId; info.deviceId = description.DeviceId;
            info.featureLevel = static_cast<std::uint32_t>(actual);
            std::memcpy(&info.adapterLuid, &description.AdapterLuid, sizeof(info.adapterLuid));
            info.hardware = true; info.debugLayer = flags != 0;
            if (debugMissing) info.diagnostics = "D3D11 debug layer unavailable (SDK_COMPONENT_MISSING); hardware device created without it.\n";
            break;
        }
        if (!device) {
            error = "No hardware D3D11 feature-level 11_0 adapter: HRESULT=" + Hex(creation);
            return false;
        }
        if (!Format(DXGI_FORMAT_R32G32B32A32_FLOAT, D3D11_FORMAT_SUPPORT_TEXTURE2D
            | D3D11_FORMAT_SUPPORT_RENDER_TARGET | D3D11_FORMAT_SUPPORT_SHADER_SAMPLE, error)
            || !Format(DXGI_FORMAT_R32_UINT, D3D11_FORMAT_SUPPORT_TEXTURE2D | D3D11_FORMAT_SUPPORT_RENDER_TARGET, error)
            || !Format(DXGI_FORMAT_D32_FLOAT, D3D11_FORMAT_SUPPORT_TEXTURE2D | D3D11_FORMAT_SUPPORT_DEPTH_STENCIL, error)) return false;

        const auto vs = CompileShader(Detail::kOpaqueUnlitShader, "AstralOpaqueUnlit", "VSMain", "vs_5_0", debug);
        const auto ps = CompileShader(Detail::kOpaqueUnlitShader, "AstralOpaqueUnlit", "PSMain", "ps_5_0", debug);
        info.diagnostics += vs.diagnostics + '\n' + ps.diagnostics;
        if (!vs.Succeeded() || !ps.Succeeded()) { error = info.diagnostics; return false; }
        if (!Check(device->CreateVertexShader(vs.bytecode.data(), vs.bytecode.size(), nullptr, &vertexShader), "CreateVertexShader", error)
            || !Check(device->CreatePixelShader(ps.bytecode.data(), ps.bytecode.size(), nullptr, &pixelShader), "CreatePixelShader", error)) return false;
        const D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, static_cast<UINT>(offsetof(Vertex, position)), D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, static_cast<UINT>(offsetof(Vertex, normal)), D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, static_cast<UINT>(offsetof(Vertex, uv)), D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, static_cast<UINT>(offsetof(Vertex, color)), D3D11_INPUT_PER_VERTEX_DATA, 0}};
        if (!Check(device->CreateInputLayout(layout, 4, vs.bytecode.data(), vs.bytecode.size(), &inputLayout), "CreateInputLayout", error)) return false;
        D3D11_BUFFER_DESC cb{};
        cb.ByteWidth = sizeof(Constants); cb.Usage = D3D11_USAGE_DEFAULT; cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        if (!Check(device->CreateBuffer(&cb, nullptr, &constants), "CreateBuffer(constants)", error)) return false;
        D3D11_RASTERIZER_DESC raster{};
        raster.FillMode = D3D11_FILL_SOLID; raster.CullMode = D3D11_CULL_BACK;
        raster.FrontCounterClockwise = FALSE; raster.DepthClipEnable = TRUE;
        if (!Check(device->CreateRasterizerState(&raster, &cullBack), "CreateRasterizerState(back)", error)) return false;
        raster.CullMode = D3D11_CULL_NONE;
        if (!Check(device->CreateRasterizerState(&raster, &cullNone), "CreateRasterizerState(none)", error)) return false;
        D3D11_DEPTH_STENCIL_DESC depth{};
        depth.DepthEnable = TRUE; depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; depth.DepthFunc = D3D11_COMPARISON_LESS;
        if (!Check(device->CreateDepthStencilState(&depth, &depthState), "CreateDepthStencilState", error)) return false;
        D3D11_SAMPLER_DESC sampler{};
        sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler.MaxLOD = D3D11_FLOAT32_MAX; sampler.MaxAnisotropy = 1;
        if (!Check(device->CreateSamplerState(&sampler, &repeatSampler), "CreateSamplerState(repeat)", error)) return false;
        sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        return Check(device->CreateSamplerState(&sampler, &clampSampler), "CreateSamplerState(clamp)", error);
    }

    bool Prepare(const RenderScene& scene, const RenderView& view, Color clear,
        std::vector<MeshUpload>& meshes, std::vector<TextureUpload>& textures,
        std::vector<PreparedDraw>& draws, FrameStats& stats, std::string& error,
        std::size_t targetPixelBytes = 44, std::size_t targetObjects = 6, std::size_t extraFixedBytes = 0) {
        if (!Math::IsFinite(clear) || !Finite(view.view) || !Finite(view.projection)) {
            error = "Non-finite clear color or view/projection"; return false;
        }
        if (!scene.billboards.empty() || !scene.lines.empty()) {
            error = "Opaque-Unlit pass does not support billboards/debug lines"; return false;
        }
        if (scene.draws.size() > limits.maxDraws || scene.draws.size() > UINT_MAX) {
            error = "Draw count budget exceeded"; return false;
        }
        const std::size_t pixels = static_cast<std::size_t>(view.width) * static_cast<std::size_t>(view.height);
        // Two RGBA32 buffers, two R32 ID buffers, D32, and the persistent CB.
        std::size_t targetBytes{};
        if (!Bytes(pixels, targetPixelBytes, targetBytes)
            || !AddBytes(sizeof(Constants), limits.maxResourceBytes, targetBytes)
            || !AddBytes(extraFixedBytes, limits.maxResourceBytes, targetBytes)) {
            error = "Render target resource budget exceeded"; return false;
        }
        stats.resourceBytes = targetBytes; stats.resourceCount = targetObjects;
        std::size_t geometryBytes = 0, textureBytes = 0;
        std::unordered_map<const MeshData*, std::size_t> meshLookup;
        std::unordered_map<const Texture2D*, std::size_t> textureLookup;
        const Math::Mat4 vp = view.ViewProjection();
        for (std::size_t index = 0; index < scene.draws.size(); ++index) {
            const DrawItem& draw = scene.draws[index];
            const auto reject = [&](const std::string& reason) {
                error = "Draw[" + std::to_string(index) + "]";
                if (draw.material && !draw.material->name.empty()) error += " material '" + draw.material->name + "'";
                error += ": " + reason; return false;
            };
            if (!draw.mesh || !draw.material) return reject("mesh and explicit material required");
            const Material& material = *draw.material;
            if (material.shading != ShadingModel::Unlit || material.blend != BlendMode::Opaque
                || material.cameraFade || material.opacity != 1.0f || draw.opacity != 1.0f)
                return reject("requires opaque Unlit, opacity 1 and cameraFade false; shading="
                    + std::string(ShadingLabel(material.shading)) + " blend="
                    + BlendLabel(material.blend) + " materialOpacity="
                    + std::to_string(material.opacity) + " drawOpacity=" + std::to_string(draw.opacity)
                    + " cameraFade=" + std::to_string(material.cameraFade));
            if (!Finite(draw.world) || !Math::IsFinite(material.baseColor) || !Math::IsFinite(draw.tint)
                || !Math::IsFinite(material.emissive) || !Math::IsFinite(draw.emissiveBoost)
                || !std::isfinite(material.uvScale.x) || !std::isfinite(material.uvScale.y)) return reject("non-finite material/transform");
            PreparedDraw prepared;
            prepared.constants.clip = vp * draw.world;
            const Color tint = Multiply(material.baseColor, draw.tint);
            const Color emissive = material.emissive + draw.emissiveBoost;
            if (!Finite(prepared.constants.clip) || !Math::IsFinite(tint) || !Math::IsFinite(emissive)) return reject("material/transform arithmetic overflow");
            prepared.constants.baseTint = {tint.x, tint.y, tint.z, 1};
            prepared.constants.emissive = {emissive.x, emissive.y, emissive.z, 0};
            prepared.constants.uvSize = {material.uvScale.x, material.uvScale.y, 1, 1};
            prepared.constants.flags[0] = material.vertexColor ? 1 : 0;
            prepared.constants.flags[2] = draw.objectId ? draw.objectId : static_cast<UINT>(index + 1);
            prepared.doubleSided = material.doubleSided;
            const MeshData& mesh = *draw.mesh;
            auto foundMesh = meshLookup.find(&mesh);
            if (foundMesh == meshLookup.end()) {
                std::size_t vb{}, ib{};
                if (!Bytes(mesh.vertices.size(), sizeof(Vertex), vb) || !Bytes(mesh.indices.size(), sizeof(std::uint32_t), ib)
                    || vb > UINT_MAX || ib > UINT_MAX || !AddBytes(vb, limits.maxGeometryBytes, geometryBytes)
                    || !AddBytes(ib, limits.maxGeometryBytes, geometryBytes)
                    || !AddBytes(vb, limits.maxResourceBytes, stats.resourceBytes)
                    || !AddBytes(ib, limits.maxResourceBytes, stats.resourceBytes)) return reject("geometry/resource budget exceeded");
                if (mesh.vertices.empty() || mesh.indices.empty() || mesh.indices.size() % 3) return reject("empty mesh or incomplete triangle indices");
                std::string validation;
                if (!mesh.Validate(validation)) return reject("invalid mesh: " + validation);
                prepared.mesh = meshes.size();
                meshLookup.emplace(&mesh, prepared.mesh);
                MeshUpload upload; upload.source = &mesh;
                upload.vertexBytes = static_cast<UINT>(vb); upload.indexBytes = static_cast<UINT>(ib);
                meshes.push_back(std::move(upload)); stats.resourceCount += 2;
            } else prepared.mesh = foundMesh->second;
            // Reject overflow before handing clip coordinates to the GPU.
            for (const Vertex& vertex : mesh.vertices) {
                if (!Finite(vertex.color)) return reject("non-finite vertex color");
                if (!Finite(prepared.constants.clip * Math::Vec4{vertex.position.x, vertex.position.y, vertex.position.z, 1})) return reject("clip transform overflow");
                if (!std::isfinite(vertex.uv.x * material.uvScale.x) || !std::isfinite(vertex.uv.y * material.uvScale.y)) return reject("UV scale overflow");
            }
            if (material.baseTexture) {
                const Texture2D& texture = *material.baseTexture;
                if (!texture.Valid()) return reject("invalid texture");
                prepared.textured = true;
                prepared.constants.flags[1] = 1;
                prepared.constants.uvSize.z = static_cast<float>(texture.Width());
                prepared.constants.uvSize.w = static_cast<float>(texture.Height());
                auto foundTexture = textureLookup.find(&texture);
                if (foundTexture == textureLookup.end()) {
                    if (texture.Width() > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || texture.Height() > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION
                        || texture.LevelCount() > D3D11_REQ_MIP_LEVELS) return reject("texture dimension/mip limit exceeded");
                    if (texture.wrap != TextureWrap::Repeat && texture.wrap != TextureWrap::Clamp) return reject("invalid texture wrap");
                    int width = texture.Width(), height = texture.Height();
                    for (int level = 0; level < texture.LevelCount(); ++level) {
                        const auto& mip = texture.GetLevel(level);
                        std::size_t texels{}, bytes{};
                        if (width <= 0 || height <= 0 || mip.width != width || mip.height != height
                            || !Bytes(static_cast<std::size_t>(width), static_cast<std::size_t>(height), texels)
                            || texels != mip.texels.size() || !Bytes(texels, sizeof(Math::Vec4), bytes)
                            || !AddBytes(bytes, limits.maxTextureBytes, textureBytes)
                            || !AddBytes(bytes, limits.maxResourceBytes, stats.resourceBytes)) return reject("invalid texture mip or texture/resource budget exceeded");
                        for (const auto& texel : mip.texels) if (!Finite(texel)) return reject("non-finite texel");
                        width = std::max(1, width / 2); height = std::max(1, height / 2);
                    }
                    prepared.texture = textures.size();
                    textureLookup.emplace(&texture, prepared.texture);
                    TextureUpload upload; upload.source = &texture; textures.push_back(std::move(upload));
                    ++stats.resourceCount;
                } else prepared.texture = foundTexture->second;
            }
            stats.triangles += mesh.TriangleCount(); stats.vertices += mesh.vertices.size();
            draws.push_back(prepared);
        }
        stats.draws = draws.size(); stats.textureCount = textures.size();
        return true;
    }

    bool Upload(std::vector<MeshUpload>& meshes, std::vector<TextureUpload>& textures, std::string& error) {
        for (auto& mesh : meshes) {
            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = mesh.vertexBytes; desc.Usage = D3D11_USAGE_IMMUTABLE; desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            D3D11_SUBRESOURCE_DATA data{}; data.pSysMem = mesh.source->vertices.data();
            if (!Check(device->CreateBuffer(&desc, &data, &mesh.vertices), "CreateBuffer(vertices)", error)) return false;
            desc.ByteWidth = mesh.indexBytes; desc.BindFlags = D3D11_BIND_INDEX_BUFFER; data.pSysMem = mesh.source->indices.data();
            if (!Check(device->CreateBuffer(&desc, &data, &mesh.indices), "CreateBuffer(indices)", error)) return false;
        }
        for (auto& texture : textures) {
            const Texture2D& source = *texture.source;
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = static_cast<UINT>(source.Width()); desc.Height = static_cast<UINT>(source.Height());
            desc.MipLevels = static_cast<UINT>(source.LevelCount()); desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_IMMUTABLE; desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            std::vector<D3D11_SUBRESOURCE_DATA> data(static_cast<std::size_t>(source.LevelCount()));
            for (int level = 0; level < source.LevelCount(); ++level) {
                data[static_cast<std::size_t>(level)].pSysMem = source.GetLevel(level).texels.data();
                data[static_cast<std::size_t>(level)].SysMemPitch = static_cast<UINT>(source.GetLevel(level).width * sizeof(Math::Vec4));
            }
            if (!Check(device->CreateTexture2D(&desc, data.data(), &texture.texture), "CreateTexture2D(material)", error)
                || !Check(device->CreateShaderResourceView(texture.texture.Get(), nullptr, &texture.view), "CreateShaderResourceView", error)) return false;
        }
        return true;
    }

    bool CreateTargets(int width, int height, Targets& target, std::string& error, bool readback = true) {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = static_cast<UINT>(width); desc.Height = static_cast<UINT>(height);
        desc.MipLevels = desc.ArraySize = desc.SampleDesc.Count = 1;
        desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; desc.BindFlags = D3D11_BIND_RENDER_TARGET;
        if (!readback) desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        desc.Usage = D3D11_USAGE_DEFAULT;
        if (!Check(device->CreateTexture2D(&desc, nullptr, &target.color), "CreateTexture2D(color)", error)
            || !Check(device->CreateRenderTargetView(target.color.Get(), nullptr, &target.colorView), "CreateRenderTargetView(color)", error)) return false;
        if (!readback && !Check(device->CreateShaderResourceView(target.color.Get(), nullptr, &target.colorResource),
            "CreateShaderResourceView(HDR)", error)) return false;
        desc.Format = DXGI_FORMAT_R32_UINT; desc.BindFlags = D3D11_BIND_RENDER_TARGET;
        if (!Check(device->CreateTexture2D(&desc, nullptr, &target.ids), "CreateTexture2D(ids)", error)
            || !Check(device->CreateRenderTargetView(target.ids.Get(), nullptr, &target.idView), "CreateRenderTargetView(ids)", error)) return false;
        desc.Format = DXGI_FORMAT_D32_FLOAT; desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        if (!Check(device->CreateTexture2D(&desc, nullptr, &target.depth), "CreateTexture2D(depth)", error)
            || !Check(device->CreateDepthStencilView(target.depth.Get(), nullptr, &target.depthView), "CreateDepthStencilView", error)) return false;
        if (!readback) return true;
        desc.BindFlags = 0; desc.Usage = D3D11_USAGE_STAGING; desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        if (!Check(device->CreateTexture2D(&desc, nullptr, &target.colorRead), "CreateTexture2D(color staging)", error)) return false;
        desc.Format = DXGI_FORMAT_R32_UINT;
        return Check(device->CreateTexture2D(&desc, nullptr, &target.idsRead), "CreateTexture2D(ids staging)", error);
    }
    void DrawScene(const RenderView& view, Color clear, Targets& target,
        const std::vector<MeshUpload>& meshes, const std::vector<TextureUpload>& textures,
        const std::vector<PreparedDraw>& draws) {
        auto* immediate = context.Get();
        ID3D11RenderTargetView* targets[] = {target.colorView.Get(), target.idView.Get()};
        immediate->OMSetRenderTargets(2, targets, target.depthView.Get());
        const float clearColor[] = {clear.x, clear.y, clear.z, 1};
        const float clearIds[] = {0, 0, 0, 0};
        immediate->ClearRenderTargetView(target.colorView.Get(), clearColor);
        immediate->ClearRenderTargetView(target.idView.Get(), clearIds);
        immediate->ClearDepthStencilView(target.depthView.Get(), D3D11_CLEAR_DEPTH, 1, 0);
        const D3D11_VIEWPORT viewport{0, 0, static_cast<float>(view.width), static_cast<float>(view.height), 0, 1};
        immediate->RSSetViewports(1, &viewport);
        immediate->OMSetDepthStencilState(depthState.Get(), 0);
        immediate->OMSetBlendState(nullptr, nullptr, UINT_MAX);
        immediate->IASetInputLayout(inputLayout.Get());
        immediate->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        immediate->VSSetShader(vertexShader.Get(), nullptr, 0); immediate->PSSetShader(pixelShader.Get(), nullptr, 0);
        ID3D11Buffer* cb = constants.Get();
        immediate->VSSetConstantBuffers(0, 1, &cb); immediate->PSSetConstantBuffers(0, 1, &cb);
        for (const PreparedDraw& draw : draws) {
            const MeshUpload& mesh = meshes[draw.mesh];
            const UINT stride = sizeof(Vertex), offset = 0;
            ID3D11Buffer* vb = mesh.vertices.Get();
            immediate->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
            immediate->IASetIndexBuffer(mesh.indices.Get(), DXGI_FORMAT_R32_UINT, 0);
            immediate->UpdateSubresource(constants.Get(), 0, nullptr, &draw.constants, 0, 0);
            immediate->RSSetState(draw.doubleSided ? cullNone.Get() : cullBack.Get());
            ID3D11ShaderResourceView* texture = draw.textured ? textures[draw.texture].view.Get() : nullptr;
            immediate->PSSetShaderResources(0, 1, &texture);
            ID3D11SamplerState* sampler = draw.textured && textures[draw.texture].source->wrap == TextureWrap::Clamp
                ? clampSampler.Get() : repeatSampler.Get();
            immediate->PSSetSamplers(0, 1, &sampler);
            immediate->DrawIndexed(static_cast<UINT>(mesh.source->indices.size()), 0, 0);
        }
    }
    bool Viewport(const RenderView& view, std::string& error) const {
        std::size_t pixels{};
        if (view.width < 0 || view.height < 0) { error = "Negative viewport"; return false; }
        if (view.width > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || view.height > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION
            || !Bytes(static_cast<std::size_t>(view.width), static_cast<std::size_t>(view.height), pixels)
            || pixels > limits.maxPixels) { error = "Viewport pixel/dimension budget exceeded"; return false; }
        return true;
    }
    bool CreateQuery(D3D11_QUERY kind, ComPtr<ID3D11Query>& query, std::string& error) {
        const D3D11_QUERY_DESC desc{kind, 0};
        return Check(device->CreateQuery(&desc, &query), "CreateQuery(native)", error);
    }
    bool Retire(bool drain, std::string& error) {
        while (!inFlight.empty()) {
            Submission& submission = inFlight.front();
            BOOL finished = FALSE;
            const HRESULT ready = context->GetData(submission.complete.Get(), &finished, sizeof(finished), D3D11_ASYNC_GETDATA_DONOTFLUSH);
            if (FAILED(ready)) return Check(ready, "GetData(native complete)", error);
            if (ready != S_OK && !drain) break;
            const auto deadline = Clock::now() + std::chrono::seconds(5);
            if (ready != S_OK && !Wait(submission.complete.Get(), &finished, sizeof(finished), deadline, error)) return false;
            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT frequency{};
            UINT64 begin{}, uploaded{}, sceneEnd{}, end{};
            if (!Wait(submission.disjoint.Get(), &frequency, sizeof(frequency), deadline, error)
                || !Wait(submission.begin.Get(), &begin, sizeof(begin), deadline, error)
                || !Wait(submission.uploaded.Get(), &uploaded, sizeof(uploaded), deadline, error)
                || !Wait(submission.sceneEnd.Get(), &sceneEnd, sizeof(sceneEnd), deadline, error)
                || !Wait(submission.end.Get(), &end, sizeof(end), deadline, error)) return false;
            FrameStats& stats = submission.stats;
            if (!frequency.Disjoint && frequency.Frequency && begin <= uploaded && uploaded <= sceneEnd && sceneEnd <= end) {
                stats.gpuTimingValid = true;
                const double scale = 1000.0 / static_cast<double>(frequency.Frequency);
                stats.gpuUploadMs = static_cast<double>(uploaded - begin) * scale;
                stats.gpuSceneMs = static_cast<double>(sceneEnd - uploaded) * scale;
                stats.gpuDisplayMs = static_cast<double>(end - sceneEnd) * scale;
                stats.gpuMs = static_cast<double>(end - begin) * scale;
            }
            if (completedFrames.size() >= 256) {
                error = "Completed native timing queue exceeded 256; consumer must collect every frame";
                return false;
            }
            completedFrames.push_back(stats);
            inFlight.pop_front();
        }
        return true;
    }
    bool ResizeNative(int width, int height, std::string& error) {
        if (width == nativeWidth && height == nativeHeight && backbufferView) return true;
        if (!Retire(true, error)) return false;
        context->ClearState(); context->Flush();
        nativeTargets = {}; uiResource.Reset(); uiTexture.Reset(); backbufferView.Reset();
        nativeWidth = nativeHeight = 0;
        if (!Check(swapChain->ResizeBuffers(2, static_cast<UINT>(width), static_cast<UINT>(height),
            DXGI_FORMAT_R8G8B8A8_UNORM, 0), "ResizeBuffers", error)) return false;
        ComPtr<ID3D11Texture2D> backbuffer;
        if (!Check(swapChain->GetBuffer(0, IID_PPV_ARGS(&backbuffer)), "GetBuffer", error)) return false;
        D3D11_TEXTURE2D_DESC actual{}; backbuffer->GetDesc(&actual);
        if (actual.Width != static_cast<UINT>(width) || actual.Height != static_cast<UINT>(height)) {
            error = "Actual swapchain backbuffer differs from requested full HWND client: " + std::to_string(actual.Width)
                + "x" + std::to_string(actual.Height); return false;
        }
        if (!Check(device->CreateRenderTargetView(backbuffer.Get(), nullptr, &backbufferView), "CreateRenderTargetView(backbuffer)", error)
            || !CreateTargets(width, height, nativeTargets, error, false)) return false;
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = static_cast<UINT>(width); desc.Height = static_cast<UINT>(height);
        desc.MipLevels = desc.ArraySize = desc.SampleDesc.Count = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        if (!Check(device->CreateTexture2D(&desc, nullptr, &uiTexture), "CreateTexture2D(UI)", error)
            || !Check(device->CreateShaderResourceView(uiTexture.Get(), nullptr, &uiResource), "CreateShaderResourceView(UI)", error)) return false;
        nativeWidth = width; nativeHeight = height;
        return true;
    }
    void DrawDisplay(float exposure) {
        auto* immediate = context.Get();
        immediate->OMSetRenderTargets(0, nullptr, nullptr);
        ID3D11RenderTargetView* target = backbufferView.Get();
        immediate->OMSetRenderTargets(1, &target, nullptr);
        immediate->OMSetDepthStencilState(displayDepth.Get(), 0);
        immediate->RSSetState(cullNone.Get());
        immediate->IASetInputLayout(nullptr);
        ID3D11Buffer* noBuffer = nullptr; const UINT zero = 0;
        immediate->IASetVertexBuffers(0, 1, &noBuffer, &zero, &zero);
        immediate->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
        immediate->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        immediate->VSSetShader(displayVertex.Get(), nullptr, 0);
        immediate->PSSetShader(displayPixel.Get(), nullptr, 0);
        const float constantsData[4] = {exposure, 0, 0, 0};
        immediate->UpdateSubresource(displayConstants.Get(), 0, nullptr, constantsData, 0, 0);
        ID3D11Buffer* cb = displayConstants.Get(); immediate->PSSetConstantBuffers(0, 1, &cb);
        ID3D11ShaderResourceView* resources[] = {nativeTargets.colorResource.Get(), uiResource.Get()};
        immediate->PSSetShaderResources(0, 2, resources);
        immediate->Draw(3, 0);
        immediate->ClearState();
    }
    bool Wait(ID3D11Asynchronous* query, void* data, UINT size, Clock::time_point deadline, std::string& error) {
        for (;;) {
            const HRESULT hr = context->GetData(query, data, size, D3D11_ASYNC_GETDATA_DONOTFLUSH);
            if (hr == S_OK) return true;
            if (FAILED(hr)) return Check(hr, "GetData", error);
            if (Clock::now() >= deadline) {
                error = "GPU completion exceeded five-second deadline; removalReason=" + Hex(device->GetDeviceRemovedReason());
                Reset(); return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    bool Read(ID3D11Texture2D* texture, Frame& frame, bool color, Clock::time_point deadline, std::string& error) {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        for (;;) {
            const HRESULT hr = context->Map(texture, 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &mapped);
            if (hr == S_OK) break;
            if (hr != DXGI_ERROR_WAS_STILL_DRAWING) return Check(hr, "Map(staging)", error);
            if (Clock::now() >= deadline) {
                error = "GPU readback exceeded five-second deadline; removalReason=" + Hex(device->GetDeviceRemovedReason());
                Reset(); return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        // Allocate before Map in Render, so nothing throwing can strand a mapping.
        const auto* bytes = static_cast<const std::uint8_t*>(mapped.pData);
        const std::size_t rowBytes = static_cast<std::size_t>(frame.width) * (color ? sizeof(Math::Vec4) : sizeof(std::uint32_t));
        if (mapped.RowPitch < rowBytes) {
            context->Unmap(texture, 0); error = "Staging RowPitch smaller than image width"; return false;
        }
        for (int y = 0; y < frame.height; ++y) {
            const std::size_t offset = static_cast<std::size_t>(y) * static_cast<std::size_t>(frame.width);
            if (color) {
                const auto* row = reinterpret_cast<const Math::Vec4*>(bytes + static_cast<std::size_t>(y) * mapped.RowPitch);
                for (int x = 0; x < frame.width; ++x) frame.hdr[offset + static_cast<std::size_t>(x)] = {row[x].x, row[x].y, row[x].z};
            } else std::memcpy(frame.objectId.data() + offset, bytes + static_cast<std::size_t>(y) * mapped.RowPitch, rowBytes);
        }
        context->Unmap(texture, 0);
        return true;
    }
};

D3D11Renderer::D3D11Renderer() : impl_(std::make_unique<Impl>()) {}
D3D11Renderer::~D3D11Renderer() { Shutdown(); }
void D3D11Renderer::Shutdown() {
    std::string ignored;
    DetachWindow(ignored);
    impl_->Reset(); impl_->info = {};
}
bool D3D11Renderer::Ready() const { return impl_->device != nullptr; }
const DeviceInfo& D3D11Renderer::Info() const { return impl_->info; }

bool D3D11Renderer::Initialize(std::string& error, Limits limits, bool requestDebugLayer) {
    error.clear();
    if (!impl_->Thread(error)) return false;
    Shutdown(); impl_->limits = limits;
    if (limits.maxResourceBytes < sizeof(Constants)) {
        error = "Resource budget cannot hold the constant buffer";
        return false;
    }
    try {
        if (impl_->Create(error, requestDebugLayer)) return true;
    } catch (const std::exception& exception) { error = std::string("D3D11 initialization: ") + exception.what(); }
    impl_->Reset();
    return false;
}

RenderStatus D3D11Renderer::RenderOpaqueUnlit(const RenderScene& scene, const RenderView& view,
    Frame& output, std::string& error, Color clear) {
    error.clear();
    Impl& state = *impl_;
    if (!state.Thread(error)) return RenderStatus::Failed;
    if (!Ready()) { error = "D3D11 renderer is not initialized"; return RenderStatus::Failed; }
    if (view.width < 0 || view.height < 0) { error = "Negative viewport"; return RenderStatus::Failed; }
    if (!view.width || !view.height) return RenderStatus::Skipped;
    std::size_t pixels{};
    if (view.width > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || view.height > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION
        || !Bytes(static_cast<std::size_t>(view.width), static_cast<std::size_t>(view.height), pixels)
        || pixels > state.limits.maxPixels) { error = "Viewport pixel/dimension budget exceeded"; return RenderStatus::Failed; }
    const auto start = Clock::now();
    try {
        Frame frame; frame.width = view.width; frame.height = view.height;
        std::vector<MeshUpload> meshes; std::vector<TextureUpload> textures; std::vector<PreparedDraw> draws;
        if (!state.Prepare(scene, view, clear, meshes, textures, draws, frame.stats, error)) return RenderStatus::Failed;
        frame.hdr.resize(pixels); frame.objectId.resize(pixels);
        Targets target;
        if (!state.Upload(meshes, textures, error) || !state.CreateTargets(view.width, view.height, target, error)) return RenderStatus::Failed;
        ComPtr<ID3D11Query> disjoint, begin, end, complete;
        D3D11_QUERY_DESC query{}; query.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
        if (!state.Check(state.device->CreateQuery(&query, &disjoint), "CreateQuery(disjoint)", error)) return RenderStatus::Failed;
        query.Query = D3D11_QUERY_TIMESTAMP;
        if (!state.Check(state.device->CreateQuery(&query, &begin), "CreateQuery(start)", error)
            || !state.Check(state.device->CreateQuery(&query, &end), "CreateQuery(end)", error)) return RenderStatus::Failed;
        query.Query = D3D11_QUERY_EVENT;
        if (!state.Check(state.device->CreateQuery(&query, &complete), "CreateQuery(complete)", error)) return RenderStatus::Failed;
        const auto uploaded = Clock::now();
        frame.stats.uploadMs = Milliseconds(start, uploaded);
        auto* context = state.context.Get();
        context->Begin(disjoint.Get()); context->End(begin.Get());
        state.DrawScene(view, clear, target, meshes, textures, draws);
        context->End(end.Get()); context->End(disjoint.Get());
        context->OMSetRenderTargets(0, nullptr, nullptr);
        context->CopyResource(target.colorRead.Get(), target.color.Get()); context->CopyResource(target.idsRead.Get(), target.ids.Get());
        context->End(complete.Get()); context->ClearState(); context->Flush();
        const auto submitted = Clock::now();
        frame.stats.submissionMs = Milliseconds(uploaded, submitted);
        const auto deadline = submitted + std::chrono::seconds(5);
        BOOL completed = FALSE;
        if (!state.Wait(complete.Get(), &completed, sizeof(completed), deadline, error)) return RenderStatus::Failed;
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT frequency{}; UINT64 t0{}, t1{};
        if (!state.Wait(disjoint.Get(), &frequency, sizeof(frequency), deadline, error)
            || !state.Wait(begin.Get(), &t0, sizeof(t0), deadline, error)
            || !state.Wait(end.Get(), &t1, sizeof(t1), deadline, error)
            || !state.Read(target.colorRead.Get(), frame, true, deadline, error)
            || !state.Read(target.idsRead.Get(), frame, false, deadline, error)) return RenderStatus::Failed;
        if (!frequency.Disjoint && frequency.Frequency && t1 >= t0) {
            frame.stats.gpuTimingValid = true;
            frame.stats.gpuMs = static_cast<double>(t1 - t0) * 1000.0 / static_cast<double>(frequency.Frequency);
        }
        for (Color color : frame.hdr) if (!Math::IsFinite(color)) { error = "Non-finite GPU HDR output"; return RenderStatus::Failed; }
        ComPtr<IDXGIAdapter3> memory;
        if (SUCCEEDED(state.adapter.As(&memory))) {
            DXGI_QUERY_VIDEO_MEMORY_INFO usage{};
            if (SUCCEEDED(memory->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &usage))) {
                frame.stats.videoMemoryAvailable = true;
                frame.stats.localVideoUsage = usage.CurrentUsage; frame.stats.localVideoBudget = usage.Budget;
            }
        }
        const auto finished = Clock::now();
        frame.stats.completionReadbackMs = Milliseconds(submitted, finished);
        frame.stats.totalMs = Milliseconds(start, finished);
        output = std::move(frame);
        return RenderStatus::Rendered;
    } catch (const std::exception& exception) {
        if (state.context) state.context->ClearState();
        error = std::string("D3D11 render: ") + exception.what();
        return RenderStatus::Failed;
    }
}

bool D3D11Renderer::ValidateOpaqueUnlit(const RenderScene& scene, const RenderView& view, std::string& error, Color clear) {
    error.clear();
    try {
    if (!impl_->Thread(error) || !impl_->Viewport(view, error)) return false;
    if (!view.width || !view.height) { error = "GPU selection requires a nonzero client"; return false; }
    if (!std::isfinite(scene.post.exposure) || scene.post.exposure < 0) { error = "Non-finite or negative display exposure"; return false; }
    if (!Math::IsFinite(clear)) { error = "Non-finite native clear color"; return false; }
    for (float channel : {clear.x, clear.y, clear.z}) {
        if (std::fabs(static_cast<double>(channel)) * scene.post.exposure > std::numeric_limits<float>::max()) {
            error = "Clear color: possible HDR/display arithmetic overflow"; return false;
        }
    }
    std::vector<MeshUpload> meshes; std::vector<TextureUpload> textures; std::vector<PreparedDraw> draws;
    FrameStats stats;
    if (!impl_->Prepare(scene, view, clear, meshes, textures, draws, stats, error, 36, 8, 16)) return false;
    // Normal presentation has no HDR readback. Reject possible arithmetic
    // overflow before submission instead of hiding an Inf in the display pass.
    for (std::size_t index = 0; index < draws.size(); ++index) {
        const PreparedDraw& draw = draws[index];
        double vertexMaximum[3] = {1, 1, 1}, textureMaximum[3] = {1, 1, 1};
        if (draw.constants.flags[0]) {
            std::fill(std::begin(vertexMaximum), std::end(vertexMaximum), 0);
            for (const Vertex& vertex : meshes[draw.mesh].source->vertices) {
                const float values[] = {vertex.color.x, vertex.color.y, vertex.color.z};
                for (int c = 0; c < 3; ++c) vertexMaximum[c] = std::max(vertexMaximum[c], std::fabs(static_cast<double>(values[c])));
            }
        }
        if (draw.textured) {
            std::fill(std::begin(textureMaximum), std::end(textureMaximum), 0);
            const Texture2D& texture = *textures[draw.texture].source;
            for (int level = 0; level < texture.LevelCount(); ++level) for (const Math::Vec4& texel : texture.GetLevel(level).texels) {
                const float values[] = {texel.x, texel.y, texel.z};
                for (int c = 0; c < 3; ++c) textureMaximum[c] = std::max(textureMaximum[c], std::fabs(static_cast<double>(values[c])));
            }
        }
        const float base[] = {draw.constants.baseTint.x, draw.constants.baseTint.y, draw.constants.baseTint.z};
        const float glow[] = {draw.constants.emissive.x, draw.constants.emissive.y, draw.constants.emissive.z};
        for (int c = 0; c < 3; ++c) {
            const double vertexProduct = std::fabs(static_cast<double>(base[c])) * vertexMaximum[c];
            const double hdrBound = vertexProduct * textureMaximum[c] + std::fabs(static_cast<double>(glow[c]));
            if (vertexProduct > std::numeric_limits<float>::max() || hdrBound > std::numeric_limits<float>::max()
                || hdrBound * scene.post.exposure > std::numeric_limits<float>::max()) {
                error = "Draw[" + std::to_string(index) + "]: possible HDR/display arithmetic overflow"; return false;
            }
        }
    }
    return true;
    } catch (const std::exception& exception) {
        error = std::string("Native validation: ") + exception.what(); return false;
    }
}

bool D3D11Renderer::AttachWindow(void* nativeWindow, std::string& error) {
    error.clear();
    Impl& state = *impl_;
    if (!state.Thread(error)) return false;
    if (!Ready()) { error = "D3D11 renderer is not initialized"; return false; }
    HWND window = static_cast<HWND>(nativeWindow);
    if (!window || !IsWindow(window) || GetWindowThreadProcessId(window, nullptr) != GetCurrentThreadId()) {
        error = "Native GPU HWND must be live and owned by this thread"; return false;
    }
    RECT client{}; RenderView attachment;
    if (!GetClientRect(window, &client) || client.right <= 0 || client.bottom <= 0) { error = "GPU HWND requires a nonzero client"; return false; }
    attachment.width = client.right; attachment.height = client.bottom;
    std::size_t attachmentBytes{};
    if (!state.Viewport(attachment, error)) return false;
    if (!Bytes(static_cast<std::size_t>(client.right) * client.bottom, 36, attachmentBytes)
        || !AddBytes(sizeof(Constants) + 16, state.limits.maxResourceBytes, attachmentBytes)) {
        error = "Native attachment resource budget exceeded"; return false;
    }
    if (!DetachWindow(error)) return false;
    try {
        ComPtr<IDXGIFactory2> factory;
        if (!state.Check(state.adapter->GetParent(IID_PPV_ARGS(&factory)), "GetParent(factory2)", error)) return false;
        DXGI_SWAP_CHAIN_DESC1 desc{};
        desc.Width = static_cast<UINT>(client.right); desc.Height = static_cast<UINT>(client.bottom);
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; desc.SampleDesc.Count = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; desc.BufferCount = 2;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; desc.Scaling = DXGI_SCALING_STRETCH;
        desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        if (!state.Check(factory->CreateSwapChainForHwnd(state.device.Get(), window, &desc, nullptr, nullptr, &state.swapChain),
                "CreateSwapChainForHwnd", error)
            || !state.Check(factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER), "MakeWindowAssociation", error)) {
            state.ResetPresentation(); return false;
        }
        const auto vs = CompileShader(Detail::kNativeDisplayShader, "AstralNativeDisplay", "VSMain", "vs_5_0", state.info.debugLayer);
        const auto ps = CompileShader(Detail::kNativeDisplayShader, "AstralNativeDisplay", "PSMain", "ps_5_0", state.info.debugLayer);
        if (!vs.Succeeded() || !ps.Succeeded()) { error = vs.diagnostics + '\n' + ps.diagnostics; state.ResetPresentation(); return false; }
        D3D11_BUFFER_DESC cb{}; cb.ByteWidth = 16; cb.Usage = D3D11_USAGE_DEFAULT; cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        D3D11_DEPTH_STENCIL_DESC depth{};
        depth.DepthEnable = FALSE; depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; depth.DepthFunc = D3D11_COMPARISON_ALWAYS;
        if (!state.Check(state.device->CreateVertexShader(vs.bytecode.data(), vs.bytecode.size(), nullptr, &state.displayVertex), "CreateVertexShader(display)", error)
            || !state.Check(state.device->CreatePixelShader(ps.bytecode.data(), ps.bytecode.size(), nullptr, &state.displayPixel), "CreatePixelShader(display)", error)
            || !state.Check(state.device->CreateBuffer(&cb, nullptr, &state.displayConstants), "CreateBuffer(display constants)", error)
            || !state.Check(state.device->CreateDepthStencilState(&depth, &state.displayDepth), "CreateDepthStencilState(display)", error)) {
            state.ResetPresentation(); return false;
        }
        state.nativeWindow = window;
        return true;
    } catch (const std::exception& exception) {
        error = std::string("Native attach: ") + exception.what(); state.ResetPresentation(); return false;
    }
}

bool D3D11Renderer::DetachWindow(std::string& error) {
    error.clear();
    if (!impl_->Thread(error)) return false;
    bool retired = true;
    if (impl_->context) {
        retired = impl_->Retire(true, error);
        if (impl_->context) { impl_->context->ClearState(); impl_->context->Flush(); }
    }
    impl_->ResetPresentation();
    if (impl_->context) impl_->context->Flush(); // retire deferred releases before HWND reuse
    return retired;
}

bool D3D11Renderer::HasWindow() const { return impl_->swapChain != nullptr; }

bool D3D11Renderer::CollectCompletedFrames(std::vector<FrameStats>& frames, std::string& error, bool drain) {
    error.clear();
    if (!impl_->Thread(error)) return false;
    if (!impl_->context) { error = "D3D11 renderer is not initialized"; return false; }
    if (!impl_->Retire(drain, error)) return false;
    frames.swap(impl_->completedFrames);
    impl_->completedFrames.clear();
    return true;
}

RenderStatus D3D11Renderer::PresentOpaqueUnlit(const RenderScene& scene, const RenderView& view,
    const ImageRgba8& overlay, FrameStats& stats, std::string& error, PresentationCapture* capture, Color clear) {
    error.clear();
    Impl& state = *impl_;
    if (!state.Thread(error)) return RenderStatus::Failed;
    if (!Ready() || !HasWindow()) { error = "Native D3D11 presentation is not attached"; return RenderStatus::Failed; }
    if (!state.Viewport(view, error)) return RenderStatus::Failed;
    if (!view.width || !view.height) return RenderStatus::Skipped;
    const auto validationStarted = Clock::now();
    if (!ValidateOpaqueUnlit(scene, view, error, clear)) return RenderStatus::Failed;
    const double validationMs = Milliseconds(validationStarted, Clock::now());
    const std::size_t pixels = static_cast<std::size_t>(view.width) * view.height;
    if (overlay.width != view.width || overlay.height != view.height || overlay.pixels.size() != pixels * 4) {
        error = "UI overlay must exactly match the full native viewport"; return RenderStatus::Failed;
    }
    const auto started = Clock::now();
    try {
        if (!state.Retire(false, error)) return RenderStatus::Failed;
        if (state.inFlight.size() >= 2 && !state.Retire(true, error)) return RenderStatus::Failed;
        if ((view.width != state.nativeWidth || view.height != state.nativeHeight) && !state.Retire(true, error)) return RenderStatus::Failed;
        Submission submission;
        ComPtr<ID3D11Texture2D> hdrRead, idsRead, displayRead;
        PresentationCapture captured;
        std::vector<PreparedDraw> draws;
        if (!state.Prepare(scene, view, clear, submission.meshes, submission.textures, draws, submission.stats, error, 36, 8, 16)) return RenderStatus::Failed;
        submission.stats.validationMs = validationMs;
        const std::size_t baseBytes = pixels * 36 + sizeof(Constants) + 16;
        submission.payloadBytes = submission.stats.resourceBytes - baseBytes;
        submission.payloadObjects = submission.stats.resourceCount - 8;
        for (const Submission& previous : state.inFlight) {
            if (!AddBytes(previous.payloadBytes, state.limits.maxResourceBytes, submission.stats.resourceBytes)) {
                error = "In-flight native resource budget exceeded"; return RenderStatus::Failed;
            }
            submission.stats.resourceCount += previous.payloadObjects;
        }
        if (capture && !AddBytes(pixels * 24, state.limits.maxResourceBytes, submission.stats.resourceBytes)) {
            error = "Native capture staging resource budget exceeded"; return RenderStatus::Failed;
        }
        if (capture) submission.stats.resourceCount += 3;
        if (!state.ResizeNative(view.width, view.height, error)
            || !state.CreateQuery(D3D11_QUERY_TIMESTAMP_DISJOINT, submission.disjoint, error)
            || !state.CreateQuery(D3D11_QUERY_TIMESTAMP, submission.begin, error)
            || !state.CreateQuery(D3D11_QUERY_TIMESTAMP, submission.uploaded, error)
            || !state.CreateQuery(D3D11_QUERY_TIMESTAMP, submission.sceneEnd, error)
            || !state.CreateQuery(D3D11_QUERY_TIMESTAMP, submission.end, error)
            || !state.CreateQuery(D3D11_QUERY_EVENT, submission.complete, error)) return RenderStatus::Failed;
        // Declared after every local GPU resource owner so it fences failed
        // work before those owners are destroyed. Successful work transfers to
        // inFlight; failed Begin/upload/draw/capture/Present paths stay local.
        struct FailedFrameGuard {
            Impl& state;
            Submission& submission;
            std::string& error;
            bool started{}, timingOpen{}, committed{};
            ~FailedFrameGuard() noexcept {
                if (!started || committed || !state.context) return;
                try {
                    state.context->ClearState();
                    if (timingOpen) state.context->End(submission.disjoint.Get());
                    state.context->End(submission.complete.Get()); state.context->Flush();
                    BOOL done = FALSE; std::string cleanup;
                    if (!state.Wait(submission.complete.Get(), &done, sizeof(done), Clock::now() + std::chrono::seconds(5), cleanup)) {
                        state.Reset(); error += " | failed-frame cleanup: " + cleanup;
                    }
                } catch (...) { state.Reset(); }
            }
        } failedFrame{state, submission, error};
        auto* context = state.context.Get();
        context->Begin(submission.disjoint.Get()); context->End(submission.begin.Get());
        failedFrame.started = failedFrame.timingOpen = true;
        if (!state.Upload(submission.meshes, submission.textures, error)) return RenderStatus::Failed;
        const auto overlayBegin = Clock::now();
        context->UpdateSubresource(state.uiTexture.Get(), 0, nullptr, overlay.pixels.data(), static_cast<UINT>(view.width * 4), 0);
        const auto uploaded = Clock::now();
        submission.stats.overlayUploadMs = Milliseconds(overlayBegin, uploaded);
        submission.stats.uploadMs = Milliseconds(started, uploaded);
        context->End(submission.uploaded.Get());
        state.DrawScene(view, clear, state.nativeTargets, submission.meshes, submission.textures, draws);
        context->End(submission.sceneEnd.Get());
        state.DrawDisplay(scene.post.exposure);
        context->End(submission.end.Get()); context->End(submission.disjoint.Get());
        failedFrame.timingOpen = false;

        if (capture) {
            captured.overlay = overlay;
            captured.geometry.width = view.width; captured.geometry.height = view.height;
            captured.geometry.hdr.resize(pixels); captured.geometry.objectId.resize(pixels);
            captured.display.Resize(view.width, view.height);
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = view.width; desc.Height = view.height; desc.MipLevels = desc.ArraySize = desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_STAGING; desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            if (!state.Check(state.device->CreateTexture2D(&desc, nullptr, &hdrRead), "CreateTexture2D(capture HDR)", error)) return RenderStatus::Failed;
            desc.Format = DXGI_FORMAT_R32_UINT;
            if (!state.Check(state.device->CreateTexture2D(&desc, nullptr, &idsRead), "CreateTexture2D(capture IDs)", error)) return RenderStatus::Failed;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            if (!state.Check(state.device->CreateTexture2D(&desc, nullptr, &displayRead), "CreateTexture2D(capture display)", error)) return RenderStatus::Failed;
            ComPtr<ID3D11Texture2D> backbuffer;
            if (!state.Check(state.swapChain->GetBuffer(0, IID_PPV_ARGS(&backbuffer)), "GetBuffer(capture)", error)) return RenderStatus::Failed;
            context->CopyResource(hdrRead.Get(), state.nativeTargets.color.Get());
            context->CopyResource(idsRead.Get(), state.nativeTargets.ids.Get());
            context->CopyResource(displayRead.Get(), backbuffer.Get());
        }
        context->End(submission.complete.Get()); context->Flush();
        const auto submitted = Clock::now();
        submission.stats.submissionMs = Milliseconds(uploaded, submitted);
        submission.stats.frameId = state.nextFrameId++;
        submission.stats.inFlightFrames = state.inFlight.size() + 1;
        submission.stats.queryCount = submission.stats.inFlightFrames * 6;
        const HRESULT present = state.swapChain->Present(1, 0);
        submission.stats.presentMs = Milliseconds(submitted, Clock::now());
        if (!state.Check(present, "Present(1)", error)) return RenderStatus::Failed;
        submission.stats.occluded = present == DXGI_STATUS_OCCLUDED;
        submission.stats.totalMs = Milliseconds(started, Clock::now());
        ComPtr<IDXGIAdapter3> memory;
        if (SUCCEEDED(state.adapter.As(&memory))) {
            DXGI_QUERY_VIDEO_MEMORY_INFO usage{};
            if (SUCCEEDED(memory->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &usage))) {
                submission.stats.videoMemoryAvailable = true;
                submission.stats.localVideoUsage = usage.CurrentUsage; submission.stats.localVideoBudget = usage.Budget;
            }
        }
        // GPU resources are retained, but no borrowed world/asset pointer is.
        for (auto& mesh : submission.meshes) mesh.source = nullptr;
        for (auto& texture : submission.textures) texture.source = nullptr;
        stats = submission.stats;
        state.inFlight.push_back(std::move(submission));
        failedFrame.committed = true;
        if (capture) {
            if (!state.Retire(true, error)) return RenderStatus::Failed;
            if (!state.completedFrames.empty() && state.completedFrames.back().frameId == stats.frameId) stats = state.completedFrames.back();
            const auto deadline = Clock::now() + std::chrono::seconds(5);
            if (!state.Read(hdrRead.Get(), captured.geometry, true, deadline, error)
                || !state.Read(idsRead.Get(), captured.geometry, false, deadline, error)) return RenderStatus::Failed;
            for (Color color : captured.geometry.hdr) if (!Math::IsFinite(color)) { error = "Non-finite native GPU HDR output"; return RenderStatus::Failed; }
            D3D11_MAPPED_SUBRESOURCE mapped{};
            for (;;) {
                const HRESULT hr = context->Map(displayRead.Get(), 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &mapped);
                if (hr == S_OK) break;
                if (hr != DXGI_ERROR_WAS_STILL_DRAWING) { state.Check(hr, "Map(native display capture)", error); return RenderStatus::Failed; }
                if (Clock::now() >= deadline) { error = "Native display readback exceeded five-second deadline"; state.Reset(); return RenderStatus::Failed; }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            const std::size_t rowBytes = static_cast<std::size_t>(view.width) * 4;
            if (mapped.RowPitch < rowBytes) { context->Unmap(displayRead.Get(), 0); error = "Invalid display capture RowPitch"; return RenderStatus::Failed; }
            for (int y = 0; y < view.height; ++y) std::memcpy(captured.display.pixels.data() + y * rowBytes,
                static_cast<const std::uint8_t*>(mapped.pData) + static_cast<std::size_t>(y) * mapped.RowPitch, rowBytes);
            context->Unmap(displayRead.Get(), 0);
            captured.geometry.stats = stats;
            *capture = std::move(captured);
        }
        return stats.occluded ? RenderStatus::Skipped : RenderStatus::Rendered;
    } catch (const std::exception& exception) {
        if (state.context) state.context->ClearState();
        error = std::string("Native presentation: ") + exception.what(); return RenderStatus::Failed;
    }
}

std::string D3D11Renderer::PresentationDiagnostics() const {
    std::ostringstream result;
    result << "renderAdapter=" << impl_->info.adapter << " renderLuid=" << impl_->info.adapterLuid
        << " hardware=" << impl_->info.hardware << " debugLayer=" << impl_->info.debugLayer
        << " backbuffer=" << impl_->nativeWidth << 'x' << impl_->nativeHeight << " syncInterval=1";
    if (!impl_->info.diagnostics.empty()) result << " deviceDiagnostics=[" << impl_->info.diagnostics << ']';
    if (impl_->swapChain) {
        ComPtr<IDXGIOutput> output;
        const HRESULT hr = impl_->swapChain->GetContainingOutput(&output);
        result << " containingOutputHRESULT=" << Hex(hr);
        if (SUCCEEDED(hr)) {
            DXGI_OUTPUT_DESC desc{};
            const HRESULT description = output->GetDesc(&desc);
            result << " outputDescHRESULT=" << Hex(description);
            if (SUCCEEDED(description)) result << " output=" << Utf8(desc.DeviceName)
                << " outputDesktop=" << desc.DesktopCoordinates.left << ',' << desc.DesktopCoordinates.top
                << ',' << desc.DesktopCoordinates.right << ',' << desc.DesktopCoordinates.bottom;
            ComPtr<IDXGIAdapter1> owner;
            const HRESULT parent = output->GetParent(IID_PPV_ARGS(&owner));
            result << " outputParentHRESULT=" << Hex(parent);
            if (SUCCEEDED(parent)) {
                DXGI_ADAPTER_DESC1 adapter{};
                const HRESULT adapterDescription = owner->GetDesc1(&adapter);
                result << " outputAdapterDescHRESULT=" << Hex(adapterDescription);
                if (SUCCEEDED(adapterDescription)) {
                    std::uint64_t luid{}; std::memcpy(&luid, &adapter.AdapterLuid, sizeof(luid));
                    result << " outputAdapter=" << Utf8(adapter.Description) << " outputAdapterLuid=" << luid;
                }
            }
        }
    }
    return result.str();
}

bool D3D11Renderer::ReportLiveObjects(std::string& error) const {
    error.clear();
    if (!impl_->Thread(error)) return false;
    if (!impl_->device || !impl_->info.debugLayer) { error = "Actual D3D11 debug layer is unavailable"; return false; }
    ComPtr<ID3D11Debug> debug;
    HRESULT hr = impl_->device.As(&debug);
    if (SUCCEEDED(hr)) hr = debug->ReportLiveDeviceObjects(static_cast<D3D11_RLDO_FLAGS>(D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL));
    if (FAILED(hr)) { error = "ReportLiveDeviceObjects HRESULT=" + Hex(hr); return false; }
    return true;
}

} // namespace Astral::Graphics::D3D11
