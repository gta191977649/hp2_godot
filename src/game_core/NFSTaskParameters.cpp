#include "game_core/port/race/reward_observation.hpp"
// TASK-0110 E-0110-F03. Original source path unresolved: port-side placement,
// not an assertion that these globals were defined beside the original class.
// Debug NFSHP2.ELF SHA-256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
// Function records and instruction-query manifest: reverse/task0110/rewards.md.
#include "game_core/NFSTaskParameters.hpp"
#include "game_core/indep/inc/Race.hpp"
#include "game_core/port/race/damage_resources.hpp"
#include "game_core/port/not_implemented.hpp"
namespace hp2::debug {
// Debug002441E0; E-0110-F03. Keep the original unused visible-damage read, then test the
// player's clean-race flag; MinDamage is not a numeric damage threshold here.
bool NFSTaskParameters::CheckForCompletion(::port::driving::Memory& m,std::uint32_t a,std::uint32_t racing){
 const auto car=m.word(racing),damage=m.word(car+0x844);
 (void)BodyDamage(m,damage+8).GetVisibleTotalDamage();
 const auto laps_led=m.signed_half(racing+0x10);
 const auto players=m.signed_word(0x311fa8),opponents=m.signed_word(0x311fac),laps=m.signed_word(0x311fa4),skill=m.signed_word(0x31226c),type=m.signed_word(0x2f3890+0x1072c);
 const int cops=m.signed_word(0x3121d4)>0;
 const auto player=RacingCar(m,racing).GetPlayer();
 if(!player)hp2::not_implemented("0x002441E0","original task predicate requires a real player RacingCar");
 const auto clean=m.word(player+0x9dc); // Source delay slot reads this even for MinDamage=-1.
 if(m.scalar(a+4)!=-1.f&&!clean)return false;
 if(m.signed_word(a+8)!=-1&&players!=m.signed_word(a+8))return false;
 const int required_type=m.signed_word(a+0xc);
 if(required_type!=-1){
  if(type!=required_type)return false;
  if(type==4&&(static_cast<std::int32_t>(m.word(0x311fa8)+m.word(0x311fac))>=3||m.signed_byte(racing+0xc)!=1))return false;
  if(type==5&&m.signed_byte(racing+0xc)!=1)return false;
 }
 if(m.signed_word(a+0x10)!=-1&&opponents!=m.signed_word(a+0x10))return false;
 if(m.signed_word(a+0x14)!=-1&&laps!=m.signed_word(a+0x14))return false;
 if(m.signed_word(a+0x18)!=-1&&skill!=m.signed_word(a+0x18))return false;
 if(m.signed_word(a+0x1c)!=-1&&laps_led!=m.signed_word(a+0x1c))return false;
 return m.signed_word(a+0x20)==-1||cops==m.signed_word(a+0x20);
}
// Debug002445E8; E-0110-F03. Signed points comparison, with equality accepted.
bool NFSPointsRewards::CheckForCompletion(const ::port::driving::Memory& m,std::uint32_t a,int points){return m.signed_word(a)<=points;}
} // namespace hp2::debug
// Debug002443C8; E-0110-F03. Original table order, including already-earned task checks.
int hp2::debug::GetTaskRewardList(::port::driving::Memory& m,std::uint32_t out,std::uint32_t count,std::uint32_t racing){
 m.word(count,0);
 for(std::uint32_t a=0x314510;a<0x314740;a+=0x38){
  if(NFSTaskParameters::CheckForCompletion(m,a,racing)&&!m.word(0x303f44+4*m.word(a+0x34))){
   const auto n=m.word(count);m.word(out+4*n,a);m.word(count,n+1);
  }
 }
 return m.signed_word(count);
}
// Debug002445F8; E-0110-F03. Completion flags and source indices come from raw Debug data.
int hp2::debug::GetPointsRewardList(::port::driving::Memory& m,std::uint32_t out,std::uint32_t count,int points){
 m.word(count,0);
 for(std::uint32_t a=0x314740;a<0x314824;a+=12){
  if(NFSPointsRewards::CheckForCompletion(m,a,points)&&!m.word(0x303f6c+4*m.word(a+8))){
   const auto n=m.word(count);m.word(out+4*n,a);m.word(count,n+1);
  }
 }
 return m.signed_word(count);
}
// Debug00244D00; E-0110-F03. Mark every newly earned reward, retaining the last original
// table entry in each transient notification field. This does not perform I/O.
void hp2::debug::CheckForTaskAndPointRewards(::port::driving::Memory& m,std::uint32_t race){
 m.word(0x2f3c1c,0xffffffff);m.word(0x2f3c20,0xffffffff);
 const auto player=m.word(0x327770);
 if(m.signed_word(0x327780)>=2||!player||m.signed_word(0x2f3890+0x1072c)==2)return;
 const auto racing=Race(m,race).GetRacingCar(m.word(player+4));
 ::port::driving::Memory::Temporary tasks(m,40),points(m,76),counts(m,8);
 GetTaskRewardList(m,tasks.address(),counts.address(),racing);
 for(int i=0;i<m.signed_word(counts.address());++i){const auto a=m.word(tasks.address()+4*i);const auto completion_index=m.word(a+0x34);m.word(0x303f44+4*completion_index,1);::port::race::observe_task_reward_store(completion_index);m.word(0x2f3c20,m.word(a));}
 GetPointsRewardList(m,points.address(),counts.address()+4,m.signed_word(0x304694));
 for(int i=0;i<m.signed_word(counts.address()+4);++i){const auto a=m.word(points.address()+4*i);const auto completion_index=m.word(a+8);m.word(0x303f6c+4*completion_index,1);::port::race::observe_point_reward_store(completion_index);m.word(0x2f3c1c,m.word(a+8));}
}
