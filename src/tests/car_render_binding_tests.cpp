#include "game_core/CarRenderInfo.hpp"
#include "game_core/port/not_implemented.hpp"
#include <iostream>
#include <stdexcept>
#include <limits>
#include <cmath>
using hp2::recovered::CarRenderInfo;
void check(bool value) { if (!value) throw std::runtime_error("Car render slice assertion failed"); }
int main() {
    const auto normal = CarRenderInfo::Sub_0011E860("SAMPLE",0);
    check(normal.body[0].name=="SAMPLE_A" && normal.body[3].name=="SAMPLE_D");
    check(normal.body[3].object_offset==0x88 && normal.tire_rear[2].object_offset==0x118);
    check(normal.tire_rear[2].fallback=="SAMPLE_TIRE_FRONT_C");
    check(normal.body[0].texture_replacements[0].target_hash!=normal.body[2].texture_replacements[0].target_hash);
    check(normal.body[2].texture_replacements[0].target_hash==normal.body[2].texture_replacements[1].target_hash);
    check(normal.body[0].texture_replacements.size()==12 && normal.body[3].texture_replacements.size()==2);
    const auto type2 = CarRenderInfo::Sub_0011E860("SAMPLE",2);
    check(type2.body[0].texture_replacements[0].target_hash==type2.body[3].texture_replacements[0].target_hash);
    check(type2.body[1].name=="SAMPLE_A" && type2.body[3].name=="SAMPLE_C");
    check(type2.tire_front[2].name=="SAMPLE_TIRE_FRONT_B");
    const std::array<bool,4> all{{true,true,true,true}};
    for (auto pair : {std::array<int,2>{120,0},{119,1},{45,1},{44,2},{20,2},{19,3},{4,3}}) {
        const auto lod=CarRenderInfo::Sub_00120EF8(pair[0],all);
        check(lod.visible && lod.body==pair[1] && lod.tire==(pair[1]<2?pair[1]:2));
        check(lod.mirrors==(pair[1]<2) && lod.license_plates==(pair[1]<3));
    }
    check(!CarRenderInfo::Sub_00120EF8(3,all).visible);
    check(CarRenderInfo::Sub_00120EF8(10,{{true,false,true,true}}).body==0);
    check(CarRenderInfo::Sub_00120EF8(10,{{true,true,false,true}}).body==1);
    const std::array<float,3> zero{{0,0,0}},forward{{1,0,0}};
    check(port::Sub_00107600(3,100,{{9,0,0}},zero,forward)==50);
    check(port::Sub_00107600(3,100,{{0,0,0}},zero,forward)==200);
    check(port::Sub_00107600(3,100,{{-4,0,0}},zero,forward)==0);
    check(port::Sub_00107600(3,100,{{-3,0,0}},zero,forward)==200);
    const auto uv=port::Sub_0010C158(512,320,32768,32768);
    check(uv.original[0]==1 && uv.original[1]==0.625f && uv.cropped_image[0]==1 && uv.cropped_image[1]==1);
    const auto uv_stored=port::Sub_0010C158(512,320,32768,20480);
    check(uv_stored.original[1]==1 && uv_stored.cropped_image[1]==1.6f);
    std::array<float,19> material{};
    material[0]=1;material[1]=1;material[2]=1;material[3]=1;
    material[4]=2;material[5]=3;material[6]=4;material[7]=0.25f;material[11]=0.75f;
    material[16]=8;material[17]=12;material[18]=255;
    const auto coefficients=port::Sub_00116828(material);
    check(coefficients[0]==1 && coefficients[1]==0 && coefficients[2]==0.1f && coefficients[3]==1);
    check(coefficients[4]==2 && coefficients[7]==64 && coefficients[11]==-32);
    check(coefficients[12]==0 && coefficients[13]==0 && coefficients[14]==8 && coefficients[15]==4);
    const std::array<float,16> basis{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
    const auto blur_bindings=CarRenderInfo::Sub_0011E860("TEST",0,1);
    check(blur_bindings.wheel_blur[0].object_offset==0x130 && blur_bindings.wheel_blur[1].object_offset==0x148);
    check(blur_bindings.wheel_blur[0].name=="TEST_WHEEL_BLUR" && blur_bindings.wheel_blur[0].fallback=="TEST_WHEEL_BLUR_F");
    check(blur_bindings.wheel_blur[1].fallback=="TEST_WHEEL_BLUR_R");
    const auto lights=port::Sub_00115978(basis);
    check(lights[0]==128 && lights[4]==64 && lights[12]==0 && lights[15]==128);
    check(lights[16]==0 && lights[20]<-0.707f && lights[24]>0.707f);
    const auto identity=port::Sub_0011B7D8(0);
    const auto reverse=port::Sub_0011B7D8(0x8000);
    check(identity[0]>0.99999f && identity[1]==0 && identity[10]==1);
    check(reverse[0]<-0.99999f && reverse[10]==1 && reverse[15]==1);
    port::CarRenderConfig config;
    config.Field_0x120_0x180[0]={{1,2,3,4}};
    const auto matrices=CarRenderInfo::Sub_00120EF8(config,0);
    const auto rolling=port::Sub_001C0D78(config,{{1.57079637f,1.57079637f,1.57079637f,1.57079637f}},0);
    check(rolling.tire[1][2]<-.999f && rolling.tire[1][8]>.999f);
    check(rolling.tire[0][2]>.999f && rolling.tire[0][8]>.999f);
    check(rolling.blur[1][0]==1 && rolling.blur[1][2]==0);
    check(rolling.blur[0][0]<-.999f && rolling.blur[0][2]==0);
    check(port::Sub_00120EA8(0)==0 && std::fabs(port::Sub_00120EA8(25)-.5f)<1e-6f);
    check(port::Sub_00120EA8(-25)==port::Sub_00120EA8(25) && port::Sub_00120EA8(100)==1);
    check(matrices.tire[0][12]==1 && matrices.tire[0][13]==2 && matrices.tire[0][14]==3);
    check(matrices.tire[0][0]<-0.9999f && matrices.tire[1][0]>0.9999f);
    check(matrices.brake[0][5]<-0.9999f && matrices.brake[3][5]==-1);
    check(matrices.brake[1][5]>0.9999f && matrices.brake[2][5]==1);
    // E-0038-W01: raw native wheel matrices need the original render-only
    // basis once. A nonzero spin catches a wrong multiplication side.
    port::CarAttachmentMatrices raw_wheels;
    for(unsigned i=0;i<4;++i){
        raw_wheels.tire[i]={{0,0,-1,0,0,1,0,0,1,0,0,0,float(i+1),2,3,1}};
        raw_wheels.blur[i]=basis;raw_wheels.blur[i][12]=float(i+1);
        raw_wheels.brake[i]={{0,1,0,0,-1,0,0,0,0,0,1,0,float(i+1),2,3,1}};
    }
    const auto raw_before=raw_wheels;
    const auto submitted=CarRenderInfo::Sub_00120EF8(raw_wheels);
    for(unsigned i=0;i<4;++i){
        check(raw_wheels.tire[i]==raw_before.tire[i] && raw_wheels.brake[i]==raw_before.brake[i] && raw_wheels.blur[i]==raw_before.blur[i]);
        if(i==0||i==3){
            check(submitted.tire[i][2]>.9999f && submitted.tire[i][8]>.9999f && submitted.tire[i][5]<-.9999f);
            check(submitted.blur[i][0]<-.9999f && submitted.blur[i][5]<-.9999f);
            check(submitted.brake[i][1]==1 && submitted.brake[i][4]==1);
        }else check(submitted.tire[i]==raw_wheels.tire[i] && submitted.brake[i]==raw_wheels.brake[i] && submitted.blur[i]==raw_wheels.blur[i]);
        check(submitted.tire[i][12]==float(i+1) && submitted.tire[i][13]==2 && submitted.tire[i][14]==3);
        check(submitted.brake[i][12]==float(i+1) && submitted.brake[i][13]==2 && submitted.brake[i][14]==3);
    }
    std::vector<std::uint8_t> catalog(16+64*0x560,0);
    const auto write32=[&](std::size_t at,std::uint32_t value) {
        for(unsigned lane=0;lane<4;++lane)catalog[at+lane]=static_cast<std::uint8_t>(value>>(lane*8));
    };
    write32(0,0x34600);write32(4,static_cast<std::uint32_t>(catalog.size()-8));
    const std::string code="TEST",prefix="SHARED";
    for(std::size_t i=0;i<code.size();++i)catalog[16+i]=code[i];
    for(std::size_t i=0;i<prefix.size();++i)catalog[16+0x20+i]=prefix[i];
    write32(16+0x538,2);write32(16+0x120,0x3f800000);
    const auto skin_chunk=catalog.size();catalog.resize(catalog.size()+72,0);
    write32(skin_chunk,0x34601);write32(skin_chunk+4,64);write32(skin_chunk+12,1);
    write32(skin_chunk+8+0x2c,0x93c0eaf6);
    const auto parsed=port::read_car_render_config(catalog,"TEST");
    check(parsed.prefix=="SHARED" && parsed.material_hash==0x93c0eaf6 && parsed.Field_0x538==2 && parsed.Field_0x120_0x180[0][0]==1);
    auto broken=catalog;broken.resize(broken.size()-1);
    bool truncated=false;
    try {port::read_car_render_config(broken,"TEST");}catch(const std::exception&){truncated=true;}
    check(truncated);
    write32(16+0x120,0x7fc00000);
    bool nonfinite=false;
    try {port::read_car_render_config(catalog,"TEST");}catch(const std::exception&){nonfinite=true;}
    check(nonfinite);
    bool rejected=false;
    try {port::read_car_render_config({},"missing");} catch(const std::exception&) {rejected=true;}
    check(rejected);
    check(!hp2::guard([]{CarRenderInfo{}.Sub_00120EF8();}).empty());
    std::cout<<"CAR_RENDER_BINDINGS_OK\n";
}
