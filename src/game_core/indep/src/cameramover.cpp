// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\cameramover.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/inc/CameraMover.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/port/frontend/selection_camera.hpp"
#include <cmath>
#include "game_core/port/race/car_render_resources.hpp"
#include "game_core/indep/src/track.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/indep/src/copmgr.hpp"
#include "game_core/src/ecstasy/emathe.hpp"
#include "game_core/port/driving/ee_scalar.hpp"
#include "game_core/indep/inc/Car.hpp"
#include "game_core/indep/inc/Camera.hpp"
#include "game_core/indep/src/rumble.hpp"

namespace hp2::debug {
// Debug0015C138, E-0099-W50. Exact original manager/pursuit relationships;
// output is the logical bool value, not a host recreation of PS2 bool storage.
std::uint32_t GetLookbackCar(::port::driving::Memory& m,std::uint32_t car,bool& slow){slow=false;const auto manager=m.word(0x328c1c);if(!manager)return 0;tCopManager cops(m,manager);const auto own=cops.GetCopByCar(car);if(!own){const auto chasing=cops.GetCopByChase(car);return chasing?m.word(chasing+4):0;}const auto pursuit=m.word(own);if(!pursuit)return 0;slow=true;return m.word(pursuit+0x10);}
// Debug0015C060/listing, E-0099-W50. Actual CarState.Position+20, four-vector
// normalization and original signed-zero DIV.S projection at0015C0B0.
std::uint16_t CalculateLookbackAngle(::port::driving::Memory& m,std::uint32_t car,std::uint32_t follow,const std::array<float,16>& body){auto delta=m.vector(follow+0x60);const auto position=m.vector(car+0x60);for(unsigned lane=0;lane<4;++lane)delta[lane]-=position[lane];const float distance=std::sqrt((delta[0]*delta[0]+delta[1]*delta[1])+delta[2]*delta[2]);const float inverse=distance==0?::port::driving::ee_div_s_zero(1,distance,"0x0015C0B0"):1/distance;for(auto& lane:delta)lane*=inverse;const float forward=(body[0]*delta[0]+body[1]*delta[1])+body[2]*delta[2],side=(body[4]*delta[0]+body[5]*delta[1])+body[6]*delta[2];if(m.scalar(0x32508c)<forward)return 0;if(forward<m.scalar(0x325090))return 0x8000;return side<0?0x4000:0xc000;}
// Debug001571E8/listing, E-0099-W50. ViewID-1 is the actual unbound branch.
void CameraMover::Sub_001571E8(int view_id,int type){m_.word(address_+0x28,0x2a9cf0);m_.word(address_+8,std::uint32_t(type));m_.word(address_+0xc,std::uint32_t(view_id));m_.word(address_+0x10,0);m_.word(address_+0x1c,0);
 if(view_id==-1){for(unsigned offset:{0x14u,0x18u,0x20u,0x24u})m_.word(address_+offset,0);return;}
 const auto view=::port::race::eGetView(m_,view_id),camera=m_.word(view+0x38);if(!camera)hp2::not_implemented("0x001571E8","original CameraMover requires initialized eView camera");m_.word(address_+0x14,view);const auto dash=m_.word(camera+0x1c0),car=m_.word(camera+0x1bc);m_.scalar(camera+0x80,m_.scalar(0x324f98));m_.word(address_+0x20,car);m_.word(address_+0x24,dash);m_.word(address_+0x18,camera);Enable();
}
// Debug001572D8, E-0099-W50. Preserve original StopUpdating and attachment order.
void CameraMover::Enable(){const auto stopped=m_.word(0x327428);if(!m_.word(address_+0x10)){const auto camera=m_.word(address_+0x18);m_.word(address_+0x10,1);if(!stopped)m_.word(camera+0x1bc,m_.word(address_+0x20));if(!stopped)m_.word(camera+0x1c0,m_.word(address_+0x24));::port::race::eView(m_,m_.word(address_+0x14)).AttachCameraMover(address_);m_.scalar(m_.word(address_+0x18)+0x7c,0.5f);}}
// Debug00157350, E-0099-W50. Does not clear detached node links.
void CameraMover::Disable(){if(m_.word(address_+0x10)){m_.word(address_+0x10,0);const auto camera=m_.word(address_+0x18),dash=m_.word(camera+0x1c0);m_.word(address_+0x20,m_.word(camera+0x1bc));m_.word(address_+0x24,dash);::port::race::eView(m_,m_.word(address_+0x14)).UnattachCameraMover(address_);}}
// Debug00157280 and node dtor0015F498, E-0099-W50.
void CameraMover::Sub_00157280(int flags){m_.word(address_+0x28,0x2a9cf0);Disable();if(flags&1)m_.release(address_);}
// Debug001572C8/001572D0, E-0099-W50. Exact empty base virtual methods.
void CameraMover::Update(){}
void CameraMover::Render(std::uint32_t view){(void)view;}
// Debug00157420, E-0099-W50. Actual shared topology references and original
// three-pass plane-shaft averaging; scratch W is unconsumed, result W is zero.
void CameraMover::EnforceMinGapToWalls(std::uint32_t topology,const std::array<float,4>& car,const std::array<float,4>& camera,std::array<float,4>& adjustment){auto constrained=camera;TopologyCoordinate(m_,topology).ConstrainToTopology(eCONSTRAIN_BY_SCALE,true,car,constrained,m_.scalar(0x324f9c),1);for(unsigned lane=0;lane<4;++lane)adjustment[lane]=constrained[lane]-camera[lane];for(unsigned pass=0;pass<3;++pass){std::array<float,4> point{},average{};for(unsigned lane=0;lane<4;++lane)point[lane]=camera[lane]+adjustment[lane];unsigned count=0;for(int i=0;i<m_.signed_word(topology+0x2a0);++i){const auto polygon=m_.word(topology+0x20+std::uint32_t(i)*4);const auto terrain=m_.signed_byte(polygon+0xb);if(m_.byte(0x3102e8+std::uint32_t(terrain)*0x90+0xd))continue;float elevation;if(TrackPolygon(m_,polygon).PointInShaft(point,&elevation)&&0<=elevation&&elevation<m_.scalar(0x324fa0)){const auto normal=m_.vector(polygon+0x70);const float depth=m_.scalar(0x324fa0)-elevation;for(unsigned lane=0;lane<4;++lane)average[lane]=normal[lane]*depth+average[lane];++count;}}if(!count)break;const float inverse=1/float(count);for(unsigned lane=0;lane<4;++lane)adjustment[lane]=average[lane]*inverse+adjustment[lane];}adjustment[3]=0;}
// Debug00157618, E-0099-W50. Original two look-at matrices, rigid inverse and
// actual vector3 homogeneous source. Temporary W never replaces authored XYZ.
void CameraMover::IsoProjectionMatrix(std::array<float,16>& output,const std::array<float,4>& eye,const std::array<float,4>& look,const std::array<float,2>& projection){const std::array<float,4> up{{0,0,1,0}};std::array<float,16> world_to_camera{},camera_to_world{};eCreateLookAtMatrix(world_to_camera,eye,look,up);eInvertTransformationMatrix(camera_to_world,world_to_camera);std::array<float,4> projected{},adjusted{};eMulVector(m_,projected,world_to_camera,look);projected[1]-=projected[2]*projection[1];projected[0]-=projected[2]*projection[0];eMulVector(m_,adjusted,camera_to_world,projected);eCreateLookAtMatrix(output,eye,adjusted,up);}
// Debug0015BD10, E-0099-W50. Actual shared Car/CameraInfo and original topology
// component. No camera-update surrogate or synthetic POV table is installed.
void DriveCameraMover::Sub_0015BD10(int view_id,std::uint32_t car,int pov_index,float lookback_speed,int disable_lag,int lookback,int perfect_focus){
 Sub_001571E8(view_id,1);m_.half(address_+0x8c,0);for(unsigned offset:{0x7cu,0x80u,0x88u,0x84u,0x94u,0xa0u})m_.word(address_+offset,0);
 const auto yaw=m_.half(car+0x3a4);const float pitch=m_.scalar(car+0x98);m_.word(address_+0x70,car);m_.word(address_+0x28,0x2a9c80);m_.word(address_+0x90,yaw);m_.word(address_+0x9c,hp2::recovered::bASin(m_,pitch));
 TopologyCoordinate(m_,address_+0xb4).Sub_00151878();m_.word(address_+0x374,std::uint32_t(disable_lag));m_.scalar(address_+0x378,lookback_speed);m_.word(address_+0x3e0,std::uint32_t(perfect_focus));m_.scalar(address_+0x7c,lookback?1:0);
 m_.scalar(address_+0xa4,0);m_.scalar(address_+0xa8,0);for(int i=19;i>=0;--i)m_.scalar(address_+0x37c+std::uint32_t(i)*4,0);for(unsigned offset:{0x3ccu,0x3d0u,0x3d4u,0x3d8u})m_.scalar(address_+offset,0);m_.scalar(address_+0x3dc,1);m_.scalar(address_+0xac,m_.scalar(car+0x3c0));
 if(!m_.word(0x327428)){const auto camera=m_.word(address_+0x18);m_.word(camera+0x1bc,0);m_.word(camera+0x1c0,0);}SetPOVIndex(pov_index);
 auto info=m_.word(car+0xc)+0x3c0;m_.word(address_+0x74,info);if(!m_.word(info)||!m_.word(info+4)){const auto original=m_.word(0x3278e0);if(!original)hp2::not_implemented("0x0015BD10","original CameraInfo fallback requires the loaded CarTypeInfo table");m_.word(address_+0x74,original+0x3c0);}m_.scalar(address_+0xb0,1);
}
// Debug0015BEB8, E-0099-W50. Original camera resets then topology/base cleanup.
void DriveCameraMover::Sub_0015BEB8(int flags){m_.word(address_+0x28,0x2a9c80);if(!m_.word(0x327428)){const auto camera=m_.word(address_+0x18);m_.word(camera+0x1bc,0);m_.scalar(camera+0x74,0);m_.scalar(camera+0x78,0);}TopologyCoordinate(m_,address_+0xb4).Sub_001518C0(2);Sub_00157280(flags);}
// Debug0015BFD8/0015BFE0, E-0099-W50. Preserve original signed index comparison
// (no invented lower bound); only TWOH selects the authored split-screen table.
void DriveCameraMover::SetPOVIndex(int index){m_.word(address_+0x78,std::uint32_t(index));}
// Debug0015F678 / 0015C218; E-0112-C01. Actual Player LookbackMode and live interpolation.
void DriveCameraMover::SetLookBack(int lookback){m_.scalar(address_+0x7c,lookback?1.f:0.f);}
bool DriveCameraMover::OutsidePOV(){const auto table=m_.word(address_+0x28);if(m_.word(table+0x3c)!=0x15bfe0)hp2::not_implemented("0x0015C218","derived camera GetPOV virtual target is not reconstructed");const auto pov=DriveCameraMover(m_,address_+std::int32_t(m_.signed_half(table+0x38))).GetPOV(m_.signed_word(address_+0x78)),player=m_.word(address_+0x1c);const bool mode=player&&m_.word(player+0x28)==0;return !pov||m_.word(pov)==2||(mode&&m_.scalar(address_+0x80)>0);}
// Debug0015BF28, E-0099-W50. Reset actual follow state/history only; original
// POV, camera info, lag settings and topology ownership remain untouched.
void DriveCameraMover::ResetState(){m_.half(address_+0x8c,0);for(unsigned offset:{0x7cu,0x80u,0x88u,0x84u,0x94u,0xa0u})m_.word(address_+offset,0);const auto car=m_.word(address_+0x70);const auto yaw=m_.half(car+0x3a4);const float pitch=m_.scalar(car+0x98);m_.word(address_+0x90,yaw);m_.word(address_+0x9c,hp2::recovered::bASin(m_,pitch));m_.scalar(address_+0xa4,0);m_.scalar(address_+0xa8,0);for(unsigned offset:{0x3ccu,0x3d4u,0x3d0u})m_.scalar(address_+offset,0);for(int i=19;i>=0;--i)m_.scalar(address_+0x37c+std::uint32_t(i)*4,0);m_.scalar(address_+0x3d8,0);m_.scalar(address_+0x3dc,1);m_.scalar(address_+0xb0,1);m_.scalar(address_+0xac,m_.scalar(car+0x3c0));}
// Debug0015BC10, E-0099-W50. Original signed-short magnitude, table atan and
// source coefficient/sign. The half-turn boundary follows its sign bit.
float DriveCameraMover::GetCorrectionYaw(::port::driving::Memory& m,std::uint16_t angle){const int signed_angle=std::int16_t(angle);const auto magnitude=signed_angle<0?-signed_angle:signed_angle;const auto correction=std::uint16_t(hp2::recovered::bATan(m,1,float(magnitude)*m.scalar(0x325084)));auto result=float(correction)*m.scalar(0x325088);if(signed_angle<0)result=-result;return result;}
// Debug0015BC88, E-0099-W50. Strict >8000 wrapping and original near-zero
// interval [-7,7]; larger negative quotients round toward zero by adding7.
int DriveCameraMover::GetCorrectionPitch(std::uint16_t angle){int value=angle;if(0x8000<value)value-=0x10000;if(std::uint32_t(value+7)>14){const int inverse=-value;return (inverse<0?inverse+7:inverse)>>3;}return -value;}
std::uint32_t DriveCameraMover::GetPOV(int index){const auto mode=::port::race::eGetCurrentViewMode(m_);const auto info=m_.word(address_+0x74);if(mode==3){if(m_.signed_word(info+4)<=index)return 0;return info+0x90+std::uint32_t(index)*0x20;}if(m_.signed_word(info)<=index)return 0;return info+0x10+std::uint32_t(index)*0x20;}
}

