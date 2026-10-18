#pragma once

namespace Astral::Graphics::D3D11::Detail {

inline constexpr char kOpaqueUnlitShader[] = R"hlsl(
cbuffer DrawConstants : register(b0) {
    row_major float4x4 clipFromLocal;
    float4 baseTint;
    float4 emissive;
    float4 uvAndTextureSize;
    uint4 flags; // vertex color, texture, object ID, unused
};
Texture2D<float4> baseTexture : register(t0);
SamplerState textureSampler : register(s0);
struct Input {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};
struct Interpolated {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};
Interpolated VSMain(Input input) {
    Interpolated output;
    output.position = mul(clipFromLocal, float4(input.position, 1));
    output.uv = input.uv * uvAndTextureSize.xy;
    output.color = input.color;
    return output;
}
struct Pixel {
    float4 hdr : SV_Target0;
    uint objectId : SV_Target1;
};
Pixel PSMain(Interpolated input) {
    float3 albedo = baseTint.rgb;
    if (flags.x != 0) albedo *= input.color.rgb;
    if (flags.y != 0) {
        float2 dx = ddx(input.uv) * uvAndTextureSize.zw;
        float2 dy = ddy(input.uv) * uvAndTextureSize.zw;
        float rho = max(dot(dx, dx), dot(dy, dy));
        float lod = rho > 1 ? 0.5 * log2(rho) : 0;
        // CPU Texture2D::Sample elides interpolation for a fraction below .01.
        if (frac(lod) < 0.01) lod = floor(lod);
        albedo *= baseTexture.SampleLevel(textureSampler, input.uv, lod).rgb;
    }
    Pixel output;
    output.hdr = float4(albedo + emissive.rgb, 1);
    output.objectId = flags.z;
    return output;
}
)hlsl";

inline constexpr char kNativeDisplayShader[] = R"hlsl(
cbuffer DisplayConstants : register(b0) { float exposure; float3 unused; };
Texture2D<float4> hdrTexture : register(t0);
Texture2D<float4> uiTexture : register(t1);
struct DisplayVertex { float4 position : SV_Position; };
DisplayVertex VSMain(uint id : SV_VertexID) {
    DisplayVertex output;
    output.position = float4(id == 2 ? 3 : -1, id == 1 ? 3 : -1, 0, 1);
    return output;
}
float3 Neutral(float3 color) {
    const float startCompression = 0.76;
    float x = min(color.x, min(color.y, color.z));
    float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
    color -= offset;
    float peak = max(color.x, max(color.y, color.z));
    float3 result = color;
    if (peak >= startCompression) {
        float d = 1 - startCompression;
        float newPeak = 1 - d * d / (peak + d - startCompression);
        color *= newPeak / peak;
        float g = 1 - 1 / (0.15 * (peak - newPeak) + 1);
        result = lerp(color, newPeak.xxx, g);
    }
    return result;
}
float Encode(float value) {
    value = saturate(value);
    return value <= 0.0031308 ? value * 12.92 : 1.055 * pow(value, 1.0 / 2.4) - 0.055;
}
float4 PSMain(DisplayVertex input) : SV_Target {
    int3 pixel = int3(int2(input.position.xy), 0);
    float3 mappedColor = Neutral(max(hdrTexture.Load(pixel).rgb * exposure, 0));
    float3 display = float3(Encode(mappedColor.r), Encode(mappedColor.g), Encode(mappedColor.b));
    // Existing Canvas/UI bytes use straight alpha in display-encoded space.
    float4 ui = uiTexture.Load(pixel);
    return float4(lerp(display, ui.rgb, ui.a), 1);
}
)hlsl";

} // namespace Astral::Graphics::D3D11::Detail
