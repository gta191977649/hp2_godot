#include "game_core/src/ecstasy/estripdatae.hpp"
#include "port/assets/vehicle_geometry.hpp"
#include "platform/ps2_asset_root.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
namespace {
void check(bool value){if(!value)throw std::runtime_error("Vehicle geometry assertion failed");}
void word(std::vector<std::uint8_t>& data,std::size_t offset,std::uint32_t value){for(int i=0;i<4;++i)data[offset+i]=std::uint8_t(value>>(i*8));}
void floating(std::vector<std::uint8_t>& data,std::size_t offset,float value){std::uint32_t bits;std::memcpy(&bits,&value,4);word(data,offset,bits);}
template<class F> void rejects(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected);}
}
int main(int argc,char** argv){
    using Packet=hp2::recovered::eStripData;
    // A one-vertex packet and a four-vertex packet exercise distinct packed tails.
    std::vector<std::uint8_t> single(96);single[4]=1;
    floating(single,32,1);floating(single,36,2);floating(single,40,3);floating(single,48,.25f);floating(single,52,.75f);
    check(Packet::GetXYZ(single,0,0)==std::array<float,3>{1,2,3});
    check(Packet::GetST0(single,0,0)==std::array<float,2>{.25f,.75f});
    std::vector<std::uint8_t> quad(160);quad[4]=4;
    for(std::size_t i=0;i<4;++i){floating(quad,32+i*4,float(i));floating(quad,48+i*4,float(i+10));floating(quad,64+i*4,float(i+20));}
    floating(quad,84,.1f);floating(quad,88,.2f);floating(quad,92,.4f);floating(quad,96,.3f);
    check(Packet::GetXYZ(quad,0,3)==std::array<float,3>{3,13,23});
    check(Packet::GetST0(quad,0,0)==std::array<float,2>{.1f,.2f});
    check(Packet::GetST0(quad,0,1)==std::array<float,2>{.3f,.4f});
    auto five=quad;five[4]=5;
    floating(five,84,4);floating(five,88,14);floating(five,92,24);
    floating(five,136,.125f);floating(five,140,.875f);
    check(Packet::GetXYZ(five,0,4)==std::array<float,3>{4,14,24});
    check(Packet::GetST0(five,0,4)==std::array<float,2>{.125f,.875f});
    single[60]=0x1f;single[61]=0x80;
    word(single,64,0x6e03805c);single[76]=127;
    single[68]=128;
    check(hp2::recovered::eStripDataE::GetElementRGBA(single,0,0)==0x801f);
    rejects([&]{Packet::GetXYZ(single,0,1);});
    rejects([&]{Packet::GetST0({},0,0);});
    rejects([&]{port::assets::decode_vehicle_geometry({});});
    std::vector<std::uint8_t> truncated(8);word(truncated,0,0x80034000);word(truncated,4,100);
    rejects([&]{port::assets::decode_vehicle_geometry(truncated);});
    // A complete synthetic chunk hierarchy tests pointer relocation and table bounds.
    std::vector<std::uint8_t> container(432);
    word(container,0,0x80034000);word(container,4,424);
    word(container,8,0x80034002);word(container,12,416);
    word(container,16,0x34003);word(container,20,200);
    word(container,32+8,0x12345678);container[32+16]='X';container[32+52]=1;
    word(container,224,0x34004);word(container,228,72);
    word(container,240+12,6);container[240+29]=1;container[240+31]=0xff;
    word(container,304,0x34006);word(container,308,8);word(container,312,0xabcdef01);
    word(container,320,0x34005);word(container,324,104);
    std::copy(single.begin(),single.end(),container.begin()+336);
    word(container,336,0x6e018000);word(container,344,0x6c01c001);
    const auto decoded=port::assets::decode_vehicle_geometry(container);
    check(decoded.solids.size()==1 && decoded.solids[0].hash==0x12345678);
    check(decoded.solids[0].surfaces[0].texture_hash==0xabcdef01);
    check(decoded.solids[0].surfaces[0].vertices[0].position==std::array<float,3>{1,2,3});
    check(decoded.solids[0].surfaces[0].vertices[0].raw_normal[0]==-128);
    check(decoded.solids[0].surfaces[0].vertices[0].normal[0]<-1.f);
    check(decoded.solids[0].surfaces[0].vertices[0].normal[2]==1.f);
    auto invalid_uv=container;word(invalid_uv,384,0x7f800000);
    rejects([&]{port::assets::decode_vehicle_geometry(invalid_uv);});
    auto no_normal=container;word(no_normal,400,0x14000000);
    check(!port::assets::decode_vehicle_geometry(no_normal).solids[0].surfaces[0].vertices[0].has_normal);
    no_normal[254]=0x80;
    rejects([&]{port::assets::decode_vehicle_geometry(no_normal);});
    auto strip_container=container;strip_container.resize(496);
    word(strip_container,4,488);word(strip_container,12,480);word(strip_container,324,168);
    word(strip_container,252,10);strip_container[269]=4;strip_container[270]=2;
    word(quad,128,0x6e03805c);for(unsigned i=0;i<4;++i)quad[140+i]=127;
    std::copy(quad.begin(),quad.end(),strip_container.begin()+336);
    word(strip_container,336,0x6e018000);word(strip_container,344,0x6c01c001);
    word(strip_container,348,0x6000);
    const auto strip_decoded=port::assets::decode_vehicle_geometry(strip_container);
    check(strip_decoded.solids[0].surfaces[0].indices==std::vector<std::uint32_t>{0,1,2,2,1,3});
    auto invalid_triangles=strip_container;invalid_triangles[270]=1;
    rejects([&]{port::assets::decode_vehicle_geometry(invalid_triangles);});
    auto invalid_mode=strip_container;invalid_mode[341]=7;
    rejects([&]{port::assets::decode_vehicle_geometry(invalid_mode);});
    // Remaining bytes are globally valid; the short packet must not borrow them.
    auto invalid_packet_length=strip_container;word(invalid_packet_length,252,4);
    rejects([&]{port::assets::decode_vehicle_geometry(invalid_packet_length);});
    auto invalid_offset=container;word(invalid_offset,248,0xfffffff0);
    rejects([&]{port::assets::decode_vehicle_geometry(invalid_offset);});
    auto invalid_texture=container;invalid_texture[240]=1;
    rejects([&]{port::assets::decode_vehicle_geometry(invalid_texture);});
    auto invalid_count=container;invalid_count[32+52]=2;
    rejects([&]{port::assets::decode_vehicle_geometry(invalid_count);});
    auto invalid_packet=container;invalid_packet[336]=1;
    rejects([&]{port::assets::decode_vehicle_geometry(invalid_packet);});
    auto invalid_position=container;word(invalid_position,336+32,0x7fc00000);
    rejects([&]{port::assets::decode_vehicle_geometry(invalid_position);});
    std::vector<std::string> original_paths;
    for(int i=1;i<argc;++i)original_paths.push_back(argv[i]);
    if(original_paths.empty()) {
        const auto start=std::filesystem::current_path().string();
        const auto root=hp2::platform::resolve_zzdata_root(start);
        if(root)for(const auto* name:{"911TURBO","BMWM5COP","4DR_SEDAN"})
            original_paths.push_back((std::filesystem::path(*root)/"CARS"/name/"GEOMETRY.BIN").string());
        else {
            const auto* configured=std::getenv(hp2::platform::kAssetRootEnvVar);
            if((configured&&*configured)||hp2::platform::resolve_local_config_value(start,"root"))
                throw std::runtime_error("Configured original asset root is invalid");
            std::cout<<"Original vehicle geometry checks skipped: asset root is not configured\n";
        }
    }
    for(const auto& original_path:original_paths){
        std::ifstream file(original_path,std::ios::binary);
        if(!file)throw std::runtime_error("Cannot open configured vehicle geometry: "+original_path);
        std::vector<std::uint8_t> data{std::istreambuf_iterator<char>(file),{}};
        const auto geometry=port::assets::decode_vehicle_geometry(data);check(!geometry.solids.empty());
        std::size_t vertices=0,triangles=0;
        for(const auto& solid:geometry.solids)for(const auto& surface:solid.surfaces){vertices+=surface.vertices.size();triangles+=surface.indices.size()/3;}
        check(vertices>0);std::cout<<original_path<<" solids="<<geometry.solids.size()<<" vertices="<<vertices<<" triangles="<<triangles<<"\n";
        data.resize(data.size()/2);rejects([&]{port::assets::decode_vehicle_geometry(data);});
    }
    std::cout<<"VEHICLE_GEOMETRY_OK\n";
}
