#include "Engine/Graphics/D3D11/D3D11Renderer.h"
#include "Engine/Graphics/D3D11/BuiltinShaders.h"
#include "Engine/Graphics/D3D11/ShaderCompiler.h"

#include <d3d11.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstring>
#include <exception>
#include <iomanip>
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
        clampSampler.Reset(); repeatSampler.Reset(); depthState.Reset();
        cullNone.Reset(); cullBack.Reset(); constants.Reset(); inputLayout.Reset();
        pixelShader.Reset(); vertexShader.Reset(); context.Reset(); device.Reset(); adapter.Reset();
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
        std::vector<PreparedDraw>& draws, FrameStats& stats, std::string& error) {
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
        if (!Bytes(pixels, 44, targetBytes) || !AddBytes(sizeof(Constants), limits.maxResourceBytes, targetBytes)) {
            error = "Render target resource budget exceeded"; return false;
        }
        stats.resourceBytes = targetBytes; stats.resourceCount = 6;
        std::size_t geometryBytes = 0, textureBytes = 0;
        std::unordered_map<const MeshData*, std::size_t> meshLookup;
        std::unordered_map<const Texture2D*, std::size_t> textureLookup;
        const Math::Mat4 vp = view.ViewProjection();
        for (std::size_t index = 0; index < scene.draws.size(); ++index) {
            const DrawItem& draw = scene.draws[index];
            const auto reject = [&](const std::string& reason) { error = "Draw[" + std::to_string(index) + "]: " + reason; return false; };
            if (!draw.mesh || !draw.material) return reject("mesh and explicit material required");
            const Material& material = *draw.material;
            if (material.shading != ShadingModel::Unlit || material.blend != BlendMode::Opaque
                || material.cameraFade || material.opacity != 1.0f || draw.opacity != 1.0f)
                return reject("only opaque Unlit, opacity 1, without cameraFade is supported");
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

    bool CreateTargets(int width, int height, Targets& target, std::string& error) {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = static_cast<UINT>(width); desc.Height = static_cast<UINT>(height);
        desc.MipLevels = desc.ArraySize = desc.SampleDesc.Count = 1;
        desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; desc.BindFlags = D3D11_BIND_RENDER_TARGET;
        desc.Usage = D3D11_USAGE_DEFAULT;
        if (!Check(device->CreateTexture2D(&desc, nullptr, &target.color), "CreateTexture2D(color)", error)
            || !Check(device->CreateRenderTargetView(target.color.Get(), nullptr, &target.colorView), "CreateRenderTargetView(color)", error)) return false;
        desc.Format = DXGI_FORMAT_R32_UINT;
        if (!Check(device->CreateTexture2D(&desc, nullptr, &target.ids), "CreateTexture2D(ids)", error)
            || !Check(device->CreateRenderTargetView(target.ids.Get(), nullptr, &target.idView), "CreateRenderTargetView(ids)", error)) return false;
        desc.Format = DXGI_FORMAT_D32_FLOAT; desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        if (!Check(device->CreateTexture2D(&desc, nullptr, &target.depth), "CreateTexture2D(depth)", error)
            || !Check(device->CreateDepthStencilView(target.depth.Get(), nullptr, &target.depthView), "CreateDepthStencilView", error)) return false;
        desc.BindFlags = 0; desc.Usage = D3D11_USAGE_STAGING; desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        if (!Check(device->CreateTexture2D(&desc, nullptr, &target.colorRead), "CreateTexture2D(color staging)", error)) return false;
        desc.Format = DXGI_FORMAT_R32_UINT;
        return Check(device->CreateTexture2D(&desc, nullptr, &target.idsRead), "CreateTexture2D(ids staging)", error);
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
void D3D11Renderer::Shutdown() { impl_->Reset(); impl_->info = {}; }
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
        ID3D11RenderTargetView* targets[] = {target.colorView.Get(), target.idView.Get()};
        context->OMSetRenderTargets(2, targets, target.depthView.Get());
        const float clearColor[] = {clear.x, clear.y, clear.z, 1};
        const float clearIds[] = {0, 0, 0, 0};
        context->ClearRenderTargetView(target.colorView.Get(), clearColor);
        context->ClearRenderTargetView(target.idView.Get(), clearIds);
        context->ClearDepthStencilView(target.depthView.Get(), D3D11_CLEAR_DEPTH, 1, 0);
        const D3D11_VIEWPORT viewport{0, 0, static_cast<float>(view.width), static_cast<float>(view.height), 0, 1};
        context->RSSetViewports(1, &viewport);
        context->OMSetDepthStencilState(state.depthState.Get(), 0);
        context->OMSetBlendState(nullptr, nullptr, UINT_MAX);
        context->IASetInputLayout(state.inputLayout.Get());
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(state.vertexShader.Get(), nullptr, 0); context->PSSetShader(state.pixelShader.Get(), nullptr, 0);
        ID3D11Buffer* cb = state.constants.Get();
        context->VSSetConstantBuffers(0, 1, &cb); context->PSSetConstantBuffers(0, 1, &cb);
        for (const PreparedDraw& draw : draws) {
            const MeshUpload& mesh = meshes[draw.mesh];
            const UINT stride = sizeof(Vertex), offset = 0;
            ID3D11Buffer* vb = mesh.vertices.Get();
            context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
            context->IASetIndexBuffer(mesh.indices.Get(), DXGI_FORMAT_R32_UINT, 0);
            context->UpdateSubresource(state.constants.Get(), 0, nullptr, &draw.constants, 0, 0);
            context->RSSetState(draw.doubleSided ? state.cullNone.Get() : state.cullBack.Get());
            ID3D11ShaderResourceView* texture = draw.textured ? textures[draw.texture].view.Get() : nullptr;
            context->PSSetShaderResources(0, 1, &texture);
            ID3D11SamplerState* sampler = draw.textured && textures[draw.texture].source->wrap == TextureWrap::Clamp
                ? state.clampSampler.Get() : state.repeatSampler.Get();
            context->PSSetSamplers(0, 1, &sampler);
            context->DrawIndexed(static_cast<UINT>(mesh.source->indices.size()), 0, 0);
        }
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

} // namespace Astral::Graphics::D3D11
