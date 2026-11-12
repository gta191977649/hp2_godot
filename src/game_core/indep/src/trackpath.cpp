// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\trackpath.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): TrackPathCoordinate::UpdatePosition() [0x002BB238]
#include "game_core/indep/inc/TrackPath.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/indep/src/trackpath.hpp"
#include <algorithm>
#include <cmath>

namespace hp2::debug {
// Debug00202BE0, E-0099-W122; original trackpath.cpp. Raw shared points retain
// source XYZ/distance values; path-order/first-candidate ties are unchanged.
std::uint32_t TrackPathManager::FindClosestPoint(::port::driving::Memory& m,const ::port::track::RoutePaths& paths,const std::array<float,2>& position,float distance){std::uint32_t selected=0;float best=0;for(const auto& row:paths.geometry.paths)for(int i=row.begin;i<row.end;++i){if(i<0||std::size_t(i)>=paths.geometry.points.size()||!paths.point_table)hp2::not_implemented("0x00202BE0","original distance-biased path search requires valid shared point records");const auto p=paths.point_table+20u*unsigned(i);const float x=m.scalar(p)-position[0],y=m.scalar(p+4)-position[1];const float spatial=std::sqrt(x*x+y*y),bias=std::fabs(CompareDistances(paths.track_length,m.scalar(p+16),distance))-100.f;const float score=std::max(spatial,bias);if(!selected||score<best){selected=p;best=score;}}return selected;}
// Debug00202ED0, E-0099-W122. Selected point/XY stores precede UpdatePosition.
void TrackPathManager::ResetPosition(::port::driving::Memory& m,std::uint32_t coordinate,const ::port::track::RoutePaths& paths,const std::array<float,2>& position,float distance){m.word(coordinate+8,FindClosestPoint(m,paths,position,distance));m.scalar(coordinate,position[0]);m.scalar(coordinate+4,position[1]);TrackPathCoordinate::UpdatePosition(m,coordinate,paths,position);}
// Debug00203720, E-0099-W122; actual source wrapper, not interior00202F10.
void TrackPathCoordinate::ResetPosition(::port::driving::Memory& m,std::uint32_t coordinate,const ::port::track::RoutePaths& paths,const std::array<float,2>& position,float distance){TrackPathManager::ResetPosition(m,coordinate,paths,position,distance);}
// Debug00203480, E-0099-W17. Explicit track length binds the original manager
// field without claiming the host decoded carrier has the original ABI.
float TrackPathManager::CompareDistances(float length,float first,float second){
    const float delta=first-second;
    if(delta< -length*0.5f)return delta+length;
    if(delta<=length*0.5f)return delta;
    return delta-length;
}
}
#include "game_core/indep/src/quickspline.hpp"
#include <cmath>
#include <stdexcept>

