#include "assets.hpp"
#include "port/assets/vehicle_geometry.hpp"
#include "game_core/src/ecstasy/elighte.hpp"
#include "game_core/src/ecstasy/ecstasye.hpp"
#include "game_core/port/not_implemented.hpp"
#include "texture_decode.hpp"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
#include <sstream>

namespace port::track {
namespace {
std::uint32_t word(const std::vector<std::uint8_t>& b,std::size_t p) {
    if(p>b.size()||b.size()-p<4)throw std::runtime_error("Truncated track word");
    return std::uint32_t(b[p])|std::uint32_t(b[p+1])<<8|std::uint32_t(b[p+2])<<16|std::uint32_t(b[p+3])<<24;
}
std::string text(const std::vector<std::uint8_t>& b,std::size_t p,std::size_t count) {
    if(p>b.size()||count>b.size()-p)throw std::runtime_error("Truncated track text");
    const auto end=std::find(b.begin()+p,b.begin()+p+count,0);
    return {b.begin()+p,end};
}
template<class F>void chunks(const std::vector<std::uint8_t>& b,F f) {
    for(std::size_t p=0;p<b.size();) {
        const auto id=word(b,p),size=word(b,p+4);
        if(size>b.size()-p-8)throw std::runtime_error("Track chunk exceeds file");
        f(id,p+8,std::size_t(size));p+=8+size;
    }
}
// E-0048-A06: ownership/bounds adapter; numerical queries live in original classes.
void world_context(Scene& scene) {
    const auto& b=scene.bundle;std::vector<LightPack> packs;
    auto half=[&](std::size_t at){if(at>b.size()||b.size()-at<2)throw std::runtime_error("Truncated world-light halfword");return static_cast<std::int16_t>(std::uint16_t(b[at])|std::uint16_t(b[at+1])<<8);};
    auto scalar=[&](std::size_t at){auto bits=word(b,at);float f;std::memcpy(&f,&bits,4);if(!std::isfinite(f))throw std::runtime_error("Nonfinite world-light value");return f;};
    chunks(b,[&](auto id,auto payload,auto length){
        if(id!=0x80034015)return;
        std::size_t tree=0,tree_end=0,lights=0,light_end=0;bool supported=false;
        for(auto at=payload;at<payload+length;){
            if(payload+length-at<8)throw std::runtime_error("Truncated world-light chunk");
            auto child=word(b,at),size=word(b,at+4);if(size>payload+length-at-8)throw std::runtime_error("World-light chunk exceeds pack");
            const auto aligned=(at+0x17)&~std::size_t(15);
            if(aligned>at+8+size)throw std::runtime_error("World-light alignment exceeds child");
            if(child==0x34016){if(at+8+size-aligned<12)throw std::runtime_error("Truncated light pack header");supported=word(b,aligned+8)==1;}
            if(child==0x34017){tree=aligned;tree_end=at+8+size;}
            if(child==0x34018){lights=aligned;light_end=at+8+size;}
            at+=size+8;
        }
        if(!supported)throw std::runtime_error("Unsupported original light pack version");
        if(!tree||!lights||tree_end-tree<16||(light_end-lights)%96)throw std::runtime_error("Incomplete original light pack");
        const auto count=half(tree+8);tree+=16;
        if(count<1||std::size_t(count)>(tree_end-tree)/48)throw std::runtime_error("Invalid light tree node count");
        LightPack pack;
        for(int n=0;n<count;++n){
            const auto at=tree+std::size_t(n)*48;hp2::recovered::vAABB node;
            node.PositionX=scalar(at);node.PositionY=scalar(at+4);node.PositionZ=scalar(at+8);
            node.ParentIndex=half(at+12);node.NumChildren=half(at+14);
            node.ExtentX=scalar(at+16);node.ExtentY=scalar(at+20);node.ExtentZ=scalar(at+24);
            if(node.NumChildren < -10||node.NumChildren>10)throw std::runtime_error("Light tree child count exceeds ten");
            for(std::size_t j=0;j<10;++j)node.ChildrenIndicies[j]=half(at+28+j*2);
            pack.tree.NodeArray.push_back(node);
        }
        std::vector<unsigned char> visits(count);
        const auto validate=[&](auto&& self,int index)->void {
            if(index<0||index>=count)throw std::runtime_error("Light tree index exceeds table");
            if(visits[index]==1)throw std::runtime_error("Cyclic light tree");if(visits[index]==2)return;
            visits[index]=1;const auto& node=pack.tree.NodeArray[index];
            for(int n=0;n<node.NumChildren;++n)self(self,node.ChildrenIndicies[n]);visits[index]=2;
        };
        for(int n=0;n<count;++n)validate(validate,n);
        for(auto at=lights;at<light_end;at+=96){
            hp2::recovered::eLight light;light.Type=b[at+36];light.AttenuationType=b[at+37];light.Colour=word(b,at+44);
            light.PositionX=scalar(at+48);light.PositionY=scalar(at+52);light.PositionZ=scalar(at+56);light.Size=scalar(at+60);
            light.Intensity=scalar(at+76);light.FarStart=scalar(at+80);pack.lights.push_back(light);
        }
        for(const auto& node:pack.tree.NodeArray)for(int n=0;n < -node.NumChildren;++n)
            if(node.ChildrenIndicies[n]<0||std::size_t(node.ChildrenIndicies[n])>=pack.lights.size())throw std::runtime_error("Original light leaf index exceeds pack");
        packs.push_back(std::move(pack));
    });
    scene.light_packs=std::move(packs);
    scene.world_lights=hp2::recovered::elSetupLightContext(scene.light_packs);scene.world_lights_available=true;
}

}
// E-0048-A01: Retail GLOBALB 34201 and Retail 00153608 / 00153420.
std::vector<Entry> load_catalog(assets::Source& source) {
    auto b=source.read("GLOBAL/GLOBALB.BUN");std::vector<Entry> result;bool found=false;
    chunks(b,[&](auto id,auto p,auto size){
        if(id!=0x34201)return;found=true;
        if(size%0xa4)throw std::runtime_error("Retail TrackInfo table stride mismatch");
        for(std::size_t n=0;n<size/0xa4;++n){
            auto at=p+n*0xa4;auto name=text(b,at,32);if(name.empty()||n==53)continue;
            Entry e;e.number=int(n+1);e.resource_number=b[at+0x41];e.name=std::move(name);
            e.location=text(b,at+32,32);e.time_of_day=b[at+0x44];
            // Each table record already identifies its time-specific resource number.
            const auto number=std::to_string(e.resource_number);
            e.resources={"TRACKS/TRACKA"+number+".BUN","TRACKS/TRACKB"+number+".LZC",
                "TRACKS/TEX"+number+"LOCATION.BIN","TRACKS/TEX"+number+"TRACK.BIN"};
            result.push_back(std::move(e));
        }
    });
    if(!found||result.empty())throw std::runtime_error("Original Retail track catalogue not found");
    return result;
}
// Bounded host translation of Debug LZDecompress 001B1658; E-0047-002.
// COMP framing is cross-checked against every supplied Retail TRACKB asset.
std::vector<std::uint8_t> decompress_bundle(const std::vector<std::uint8_t>& b) {
    if(b.size()<4||word(b,0)!=0x504d4f43)return b;
    if(b.size()<16||word(b,4)>0x11001||((word(b,4)&0xffff)!=0x1001)||word(b,12)!=b.size())
        throw std::runtime_error("Unsupported or truncated track COMP header");
    const auto expected=word(b,8);if(expected>256u*1024*1024)throw std::runtime_error("Track COMP allocation exceeds host limit");
    std::vector<std::uint8_t> out;out.reserve(expected);
    if(word(b,4)>>16==1)out.assign(b.begin()+16,b.end());
    else {
        std::size_t p=16;std::uint32_t flags=1;
        while(p<b.size()) {
            if(flags==1){if(b.size()-p<2)throw std::runtime_error("Truncated COMP flags");flags=std::uint32_t(b[p])|(std::uint32_t(b[p+1])<<8)|0x10000;p+=2;}
            if(flags&1){
                if(b.size()-p<2)throw std::runtime_error("Truncated COMP backreference");
                const auto a=b[p++],v=b[p++];const std::size_t distance=((a&0xf0)<<4)|v,count=(a&15)+3;
                if(!distance||distance>out.size()||count>expected-out.size())throw std::runtime_error("Invalid COMP backreference");
                for(std::size_t i=0;i<count;++i)out.push_back(out[out.size()-distance]);
            }else {if(p==b.size()||out.size()==expected)throw std::runtime_error("COMP literal exceeds output");out.push_back(b[p++]);}
            flags>>=1;
        }
    }
    if(out.size()!=expected)throw std::runtime_error("Track COMP output size mismatch");
    return out;
}
Scene load_scene(assets::Source& source,const Entry& entry) {
    if(entry.resources.size()!=4)throw std::invalid_argument("Track entry lacks original resource paths");
    Scene result;result.entry=entry;
    // Empty TRACKA is present in all 30 shipped records; do not confuse it with a failed read.
    const auto a=source.read(entry.resources[0]);
    result.bundle=decompress_bundle(source.read(entry.resources[1]));
    result.geometry=assets::decode_vehicle_geometry(result.bundle);
    world_context(result);
    result.environment=decode_environment(result.bundle);
    result.particles=decode_particles(result.bundle);
    // Explicit host projection of Debug Scenery layouts onto independently checked
    // Retail chunk spans; E-0048-A02. No pointer-layout reinterpretation.
    chunks(result.bundle,[&](auto id,auto p,auto size){
        if(id!=0x80034100)return;
        std::size_t info=0,info_count=0,instances=0,instance_count=0,header=0,tree=0,tree_count=0;
        const auto& bytes=result.bundle;
        for(auto at=p;at<p+size;){
            if(p+size-at<8)throw std::runtime_error("Truncated scenery child header");
            auto child=word(bytes,at),length=word(bytes,at+4);
            if(length>p+size-at-8)throw std::runtime_error("Scenery child exceeds section");
            if(child==0x34101){header=(at+0x17)&~std::size_t(15);if(header+56>at+8+length)throw std::runtime_error("Truncated scenery section header");}
            if(child==0x34104){tree=at+8;if(length%24)throw std::runtime_error("Scenery tree stride mismatch");tree_count=length/24;}
            if(child==0x34102){info=at+8;if(length%40)throw std::runtime_error("Scenery info stride mismatch");info_count=length/40;}
            if(child==0x34103){instances=(at+0x17)&~std::size_t(15);if(instances>at+8+length||(at+8+length-instances)%48)throw std::runtime_error("Scenery instance stride mismatch");instance_count=(at+8+length-instances)/48;}
            at+=8+length;
        }
        if(!info||!instances)throw std::runtime_error("Incomplete scenery section");
        auto half=[&](auto at){if(at+2>bytes.size())throw std::runtime_error("Truncated scenery halfword");return std::uint16_t(bytes[at]|std::uint16_t(bytes[at+1])<<8);};
        auto scalar=[&](auto at){auto raw=word(bytes,at);float f;std::memcpy(&f,&raw,4);if(!std::isfinite(f))throw std::runtime_error("Nonfinite scenery position");return f;};
        Section section;section.instance_begin=result.instances.size();section.instance_count=instance_count;
        if(!header)throw std::runtime_error("Missing scenery section header");section.number=static_cast<int>(word(bytes,header+8));
        for(std::size_t n=0;n<tree_count;++n){SceneryNode node;const auto row=tree+n*24;for(unsigned k=0;k<3;++k){node.minimum[k]=static_cast<std::int16_t>(half(row+k*2));node.maximum[k]=static_cast<std::int16_t>(half(row+6+k*2));}const auto count=static_cast<std::int16_t>(half(row+12));if(count<0||count>5)throw std::runtime_error("Invalid scenery node child count");for(int k=0;k<count;++k){const auto code=static_cast<std::int16_t>(half(row+14+k*2));if(code<0?std::size_t(-int(code))>=tree_count:std::size_t(code)>=instance_count)throw std::runtime_error("Scenery child exceeds table");node.children.push_back(code);}section.nodes.push_back(std::move(node));}
        std::vector<std::size_t> incoming(section.nodes.size());
        for(const auto& node:section.nodes){for(unsigned k=0;k<3;++k)if(node.minimum[k]>node.maximum[k])throw std::runtime_error("Inverted scenery node bounds");for(auto code:node.children)if(code<0)++incoming[std::size_t(-int(code))];}
        std::vector<std::size_t> roots;for(std::size_t k=0;k<incoming.size();++k)if(!incoming[k])roots.push_back(k);
        for(std::size_t k=0;k<roots.size();++k)for(auto code:section.nodes[roots[k]].children)if(code<0&&!--incoming[std::size_t(-int(code))])roots.push_back(std::size_t(-int(code)));
        if(roots.size()!=section.nodes.size())throw std::runtime_error("Cyclic scenery node references");
        if(section.number<0||std::any_of(result.sections.begin(),result.sections.end(),[&](const auto& existing){return existing.number==section.number;}))throw std::runtime_error("Invalid or duplicate original scenery section number");
        result.sections.push_back(std::move(section));
        for(std::size_t n=0;n<instance_count;++n){
            auto at=instances+n*48;auto index=half(at+12);if(index>=info_count)throw std::runtime_error("Scenery info index exceeds section");
            auto row=info+index*40;std::size_t lod=0;while(lod<3&&!word(bytes,row+lod*4))++lod;
            if(lod==3)throw std::runtime_error("Scenery instance lacks model hash");
            Instance instance;instance.solid_hash=word(bytes,row+lod*4);instance.flags=half(at+14);instance.facade=bytes[row+32+lod]!=0;
            for(unsigned k=0;k<3;++k){instance.lod_hashes[k]=word(bytes,row+k*4);instance.far_clip_sizes[k]=static_cast<std::int16_t>(half(row+12+k*2));instance.facades[k]=bytes[row+32+k]!=0;instance.position[k]=scalar(at+16+k*4);instance.bbox_min[k]=static_cast<std::int16_t>(half(at+k*2));instance.bbox_max[k]=static_cast<std::int16_t>(half(at+6+k*2));}
            instance.radius=scalar(row+36);
            for(unsigned k=0;k<3;++k)if(instance.bbox_min[k]>instance.bbox_max[k])throw std::runtime_error("Inverted scenery instance bounds");
            instance.transform[15]=1;
            if(instance.flags&0x200){instance.transform[0]=instance.transform[5]=instance.transform[10]=1;}
            else {
                for(std::size_t col=0;col<3;++col)for(std::size_t r=0;r<3;++r)instance.transform[col*4+r]=float(static_cast<std::int16_t>(half(at+28+(col*3+r)*2)))*(1.0f/16384.0f);
                for(std::size_t r=0;r<3;++r)instance.transform[12+r]=scalar(at+16+r*4);
            }
            result.instances.push_back(instance);
        }
    });
    chunks(result.bundle,[&](auto id,auto payload,auto length){if(id!=0x34106)return;if(length%288)throw std::runtime_error("Visible zone stride mismatch");const auto& b=result.bundle;auto scalar=[&](std::size_t at){auto bits=word(b,at);float v;std::memcpy(&v,&bits,4);if(!std::isfinite(v))throw std::runtime_error("Nonfinite visible zone point");return v;};for(auto at=payload;at<payload+length;at+=288){Zone z;z.number=static_cast<int>(word(b,at+8));const auto count=word(b,at+24),points=word(b,at+156);if(count>64||points>16||points<3)throw std::runtime_error("Invalid visible zone counts");for(unsigned k=0;k<count;++k)z.sections.push_back(static_cast<std::int16_t>(std::uint16_t(b[at+28+k*2])|std::uint16_t(b[at+29+k*2])<<8));for(unsigned k=0;k<points;++k)z.perimeter.push_back({{scalar(at+160+k*8),scalar(at+164+k*8)}});result.zones.push_back(std::move(z));}});
    std::set<std::uint32_t> present,wanted;
    for(const auto& solid:result.geometry.solids)present.insert(solid.hash);
    for(const auto& instance:result.instances)for(auto hash:instance.lod_hashes)if(hash&&!present.count(hash))wanted.insert(hash);
    if(!wanted.empty()){
        auto shared=assets::decode_vehicle_geometry(source.read("GLOBAL/INGAMEB.BUN"));
        for(auto& solid:shared.solids)if(wanted.count(solid.hash)){present.insert(solid.hash);result.geometry.solids.push_back(std::move(solid));}
    }
    std::set<std::uint32_t> requested,used;
    if(result.environment.sun.available)for(auto hash:result.environment.sun_textures)requested.insert(hash);
    // afxUpdateTextures resolves original TextureNameHash with GetTextureInfo(hash,0,0).
    for(const auto& particle:result.particles.definitions)if(particle.texture_hash)requested.insert(particle.texture_hash);
    for(const auto& instance:result.instances)for(auto hash:instance.lod_hashes)if(hash)used.insert(hash);
    for(const auto& solid:result.geometry.solids)if(used.count(solid.hash))for(const auto& surface:solid.surfaces)requested.insert(surface.texture_hash);
    for(const auto& solid:result.geometry.solids)if(used.count(solid.hash))for(const auto& surface:solid.surfaces)if(surface.render_flags&0x40)requested.insert(0x0184d1f8); // Original pTextureInfoSpecularFalloff, eFixUpTables.
    const auto global=source.read("GLOBAL/GLOBALB.BUN");
    for(const auto& solid:result.geometry.solids)if(used.count(solid.hash))for(const auto& surface:solid.surfaces)
        if((surface.render_flags&0x1c0)&&!result.materials.count(surface.material_hash))result.materials.emplace(surface.material_hash,port::read_car_material(global,surface.material_hash));
    const std::vector<std::string> texture_paths{entry.resources[2],entry.resources[3],"GLOBAL/GLOBALB.BUN","GLOBAL/INGAMEB.BUN","GLOBAL/DYNTEX.BIN"};
    const auto decode=[&](const hp2::assets::Hp2TpkArchive& pack,const hp2::assets::TpkTextureHeader& header,const std::string& path){
            auto bitmap=decode_texture_level(pack,header);
            if(!bitmap)throw std::runtime_error("Cannot decode track texture: "+header.name);
            if(header.Field_0x6e){
                if(header.Field_0x6e>6||header.BaseImageSize<=0)throw std::runtime_error("Invalid original mip chain header");
                std::size_t offset=0;
                for(std::size_t level=0;level<=header.Field_0x6e;++level){
                    const auto size=std::size_t(header.BaseImageSize)>>(level*2);
                    if(!size||offset>std::size_t(header.image_size)||size>std::size_t(header.image_size)-offset)throw std::runtime_error("Original mip chain exceeds texture image data: "+header.name+" level="+std::to_string(level)+" base="+std::to_string(header.BaseImageSize)+" image="+std::to_string(header.image_size)+" offset="+std::to_string(offset)+" level_size="+std::to_string(size));
                    if(level){
                        auto mip=header;mip.width=header.width>>level;mip.height=header.height>>level;
                        if(!mip.width||!mip.height||level>header.shift_width||level>header.shift_height)throw std::runtime_error("Invalid mip level dimensions");
                        mip.shift_width=header.shift_width-std::uint8_t(level);mip.shift_height=header.shift_height-std::uint8_t(level);
                        mip.image_placement+=static_cast<std::int32_t>(offset);mip.image_size=static_cast<std::int32_t>(size);mip.is_swizzled=header.mip_is_swizzled[level];
                        auto pixels=decode_texture_level(pack,mip);
                        if(!pixels)throw std::runtime_error("Cannot decode original mip level");
                        result.mipmaps[header.tex_hash].push_back(std::move(*pixels));
                    }
                    offset+=size;
                }
            }
            result.textures.push_back({header,std::move(*bitmap),path});
    };
    std::set<std::uint32_t> pack_hashes;
    for(const auto& path:texture_paths){
        auto archive=hp2::assets::Hp2TpkArchive::load(source.read(path));
        if(!archive)throw std::runtime_error("Invalid track texture archive: "+path);
        for(const auto& header:archive->headers()){
            if(!pack_hashes.insert(header.tex_hash).second)throw std::runtime_error("Ambiguous track texture hash across original packs");
            if(!requested.erase(header.tex_hash))continue;
            decode(*archive,header,path);
        }
    }
    // E0056T01: original eSolid texture-table fix-up. The decoded surface carrier
    // stores the bound TextureInfo hash; requested NameHash remains in texture_fixups
    // and in the unchanged original bundle. No synthetic pixels or name heuristics.
    const std::set<std::uint32_t> render_targets{
        0x024ac102,0x024ac10e,0x024ac0fe,0x024ac108,0x024ac111,0x024ac100,0xbdd3acce,
        0x7f3e2fba,0x7f3e2fbb,0x7f3e2fbc,0x1bb8d749,0x1bb8d74a,0x1bb8d74b,0x1bb8d74c,
        0x39284ec8,0x5ff88783,0x5ff88784,0x305c1137,0x1ef0332d,0xd469017d,0xd469017e,0xd0147182};
    bool needs_default=false;
    for(auto& solid:result.geometry.solids)if(used.count(solid.hash)){
        std::vector<std::uint32_t> names;std::set<std::uint32_t> seen;
        for(const auto& surface:solid.surfaces)if(seen.insert(surface.texture_hash).second)names.push_back(surface.texture_hash);
        const auto fixed=hp2::recovered::eSolid::FixTextureTable(names,pack_hashes,render_targets);
        std::map<std::uint32_t,std::uint32_t> bindings;
        for(std::size_t i=0;i<names.size();++i){
            bindings.emplace(names[i],fixed[i]);
            if(!pack_hashes.count(fixed[i]))hp2::not_implemented("0x001057D8","Scenery model references an original render-target texture; host binding not restored");
            if(names[i]!=fixed[i]){result.texture_fixups[names[i]]=fixed[i];requested.erase(names[i]);needs_default|=fixed[i]==0x19006835;}
        }
        for(auto& surface:solid.surfaces)surface.texture_hash=bindings.at(surface.texture_hash);
    }
    if(needs_default&&std::none_of(result.textures.begin(),result.textures.end(),[](const auto& texture){return texture.header.tex_hash==0x19006835;})){
        const auto archive=hp2::assets::Hp2TpkArchive::load(global);
        if(!archive)throw std::runtime_error("Invalid original GLOBALB default texture archive");
        const auto& headers=archive->headers();const auto header=std::find_if(headers.begin(),headers.end(),[](const auto& h){return h.tex_hash==0x19006835;});
        if(header==headers.end())throw std::runtime_error("Original DEFAULTTEXTURE resource missing from GLOBALB");
        decode(*archive,*header,"GLOBAL/GLOBALB.BUN");
    }
    if(!a.empty())result.gaps.push_back("Nonempty TRACKA resource is not decoded");
    if(!requested.empty()){
        std::ostringstream message;message<<"Texture hashes absent from selected track/shared archives:";
        for(auto hash:requested)message<<" 0x"<<std::hex<<hash;
        result.gaps.push_back(message.str());
    }
    std::set<std::uint32_t> missing_models;
    for(const auto& instance:result.instances)for(auto hash:instance.lod_hashes)if(hash&&!present.count(hash))missing_models.insert(hash);
    if(!missing_models.empty()){
        std::ostringstream message;message<<"Scenery model hashes absent from track/shared geometry:";
        for(auto hash:missing_models)message<<" 0x"<<std::hex<<hash;
        result.gaps.push_back(message.str());
    }

    append_particle_library(result,source);
    return result;
}

void append_particle_library(Scene& scene,assets::Source& source){
    const auto bytes=source.read("GLOBAL/GLOBALB.BUN");
    auto library=decode_particles(bytes);
    // Global library precedes track load in the original loader. Library State3
    // emitters remain non-auto; terrain lists receive independent runtime copies.
    library.definitions.insert(library.definitions.end(),scene.particles.definitions.begin(),scene.particles.definitions.end());
    library.emitters.insert(library.emitters.end(),scene.particles.emitters.begin(),scene.particles.emitters.end());
    scene.particles=std::move(library);
    std::set<std::uint32_t> wanted;
    for(const auto& d:scene.particles.definitions)if(d.texture_hash)wanted.insert(d.texture_hash);
    for(const auto& texture:scene.textures)wanted.erase(texture.header.tex_hash);
    for(const std::string resource:{"GLOBAL/GLOBALB.BUN","GLOBAL/INGAMEB.BUN"}){
    const auto archive=hp2::assets::Hp2TpkArchive::load(resource=="GLOBAL/GLOBALB.BUN"?bytes:source.read(resource));
    if(!archive)throw std::runtime_error("Invalid original shared particle texture archive");
    for(const auto& header:archive->headers())if(wanted.erase(header.tex_hash)){
        auto bitmap=decode_texture_level(*archive,header);
        if(!bitmap)throw std::runtime_error("Original particle texture decode failed: "+header.name);
        if(header.Field_0x6e>6)throw std::runtime_error("Original particle mip count exceeds supported levels");
        std::size_t offset=0;
        for(unsigned level=0;level<=header.Field_0x6e;++level){
            const auto size=std::size_t(header.BaseImageSize)>>(2*level);
            if(!size||offset>std::size_t(header.image_size)||size>std::size_t(header.image_size)-offset)throw std::runtime_error("Original particle mip payload exceeds texture");
            if(level){auto mip=header;mip.width>>=level;mip.height>>=level;
                if(!mip.width||!mip.height||level>header.shift_width||level>header.shift_height)throw std::runtime_error("Invalid original particle mip dimensions");
                mip.shift_width-=level;mip.shift_height-=level;mip.image_placement+=static_cast<std::int32_t>(offset);mip.image_size=static_cast<std::int32_t>(size);mip.is_swizzled=header.mip_is_swizzled[level];
                auto decoded=decode_texture_level(*archive,mip);if(!decoded)throw std::runtime_error("Original particle mip decode failed");scene.mipmaps[header.tex_hash].push_back(std::move(*decoded));}
            offset+=size;
        }
        scene.textures.push_back({header,std::move(*bitmap),resource});
    }
    }
}
}
