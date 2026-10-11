"""Compose local ray-tested lava lighting; never edit dependency/generated sources."""
from pathlib import Path
import difflib
import sys
from water_patch_baseline import read_before

root=Path(__file__).resolve().parents[1]
stage=Path(sys.argv[1]).resolve()
output=root/'patches/enhanced-water/dkr-lava-lighting.patch'
layers=[output] if 'finishLava' in (stage/'src/render/rt64_framebuffer_renderer.cpp').read_text() else []
changes=[]
def replace(text,before,after):
    if text.count(before)!=1:raise SystemExit(f'Non-unique lighting context: {before[:120]}')
    return text.replace(before,after,1)
def change(path,fn):
    overlay=None
    if layers and path=='src/render/rt64_framebuffer_renderer.cpp':
        overlay=(stage/path).read_text()
        if '                instanceDrawCall.worldProjection=false;' not in overlay:
            overlay=replace(overlay,'                instanceDrawCall.waterIndex = UINT32_MAX;',
                '                instanceDrawCall.waterIndex = UINT32_MAX;\n                instanceDrawCall.worldProjection=false;\n                instanceDrawCall.projectionIndex=UINT32_MAX;')
    before=read_before(stage,path,layers,overlay);after=fn(before)
    if before==after:raise SystemExit(f'No lighting change: {path}')
    changes.append(f'diff --git a/{path} b/{path}\n')
    # Narrow context preserves Android's adjacent draw-metrics instrumentation.
    # Every edited semantic anchor above is still required to be unique.
    changes.extend(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile=f'a/{path}',tofile=f'b/{path}',n=1))

change('src/render/rt64_framebuffer_renderer_call.h',lambda s:replace(s,
    '        uint32_t waterIndex = UINT32_MAX;',
    '        uint32_t waterIndex = UINT32_MAX;\n        bool worldProjection = false;\n        uint32_t projectionIndex = UINT32_MAX;'))
change('src/render/rt64_water_renderer.h',lambda s:replace(s,
    '    WaterRayScene rays;',
    '''    WaterRayScene rays;
    std::unique_ptr<RenderShader> lavaLightingVS,lavaLightingPS;
    std::map<uint32_t,std::unique_ptr<RenderPipeline>> lavaLightingPipelines;
    bool lavaLightingUnavailable=false;
    RenderPipeline *lavaLightingPipeline(RenderDevice *device,RenderShaderFormat format,const RenderMultisampling& samples);
    bool drawLavaLighting(RenderWorker *worker,uint32_t drawIndex,bool depthReadOnly,const RenderViewport& viewport);
'''))

implementation=r'''
RenderPipeline *WaterRenderer::lavaLightingPipeline(RenderDevice *device,RenderShaderFormat format,const RenderMultisampling& samples) {
    if(lavaLightingUnavailable || !device->getCapabilities().rayQuery || !rayLayout)return nullptr;
    const uint32_t key=uint32_t(samples.sampleCount);
    if(auto it=lavaLightingPipelines.find(key);it!=lavaLightingPipelines.end())return it->second.get();
    try {
        if(!lavaLightingVS || !lavaLightingPS) {
            lavaLightingVS=checkedWaterResource(device->createShader(CREATE_SHADER_INPUTS(LavaLightingVSBlobDXIL,LavaLightingVSBlobSPIRV,LavaLightingVSBlobMSL,"VSMain",format)));
            lavaLightingPS=checkedWaterResource(device->createShader(CREATE_SHADER_INPUTS(LavaLightingPSBlobDXIL,LavaLightingPSBlobSPIRV,LavaLightingPSBlobMSL,"PSMain",format)));
        }
        RenderGraphicsPipelineDesc desc;
        desc.pipelineLayout=rayLayout.get();desc.vertexShader=lavaLightingVS.get();desc.pixelShader=lavaLightingPS.get();
        desc.renderTargetCount=1;desc.renderTargetFormat[0]=RenderTarget::colorBufferFormat(usesHDR);
        desc.renderTargetBlend[0]=RenderBlendDesc::Copy();desc.multisampling=samples;
        desc.depthEnabled=false;desc.depthWriteEnabled=false;desc.depthTargetFormat=RenderFormat::D32_FLOAT;
        desc.depthClipEnabled=false;desc.cullMode=RenderCullMode::NONE;desc.primitiveTopology=RenderPrimitiveTopology::TRIANGLE_LIST;
        auto result=device->createGraphicsPipeline(desc);
        if(!waterGraphicsPipelineValid(result.get()))throw std::runtime_error("lava lighting pipeline rejected");
        auto* pointer=result.get();lavaLightingPipelines.emplace(key,std::move(result));return pointer;
    } catch(const std::exception& error) {
        lavaLightingUnavailable=true;
        std::fprintf(stderr,"[water][lava-lighting] retaining emissive lava without scene lighting: %s\n",error.what());
        return nullptr;
    }
}
bool WaterRenderer::drawLavaLighting(RenderWorker *worker,uint32_t drawIndex,bool depthReadOnly,const RenderViewport& viewport) {
    if(drawIndex>=draws.size() || drawIndex>=drawScenes.size() || !rays.set || lavaLightingUnavailable)return false;
    auto& scene=scenes[drawScenes[drawIndex]];
    const auto& d=draws[drawIndex];
    if(d.material.effects.z!=5 || d.material.effects.w<0.5f || d.material.sunDirection.w<=0 || !scene.sourceColor)return false;
    // Pipeline/resource creation belongs to workload preparation, never record.
    auto it=lavaLightingPipelines.find(uint32_t(scene.sourceColor->multisampling.sampleCount));
    if(it==lavaLightingPipelines.end() || !it->second)return false;
    // Fresh capture after world geometry, before orthographic HUD. It shares
    // existing viewport scratch resources; no new full-resolution attachment.
    scene.captured=false;scenes[scene.captureOwner].opticalCaptured=false;
    captureScene(worker,drawIndex,depthReadOnly);
    worker->commandList->setFramebuffer(nullptr);
    worker->commandList->setViewports(viewport);
    worker->commandList->setScissors(RenderRect(int32_t(d.viewBounds.x),int32_t(d.viewBounds.y),int32_t(d.viewBounds.z),int32_t(d.viewBounds.w)));
    worker->commandList->setGraphicsPipelineLayout(rayLayout.get());
    worker->commandList->setGraphicsDescriptorSet(materialSet(drawIndex),0);
    worker->commandList->setGraphicsDescriptorSet(rays.set->get(),1);
    worker->commandList->setPipeline(it->second.get());
    interop::WaterPushConstants pc{drawIndex};worker->commandList->setGraphicsPushConstants(0,&pc);
    return true;
}
'''
# drawLavaLighting prepares bindings; caller binds the target and issues the draw.
def renderer(s):
    for fmt in ('spirv','dxil'):
        s=replace(s,f'#include "shaders/WaterHardwarePS.hlsl.{fmt}.h"',
            f'#include "shaders/WaterHardwarePS.hlsl.{fmt}.h"\n#include "shaders/LavaLightingVS.hlsl.{fmt}.h"\n#include "shaders/LavaLightingPS.hlsl.{fmt}.h"')
    return replace(s,'bool WaterRenderer::prepareRays(',implementation+'\nbool WaterRenderer::prepareRays(')
