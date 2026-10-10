#pragma once
struct WaterRayVertex {float4 position;float4 colour;};
RaytracingAccelerationStructure waterWorld : register(t1,space1);
StructuredBuffer<WaterRayVertex> waterRayVertices : register(t2,space1);

float4 hardwareWaterReflection(float3 world,float3 normal,float3 view,WaterDrawParams p) {
    if(p.material.effects.w<0.5)return 0;
    RayDesc ray;ray.Origin=world+normal*3;ray.Direction=reflect(-view,normal);ray.TMin=1;ray.TMax=6000;
    RayQuery<RAY_FLAG_CULL_NON_OPAQUE> query;
    query.TraceRayInline(waterWorld,RAY_FLAG_NONE,0xFF,ray);
    while(query.Proceed()) {}
    if(query.CommittedStatus()!=COMMITTED_TRIANGLE_HIT)return 0;
    uint first=query.CommittedPrimitiveIndex()*3;
    float2 uv=query.CommittedTriangleBarycentrics();float3 weights=float3(1-uv.x-uv.y,uv.x,uv.y);
    float3 hit=ray.Origin+ray.Direction*query.CommittedRayT();
    // Prefer the exact original combiner/replacement texture/fog result when
    // this hit is visible. Off-screen static geometry uses its authored vertex
    // colours; no invented texture or foreground pixel is pulled into it.
    float4 projected=waterPixelFromWorld(hit,p);
    if(projected.w>0 && all(projected.xy>=p.viewBounds.xy+2) && all(projected.xy<p.viewBounds.zw-2)) {
        float depth=sceneDepth.Load(int3(int2(projected.xy),0));
        if(depth<0.9999 && length(waterWorldFromPixel(projected.xy,depth,p)-hit)<45)
            return float4(backgroundColor(projected.xy,p),1);
    }
    float3 authored=waterRayVertices[first].colour.rgb*weights.x+
        waterRayVertices[first+1].colour.rgb*weights.y+waterRayVertices[first+2].colour.rgb*weights.z;
    return float4(pow(saturate(authored),2.2)*0.45,saturate(1-query.CommittedRayT()/8000));
}
