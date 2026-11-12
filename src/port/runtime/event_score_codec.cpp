#include "event_score_codec.hpp"
#include "game_core/port/driving/memory.hpp"
#include "game_core/port/assets/elab_chunks.hpp"
#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>
namespace port::runtime {
namespace {
struct Layout {std::size_t size,total,lap,names;};
Layout layout(EventScoreVersion v){
 switch(v){
 case EventScoreVersion::Retail:return {0x114e0,0x6b8,0xe7b8,0x100b8};
 case EventScoreVersion::Debug:return {0x114dc,0x6b4,0xe7b4,0x100b4};
 }
 throw std::invalid_argument("Unknown event score profile version");
}
std::vector<EventScoreTrack> ordered(const EventScoreAxes& a){
 auto rows=a.tracks;std::sort(rows.begin(),rows.end(),[](const auto& x,const auto& y){return x.number<y.number;});return rows;
}
void require_same_axes(const EventScoreAxes& a,const EventScoreAxes& b){
 validate_event_score_axes(a);validate_event_score_axes(b);
 const auto x=ordered(a),y=ordered(b);
 if(a.lap_counts!=b.lap_counts||x.size()!=y.size())throw std::invalid_argument("Source/destination score axis identities differ");
 for(std::size_t i=0;i<x.size();++i)if(x[i].number!=y[i].number||x[i].same_as!=y[i].same_as||x[i].score_index!=y[i].score_index)throw std::invalid_argument("Source/destination score track aliases differ");
}
void require_extent(const std::vector<std::uint8_t>& p,const Layout& l){if(p.size()!=l.size)throw std::invalid_argument("Event score profile extent differs from explicit version");}
std::uint32_t u32(const std::uint8_t* p){return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);}
std::uint16_t u16(const std::uint8_t* p){return std::uint16_t(unsigned(p[0])|(unsigned(p[1])<<8));}
template<class S,class U>S signed_bits(U x){S out;static_assert(sizeof(out)==sizeof(x));std::memcpy(&out,&x,sizeof(x));return out;}
void put16(std::uint8_t* p,std::uint16_t x){p[0]=std::uint8_t(x);p[1]=std::uint8_t(x>>8);}
void put32(std::uint8_t* p,std::uint32_t x){for(unsigned i=0;i<4;++i)p[i]=std::uint8_t(x>>(8*i));}
EventScoreRecord get_record(const std::uint8_t* p){return {signed_bits<std::int32_t>(u32(p)),signed_bits<std::int16_t>(u16(p+4)),signed_bits<std::int16_t>(u16(p+6))};}
void put_record(std::uint8_t* p,const EventScoreRecord& r){put32(p,std::uint32_t(r.time));put16(p+4,std::uint16_t(r.name_index));put16(p+6,std::uint16_t(r.car_type));}
std::uint32_t address(std::uint32_t fred,std::size_t offset,std::size_t size){
 const auto a=std::uint64_t(fred)+offset;
 if(!fred||a+size>0x100000000ull)throw std::invalid_argument("Debug score destination wraps logical address space");
 return std::uint32_t(a);
}
void require_debug_axes(const driving::Memory& m,const EventScoreAxes& axes){require_same_axes(axes,read_debug_event_score_axes(m));}
}
void validate_event_score_axes(const EventScoreAxes& a){
 constexpr std::array<int,9> original{{1,2,3,4,5,6,7,8,10}};
 if(a.lap_counts!=original||a.tracks.empty())throw std::invalid_argument("Missing/unreviewed original score axes");
 const auto rows=ordered(a);int previous=0,canonical=0;
 for(const auto& r:rows){
  if(r.number<=previous||r.number>127||r.same_as<1||r.same_as>127||r.score_index<0||r.score_index>15)throw std::invalid_argument("Duplicate/out-of-domain source score track metadata");
  previous=r.number;
  if(r.number==r.same_as){const int expected=canonical<16?canonical:0;++canonical;if(r.score_index!=expected)throw std::invalid_argument("Score metadata is not original initialized canonical order");}
 }
 for(const auto& r:rows)if(r.number!=r.same_as){
  const auto parent=std::find_if(rows.begin(),rows.end(),[&](const auto& x){return x.number==r.same_as;});
  if(parent==rows.end()||parent->number!=parent->same_as||r.score_index!=parent->score_index)throw std::invalid_argument("Unsupported/missing source score alias parent");
 }
}
EventScoreAxes read_debug_event_score_axes(const driving::Memory& m){
 EventScoreAxes out;
 for(unsigned i=0;i<9;++i)out.lap_counts[i]=m.signed_byte(0x2ac028+i);
 const auto count=m.signed_word(0x3273a0);const auto base=m.word(0x32739c);
 if(count<1||count>127||!base)throw std::invalid_argument("Source TrackInfo score metadata is unavailable");
 for(int number=1;number<=count;++number){const auto row=address(base,std::size_t(number-1)*0xa0,0x48);
  if(!m.byte(row)||!m.signed_byte(row+0x40))continue;
  if(m.signed_byte(row+0x41)!=number)throw std::invalid_argument("Source TrackInfo slot/number correspondence differs");
  out.tracks.push_back({number,m.signed_byte(row+0x42),m.signed_half(row+0x46)});
 }
 validate_event_score_axes(out);return out;
}
EventScoreAxes read_retail_event_score_axes(const std::vector<std::uint8_t>& global){
 const auto chunks=hp2::assets::parse_chunks_top(global);const auto* table=hp2::assets::find_first(chunks,0x34201);
 if(!table||!table->size||table->size%0xa4||table->size/0xa4>127)throw std::invalid_argument("Retail score TrackInfo catalog is missing/misaligned");
 EventScoreAxes out;out.lap_counts={{1,2,3,4,5,6,7,8,10}};int canonical=0;
 for(unsigned i=0;i<table->size/0xa4;++i){const auto at=table->payload_offset()+i*0xa4;const auto* row=global.data()+at;
  // Independent Retail153608 and Debug153258 both clear named track54.
  if(!row[0]||!row[0x40]||i==53)continue;
  const int number=signed_bits<std::int8_t>(row[0x41]),parent=signed_bits<std::int8_t>(row[0x42]);
  if(number!=int(i+1))throw std::invalid_argument("Retail score TrackInfo slot/number correspondence differs");
  const int index=number==parent?(canonical<16?canonical++:(++canonical,0)):-1;
  out.tracks.push_back({number,parent,index});
 }
 for(auto& row:out.tracks)if(row.number!=row.same_as){const auto parent=std::find_if(out.tracks.begin(),out.tracks.end(),[&](const auto& p){return p.number==row.same_as;});
  if(parent==out.tracks.end()||parent->number!=parent->same_as)throw std::invalid_argument("Retail score alias requires an available canonical parent");
  row.score_index=parent->score_index;
 }
 validate_event_score_axes(out);return out;
}
EventScoreProfile decode_event_scores(EventScoreVersion version,const EventScoreAxes& axes,const std::vector<std::uint8_t>& p){
 validate_event_score_axes(axes);const auto l=layout(version);require_extent(p,l);EventScoreProfile out;
 for(std::size_t i=0;i<out.total.size();++i)out.total[i]=get_record(p.data()+l.total+8*i);
 for(std::size_t i=0;i<out.lap.size();++i)out.lap[i]=get_record(p.data()+l.lap+8*i);
 for(std::size_t i=0;i<out.names.size();++i)for(unsigned j=0;j<12;++j)out.names[i][j]=p[l.names+12*i+j];
 return out;
}
std::vector<std::uint8_t> encode_event_scores(EventScoreVersion version,const EventScoreAxes& axes,const EventScoreProfile& scores,const std::vector<std::uint8_t>& baseline){
 validate_event_score_axes(axes);const auto l=layout(version);require_extent(baseline,l);auto out=baseline;
 for(std::size_t i=0;i<scores.total.size();++i)put_record(out.data()+l.total+8*i,scores.total[i]);
 for(std::size_t i=0;i<scores.lap.size();++i)put_record(out.data()+l.lap+8*i,scores.lap[i]);
 for(std::size_t i=0;i<scores.names.size();++i)for(unsigned j=0;j<12;++j)out[l.names+12*i+j]=scores.names[i][j];
 return out;
}
EventScoreProfile transcode_event_scores(const EventScoreAxes& source_axes,const EventScoreProfile& scores,const EventScoreAxes& destination_axes){require_same_axes(source_axes,destination_axes);return scores;}
EventScoreProfile read_debug_event_scores(const driving::Memory& m,std::uint32_t fred,const EventScoreAxes& axes){
 require_debug_axes(m,axes);EventScoreProfile out;const auto l=layout(EventScoreVersion::Debug);
 const auto read=[&](std::size_t offset){const auto a=address(fred,offset,8);return EventScoreRecord{m.signed_word(a),m.signed_half(a+4),m.signed_half(a+6)};};
 for(std::size_t i=0;i<out.total.size();++i)out.total[i]=read(l.total+8*i);
 for(std::size_t i=0;i<out.lap.size();++i)out.lap[i]=read(l.lap+8*i);
 for(std::size_t i=0;i<out.names.size();++i)for(unsigned j=0;j<12;++j)out.names[i][j]=m.byte(address(fred,l.names+12*i+j,1));
 return out;
}
void write_debug_event_scores(driving::Memory& m,std::uint32_t fred,const EventScoreAxes& axes,const EventScoreProfile& scores){
 require_debug_axes(m,axes);const auto l=layout(EventScoreVersion::Debug);
 // All allocation, metadata checks and Memory resolution happen before writes.
 // No source pointer is captured in the public semantic payload.
 std::array<std::uint8_t*,7200> total;std::array<std::uint8_t*,800> lap;std::array<std::uint8_t*,128> names;
 for(std::size_t i=0;i<total.size();++i)total[i]=m.view(address(fred,l.total+8*i,8),8).data;
 for(std::size_t i=0;i<lap.size();++i)lap[i]=m.view(address(fred,l.lap+8*i,8),8).data;
 for(std::size_t i=0;i<names.size();++i)names[i]=m.view(address(fred,l.names+12*i,12),12).data;
 for(std::size_t i=0;i<total.size();++i)put_record(total[i],scores.total[i]);
 for(std::size_t i=0;i<lap.size();++i)put_record(lap[i],scores.lap[i]);
 for(std::size_t i=0;i<names.size();++i)for(unsigned j=0;j<12;++j)names[i][j]=scores.names[i][j];
}
}