namespace hp2::debug {
// Debug0015A568, E-0098-C01; exact cameramover.cpp source E-0042-S001.
void MyQuaternionMultiply(bQuaternion& out,const bQuaternion& first,const bQuaternion& second){
    // Original preloads every component, so destination can alias either source.
    const auto a=first,b=second;
    out.w=((a.w*b.w-a.x*b.x)-a.y*b.y)-a.z*b.z;
    out.x=((a.y*b.z-a.z*b.y)+a.w*b.x)+a.x*b.w;
    out.y=((a.z*b.x-a.x*b.z)+a.w*b.y)+a.y*b.w;
    out.z=((a.x*b.y-a.y*b.x)+a.w*b.z)+a.z*b.w;
}
// Debug0015A610, E-0098-C01; exact cameramover.cpp source E-0042-S001.
void NormalizeQuaternion(bQuaternion& q){
    const float norm=std::sqrt(((q.x*q.x+q.y*q.y)+q.z*q.z)+q.w*q.w);
    q.w=q.w/norm;q.x=q.x/norm;q.y=q.y/norm;q.z=q.z/norm;
}
}

namespace hp2::recovered {
// ELF 0x001577D0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001577d0.md.
void BumperCameraMover::Sub_001577D0() {
    hp2::not_implemented("0x001577D0", "BumperCameraMover::Sub_001577D0 (signature and full body unresolved)");
}

// ELF 0x001577D8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001577d8.md.
void BumperCameraMover::Sub_001577D8() {
    hp2::not_implemented("0x001577D8", "BumperCameraMover::Sub_001577D8 (signature and full body unresolved)");
}

// ELF 0x00157850; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00157850.md.
void BumperCameraMover::Sub_00157850() {
    hp2::not_implemented("0x00157850", "BumperCameraMover::Sub_00157850 (signature and full body unresolved)");
}

// ELF 0x00157920; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00157920.md.
void BumperCameraMover::Sub_00157920() {
    hp2::not_implemented("0x00157920", "BumperCameraMover::Sub_00157920 (signature and full body unresolved)");
}

// ELF 0x00158800; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00158800.md.
void BumperCameraMover::Sub_00158800() {
    hp2::not_implemented("0x00158800", "BumperCameraMover::Sub_00158800 (signature and full body unresolved)");
}

// ELF 0x00158910; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00158910.md.
void BumperCameraMover::Sub_00158910() {
    hp2::not_implemented("0x00158910", "BumperCameraMover::Sub_00158910 (signature and full body unresolved)");
}

// ELF 0x001589A8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001589a8.md.
void BumperCameraMover::Sub_001589A8() {
    hp2::not_implemented("0x001589A8", "BumperCameraMover::Sub_001589A8 (signature and full body unresolved)");
}

// ELF 0x00158A68; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00158a68.md.
void BumperCameraMover::Sub_00158A68() {
    hp2::not_implemented("0x00158A68", "BumperCameraMover::Sub_00158A68 (signature and full body unresolved)");
}

// ELF 0x0015FB10; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fb10.md.
void BumperCameraMover::Sub_0015FB10() {
    hp2::not_implemented("0x0015FB10", "BumperCameraMover::Sub_0015FB10 (signature and full body unresolved)");
}

// ELF 0x0015FD10; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd10.md.
void BumperCameraMover::Sub_0015FD10() {
    hp2::not_implemented("0x0015FD10", "BumperCameraMover::Sub_0015FD10 (signature and full body unresolved)");
}

// ELF 0x0015FD18; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd18.md.
void BumperCameraMover::Sub_0015FD18() {
    hp2::not_implemented("0x0015FD18", "BumperCameraMover::Sub_0015FD18 (signature and full body unresolved)");
}

// ELF 0x0015FD20; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd20.md.
void BumperCameraMover::Sub_0015FD20() {
    hp2::not_implemented("0x0015FD20", "BumperCameraMover::Sub_0015FD20 (signature and full body unresolved)");
}

// ELF 0x0015FD28; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd28.md.
void BumperCameraMover::Sub_0015FD28() {
    hp2::not_implemented("0x0015FD28", "BumperCameraMover::Sub_0015FD28 (signature and full body unresolved)");
}

// ELF 0x0015FD30; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd30.md.
void BumperCameraMover::Sub_0015FD30() {
    hp2::not_implemented("0x0015FD30", "BumperCameraMover::Sub_0015FD30 (signature and full body unresolved)");
}

// ELF 0x0015FD38; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd38.md.
void BumperCameraMover::Sub_0015FD38() {
    hp2::not_implemented("0x0015FD38", "BumperCameraMover::Sub_0015FD38 (signature and full body unresolved)");
}

// ELF 0x0015FD40; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd40.md.
void BumperCameraMover::Sub_0015FD40() {
    hp2::not_implemented("0x0015FD40", "BumperCameraMover::Sub_0015FD40 (signature and full body unresolved)");
}
} // namespace hp2::recovered

// Original translation unit: `..\indep\src\cameramover.cpp` (ELF 0x002B1668; E-0029-P02).
// Directory binding through 0x0015E7B0; see reverse/classes/source-path-bindings.json.
// This per-class file is a port-side partition, not the original translation unit or header.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: confirmed original directory (E-0029-P02); per-class basename/header extension port-side.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/inc/CameraMover.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001577D0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001577d0.md.
void CubicCameraMover::Sub_001577D0() {
    hp2::not_implemented("0x001577D0", "CubicCameraMover::Sub_001577D0 (signature and full body unresolved)");
}

// ELF 0x001577D8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001577d8.md.
void CubicCameraMover::Sub_001577D8() {
    hp2::not_implemented("0x001577D8", "CubicCameraMover::Sub_001577D8 (signature and full body unresolved)");
}

// ELF 0x00157850; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00157850.md.
void CubicCameraMover::Sub_00157850() {
    hp2::not_implemented("0x00157850", "CubicCameraMover::Sub_00157850 (signature and full body unresolved)");
}

// ELF 0x00157920; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00157920.md.
void CubicCameraMover::Sub_00157920() {
    hp2::not_implemented("0x00157920", "CubicCameraMover::Sub_00157920 (signature and full body unresolved)");
}

// ELF 0x0015E7B0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015e7b0.md.
void CubicCameraMover::Sub_0015E7B0() {
    hp2::not_implemented("0x0015E7B0", "CubicCameraMover::Sub_0015E7B0 (signature and full body unresolved)");
}

// ELF 0x0015EA30; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015ea30.md.
void CubicCameraMover::Sub_0015EA30() {
    hp2::not_implemented("0x0015EA30", "CubicCameraMover::Sub_0015EA30 (signature and full body unresolved)");
}

// ELF 0x0015ECE0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015ece0.md.
void CubicCameraMover::Sub_0015ECE0() {
    hp2::not_implemented("0x0015ECE0", "CubicCameraMover::Sub_0015ECE0 (signature and full body unresolved)");
}

// ELF 0x0015FA78; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fa78.md.
void CubicCameraMover::Sub_0015FA78() {
    hp2::not_implemented("0x0015FA78", "CubicCameraMover::Sub_0015FA78 (signature and full body unresolved)");
}

// ELF 0x0015FCB0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fcb0.md.
void CubicCameraMover::Sub_0015FCB0() {
    hp2::not_implemented("0x0015FCB0", "CubicCameraMover::Sub_0015FCB0 (signature and full body unresolved)");
}

// ELF 0x0015FD10; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd10.md.
void CubicCameraMover::Sub_0015FD10() {
    hp2::not_implemented("0x0015FD10", "CubicCameraMover::Sub_0015FD10 (signature and full body unresolved)");
}

// ELF 0x0015FD18; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd18.md.
void CubicCameraMover::Sub_0015FD18() {
    hp2::not_implemented("0x0015FD18", "CubicCameraMover::Sub_0015FD18 (signature and full body unresolved)");
}

// ELF 0x0015FD20; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd20.md.
void CubicCameraMover::Sub_0015FD20() {
    hp2::not_implemented("0x0015FD20", "CubicCameraMover::Sub_0015FD20 (signature and full body unresolved)");
}

// ELF 0x0015FD28; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd28.md.
void CubicCameraMover::Sub_0015FD28() {
    hp2::not_implemented("0x0015FD28", "CubicCameraMover::Sub_0015FD28 (signature and full body unresolved)");
}

// ELF 0x0015FD30; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd30.md.
void CubicCameraMover::Sub_0015FD30() {
    hp2::not_implemented("0x0015FD30", "CubicCameraMover::Sub_0015FD30 (signature and full body unresolved)");
}

// ELF 0x0015FD38; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd38.md.
void CubicCameraMover::Sub_0015FD38() {
    hp2::not_implemented("0x0015FD38", "CubicCameraMover::Sub_0015FD38 (signature and full body unresolved)");
}

// ELF 0x0015FD40; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd40.md.
void CubicCameraMover::Sub_0015FD40() {
    hp2::not_implemented("0x0015FD40", "CubicCameraMover::Sub_0015FD40 (signature and full body unresolved)");
}
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/DebugWorldCameraMover.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001577D0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001577d0.md.
void DebugWorldCameraMover::Sub_001577D0() {
    hp2::not_implemented("0x001577D0", "DebugWorldCameraMover::Sub_001577D0 (signature and full body unresolved)");
}

// ELF 0x001577D8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001577d8.md.
void DebugWorldCameraMover::Sub_001577D8() {
    hp2::not_implemented("0x001577D8", "DebugWorldCameraMover::Sub_001577D8 (signature and full body unresolved)");
}

// ELF 0x00157850; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00157850.md.
void DebugWorldCameraMover::Sub_00157850() {
    hp2::not_implemented("0x00157850", "DebugWorldCameraMover::Sub_00157850 (signature and full body unresolved)");
}

// ELF 0x00157920; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00157920.md.
void DebugWorldCameraMover::Sub_00157920() {
    hp2::not_implemented("0x00157920", "DebugWorldCameraMover::Sub_00157920 (signature and full body unresolved)");
}

// ELF 0x0015B268; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015b268.md.
void DebugWorldCameraMover::Sub_0015B268() {
    hp2::not_implemented("0x0015B268", "DebugWorldCameraMover::Sub_0015B268 (signature and full body unresolved)");
}

// ELF 0x0015B318; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015b318.md.
void DebugWorldCameraMover::Sub_0015B318() {
    hp2::not_implemented("0x0015B318", "DebugWorldCameraMover::Sub_0015B318 (signature and full body unresolved)");
}

// ELF 0x0015B360; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015b360.md.
void DebugWorldCameraMover::Sub_0015B360() {
    hp2::not_implemented("0x0015B360", "DebugWorldCameraMover::Sub_0015B360 (signature and full body unresolved)");
}

// ELF 0x0015B4D0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015b4d0.md.
void DebugWorldCameraMover::Sub_0015B4D0() {
    hp2::not_implemented("0x0015B4D0", "DebugWorldCameraMover::Sub_0015B4D0 (signature and full body unresolved)");
}

// ELF 0x0015B878; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015b878.md.
void DebugWorldCameraMover::Sub_0015B878() {
    hp2::not_implemented("0x0015B878", "DebugWorldCameraMover::Sub_0015B878 (signature and full body unresolved)");
}

// ELF 0x0015FCB0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fcb0.md.
void DebugWorldCameraMover::Sub_0015FCB0() {
    hp2::not_implemented("0x0015FCB0", "DebugWorldCameraMover::Sub_0015FCB0 (signature and full body unresolved)");
}

