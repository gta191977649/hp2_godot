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


#include "game_core/port/race/control_construction.hpp"
namespace hp2::debug {
// Debug00236B10; E-0099-W24. Listing resolves misleading packed-vector decompilation.
void InitBasisMatricies(::port::driving::Memory& m){
 if(m.word(0x00328da0))return;
 const std::array<std::array<float,16>,3> matrices{{
  {{0,-1,2,-1, 2,0,-5,3, 0,1,4,-3, 0,0,-1,1}},
  {{-1,4,-3,0, 0,-10,9,0, 1,8,-9,0, 0,-2,3,0}},
  {{4,-6,0,0, -10,18,0,0, 8,-18,0,0, -2,6,0,0}}
 }};
 for(unsigned k=0;k<3;++k){std::array<float,16> matrix{};
  for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col)matrix[row*4+col]=matrices[k][col*4+row]*0.5f;
  m.matrix(0x003141f0+k*0x40,matrix);
 }
 m.word(0x00328da0,1);
}
// Debug00237010; E-0099-W24.
void QuickSpline::SetNumControlPoints(int n){
 auto& m=m_;const auto a=address_;m.byte(a+0x14,1);m.half(a+0x1a,static_cast<std::uint16_t>(n));
 if(!(n&0xffff)){m.word(a+0xc,0);m.word(a+8,0);return;}
 const auto endpoint=m.signed_word(a);
 if(endpoint==1){m.word(a+0xc,0);m.scalar(a+8,static_cast<float>(m.half(a+0x1a)-1));}
 else if(endpoint==2){m.word(a+0xc,m.word(0x00326170));m.word(a+8,m.word(0x00326174));}
 else if(endpoint==0){m.word(a+0xc,0);m.scalar(a+8,static_cast<float>(m.half(a+0x1a)));}
}
// Debug00236EE0; E-0099-W24.
void QuickSpline::Init(int endpoint,int basis){
 auto& m=m_;const auto a=address_;m.word(a,endpoint);m.word(a+4,basis);m.word(a+0x24,0);
 if(basis==0)m.word(a+0x24,0x003141f0);
 m.byte(a+0x16,2);m.word(a+0x20,m.word(a+0x1c)+16);if(m.word(a)==0)m.byte(a+0x16,3);
 m.word(a+0x10,0);SetNumControlPoints(0);InitBasisMatricies(m);
}
// Debug00236D88; E-0099-W24. Vector default constructors write no buffer bytes.
void QuickSpline::Sub_00236D88(int endpoint,int basis,int maximum){
 m_.half(address_+0x18,static_cast<std::uint16_t>(maximum));m_.byte(address_+0x15,1);
 m_.word(address_+0x1c,m_.allocate((static_cast<std::uint32_t>(maximum)&0xffffu)*16+48));Init(endpoint,basis);
}
}


namespace hp2::debug {
// Debug00236E78; E-0099-W24. Borrowed buffers are never freed.
void QuickSpline::Sub_00236E78(int flags){if(m_.byte(address_+0x15))if(const auto buffer=m_.word(address_+0x1c))m_.release(buffer);if(flags&1)m_.release(address_);}
}

