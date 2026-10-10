// The sampling/gamma/PixelAntialiasing path is preserved from RT64's
// VideoInterfacePS.hlsl (see THIRD_PARTY.md). Only final presentation UVs rotate;
// this does not add an intermediate image, filter or extra draw.
struct AndroidVideoConstants {
    float2 videoResolution;
    float2 textureResolution;
    float gamma;
    uint rotation;
};
[[vk::push_constant]] ConstantBuffer<AndroidVideoConstants> gConstants : register(b0);
Texture2D<float4> gInput : register(t1);
SamplerState gSampler : register(s2);

float4 SampleInput(float2 uv) {
    const float2 LowerRight = gConstants.videoResolution / gConstants.textureResolution;
    const float2 HalfPixel = float2(0.5f, 0.5f) / gConstants.textureResolution;
    float2 outsideBorder = step(LowerRight, uv);
    float4 sampledColor = gInput.SampleLevel(gSampler, clamp(uv, HalfPixel, LowerRight - HalfPixel), 0);
    float4 gammaCorrectedColor = pow(sampledColor, gConstants.gamma);
    gammaCorrectedColor.rgb *= max(1.0f - outsideBorder.x - outsideBorder.y, 0.0f);
    gammaCorrectedColor.a = 1.0f;
    return gammaCorrectedColor;
}

float4 PixelAntialiasing(float2 uv) {
    float2 uvTexspace = uv * gConstants.videoResolution;
    float2 seam = floor(uvTexspace + 0.5f);
    uvTexspace = (uvTexspace - seam) / fwidth(uvTexspace) + seam;
    uvTexspace = clamp(uvTexspace, seam - 0.5f, seam + 0.5f);
    return SampleInput(uvTexspace / gConstants.textureResolution);
}

float4 PSMain(in float4 pos : SV_Position, in float2 uv : TEXCOORD0) : SV_TARGET {
    // Inverse of logical->image rotation, matching android_rotation_policy.hpp.
    if (gConstants.rotation == 2) uv = float2(uv.y, 1.0f - uv.x);
    else if (gConstants.rotation == 4) uv = 1.0f - uv;
    else if (gConstants.rotation == 8) uv = float2(1.0f - uv.y, uv.x);
#ifdef PIXEL_ANTIALIASING
    return PixelAntialiasing(uv);
#else
    return SampleInput((uv / gConstants.textureResolution) * gConstants.videoResolution);
#endif
}
