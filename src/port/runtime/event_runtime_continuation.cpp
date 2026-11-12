#include "event_runtime_continuation.hpp"
#include "game_core/indep/inc/Fred.hpp"
#include "game_core/port/race/character_resources.hpp"
#include "game_core/src/bSound.hpp"
#include "game_core/indep/src/sound.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <initializer_list>
#include <utility>
namespace port::race {
namespace {
constexpr std::uint32_t fred=0x2f3890,rp=0x311f78,event_object=0x312cd0;
constexpr std::array<std::uint32_t,40> parameter_offsets{{0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60,64,68,72,76,80,0x254,0x258,0x25c,0x260,0x264,0x268,0x26c,0x270,0x274,0x278,0x27c,0x280,0x284,0x288,0x2f4,0x2f8,0x2fc,0x300,0x30c}};
template<std::size_t N> void read_bytes(const driving::Memory& m,std::uint32_t p,std::array<std::uint8_t,N>& a){for(std::size_t i=0;i<N;++i)a[i]=m.byte(p+unsigned(i));}
template<std::size_t N> void write_bytes(driving::Memory& m,std::uint32_t p,const std::array<std::uint8_t,N>& a){std::memcpy(m.view(p,N).data,a.data(),N);}
void check_identity(const EventLaunchRequest& r,const EventRuntimeContinuation& c){if(!r.generation||r.generation!=c.storage_generation||r.tree_type!=c.tree_type||r.event_index!=c.event_index||r.event_number!=c.event_number||c.drivers.size()>14||c.track<0||c.car_id<0||c.car_id>=64||c.skin_id<0||c.skin_id>255)throw std::invalid_argument("Stale, malformed or mismatched source runtime continuation");}
}
EventRuntimeContinuation capture_event_runtime_continuation(driving::Memory& m,const EventLaunchRequest& request,const FrontendEventResources& resources){
 if(m.word(0x3275b0)!=7||m.word(0x3275b4)!=5)throw std::logic_error("Runtime continuation requires source track-specific reload transition");
 EventRuntimeContinuation c;c.storage_generation=request.generation;c.tree_type=request.tree_type;c.event_index=request.event_index;c.event_number=request.event_number;
 std::uint32_t tree=0;for(auto t:resources.trees())if(m.signed_word(t+8)==request.tree_type)tree=t;
 if(!tree||request.event_index<0||unsigned(request.event_index)>=m.word(tree+0x44))throw std::invalid_argument("Continuation event tree/row absent");
 const auto info=m.word(tree+0x3c)+unsigned(request.event_index)*0xbc;
 if(m.signed_byte(info+0x28)!=request.event_number)throw std::invalid_argument("Continuation event identity differs from source tree");
 c.prepared_event=runtime::capture_prepared_event(m,tree,info);c.track=m.signed_word(rp);
 if(m.word(rp+12)>1)throw std::invalid_argument("Continuation source track direction outside supported original enum");c.reverse=m.word(rp+12)!=0;
 const auto ride=hp2::debug::FrontendPlayerInfo::GetSelectedRideInfo(m,fred+0x10738,m.signed_word(fred+0x1072c));c.car_id=m.signed_word(ride);c.skin_id=m.byte(ride+5);c.has_dash=m.byte(ride+6)!=0;c.vertex_damage=m.byte(ride+7)!=0;
 read_bytes(m,fred,c.frontend_preferences);for(unsigned t=0;t<2;++t)read_bytes(m,fred+0x184+t*256,c.debug_event_blocks[t]);read_bytes(m,fred+0x388,c.transient_stats);
 c.score_axes=runtime::read_debug_event_score_axes(m);c.scores=runtime::read_debug_event_scores(m,fred,c.score_axes);
 for(unsigned i=0;i<10;++i)c.task_rewards[i]=m.word(fred+0x106b4+4*i);for(unsigned i=0;i<19;++i)c.point_rewards[i]=m.word(fred+0x106dc+4*i);for(unsigned i=0;i<4;++i)c.frontend_attempt_words[i]=m.word(fred+0x10728+4*i);
 for(unsigned i=0;i<2;++i)read_bytes(m,fred+0x10738+i*0x6d0,c.frontend_players[i]);c.frontend_checksum=m.word(fred+0x114d8);
 for(unsigned i=0;i<parameter_offsets.size();++i)c.parameter_words[i]=m.word(rp+parameter_offsets[i]);read_bytes(m,rp+0x54,c.objective);for(unsigned i=0;i<2;++i)read_bytes(m,rp+0x28c+i*48,c.race_players[i]);read_bytes(m,rp+0x2ec,c.race_display);
 const auto n=m.word(rp+0x308),drivers=m.word(rp+0x304);if(n>14||(!drivers&&n))throw std::invalid_argument("Continuation source driver table invalid");c.drivers.resize(n);
 for(unsigned i=0;i<n;++i){const auto d=drivers+24*i;if(m.word(d+20)!=hp2::debug::GetCharacterInfo(m,m.signed_byte(d+2)))throw std::logic_error("Continuation source driver character ownership differs");read_bytes(m,d,c.drivers[i]);}
 read_bytes(m,event_object+8,c.event_old_state);read_bytes(m,event_object+0x18,c.event_previous_state);c.event_initial_players=m.half(event_object+0x76);c.event_bonus_prediction=m.word(event_object+0x78);for(unsigned i=0;i<8;++i)for(unsigned j=0;j<3;++j)c.event_results[i][j]=m.word(event_object+0x150+i*12+j*4);
 for(unsigned i=0;i<16;++i)c.clock_words[i]=m.word(0x327cd8+4*i);c.random_state=m.word(0x329060);c.audio_mode=m.word(0x327f9c);const auto sound=m.word(0x329320);if(!sound)throw std::logic_error("Continuation source sound owner absent");for(unsigned i=0;i<16;++i)c.category_volume_bits[i]=m.word(sound+12+4*i);
 check_identity(request,c);return c;
}
void restore_event_runtime_continuation(driving::Memory& m,const EventLaunchRequest& request,const EventRuntimeContinuation& c){
 check_identity(request,c);
 // Validate all destination owners and spans before first mutation. Guest
 // pointers are supplied exclusively by the newly loaded destination resources.
 const auto tree=m.word(event_object),info=m.word(tree+0x5c),drivers=m.word(rp+0x304);
 if(!tree||!info||!drivers||m.signed_word(tree+8)!=c.tree_type||m.signed_byte(info+0x28)!=c.event_number||m.word(event_object+4)!=m.word(info+4))throw std::logic_error("Continuation destination event owners were not rebound");
 std::vector<std::uint32_t> characters;for(const auto& d:c.drivers)characters.push_back(hp2::debug::GetCharacterInfo(m,static_cast<std::int8_t>(d[2])));
 const auto axes=runtime::read_debug_event_score_axes(m);const auto scores=runtime::transcode_event_scores(c.score_axes,c.scores,axes);
 for(auto span:std::initializer_list<std::pair<std::uint32_t,unsigned>>{{fred,0x184},{fred+0x184,512},{fred+0x388,812},{fred+0x106b4,40},{fred+0x106dc,76},{fred+0x10728,16},{fred+0x10738,2*0x6d0},{fred+0x114d8,4},{rp,0x310},{drivers,14*24},{event_object+8,32},{event_object+0x76,6},{event_object+0x150,96},{0x327cd8,64},{0x329060,4}})m.view(span.first,span.second);
 // Score writer itself preflights all its proven spans before touching them.
 runtime::write_debug_event_scores(m,fred,axes,scores);
 write_bytes(m,fred,c.frontend_preferences);for(unsigned t=0;t<2;++t)write_bytes(m,fred+0x184+t*256,c.debug_event_blocks[t]);write_bytes(m,fred+0x388,c.transient_stats);
 for(unsigned i=0;i<10;++i)m.word(fred+0x106b4+4*i,c.task_rewards[i]);for(unsigned i=0;i<19;++i)m.word(fred+0x106dc+4*i,c.point_rewards[i]);for(unsigned i=0;i<4;++i)m.word(fred+0x10728+4*i,c.frontend_attempt_words[i]);for(unsigned i=0;i<2;++i)write_bytes(m,fred+0x10738+i*0x6d0,c.frontend_players[i]);m.word(fred+0x114d8,c.frontend_checksum);
 for(unsigned i=0;i<parameter_offsets.size();++i)m.word(rp+parameter_offsets[i],c.parameter_words[i]);write_bytes(m,rp+0x54,c.objective);for(unsigned i=0;i<2;++i)write_bytes(m,rp+0x28c+i*48,c.race_players[i]);write_bytes(m,rp+0x2ec,c.race_display);m.word(rp+0x308,unsigned(c.drivers.size()));for(unsigned i=0;i<c.drivers.size();++i){write_bytes(m,drivers+24*i,c.drivers[i]);m.word(drivers+24*i+20,characters[i]);}
 write_bytes(m,event_object+8,c.event_old_state);write_bytes(m,event_object+0x18,c.event_previous_state);m.half(event_object+0x76,c.event_initial_players);m.word(event_object+0x78,c.event_bonus_prediction);for(unsigned i=0;i<8;++i)for(unsigned j=0;j<3;++j)m.word(event_object+0x150+i*12+j*4,c.event_results[i][j]);for(unsigned i=0;i<16;++i)m.word(0x327cd8+4*i,c.clock_words[i]);m.word(0x329060,c.random_state);
}
void restore_event_continuation_audio(driving::Memory& m,const EventRuntimeContinuation& c){const auto owner=m.word(0x329320);if(!owner)throw std::logic_error("Continuation destination sound owner absent");hp2::debug::bSound sound(m,owner);for(unsigned i=0;i<16;++i){float gain;std::memcpy(&gain,&c.category_volume_bits[i],4);sound.SetCategoryVolume(i,gain);}hp2::debug::sound::SetAudioMode(m,int(c.audio_mode));}
}