#include "game_core/src/bMath.hpp"
#include <cmath>
namespace hp2::debug {
// Debug002370A8, E-0099-W60. Real allocated endpoint slots and original
// four-lane extrapolation; optional length changes only source W fields.
void QuickSpline::FixupEndpoints(){auto& m=m_;const auto a=address_;const auto n=m.half(a+0x1a);if(!n)return;const auto p=m.word(a+0x20),last=p+std::uint32_t(n-1)*16,end=p+std::uint32_t(n)*16;const int kind=m.signed_word(a);const float length=m.scalar(a+0x10);
 if(kind==1){m.vector(p-16,m.vector(p));m.vector(end,m.vector(last));if(length!=0){m.scalar(p-4,-m.scalar(p+28));m.scalar(end+12,(length+length)-m.scalar(last+12));}return;}
 if(kind==0){m.vector(p-16,m.vector(last));m.vector(end,m.vector(p));m.vector(end+16,m.vector(p+16));if(length!=0){m.scalar(p-4,m.scalar(last+12)-length);m.scalar(end+12,length+m.scalar(p+12));m.scalar(end+28,length+m.scalar(p+28));}return;}
 if(kind!=2)return;
 if(n<2)hp2::not_implemented("0x002370A8","extrapolated original spline requires its second authored control point");
 const auto first=m.vector(p),second=m.vector(p+16),previous=m.vector(last-16),final=m.vector(last);std::array<float,4> before{},after{};for(unsigned lane=0;lane<4;++lane){before[lane]=(second[lane]-first[lane])*-1+first[lane];after[lane]=(final[lane]-previous[lane])+final[lane];}m.vector(p-16,before);m.vector(end,after);if(length!=0){m.scalar(p-4,-m.scalar(p+28));m.scalar(end+12,(length+length)-m.scalar(last+12));}
}
// Debug002372E8, E-0099-W60. Extrapolated type2 intentionally does not clamp.
float QuickSpline::ClampParam(float value){const auto kind=m_.signed_word(address_);if(kind==0)hp2::not_implemented("0x002372E8","original loop bFMod dependency is not reconstructed");if(kind==1){const float minimum=m_.scalar(address_+0xc),maximum=m_.scalar(address_+8)-m_.scalar(0x326178);if(!(minimum<value))value=minimum;if(maximum<value)value=maximum;}return value;}
// Debug00237340/listing, E-0099-W60. The exact source permits -1 and n
// through fixed endpoint storage; farther indices extrapolate whole vectors.
void QuickSpline::GenerateExtrapolatedControlPoints(std::array<std::array<float,4>,4>& output,int start){const int n=m_.half(address_+0x1a);if(n<2)hp2::not_implemented("0x00237340","original extrapolation requires two real control points");const auto p=m_.word(address_+0x20);for(int i=0;i<4;++i){const int index=start+i;if(index<-1){const auto first=m_.vector(p),next=m_.vector(p+16);for(unsigned lane=0;lane<4;++lane)output[std::size_t(i)][lane]=(first[lane]-next[lane])*float(-index)+first[lane];}else if(n<index){const auto last=m_.vector(p+std::uint32_t(n-1)*16),previous=m_.vector(p+std::uint32_t(n-2)*16);for(unsigned lane=0;lane<4;++lane)output[std::size_t(i)][lane]=(last[lane]-previous[lane])*float(index-n+1)+last[lane];}else output[std::size_t(i)]=m_.vector(p+std::uint32_t(index)*16);}}
// Debug00237268, E-0099-W60. Actual initialized basis matrices and original
// vector4 multiply order for polynomial weights then authored control points.
void QuickSpline::EvaluateSpline(std::array<float,4>& out,float t,int derivative,const std::array<std::array<float,4>,4>& controls){if(derivative<0||derivative>2)hp2::not_implemented("0x00237268","original spline derivative outside recovered basis-matrix allocation");const auto basis=m_.word(address_+0x24);if(!basis)hp2::not_implemented("0x00237268","actual original spline basis matrix is not bound");const float square=t*t;std::array<float,4> polynomial{{1,t,square,t*square}},weights{};hp2::recovered::bMulMatrix(weights,m_.matrix(basis+std::uint32_t(derivative)*64),polynomial);std::array<float,16> matrix{};for(unsigned i=0;i<4;++i)for(unsigned lane=0;lane<4;++lane)matrix[i*4+lane]=controls[i][lane];hp2::recovered::bMulMatrix(out,matrix,weights);}
// Debug00237448, E-0099-W60. Negative parameter floor and extrapolated
// endpoint selection remain source operations, not a substituted curve fit.
void QuickSpline::GetPoint(std::array<float,4>& out,float parameter,int derivative){if(m_.byte(address_+0x14)){FixupEndpoints();m_.byte(address_+0x14,0);}const int n=m_.half(address_+0x1a);if(!n){out={{0,0,0,0}};return;}const float value=ClampParam(parameter);if(!std::isfinite(value)||value<=-2147483648.f||value>=2147483648.f)hp2::not_implemented("0x00237448","exceptional original spline parameter integer conversion");const float lower=std::floor(value);const int index=int(lower);std::array<std::array<float,4>,4> controls{};if(m_.word(address_)==2&&(index<0||n-1<=index))GenerateExtrapolatedControlPoints(controls,index-1);else{const auto p=m_.word(address_+0x20);for(unsigned i=0;i<4;++i)controls[i]=m_.vector(p+std::uint32_t(index-1+int(i))*16);}EvaluateSpline(out,value-lower,derivative,controls);}
// Debug00237748, E-0099-W60. Original capped secant/derivative iteration,
// signed step reversal and source stopping tolerance; all dots consume XYZ.
float QuickSpline::FindClosestParam(const std::array<float,4>& point,float parameter,float tolerance){if(m_.byte(address_+0x14)){FixupEndpoints();m_.byte(address_+0x14,0);}if(!m_.half(address_+0x1a))return 0;float maximum=1,last_step=0,last_error=0;for(int iteration=0;iteration<0x3e6;++iteration){std::array<float,4> position{},derivative{};GetPoint(position,parameter,0);GetPoint(derivative,parameter,1);float slope=std::sqrt((derivative[0]*derivative[0]+derivative[1]*derivative[1])+derivative[2]*derivative[2]);if(slope==0)return parameter;const float error=(((point[0]-position[0])*derivative[0]+(point[1]-position[1])*derivative[1])+(point[2]-position[2])*derivative[2])/slope;if(std::abs(error)<tolerance)return parameter;if(last_step!=0){slope=(last_error-error)/last_step;if(slope==0)return parameter;}float step=error/slope;const float negative=-maximum;if(!(negative<step))step=negative;if(maximum<step)step=maximum;parameter=ClampParam(parameter+step);if(step*last_step<0)maximum*=.5f;last_step=step;last_error=error;}return parameter;}
}

