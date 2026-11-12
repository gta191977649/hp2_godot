#include "ai_drive_target.hpp"
#include "game_core/port/not_implemented.hpp"
#include <stdexcept>
namespace port::race {
void AiPlanningResources::load_retail_difficulty_info(const std::vector<std::uint8_t>& bytes){
 if(skill_chances_loaded_)throw std::logic_error("AI difficulty source already loaded");
 const auto word=[&](std::size_t p){if(p>bytes.size()||bytes.size()-p<4)throw std::runtime_error("Truncated AI difficulty chunk word");return std::uint32_t(bytes[p])|std::uint32_t(bytes[p+1])<<8|std::uint32_t(bytes[p+2])<<16|std::uint32_t(bytes[p+3])<<24;};
 const auto walk=[&](auto&& self,std::size_t begin,std::size_t end)->void{for(auto p=begin;p<end;){if(end-p<8)throw std::runtime_error("Truncated AI difficulty chunk");const auto id=word(p),size=word(p+4);if(size>end-p-8)throw std::runtime_error("AI difficulty chunk exceeds bundle");if(id==0x34b00){if(size!=0x3e0)throw std::runtime_error("Reviewed Retail DifficultyInfo requires3E0 bytes");const auto raw=memory_.allocate(size+8);for(unsigned i=0;i<size+8;++i)memory_.byte(raw+i,bytes[p+i]);raw_chunks_.push_back(raw);for(unsigned skill=0;skill<10;++skill)for(unsigned chance=0;chance<3;++chance)memory_.word(0x2e7af0+skill*28+chance*4,word(p+8+0x170+skill*28+chance*4));for(unsigned skill=0;skill<10;++skill){memory_.word(0x2e7b00+skill*28,word(p+8+0x180+skill*28));memory_.byte(0x2e7b08+skill*28,bytes[p+8+0x188+skill*28]);}memory_.word(0x2e7cdc,word(p+8+0x3d4));memory_.word(0x2e7ce0,word(p+8+0x3d8));// E-0107-F01: Debug1835C4 and Retail1851E4 both index this individual
// three-by-five BoostScale table at DifficultyInfo+288, skill stride20.
for(unsigned i=0;i<15;++i)memory_.word(0x2e7980+0x288+4*i,word(p+8+0x288+4*i));
// E-0099-W92: each field family independently matched to original
// Debug190AB0/190760 and Retail192C78/1928F0 consumers.
for(unsigned base:{0u,0x38u})for(unsigned entry=0;entry<7;++entry)for(unsigned lane=0;lane<2;++lane){const auto field=base+entry*8+lane*4;memory_.word(0x2e7980+field,word(p+8+field));}
for(unsigned base:{0x70u,0xb0u})for(unsigned entry=0;entry<16;++entry){const auto field=base+entry*4;memory_.word(0x2e7980+field,word(p+8+field));}
for(unsigned base:{0xf0u,0x130u})for(unsigned entry=0;entry<8;++entry)for(unsigned end=0;end<2;++end){const auto field=base+entry*8+end*4;memory_.word(0x2e7980+field,word(p+8+field));}
for(unsigned skill=0;skill<10;++skill)memory_.word(0x2e7afc+skill*28,word(p+8+0x17c+skill*28));
for(unsigned skill=0;skill<10;++skill)memory_.word(0x2e7b04+skill*28,word(p+8+0x184+skill*28));
memory_.word(0x2e7ce4,word(p+8+0x3dc));skill_chances_loaded_=true;}else if(id&0x80000000u)self(self,p+8,p+8+size);p+=8+size;}};walk(walk,0,bytes.size());if(!skill_chances_loaded_)hp2::not_implemented("0x0018FD20","authored34B00 AI sign-chance table is not loaded");
}
float AiPlanningResources::take_sign_chance(const ::port::driving::Memory& m,int skill,int chance)const{if(&m!=&memory_)throw std::logic_error("AI difficulty escaped shared Memory");if(!skill_chances_loaded_)hp2::not_implemented("0x0018FD20","reviewed authored AI sign chances are not loaded");if(skill<0||skill>=10||chance<0||chance>=3)throw std::out_of_range("AI sign-chance field outside original table");return m.scalar(0x2e7af0+skill*28+chance*4);}
void AiPlanningResources::close(){for(auto p:raw_chunks_)memory_.release(p);raw_chunks_.clear();skill_chances_loaded_=false;}
}

