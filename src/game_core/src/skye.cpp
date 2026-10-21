#include "skye.hpp"
namespace hp2::recovered {
// src/skye.cpp. Retail0011C450 / Debug0011C7C8, bounded model/matrix binding.
port::track::SkyModel SkyInitModel(const std::vector<SkyScenerySection>& sections,std::uint32_t hash){
 port::track::SkyModel model;model.hash=hash;model.local_world[0]=model.local_world[5]=model.local_world[10]=model.local_world[15]=1;
 for(const auto& section:sections)for(const auto& c:section.values)if(c.hash==hash){model.available=true;model.instance=c.instance;model.local_world=c.matrix;return model;}return model;
}
// Retail0011C5F8 / Debug0011C970. Original missing-environment-sky fallback.
void SkyLoader(port::track::EnvironmentResources& out,const std::vector<SkyScenerySection>& sections){
 out.sky=SkyInitModel(sections,0xbe43edbb);out.environment_sky=SkyInitModel(sections,0x72923c81);
 if(!out.environment_sky.available&&out.sky.available)out.environment_sky=out.sky;
 // SkyInitModel UnInit on the matching SceneryInfo's three models removes all
 // instances of that info from ordinary scenery, independently of LOD.
 for(const auto& section:sections)for(const auto& c:section.values)if(c.hash==0xbe43edbb||c.hash==0x72923c81)out.sky_instances.push_back(c.instance);
}
// Retail0011C6F8 / Debug0011CA70. Model selection only; GPU submission is host owned.
const port::track::SkyModel* StuffSky(const port::track::EnvironmentResources& r,unsigned id,bool draw){
 if(!draw)return nullptr;const auto& model=id>=1&&id<=3?r.sky:r.environment_sky;return model.available?&model:nullptr;
}
}