// ELF 0x0015FCF8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fcf8.md.
void DebugWorldCameraMover::Sub_0015FCF8() {
    hp2::not_implemented("0x0015FCF8", "DebugWorldCameraMover::Sub_0015FCF8 (signature and full body unresolved)");
}

// ELF 0x0015FD10; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd10.md.
void DebugWorldCameraMover::Sub_0015FD10() {
    hp2::not_implemented("0x0015FD10", "DebugWorldCameraMover::Sub_0015FD10 (signature and full body unresolved)");
}

// ELF 0x0015FD18; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd18.md.
void DebugWorldCameraMover::Sub_0015FD18() {
    hp2::not_implemented("0x0015FD18", "DebugWorldCameraMover::Sub_0015FD18 (signature and full body unresolved)");
}

// ELF 0x0015FD20; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd20.md.
void DebugWorldCameraMover::Sub_0015FD20() {
    hp2::not_implemented("0x0015FD20", "DebugWorldCameraMover::Sub_0015FD20 (signature and full body unresolved)");
}

// ELF 0x0015FD28; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd28.md.
void DebugWorldCameraMover::Sub_0015FD28() {
    hp2::not_implemented("0x0015FD28", "DebugWorldCameraMover::Sub_0015FD28 (signature and full body unresolved)");
}

// ELF 0x0015FD30; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd30.md.
void DebugWorldCameraMover::Sub_0015FD30() {
    hp2::not_implemented("0x0015FD30", "DebugWorldCameraMover::Sub_0015FD30 (signature and full body unresolved)");
}
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/inc/CameraMover.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001577D0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001577d0.md.
void DriveCameraMover::Sub_001577D0() {
    hp2::not_implemented("0x001577D0", "DriveCameraMover::Sub_001577D0 (signature and full body unresolved)");
}

// ELF 0x001577D8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001577d8.md.
void DriveCameraMover::Sub_001577D8() {
    hp2::not_implemented("0x001577D8", "DriveCameraMover::Sub_001577D8 (signature and full body unresolved)");
}

// ELF 0x00157850; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00157850.md.
void DriveCameraMover::Sub_00157850() {
    hp2::not_implemented("0x00157850", "DriveCameraMover::Sub_00157850 (signature and full body unresolved)");
}

// ELF 0x0015C138; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015c138.md.
void DriveCameraMover::Sub_0015C138() {
    hp2::not_implemented("0x0015C138", "DriveCameraMover::Sub_0015C138 (signature and full body unresolved)");
}

// ELF 0x0015C258; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015c258.md.
void DriveCameraMover::Sub_0015C258() {
    hp2::not_implemented("0x0015C258", "DriveCameraMover::Sub_0015C258 (signature and full body unresolved)");
}

// ELF 0x0015C400; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015c400.md.
void DriveCameraMover::Sub_0015C400() {
    hp2::not_implemented("0x0015C400", "DriveCameraMover::Sub_0015C400 (signature and full body unresolved)");
}

// ELF 0x0015C470; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015c470.md.
void DriveCameraMover::Sub_0015C470() {
    hp2::not_implemented("0x0015C470", "DriveCameraMover::Sub_0015C470 (signature and full body unresolved)");
}

// ELF 0x0015C528; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015c528.md.
void DriveCameraMover::Sub_0015C528() {
    hp2::not_implemented("0x0015C528", "DriveCameraMover::Sub_0015C528 (signature and full body unresolved)");
}

// ELF 0x0015C760; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015c760.md.
void DriveCameraMover::Sub_0015C760() {
    hp2::not_implemented("0x0015C760", "DriveCameraMover::Sub_0015C760 (signature and full body unresolved)");
}

// ELF 0x0015C7A0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015c7a0.md.
void DriveCameraMover::Sub_0015C7A0() {
    hp2::not_implemented("0x0015C7A0", "DriveCameraMover::Sub_0015C7A0 (signature and full body unresolved)");
}

// ELF 0x0015C838; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015c838.md.
void DriveCameraMover::Sub_0015C838() {
    hp2::not_implemented("0x0015C838", "DriveCameraMover::Sub_0015C838 (signature and full body unresolved)");
}

// ELF 0x0015C880; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015c880.md.
void DriveCameraMover::Sub_0015C880() {
    hp2::not_implemented("0x0015C880", "DriveCameraMover::Sub_0015C880 (signature and full body unresolved)");
}

// ELF 0x0015FBF0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fbf0.md.
void DriveCameraMover::Sub_0015FBF0() {
    hp2::not_implemented("0x0015FBF0", "DriveCameraMover::Sub_0015FBF0 (signature and full body unresolved)");
}

// ELF 0x0015FC20; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fc20.md.
void DriveCameraMover::Sub_0015FC20() {
    hp2::not_implemented("0x0015FC20", "DriveCameraMover::Sub_0015FC20 (signature and full body unresolved)");
}

// ELF 0x0015FD38; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd38.md.
void DriveCameraMover::Sub_0015FD38() {
    hp2::not_implemented("0x0015FD38", "DriveCameraMover::Sub_0015FD38 (signature and full body unresolved)");
}

// ELF 0x0015FD40; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd40.md.
void DriveCameraMover::Sub_0015FD40() {
    hp2::not_implemented("0x0015FD40", "DriveCameraMover::Sub_0015FD40 (signature and full body unresolved)");
}
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/inc/CameraMover.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001577D0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001577d0.md.
void MaxCameraMover::Sub_001577D0() {
    hp2::not_implemented("0x001577D0", "MaxCameraMover::Sub_001577D0 (signature and full body unresolved)");
}

// ELF 0x001577D8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001577d8.md.
void MaxCameraMover::Sub_001577D8() {
    hp2::not_implemented("0x001577D8", "MaxCameraMover::Sub_001577D8 (signature and full body unresolved)");
}

// ELF 0x00157850; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00157850.md.
void MaxCameraMover::Sub_00157850() {
    hp2::not_implemented("0x00157850", "MaxCameraMover::Sub_00157850 (signature and full body unresolved)");
}

// ELF 0x00157920; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00157920.md.
void MaxCameraMover::Sub_00157920() {
    hp2::not_implemented("0x00157920", "MaxCameraMover::Sub_00157920 (signature and full body unresolved)");
}

// ELF 0x0015A340; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015a340.md.
void MaxCameraMover::Sub_0015A340() {
    hp2::not_implemented("0x0015A340", "MaxCameraMover::Sub_0015A340 (signature and full body unresolved)");
}

// ELF 0x0015A3F8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015a3f8.md.
void MaxCameraMover::Sub_0015A3F8() {
    hp2::not_implemented("0x0015A3F8", "MaxCameraMover::Sub_0015A3F8 (signature and full body unresolved)");
}

// ELF 0x0015A410; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015a410.md.
void MaxCameraMover::Sub_0015A410() {
    hp2::not_implemented("0x0015A410", "MaxCameraMover::Sub_0015A410 (signature and full body unresolved)");
}

// ELF 0x0015FAF0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015faf0.md.
void MaxCameraMover::Sub_0015FAF0() {
    hp2::not_implemented("0x0015FAF0", "MaxCameraMover::Sub_0015FAF0 (signature and full body unresolved)");
}

// ELF 0x0015FCB0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fcb0.md.
void MaxCameraMover::Sub_0015FCB0() {
    hp2::not_implemented("0x0015FCB0", "MaxCameraMover::Sub_0015FCB0 (signature and full body unresolved)");
}

// ELF 0x0015FD10; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd10.md.
void MaxCameraMover::Sub_0015FD10() {
    hp2::not_implemented("0x0015FD10", "MaxCameraMover::Sub_0015FD10 (signature and full body unresolved)");
}

// ELF 0x0015FD18; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd18.md.
void MaxCameraMover::Sub_0015FD18() {
    hp2::not_implemented("0x0015FD18", "MaxCameraMover::Sub_0015FD18 (signature and full body unresolved)");
}

// ELF 0x0015FD20; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd20.md.
void MaxCameraMover::Sub_0015FD20() {
    hp2::not_implemented("0x0015FD20", "MaxCameraMover::Sub_0015FD20 (signature and full body unresolved)");
}

// ELF 0x0015FD28; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd28.md.
void MaxCameraMover::Sub_0015FD28() {
    hp2::not_implemented("0x0015FD28", "MaxCameraMover::Sub_0015FD28 (signature and full body unresolved)");
}

// ELF 0x0015FD30; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd30.md.
void MaxCameraMover::Sub_0015FD30() {
    hp2::not_implemented("0x0015FD30", "MaxCameraMover::Sub_0015FD30 (signature and full body unresolved)");
}

// ELF 0x0015FD38; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd38.md.
void MaxCameraMover::Sub_0015FD38() {
    hp2::not_implemented("0x0015FD38", "MaxCameraMover::Sub_0015FD38 (signature and full body unresolved)");
}

// ELF 0x0015FD40; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd40.md.
void MaxCameraMover::Sub_0015FD40() {
    hp2::not_implemented("0x0015FD40", "MaxCameraMover::Sub_0015FD40 (signature and full body unresolved)");
}
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/inc/CameraMover.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001577D0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001577d0.md.
void OrbitCarCameraMover::Sub_001577D0() {
    hp2::not_implemented("0x001577D0", "OrbitCarCameraMover::Sub_001577D0 (signature and full body unresolved)");
}

// ELF 0x001577D8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001577d8.md.
void OrbitCarCameraMover::Sub_001577D8() {
    hp2::not_implemented("0x001577D8", "OrbitCarCameraMover::Sub_001577D8 (signature and full body unresolved)");
}

// ELF 0x00157850; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00157850.md.
void OrbitCarCameraMover::Sub_00157850() {
    hp2::not_implemented("0x00157850", "OrbitCarCameraMover::Sub_00157850 (signature and full body unresolved)");
}

// ELF 0x00157920; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00157920.md.
void OrbitCarCameraMover::Sub_00157920() {
    hp2::not_implemented("0x00157920", "OrbitCarCameraMover::Sub_00157920 (signature and full body unresolved)");
}

// ELF 0x00157E48; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00157e48.md.
void OrbitCarCameraMover::Sub_00157E48() {
    hp2::not_implemented("0x00157E48", "OrbitCarCameraMover::Sub_00157E48 (signature and full body unresolved)");
}

// ELF 0x00158080; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00158080.md.
void OrbitCarCameraMover::Sub_00158080() {
    hp2::not_implemented("0x00158080", "OrbitCarCameraMover::Sub_00158080 (signature and full body unresolved)");
}

// ELF 0x001580E0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001580e0.md.
void OrbitCarCameraMover::Sub_001580E0() {
    hp2::not_implemented("0x001580E0", "OrbitCarCameraMover::Sub_001580E0 (signature and full body unresolved)");
}

// ELF 0x00158100; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00158100.md.
void OrbitCarCameraMover::Sub_00158100() {
    hp2::not_implemented("0x00158100", "OrbitCarCameraMover::Sub_00158100 (signature and full body unresolved)");
}

// ELF 0x001581F8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001581f8.md.
void OrbitCarCameraMover::Sub_001581F8() {
    hp2::not_implemented("0x001581F8", "OrbitCarCameraMover::Sub_001581F8 (signature and full body unresolved)");
}

// ELF 0x00158298; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00158298.md.
void OrbitCarCameraMover::Sub_00158298() {
    hp2::not_implemented("0x00158298", "OrbitCarCameraMover::Sub_00158298 (signature and full body unresolved)");
}

// ELF 0x0015FB18; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fb18.md.
void OrbitCarCameraMover::Sub_0015FB18() {
    hp2::not_implemented("0x0015FB18", "OrbitCarCameraMover::Sub_0015FB18 (signature and full body unresolved)");
}

// ELF 0x0015FD10; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd10.md.
void OrbitCarCameraMover::Sub_0015FD10() {
    hp2::not_implemented("0x0015FD10", "OrbitCarCameraMover::Sub_0015FD10 (signature and full body unresolved)");
}

// ELF 0x0015FD18; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd18.md.
void OrbitCarCameraMover::Sub_0015FD18() {
    hp2::not_implemented("0x0015FD18", "OrbitCarCameraMover::Sub_0015FD18 (signature and full body unresolved)");
}

// ELF 0x0015FD20; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd20.md.
void OrbitCarCameraMover::Sub_0015FD20() {
    hp2::not_implemented("0x0015FD20", "OrbitCarCameraMover::Sub_0015FD20 (signature and full body unresolved)");
}

