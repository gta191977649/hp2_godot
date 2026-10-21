#include "weatherman.hpp"
#include <limits>
#include <cmath>
#include <stdexcept>
namespace hp2::recovered {
port::track::WeatherResult WeatherManQuery(const std::vector<port::track::WeatherRegion>& regions,
    const port::track::EnvironmentVector& position) {
    using namespace port::track;
    for(float v:position)if(!std::isfinite(v))throw std::invalid_argument("Nonfinite weather query position");
    WeatherResult result;result.fog_regions.fill(std::numeric_limits<std::size_t>::max());
    for(int type=0;type<4;++type){
        std::size_t root=std::numeric_limits<std::size_t>::max();
        float nearest=std::numeric_limits<float>::max();
        std::array<std::size_t,3> hits{};std::array<float,3> scales{};unsigned count=0;
        for(std::size_t i=0;i<regions.size();++i){const auto& r=regions[i];if(r.type!=type)continue;
            const float x=position[0]-r.position[0],y=position[1]-r.position[1],z=position[2]-r.position[2];
            const float distance=x*x+y*y+z*z,radius=r.radius*r.radius,falloff=r.falloff_start*r.falloff_start;
            if(distance<radius){if(radius==falloff){if(distance<nearest){nearest=distance;root=i;}}
                else {const float influence=1.f-(distance-falloff)/(radius-falloff);if(influence>0){hits[count]=i;scales[count++]=influence;}}}
            if(count==3)break;
        }
        const float sum=scales[0]+scales[1]+scales[2];float root_scale=1.f-sum,hit_scale=1.f;
        if(sum>1.f){root_scale=0;hit_scale=1.f/sum;}
        float value=root<regions.size()?regions[root].intensity*root_scale:0.f;
        if(type!=1){for(unsigned i=0;i<count;++i)value+=regions[hits[i]].intensity*hit_scale;}
        else {
            value=0;float red=0,green=0,blue=0,start=0;
            auto add=[&](std::size_t index,float weight,unsigned slot){const auto& r=regions[index];
                result.fog_regions[slot]=index;result.fog_scales[slot]=weight;
                value+=r.intensity*weight;red+=float(r.colour&255)*weight;
                green+=float((r.colour>>8)&255)*weight;blue+=float((r.colour>>16)&255)*weight;start+=r.fog_start*weight;};
            if(root<regions.size())add(root,root_scale,0);
            for(unsigned i=0;i<count;++i)add(hits[i],scales[i]*hit_scale,i+1);
            result.fog_colour=(std::uint32_t(int(red))&255)|((std::uint32_t(int(green))&255)<<8)|((std::uint32_t(int(blue))&255)<<16);
            result.fog_start=start;
        }
        switch(type){case 0:result.rain=value;break;case 1:result.fog=value;break;case 2:result.snow=value;break;case 3:result.wind=value;break;}
    }
    return result;
}
}
