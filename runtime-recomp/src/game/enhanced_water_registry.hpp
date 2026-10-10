#pragma once
#include "enhanced_water_policy.hpp"
#include "enhanced_water_ray_geometry.hpp"
#include <bit>
#include <memory>
#include <vector>
#include <cstring>

namespace dkr::runtime::water::enhanced {

// All offsets are reviewed decomp struct offsets, shared by US v77/v80.
// Inputs are a graphics task's immutable, word-swapped RDRAM snapshot.
class Memory {
    std::span<const std::uint8_t> bytes_;
public:
    explicit Memory(std::span<const std::uint8_t> bytes) : bytes_(bytes) {}
    bool valid(std::uint32_t p, std::uint64_t size) const {
        if (p>=0x80000000U && p<0xA0000000U) p &= 0x1FFFFFFFU;
        return std::uint64_t(p)+size<=bytes_.size();
    }
    std::uint8_t u8(std::uint32_t p) const {
        p &= 0x1FFFFFFFU; return (p^3U)<bytes_.size() ? bytes_[p^3U] : 0;
    }
    std::uint16_t u16(std::uint32_t p) const { return (std::uint16_t(u8(p))<<8U)|u8(p+1); }
    std::int16_t s16(std::uint32_t p) const { return std::bit_cast<std::int16_t>(u16(p)); }
    std::uint32_t u32(std::uint32_t p) const { return (std::uint32_t(u16(p))<<16U)|u16(p+2); }
    float f32(std::uint32_t p) const { return std::bit_cast<float>(u32(p)); }
    Vec3 vertex(std::uint32_t p) const { return {float(s16(p)),float(s16(p+2)),float(s16(p+4))}; }
};

struct Surface {
    std::uint32_t begin = 0, end = 0, batch = 0;
    Kind kind = Kind::None;
    float height = 0;
};

struct WaveSymbols {
    std::uint32_t controller, players, batch, triangles, fallback, texture, model;
};
constexpr WaveSymbols wave_symbols(bool us_v80) {
    // Reviewed waves_alloc/waves_free/waves_render addresses. The v80 data
    // arrays move by 0x590, while its BSS controller moves by 0x5C0.
    return us_v80
        ? WaveSymbols{0x8012A588U,0x8012A638U,0x8012A63CU,0x800E3610U,
                      0x800E3620U,0x800E3660U,0x800E3668U}
        : WaveSymbols{0x80129FC8U,0x8012A078U,0x8012A07CU,0x800E3080U,
                      0x800E3090U,0x800E30D0U,0x800E30D8U};
}

class Registry {
    struct WetTriangle { Vec3 a,b,c; };
    std::vector<Surface> surfaces_;
    std::vector<WetTriangle> wet_triangles_;
    struct VerticalImage {std::uint32_t begin,end;Kind kind;};
    std::vector<VerticalImage> vertical_images_;
    std::optional<Surface> procedural_;
    std::optional<Metadata> metadata_;
    std::shared_ptr<const RayGeometry> ray_geometry_;
public:
    const std::shared_ptr<const RayGeometry>& ray_geometry() const {return ray_geometry_;}
    const std::optional<Metadata>& metadata() const {return metadata_;}
    std::span<const Surface> surfaces() const { return surfaces_; }
    const std::optional<Surface>& procedural() const { return procedural_; }
    Kind vertical_image_kind(std::uint32_t physical) const {
        physical &= 0x1FFFFFFFU;
        for(const auto& image:vertical_images_)
            if(physical>=image.begin && physical<image.end)return image.kind;
        return Kind::None;
    }
    std::optional<float> liquid_height(float x,float z,float near_y=0) const {
        if(!std::isfinite(x) || !std::isfinite(z) || !std::isfinite(near_y))return {};
        std::optional<float> best;
        for(const auto& t:wet_triangles_) {
            if(x<(std::min)({t.a.x,t.b.x,t.c.x}) || x>(std::max)({t.a.x,t.b.x,t.c.x}) ||
               z<(std::min)({t.a.z,t.b.z,t.c.z}) || z>(std::max)({t.a.z,t.b.z,t.c.z}))continue;
            const float denominator=(t.b.z-t.c.z)*(t.a.x-t.c.x)+(t.c.x-t.b.x)*(t.a.z-t.c.z);
            if(std::abs(denominator)<0.0001F)continue;
            const float a=((t.b.z-t.c.z)*(x-t.c.x)+(t.c.x-t.b.x)*(z-t.c.z))/denominator;
            const float b=((t.c.z-t.a.z)*(x-t.c.x)+(t.a.x-t.c.x)*(z-t.c.z))/denominator;
            if(a>=-0.0001F && b>=-0.0001F && a+b<=1.0001F) {
                const float height=a*t.a.y+b*t.b.y+(1-a-b)*t.c.y;
                if(!best || std::abs(height-near_y)<std::abs(*best-near_y))best=height;
            }
        }
        return best;
    }
    std::optional<Surface> find(std::uint32_t physical, std::uint32_t triangles) const {
        physical &= 0x1FFFFFFFU;
        if(!triangles) return {};
        const auto it=std::upper_bound(surfaces_.begin(),surfaces_.end(),physical,
            [](std::uint32_t p,const Surface& s){return p<s.begin;});
        if(it==surfaces_.begin()) return {};
        const auto& s=*std::prev(it);
        if(physical<s.begin || (physical-s.begin)%16 ||
            std::uint64_t(physical)+std::uint64_t(triangles)*16>s.end) return {};
        return s;
    }
    std::optional<Surface> find_dynamic(Memory m,WaveSymbols symbols,
        std::uint32_t physical,std::uint32_t triangles) const {
        // waves_render submits rows from gWaveTriangles, NOT level triangles.
        // Its interpolation identity is optional and cannot own material
        // classification. Read this task's snapshot each time: allocations
        // can change without the cached static level model changing.
        if(!procedural_ || !triangles || !m.valid(symbols.controller,4) ||
            !m.valid(symbols.players,4) || !m.valid(symbols.batch,4) ||
            !m.valid(symbols.triangles,16) || !m.valid(symbols.texture,4) ||
            !m.valid(symbols.model,4)) return {};
        if((m.u32(symbols.batch)&0x1FFFFFFFU)!=procedural_->batch) return {};
        const auto model=m.u32(symbols.model),texture=m.u32(symbols.texture);
        if(!model || !texture || !m.valid(model,0x1C) || !m.valid(texture,32)) return {};
        const auto n=m.u32(symbols.controller),players=m.u32(symbols.players);
        // Two vertex rows must fit the retail 32-entry RSP vertex cache.
        if(n<1 || n>15 || players<1 || players>4) return {};
        const auto bytes=32U*n*n, buffers=players==2 ? 4U : 2U;
        const auto base=m.u32(symbols.triangles);
        if(!base || (base&7U) || !m.valid(base,std::uint64_t(bytes)*buffers)) return {};
        for(unsigned i=1;i<buffers;++i)
            if(std::uint64_t(m.u32(symbols.triangles+i*4))!=std::uint64_t(base)+bytes*i) return {};
        physical &= 0x1FFFFFFFU;
        const auto bounded=[&](std::uint32_t begin,std::uint32_t end)->std::optional<Surface> {
            if(physical<begin || (physical-begin)%16 ||
                std::uint64_t(physical)+std::uint64_t(triangles)*16>end) return {};
            auto surface=*procedural_;surface.begin=begin;surface.end=end;
            return surface;
        };
        for(unsigned i=0;i<buffers;++i) {
            const auto begin=(base&0x1FFFFFFFU)+bytes*i;
            if(auto surface=bounded(begin,begin+bytes)) return surface;
        }
        // Distant/low-density tiles use one of two authored two-triangle
        // quads. Do not let a draw span the opposite frame's quad.
        if(m.valid(symbols.fallback,64)) for(unsigned i=0;i<2;++i) {
            const auto begin=(symbols.fallback&0x1FFFFFFFU)+i*32;
            if(auto surface=bounded(begin,begin+32)) return surface;
        }
        return {};
    }
    void rebuild(Memory m,std::uint32_t model,std::uint32_t cache,std::uint32_t texture_count,
                 std::uint32_t procedural_batch,std::optional<Metadata> metadata={},bool collect_rays=false) {
        surfaces_.clear(); wet_triangles_.clear(); vertical_images_.clear(); procedural_.reset();
        const auto previous_rays=std::move(ray_geometry_);
        auto rays=collect_rays ? std::make_shared<RayGeometry>() : nullptr;
        metadata_=std::move(metadata);
        if(!m.valid(model,0x4C) || !model) return;
        const auto textures=m.u32(model),segments=m.u32(model+4);
        const auto nt=m.u16(model+0x18),ns=m.u16(model+0x1A);
        if(nt>256 || ns>127 || !m.valid(textures,std::uint64_t(nt)*8) ||
            !m.valid(segments,std::uint64_t(ns)*0x44) || texture_count>2048 ||
            !m.valid(cache,std::uint64_t(texture_count)*8)) return;
        // Waterfall/fountain artwork can be carried by an object model, not
        // the level segment table. Recognize only reviewed liquid texture IDs
        // and validated texel ranges, never a sprite name or arbitrary alpha.
        // Camera provenance at the draw bridge excludes the ordinary 2D HUD.
        for(std::uint32_t c=0;c<texture_count;++c) {
            const auto id=texture_3d_id(m.u32(cache+c*8));
            const auto ptr=m.u32(cache+c*8+4);
            const Kind kind=id==1241 ? Kind::Fountain :
                (id==317 || id==1177 || id==1391) ? Kind::Waterfall : Kind::None;
            if(kind==Kind::None || !ptr || !m.valid(ptr,32))continue;
            const auto frames=(std::max)(1U,std::uint32_t(m.u16(ptr+0x12)>>8));
            const auto stride=m.u16(ptr+0x16);
            if(frames>256 || stride<32 || !m.valid(ptr,std::uint64_t(frames)*stride))continue;
            for(std::uint32_t frame=0;frame<frames && vertical_images_.size()<2048;++frame) {
                const auto header=ptr+frame*stride;
                const auto pixels=std::uint32_t(m.u8(header))*m.u8(header+1),format=m.u8(header+2)&15U;
                const auto bytes=format==0 ? pixels*4 : (format==1 || format==4) ? pixels*2 :
                    (format==2 || format==5 || format==8) ? pixels :
                    (format==3 || format==6 || format==7) ? (pixels+1)/2 : 0;
                if(!bytes || bytes>stride-32 || !m.valid(header+32,bytes))continue;
                vertical_images_.push_back({(header+32)&0x1FFFFFFFU,(header+32+bytes)&0x1FFFFFFFU,kind});
            }
        }
        std::vector<MaterialEvidence> materials(nt);
        std::uint32_t total_batches=0,total_triangles=0;
        for(std::uint32_t t=0;t<nt;++t) {
            auto& e=materials[t]; const auto info=textures+t*8,ptr=m.u32(info);
            e.texture_valid=ptr && m.valid(ptr,0x20); e.texture_id=65535;
            e.surface_type=m.u8(info+7);
            if(metadata_)e.hint=metadata_->materials[t];
            for(std::uint32_t c=0;c<texture_count;++c) if(m.u32(cache+c*8+4)==ptr) {
                if(const auto id=texture_3d_id(m.u32(cache+c*8))) e.texture_id=*id;
                break;
            }
        }
        for(std::uint32_t s=0;s<ns;++s) {
            const auto seg=segments+s*0x44;
            const auto verts=m.u32(seg),tris=m.u32(seg+4),batches=m.u32(seg+12);
            const auto nv=m.u16(seg+0x1C),nf=m.u16(seg+0x1E),nb=m.u16(seg+0x20);
            if(nv>32767 || nf>32767 || nb>4096 ||
                !m.valid(verts,std::uint64_t(nv)*10) || !m.valid(tris,std::uint64_t(nf)*16) ||
                !m.valid(batches,std::uint64_t(nb+1)*12)) continue;
            for(std::uint32_t b=0;b<nb;++b) {
                if(++total_batches>8192) {surfaces_.clear();wet_triangles_.clear();procedural_.reset();return;}
                const auto batch=batches+b*12;
                const auto texture=m.u8(batch);
                const auto v=m.u16(batch+2),f=m.u16(batch+4);
                const auto ve=m.u16(batch+14),fe=m.u16(batch+16);
                if((texture>=nt && texture!=255) || ve<=v || ve>nv || fe<=f || fe>nf) continue;
                total_triangles+=fe-f;
                if(total_triangles>262144) {surfaces_.clear();wet_triangles_.clear();procedural_.reset();return;}
                auto e=texture==255 ? MaterialEvidence{} : materials[texture];
                e.flags=m.u32(batch+8); e.geometry_valid=true;
                auto kind=classify(e);
                if(kind==Kind::None) {
                    const auto texture_flags=texture==255 ? 0 : m.u16(m.u32(textures+texture*8)+6);
                    if(!rays || (texture!=255 && !e.texture_valid) || !opaque_ray_material(e.flags,texture_flags))continue;
                    for(std::uint32_t i=f;i<fe;++i) {
                        const auto tri=tris+i*16;
                        const std::array<std::uint32_t,3> indices{m.u8(tri+1),m.u8(tri+2),m.u8(tri+3)};
                        if(std::any_of(indices.begin(),indices.end(),[&](auto n){return n>=ve-v;}))continue;
                        std::array<RayVertex,3> vertices{};
                        bool opaque=true;
                        for(unsigned j=0;j<3;++j) {
                            const auto p=verts+(v+indices[j])*10;
                            const auto at=m.vertex(p);
                            vertices[j].position={at.x,at.y,at.z,1};
                            vertices[j].colour={m.u8(p+6)/255.0F,m.u8(p+7)/255.0F,m.u8(p+8)/255.0F,1};
                            opaque &= m.u8(p+9)==255;
                        }
                        const auto p=[&](unsigned j){return Vec3{vertices[j].position[0],vertices[j].position[1],vertices[j].position[2]};};
                        if(opaque && valid_triangle(p(0),p(1),p(2))) {
                            if(rays->vertices.size()/3>=kMaximumRayTriangles) {
                                // Never trace a silently truncated world: it
                                // would produce holes. This level uses SSR.
                                rays.reset();break;
                            }
                            rays->vertices.insert(rays->vertices.end(),vertices.begin(),vertices.end());
                        }
                    }
                    continue;
                }
                double height=0; std::uint32_t points=0;
                const auto wet_begin=wet_triangles_.size();
                for(std::uint32_t i=f;i<fe && e.geometry_valid;++i) {
                    const auto tri=tris+i*16;
                    const auto a=m.u8(tri+1),bb=m.u8(tri+2),c=m.u8(tri+3);
                    if(a>=ve-v || bb>=ve-v || c>=ve-v) {e.geometry_valid=false;break;}
                    const auto pa=m.vertex(verts+(v+a)*10),pb=m.vertex(verts+(v+bb)*10),pc=m.vertex(verts+(v+c)*10);
                    if(!valid_triangle(pa,pb,pc)) continue; // Zero-area separators carry no surface.
                    height+=pa.y+pb.y+pc.y; points+=3;
                    if((kind==Kind::Calm || kind==Kind::Waves) && wet_triangles_.size()<8192)
                        wet_triangles_.push_back({pa,pb,pc});
                }
                if(!e.geometry_valid || !points) {wet_triangles_.resize(wet_begin);continue;}
                Surface surface{(tris+f*16)&0x1FFFFFFFU,(tris+fe*16)&0x1FFFFFFFU,
                    batch&0x1FFFFFFFU,kind,static_cast<float>(height/points)};
                surfaces_.push_back(surface);
                if((procedural_batch&0x1FFFFFFFU)==surface.batch && kind==Kind::Waves)
                    procedural_=surface;
            }
        }
        std::sort(surfaces_.begin(),surfaces_.end(),[](const Surface& a,const Surface& b){return a.begin<b.begin;});
        if(rays && !rays->vertices.empty()) {
            // Texture-cache churn is not a new static world. Exact comparison
            // keeps the submitted GPU scene alive without hash collisions or
            // accidentally reusing a different custom track at the same address.
            if(previous_rays && previous_rays->vertices.size()==rays->vertices.size() &&
                std::memcmp(previous_rays->vertices.data(),rays->vertices.data(),rays->vertices.size()*sizeof(RayVertex))==0)
                ray_geometry_=previous_rays;
            else ray_geometry_=std::move(rays);
        }
    }
};
}
