#include "vehicle_geometry.hpp"
#include "game_core/src/ecstasy/estripdatae.hpp"
#include "port/vehicle/topology.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <map>
#include <functional>
#include "game_core/port/not_implemented.hpp"

namespace port::assets {
namespace {
struct Reader {
    const std::vector<std::uint8_t>& data;
    void require(std::size_t offset, std::size_t count) const {
        if(offset>data.size() || count>data.size()-offset) throw std::runtime_error("Truncated vehicle geometry");
    }
    std::uint32_t u32(std::size_t offset) const {
        require(offset,4); return std::uint32_t(data[offset]) | std::uint32_t(data[offset+1])<<8 |
            std::uint32_t(data[offset+2])<<16 | std::uint32_t(data[offset+3])<<24;
    }
    std::uint16_t u16(std::size_t offset) const {require(offset,2);return std::uint16_t(data[offset]) | std::uint16_t(data[offset+1])<<8;}
    float f32(std::size_t offset) const {
        auto bits=u32(offset); float result;std::memcpy(&result,&bits,4);
        if(!std::isfinite(result)) throw std::runtime_error("Non-finite vehicle geometry component");
        return result;
    }
};
struct Chunk {std::uint32_t id; std::size_t offset,end;};
std::vector<Chunk> chunks(const Reader& r,std::size_t begin,std::size_t end) {
    std::vector<Chunk> result;
    for(auto offset=begin;offset<end;) {
        if(end-offset<8) throw std::runtime_error("Truncated vehicle chunk header");
        auto size=r.u32(offset+4);
        if(size>end-offset-8) throw std::runtime_error("Vehicle chunk exceeds parent");
        result.push_back({r.u32(offset),offset,offset+8+size});offset+=8+size;
    }
    return result;
}
std::size_t aligned_payload(const Chunk& c) {
    const auto offset=(c.offset+0x17)&~std::size_t(15);
    if(offset>c.end) throw std::runtime_error("Invalid vehicle chunk padding");
    return offset;
}
void position_markers(const Reader& r,std::size_t begin,std::size_t end,
                      std::vector<render::VehiclePositionMarker>& result) {
    // Debug001A5670: aligned chunk0x34013,32-byte records; retain file/list order.
    for(const auto& c:chunks(r,begin,end)) {
        if(c.id==0x34013) {
            const auto first=aligned_payload(c);
            const auto count=(c.end-first)/32;
            for(std::size_t i=0;i<count;++i) {
                const auto at=first+i*32;render::VehiclePositionMarker marker;
                marker.name_hash=r.u32(at+8);marker.type_hash=r.u32(at+12);
                for(unsigned axis=0;axis<3;++axis)marker.position[axis]=r.f32(at+16+axis*4);
                const auto bits=r.u32(at+28);std::memcpy(&marker.param,&bits,4);
                result.push_back(marker);
            }
        }else if(c.id&0x80000000u)position_markers(r,c.offset+8,c.end,result);
    }
}
std::optional<render::VehicleRigidAnimation> rigid_animation(const Reader& r,const Chunk& parent) {
    // Debug LoaderSolidList00103588: aligned34012; eRigidAnimation header48.
    for(const auto& c:chunks(r,parent.offset+8,parent.end))if(c.id==0x34012) {
        const auto at=aligned_payload(c);
        if(c.end-at<48)throw std::runtime_error("Short rigid animation header");
        render::VehicleRigidAnimation result;
        result.num_frames=static_cast<std::int16_t>(r.u16(at));
        result.triggerable_flag=static_cast<std::int8_t>(r.data[at+2]);
        result.frame_rate_scale=static_cast<std::int8_t>(r.data[at+3]);
        result.name_hash=r.u32(at+4);
        for(unsigned i=0;i<32&&r.data[at+16+i];++i)result.name.push_back(char(r.data[at+16+i]));
        if(result.num_frames<=0||result.frame_rate_scale<=0)
            hp2::not_implemented("0x001A4CF8","Invalid original rigid animation count/rate divisor");
        if(std::size_t(result.num_frames)>(c.end-at-48)/64)throw std::runtime_error("Rigid animation frames exceed chunk");
        for(int frame=0;frame<result.num_frames;++frame){std::array<float,16> matrix{};
            for(unsigned i=0;i<16;++i)matrix[i]=r.f32(at+48+std::size_t(frame)*64+i*4);
            result.matrices.push_back(matrix);
        }
        return result;
    }
    return {};
}
render::VehicleSolid solid(const Reader& r,const Chunk& parent) {
    std::size_t header=0,entries=0,stream=0,textures=0,materials=0,entry_end=0,stream_end=0,texture_end=0,material_end=0;
    for(const auto& c:chunks(r,parent.offset+8,parent.end)) {
        switch(c.id) {
        case 0x34003:header=aligned_payload(c);r.require(header,0xc0);if(header+0xc0>c.end)throw std::runtime_error("Short vehicle solid header");break;
        case 0x34004:entries=aligned_payload(c);entry_end=c.end;break;
        case 0x34005:stream=aligned_payload(c);stream_end=c.end;break;
        case 0x34006:textures=c.offset+8;texture_end=c.end;break;
        case 0x3401d:materials=aligned_payload(c);material_end=c.end;break;
        default:break;
        }
    }
    if(!header||!entries||!stream||!textures) throw std::runtime_error("Vehicle solid lacks required chunks");
    render::VehicleSolid result;result.hash=r.u32(header+8);
    for(std::size_t i=0;i<24 && r.data[header+0x10+i];++i)result.name.push_back(char(r.data[header+0x10+i]));
    result.external_rigid_animation_hash=r.u32(header+0x28);
    result.strip_flags=r.u16(header+0x32);
    for(std::size_t i=0;i<3;++i){result.bounds_min[i]=r.f32(header+0x40+i*4);result.bounds_max[i]=r.f32(header+0x50+i*4);}
    for(std::size_t i=0;i<16;++i)result.transform[i]=r.f32(header+0x60+i*4);
    const auto count=r.u16(header+0x34);
    if(std::size_t(count)>(entry_end-entries)/64) throw std::runtime_error("Vehicle strip table count exceeds chunk");
    for(std::size_t i=0;i<count;++i) {
        const auto entry=entries+i*64;
        const auto relative=r.u32(entry+8);
        if(relative>stream_end-stream || stream_end-stream-relative<32)throw std::runtime_error("Vehicle packet offset exceeds stream");
        const auto packet=stream+relative;
        if(r.u32(packet)!=0x6e018000 || r.u32(packet+8)!=0x6c01c001)
            throw std::runtime_error("Unsupported vehicle VIF packet layout");
        const auto packet_end=packet+(r.u32(entry+12)&0xffffu)*16;
        if(packet_end>stream_end || packet_end<packet+32)throw std::runtime_error("Vehicle packet length exceeds stream");
        const std::vector<std::uint8_t> packet_data(r.data.begin()+packet,r.data.begin()+packet_end);
        const auto vertex_count=hp2::recovered::eStripData::GetNumVerts(packet_data,0);
        if(vertex_count!=r.data[entry+0x1d])throw std::runtime_error("Vehicle packet vertex count mismatch");
        render::VehicleSurface surface;
        surface.render_flags=r.u16(entry+0xe);
        surface.packet_mode=r.data[packet+5];
        std::copy_n(r.data.begin()+entry,64,surface.strip_entry.begin());
        for(std::size_t j=0;j<4;++j)surface.packet_control[j]=r.u32(packet+12+j*4);
        const auto texture_index=r.data[entry];
        if(std::size_t(texture_index)>=(texture_end-textures)/8)throw std::runtime_error("Vehicle texture index exceeds table");
        surface.texture_hash=r.u32(textures+texture_index*8);
        const auto material_index=r.data[entry+0x1f];
        if(materials && material_index!=0xff) {
            if(std::size_t(material_index)>=(material_end-materials)/8)throw std::runtime_error("Vehicle material index exceeds table");
            surface.material_hash=r.u32(materials+material_index*8);
        }
        const std::size_t color_begin=vertex_count*20+0x28+
            (vertex_count%4&&vertex_count/4?4:0)+(vertex_count%2&&vertex_count/2?4:0);
        const std::size_t normal_command=color_begin+((vertex_count+1)/2)*4;
        const std::size_t normal_groups=(vertex_count+3)/4;
        Reader packet_reader{packet_data};
        const auto command=packet_reader.u32(normal_command);
        const bool has_normals=command==(0x6e00805cu|std::uint32_t(normal_groups*3)<<16);
        if(!has_normals&&(command!=0x14000000||(surface.render_flags&0x1c0)))
            throw std::runtime_error("Unsupported vehicle normal VIF layout");
        if(has_normals)packet_reader.require(normal_command+4,normal_groups*12);
        // Original VU2D4 scalar bits, not normalization by a host engine.
        std::uint32_t normal_scale_bits=0x3c010204;float normal_scale;
        std::memcpy(&normal_scale,&normal_scale_bits,4);
        for(std::size_t j=0;j<vertex_count;++j) {
            render::VehicleVertex vertex;
            vertex.position=hp2::recovered::eStripData::GetXYZ(packet_data,0,j);
            vertex.uv=hp2::recovered::eStripData::GetST0(packet_data,0,j);
            vertex.raw_color=hp2::recovered::eStripDataE::GetElementRGBA(packet_data,0,j);
            vertex.has_color=true;
            for(std::size_t component=0;has_normals&&component<3;++component){
                const auto raw=packet_data[normal_command+4+(j/4)*12+component*4+j%4];
                const auto signed_value=raw<128?int(raw):int(raw)-256;
                vertex.raw_normal[component]=static_cast<std::int8_t>(signed_value);
                vertex.normal[component]=float(signed_value)*normal_scale;
            }
            vertex.has_normal=has_normals;
            for(auto v:vertex.position)if(!std::isfinite(v))throw std::runtime_error("Non-finite vehicle position");
            for(auto v:vertex.uv)if(!std::isfinite(v))throw std::runtime_error("Non-finite vehicle UV");
            surface.vertices.push_back(vertex);
        }
        const auto adc=vehicle::adc_002A76E0(vertex_count,surface.packet_mode,surface.packet_control);
        for(std::uint32_t j=2;j<vertex_count;++j) {
            if(adc[j])continue;
            // Port-side triangle-list expansion of the original strip stream.
            // ADC suppresses drawing while retaining the two previous vertices.
            if(j&1)surface.indices.insert(surface.indices.end(),{j-1,j-2,j});
            else surface.indices.insert(surface.indices.end(),{j-2,j-1,j});
        }
        if(surface.indices.size()/3!=r.data[entry+0x1e])
            throw std::runtime_error("Vehicle VU topology does not match original triangle count");
        result.surfaces.push_back(std::move(surface));
    }
    return result;
}
}
std::vector<render::VehiclePositionMarker> decode_vehicle_position_markers(const std::vector<std::uint8_t>& data) {
    Reader reader{data};std::vector<render::VehiclePositionMarker> result;
    position_markers(reader,0,data.size(),result);return result;
}
render::VehicleGeometry decode_vehicle_geometry(const std::vector<std::uint8_t>& data,const std::set<std::uint32_t>& hashes) {
    Reader reader{data};render::VehicleGeometry result;bool found=false;
    position_markers(reader,0,data.size(),result.position_markers);
    struct AnimationEntry {std::uint32_t external{};std::optional<render::VehicleRigidAnimation> animation;};
    std::map<std::uint32_t,AnimationEntry> animations;
    for(const auto& root:chunks(reader,0,data.size()))if(root.id==0x80034000)
        for(const auto& child:chunks(reader,root.offset+8,root.end))if(child.id==0x80034002)
            for(const auto& entry:chunks(reader,child.offset+8,child.end))if(entry.id==0x34003){
                const auto at=aligned_payload(entry);if(entry.end-at<0xc0)throw std::runtime_error("Short vehicle solid header");
                animations.emplace(reader.u32(at+8),AnimationEntry{reader.u32(at+0x28),rigid_animation(reader,child)});
            }
    std::set<std::uint32_t> resolving;bool unresolved=false;
    std::function<std::optional<render::VehicleRigidAnimation>(std::uint32_t)> resolve=[&](std::uint32_t hash){
        const auto it=animations.find(hash);
        if(it==animations.end()){unresolved=true;return std::optional<render::VehicleRigidAnimation>{};}
        if(!resolving.insert(hash).second){unresolved=true;return std::optional<render::VehicleRigidAnimation>{};}
        auto animation=it->second.animation;
        if(it->second.external)animation=resolve(it->second.external);
        resolving.erase(hash);return animation;
    };

    for(const auto& c:chunks(reader,0,data.size())) {
        if(c.id!=0x80034000)continue;
        found=true;
        for(const auto& child:chunks(reader,c.offset+8,c.end))if(child.id==0x80034002){
            bool wanted=hashes.empty();
            for(const auto& entry:chunks(reader,child.offset+8,child.end))if(entry.id==0x34003){
                const auto at=aligned_payload(entry);reader.require(at,12);
                if(at+12>entry.end)throw std::runtime_error("Short vehicle solid identity");
                wanted=wanted||hashes.count(reader.u32(at+8));
            }
            if(wanted){auto decoded=solid(reader,child);unresolved=false;decoded.rigid_animation=resolve(decoded.hash);decoded.rigid_animation_unresolved=unresolved;if(unresolved)result.gaps.push_back("Unresolved original external rigid animation for passive solid "+decoded.name+"; active animation requires bound external resources");result.solids.push_back(std::move(decoded));}
        }
    }
    if(!found||result.solids.empty())throw std::runtime_error("Vehicle geometry container not found");
    result.gaps.push_back("Geometry, UV and ADC topology decoded; original GS materials and partial-view culling remain incomplete");
    return result;
}
}
