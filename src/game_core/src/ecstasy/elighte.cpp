// E-0048-A06: exact ECOFF source marker src\ecstasy\elighte.cpp.
#include "elighte.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
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
