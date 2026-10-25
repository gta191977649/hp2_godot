// E-0048-A06: exact ECOFF source marker src\ecstasy\elighte.cpp.
#include "elighte.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include "game_core/port/not_implemented.hpp"

namespace port::selection {
// E-0098-R02: selected original initialization fields only, not whole00117708.
LightingDefaults load_lighting_defaults(port::driving::Memory& memory){
    LightingDefaults values;
    values.normal.Light0.Position={{memory.scalar(0x324aec),memory.scalar(0x324af8),memory.scalar(0x324af0)}};
    values.normal.Light1.Position={{-5,-1,0}};
    values.normal.Light2.Position={{memory.scalar(0x324af4),memory.scalar(0x324afc),memory.scalar(0x324af4)}};
    values.normal.Light0.Colour={{80,80,80}};values.normal.Light1.Colour={{64,64,64}};
    values.normal.Light2.Colour={{128,128,180}};values.normal.LightA.Colour={{32,32,32}};
    values.silouette.Light0.Position={{0,memory.scalar(0x324b08),-5}};
    values.silouette.Light1.Position={{-5,10,0}};
    values.silouette.Light2.Position={{memory.scalar(0x324b00),10,memory.scalar(0x324b04)}};
    return values;
}
}
namespace hp2::debug {
// Debug00115BD0, E-0098-R02. View2 is player2; all other view IDs use player1.
void SetSelectCarLighting(std::array<SelectCarLights,2>& players,int view_id,float intensity,int silouette,
    const port::selection::LightingDefaults& defaults){
    auto& result=players[view_id==2?1:0];
    if(silouette){
        result.Light0=defaults.silouette.Light0;result.Light1=defaults.silouette.Light1;
        result.Light2=defaults.silouette.Light2;result.LightA.Colour=defaults.silouette.LightA.Colour;
        return;
    }
    const float shift=intensity*30.f;
    const auto shifted=[&](SelectCarLight& out,const SelectCarLight& source){
        out.Position=source.Position;
        for(int i=0;i<3;++i)out.Colour[i]=std::min(source.Colour[i]+shift,255.f);
    };
    shifted(result.Light0,defaults.normal.Light0);shifted(result.Light1,defaults.normal.Light1);
    shifted(result.Light2,defaults.normal.Light2);
    for(int i=0;i<3;++i)result.LightA.Colour[i]=std::min(defaults.normal.LightA.Colour[i],255.f);
}
// Debug00115D88 selector frontend branch, E-0098-R02; E-0048-A06 original TU.
std::array<float,28> elSetupLightContext(const SelectCarLights& lights,
    const std::array<float,16>& view,const std::array<float,16>& part){
    std::array<float,28> packet{};
    const std::array<const SelectCarLight*,3> sources{{&lights.Light0,&lights.Light1,&lights.Light2}};
    const auto inverse_rotate=[](const std::array<float,3>& v,const std::array<float,16>& matrix){
        std::array<float,3> out{};
        for(int i=0;i<3;++i)out[i]=(v[0]*matrix[i*4]+v[1]*matrix[i*4+1])+v[2]*matrix[i*4+2];
        return out;
    };
    for(int light=0;light<3;++light){
        const auto& source=*sources[light];
        const auto world=inverse_rotate(source.Position,view),local=inverse_rotate(world,part);
        const float squared=(local[0]*local[0]+local[1]*local[1])+local[2]*local[2];
        if(!(squared>0) || !std::isfinite(squared))hp2::not_implemented("0x00115D88","selection light direction outside finite nonzero domain");
        const float reciprocal=1.f/std::sqrt(squared);
        for(int axis=0;axis<3;++axis){packet[light*4+axis]=source.Colour[axis];packet[16+axis*4+light]=local[axis]*reciprocal;}
    }
    for(int axis=0;axis<3;++axis)packet[12+axis]=lights.LightA.Colour[axis];
    packet[15]=128;return packet;
}
}
namespace hp2::recovered {
// Debug00115780 / Retail00115370; E-0048-A06.
int QueryLightDatabase(eLightQuery& query,const std::vector<port::track::LightPack>& packs,
    const std::array<float,3>& position) {
    query={};std::array<float,4> ambient{{0,0,0,1}};
    std::vector<std::array<float,4>> additive;float total=0,nearest=0;
    const std::uint32_t nearest_bits=0x7f7fffff;std::memcpy(&nearest,&nearest_bits,4);
    int light_count=0;
    for(const auto& pack:packs){
        if(light_count>=10||additive.size()>=10)break;
        const auto* leaf=pack.tree.QueryLeaf(position[0],position[1],position[2]);if(!leaf)continue;
        const auto count=-leaf->NumChildren;if(count>10)throw std::runtime_error("Light leaf exceeds ten entries");
        for(int n=0;n<count&&light_count<10;++n){
            const auto index=leaf->ChildrenIndicies[n];if(index<0||std::size_t(index)>=pack.lights.size())throw std::runtime_error("Light leaf index exceeds pack");
            const auto& light=pack.lights[index];
            const float x=std::abs(position[0]-light.PositionX),y=std::abs(position[1]-light.PositionY),z=std::abs(position[2]-light.PositionZ);
            float distance=x*x;distance+=y*y;distance+=z*z;const float radius_squared=light.Size*light.Size;
            if(!(distance<radius_squared))continue;
            const float far_squared=light.FarStart*light.FarStart;
            float influence=1;if(far_squared<distance)influence=1-(distance-far_squared)/(radius_squared-far_squared);
            if(light.Type==2){query.Lights[light_count]=&light;query.Influences[light_count++]=influence;continue;}
            if(light.Type!=0)continue;
            std::array<float,4> value{{float(light.Colour&255),float((light.Colour>>8)&255),float((light.Colour>>16)&255),light.Intensity}};
            if(light.AttenuationType==2){if(influence>0&&additive.size()<10){for(auto& c:value)c*=influence;additive.push_back(value);total+=influence;}}
            else if(distance<nearest){ambient=value;nearest=distance;}
        }
    }
    float base_weight=1-total,additive_weight=total;if(total>1){additive_weight=1/total;base_weight=0;}
    for(std::size_t channel=0;channel<4;++channel){ambient[channel]*=base_weight;for(const auto& value:additive)ambient[channel]+=value[channel]*additive_weight;}
    query.AmbientR=ambient[0];query.AmbientG=ambient[1];query.AmbientB=ambient[2];query.LightScale=ambient[3];
    return light_count;
}
// Retail00115978 / Debug00115D88; E-0048-A06. Bounded world identity path.
std::array<float,28> elSetupLightContext(const std::vector<port::track::LightPack>& packs) {
    eLightQuery query;const auto count=QueryLightDatabase(query,packs,{{0,0,0}});
    std::array<float,28> out{};
    const float sunlight=std::clamp(query.LightScale,0.f,1.f);
    for(std::size_t c=0;c<3;++c)out[c]=80*sunlight;
    out[12]=query.AmbientR;out[13]=query.AmbientG;out[14]=query.AmbientB;out[15]=128;
    // The initialized AmbientColourIG override is zero. Original no-ambient
    // fallback is RGB85, independent of the directional scale.
    if(out[12]==0&&out[13]==0&&out[14]==0)out[12]=out[13]=out[14]=85;
    const std::array<std::uint32_t,3> direction_bits{{0xbf3d70a4,0,0x3f0ccccd}};
    std::array<std::array<float,3>,3> directions{};
    for(std::size_t i=0;i<3;++i){std::memcpy(&directions[0][i],&direction_bits[i],4);directions[0][i]*=10000.f;}
    for(int light=0;light<std::min(count,2);++light){
        const auto* source=query.Lights[light];directions[light+1]={{source->PositionX,source->PositionY,source->PositionZ}};
        out[(light+1)*4]=query.Influences[light]*float(source->Colour&255);
        out[(light+1)*4+1]=query.Influences[light]*float((source->Colour>>8)&255);
        // Original00115D88/00115978 stores zero in the local point-light blue channel.
    }
    for(std::size_t light=0;light<3;++light){
        const auto& d=directions[light];float squared=d[0]*d[0];squared+=d[1]*d[1];squared+=d[2]*d[2];
        if(squared>0){const float inverse=1/std::sqrt(squared);for(std::size_t i=0;i<3;++i)out[16+i*4+light]=d[i]*inverse;}
    }
    return out;
}
}
// TASK-0051 E-0051-L01, Retail00115978 / Debug00115D88 in-game branch.
namespace hp2::recovered {
std::array<float,28> elSetupLightContext(const std::vector<port::track::LightPack>& packs,const std::array<float,16>& matrix){
    const std::array<float,3> position{{matrix[12],matrix[13],matrix[14]}};
    eLightQuery query;const auto count=QueryLightDatabase(query,packs,position);
    std::array<float,28> out{};const float sunlight=std::clamp(query.LightScale,0.f,1.f);
    for(unsigned k=0;k<3;++k)out[k]=80*sunlight;
    out[12]=query.AmbientR;out[13]=query.AmbientG;out[14]=query.AmbientB;out[15]=128;
    if(out[12]==0&&out[13]==0&&out[14]==0)out[12]=out[13]=out[14]=85;
    const std::array<std::uint32_t,3> bits{{0xbf3d70a4,0,0x3f0ccccd}};
    std::array<std::array<float,3>,3> directions{};
    for(unsigned k=0;k<3;++k){float v;std::memcpy(&v,&bits[k],4);directions[0][k]=v*10000.f-position[k];}
    for(int i=0;i<std::min(count,2);++i){const auto* source=query.Lights[i];directions[i+1]={{source->PositionX-position[0],source->PositionY-position[1],source->PositionZ-position[2]}};out[(i+1)*4]=query.Influences[i]*float(source->Colour&255);out[(i+1)*4+1]=query.Influences[i]*float((source->Colour>>8)&255);}
    for(unsigned i=0;i<3;++i){std::array<float,3> local{};for(unsigned col=0;col<3;++col){local[col]=matrix[col*4]*directions[i][0];local[col]+=matrix[col*4+1]*directions[i][1];local[col]+=matrix[col*4+2]*directions[i][2];}float squared=local[0]*local[0];squared+=local[1]*local[1];squared+=local[2]*local[2];if(squared>0){const float inverse=1/std::sqrt(squared);for(unsigned k=0;k<3;++k)out[16+k*4+i]=local[k]*inverse;}}
    return out;
}
}
