#include "game_core/port/track/assets.hpp"
#include "game_core/port/track/texture_state.hpp"
#include "game_core/src/ecstasy/elighte.hpp"
#include <iostream>
#include <stdexcept>

namespace {
void check(bool v){if(!v)throw std::runtime_error("Track asset assertion failed");}
template<class F>void rejects(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected);}
void word(std::vector<std::uint8_t>& b,std::size_t p,std::uint32_t v){for(int i=0;i<4;++i)b[p+i]=std::uint8_t(v>>(i*8));}
struct Source:port::assets::Source {
    std::vector<std::uint8_t> data;std::string name="fixture";
    std::vector<std::uint8_t> read(const std::string&)override{return data;}
    const std::string& root()const override{return name;}
};
}
int main(int argc,char** argv){
    using namespace port::track;
    std::vector<std::uint8_t> packed(21);word(packed,0,0x504d4f43);word(packed,4,0x1001);word(packed,8,7);word(packed,12,21);
    packed[16]=2;packed[18]='A';packed[19]=3;packed[20]=1;
    check(decompress_bundle(packed)==std::vector<std::uint8_t>(7,'A'));
    auto bad=packed;bad[20]=0;rejects([&]{decompress_bundle(bad);});
    bad=packed;word(bad,8,6);rejects([&]{decompress_bundle(bad);});
    bad=packed;bad.resize(15);rejects([&]{decompress_bundle(bad);});
    Source source;source.data.resize(8+164);word(source.data,0,0x34201);word(source.data,4,164);
    source.data[8]='X';source.data[8+0x41]=11;
    auto entries=load_catalog(source);check(entries.size()==1&&entries[0].number==1&&entries[0].resource_number==11);
    check(entries[0].resources[1]=="TRACKS/TRACKB11.LZC");
    source.data.pop_back();rejects([&]{load_catalog(source);});
    rejects([&]{load_scene(source,Entry{});});
    hp2::assets::TpkTextureHeader header;header.Field_0x6e=4;header.Field_0x6f=1;header.Field_0x70=3;header.alpha_bits=0x0a;
    auto state=texture_state_00108CC0(header);check(state.tex1_register==0xf0&&state.requires_lod_projection&&state.maximum_mip_level==4);
    header.alpha_bits=0x48;state=texture_state_00108CC0(header);check(state.additive&&state.alpha_register==0x48);
    header.Field_0x6e=7;rejects([&]{texture_state_00108CC0(header);});
    hp2::recovered::vAABB volume;volume.ExtentX=volume.ExtentY=volume.ExtentZ=10;
    check(volume.Contains(0,0,0)&&!volume.Contains(10,0,0)&&!volume.Contains(0,-10,0));
    LightPack light_pack;volume.NumChildren=-1;volume.ChildrenIndicies[0]=0;light_pack.tree.NodeArray.push_back(volume);
    hp2::recovered::eLight ambient;ambient.Colour=0x001e140a;ambient.Intensity=.5f;ambient.Size=100;
    light_pack.lights.push_back(ambient);std::vector<LightPack> packs{light_pack};
    hp2::recovered::eLightQuery query;check(hp2::recovered::QueryLightDatabase(query,packs,{{0,0,0}})==0);
    check(query.AmbientR==10&&query.AmbientG==20&&query.AmbientB==30&&query.LightScale==.5f);
    const auto lighting=hp2::recovered::elSetupLightContext(packs);check(lighting[0]==40&&lighting[12]==10&&lighting[15]==128);
    auto point=ambient;point.Type=2;point.PositionX=3;point.PositionY=4;point.Size=10;point.FarStart=5;
    packs[0].lights.push_back(point);packs[0].tree.NodeArray[0].NumChildren=-2;packs[0].tree.NodeArray[0].ChildrenIndicies[1]=1;
    check(hp2::recovered::QueryLightDatabase(query,packs,{{0,0,0}})==1&&query.Influences[0]==1);
    const auto point_lighting=hp2::recovered::elSetupLightContext(packs);check(point_lighting[4]==10&&point_lighting[5]==20&&point_lighting[6]==0);
    check(point_lighting[17]>.59f&&point_lighting[17]<.61f&&point_lighting[21]>.79f&&point_lighting[21]<.81f);
    packs[0].tree.NodeArray[0].ChildrenIndicies[0]=2;rejects([&]{hp2::recovered::QueryLightDatabase(query,packs,{{0,0,0}});});
    // Optional player-owned integration sweep; no proprietary fixture is checked in.
    if(argc==2){
        port::assets::DirectorySource original(argv[1]);const auto catalog=load_catalog(original);check(catalog.size()==30);
        for(const auto& entry:catalog){
            auto scene=load_scene(original,entry);check(!scene.geometry.solids.empty()&&!scene.instances.empty()&&!scene.textures.empty());
            check(scene.world_lights_available);
            for(const auto& texture:scene.textures){
                if(!texture.header.Field_0x6e)continue;
                const auto& mips=scene.mipmaps.at(texture.header.tex_hash);check(mips.size()==texture.header.Field_0x6e);
                for(std::size_t level=0;level<mips.size();++level){const auto& mip=mips[level];check(mip.width==(texture.bitmap.width>>(level+1))&&mip.height==(texture.bitmap.height>>(level+1)));check(mip.rgba.size()==std::size_t(mip.width)*mip.height*4);}
            }
            std::cout<<entry.number<<" solids="<<scene.geometry.solids.size()<<" instances="<<scene.instances.size()<<" textures="<<scene.textures.size()<<'\n';
        }
    }
    std::cout<<"Track asset tests passed\n";
}
