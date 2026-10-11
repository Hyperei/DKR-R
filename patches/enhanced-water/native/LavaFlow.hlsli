#pragma once
// A predominantly molten surface, not black plates with isolated cracks.
// Continuous world-space convection; no feedback, time wrap or extra pass.
static const float LAVA_WORLD_SCALE=0.018;
static const float2 LAVA_FLOW=float2(6,1.5);
float lavaHash(float2 cell) {
    // Integer seeds are identical across neighbouring cells, shader inlining
    // and D3D/Vulkan contraction rules. A floating-point fract hash can give
    // the same cell different values and cause emissive details to sparkle.
    uint2 key=asuint(int2(cell));
    uint hash=(key.x*0x8DA6B343u)^(key.y*0xD8163841u);
    hash^=hash>>16;hash*=0x7FEB352Du;
    hash^=hash>>15;hash*=0x846CA68Bu;
    hash^=hash>>16;
    return float(hash>>8)*(1.0/16777216.0);
}
float lavaNoise(float2 at) {
    float2 cell=floor(at),f=frac(at);
    f=f*f*f*(f*(f*6-15)+10);
    return lerp(lerp(lavaHash(cell),lavaHash(cell+float2(1,0)),f.x),
        lerp(lavaHash(cell+float2(0,1)),lavaHash(cell+1),f.x),f.y);
}
float4 lavaSurface(float2 world,float time,float footprint) {
    float2 at=(world-LAVA_FLOW*time)*LAVA_WORLD_SCALE;
    // The two flow layers evolve at different rates, so the surface churns
    // rather than sliding a static, oversized pattern across the lava mesh.
    at+=float2(lavaNoise(at*0.53+float2(0.06,-0.045)*time+11),
        lavaNoise(at*0.53+float2(-0.04,0.05)*time-17))*1.5;
    float broadFilter=1-smoothstep(0.3,1.1,footprint);
    float cellFilter=1-smoothstep(0.18,0.7,footprint*1.75);
    float fineFilter=1-smoothstep(0.18,0.7,footprint*5.3);
    float broad=lerp(0.5,lavaNoise(at),broadFilter);
    float cells=lerp(0.5,lavaNoise(at*1.75+float2(-0.12,0.09)*time+23),cellFilter);
    float grain=0.5;
    if(fineFilter>0)grain=lerp(0.5,lavaNoise(at*5.3+float2(0.08,-0.06)*time-53),fineFilter);
    float veins=(1-smoothstep(0.025,0.14,abs(cells-0.5)))*cellFilter;
    // Wide yellow-orange pools with soft white-hot filaments. Only small
    // cooling flecks remain; even those retain a dull red self-emission.
    float heat=saturate(0.62+(broad-0.5)*0.8+(cells-0.5)*0.65+(grain-0.5)*0.20+veins*0.22);
    float3 molten=lerp(float3(2.4,0.38,0.005),float3(5,2.8,0.035),smoothstep(0.15,0.64,heat));
    molten=lerp(molten,float3(8,6,1.8),smoothstep(0.62,1.05,heat));
    float crust=(1-smoothstep(0.19,0.32,broad))*smoothstep(0.58,0.77,cells)*0.85;
    return float4(lerp(molten,float3(0.35,0.035,0.005),crust),crust);
}
float3 lavaDisplayColor(float3 radiance,float3 fogColor,float fogAmount) {
    // Compress emission without clipping all the bright detail to solid
    // white. Apply the course's distance fog after the display compression.
    radiance=max(radiance,0);
    float3 color=radiance/(1+radiance);
    color=lerp(color,fogColor,saturate(fogAmount));
    return pow(saturate(color),1.0/2.2);
}
