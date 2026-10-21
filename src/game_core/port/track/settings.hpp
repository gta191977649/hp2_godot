#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
namespace port::track {
// Session-scoped host permission gates. Original activation remains a separate
// per-view decision; enabling a gate never creates an original-inactive effect.
enum class EffectId:unsigned { Roadshine,Environment,Sky,Sun,Shadows,Weather,Particles,Fog,DepthOfField,ScreenFilter,RadialBlur,Count };
struct EffectDescription {EffectId id;const char* key;const char* name;const char* category;};
inline constexpr std::array<EffectDescription,unsigned(EffectId::Count)> effect_descriptions{{
 {EffectId::Roadshine,"roadshine","Roadshine / surface specular","Surface"},
 {EffectId::Environment,"environment","Dynamic environment reflection","Surface"},
 {EffectId::Sky,"sky","Sky","Environment"},
 {EffectId::Sun,"sun","Sun and light flares","Environment"},
 {EffectId::Shadows,"shadows","Shadows","Environment"},
 {EffectId::Weather,"weather","Weather","Weather / particles"},
 {EffectId::Particles,"particles","Particles","Weather / particles"},
 {EffectId::Fog,"fog","Fog","Post processing"},
 {EffectId::DepthOfField,"dof","Depth of field","Post processing"},
 {EffectId::ScreenFilter,"screen_filter","Screen filter","Post processing"},
 {EffectId::RadialBlur,"radial_blur","Radial / previous-frame blend","Post processing"}
}};
struct RenderSettings {
 std::array<bool,unsigned(EffectId::Count)> allowed{};
 bool render_props=true; // Port viewer gate for transformed scenery (trees and props).
 void set_props(bool value){if(render_props!=value){render_props=value;++revision;}}
 bool distance_override=false;float distance=10000;std::uint64_t revision=1;
 RenderSettings(){allowed.fill(true);allowed[unsigned(EffectId::Environment)]=false;}
 bool enabled(EffectId id)const{return allowed.at(unsigned(id));}
 void set(const std::string& key,bool value){for(const auto& d:effect_descriptions)if(key==d.key){if(allowed[unsigned(d.id)]!=value){allowed[unsigned(d.id)]=value;++revision;}return;}throw std::invalid_argument("Unknown track effect: "+key);}
 void set_distance(bool enabled,float value){if(!std::isfinite(value)||value<=0)throw std::invalid_argument("Track distance override must be finite and positive");if(distance_override!=enabled||distance!=value){distance_override=enabled;distance=value;++revision;}}
 void restore(){render_props=true;allowed.fill(true);allowed[unsigned(EffectId::Environment)]=false;distance_override=false;distance=10000;++revision;}
};
inline RenderSettings& shared_render_settings(){static RenderSettings settings;return settings;}
}