namespace port::race {
// W47: independent Retail1932E8/Debug1910B0 consumers prove these individual
// fields; whole difficulty schemas remain different and opaque bytes survive.
// E-0110-SK01: source LB retains finished-player skill-2, and both Debug
// 191264/Retail193550 read payload180 +signedSkill*1C with no lower clamp.
// For-2 this is payload148, already independently mapped by W92's identical
// RatioUnderOne130..16F fields. Preserve that actual adjacent-field read;
// it is not a new skill row or a substituted constant. Other negatives remain
// unreviewed even if they happen to fall within some mapped resource bytes.
float AiPlanningResources::speed_factor(const ::port::driving::Memory& m,int skill)const{if(&m!=&memory_)throw std::logic_error("AI difficulty escaped shared Memory");if(!skill_chances_loaded_)hp2::not_implemented("0x001910B0","reviewed authored AI speed factor is not loaded");if((skill<0&&skill!=-2)||skill>=10)hp2::not_implemented("0x001910B0","special signed skill speed-factor asset correspondence is not reviewed");return m.scalar(0x2e7b00+skill*28);}
std::uint8_t AiPlanningResources::route_point_variance(const ::port::driving::Memory& m,int skill)const{if(&m!=&memory_)throw std::logic_error("AI difficulty escaped shared Memory");if(!skill_chances_loaded_)hp2::not_implemented("0x001910B0","reviewed authored AI route variance is not loaded");if(skill<0||skill>=10)throw std::out_of_range("AI route variance outside reviewed skill table");return m.byte(0x2e7b08+skill*28);}
float AiPlanningResources::route_point_change_frequency(const ::port::driving::Memory& m)const{if(&m!=&memory_)throw std::logic_error("AI difficulty escaped shared Memory");if(!skill_chances_loaded_)hp2::not_implemented("0x001910B0","reviewed authored route-point frequency is not loaded");return m.scalar(0x2e7cdc);}
}

namespace port::race {
// Debug190760 / Retail1928F0: distinct Misc schemas; only reviewed scalar maps.
float AiPlanningResources::boost_calculation_frequency(const ::port::driving::Memory& m)const{if(&m!=&memory_)throw std::logic_error("AI difficulty escaped shared Memory");if(!skill_chances_loaded_)hp2::not_implemented("0x00190760","reviewed authored boost frequency is not loaded");return m.scalar(0x2e7ce0);}
}

namespace port::race {
void AiPlanningResources::require_boost_fields(const ::port::driving::Memory& m)const{if(&m!=&memory_)throw std::logic_error("AI difficulty escaped shared Memory");if(!skill_chances_loaded_)hp2::not_implemented("0x00190760","reviewed authored AI boost field owner is not loaded");}
}

namespace port::race {
// Debug1308F8 and Retail1307B0 read the same individual skill chance17C/1C.
float AiPlanningResources::collision_avoidance_chance(const ::port::driving::Memory& m,int skill)const{if(&m!=&memory_)throw std::logic_error("AI difficulty escaped shared Memory");if(!skill_chances_loaded_)hp2::not_implemented("0x001308F8","authored collision avoidance chance is not loaded");if(skill<0||skill>=10)hp2::not_implemented("0x001308F8","special signed skill collision-chance correspondence is not reviewed");return m.scalar(0x2e7afc+28*skill);}
}
