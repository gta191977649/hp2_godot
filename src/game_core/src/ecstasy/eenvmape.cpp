// TASK0052, original src/ecstasy/eenvmape.cpp established by Debug ECOFF E-0042-S001.
#include "eenvmape.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/CarRenderInfo.hpp"
#include <cmath>
#include <stdexcept>
namespace hp2::recovered {
using namespace port::track;
// Retail001179B0 / Debug00117D78. Original normalization followed by sphere ST.
void ComputeReflectionST(EnvironmentVector& vector,std::array<float,2>& st){
    float square=vector[0]*vector[0];square+=vector[1]*vector[1];square+=vector[2]*vector[2];
    if(!(square>0)||!std::isfinite(square))throw std::invalid_argument("Invalid original reflection vector");
    const float inverse=1/std::sqrt(square);for(auto& value:vector)value*=inverse;
    const float z=vector[2]+1;float divisor=std::sqrt(vector[0]*vector[0]+vector[1]*vector[1]+z*z);divisor+=divisor;
    if(divisor==0)throw std::invalid_argument("Original reflection ST singularity");
    st={{vector[0]/divisor+.5f,vector[1]/divisor+.5f}};
}
// Retail00117A38 / Debug00117E00. Original 6x6 face grid and four back patches.
ReflectionTables CreateRefSampleTable(){
    ReflectionTables tables;const float step=.4f,stretch=.16f;
    for(unsigned face=0;face<6;++face)for(unsigned row=0;row<6;++row)for(unsigned col=0;col<6;++col){
        const float r=step*float(row),c=step*float(col);EnvironmentVector v{};
        switch(face){case 0:v={{r-1,c-1,1}};break;case 1:v={{1,c-1,1-r}};break;case 2:v={{1-r,c-1,-1}};break;case 3:v={{-1,c-1,r-1}};break;case 4:v={{r-1,-1,c-1}};break;case 5:v={{r-1,1,1-c}};break;}
        auto raw=v;std::array<float,2> st;ComputeReflectionST(v,st);tables.front[face][row*6+col]={raw[0],raw[1],raw[2],st[0],st[1],r*.5f,c*.5f};
    }
    for(unsigned patch=0;patch<4;++patch)for(unsigned row=0;row<6;++row){
        const ReflectionSample* edge=nullptr;
        if(patch==0)edge=&tables.front[4][row*6];if(patch==1)edge=&tables.front[1][5*6+row];if(patch==2)edge=&tables.front[5][row*6+5];if(patch==3)edge=&tables.front[3][row];
        const float x=edge->rs-.5f,y=edge->rt-.5f;const float inverse=1/std::sqrt(x*x+y*y);const float nx=x*inverse,ny=y*inverse;
        for(unsigned col=0;col<6;++col){const float t=float(col)/5.f,r=float(row)/5.f;auto& sample=tables.back[patch][row*6+col];sample.rs=x+.5f+nx*.5f*t*stretch;sample.rt=y+.5f+ny*.5f*t*stretch;
            if(patch==0){const float a=1-r;sample.fs=a+(.5f-a)*t;sample.ft=t*.5f;}
            if(patch==1){sample.fs=t*.5f;sample.ft=r+(.5f-r)*t;}
            if(patch==2){const float a=1-r;sample.fs=a+(.5f-a)*t;sample.ft=1-t*.5f;}
            if(patch==3){sample.fs=1-t*.5f;sample.ft=r+(.5f-r)*t;}
        }
    }return tables;
}
namespace {
EnvironmentWarp warp(const std::array<ReflectionSample,36>& samples,unsigned face,bool back,unsigned width,unsigned height){
    if(!width||!height||width>4096||height>4096)throw std::invalid_argument("Invalid original environment destination dimensions");
    EnvironmentWarp out;out.source_face=face;out.back=back;
    for(unsigned row=0;row<5;++row){const auto begin=out.vertices.size();for(unsigned col=0;col<6;++col)for(unsigned side=0;side<2;++side){const auto& s=samples[(row+side)*6+col];EnvironmentWarpVertex vertex;const float w=float(width),h=float(height);vertex.destination={{(std::trunc(s.rs*w-w*.5f+2048)-2048+w*.5f)/w,(std::trunc(s.rt*h-h*.5f+2048)-2048+h*.5f)/h}};vertex.source_uv={{s.fs,s.ft}};out.vertices.push_back(vertex);}
        for(unsigned k=2;k<12;++k){auto a=std::uint32_t(begin+k-2),b=std::uint32_t(begin+k-1),c=std::uint32_t(begin+k);if(k&1)std::swap(a,b);out.indices.insert(out.indices.end(),{a,b,c});}}
    return out;
}
EnvironmentMatrix rotation(EnvironmentMatrix matrix){matrix[12]=matrix[13]=matrix[14]=0;matrix[15]=1;matrix[3]=matrix[7]=matrix[11]=0;return matrix;}
EnvironmentVector transform(const EnvironmentMatrix& matrix,const EnvironmentVector& v){EnvironmentVector out{};for(unsigned k=0;k<3;++k){out[k]=matrix[k]*v[0];out[k]+=matrix[4+k]*v[1];out[k]+=matrix[8+k]*v[2];out[k]+=matrix[12+k];}return out;}
}
// Retail00118150 / Debug00118518; packed XY truncation retained in normalized host positions.
EnvironmentWarp CreateEnvMap(const ReflectionTables& tables,unsigned face,unsigned width,unsigned height){return warp(tables.front.at(face),face,false,width,height);}
// Retail00118498 / Debug00118860; all back sectors sample original source face2.
EnvironmentWarp CreateBackEnvMap(const ReflectionTables& tables,unsigned patch,unsigned width,unsigned height){return warp(tables.back.at(patch),2,true,width,height);}
// Retail00118810 / Debug00118BD8, exact front then back submission order.
std::vector<EnvironmentWarp> eCreateEnvMapPrims(const ReflectionTables& tables,const std::array<bool,6>& active,unsigned width,unsigned height){std::vector<EnvironmentWarp> out;for(unsigned i=0;i<6;++i)if(i!=2&&active[i])out.push_back(CreateEnvMap(tables,i,width,height));if(active[2])for(unsigned i=0;i<4;++i)out.push_back(CreateBackEnvMap(tables,i,width,height));return out;}
// Retail001175C8 controls schedule, not the different Debug parity-only table.
EnvironmentCameras eEnvMap::UpdateCameras(const EnvironmentVector& viewer,const EnvironmentVector& position,std::uint32_t counter,int mode,bool in_game){
    const EnvironmentVector zero{{0,0,0}},forward{{1,0,0}},up{{0,0,1}};
    auto basis=rotation(port::eCreateLookAtMatrix(zero,forward,up));auto camera=rotation(port::eCreateLookAtMatrix(viewer,position,up));EnvironmentMatrix transposed{},orientation{};bTransposeMatrix(transposed,basis);eMulMatrix(orientation,camera,transposed);orientation=rotation(orientation);bTransposeMatrix(transposed,orientation);orientation=transposed;
    constexpr std::array<EnvironmentVector,6> directions_fe{{{{-1,0,0}},{{0,1,0}},{{1,0,0}},{{0,-1,0}},{{0,0,1}},{{0,0,-1}}}};
    constexpr std::array<EnvironmentVector,6> directions_ig{{{{0,0,1}},{{0,-1,0}},{{0,0,-1}},{{0,1,0}},{{-1,0,0}},{{1,0,0}}}};
    constexpr std::array<EnvironmentVector,6> up_fe{{{{0,0,1}},{{0,0,1}},{{0,0,1}},{{0,0,1}},{{1,0,0}},{{-1,0,0}}}};
    constexpr std::array<EnvironmentVector,6> up_ig{{{{-1,0,0}},{{-1,0,0}},{{-1,0,0}},{{-1,0,0}},{{0,0,-1}},{{0,0,1}}}};
    constexpr unsigned phases[4][3]={{0,1,5},{0,2,5},{0,3,5},{0,4,5}};const bool single=mode==1||mode==2;EnvironmentCameras out;out.hack_man_matrix=camera;
    for(unsigned i=0;i<6;++i){auto look=transform(orientation,in_game?directions_ig[i]:directions_fe[i]);const auto vertical=transform(orientation,in_game?up_ig[i]:up_fe[i]);for(unsigned k=0;k<3;++k)look[k]+=position[k];out.faces[i].camera_matrix=port::eCreateLookAtMatrix(position,look,vertical);out.faces[i].active=single?(i==phases[counter%4][0]||i==phases[counter%4][1]||i==phases[counter%4][2]):i==counter%6;if(i==2)out.hack_man_matrix=out.faces[i].camera_matrix;}
    return out;
}
}
