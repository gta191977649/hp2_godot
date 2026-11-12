#include "position_markers.hpp"
#include <stdexcept>
namespace port::track {
void initialize_position_marker_list(::port::driving::Memory& m){m.word(0x327378,0x327378);m.word(0x32737c,0x327378);}
PositionMarkerResources load_retail_position_markers(::port::driving::Memory& m,const std::vector<std::uint8_t>& bytes){
 PositionMarkerResources result;result.owner=&m;
 const auto word=[&](std::size_t p){if(p>bytes.size()||bytes.size()-p<4)throw std::runtime_error("Truncated track marker chunk word");return std::uint32_t(bytes[p])|std::uint32_t(bytes[p+1])<<8|std::uint32_t(bytes[p+2])<<16|std::uint32_t(bytes[p+3])<<24;};
 const auto walk=[&](auto&& self,std::size_t begin,std::size_t end)->void{
  for(auto p=begin;p<end;){if(end-p<8)throw std::runtime_error("Truncated track marker chunk");const auto id=word(p),size=word(p+4);if(size>end-p-8)throw std::runtime_error("Track marker chunk exceeds bundle");
   if(id==0x34146){if(size%24)throw std::runtime_error("Track marker payload is not an authored24-byte table");const auto a=m.allocate(size+8);for(std::size_t i=0;i<size+8;++i)m.byte(a+static_cast<std::uint32_t>(i),bytes[p+i]);hp2::debug::LoaderTrackGeometry(m,a);result.chunks.push_back(a);}
   else if(id&0x80000000u)self(self,p+8,p+8+size);
   p+=8+size;
  }
 };walk(walk,0,bytes.size());return result;
}
void unload_position_markers(::port::driving::Memory& m,PositionMarkerResources& r){if(r.owner!=&m)throw std::logic_error("Track marker Memory owner mismatch");for(auto i=r.chunks.rbegin();i!=r.chunks.rend();++i){hp2::debug::UnloaderTrackGeometry(m,*i);m.release(*i);}r.chunks.clear();r.owner=nullptr;}
}
