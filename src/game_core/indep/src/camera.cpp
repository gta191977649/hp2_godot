// TASK-0099 E-0099-W50. Original ../indep/src/camera.cpp ownership confirmed
// by live read-only MCP FDR/SYMR/marker/path bytes; Debug authority, not Retail.
#include "game_core/indep/inc/Camera.hpp"
#include "game_core/port/driving/projections.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/src/ecstasy/emathe.hpp"
#include <cmath>
namespace hp2::debug {
// Debug00156868/listing, E-0099-W50. Finite positive normalization domain;
// exceptional VU reciprocal square root is an explicit original-address gap.
void Camera::SetCameraMatrix(std::uint32_t input,float time){
 if(m_.word(0x327428))return;
 m_.copy(address_+0x90,address_,0x90);m_.scalar(address_+0x1b4,time);
 auto matrix=m_.matrix(input);
 if(m_.word(0x2f20e0)&&!m_.word(0x326f20)){
  m_.word(0x32742c,1);matrix=m_.matrix(0x2f20e4);
  for(unsigned i=12;i<16;++i)matrix[i]*=m_.scalar(0x324f94);
  matrix[15]=1;
 }else if(m_.word(0x32742c))m_.word(0x32742c,0);
 m_.matrix(address_,matrix);
 // Exact eTransposeMatrix0011BEA0 followed by three homogeneous-lane clears.
 std::array<float,16> transpose{};eTransposeMatrix(transpose,matrix);
 transpose[3]=transpose[7]=transpose[11]=0;
 // eMulVector0011B6A8 initializes vf4 from actual002D39D0; XYZ are added,
 // while its W is retained. Preserve ordered multiply/add and all four lanes.
 std::array<float,4> position{};eMulVector(m_,position,transpose,{{matrix[12],matrix[13],matrix[14],matrix[15]}});
 for(unsigned lane=0;lane<4;++lane)position[lane]=(lane==3?1.0f:0.0f)-position[lane];
 m_.vector(address_+0x40,position);
 const float length=(transpose[8]*transpose[8]+transpose[9]*transpose[9])+transpose[10]*transpose[10];
 if(!(length>0)||!std::isfinite(length))hp2::not_implemented("0x00156868","exceptional original VU camera-direction reciprocal square root");
 const float reciprocal=1/std::sqrt(length);std::array<float,4> direction{},target{};
 for(unsigned lane=0;lane<4;++lane){direction[lane]=transpose[8+lane]*reciprocal;target[lane]=direction[lane]*m_.scalar(address_+0x70);target[lane]+=position[lane];}
 m_.vector(address_+0x50,direction);m_.vector(address_+0x60,target);
 if(m_.word(address_+0x1b0)){m_.word(address_+0x1b0,0);m_.copy(address_+0x90,address_,0x90);}
 const float inverse=1/m_.scalar(address_+0x1b4);
 for(unsigned offset:{0x40u,0x50u,0x60u}){auto velocity=m_.vector(address_+offset);const auto previous=m_.vector(address_+0x90+offset);for(unsigned lane=0;lane<4;++lane)velocity[lane]=(velocity[lane]-previous[lane])*inverse;m_.vector(address_+0x120+offset,velocity);}
 for(unsigned offset:{0x70u,0x74u,0x78u,0x7cu,0x80u})m_.scalar(address_+0x120+offset,(m_.scalar(address_+offset)-m_.scalar(address_+0x90+offset))*inverse);
 const auto difference=std::uint16_t(m_.half(address_+0x84)-m_.half(address_+0x114));m_.half(address_+0x1a4,std::uint16_t(::port::driving::cvt_w_s(inverse*float(difference))));
}
// Debug001566E0/listing, E-0099-W50. Preserve authored startup pose and both
// SetCameraMatrix calls; second call establishes zero velocity/PreviousKey.
void Camera::Sub_001566E0(){
 m_.scalar(address_+0x1b4,1);m_.word(address_+0x1b0,0);m_.word(address_+0x1bc,0);m_.word(address_+0x1c0,0);
 m_.scalar(address_+0x74,0);m_.scalar(address_+0x78,0);m_.word(address_+0x1b8,0x80000000);
 m_.scalar(address_+0x70,10);m_.half(address_+0x84,0x36fb);m_.scalar(address_+0x7c,0.5f);m_.scalar(address_+0x80,m_.scalar(0x324f90));
 ::port::driving::Memory::Temporary matrix(m_,64);m_.matrix(matrix.address(),{{1,0,0,0,0,-1,0,0,0,0,-1,100,0,0,1200,1}});
 SetCameraMatrix(matrix.address(),1);SetCameraMatrix(matrix.address(),1);
}
}