namespace hp2::debug {
// Debug002375A8,E-0099-W102. Source local forward then original-start backward
// descent, strict distance comparisons retain ties. XYZ only; W is unconsumed.
int QuickSpline::FindClosestControlPoint(const std::array<float,4>& point,int start){const int n=m_.half(address_+0x1a);const auto points=m_.word(address_+0x20);if(n<=0||start<0||start>=n)hp2::not_implemented("0x002375A8","original closest-control-point input requires defined authored storage");auto distance=[&](int index){const auto p=points+16*index;const float x=point[0]-m_.scalar(p),y=point[1]-m_.scalar(p+4),z=point[2]-m_.scalar(p+8),square=(x*x+y*y)+z*z;if(!std::isfinite(square))hp2::not_implemented("0x002375A8","exceptional original VU closest-control-point distance");return std::sqrt(square);};int best=start;float value=distance(start);for(int i=start+1;i<n;++i){const float next=distance(i);if(value<=next)break;best=i;value=next;}for(int i=start-1;i>=0;--i){const float next=distance(i);if(value<=next)return best;best=i;value=next;}return best;}
}

#include "game_core/indep/inc/Replay.hpp"
namespace hp2::debug {
// Debug00236F48,E-0099-W110. Source halfword count, actual derived-state
// refresh on capture/restore, then per-point num_fields scalar bytes.
void QuickSpline::DoSnapshot(std::uint32_t snapshot,int num_fields){auto& m=m_;const auto a=address_;ReplaySnapshot s(m,snapshot);s.BeginSection(0x2b5d78);s.MoveData(a+0x1a,2);if(m.byte(snapshot+0x3c)||m.byte(snapshot+0x3d))SetNumControlPoints(m.half(a+0x1a));if(num_fields<0||num_fields>4)hp2::not_implemented("0x00236F48","source QuickSpline snapshot field width outside reviewed vector storage");if(m.half(a+0x1a)>m.half(a+0x18))hp2::not_implemented("0x00236F48","restored source QuickSpline count exceeds existing owned control buffer");for(unsigned i=0;i<m.half(a+0x1a);++i)s.MoveData(m.word(a+0x20)+16*i,num_fields*4);s.EndSection();}
}