namespace hp2::recovered {
// ELF 0x002086B8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002086b8.md.
void TrackPathCoordinate::Sub_002086B8() {
    hp2::not_implemented("0x002086B8", "TrackPathCoordinate::Sub_002086B8 (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace port::track {
namespace {
// Debug002370A8/00237340/00237448, E-0099-W08. Calibrated W has
// distance-continuous endpoint values even when XY wraps around a loop.
float distance_component(const RoutePaths& paths,std::size_t row,float param){
    const auto& spline=paths.geometry.paths.at(row).spline;
    const auto& d=paths.distances.at(row);const int count=int(d.cumulative.size());
    param=hp2::recovered::QuickSpline::ClampParam(spline,param);
    const int base=int(std::floor(param));
    std::array<std::array<float,2>,4> points{};
    for(int j=0;j<4;++j){const int i=base+j-1;float value;
        if(i>=0 && i<count)value=d.cumulative.at(i);
        else if(spline.looping){
            if(i==-1)value=d.cumulative.back()-d.length;
            else if(i==count)value=d.length+d.cumulative.front();
            else if(i==count+1)value=d.length+d.cumulative.at(1);
            else throw std::out_of_range("Calibrated loop distance control point");
        }else if(i==-1)value=-d.cumulative.at(1);
        else if(i==count)value=(d.length+d.length)-d.cumulative.back();
        else if(i<0)value=d.cumulative.front()+(d.cumulative.front()-d.cumulative.at(1))*float(-i);
        else value=d.cumulative.back()+(d.cumulative.back()-d.cumulative.at(count-2))*float(i-count+1);
        points[j][0]=value;
    }
    return hp2::recovered::QuickSpline::EvaluateSpline(points,param-float(base),0)[0];
}
float point_distance(const RoutePaths& paths,int index,const std::array<float,2>& p){
    const auto& xy=paths.geometry.points.at(index).position;const float x=xy[0]-p[0],y=xy[1]-p[1];
    return std::sqrt(x*x+y*y);
}
}
}
namespace hp2::debug {
// Debug00202780/00237940; E-0099-W08. Original cumulative arc-length
// samples, not a distance inferred from nearest-point coordinates.
void TrackPathManager::Init(port::track::RoutePaths& paths){
    if(paths.geometry.paths.size()!=paths.distances.size() || paths.distances.empty())
        throw std::invalid_argument("Track path distance metadata is missing");
    paths.track_length=paths.distances.front().end;
    for(std::size_t row=0;row<paths.geometry.paths.size();++row){
        const auto& spline=paths.geometry.paths[row].spline;auto& d=paths.distances[row];
        const int count=int(spline.points.size());
        if(count<(spline.looping?3:2))throw std::invalid_argument("Insufficient original track path control points");
        // EXTRAPOLATED has MaxParam=9999999 (00326174), so calibration
        // also samples the segment AFTER the last stored control point.
        d.cumulative.resize(count);float total=0;const int maximum=count;
        for(int i=0;i<count;++i){d.cumulative[i]=total;
            if(i<maximum){auto previous=hp2::recovered::QuickSpline::GetPoint(spline,float(i),0);
                for(int step=1;step<=16;++step){const auto p=hp2::recovered::QuickSpline::GetPoint(spline,float(i)+float(step)/16.f,0);
                    const float x=p[0]-previous[0],y=p[1]-previous[1];total+=std::sqrt(x*x+y*y);previous=p;}
            }
        }
        d.length=total;
    }
}
// Debug00202B00; E-0099-W08. Retains first-candidate tie behavior and path order.
int TrackPathManager::FindClosestPoint(const port::track::RoutePaths& paths,const std::array<float,2>& p){
    int selected=-1;float best=0;
    for(const auto& row:paths.geometry.paths)for(int i=row.begin;i<row.end;++i){
        const float d=port::track::point_distance(paths,i,p);if(selected<0||d<best){selected=i;best=d;}}
    return selected;
}
// Debug00202D58; E-0099-W08.
int TrackPathManager::FindClosestAdjacentPointOnSamePath(const port::track::RoutePaths& paths,
    const std::array<float,2>& p,int point){
    const auto& row=paths.geometry.paths.at(paths.geometry.points.at(point).path);
    std::array<int,3> candidates{{point,-1,-1}};int count=1;
    if(point==row.begin){if(row.spline.looping)candidates[count++]=row.end-1;}
    else candidates[count++]=point-1;
    if(point==row.end-1){if(row.spline.looping)candidates[count++]=row.begin;}
    else candidates[count++]=point+1;
    int selected=-1;float best=0;
    for(int i=0;i<count;++i){const float d=port::track::point_distance(paths,candidates[i],p);
        if(selected<0||d<best){selected=candidates[i];best=d;}}
    return selected;
}
// Debug002035D8/002036E0 ->00202F30; E-0099-W08.
TrackPathCoordinate::TrackPathCoordinate(const port::track::RoutePaths& paths,
    const std::array<float,2>& p,float tolerance){UpdatePosition(paths,p,tolerance);}
void TrackPathCoordinate::UpdatePosition(const port::track::RoutePaths& paths,
    const std::array<float,2>& p,float tolerance){
    int point=pTrackPoint;const float x=p[0]-Position[0],y=p[1]-Position[1];
    if(point<0 || 128.f<std::sqrt(x*x+y*y))point=TrackPathManager::FindClosestPoint(paths,p);
    if(point<0){Position=p;PathDirection={{1,0}};RatioAroundTrack=0;DistanceAroundTrack=0;return;}
    point=TrackPathManager::FindClosestAdjacentPointOnSamePath(paths,p,point);
    float best=port::track::point_distance(paths,point,p);
    // The original rereads the currently selected point's neighbour at each step.
    for(int i=0;i<3;++i){const int next=paths.geometry.points.at(point).neighbours[i];
        if(next>=0){const float d=port::track::point_distance(paths,next,p);if(d<best){best=d;point=next;}}}
    const auto& row=paths.geometry.paths.at(paths.geometry.points.at(point).path);
    float initial=SplineParam;if(point!=pTrackPoint){pTrackPoint=point;initial=float(point-row.begin);}
    const float param=hp2::recovered::QuickSpline::FindClosestParam(row.spline,p,initial,tolerance);
    auto direction=hp2::recovered::QuickSpline::GetPoint(row.spline,param,1);
    const float length=std::sqrt(direction[0]*direction[0]+direction[1]*direction[1]);
    if(!std::isfinite(length)||length==0)hp2::not_implemented("0x00202F30","degenerate original path derivative normalization");
    direction[0]/=length;direction[1]/=length;
    const auto index=std::size_t(paths.geometry.points.at(point).path);const auto& d=paths.distances.at(index);
    float distance=d.begin+port::track::distance_component(paths,index,param)*d.ratio;
    if(paths.reverse_direction){direction[0]=-direction[0];direction[1]=-direction[1];distance=paths.track_length-distance;}
    SplineParam=param;Position=p;PathDirection=direction;DistanceAroundTrack=distance;
    if(paths.track_length==0)hp2::not_implemented("0x00202F30","zero original track length division");
    RatioAroundTrack=distance/paths.track_length;
}
}

#include "game_core/indep/src/trackpath.hpp"
#include "game_core/indep/src/quickspline.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace hp2::recovered {
// Debug00202B00/00202D58/00202F30; E-0048-D02; bounded first-position heading.
std::array<float,2> TrackPathManager::UpdatePosition(const port::track::Paths& data,const std::array<float,2>& position,float tolerance){
    auto distance=[&](int index){const auto& p=data.points.at(index).position;float x=p[0]-position[0],y=p[1]-position[1];return std::sqrt(x*x+y*y);};
    int selected=-1;float best=0;
    for(const auto& path:data.paths)for(int i=path.begin;i<path.end;++i){float d=distance(i);if(selected<0||d<best){selected=i;best=d;}}
    if(selected<0)throw std::runtime_error("Original track path has no starting direction");
    const auto& initial=data.points.at(selected);const auto& row=data.paths.at(initial.path);
    std::vector<int> adjacent{selected};
    if(selected==row.begin){if(row.spline.looping)adjacent.push_back(row.end-1);}else adjacent.push_back(selected-1);
    if(selected==row.end-1){if(row.spline.looping)adjacent.push_back(row.begin);}else adjacent.push_back(selected+1);
    int nearest=-1;for(int i:adjacent){float d=distance(i);if(nearest<0||d<best){nearest=i;best=d;}}
    selected=nearest;best=distance(selected);
    // The original scans exactly three neighbour numbers after same-path selection.
    for(unsigned n=0;n<3;++n){const int i=data.points.at(selected).neighbours[n];if(i>=0){float d=distance(i);if(d<best){selected=i;best=d;}}}
    const auto& point=data.points.at(selected);const auto& path=data.paths.at(point.path);
    const float param=QuickSpline::FindClosestParam(path.spline,position,float(selected-path.begin),tolerance);
    auto direction=QuickSpline::GetPoint(path.spline,param,1);
    const float length=std::sqrt(direction[0]*direction[0]+direction[1]*direction[1]);
    if(!std::isfinite(length)||length==0)throw std::runtime_error("Original starting path derivative is degenerate");
    direction[0]/=length;direction[1]/=length;return direction;
}
}

namespace hp2::recovered {
// ELF 0x00207C68; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00207c68.md.
void TrackPathManager::FindClosestPoint() {
    hp2::not_implemented("0x00207C68", "TrackPathManager::FindClosestPoint (signature and full body unresolved)");
}

// ELF 0x00207ED0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00207ed0.md.
void TrackPathManager::FindClosestAdjacentPointOnSamePath() {
    hp2::not_implemented("0x00207ED0", "TrackPathManager::FindClosestAdjacentPointOnSamePath (signature and full body unresolved)");
}

// ELF 0x00208598; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00208598.md.
void TrackPathManager::CompareDistances() {
    hp2::not_implemented("0x00208598", "TrackPathManager::CompareDistances (signature and full body unresolved)");
}

// ELF 0x002085E8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002085e8.md.
void TrackPathManager::ComparePercentDistances() {
    hp2::not_implemented("0x002085E8", "TrackPathManager::ComparePercentDistances (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::debug {
// Debug002035A0; E-0099-W26. Exact logical coordinate constructor stores.
void TrackPathCoordinate::Sub_002035A0(::port::driving::Memory& m,std::uint32_t a){
 m.word(a,0);m.scalar(a+0x18,1);m.word(a+4,0);m.word(a+8,0);m.word(a+0xc,0);m.word(a+0x10,0);m.word(a+0x14,0);
 m.word(a+0x1c,0);m.word(a+0x20,0);m.word(a+0x24,0);
}
// Debug002036E0 ->00202F30; E-0099-W26. Uses source-backed raw logical pointers.
void TrackPathCoordinate::UpdatePosition(::port::driving::Memory& m,std::uint32_t a,const ::port::track::RoutePaths& paths,const std::array<float,2>& pos){
 if(!paths.point_table)hp2::not_implemented("0x002036E0","shared original TrackPathPoint resource table is not loaded");
 TrackPathCoordinate decoded;decoded.Position={{m.scalar(a),m.scalar(a+4)}};
 decoded.PathDirection={{m.scalar(a+0x18),m.scalar(a+0x1c)}};decoded.SplineParam=m.scalar(a+0xc);
 decoded.RatioAroundTrack=m.scalar(a+0x10);decoded.DistanceAroundTrack=m.scalar(a+0x14);
 const auto old=m.word(a+8);
 if(old){const auto offset=std::uint64_t(old)-paths.point_table;
  if(old<paths.point_table||offset%20||offset/20>=paths.geometry.points.size())throw std::out_of_range("TrackPathCoordinate original point pointer outside loaded table");
  decoded.pTrackPoint=static_cast<int>(offset/20);
 }
 decoded.UpdatePosition(paths,pos,m.scalar(0x00325da0));
 m.scalar(a,decoded.Position[0]);m.scalar(a+4,decoded.Position[1]);
 if(decoded.pTrackPoint>=0)m.word(a+8,paths.point_table+static_cast<std::uint32_t>(decoded.pTrackPoint)*20);
 m.scalar(a+0xc,decoded.SplineParam);m.scalar(a+0x10,decoded.RatioAroundTrack);m.scalar(a+0x14,decoded.DistanceAroundTrack);
 m.scalar(a+0x18,decoded.PathDirection[0]);m.scalar(a+0x1c,decoded.PathDirection[1]);
}
}

namespace hp2::debug {
// Debug00203630; E-0099-W26. Confirmed no stores/callees on embedded-object destruction.
void TrackPathCoordinate::Sub_00203630(::port::driving::Memory& m,std::uint32_t a,int in_chrg){if(in_chrg&1)m.release(a);}
}

namespace hp2::debug {
// Debug00203708 ->00203238; E-0099-W34. The branch at00203404 is preserved:
// only a zero decremented NumPoints branches back; otherwise original code
// reaches00203410 without initializing direction. Those stack bits are unknown.
void TrackPathCoordinate::UpdatePosition(::port::driving::Memory& m,std::uint32_t a,const ::port::track::RoutePaths& paths,float d,float lateral,int path_number){
 const auto& data=paths.geometry;int selected=-1;const auto old=m.word(a+8);
 if(old){if(old<paths.point_table||(old-paths.point_table)%20||(old-paths.point_table)/20>=data.points.size())throw std::out_of_range("Original distance coordinate point outside loaded table");selected=static_cast<int>((old-paths.point_table)/20);}
 else if(path_number>=0&&std::size_t(path_number)<data.paths.size()&&data.paths[path_number].begin>=0)selected=data.paths[path_number].begin;
 if(selected<0){m.scalar(a+0x10,0);m.scalar(a+0x18,1);m.scalar(a,0);m.scalar(a+4,0);m.scalar(a+0x1c,0);m.scalar(a+0x14,0);return;}
 const auto& row=data.paths.at(data.points.at(selected).path);const int next=selected<row.end-1?selected+1:-1,prev=selected>row.begin?selected-1:-1;
 const auto distance=[&](int i){if(i<0)hp2::not_implemented("0x00203378","original distance walk dereferences missing adjacent point");return m.scalar(paths.point_table+20*static_cast<unsigned>(i)+16);};
 if((d<distance(selected)&&prev>=0)||(distance(selected)<d&&distance(next)<d))hp2::not_implemented("0x00203410","original distance-walk branch consumes uninitialized stack direction; no stack bits are known");
 const auto& v=data.points.at(selected).position;std::array<float,2> direction{};
 if(next>=0){const auto& n=data.points.at(next).position;direction={{n[0]-v[0],n[1]-v[1]}};}
 else {if(prev<0)hp2::not_implemented("0x002033C8","original single-point path has no previous direction");const auto& p=data.points.at(prev).position;direction={{v[0]-p[0],v[1]-p[1]}};}
 // Original bNormalize00251C70 (zero vector becomes1,0), then two sequential
 // bScaleAdd00251DA8 operations. Original d is not reduced by point.Distance.
 const float length=std::sqrt(direction[0]*direction[0]+direction[1]*direction[1]);if(length==0)direction={{1,0}};else{direction[0]/=length;direction[1]/=length;}
 std::array<float,2> position{{v[0]+direction[0]*d,v[1]+direction[1]*d}};
 position[0]=position[0]+direction[1]*lateral;position[1]=position[1]-direction[0]*lateral;UpdatePosition(m,a,paths,position);
}
}

namespace hp2::debug {
// Debug002034D0; TASK-0099 E-0099-W34.
float TrackPathManager::ComparePercentDistances(float a,float b){float d=a-b;if(d< -50)return d+100;if(d<=50)return d;return d-100;}
}

#include "game_core/indep/inc/Replay.hpp"
namespace hp2::debug {
// Debug00203650 ->00202A68; E-0099-W74. RoutePaths binds actual loaded original point-table operand.
void TrackPathCoordinate::DoSnapshot(::port::driving::Memory& m,std::uint32_t a,std::uint32_t snapshot,const ::port::track::RoutePaths& paths){
 ReplaySnapshot s(m,snapshot);s.BeginSection(0x2b2e20);s.MoveData(a,8);
 ::port::driving::Memory::Temporary local(m,2);const auto point=m.word(a+8);std::uint16_t number=0xffff;
 if(point){if(!paths.point_table)hp2::not_implemented("0x00202A68","Original loaded path point table is absent");number=static_cast<std::uint16_t>(((point-paths.point_table)*0xcccccccdu)>>2);}
 m.half(local.address(),number);s.MoveData(local.address(),2);const auto restored=m.signed_half(local.address());
 m.word(a+8,restored<0?0:paths.point_table+20u*static_cast<unsigned>(restored));
 for(unsigned off:{12u,16u,20u})s.MoveData(a+off,4);
 s.MoveData(a+24,8);s.EndSection();
}
}
