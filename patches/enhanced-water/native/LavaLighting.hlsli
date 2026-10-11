#pragma once
#define WATER_RAY_OPACITY_ONLY
#include "WaterRays.hlsli"

float lavaPulse(float seconds,float strength,float speed) {
    return 1-saturate(strength)*0.5*(1-sin(seconds*speed*1.7));
}
bool lavaVisible(float3 origin,float3 destination) {
    float3 delta=destination-origin;
    float distance=length(delta);
    if(distance<=4)return true;
    RayDesc ray;ray.Origin=origin;ray.Direction=delta/distance;
    ray.TMin=2;ray.TMax=distance-2;
    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> query;
    query.TraceRayInline(waterWorld,RAY_FLAG_NONE,0xFF,ray);
    while(query.Proceed()) {
        if(query.CandidateType()==CANDIDATE_NON_OPAQUE_TRIANGLE) {
            uint first=waterRayFirst(query.CandidateGeometryIndex(),query.CandidatePrimitiveIndex());
            float2 uv=query.CandidateTriangleBarycentrics();
            if(waterfallRayOpacity(first,float3(1-uv.x-uv.y,uv.x,uv.y))>0.03)
                query.CommitNonOpaqueTriangleHit();
        }
    }
    return query.CommittedStatus()==COMMITTED_NOTHING;
}
float3 lavaIrradiance(float3 world,float3 normal,WaterMaterial m,float seconds) {
    if(m.sunDirection.w<=0)return 0;
    // Deterministic bounded selection, not sixteen rays for every pixel.
    float2 strongest=0;uint2 indices=0;
    [unroll] for(uint i=0;i<16;++i) {
        float4 emitter=m.shoreSegments[i];
        if(emitter.w<=0)continue;
        float3 delta=emitter.xyz-world;
        float distance=length(delta);
        float range=max(80,emitter.w*3+180)*m.skyColor.w;
        float falloff=saturate(1-distance/range);
        float weight=falloff*falloff*saturate(dot(normal,delta/max(distance,0.001)));
        if(weight>strongest.x){strongest.y=strongest.x;indices.y=indices.x;strongest.x=weight;indices.x=i;}
        else if(weight>strongest.y){strongest.y=weight;indices.y=i;}
    }
    float energy=0;
    [unroll] for(uint j=0;j<2;++j) if(strongest[j]>0.002 &&
        lavaVisible(world+normal*3,m.shoreSegments[indices[j]].xyz))energy+=strongest[j];
    return float3(1,0.19,0.012)*energy*m.sunDirection.w*
        lavaPulse(seconds,m.sunColor.w,m.optics.y)*1.8;
}
