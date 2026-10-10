// Headless qualification only. No scene, window, ROM or gameplay input.
RaytracingAccelerationStructure world : register(t1,space1);
RWStructuredBuffer<uint4> results : register(u1,space0);
[numthreads(3,1,1)]
void CSMain(uint3 id : SV_DispatchThreadID) {
    RayDesc ray;
    ray.Origin=float3(id.x==1 ? 20 : 0,0,0);
    ray.Direction=float3(0,0,id.x==2 ? -1 : 1);
    ray.TMin=0.01;ray.TMax=100;
    RayQuery<RAY_FLAG_CULL_NON_OPAQUE> query;
    query.TraceRayInline(world,RAY_FLAG_NONE,0xFF,ray);
    while(query.Proceed()) {}
    bool hit=query.CommittedStatus()==COMMITTED_TRIANGLE_HIT;
    results[id.x]=hit ? uint4(1,uint(round(query.CommittedRayT()*1000)),query.CommittedPrimitiveIndex(),0) : 0;
}