// ELF 0x0015FD28; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd28.md.
void OrbitCarCameraMover::Sub_0015FD28() {
    hp2::not_implemented("0x0015FD28", "OrbitCarCameraMover::Sub_0015FD28 (signature and full body unresolved)");
}

// ELF 0x0015FD30; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd30.md.
void OrbitCarCameraMover::Sub_0015FD30() {
    hp2::not_implemented("0x0015FD30", "OrbitCarCameraMover::Sub_0015FD30 (signature and full body unresolved)");
}
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/inc/CameraMover.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001577D0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001577d0.md.
void RearViewMirrorCameraMover::Sub_001577D0() {
    hp2::not_implemented("0x001577D0", "RearViewMirrorCameraMover::Sub_001577D0 (signature and full body unresolved)");
}

// ELF 0x001577D8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001577d8.md.
void RearViewMirrorCameraMover::Sub_001577D8() {
    hp2::not_implemented("0x001577D8", "RearViewMirrorCameraMover::Sub_001577D8 (signature and full body unresolved)");
}

// ELF 0x00157850; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00157850.md.
void RearViewMirrorCameraMover::Sub_00157850() {
    hp2::not_implemented("0x00157850", "RearViewMirrorCameraMover::Sub_00157850 (signature and full body unresolved)");
}

// ELF 0x00157920; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00157920.md.
void RearViewMirrorCameraMover::Sub_00157920() {
    hp2::not_implemented("0x00157920", "RearViewMirrorCameraMover::Sub_00157920 (signature and full body unresolved)");
}

// ELF 0x00158F78; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00158f78.md.
void RearViewMirrorCameraMover::Sub_00158F78() {
    hp2::not_implemented("0x00158F78", "RearViewMirrorCameraMover::Sub_00158F78 (signature and full body unresolved)");
}

// ELF 0x00158FE0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00158fe0.md.
void RearViewMirrorCameraMover::Sub_00158FE0() {
    hp2::not_implemented("0x00158FE0", "RearViewMirrorCameraMover::Sub_00158FE0 (signature and full body unresolved)");
}

// ELF 0x00158FF8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00158ff8.md.
void RearViewMirrorCameraMover::Sub_00158FF8() {
    hp2::not_implemented("0x00158FF8", "RearViewMirrorCameraMover::Sub_00158FF8 (signature and full body unresolved)");
}

// ELF 0x0015FB08; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fb08.md.
void RearViewMirrorCameraMover::Sub_0015FB08() {
    hp2::not_implemented("0x0015FB08", "RearViewMirrorCameraMover::Sub_0015FB08 (signature and full body unresolved)");
}

// ELF 0x0015FCB0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fcb0.md.
void RearViewMirrorCameraMover::Sub_0015FCB0() {
    hp2::not_implemented("0x0015FCB0", "RearViewMirrorCameraMover::Sub_0015FCB0 (signature and full body unresolved)");
}

// ELF 0x0015FD10; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd10.md.
void RearViewMirrorCameraMover::Sub_0015FD10() {
    hp2::not_implemented("0x0015FD10", "RearViewMirrorCameraMover::Sub_0015FD10 (signature and full body unresolved)");
}

// ELF 0x0015FD18; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd18.md.
void RearViewMirrorCameraMover::Sub_0015FD18() {
    hp2::not_implemented("0x0015FD18", "RearViewMirrorCameraMover::Sub_0015FD18 (signature and full body unresolved)");
}

// ELF 0x0015FD20; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd20.md.
void RearViewMirrorCameraMover::Sub_0015FD20() {
    hp2::not_implemented("0x0015FD20", "RearViewMirrorCameraMover::Sub_0015FD20 (signature and full body unresolved)");
}

// ELF 0x0015FD28; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd28.md.
void RearViewMirrorCameraMover::Sub_0015FD28() {
    hp2::not_implemented("0x0015FD28", "RearViewMirrorCameraMover::Sub_0015FD28 (signature and full body unresolved)");
}

// ELF 0x0015FD30; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd30.md.
void RearViewMirrorCameraMover::Sub_0015FD30() {
    hp2::not_implemented("0x0015FD30", "RearViewMirrorCameraMover::Sub_0015FD30 (signature and full body unresolved)");
}

// ELF 0x0015FD38; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd38.md.
void RearViewMirrorCameraMover::Sub_0015FD38() {
    hp2::not_implemented("0x0015FD38", "RearViewMirrorCameraMover::Sub_0015FD38 (signature and full body unresolved)");
}

// ELF 0x0015FD40; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd40.md.
void RearViewMirrorCameraMover::Sub_0015FD40() {
    hp2::not_implemented("0x0015FD40", "RearViewMirrorCameraMover::Sub_0015FD40 (signature and full body unresolved)");
}
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/inc/CameraMover.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001577D0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001577d0.md.
void StillCameraMover::Sub_001577D0() {
    hp2::not_implemented("0x001577D0", "StillCameraMover::Sub_001577D0 (signature and full body unresolved)");
}

// ELF 0x001577D8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001577d8.md.
void StillCameraMover::Sub_001577D8() {
    hp2::not_implemented("0x001577D8", "StillCameraMover::Sub_001577D8 (signature and full body unresolved)");
}

// ELF 0x00157850; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00157850.md.
void StillCameraMover::Sub_00157850() {
    hp2::not_implemented("0x00157850", "StillCameraMover::Sub_00157850 (signature and full body unresolved)");
}

// ELF 0x00157920; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00157920.md.
void StillCameraMover::Sub_00157920() {
    hp2::not_implemented("0x00157920", "StillCameraMover::Sub_00157920 (signature and full body unresolved)");
}

// ELF 0x0015E6A0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/0015e6a0.md.
void StillCameraMover::Sub_0015E6A0() {
    hp2::not_implemented("0x0015E6A0", "StillCameraMover::Sub_0015E6A0 (signature and full body unresolved)");
}

// ELF 0x0015E718; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015e718.md.
void StillCameraMover::Sub_0015E718() {
    hp2::not_implemented("0x0015E718", "StillCameraMover::Sub_0015E718 (signature and full body unresolved)");
}

// ELF 0x0015E730; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015e730.md.
void StillCameraMover::Sub_0015E730() {
    hp2::not_implemented("0x0015E730", "StillCameraMover::Sub_0015E730 (signature and full body unresolved)");
}

// ELF 0x0015FCB0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fcb0.md.
void StillCameraMover::Sub_0015FCB0() {
    hp2::not_implemented("0x0015FCB0", "StillCameraMover::Sub_0015FCB0 (signature and full body unresolved)");
}

// ELF 0x0015FCF8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fcf8.md.
void StillCameraMover::Sub_0015FCF8() {
    hp2::not_implemented("0x0015FCF8", "StillCameraMover::Sub_0015FCF8 (signature and full body unresolved)");
}

// ELF 0x0015FD10; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd10.md.
void StillCameraMover::Sub_0015FD10() {
    hp2::not_implemented("0x0015FD10", "StillCameraMover::Sub_0015FD10 (signature and full body unresolved)");
}

// ELF 0x0015FD18; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd18.md.
void StillCameraMover::Sub_0015FD18() {
    hp2::not_implemented("0x0015FD18", "StillCameraMover::Sub_0015FD18 (signature and full body unresolved)");
}

// ELF 0x0015FD20; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd20.md.
void StillCameraMover::Sub_0015FD20() {
    hp2::not_implemented("0x0015FD20", "StillCameraMover::Sub_0015FD20 (signature and full body unresolved)");
}

// ELF 0x0015FD28; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd28.md.
void StillCameraMover::Sub_0015FD28() {
    hp2::not_implemented("0x0015FD28", "StillCameraMover::Sub_0015FD28 (signature and full body unresolved)");
}

// ELF 0x0015FD30; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd30.md.
void StillCameraMover::Sub_0015FD30() {
    hp2::not_implemented("0x0015FD30", "StillCameraMover::Sub_0015FD30 (signature and full body unresolved)");
}

// ELF 0x0015FD38; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd38.md.
void StillCameraMover::Sub_0015FD38() {
    hp2::not_implemented("0x0015FD38", "StillCameraMover::Sub_0015FD38 (signature and full body unresolved)");
}

// ELF 0x0015FD40; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd40.md.
void StillCameraMover::Sub_0015FD40() {
    hp2::not_implemented("0x0015FD40", "StillCameraMover::Sub_0015FD40 (signature and full body unresolved)");
}
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/inc/CameraMover.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001577D0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001577d0.md.
void TrackCarCameraMover::Sub_001577D0() {
    hp2::not_implemented("0x001577D0", "TrackCarCameraMover::Sub_001577D0 (signature and full body unresolved)");
}

// ELF 0x001577D8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001577d8.md.
void TrackCarCameraMover::Sub_001577D8() {
    hp2::not_implemented("0x001577D8", "TrackCarCameraMover::Sub_001577D8 (signature and full body unresolved)");
}

// ELF 0x00157850; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00157850.md.
void TrackCarCameraMover::Sub_00157850() {
    hp2::not_implemented("0x00157850", "TrackCarCameraMover::Sub_00157850 (signature and full body unresolved)");
}

// ELF 0x00157920; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00157920.md.
void TrackCarCameraMover::Sub_00157920() {
    hp2::not_implemented("0x00157920", "TrackCarCameraMover::Sub_00157920 (signature and full body unresolved)");
}

// ELF 0x00159130; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00159130.md.
void TrackCarCameraMover::Sub_00159130() {
    hp2::not_implemented("0x00159130", "TrackCarCameraMover::Sub_00159130 (signature and full body unresolved)");
}

// ELF 0x00159490; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00159490.md.
void TrackCarCameraMover::Sub_00159490() {
    hp2::not_implemented("0x00159490", "TrackCarCameraMover::Sub_00159490 (signature and full body unresolved)");
}

// ELF 0x00159520; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00159520.md.
void TrackCarCameraMover::Sub_00159520() {
    hp2::not_implemented("0x00159520", "TrackCarCameraMover::Sub_00159520 (signature and full body unresolved)");
}

// ELF 0x00159588; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00159588.md.
void TrackCarCameraMover::Sub_00159588() {
    hp2::not_implemented("0x00159588", "TrackCarCameraMover::Sub_00159588 (signature and full body unresolved)");
}

// ELF 0x0015FB00; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fb00.md.
void TrackCarCameraMover::Sub_0015FB00() {
    hp2::not_implemented("0x0015FB00", "TrackCarCameraMover::Sub_0015FB00 (signature and full body unresolved)");
}

// ELF 0x0015FD10; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd10.md.
void TrackCarCameraMover::Sub_0015FD10() {
    hp2::not_implemented("0x0015FD10", "TrackCarCameraMover::Sub_0015FD10 (signature and full body unresolved)");
}

// ELF 0x0015FD18; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd18.md.
void TrackCarCameraMover::Sub_0015FD18() {
    hp2::not_implemented("0x0015FD18", "TrackCarCameraMover::Sub_0015FD18 (signature and full body unresolved)");
}

// ELF 0x0015FD20; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd20.md.
void TrackCarCameraMover::Sub_0015FD20() {
    hp2::not_implemented("0x0015FD20", "TrackCarCameraMover::Sub_0015FD20 (signature and full body unresolved)");
}

// ELF 0x0015FD28; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd28.md.
void TrackCarCameraMover::Sub_0015FD28() {
    hp2::not_implemented("0x0015FD28", "TrackCarCameraMover::Sub_0015FD28 (signature and full body unresolved)");
}

// ELF 0x0015FD30; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd30.md.
void TrackCarCameraMover::Sub_0015FD30() {
    hp2::not_implemented("0x0015FD30", "TrackCarCameraMover::Sub_0015FD30 (signature and full body unresolved)");
}

// ELF 0x0015FD38; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd38.md.
void TrackCarCameraMover::Sub_0015FD38() {
    hp2::not_implemented("0x0015FD38", "TrackCarCameraMover::Sub_0015FD38 (signature and full body unresolved)");
}

// ELF 0x0015FD40; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd40.md.
void TrackCarCameraMover::Sub_0015FD40() {
    hp2::not_implemented("0x0015FD40", "TrackCarCameraMover::Sub_0015FD40 (signature and full body unresolved)");
}
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/inc/CameraMover.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001577D0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001577d0.md.
void TrackFlyCameraMover::Sub_001577D0() {
    hp2::not_implemented("0x001577D0", "TrackFlyCameraMover::Sub_001577D0 (signature and full body unresolved)");
}

// ELF 0x001577D8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001577d8.md.
void TrackFlyCameraMover::Sub_001577D8() {
    hp2::not_implemented("0x001577D8", "TrackFlyCameraMover::Sub_001577D8 (signature and full body unresolved)");
}

