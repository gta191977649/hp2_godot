#include "track_environment.hpp"
#include "game_core/port/assets/elab_chunks.hpp"
#include "game_core/indep/src/trackpath.hpp"
#include "game_core/indep/src/track.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace port::driving {
namespace {
std::uint32_t word(const std::vector<std::uint8_t>& b,std::size_t p){if(p>b.size()||b.size()-p<4)throw std::runtime_error("Truncated track driving data");return b[p]|std::uint32_t(b[p+1])<<8|std::uint32_t(b[p+2])<<16|std::uint32_t(b[p+3])<<24;}
int half(const std::vector<std::uint8_t>& b,std::size_t p){if(p>b.size()||b.size()-p<2)throw std::runtime_error("Truncated track path index");return std::int16_t(std::uint16_t(b[p])|std::uint16_t(b[p+1])<<8);}
float scalar(const std::vector<std::uint8_t>& b,std::size_t p){auto bits=word(b,p);float value;std::memcpy(&value,&bits,4);if(!std::isfinite(value))throw std::runtime_error("Nonfinite track driving data");return value;}
std::uint32_t hash(const char* name){std::uint32_t value=0xffffffffu;for(;*name;++name)value=value*33+std::uint8_t(*name);return value;}
}
std::shared_ptr<const TrackEnvironment> load_track_environment(assets::Source& source,const port::track::Entry& entry){
    if(entry.resources.size()!=4)throw std::invalid_argument("Track entry lacks original resource paths");
    port::track::Scene scene;scene.entry=entry;
    scene.bundle=port::track::decompress_bundle(source.read(entry.resources[1]));
    return load_track_environment(scene);
}
std::shared_ptr<const TrackEnvironment> load_track_environment(const port::track::Scene& scene){
    auto result=std::make_shared<TrackEnvironment>();result->number=scene.entry.number;result->label=scene.entry.name;
    const auto& bytes=scene.bundle;result->topology.load(bytes);const auto chunks=hp2::assets::parse_chunks_top(bytes);
    const auto* markers=hp2::assets::find_first(chunks,0x34146);
    if(!markers||markers->size%24)throw std::runtime_error("Original forward starting-position markers are missing or malformed");
    bool start=false;std::array<float,2> xy{};
    // Retail00153608/Debug00153258 loader; Debug00149e80 chooses Param0 forward marker.
    for(auto at=markers->payload_offset();at<markers->payload_offset()+markers->size;at+=24)
        if(word(bytes,at+8)==hash("ForwardStartingPosition")&&word(bytes,at+12)==0){xy={{scalar(bytes,at+16),scalar(bytes,at+20)}};start=true;break;}
    if(!start)throw std::runtime_error("Original ForwardStartingPosition Param0 marker is absent; path fallback is not recovered");
    const auto* path_chunk=hp2::assets::find_first(chunks,0x80034147);
    if(!path_chunk||path_chunk->children.size()<2)throw std::runtime_error("Original starting-direction track path is missing");
    const auto& paths_chunk=path_chunk->children[0];const auto& points_chunk=path_chunk->children[1];
    // E-0048-D02: shipped bundle child tags34148/34149 corroborate the loader's
    // positional schema; reject a structurally valid unrelated container.
    if(paths_chunk.tag!=0x34148||points_chunk.tag!=0x34149)throw std::runtime_error("Original track path child identities mismatch");
    if(paths_chunk.size%40||points_chunk.size%20)throw std::runtime_error("Original track path row stride mismatch");
    port::track::Paths paths;
    for(auto at=points_chunk.payload_offset();at<points_chunk.payload_offset()+points_chunk.size;at+=20){
        port::track::PathPoint p;p.position={{scalar(bytes,at),scalar(bytes,at+4)}};p.path=std::int8_t(bytes.at(at+8));for(unsigned n=0;n<3;++n)p.neighbours[n]=half(bytes,at+10+n*2);paths.points.push_back(p);
    }
    for(auto at=paths_chunk.payload_offset();at<paths_chunk.payload_offset()+paths_chunk.size;at+=40){
        port::track::PathRow p;p.begin=half(bytes,at+2);p.end=half(bytes,at+4);const auto count=half(bytes,at+6);p.spline.looping=bytes.at(at+1)!=0;
        if(p.begin<0||p.end<=p.begin||std::size_t(p.end)>paths.points.size()||count!=p.end-p.begin)throw std::runtime_error("Original track path range is invalid");
        for(int i=p.begin;i<p.end;++i)p.spline.points.push_back(paths.points[i].position);
        paths.paths.push_back(std::move(p));
    }
    for(const auto& point:paths.points){if(point.path<0||std::size_t(point.path)>=paths.paths.size())throw std::runtime_error("Track point path index is invalid");for(int n:point.neighbours)if(n>=0&&std::size_t(n)>=paths.points.size())throw std::runtime_error("Track point neighbour index is invalid");}
    // Debug00325da0 raw bits3d4ccccd, first-position heading projection E-0048-D02.
    result->direction=hp2::recovered::TrackPathManager::UpdatePosition(paths,xy,0.05f);
    auto contact=result->topology.elevation({{xy[0],xy[1],0}});
    if(!contact.found)throw std::runtime_error("Original start marker has no topology road surface");
    debug_terrain_from_retail(contact.terrain);result->start={{xy[0],xy[1],contact.height}};return result;
}

}
