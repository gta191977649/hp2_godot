// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/src/quickspline.hpp"

#include "game_core/port/not_implemented.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace hp2::recovered {
// Debug 002372E8; E-0048-D02. Only LOOP/EXTRAPOLATED track-path endpoints are in scope.
float QuickSpline::ClampParam(const port::track::Spline& s,float value){
    if(s.looping){float q=value/float(s.points.size());value=(q-std::floor(q))*float(s.points.size());}
    return value;
}
// Debug 00237268 and InitBasisMatricies00236B10; E-0048-D02.
std::array<float,2> QuickSpline::EvaluateSpline(const std::array<std::array<float,2>,4>& points,float t,int deriv){
    const std::array<std::array<float,4>,4> basis0{{{{0,-1,2,-1}},{{2,0,-5,3}},{{0,1,4,-3}},{{0,0,-1,1}}}};
    const std::array<std::array<float,4>,4> basis1{{{{-1,4,-3,0}},{{0,-10,9,0}},{{1,8,-9,0}},{{0,-2,3,0}}}};
    if(deriv<0||deriv>1)throw std::invalid_argument("Track heading spline supports derivatives 0 and 1");
    const auto& basis=deriv?basis1:basis0;const std::array<float,4> powers{{1,t,t*t,t*(t*t)}};
    std::array<float,4> weights{};
    for(unsigned i=0;i<4;++i){for(unsigned j=0;j<4;++j)weights[i]+=basis[i][j]*0.5f*powers[j];}
    std::array<float,2> out{};for(unsigned axis=0;axis<2;++axis)for(unsigned i=0;i<4;++i)out[axis]+=points[i][axis]*weights[i];return out;
}
// Debug 00237448/002370A8/00237340; E-0048-D02. XY projection of original endpoint handling.
std::array<float,2> QuickSpline::GetPoint(const port::track::Spline& s,float param,int deriv){
    if(s.points.size()<(s.looping?3u:2u))throw std::invalid_argument("Insufficient track spline points");
    param=ClampParam(s,param);if(!std::isfinite(param)||std::abs(param)>1000000)throw std::invalid_argument("Invalid track spline parameter");
    const int base=int(std::floor(param));std::array<std::array<float,2>,4> points;
    for(int j=0;j<4;++j){int index=base+j-1;const int count=int(s.points.size());
        if(s.looping){index%=count;if(index<0)index+=count;points[j]=s.points[index];}
        else if(index<0){for(unsigned axis=0;axis<2;++axis)points[j][axis]=s.points[0][axis]+(s.points[0][axis]-s.points[1][axis])*float(-index);}
        else if(index>=count){for(unsigned axis=0;axis<2;++axis)points[j][axis]=s.points[count-1][axis]+(s.points[count-1][axis]-s.points[count-2][axis])*float(index-count+1);}
        else points[j]=s.points[index];
    }
    return EvaluateSpline(points,param-float(base),deriv);
}
// Debug 00237748; E-0048-D02. No independent path-heading fit or finite-difference model.
float QuickSpline::FindClosestParam(const port::track::Spline& s,const std::array<float,2>& point,float initial,float tolerance){
    float limit=1,previous_step=0,previous_projection=0;int remaining=0x3e6;
    do{const auto position=GetPoint(s,initial,0),direction=GetPoint(s,initial,1);
        float length=std::sqrt(direction[0]*direction[0]+direction[1]*direction[1]);if(length==0)break;
        const float projection=((point[0]-position[0])*direction[0]+(point[1]-position[1])*direction[1])/length;
        if(std::abs(projection)<tolerance)return initial;
        if(previous_step!=0){length=(previous_projection-projection)/previous_step;if(length==0)return initial;}
        const float step=std::min(limit,std::max(-limit,projection/length));initial=ClampParam(s,initial+step);
        if(step*previous_step<0)limit*=0.5f;
        previous_step=step;previous_projection=projection;
    }while(--remaining!=0);return initial;
}
}

namespace hp2::recovered {
// ELF 0x0023D368; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0023d368.md.
void QuickSpline::FixupEndpoints() {
    hp2::not_implemented("0x0023D368", "QuickSpline::FixupEndpoints (signature and full body unresolved)");
}

// ELF 0x0023D600; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0023d600.md.
void QuickSpline::GenerateExtrapolatedControlPoints() {
    hp2::not_implemented("0x0023D600", "QuickSpline::GenerateExtrapolatedControlPoints (signature and full body unresolved)");
}

// ELF 0x0023D868; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0023d868.md.
void QuickSpline::FindClosestControlPoint() {
    hp2::not_implemented("0x0023D868", "QuickSpline::FindClosestControlPoint (signature and full body unresolved)");
}
} // namespace hp2::recovered