change('src/render/rt64_water_renderer.cpp',renderer)

finish=r'''        uint32_t pendingLava=UINT32_MAX,pendingLavaProjection=UINT32_MAX;
        auto finishLava = [&]() {
            if(pendingLava==UINT32_MAX)return;
            finishWater();
            if(waterRenderer->drawLavaLighting(worker,pendingLava,depthState,framebuffer.viewport)) {
                worker->commandList->setFramebuffer(depthState ? fbStorage->colorWriteDepthRead.get() : fbStorage->colorDepthWrite.get());
                worker->commandList->drawInstanced(3,1,0,0);
            }
            pendingLava=UINT32_MAX;pendingLavaProjection=UINT32_MAX;
            switchToGraphicsPipeline();
        };

'''
def framebuffer(s):
    s=replace(s,'                instanceDrawCall.waterIndex = UINT32_MAX;',
'''                instanceDrawCall.waterIndex = UINT32_MAX;
                instanceDrawCall.worldProjection=false;
                instanceDrawCall.projectionIndex=UINT32_MAX;''')
    s=replace(s,'        for (uint32_t i : rasterScene.instanceIndices) {',finish+'        for (uint32_t i : rasterScene.instanceIndices) {')
    s=replace(s,'            const InstanceDrawCall &drawCall = instanceDrawCallVector[i];',
'''            const InstanceDrawCall &drawCall = instanceDrawCallVector[i];
            if(pendingLava!=UINT32_MAX && (!drawCall.worldProjection || drawCall.projectionIndex!=pendingLavaProjection))finishLava();
            if(pendingLava==UINT32_MAX && drawCall.waterIndex!=UINT32_MAX) {
                const auto& m=waterRenderer->draws[drawCall.waterIndex].material;
                if(m.effects.z==5 && m.effects.w>0.5f && m.sunDirection.w>0) {
                    pendingLava=drawCall.waterIndex;pendingLavaProjection=drawCall.projectionIndex;
                }
            }''')
    s=replace(s,'        // Mark targets for resolve.\n        finishWater();',
        '        // Mark targets for resolve.\n        finishLava();\n        finishWater();')
    s=replace(s,'                        switch (proj.type) {',
'''                        instanceDrawCall.worldProjection=proj.type==Projection::Type::Perspective;
                        instanceDrawCall.projectionIndex=proj.transformsIndex;
                        switch (proj.type) {''')
    s=replace(s,'bool hardware=!classic && wp.material.appearance.x>0 && wp.material.effects.w>0.5f &&',
        'bool hardware=!classic && (wp.material.appearance.x>0 || (wp.material.effects.z==5 && wp.material.sunDirection.w>0)) && wp.material.effects.w>0.5f &&')
    s=replace(s,'                    wp.material.effects.w=hardware ? 1.0f : 0.0f;',
'''                    wp.material.effects.w=hardware ? 1.0f : 0.0f;
                    if(hardware && wp.material.effects.z==5 && wp.material.sunDirection.w>0 &&
                       !waterRenderer->lavaLightingPipeline(p.worker->device,p.rasterShaderCache->shaderFormat,p.fbStorage->colorTarget->multisampling))
                        wp.material.sunDirection.w=0;''')
    return s
change('src/render/rt64_framebuffer_renderer.cpp',framebuffer)
def lava_shader(s):
    s=replace(s,'        float4 reflection=0;\n#ifdef HARDWARE_WATER',
        '        float4 reflection=0;\n        if(m.appearance.x>0) {\n#ifdef HARDWARE_WATER')
    return replace(s,'#endif\n        float fresnel=0.04', '#endif\n        }\n        float fresnel=0.04')
change('src/shaders/WaterPS.hlsl',lava_shader)
output.write_text(''.join(changes),newline='\n')
print(f'Composed {output.name}: {len(changes)} diff records')
