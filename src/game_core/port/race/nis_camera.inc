// TASK-0112 E-0112-NC01. Debug815b60c7... only; Memory objects, no host ABI.
#include "game_core/port/race/nis_camera.hpp"
#include "game_core/indep/inc/CameraCoordinator.hpp"
#include "game_core/indep/inc/CameraMover.hpp"
#include "game_core/indep/inc/Camera.hpp"
#include "game_core/indep/inc/NIS.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/src/ecstasy/emathe.hpp"
#include "game_core/port/driving/projections.hpp"
namespace hp2::debug {
namespace nis_camera_detail {
constexpr std::uint32_t loader=0x307ae0,list=0x327b98;
void unlink(::port::driving::Memory& m,std::uint32_t a){const auto n=m.word(a),p=m.word(a+4);m.word(p,n);m.word(n+4,p);}
void mark(::port::driving::Memory& m){for(auto n=m.word(list);n!=list;n=m.word(n)){if(!n)hp2::not_implemented("0x00197700","source camera coordinator list is not initialized");m.word(n+0x14,1);}}
}
void initialize_source_nis_camera_state(::port::driving::Memory& m){
 using namespace nis_camera_detail;m.word(list,list);m.word(list+4,list);
 for(unsigned o:{0u,0x804u,0x888u,0x90cu,0x990u,0x9d0u})m.word(loader+o,0);
 for(unsigned o:{0x9c0u,0x9c4u,0x9c8u})m.scalar(loader+o,0);
}
int LoaderCameras(::port::driving::Memory& m,std::uint32_t chunk){
 using namespace nis_camera_detail;const auto id=m.word(chunk);std::uint32_t count=0,table=0,tag=0,capacity=0;
 switch(id){case 0x80034405:count=0;table=4;tag=0x34450;capacity=512;break;case 0x80034425:count=0x804;table=0x808;tag=0x34490;capacity=32;break;case 0x80034410:count=0x888;table=0x88c;tag=0x34460;capacity=32;break;case 0x80034415:count=0x90c;table=0x910;tag=0x34470;capacity=32;break;case 0x80034420:count=0x990;table=0x994;tag=0x34480;capacity=8;break;
 case 0x34492:m.word(loader+0x9b8,(chunk+23)&~15u);m.word(loader+0x9b4,m.word(chunk+4)/0x230);mark(m);return 1;default:return 0;}
 m.word(loader+count,0);const auto end=std::uint64_t(chunk)+8+m.word(chunk+4);
 for(std::uint64_t c=chunk+8;c<end;){if(c+8>end)hp2::not_implemented("0x00197770","malformed source camera chunk header");const auto child=std::uint32_t(c),size=m.word(child+4);if(c+8+size>end)hp2::not_implemented("0x00197770","malformed source camera chunk size");const auto childid=m.word(child),data=(child+23)&~15u;
  if(id==0x80034405&&childid==0x34440){m.copy(loader+0x9c0,data,16);m.word(loader+0x9d0,m.word(data+16));}
  if(childid==tag){const auto n=m.word(loader+count);if(n>=capacity)hp2::not_implemented("0x00197770","source fixed camera table capacity exceeded");m.word(loader+table+n*4,data);m.word(loader+count,n+1);}c+=8+size;
 }mark(m);return 1;
}
int UnloaderCameras(::port::driving::Memory& m,std::uint32_t chunk){std::uint32_t o;switch(m.word(chunk)){case 0x80034405:o=0;break;case 0x80034425:o=0x804;break;case 0x80034410:o=0x888;break;case 0x80034415:o=0x90c;break;case 0x80034420:o=0x990;break;case 0x34492:o=0x9b4;break;default:return 0;}m.word(nis_camera_detail::loader+o,0);return 1;}
void MaxCameraMover::Sub_00159DF8(int view,std::uint32_t car,std::uint32_t data,float y,int start){Sub_001571E8(view,10);m_.word(address_+0x2c,car);m_.scalar(address_+0x30,y);m_.word(address_+0x28,0x2a9900);m_.word(address_+0x34,start?std::uint32_t(start):m_.word(0x327d04));m_.word(address_+0x38,data+0x24);m_.word(address_+0x3c,data);m_.word(address_+0x40,m_.word(data+0x20));const auto camera=m_.word(address_+0x18);if(!m_.word(0x327428)){m_.word(camera+0x1bc,0);m_.word(camera+0x1c0,0);}m_.scalar(camera+0x7c,m_.scalar(0x324ffc));}
void MaxCameraMover::Sub_00159EB0(int flags){CameraMover::Sub_00157280(flags);}
void MaxCameraMover::Update(){
 auto& m=m_;const auto a=address_,rc=m.word(0x327290);if(rc&&m.word(rc)==6)return;
 const auto car=m.word(a+0x2c),view=m.word(a+0xc);const bool over=(m.word(car+0x510)>>(view&31))&1;const auto position=m.vector(car+(over?0x500:0x50));const auto body=m.matrix(car+(over?0x4c0:0x90));
 auto translation=hp2::recovered::bIdentity();for(unsigned j=0;j<3;++j)translation[12+j]=-position[j];std::array<float,16> transpose{},carmat{};eTransposeMatrix(transpose,body);hp2::recovered::eMulMatrix(carmat,translation,transpose);
 auto frame=(m.signed_word(0x327d00)-m.signed_word(a+0x34))/2;if(m.signed_word(a+0x40)<=frame){m.word(a+0x34,m.word(0x327d00));frame=0;}if(frame<0||m.signed_word(a+0x40)<=0)hp2::not_implemented("0x00159EC8","source camera key index outside retained CAM data");const auto key=m.word(a+0x38)+std::uint32_t(frame)*48;const auto q=m.vector(key+16);const float x=q[0],y=q[1],z=q[2],w=q[3],xx=x+x,yy=y+y,zz=z+z,ww=w+w,y2=yy*y,z2=zz*z,f=1-xx*x,yz=yy*z;
 auto rotation=hp2::recovered::bIdentity();rotation[0]=(1-y2)-z2;rotation[1]=xx*y-ww*z;rotation[2]=xx*z+ww*y;rotation[4]=xx*y+ww*z;rotation[5]=f-z2;rotation[6]=yz-ww*x;rotation[8]=xx*z-ww*y;rotation[9]=yz+ww*x;rotation[10]=f-y2;eTransposeMatrix(rotation,rotation);
 hp2::recovered::eRotateX(m,rotation,rotation,m.half(0x3274a8));hp2::recovered::eRotateY(m,rotation,rotation,m.half(0x3274aa));hp2::recovered::eRotateZ(m,rotation,rotation,m.half(0x3274ac));auto cc=hp2::recovered::bIdentity();hp2::recovered::eTranslate(cc,cc,{{m.scalar(a+0x30)-m.scalar(key),-m.scalar(key+4),-m.scalar(key+8)}});hp2::recovered::eMulMatrix(cc,cc,rotation);std::array<float,16> final{};hp2::recovered::eMulMatrix(final,carmat,cc);
 const auto camera=m.word(a+0x18);if(!m.word(0x327428))m.half(camera+0x84,m.scalar(0x3274b0)==32768?std::uint16_t(m.word(key+32)):std::uint16_t(::port::driving::cvt_w_s(m.scalar(0x3274b0))));::port::driving::Memory::Temporary tmp(m,64);m.matrix(tmp.address(),final);Camera(m,camera).SetCameraMatrix(tmp.address(),m.scalar(0x327ce4));
}
void NisCameraCoordinator::Sub_00198DF8(int view,std::uint32_t car,std::uint32_t other,int index,float y,int start){using namespace nis_camera_detail;const auto previous=m_.word(list+4);if(!previous)hp2::not_implemented("0x00197558","source camera coordinator startup list missing");m_.word(previous,address_);m_.word(list+4,address_);m_.word(address_+4,previous);m_.word(address_,list);m_.word(address_+8,0);m_.word(address_+0xc,0);m_.word(address_+0x10,std::uint32_t(view));m_.word(address_+0x14,0);m_.word(address_+0x18,0x2ad028);m_.word(address_+0x1c,car);m_.word(address_+0x20,other);const int count=m_.signed_word(loader+0x990);if(count<=index)index=count-1;
 if(count<1){const auto mover=m_.allocate(0x3f0);DriveCameraMover(m_,mover).Sub_0015BD10(view,car,2,m_.scalar(0x32561c),0,0,1);m_.word(address_+8,mover);}else{if(index<0)hp2::not_implemented("0x00198DF8","negative source NIS camera index");const auto mover=m_.allocate(0x44);MaxCameraMover(m_,mover).Sub_00159DF8(view,car,m_.word(loader+0x994+std::uint32_t(index)*4),-y,start);m_.word(address_+8,mover);}}
void NisCameraCoordinator::Sub_00198F18(int flags){if(const auto mover=m_.word(address_+8)){if(m_.word(mover+8)==10)MaxCameraMover(m_,mover).Sub_00159EB0(3);else if(m_.word(mover+8)==1)DriveCameraMover(m_,mover).Sub_0015BEB8(3);else hp2::not_implemented("0x00198F18","unknown source coordinator mover destructor");m_.word(address_+8,0);}nis_camera_detail::unlink(m_,address_);if(flags&1)m_.release(address_);}
void NisCameraCoordinator::Update(float){}
int NisCameraCoordinator::NumCameraKeys()const{const auto mover=m_.word(address_+8);return m_.word(mover+8)==10?m_.signed_word(mover+0x40):0;}
std::uint32_t NisCameraCoordinator::GetCameraKeys()const{const auto mover=m_.word(address_+8);return m_.word(mover+8)==10?m_.word(mover+0x38):0;}
void NisCameraCoordinator::Disable(){if(!m_.word(address_+0xc)){m_.word(address_+0xc,1);if(const auto mover=m_.word(address_+8))CameraMover(m_,mover).Disable();}}
void NisCameraCoordinator::Enable(){if(m_.word(address_+0xc)){m_.word(address_+0xc,0);if(const auto mover=m_.word(address_+8))CameraMover(m_,mover).Enable();}}
}