// ELF 0x00157850; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00157850.md.
void TrackFlyCameraMover::Sub_00157850() {
    hp2::not_implemented("0x00157850", "TrackFlyCameraMover::Sub_00157850 (signature and full body unresolved)");
}

// ELF 0x00157920; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00157920.md.
void TrackFlyCameraMover::Sub_00157920() {
    hp2::not_implemented("0x00157920", "TrackFlyCameraMover::Sub_00157920 (signature and full body unresolved)");
}

// ELF 0x00159D08; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00159d08.md.
void TrackFlyCameraMover::Sub_00159D08() {
    hp2::not_implemented("0x00159D08", "TrackFlyCameraMover::Sub_00159D08 (signature and full body unresolved)");
}

// ELF 0x00159DE8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00159de8.md.
void TrackFlyCameraMover::Sub_00159DE8() {
    hp2::not_implemented("0x00159DE8", "TrackFlyCameraMover::Sub_00159DE8 (signature and full body unresolved)");
}

// ELF 0x00159E78; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00159e78.md.
void TrackFlyCameraMover::Sub_00159E78() {
    hp2::not_implemented("0x00159E78", "TrackFlyCameraMover::Sub_00159E78 (signature and full body unresolved)");
}

// ELF 0x00159E98; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00159e98.md.
void TrackFlyCameraMover::Sub_00159E98() {
    hp2::not_implemented("0x00159E98", "TrackFlyCameraMover::Sub_00159E98 (signature and full body unresolved)");
}

// ELF 0x0015FAF8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015faf8.md.
void TrackFlyCameraMover::Sub_0015FAF8() {
    hp2::not_implemented("0x0015FAF8", "TrackFlyCameraMover::Sub_0015FAF8 (signature and full body unresolved)");
}

// ELF 0x0015FD10; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd10.md.
void TrackFlyCameraMover::Sub_0015FD10() {
    hp2::not_implemented("0x0015FD10", "TrackFlyCameraMover::Sub_0015FD10 (signature and full body unresolved)");
}

// ELF 0x0015FD18; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd18.md.
void TrackFlyCameraMover::Sub_0015FD18() {
    hp2::not_implemented("0x0015FD18", "TrackFlyCameraMover::Sub_0015FD18 (signature and full body unresolved)");
}

// ELF 0x0015FD20; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd20.md.
void TrackFlyCameraMover::Sub_0015FD20() {
    hp2::not_implemented("0x0015FD20", "TrackFlyCameraMover::Sub_0015FD20 (signature and full body unresolved)");
}

// ELF 0x0015FD28; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd28.md.
void TrackFlyCameraMover::Sub_0015FD28() {
    hp2::not_implemented("0x0015FD28", "TrackFlyCameraMover::Sub_0015FD28 (signature and full body unresolved)");
}

// ELF 0x0015FD30; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd30.md.
void TrackFlyCameraMover::Sub_0015FD30() {
    hp2::not_implemented("0x0015FD30", "TrackFlyCameraMover::Sub_0015FD30 (signature and full body unresolved)");
}

// ELF 0x0015FD38; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd38.md.
void TrackFlyCameraMover::Sub_0015FD38() {
    hp2::not_implemented("0x0015FD38", "TrackFlyCameraMover::Sub_0015FD38 (signature and full body unresolved)");
}

// ELF 0x0015FD40; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd40.md.
void TrackFlyCameraMover::Sub_0015FD40() {
    hp2::not_implemented("0x0015FD40", "TrackFlyCameraMover::Sub_0015FD40 (signature and full body unresolved)");
}
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/TrackRouteEditorCameraMover.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001577D0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001577d0.md.
void TrackRouteEditorCameraMover::Sub_001577D0() {
    hp2::not_implemented("0x001577D0", "TrackRouteEditorCameraMover::Sub_001577D0 (signature and full body unresolved)");
}

// ELF 0x001577D8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001577d8.md.
void TrackRouteEditorCameraMover::Sub_001577D8() {
    hp2::not_implemented("0x001577D8", "TrackRouteEditorCameraMover::Sub_001577D8 (signature and full body unresolved)");
}

// ELF 0x00157850; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00157850.md.
void TrackRouteEditorCameraMover::Sub_00157850() {
    hp2::not_implemented("0x00157850", "TrackRouteEditorCameraMover::Sub_00157850 (signature and full body unresolved)");
}

// ELF 0x00157920; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00157920.md.
void TrackRouteEditorCameraMover::Sub_00157920() {
    hp2::not_implemented("0x00157920", "TrackRouteEditorCameraMover::Sub_00157920 (signature and full body unresolved)");
}

// ELF 0x0015C010; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015c010.md.
void TrackRouteEditorCameraMover::Sub_0015C010() {
    hp2::not_implemented("0x0015C010", "TrackRouteEditorCameraMover::Sub_0015C010 (signature and full body unresolved)");
}

// ELF 0x0015C058; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015c058.md.
void TrackRouteEditorCameraMover::Sub_0015C058() {
    hp2::not_implemented("0x0015C058", "TrackRouteEditorCameraMover::Sub_0015C058 (signature and full body unresolved)");
}

// ELF 0x0015C0A0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015c0a0.md.
void TrackRouteEditorCameraMover::Sub_0015C0A0() {
    hp2::not_implemented("0x0015C0A0", "TrackRouteEditorCameraMover::Sub_0015C0A0 (signature and full body unresolved)");
}

// ELF 0x0015FCB0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fcb0.md.
void TrackRouteEditorCameraMover::Sub_0015FCB0() {
    hp2::not_implemented("0x0015FCB0", "TrackRouteEditorCameraMover::Sub_0015FCB0 (signature and full body unresolved)");
}

// ELF 0x0015FCF8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fcf8.md.
void TrackRouteEditorCameraMover::Sub_0015FCF8() {
    hp2::not_implemented("0x0015FCF8", "TrackRouteEditorCameraMover::Sub_0015FCF8 (signature and full body unresolved)");
}

// ELF 0x0015FD10; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd10.md.
void TrackRouteEditorCameraMover::Sub_0015FD10() {
    hp2::not_implemented("0x0015FD10", "TrackRouteEditorCameraMover::Sub_0015FD10 (signature and full body unresolved)");
}

// ELF 0x0015FD18; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd18.md.
void TrackRouteEditorCameraMover::Sub_0015FD18() {
    hp2::not_implemented("0x0015FD18", "TrackRouteEditorCameraMover::Sub_0015FD18 (signature and full body unresolved)");
}

// ELF 0x0015FD20; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd20.md.
void TrackRouteEditorCameraMover::Sub_0015FD20() {
    hp2::not_implemented("0x0015FD20", "TrackRouteEditorCameraMover::Sub_0015FD20 (signature and full body unresolved)");
}

// ELF 0x0015FD28; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd28.md.
void TrackRouteEditorCameraMover::Sub_0015FD28() {
    hp2::not_implemented("0x0015FD28", "TrackRouteEditorCameraMover::Sub_0015FD28 (signature and full body unresolved)");
}

// ELF 0x0015FD30; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd30.md.
void TrackRouteEditorCameraMover::Sub_0015FD30() {
    hp2::not_implemented("0x0015FD30", "TrackRouteEditorCameraMover::Sub_0015FD30 (signature and full body unresolved)");
}

// ELF 0x0015FD38; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd38.md.
void TrackRouteEditorCameraMover::Sub_0015FD38() {
    hp2::not_implemented("0x0015FD38", "TrackRouteEditorCameraMover::Sub_0015FD38 (signature and full body unresolved)");
}

// ELF 0x0015FD40; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0015fd40.md.
void TrackRouteEditorCameraMover::Sub_0015FD40() {
    hp2::not_implemented("0x0015FD40", "TrackRouteEditorCameraMover::Sub_0015FD40 (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x0015C1D0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0015c1d0.md.
void DriveCameraMover::GetCorrectionPitch() {
    hp2::not_implemented("0x0015C1D0", "DriveCameraMover::GetCorrectionPitch (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/src/cameramover.hpp"

namespace hp2::recovered {
// ELF 0x0015E5E8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0015e5e8.md.
void GarageCameraMover::PrevTrack() {
    hp2::not_implemented("0x0015E5E8", "GarageCameraMover::PrevTrack (signature and full body unresolved)");
}

// ELF 0x0015E610; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0015e610.md.
void GarageCameraMover::NextTrack() {
    hp2::not_implemented("0x0015E610", "GarageCameraMover::NextTrack (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00159540; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00159540.md.
void TrackCarCameraMover::CalcSpringForce() {
    hp2::not_implemented("0x00159540", "TrackCarCameraMover::CalcSpringForce (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x0015A2A0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0015a2a0.md.
void TrackFlyCameraMover::IsFinished() {
    hp2::not_implemented("0x0015A2A0", "TrackFlyCameraMover::IsFinished (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/port/driving/camera.hpp"
#include "game_core/port/driving/ee_scalar.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/indep/src/carstate.hpp"
#include "game_core/indep/src/track.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace port::driving {
namespace {
using V=std::array<float,3>;
using M=std::array<float,16>;
float camera_dot(const V& a,const V& b){return (a[0]*b[0]+a[1]*b[1])+a[2]*b[2];}
V camera_sub(const V& a,const V& b){return {{a[0]-b[0],a[1]-b[1],a[2]-b[2]}};}
V camera_add(const V& a,const V& b){return {{a[0]+b[0],a[1]+b[1],a[2]+b[2]}};}
// Debug00251CC8 vector2 normalization; EE DIV.S zero-divisor saturates, unlike
// host0/0 NaN. This preserves the stationary blend0 instruction path.
V camera_normalize2(const V& v,float length){const float norm=std::sqrt(v[0]*v[0]+v[1]*v[1]);const float divisor=length==0?ee_div_s_zero(norm,length,"0x00251CE4"):norm/length;if(divisor==0)return {{1,0,0}};return {{v[0]/divisor,v[1]/divisor,0}};}
V camera_scale(const V& a,float s){return {{a[0]*s,a[1]*s,a[2]*s}};}
float camera_length(const V& a){return std::sqrt(camera_dot(a,a));}
V camera_cross(const V& a,const V& b){return {{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]}};}
V camera_vector(const Memory& m,std::uint32_t a){return {{m.scalar(a),m.scalar(a+4),m.scalar(a+8)}};}
// Debug0011BDD8 eTranslate; row-vector matrix binding.
M camera_translate(const M& a,const V& t){auto translation=hp2::recovered::bIdentity();for(unsigned i=0;i<3;++i)translation[12+i]=t[i];M out;hp2::recovered::eMulMatrix(out,a,translation);return out;}
V camera_transform(const M& a,const V& v,float w=1){V out;for(unsigned i=0;i<3;++i)out[i]=((v[0]*a[i]+v[1]*a[4+i])+v[2]*a[8+i])+w*a[12+i];return out;}
// eInvertTransformationMatrix rigid transform. No general affine replacement.
M camera_inverse(const M& a){auto out=hp2::recovered::bIdentity();for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)out[i*4+j]=a[j*4+i];for(unsigned i=0;i<3;++i)out[12+i]=-((a[12]*out[i]+a[13]*out[4+i])+a[14]*out[8+i]);return out;}
// Debug001516D8 TestSegmentToPoly, with existing SegmentIntersects001515E0.
bool camera_test_segment(const port::track::Polygon& polygon,const V& target,const V& eye,float margin,float& nearest){
 V low,high,plow=polygon.vertices[0],phigh=plow;
 for(unsigned i=0;i<3;++i){low[i]=std::min(target[i],eye[i])-margin;high[i]=std::max(target[i],eye[i])+margin;}
 for(unsigned v=1;v<polygon.count;++v)for(unsigned i=0;i<3;++i){plow[i]=std::min(plow[i],polygon.vertices[v][i]);phigh[i]=std::max(phigh[i],polygon.vertices[v][i]);}
 for(unsigned i=0;i<3;++i)if(plow[i]>high[i]||phigh[i]<low[i])return false;
 V intersection;if(!hp2::recovered::TrackPolygon::SegmentIntersects(polygon,target,eye,&intersection))return false;
 const float distance=camera_length(camera_sub(intersection,target));if(!(distance<nearest))return false;nearest=distance;return true;
}
// Debug00152500 eCONSTRAIN_BY_SCALE,true + EnforceMinGapToWalls00157420.
V camera_wall_adjust(const Memory& m,const port::track::Topology& topology,const V& target,const V& eye){
 const V center=camera_scale(camera_add(target,eye),.5f);const float radius=camera_length(camera_sub(target,eye))*.5f+1;
 const auto candidates=topology.intersecting({{center[0]-radius,center[1]-radius}},{{center[0]+radius,center[1]+radius}});
 float nearest=m.scalar(0x324f74);const float margin=m.scalar(0x324f9c);
 for(auto index:candidates)camera_test_segment(topology.polygons.at(index),target,eye,margin,nearest);
 const auto boom=camera_sub(eye,target);V adjusted=eye;
 if(nearest<camera_length(boom))adjusted=camera_add(target,hp2::recovered::bNormalize(boom,nearest-margin));
 V adjust=camera_sub(adjusted,eye);
 for(unsigned iteration=0;iteration<3;++iteration){V average{};unsigned count=0;adjusted=camera_add(eye,adjust);
  for(auto index:candidates){const auto& polygon=topology.polygons.at(index);if(polygon.terrain<0||polygon.terrain>=33)throw std::runtime_error("Camera terrain outside original table");
   if(m.byte(0x3102e8+polygon.terrain*0x90+0xd))continue;
   float elevation;if(hp2::recovered::TrackPolygon::PointInShaft(polygon,adjusted,&elevation)&&0<=elevation&&elevation<m.scalar(0x324fa0)){
    for(unsigned i=0;i<3;++i)average[i]+=polygon.plane[i]*(m.scalar(0x324fa0)-elevation);
    ++count;
   }
  }
  if(!count)break;
  adjust=camera_add(adjust,camera_scale(average,1.f/float(count)));
 }
 return adjust;
}
// Debug0015BD10 constructor CameraInfo fallback; E-0088-C01.
std::uint32_t camera_info(const Memory& m,std::uint32_t car){auto info=m.word(car+0xc)+0x3c0;
 if(m.signed_word(info)==0||m.signed_word(info+4)==0){const auto original_default=m.word(0x3278e0);if(!original_default)hp2::not_implemented("0x0015BD10","CameraInfo fallback requires the original default CarTypeInfo camera");info=original_default+0x3c0;}
 return info;
}
// Debug0015BC10/0015BC88. Correction uses original bAngle math/table functions.
float camera_yaw_correction(Memory& m,std::uint16_t angle){const auto signed_angle=static_cast<std::int16_t>(angle);const auto magnitude=std::abs(int(signed_angle));float correction=float(static_cast<std::uint16_t>(hp2::recovered::bATan(m,1.f,float(magnitude)*m.scalar(0x325084))))*m.scalar(0x325088);return signed_angle<0?-correction:correction;}
int camera_pitch_correction(std::uint16_t angle){int value=angle;if(value>0x8000)value-=0x10000;return value<-7||value>7?-value/8:-value;}
}
// Debug Player::CycleDriveCameraPOVIndex00176EE0; E-0088-C01. The playerless
// host explicitly binds no dash when no RideInfo exists; a real RideInfo uses
// original HasDash+6. This is not construction of the original Player object.
int DrivingCamera::cycle_pov(Memory& m,std::uint32_t car){
 const auto info=camera_info(m,car);const auto ride=m.word(car+0x10);
 const bool has_dash=ride&&m.byte(ride+6)!=0,skip_fe=m.word(0x326e9c)!=0;
 int index=(pov_index_+1)%4;
 unsigned attempts=0;
 for(;attempts<4;++attempts,index=(index+1)%4){
  if(index>=m.signed_word(info))continue;
  const auto kind=m.signed_word(info+0x10+index*0x20);
  if(skip_fe||(!has_dash&&(kind==0||kind==2))||(has_dash&&kind==1))break;
 }
 pov_index_=attempts==4?0:index;return pov_index_;
}
// Debug DriveCameraMover::Update0015C2B0; E-0084-C01/E-0088-C01. Bounded fullscreen
// authored POV, default CameraSpring0 / LookbackMode1 / own car / no pursuit/shake.
// Constructor0015BD10 + ResetState0015BF28 state is port-owned, not a guest ABI.
DrivingCameraFrame DrivingCamera::update(Memory& m,std::uint32_t car,float dt,bool look_back,
 const port::track::Topology* topology,const port::track::VoronoiPolyhedron* shape){
 if(!std::isfinite(dt)||dt<0)throw std::invalid_argument("Invalid driving camera elapsed time");
 if(m.word(0x327458))hp2::not_implemented("0x0015C2B0","DriveCameraMover tweakable overrides are outside the bound camera");
 const auto state=car+0x40,info=camera_info(m,car);
 if(pov_index_>=m.signed_word(info))pov_index_=0; // Original GetPOV null fallback.
 if(m.signed_word(info)<=0)hp2::not_implemented("0x0015C2B0","Original camera fallback table has no fullscreen POV");
 const auto pov=info+0x10+static_cast<std::uint32_t>(pov_index_)*0x20;
 const int kind=m.signed_word(pov);
 if(kind<0||kind>2)hp2::not_implemented("0x0015C2B0","Original authored POV type is outside recovered bumper/in-car/outside");
 const auto body=m.matrix(state+0x50);const auto velocity=camera_vector(m,state+0x30);
 V heading{{velocity[0],velocity[1],0}};const float along=heading[0]*body[0]+heading[1]*body[1];
 if(along<0){heading[0]+=body[0]*along*-2.f;heading[1]+=body[1]*along*-2.f;}
 const float horizontal=std::sqrt(heading[0]*heading[0]+heading[1]*heading[1]);
 const float blend=std::clamp((horizontal-m.scalar(0x3250b8))*m.scalar(0x3250bc),0.f,1.f);
 heading=camera_normalize2(heading,blend);heading[0]+=body[0]*(1-blend);heading[1]+=body[1]*(1-blend);
 const auto yaw=static_cast<std::uint16_t>(hp2::recovered::bATan(m,heading[0],heading[1]));
 if(!initialized_){last_yaw_=m.half(state+0x364);initialized_=true;}
 // Original constructor LookbackSpeed argument from Player::ChangeCameraMover.
 const float desired=look_back?1.f:0.f,speed=m.scalar(0x3252e8);
 if(lookback_<desired)lookback_=std::min(desired,lookback_+speed*dt);else if(desired<lookback_)lookback_=std::max(desired,lookback_-speed*dt);
 const auto look_cos_angle=static_cast<std::uint16_t>(cvt_w_s(lookback_*32768.f));
 const float look_cos=hp2::recovered::bSin(m,static_cast<std::uint16_t>(look_cos_angle+0x4000));
 const float look_factor=1.f-(look_cos+1.f)*.5f;
 const auto look_angle=static_cast<std::uint16_t>(cvt_w_s(look_factor*32768.f));
 // Original local longitudinal acceleration and twenty-sample sixty-Hz history.
 const auto acc=camera_vector(m,state+0x40);const float local_acc=(acc[0]*body[0]+acc[1]*body[1])+acc[2]*body[2];
 int sign=0;float selected=0;if(local_acc*m.scalar(0x3274f8)<m.scalar(0x327500)){sign=-1;selected=local_acc*m.scalar(0x3274f8);}else if(m.scalar(0x3274fc)<local_acc*m.scalar(0x3274f4)){sign=1;selected=local_acc*m.scalar(0x3274f4);}
 const float time=dt*60.f+time_fract_;const int steps=cvt_w_s(time);time_fract_=time-float(steps);
 if(steps>600)hp2::not_implemented("0x0015C2B0","Camera elapsed outside bounded native frame interval");
 for(int iteration=0;iteration<steps;++iteration){for(unsigned i=19;i>0;--i)accel_history_[i]=accel_history_[i-1];accel_history_[0]=selected;
  bool change=sign<0?current_accel_>=0:sign>0?current_accel_<=0:false;float sum=selected;
  if(!sign){for(unsigned i=1;i<=9;++i)if(accel_history_[i]<m.scalar(0x3274fc)&&m.scalar(0x327500)<accel_history_[i])lag_adjust_=0;}
  if(change){for(unsigned i=1;i<=9;++i){sum+=accel_history_[i];if(sign>0?accel_history_[i]<m.scalar(0x3274fc):m.scalar(0x327500)<accel_history_[i])change=false;}
   for(unsigned i=10;i<20&&change;++i)if(sign>0?m.scalar(0x3274fc)<accel_history_[i]:accel_history_[i]<m.scalar(0x327500))change=false;
  }
  if(change){const float yaw_slip=hp2::recovered::CarState(m,state).GetYaw();lag_adjust_=sum*m.scalar(0x3250a0)*std::max(0.f,1.f-std::abs(yaw_slip)*m.scalar(0x3274f0));lag_adjust_=std::clamp(lag_adjust_,m.scalar(0x327524),m.scalar(0x327520));hold_time_=0;}
 }
 const float frame_scale=dt*60.f;
 if(lag_adjust_==0){const float step=m.scalar(0x3274dc)*frame_scale;
  if(current_accel_>m.scalar(0x32752c))current_accel_-=step+step;
  else if(current_accel_>0)current_accel_=std::max(0.f,current_accel_-step);
  else if(current_accel_<0)current_accel_=std::min(0.f,current_accel_+step);
 }else{
  bool hold=false;
  if(lag_adjust_>0){const float gap=current_accel_-lag_adjust_;if(gap>0)hold=true;
   else{float adjust=m.scalar(0x3274d8)*frame_scale;if(gap>m.scalar(0x3250a4))adjust*=m.scalar(0x3250a8);else if(gap>-1)adjust*=m.scalar(0x3250ac);current_accel_+=adjust;}
  }else if(m.scalar(state+0x380)*m.scalar(0x3250b0)<5)lag_adjust_=0;
  else if(current_accel_<lag_adjust_)hold=true;
  else current_accel_-=m.scalar(0x3274d8)*frame_scale;
  if(hold){hold_time_+=dt;if(m.scalar(0x3274e0)<hold_time_||!sign)lag_adjust_=0;}
 }
 const float lag_factor=std::max(0.f,m.scalar(0x32750c)-m.scalar(state+0x380)*m.scalar(0x3250b4));
 const float accel_lag=current_accel_*m.scalar(0x327504)*(lag_factor/m.scalar(0x32750c));
 float lag=m.scalar(pov+4),height=m.scalar(pov+8),lateral=m.scalar(pov+12);
 if(kind==2){lag+=accel_lag*(1-look_factor);if(accel_lag<0)height+=accel_lag*m.scalar(0x32751c);}
 if(kind!=2){
  // Original in-car/bumper branch follows full body orientation rather than
  // velocity-blended outside heading; its authored Angle IS consumed.
  boom_yaw_=boom_pitch_=0;last_yaw_=m.half(state+0x364);
  auto inverse_body=body;for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)inverse_body[i*4+j]=body[j*4+i];
  hp2::recovered::eRotateZ(m,inverse_body,inverse_body,look_angle);
  auto translation=hp2::recovered::bIdentity();const auto geometry=camera_vector(m,state+0x20);for(unsigned i=0;i<3;++i)translation[12+i]=-geometry[i];
  M car_matrix;hp2::recovered::eMulMatrix(car_matrix,translation,inverse_body);
  auto boom=camera_translate(hp2::recovered::bIdentity(),{{lag,lateral,-height}});
  hp2::recovered::eRotateY(m,boom,boom,static_cast<std::uint16_t>(-m.half(pov+0x10)));
  M matrix;hp2::recovered::eMulMatrix(matrix,car_matrix,boom);
  if(topology){auto camera_to_world=camera_inverse(matrix);const V eye{{camera_to_world[12],camera_to_world[13],camera_to_world[14]}};
   const float elevation=topology->elevation(eye).height+m.scalar(0x3250e8);
   if(camera_to_world[14]<elevation){camera_to_world[14]=elevation;matrix=camera_inverse(camera_to_world);}
  }
  hp2::recovered::eRotateX(m,matrix,matrix,0x4000);hp2::recovered::eRotateY(m,matrix,matrix,0xc000);
  const auto camera_to_world=camera_inverse(matrix);DrivingCameraFrame frame;
  frame.position={{camera_to_world[12],camera_to_world[13],camera_to_world[14]}};
  frame.target=camera_add(frame.position,{{camera_to_world[8],camera_to_world[9],camera_to_world[10]}});
  frame.up={{-camera_to_world[4],-camera_to_world[5],-camera_to_world[6]}};
  frame.pov_index=pov_index_;frame.lookback_param=lookback_;frame.pov_type=kind;
  frame.fov_bangle=0x4000;frame.near_z=kind==1?m.scalar(0x3250ec):.5f;
  frame.hide_car=true;frame.render_dash=kind==1&&!look_back;return frame;
 }
 touching_ground_=std::clamp(touching_ground_+(m.signed_word(state+0x37c)>2?1.f:-1.f)*m.scalar(0x3250d4),0.f,1.f);
 const auto delta=static_cast<std::uint16_t>(yaw-last_yaw_);
 const auto yaw_delta=static_cast<std::uint16_t>(delta+boom_yaw_);
 boom_yaw_=static_cast<std::uint16_t>(yaw_delta+cvt_w_s(touching_ground_*camera_yaw_correction(m,yaw_delta)*dt));last_yaw_=yaw;
 const auto car_pitch=hp2::recovered::bASin(m,body[2]);int correction_pitch=car_pitch;
 if(!look_angle){if(touching_ground_==0){if(car_pitch>0x7fff)correction_pitch=car_pitch+(0xffff-int(car_pitch))/3;else correction_pitch=car_pitch-car_pitch/9;}
 else{if(car_pitch>0x7fff)correction_pitch=car_pitch-(0xffff-int(car_pitch))/3;else correction_pitch=car_pitch+car_pitch/9;}}
 const auto pitch_delta=static_cast<std::uint16_t>(boom_pitch_-correction_pitch);
 boom_pitch_=static_cast<std::uint16_t>(boom_pitch_+cvt_w_s(float(camera_pitch_correction(pitch_delta))*dt*25.f));
 const float road_noise=hp2::recovered::GetRoadNoise(m,state+0x10,2);
 const auto noisy_pitch=static_cast<std::uint16_t>(cvt_w_s(float(boom_pitch_)+m.scalar(0x327538)*road_noise));
 auto car_matrix=hp2::recovered::bIdentity();const auto geometry=camera_vector(m,state+0x20);
 car_matrix[12]=-geometry[0];car_matrix[13]=-geometry[1];car_matrix[14]=-geometry[2];hp2::recovered::eRotateZ(m,car_matrix,car_matrix,static_cast<std::uint16_t>(-yaw));
 auto boom=hp2::recovered::bIdentity();hp2::recovered::eRotateZ(m,boom,boom,boom_yaw_);hp2::recovered::eRotateY(m,boom,boom,noisy_pitch);hp2::recovered::eRotateZ(m,boom,boom,look_angle);
 auto contender=camera_translate(boom,{{lag,lateral,-height}});hp2::recovered::eMulMatrix(contender,car_matrix,contender);
 const auto inverse=camera_inverse(contender);V eye{{inverse[12],inverse[13],inverse[14]}};
 const auto position=camera_vector(m,state+0x10);const auto projected=camera_transform(contender,position);
 if(projected[0]==0)hp2::not_implemented("0x0015C2B0","Camera projection denominator is zero");
 const float reciprocal=-std::clamp(1.f/projected[0],-1000.f,1000.f);
 const float projection_x=reciprocal*projected[1],projection_y=reciprocal*projected[2];
 if(topology){auto adjust=camera_wall_adjust(m,*topology,position,eye);eye=camera_add(eye,adjust);
  if(shape&&!shape->vertices.empty()){float low=shape->vertices.front().position[0],high=low;for(const auto& v:shape->vertices){low=std::min(low,v.position[0]);high=std::max(high,v.position[0]);}
   const float minimum=std::max(std::abs(low),high)+m.scalar(0x3250e4);const auto car_delta=camera_sub(position,eye);
   if(camera_length(car_delta)<minimum){const float squared=minimum*minimum-(car_delta[0]*car_delta[0]+car_delta[1]*car_delta[1]);if(squared<0)hp2::not_implemented("0x0015C2B0","Camera minimum car clearance square root domain");eye[2]+=std::sqrt(squared)-(eye[2]-position[2]);}
  }else if(camera_length(adjust)>0)hp2::not_implemented("0x001351C8","Adjusted driving camera requires original collision-shape bounds");
 }
 // CameraMover::IsoProjectionMatrix00157618. Original eCreateLookAt basis is
 // right/down/forward (+Z), unlike the host camera's right/up/back convention.
 const auto forward=hp2::recovered::bNormalize(camera_sub(position,eye),1);
 const auto right=hp2::recovered::bNormalize(camera_cross({{0,0,-1}},forward),1);
 const auto down=hp2::recovered::bNormalize(camera_cross(forward,right),1);
 const float distance=camera_length(camera_sub(position,eye));
 V target=position;target=camera_add(target,camera_scale(right,-distance*projection_x));target=camera_add(target,camera_scale(down,-distance*projection_y));
 return {eye,target,{{0,0,1}},pov_index_,lookback_,kind,0x32dc,.5f,false,false};
}
}

namespace hp2::debug {
// Debug0015C2B0/listing, E-0099-W50. Actual shared DriveCameraMover state,
// authored POVs, persistent topology, Car bounds, shake and Camera output.
// Debug alternate-car selection and exceptional frame timing remain explicit.
void DriveCameraMover::Update(){
 auto& m=m_;const auto a=address_;
 const float elapsed=m.scalar(0x327d08),real_elapsed=m.scalar(0x327ce4);
 if(!std::isfinite(elapsed)||elapsed<0||!std::isfinite(real_elapsed)||!(real_elapsed>0))hp2::not_implemented("0x0015C2B0","original live camera requires supported World/RealTimeElapsed context");
 if(!m.word(0x32753c))hp2::not_implemented("0x0015C2B0","original debug alternate-car selection and bRefreshTweaker are not reconstructed");
 const auto player=m.word(a+0x1c),car=m.word(a+0x70);const bool birds_eye=player&&m.byte(player+0x28)==0;
 if(!car)hp2::not_implemented("0x0015C2B0","actual followed Car is not bound");
 std::uint32_t pursuit=0;float lookback_speed=m.scalar(a+0x378);
 if(!m.word(0x328c1c)||m.scalar(a+0x7c)!=1){m.word(a+0x84,0);m.scalar(a+0x88,0);}
 else{bool slow=false;pursuit=GetLookbackCar(m,car,slow);if(slow)lookback_speed=m.scalar(0x32509c);}
 const float desired=m.scalar(a+0x7c);float lookback=m.scalar(a+0x80);
 if(lookback<desired){lookback+=lookback_speed*elapsed;if(desired<lookback)lookback=desired;m.scalar(a+0x80,lookback);}
 else if(desired<lookback){lookback-=lookback_speed*elapsed;if(lookback<desired)lookback=desired;m.scalar(a+0x80,lookback);}
 const auto get_pov=[&](){const auto table=m.word(a+0x28);if(m.word(table+0x3c)!=0x15bfe0)hp2::not_implemented("0x0015C2B0","original derived GetPOV virtual target is not reconstructed");return DriveCameraMover(m,a+std::int32_t(m.signed_half(table+0x38))).GetPOV(m.signed_word(a+0x78));};
 auto pov=get_pov();if(!pov){SetPOVIndex(0);pov=get_pov();}if(!pov)hp2::not_implemented("0x0015C2B0","actual fallback CameraInfo has no required POV");
 const auto body=m.matrix(car+0x90);m.scalar(0x3274d4,1);std::array<float,16> transposed{};eTransposeMatrix(transposed,body);auto acceleration=m.vector(car+0x80);eMulVector(m,acceleration,transposed,acceleration);
 const float multiplier=m.scalar(0x3274d4);int sign=0;float selected=0;
 if((acceleration[0]*m.scalar(0x3274f8))*multiplier<m.scalar(0x327500)*multiplier){sign=-1;selected=acceleration[0]*(m.scalar(0x3274f8)*multiplier);}
 else if(m.scalar(0x3274fc)*multiplier<(acceleration[0]*m.scalar(0x3274f4))*multiplier){sign=1;selected=acceleration[0]*(m.scalar(0x3274f4)*multiplier);}
 const float time=elapsed*60+m.scalar(a+0x3d8);const int steps=::port::driving::cvt_w_s(time);m.scalar(a+0x3d8,time-float(steps));
 for(int step=0;step<steps;++step){
  for(int i=19;i>0;--i){m.word(a+0x37c+std::uint32_t(i)*4,m.word(a+0x37c+std::uint32_t(i-1)*4));}
  m.scalar(a+0x37c,selected);
  bool change=false;float sum=selected;
  if(!sign){for(unsigned i=1;i<=9;++i){const auto value=m.scalar(a+0x37c+i*4);if(value<m.scalar(0x3274fc)*multiplier&&m.scalar(0x327500)*multiplier<value)m.scalar(a+0x3d0,0);}}
  else if((sign>0&&m.scalar(a+0x3cc)<=0)||(sign<0&&!(m.scalar(a+0x3cc)<0))){
   change=true;for(unsigned i=1;i<=9;++i){const auto value=m.scalar(a+0x37c+i*4);sum+=value;if(sign>0?value<m.scalar(0x3274fc)*multiplier:m.scalar(0x327500)*multiplier<value)change=false;}
   if(change)for(unsigned i=10;i<20;++i){const auto value=m.scalar(a+0x37c+i*4);if(sign>0?m.scalar(0x3274fc)*multiplier<value:value<m.scalar(0x327500)*multiplier)change=false;}
  }
  if(change){m.scalar(a+0x3d0,sum*m.scalar(0x3250a0));float yaw=hp2::recovered::CarState(m,car+0x40).GetYaw();if(yaw<0)yaw=-yaw;float factor=1-yaw*m.scalar(0x3274f0);if(factor<0)factor=0;float lag=m.scalar(a+0x3d0)*factor;const float cap=m.scalar(0x327520)*multiplier;if(cap<lag)lag=cap;if(lag<m.scalar(0x327524)*multiplier)lag=m.scalar(0x327524)*multiplier;m.scalar(a+0x3d0,lag);m.scalar(a+0x3d4,0);
   if(!m.word(a+0x374)||m.scalar(car+0x3c0)<=5)m.scalar(a+0x3dc,1);else{if(m.word(car+0x3dc)==1)m.scalar(a+0x3d0,m.scalar(a+0x3d0)*.75f);m.scalar(a+0x3dc,m.scalar(0x327528));m.scalar(a+0x3d0,m.scalar(a+0x3d0)*m.scalar(0x327528));}
  }
 }
 const float frame=elapsed*60;const auto hold=[&](){const float next=m.scalar(a+0x3d4)+elapsed;m.scalar(a+0x3d4,next);if(m.scalar(0x3274e0)<next||!sign)m.scalar(a+0x3d0,0);};
 const float lag_adjust=m.scalar(a+0x3d0);float current=m.scalar(a+0x3cc);
 if(lag_adjust==0){const float step=(m.scalar(0x3274dc)*frame)*m.scalar(a+0x3dc);if(m.scalar(0x32752c)<current)m.scalar(a+0x3cc,current-(step+step));else if(0<current)m.scalar(a+0x3cc,step<current?current-step:0);else if(current<0)m.scalar(a+0x3cc,current<-step?current+step:0);}
 else if(0<lag_adjust){const float gap=current-lag_adjust;if(0<gap)hold();else{float step=(m.scalar(0x3274d8)*m.scalar(a+0x3dc))*frame;if(m.scalar(0x3250a4)<gap)step*=m.scalar(0x3250a8);else if(-1<gap)step*=m.scalar(0x3250ac);m.scalar(a+0x3cc,current+step);}}
 else if(m.scalar(car+0x3c0)*m.scalar(0x3250b0)<5)m.scalar(a+0x3d0,0);
 else if(current<lag_adjust)hold();else m.scalar(a+0x3cc,current+(-m.scalar(0x3274d8)*m.scalar(a+0x3dc))*frame);
 float speed_factor=m.scalar(0x32750c)-m.scalar(car+0x3c0)*m.scalar(0x3250b4);if(speed_factor<0)speed_factor=0;
 const float acceleration_lag=!m.word(a+0x374)?(m.scalar(a+0x3cc)*m.scalar(0x327504))*(speed_factor/m.scalar(0x32750c)):0;
 const auto look_cos_angle=std::uint16_t(::port::driving::cvt_w_s(m.scalar(a+0x80)*32768));const float look_factor=1-(hp2::recovered::bSin(m,std::uint16_t(look_cos_angle+0x4000))+1)*.5f;
 const int kind=m.signed_word(pov);float lag,height,lateral;std::uint16_t angle,max_angle;
 if(!m.word(0x327458)){lag=m.scalar(pov+4);if(kind==2)lag+=acceleration_lag*(1-look_factor);height=m.scalar(pov+8);lateral=m.scalar(pov+12);angle=m.half(pov+0x10);max_angle=m.half(pov+0x12);}
 else{height=m.scalar(0x327460);lag=m.scalar(0x32745c);lateral=m.scalar(0x327464);angle=m.half(0x327468);max_angle=m.half(0x32746a);if(kind==2)lag+=acceleration_lag;}
 if(kind==2&&acceleration_lag<0)height+=acceleration_lag*m.scalar(0x32751c);
 auto translation=hp2::recovered::bIdentity();const auto geometry=m.vector(car+0x50),position=m.vector(car+0x60);for(unsigned axis=0;axis<3;++axis)translation[12+axis]=-geometry[axis];m.half(a+0x8c,std::uint16_t(::port::driving::cvt_w_s(look_factor*32768)));
 std::array<float,16> car_matrix{};std::uint16_t heading_angle=0;
 if(kind==2){auto velocity=m.vector(car+0x70);float x=velocity[0],y=velocity[1];const float along=x*body[0]+y*body[1];if(along<0){const float reflected=along*-2;x+=body[0]*reflected;y+=body[1]*reflected;}float blend=(std::sqrt(x*x+y*y)-m.scalar(0x3250b8))*m.scalar(0x3250bc);if(blend<0)blend=0;if(1<blend)blend=1;
  const float length=std::sqrt(x*x+y*y);const float divisor=blend==0?::port::driving::ee_div_s_zero(length,blend,"0x00251CE4"):length/blend;if(divisor==0){x=1;y=0;}else{x/=divisor;y/=divisor;}x=body[0]*(1-blend)+x;y=body[1]*(1-blend)+y;heading_angle=std::uint16_t(hp2::recovered::bATan(m,x,y));hp2::recovered::eRotateZ(m,car_matrix,translation,std::uint16_t(0-heading_angle));
  if(pursuit&&!birds_eye){const float dx=m.scalar(pursuit+0x60)-position[0],dy=m.scalar(pursuit+0x64)-position[1],distance=std::sqrt(dx*dx+dy*dy);const auto direction=std::uint16_t(hp2::recovered::bATan(m,dx,dy));std::uint32_t delta=(std::uint32_t(heading_angle)-direction)&0xffff;float extra=::port::race::eGetCurrentViewMode(m)==3?0:m.scalar(0x3250c0);const auto info=m.word(car+0xc);if(delta-0x2000<0xc000)extra=(m.scalar(info+0x1ec)-height)+extra;else{delta=0;extra=(m.scalar(info+0x1ec)-height)+extra*.75f;}
   auto previous=m.word(a+0x84);if(std::int32_t(previous)<std::int32_t(delta-0x8000))delta-=0x10000;if(std::int32_t(delta+0x8000)<std::int32_t(previous))delta+=0x10000;
   if(std::int32_t(previous)<std::int32_t(delta)){float rate=m.scalar(0x3250c4);if(std::int32_t(delta)<=std::int32_t(previous+10000)&&(distance<=50||(delta&0xffff)==0))rate=m.scalar(0x3250c8);previous+=std::uint32_t(::port::driving::cvt_w_s((lookback_speed*elapsed)*rate));if(std::int32_t(delta)<std::int32_t(previous))previous=delta;}
   else if(std::int32_t(delta)<std::int32_t(previous)){float rate=m.scalar(0x3250cc);if(std::int32_t(previous-10000)<=std::int32_t(delta)&&(distance<=50||(delta&0xffff)==0))rate=m.scalar(0x3250d0);previous-=std::uint32_t(::port::driving::cvt_w_s((lookback_speed*elapsed)*rate));if(std::int32_t(previous)<std::int32_t(delta))previous=delta;}
   m.word(a+0x84,previous&0xffff);
   float look_height=m.scalar(a+0x88);if(look_height<extra){look_height+=lookback_speed*elapsed;if(extra<look_height)look_height=extra;m.scalar(a+0x88,look_height);}else if(extra<look_height){look_height-=lookback_speed*elapsed;if(look_height<extra)look_height=extra;m.scalar(a+0x88,look_height);}m.half(a+0x8c,std::uint16_t(m.word(a+0x84)));
  }
 }else{std::array<float,16> rotated{};eTransposeMatrix(rotated,body);if(pursuit)m.half(a+0x8c,CalculateLookbackAngle(m,car,pursuit,body));if(!birds_eye)hp2::recovered::eRotateZ(m,rotated,rotated,m.half(a+0x8c));hp2::recovered::eMulMatrix(car_matrix,translation,rotated);}
 const bool outside=kind==2||(birds_eye&&0<m.scalar(a+0x80));auto boom=hp2::recovered::bIdentity();
 if(kind==2){const auto difference=std::uint32_t(heading_angle)-std::uint32_t(m.half(a+0x90));const float ground_step=m.signed_word(car+0x3bc)>2?1:-1;float ground=m.scalar(a+0xb0)+ground_step*m.scalar(0x3250d4);if(1<ground)ground=1;else if(ground<0)ground=0;m.scalar(a+0xb0,ground);const float correction=GetCorrectionYaw(m,std::uint16_t(difference+m.half(a+0x94)));const auto yaw=difference+m.half(a+0x94)+std::uint32_t(::port::driving::cvt_w_s((ground*correction)*elapsed));hp2::recovered::eRotateZ(m,boom,boom,std::uint16_t(yaw));m.word(a+0x94,yaw&0xffff);m.word(a+0x90,heading_angle);
  const auto pitch=hp2::recovered::bASin(m,body[2]);int adjusted=pitch;if(!m.half(a+0x8c)){if(ground==0){if(0x7fff<pitch)adjusted+=(0xffff-int(pitch))/3;else adjusted-=pitch/9;}else{if(0x7fff<pitch)adjusted-=(0xffff-int(pitch))/3;else adjusted+=pitch/9;}}
  auto boom_pitch=std::uint32_t(m.half(a+0xa0));if(ground_step!=0){const int correction_pitch=GetCorrectionPitch(std::uint16_t(boom_pitch-std::uint32_t(adjusted)));boom_pitch+=std::uint32_t(::port::driving::cvt_w_s((float(correction_pitch)*elapsed)*25));}
  const float noise=hp2::recovered::GetRoadNoise(m,car+0x60,2);const auto noisy=std::uint16_t(::port::driving::cvt_w_s(float(boom_pitch&0xffff)+m.scalar(0x327538)*noise));hp2::recovered::eRotateY(m,boom,boom,noisy);if(!birds_eye)hp2::recovered::eRotateZ(m,boom,boom,m.half(a+0x8c));m.word(a+0x9c,pitch);m.word(a+0xa0,boom_pitch&0xffff);
 }else{m.word(a+0x94,0);m.word(a+0x90,m.half(car+0x3a4));const auto pitch=hp2::recovered::bASin(m,body[2]);m.word(a+0xa0,0);m.word(a+0x9c,pitch);}
 if(birds_eye&&0<m.scalar(a+0x80)){const float back=m.scalar(a+0x80),front=1-back;const bool split=::port::race::eGetCurrentViewMode(m)==3;height=split?front*height-back:front*height+back*-3;lag=front*lag+back*(split?16.0f:18.0f);const float blend_angle=front*float(angle)+back*m.scalar(split?0x3250dc:0x3250d8);hp2::recovered::eRotateY(m,boom,boom,std::uint16_t(0-std::uint16_t(::port::driving::cvt_w_s(blend_angle))));}
 std::array<float,2> projection{{0,0}};std::array<float,4> camera_position{};std::array<float,16> result{};float height_ratio=0;
 if(outside){if(pursuit&&!birds_eye){height+=m.scalar(a+0x88);lag=m.scalar(0x3250e0);}std::array<float,16> contender{};hp2::recovered::eTranslate(contender,boom,{{lag,lateral,-height}});hp2::recovered::eMulMatrix(contender,car_matrix,contender);eInvertTransformationMatrix(result,contender);std::array<float,4> initial{{result[12],result[13],result[14],result[15]}},center{},difference{};for(unsigned lane=0;lane<4;++lane){center[lane]=(initial[lane]+position[lane])*.5f;difference[lane]=position[lane]-initial[lane];}const float radius=std::sqrt((difference[0]*difference[0]+difference[1]*difference[1])+difference[2]*difference[2])*.5f+1;const std::array<float,2> velocity{{m.scalar(car+0x70),m.scalar(car+0x74)}};TopologyCoordinate(m,a+0xb4).SetInterestBBox({{center[0],center[1]}},radius,&velocity);
  std::array<float,4> projected{};eMulVector(m,projected,contender,position);const float inverse=projected[0]==0?::port::driving::ee_div_s_zero(1,projected[0],"0x0015C2B0"):1/projected[0];const float reciprocal=-std::max(-1000.0f,std::min(1000.0f,inverse));projection={{reciprocal*projected[1],reciprocal*projected[2]}};
  std::array<float,4> adjustment{};EnforceMinGapToWalls(a+0xb4,position,initial,adjustment);for(unsigned lane=0;lane<4;++lane)camera_position[lane]=initial[lane]+adjustment[lane];
  if(!pursuit){for(unsigned lane=0;lane<4;++lane)difference[lane]=position[lane]-camera_position[lane];const float distance=std::sqrt((difference[0]*difference[0]+difference[1]*difference[1])+difference[2]*difference[2]);if(!m.word(car+0x34))hp2::not_implemented("0x001351C8","original camera clearance requires actual CarCollisionBody bounds");::port::driving::Memory::Temporary bounds(m,32);hp2::recovered::Car(m,car).GetCarSpaceBoundingBox(bounds.address(),bounds.address()+16);const float minimum=std::max(std::abs(m.scalar(bounds.address())),m.scalar(bounds.address()+16))+m.scalar(0x3250e4);if(distance<minimum){const float squared=minimum*minimum-(difference[0]*difference[0]+difference[1]*difference[1]);if(squared<0)hp2::not_implemented("0x0015C2B0","exceptional original near-car camera clearance sqrt");const float vertical=std::sqrt(squared),old_height=camera_position[2]-position[2],delta=vertical-old_height;height_ratio=vertical==0?::port::driving::ee_div_s_zero(old_height,vertical,"0x0015C2B0"):old_height/vertical;adjustment[2]+=delta;camera_position[2]+=delta;}}
  std::array<float,4> local_adjustment{};eMulVector(local_adjustment,contender,adjustment);height+=local_adjustment[2];lateral-=local_adjustment[1];lag-=local_adjustment[0];
 }
 hp2::recovered::eTranslate(boom,boom,{{lag,lateral,-height}});const auto authored_angle=std::uint16_t(std::uint32_t(angle)+std::uint32_t(::port::driving::cvt_w_s(height_ratio*float(int(max_angle)-int(angle)))));hp2::recovered::eRotateY(m,boom,boom,std::uint16_t(0-authored_angle));
 if(!outside){std::array<float,16> contender{};hp2::recovered::eMulMatrix(contender,car_matrix,boom);eInvertTransformationMatrix(contender,contender);const float floor=TopologyCoordinate(m,a+0xb4).GetElevation({{contender[12],contender[13],contender[14]}})+m.scalar(0x3250e8);if(contender[14]<floor){contender[14]=floor;eInvertTransformationMatrix(result,contender);}else hp2::recovered::eMulMatrix(result,car_matrix,boom);hp2::recovered::eRotateX(m,result,result,0x4000);hp2::recovered::eRotateY(m,result,result,0xc000);}
 else IsoProjectionMatrix(result,camera_position,position,projection);
 const auto camera=m.word(a+0x18);if(!camera)hp2::not_implemented("0x0015C2B0","original eView Camera is not bound");const bool stopped=m.word(0x327428)!=0;if(!stopped){m.word(camera+0x1c0,0);m.word(camera+0x1bc,outside?0:car);if(!outside&&kind==1&&m.scalar(a+0x7c)!=1)m.word(camera+0x1c0,1);if(kind==0||kind==1)m.half(camera+0x84,0x4000);else if(kind==2)m.half(camera+0x84,0x32dc);}m.scalar(camera+0x7c,kind==1?m.scalar(0x3250ec):.5f);
 ApplyCameraShake(m,m.signed_word(a+0xc),result);const auto old=m.vector(camera+0x40);const float x=old[0]-position[0],y=old[1]-position[1],z=old[2]-position[2];if(!m.word(0x327428))m.scalar(camera+0x70,std::sqrt((x*x+y*y)+z*z));if(!m.word(a+0x3e0)&&!m.word(0x327428)){m.scalar(camera+0x74,15);m.scalar(camera+0x78,65);}::port::driving::Memory::Temporary matrix(m,64);m.matrix(matrix.address(),result);Camera(m,camera).SetCameraMatrix(matrix.address(),real_elapsed);if(birds_eye)m.half(a+0x8c,0);
}
}
