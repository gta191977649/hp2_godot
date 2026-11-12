// TASK-0099 E-0099-W38. Original ..\indep\src\contact.cpp, MCP ECOFF source proof.
// NFSHP2.ELF Debug SHA256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/indep/inc/Contact.hpp"
#include "game_core/port/driving/ee_scalar.hpp"
#include "game_core/inc/bSlotPool.hpp"
namespace hp2::debug {
// Debug001D12D0/001D1300, E-0099-W38.
void InitContact(::port::driving::Memory& m){m.word(0x328394,bNewSlotPool(m,0x40,32,0x2b0110,16));}
void CloseContact(::port::driving::Memory& m){bDeleteSlotPool(m,m.word(0x328394));m.word(0x328394,0);}
// Debug001D26C8, E-0099-W38, inline Contact.hpp emitted in original contact.cpp.
// The bTNode<Contact> base receives0; it does not unlink or free.
void Contact::Sub_001D26C8(int flags){if(flags&1)SlotPool(m_,m_.word(0x328394)).Free(address_);}
// Debug001D1328, E-0099-W38. Save next before deletion and remove either body order.
void Contact::RemoveContacts(::port::driving::Memory& m,std::uint32_t list,std::uint32_t body){
 for(auto p=m.word(list);p!=list;){const auto next=m.word(p);if(m.word(p+0xc)==body||m.word(p+0x10)==body){const auto prev=m.word(p+4);m.word(prev,next);m.word(next+4,prev);Contact(m,p).Sub_001D26C8(3);}p=next;}
}
// Debug001D13A0, E-0099-W38. Comparator is non-strict <=, including equal-time ties.
int Contact::ContactTimeSorting(const ::port::driving::Memory& m,std::uint32_t before,std::uint32_t after){return m.scalar(before+8)<=m.scalar(after+8);}
}
#include "game_core/indep/inc/CollisionBody.hpp"
#include "game_core/indep/inc/Smokeable.hpp"
#include "game_core/indep/src/voronoiclip.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cmath>
namespace port::race::detail {
std::uint32_t shared_collision_pointer(::port::driving::Memory&,std::uint32_t,unsigned);
// Port dispatch bindings consume the actual target and signed this adjustment.
void contact_interpolate(::port::driving::Memory& m,std::uint32_t body,std::uint32_t position,std::uint32_t rotation,float backup){
 const auto table=m.word(body+0x50),target=m.word(table+0x54),adjusted=body+static_cast<std::int32_t>(m.signed_half(table+0x50));
 switch(target){case 0x1d4040:hp2::debug::DynamicCollisionBody(m,adjusted).Interpolate(position,rotation,backup);return;
 case 0x1d5d50:m.copy(rotation,0x3115e0,64);m.copy(position,0x3115d0,16);return;
 default:hp2::not_implemented("0x001D1408","Unrecovered collision Interpolate virtual target");}
}
float contact_backup_time(::port::driving::Memory& m,std::uint32_t body){
 const auto table=m.word(body+0x50),target=m.word(table+0x3c),adjusted=body+static_cast<std::int32_t>(m.signed_half(table+0x38));
 switch(target){case 0x1d5c00:return hp2::debug::DynamicCollisionBody(m,adjusted).GetMaxBackupTime();case 0x1d5da0:return 1000;
 default:hp2::not_implemented("0x001D1408","Unrecovered GetMaxBackupTime virtual target");}
}
int contact_moveable(::port::driving::Memory& m,std::uint32_t body){const auto table=m.word(body+0x50),target=m.word(table+0xac),adjusted=body+static_cast<std::int32_t>(m.signed_half(table+0xa8));
 switch(target){case 0x21c828:return hp2::debug::Smokeable(m,adjusted).IsMoveable();case 0x1d5b10:return 1;case 0x1d5c90:return 0;default:hp2::not_implemented("0x001D1698","Unrecovered IsMoveable virtual target");}
}
// Debug001D15C0, E-0099-W38. MIPS vsub.w clears only position W; the
// imported CompressedNormal names describe float bit storage at this use site.
std::array<float,4> FindWaferNormal(::port::driving::Memory& m,std::uint32_t track,std::uint32_t dynamic){
 const auto face=m.word(m.word(track+8)+0x28),pos=shared_collision_pointer(m,dynamic,0x18);
 std::array<float,4> n{{m.scalar(face+4),m.scalar(face+8),m.scalar(face+12),0}};
 const float dot=(n[0]*m.scalar(pos)+n[1]*m.scalar(pos+4))+n[2]*m.scalar(pos+8);
 if(!(m.scalar(face+16)-m.scalar(0x3283a0)<dot))for(unsigned i=0;i<3;++i)n[i]=-n[i];return n;
}
}
namespace hp2::debug {
// Debug001D13C8, E-0099-W38; original contact.cpp.
void Contact::Sub_001D13C8(std::uint32_t first,std::uint32_t second,std::uint32_t witness,std::optional<std::uint32_t> pad){
 m_.word(address_+0xc,first);m_.word(address_+0x10,second);m_.word(address_+0x14,witness);
 const float backup=FindContactTime(pad);m_.scalar(address_+8,backup);m_.word(address_+0x30,m_.word(0x327d10));
}
// Debug001D1408, E-0099-W38. Exactly8 bisections and original strict threshold.
float Contact::FindContactTime(std::optional<std::uint32_t> pad){
 auto& m=m_;if(!pad)hp2::not_implemented("0x001D1478","Original CrowbarVector store contains unwritten stack W; explicit port padding policy required");
 for(unsigned i=0;i<3;++i)m.scalar(address_+0x20+i*4,0);m.word(address_+0x2c,*pad);
 const auto b1=m.word(address_+0xc),b2=m.word(address_+0x10),w=m.word(address_+0x14);
 const float t1=::port::race::detail::contact_backup_time(m,b1),t2=::port::race::detail::contact_backup_time(m,b2);float upper=t2<t1?t2:t1,lower=0,backup=0;
 ::port::driving::Memory::Temporary temp(m,160);const auto r1=temp.address(),r2=r1+64,p1=r2+64,p2=p1+16;
 for(unsigned i=0;i<8;++i){backup=lower+(upper-lower)*0.5f;
  ::port::race::detail::contact_interpolate(m,b1,p1,r1,backup);::port::race::detail::contact_interpolate(m,b2,p2,r2,backup);
  const auto status=hp2::recovered::VCWitness::VClip(m,w+0x10,r1,p1,r2,p2,pad);
  if(status!=2){if(m.scalar(w+0x50)<m.scalar(0x328398))return backup;upper=backup;}else lower=backup;
  if(i==7&&status==2)Crowbar(backup,pad);
 }return backup;
}
// Debug001D1698, E-0099-W38. Actual full four-lane movement,20-attempt cap.
void Contact::Crowbar(float backup,std::optional<std::uint32_t> pad){
 auto& m=m_;const auto b1=m.word(address_+0xc),b2=m.word(address_+0x10),w=m.word(address_+0x14);
 const auto inspect_crowbar=[&](float result,float left,float right,unsigned lane,const char* stage,const char* pc){if(!std::isfinite(left)||!std::isfinite(right)||!std::isfinite(result)){const auto bits=[](float value){std::uint32_t word;std::memcpy(&word,&value,4);return std::to_string(word);};const std::string why=std::string("Crowbar arithmetic producer stage=")+stage+" first="+std::to_string(b1)+" second="+std::to_string(b2)+" lane="+std::to_string(lane)+" left_bits="+bits(left)+" right_bits="+bits(right)+" result_bits="+bits(result)+" backup_bits="+bits(backup);hp2::not_implemented(pc,why.c_str());}return result;};
 ::port::driving::Memory::Temporary temp(m,192);const auto r1=temp.address(),r2=r1+64,p1=r2+64,p2=p1+16,new1=p2+16,new2=new1+16;
 ::port::race::detail::contact_interpolate(m,b1,p1,r1,backup);::port::race::detail::contact_interpolate(m,b2,p2,r2,backup);
 std::array<float,4> normal{};
 if(m.byte(b1+0xc)==TRACK_POLYGON_COLLISION_BODY)normal=::port::race::detail::FindWaferNormal(m,b1,b2);
 else if(m.byte(b2+0xc)==TRACK_POLYGON_COLLISION_BODY){normal=::port::race::detail::FindWaferNormal(m,b2,b1);for(unsigned i=0;i<3;++i)normal[i]=-normal[i];normal[3]=1-normal[3];}
 else{const auto a=m.vector(p1),b=m.vector(p2);for(unsigned i=0;i<4;++i)(void)inspect_crowbar(a[i],a[i],b[i],i,"center-input","0x001D1778");for(unsigned i=0;i<4;++i)normal[i]=inspect_crowbar(::port::driving::ee_vsub_finite(b[i],a[i],"0x001D1778"),b[i],a[i],i,"center-sub","0x001D1778");const float square=(normal[0]*normal[0]+normal[1]*normal[1])+normal[2]*normal[2];
  if(!std::isfinite(square)||square<0||(square==0&&!::port::driving::pc_sqrt_carrier_enabled())){const auto bits=[](float value){std::uint32_t word;std::memcpy(&word,&value,4);return std::to_string(word);};std::string why="Crowbar center producer square_bits="+bits(square)+" first="+std::to_string(b1)+" second="+std::to_string(b2);for(unsigned lane=0;lane<4;++lane)why+=" pos1_"+std::to_string(lane)+"="+bits(a[lane])+" pos2_"+std::to_string(lane)+"="+bits(b[lane])+" normal_"+std::to_string(lane)+"="+bits(normal[lane]);hp2::not_implemented("0x001D1790",why.c_str());}for(float lane:a)if(!std::isfinite(lane))hp2::not_implemented("0x001D1778","Crowbar input center1 contains a nonfinite source lane");for(float lane:b)if(!std::isfinite(lane))hp2::not_implemented("0x001D1778","Crowbar input center2 contains a nonfinite source lane");
  float scale;if(square==0){::port::diagnostics::warn("0x001D1790","original Crowbar VRSQRT denominator is exactly zero; Sony defines signed maximum and continues","using the explicitly enabled legacy finite PC carrier; VU extended maximum encoding and flags remain unverified");scale=::port::driving::ee_vrsqrt_one_zero(square,"0x001D1790");}else scale=1/std::sqrt(square);for(unsigned lane=0;lane<4;++lane)normal[lane]=inspect_crowbar(::port::driving::ee_mul_s_finite(normal[lane],scale,"0x001D17A4"),normal[lane],scale,lane,"normalize-mul","0x001D17A4");
 }
 float distance=0,increment=m.scalar(0x325aa4);
 for(unsigned i=0;i<20;++i){distance+=increment;if(increment<m.scalar(0x325aa8))increment+=m.scalar(0x325aac);
  auto a=m.vector(p1),b=m.vector(p2);std::array<float,4> move{};for(unsigned j=0;j<4;++j)move[j]=inspect_crowbar(::port::driving::ee_mul_s_finite(normal[j],distance,"0x001D1800"),normal[j],distance,j,"distance-mul","0x001D1800");
  if(::port::race::detail::contact_moveable(m,b1))for(unsigned j=0;j<4;++j)a[j]=inspect_crowbar(::port::driving::ee_vsub_finite(a[j],move[j],"0x001D183C"),a[j],move[j],j,"position1-sub","0x001D183C");
  if(::port::race::detail::contact_moveable(m,b2))for(unsigned j=0;j<4;++j)b[j]=inspect_crowbar(::port::driving::ee_vadd_finite(b[j],move[j],"0x001D186C"),b[j],move[j],j,"position2-add","0x001D186C");m.vector(new1,a);m.vector(new2,b);
  if(hp2::recovered::VCWitness::VClip(m,w+0x10,r1,new1,r2,new2,pad)!=2){m.vector(address_+0x20,move);return;}
 }
}
}

namespace hp2::debug {
// Debug001D1AE0, E-0099-W38. Uses actual current transforms after Backup, not
// interpolation snapshots; bMulMatrix's original implicit vector W comes from
// Debug317B20, independently reviewed with W35, not the input CP padding.
void Contact::FindContactPointAndNormal(std::uint32_t point,std::uint32_t normal){
 auto& m=m_;const auto w=m.word(address_+0x14),b1=m.word(address_+0xc),b2=m.word(address_+0x10);
 const auto r1=::port::race::detail::shared_collision_pointer(m,b1,0x28),r2=::port::race::detail::shared_collision_pointer(m,b2,0x28);
 std::array<float,4> cp1,cp2;const ::port::driving::SolverMathContext context{m.vector(0x317b20)};
 const auto inspect_normal=[&](const char* stage,const std::array<float,4>& left,const std::array<float,4>& right,float scale){for(unsigned lane=0;lane<4;++lane)if(!std::isfinite(left[lane])||!std::isfinite(right[lane])){const auto bits=[](float v){std::uint32_t value;std::memcpy(&value,&v,4);return std::to_string(value);};std::string why=std::string("Contact normal producer stage=")+stage+" lane="+std::to_string(lane)+" first="+std::to_string(b1)+" second="+std::to_string(b2)+" left_bits="+bits(left[lane])+" right_bits="+bits(right[lane])+" leftW_bits="+bits(left[3])+" rightW_bits="+bits(right[3])+" scale_bits="+bits(scale)+" witness1W_bits="+bits(m.scalar(w+0x3c))+" witness2W_bits="+bits(m.scalar(w+0x4c));for(unsigned row=0;row<4;++row)why+=" matrix1W"+std::to_string(row)+"_bits="+bits(m.scalar(r1+row*16+12))+" matrix2W"+std::to_string(row)+"_bits="+bits(m.scalar(r2+row*16+12));hp2::not_implemented("0x001D1AE0",why.c_str());}};
 hp2::recovered::bMulMatrix(cp1,m.matrix(r1),m.vector(w+0x30),context);hp2::recovered::bMulMatrix(cp2,m.matrix(r2),m.vector(w+0x40),context);inspect_normal("matrix-result",cp1,cp2,0);
 const auto pos1=m.vector(::port::race::detail::shared_collision_pointer(m,b1,0x18)),pos2=m.vector(::port::race::detail::shared_collision_pointer(m,b2,0x18));
 inspect_normal("input-positions",pos1,pos2,0);std::array<float,4> cp,n;for(unsigned i=0;i<4;++i){cp1[i]=::port::driving::ee_vadd_finite(cp1[i],pos1[i],"0x001D1B84");cp2[i]=::port::driving::ee_vadd_finite(cp2[i],pos2[i],"0x001D1BA8");const float sum=::port::driving::ee_vadd_finite(cp1[i],cp2[i],"0x001D1BC8");cp[i]=::port::driving::ee_mul_s_finite(sum,.5f,"0x001D1BD8");}inspect_normal("translated-points",cp1,cp2,0);inspect_normal("averaged-contact-point",cp,cp,0);m.vector(point,cp);
 if(!m.byte(b1+0xd))n=::port::race::detail::FindWaferNormal(m,b1,b2);
 else if(!m.byte(b2+0xd)){n=::port::race::detail::FindWaferNormal(m,b2,b1);for(unsigned i=0;i<3;++i)n[i]=-n[i];n[3]=1-n[3];}
 else{for(unsigned i=0;i<4;++i)n[i]=::port::driving::ee_vsub_finite(cp2[i],cp1[i],"0x001D1C38");float square=(n[0]*n[0]+n[1]*n[1])+n[2]*n[2];
  if(!std::isfinite(square))hp2::not_implemented("0x001D1AE0","Exceptional EE contact-normal square root is not modeled");
  float length=std::sqrt(square);const bool use_centers=length<m.scalar(0x325ab0);if(use_centers){for(unsigned i=0;i<4;++i)n[i]=::port::driving::ee_vsub_finite(pos2[i],pos1[i],"0x001D1CB0");square=(n[0]*n[0]+n[1]*n[1])+n[2]*n[2];length=std::sqrt(square);}
  float factor;
  if(use_centers&&square==0){
   if(!::port::driving::pc_sqrt_carrier_enabled())hp2::not_implemented("0x001D1CC8","source zero contact-center VRSQRT requires explicit PC finite carrier");
   ::port::diagnostics::warn("0x001D1CC8","Original contact centers coincide; VU VRSQRT zero denominator produces signed MAX","using explicit finite PC reciprocal projection; zero XYZ and consumed W retained, no replacement normal or exact VU encoding claim");
   factor=::port::driving::ee_vrsqrt_one_zero(square,"0x001D1CC8");
  }else{if(!(length>0)||!std::isfinite(length))hp2::not_implemented("0x001D1AE0","Exceptional contact-normal reciprocal square root is not modeled");factor=1/length;}
  inspect_normal("normal-before-scale",n,n,factor);const auto before=n;for(auto& lane:n)lane=::port::driving::ee_vmul_finite(lane,factor,use_centers?"0x001D1CDC":"0x001D1D04");inspect_normal("normal-after-scale",n,before,factor);
 }m.vector(normal,n);
}
}

namespace hp2::debug {
// Debug001D1EA8, E-0099-W38. VOPMSUB.xyz retains loaded angularW;
// subsequent VADD.xyzw makes outputW=linearW+angularW, not an unwritten pad.
void GetContactPointVelocity(::port::driving::Memory& m,std::uint32_t output,std::uint32_t velocity,std::uint32_t angular,std::uint32_t arm){
 using namespace ::port::driving;const auto w=m.vector(angular),r=m.vector(arm),v=m.vector(velocity);
 // E-0112-CP01: source001D1EB4/1EB8 cross writesXYZ only, preserving loaded
 // angularW; source001D1EC8 then adds all four lanes. Reuse the explicit
 // bounded PC VU carrier, including true product-overflow ACC dominance.
 auto cross=w;for(unsigned lane=0;lane<3;++lane){const auto next=(lane+1)%3,last=(lane+2)%3;auto acc=ee_vmula_finite(w[next],r[last],"0x001D1EB4");cross[lane]=ee_vmsuba_finite(acc,r[next],w[last],"0x001D1EB8").value;}
 std::array<float,4> result;for(unsigned lane=0;lane<4;++lane)result[lane]=ee_vadd_finite(v[lane],cross[lane],"0x001D1EC8");m.vector(output,result);
}
// Debug001D1ED8, E-0099-W38. The original uses bMulMatrix's actual implicit
// input vector and full matrix; do not replace it with a projected3x3 multiply.
float DenominatorTerm(::port::driving::Memory& m,std::uint32_t inertia,std::uint32_t arm,std::uint32_t normal){const auto r=m.vector(arm),n=m.vector(normal);const std::array<float,4> cross{{r[1]*n[2]-r[2]*n[1],r[2]*n[0]-r[0]*n[2],r[0]*n[1]-r[1]*n[0],r[3]}};
 std::array<float,4> mat;hp2::recovered::bMulMatrix(mat,m.matrix(inertia),cross,::port::driving::SolverMathContext{m.vector(0x317b20)});
 const std::array<float,3> cross2{{mat[1]*r[2]-mat[2]*r[1],mat[2]*r[0]-mat[0]*r[2],mat[0]*r[1]-mat[1]*r[0]}};return (n[0]*cross2[0]+n[1]*cross2[1])+n[2]*cross2[2];
}
}
#include "game_core/indep/src/collision.hpp"
#include "game_core/port/driving/ee_scalar.hpp"
#include <cstring>
namespace port::race::detail {
// Port dispatch bindings of original virtual families; all deltas are loaded
// from original shared vtables, never inferred from the native C++ class.
void contact_velocities(::port::driving::Memory& m,std::uint32_t body,std::uint32_t v,std::uint32_t w){const auto table=m.word(body+0x50),target=m.word(table+0x64),adjusted=body+static_cast<std::int32_t>(m.signed_half(table+0x60));
 switch(target){case 0x1d4378:hp2::debug::DynamicCollisionBody(m,adjusted).GetCurrentVelocities(v,w);return;case 0x1d5d28:m.copy(v,0x3115d0,16);m.copy(w,0x3115d0,16);return;default:hp2::not_implemented("0x001D1F58","Unrecovered GetCurrentVelocities virtual target");}}
float contact_scalar(::port::driving::Memory& m,std::uint32_t body,unsigned slot){const auto table=m.word(body+0x50),target=m.word(table+slot+4),adjusted=body+static_cast<std::int32_t>(m.signed_half(table+slot));
 switch(target){case 0x21c7f8:return hp2::debug::Smokeable(m,adjusted).GetFriction();case 0x21c808:return hp2::debug::Smokeable(m,adjusted).GetBounciness();case 0x21c880:return hp2::debug::Smokeable(m,adjusted).GetMassInverse();case 0x1d5aa0:return m.scalar(0x325b14);case 0x1d5cd8:return m.scalar(0x325b1c);case 0x1d5a90:return m.scalar(0x325b10);case 0x1d5cc8:return m.scalar(0x325b18);case 0x1d4ed0:return hp2::debug::CarCollisionBody(m,adjusted).GetMassInverse();case 0x1d5cf8:return 0;case 0x1d5b18:case 0x1d5cb0:return 1;
 default:hp2::not_implemented("0x001D1F58","Unrecovered collision scalar/ConsiderImpulse virtual target");}}
// E-0110-F13: original ConsiderImpulse consumes the current impulse and arm.
float contact_consider(::port::driving::Memory& m,std::uint32_t body,std::uint32_t impulse,std::uint32_t arm,bool disable_sound){const auto table=m.word(body+0x50),target=m.word(table+0x7c),adjusted=body+static_cast<std::int32_t>(m.signed_half(table+0x78));
 switch(target){case 0x21bc48:return hp2::debug::Smokeable(m,adjusted).ConsiderImpulse(impulse,arm,disable_sound);case 0x1d5b18:case 0x1d5cb0:return 1;default:hp2::not_implemented("0x001D1F58","Unrecovered ConsiderImpulse virtual target");}}
std::uint32_t contact_inertia(::port::driving::Memory& m,std::uint32_t body){const auto table=m.word(body+0x50),target=m.word(table+0xa4),adjusted=body+static_cast<std::int32_t>(m.signed_half(table+0xa0));switch(target){case 0x21c850:return hp2::debug::Smokeable(m,adjusted).GetInertiaTensorInverse();case 0x1d4f08:return hp2::debug::CarCollisionBody(m,adjusted).GetInertiaTensorInverse();case 0x1d5ce8:return 0x311620;default:hp2::not_implemented("0x001D1F58","Unrecovered GetInertiaTensorInverse virtual target");}}
void contact_apply(::port::driving::Memory& m,std::uint32_t body,std::uint32_t impulse,std::uint32_t arm){const auto table=m.word(body+0x50),target=m.word(table+0x84),adjusted=body+static_cast<std::int32_t>(m.signed_half(table+0x80));switch(target){case 0x21c8e8:hp2::debug::Smokeable(m,adjusted).ApplyImpulse(impulse,arm);return;case 0x1d4f50:hp2::debug::CarCollisionBody(m,adjusted).ApplyImpulse(impulse,arm);return;case 0x1d5ca8:return;default:hp2::not_implemented("0x001D1F58","Unrecovered ApplyImpulse virtual target");}}
}
namespace hp2::debug {
// Debug001D1F58, E-0099-W38/E-0110-F13. Actual ordered ConsiderImpulse
// callbacks, fractional application and source unbounded velocity retry loop.
// Ordinary finite four-lane arithmetic only; unsupported EE/VU exceptional lanes
// are explicit boundaries, never filled from the unknown-stack padding policy.
void Contact::CollisionImpulse(bool disable_smokeable_collision_sound){
 using namespace ::port::race::detail;auto& m=m_;const auto first=m.word(address_+0xc),second=m.word(address_+0x10),witness=m.word(address_+0x14);
 ::port::driving::Memory::Temporary temp(m,208);const auto cp=temp.address(),normal=cp+16,arm1=normal+16,arm2=arm1+16,v1=arm2+16,a1=v1+16,v2=a1+16,a2=v2+16,cv1=a2+16,cv2=cv1+16,imp1=cv2+16,imp2=imp1+16;
 FindContactPointAndNormal(cp,normal);const auto collision=CollisionBodyWitness(m,witness).GetCollision(cp);
 auto r1=m.vector(cp),r2=r1;const auto p1=m.vector(shared_collision_pointer(m,first,0x18)),p2=m.vector(shared_collision_pointer(m,second,0x18));for(unsigned i=0;i<4;++i){r1[i]-=p1[i];r2[i]-=p2[i];}m.vector(arm1,r1);m.vector(arm2,r2);
 for(;;){
 contact_velocities(m,first,v1,a1);contact_velocities(m,second,v2,a2);GetContactPointVelocity(m,cv1,v1,a1,arm1);GetContactPointVelocity(m,cv2,v2,a2,arm2);
 const auto n=m.vector(normal),velocity1=m.vector(cv1),velocity2=m.vector(cv2);std::array<float,4> relative,scrape;for(unsigned i=0;i<4;++i)relative[i]=::port::driving::ee_vsub_finite(velocity2[i],velocity1[i],"0x001D20BC");const float incoming=(relative[0]*n[0]+relative[1]*n[1])+relative[2]*n[2];const float opposite=-incoming;for(unsigned i=0;i<4;++i){const float projected=::port::driving::ee_mul_s_finite(n[i],opposite,"0x001D20F4");scrape[i]=::port::driving::ee_vadd_finite(projected,relative[i],"0x001D20F8");}if(0<=incoming)return;
 const float bounce1=contact_scalar(m,first,0xc0),bounce2=contact_scalar(m,second,0xc0),inverse1=contact_scalar(m,first,0x98),inverse2=contact_scalar(m,second,0x98);
 float denominator=inverse1+inverse2;denominator+=DenominatorTerm(m,contact_inertia(m,first),arm1,normal);denominator+=DenominatorTerm(m,contact_inertia(m,second),arm2,normal);
 const float inverse=inverse1<inverse2?inverse2:inverse1;if(denominator==0||inverse==0||!std::isfinite(denominator)||!std::isfinite(inverse))hp2::not_implemented("0x001D1F58","Exceptional original contact impulse division is not modeled");
 float impulse=(-((bounce1<bounce2?bounce2:bounce1)+1)*incoming)/denominator;const float minimum=m.scalar(0x3283ac)/inverse;if(impulse<minimum)impulse=minimum;
 auto tentative=n;for(auto& lane:tentative)lane=::port::driving::ee_mul_s_finite(lane,impulse,"0x001D221C");m.vector(imp2,tentative);for(unsigned i=0;i<4;++i)tentative[i]=::port::driving::ee_vsub_finite(i==3?1.f:0.f,tentative[i],"0x001D224C");m.vector(imp1,tentative);
 float factor2=contact_consider(m,second,imp2,arm2,disable_smokeable_collision_sound),factor1=contact_consider(m,first,imp1,arm1,disable_smokeable_collision_sound);if(factor1==m.scalar(0x325ab4)||factor2==m.scalar(0x325ab4))return;
 bool stop=true;if(factor1<0)factor1=-factor1;else if(factor2<0)factor2=-factor2;else stop=false;
 const float fraction=factor2<factor1?factor2:factor1;impulse=fraction*impulse;
 const float friction1=contact_scalar(m,first,0xc8),friction2=contact_scalar(m,second,0xc8);float friction=friction1<friction2?friction2:friction1;std::uint32_t special=0;
 const auto upright=[&](std::uint32_t body,std::uint32_t bits_address){const std::uint64_t bits=std::uint64_t(m.word(bits_address))|(std::uint64_t(m.word(bits_address+4))<<32);double threshold;std::memcpy(&threshold,&bits,8);return double(m.scalar(shared_collision_pointer(m,body,0x28)+0x28))>threshold;};
 if(inverse1==0&&friction2==m.scalar(0x325ab8)&&!m.byte(second+0xc)&&upright(second,0x2b0120))special=second;
 else if(inverse2==0&&friction1==m.scalar(0x325abc)&&!m.byte(first+0xc)&&upright(first,0x2b0128))special=first;
 if(special)friction=m.scalar(0x325ac0);
 const float speed=std::sqrt((scrape[0]*scrape[0]+scrape[1]*scrape[1])+scrape[2]*scrape[2]);float friction_magnitude=impulse*friction;if(speed<m.scalar(0x3283b8))friction_magnitude*=speed/m.scalar(0x3283b8);
 const float friction_scale=speed==0?::port::driving::ee_div_s_zero(-friction_magnitude,speed,"0x001D2450"):-friction_magnitude/speed;std::array<float,4> total2,total1;for(unsigned i=0;i<4;++i){const float friction_lane=::port::driving::ee_mul_s_finite(scrape[i],friction_scale,"0x001D245C"),normal_lane=::port::driving::ee_mul_s_finite(n[i],impulse,"0x001D2468");total2[i]=::port::driving::ee_vadd_finite(normal_lane,friction_lane,"0x001D2478");total1[i]=::port::driving::ee_vsub_finite(i==3?1.0f:0.0f,total2[i],"0x001D2484");}
 if(special){const auto velocity=m.vector(shared_collision_pointer(m,special,0x20));const float linear_speed=std::sqrt((velocity[0]*velocity[0]+velocity[1]*velocity[1])+velocity[2]*velocity[2]);if(30<linear_speed){float scale=m.scalar(0x325ac4);if(linear_speed*scale<m.scalar(0x325ac8))scale=m.scalar(0x325ac8)/linear_speed;auto push=velocity;for(auto& lane:push)lane*=scale;push[2]=0;
   auto& target=inverse1==0?total2:total1;auto& arm=inverse1==0?r2:r1;const float magnitude=std::sqrt((target[0]*target[0]+target[1]*target[1])+target[2]*target[2]);for(unsigned i=0;i<4;++i)target[i]+=push[i];if(1<linear_speed+magnitude){const float ratio=magnitude/(linear_speed*m.scalar(inverse1==0?0x325acc:0x325ad0)+magnitude);for(auto& lane:arm)lane*=ratio;}
  }}
 for(unsigned i=0;i<4;++i)if(!std::isfinite(total1[i])||!std::isfinite(total2[i])){const auto bits=[](float value){std::uint32_t word;std::memcpy(&word,&value,4);return std::to_string(word);};const std::string diagnostic="Exceptional full-lane EE/VU friction impulse: lane="+std::to_string(i)+" first="+std::to_string(first)+" second="+std::to_string(second)+" speed_bits="+bits(speed)+" normal_bits="+bits(n[i])+" scrape_bits="+bits(scrape[i])+" impulse_bits="+bits(impulse)+" friction_bits="+bits(friction)+" magnitude_bits="+bits(friction_magnitude)+" scale_bits="+bits(friction_scale)+" total2_bits="+bits(total2[i])+" total1_bits="+bits(total1[i]);hp2::not_implemented("0x001D2450",diagnostic.c_str());}
 m.vector(imp2,total2);m.vector(imp1,total1);m.vector(arm2,r2);m.vector(arm1,r1);contact_apply(m,second,imp2,arm2);contact_apply(m,first,imp1,arm1);Collision(m,collision).AddImpulse(cp,imp2,speed);
 if(fraction==1||stop)return;
 }
}
}
#include "game_core/port/race/physics_construction.hpp"
#include "game_core/indep/inc/Smokeable.hpp"
#include <cstdio>
namespace port::race::detail {
// Port diagnostic only; preserves the exact required source call-site guard.
std::string contact_virtual_description(const char* method,std::uint32_t target){char text[160];std::snprintf(text,sizeof(text),"Unrecovered %s virtual target 0x%08X",method,unsigned(target));return text;}
void contact_promote(::port::driving::Memory& m,std::uint32_t body,std::uint32_t hitter,PhysicsConstructionServices& service){const auto table=m.word(body+0x50),target=m.word(table+0x74),adjusted=body+static_cast<std::int32_t>(m.signed_half(table+0x70));switch(target){case 0x1d4dd0:hp2::debug::CarCollisionBody(m,adjusted).SetToPhysicsMode(hitter,service);return;case 0x1d5bf8:hp2::debug::DynamicCollisionBody(m,adjusted).SetToPhysicsMode(hitter);return;case 0x1d5cc0:return;default:hp2::not_implemented("0x001D18D8",contact_virtual_description("SetToPhysicsMode",target).c_str());}}
void contact_backup(::port::driving::Memory& m,std::uint32_t body,float backup){const auto table=m.word(body+0x50),target=m.word(table+0x4c),adjusted=body+static_cast<std::int32_t>(m.signed_half(table+0x48));switch(target){case 0x1d4298:hp2::debug::DynamicCollisionBody(m,adjusted).Backup(backup);return;case 0x1d5d90:return;default:hp2::not_implemented("0x001D18D8","Unrecovered Backup virtual target");}}
void contact_set_position(::port::driving::Memory& m,std::uint32_t body,std::uint32_t position){const auto table=m.word(body+0x50),target=m.word(table+0x34),adjusted=body+static_cast<std::int32_t>(m.signed_half(table+0x30));switch(target){case 0x1d5c08:hp2::debug::DynamicCollisionBody(m,adjusted).SetPosition(position);return;case 0x1d5db0:return;default:hp2::not_implemented("0x001D18D8","Unrecovered SetPosition virtual target");}}
void contact_recompute(::port::driving::Memory& m,std::uint32_t body,PhysicsConstructionServices& service){const auto table=m.word(body+0x50),target=m.word(table+0x5c),adjusted=body+static_cast<std::int32_t>(m.signed_half(table+0x58));switch(target){case 0x1d4348:hp2::debug::DynamicCollisionBody(m,adjusted).RecomputeAuxilliaries(service);return;case 0x1d5d48:return;default:hp2::not_implemented("0x001D18D8","Unrecovered RecomputeAuxilliaries virtual target");}}
// E-0110-E03: original Smokeable table002B4670 points to existing0021B688.
void contact_move(::port::driving::Memory& m,std::uint32_t body,float time,PhysicsConstructionServices& service){const auto table=m.word(body+0x50),target=m.word(table+0x44),adjusted=body+static_cast<std::int32_t>(m.signed_half(table+0x40));switch(target){case 0x1d4e28:hp2::debug::CarCollisionBody(m,adjusted).Move(time,service);return;case 0x21b688:hp2::debug::Smokeable(m,adjusted).Move(time,service);return;case 0x1d5d98:return;default:hp2::not_implemented("0x001D18D8",contact_virtual_description("Move",target).c_str());}}
}
namespace hp2::debug {
// Debug001D18D8, E-0099-W38. Original ordered promotion, backup, position
// correction, two auxiliary passes around impulse, then remaining-time movement.
void Contact::Resolve(::port::race::PhysicsConstructionServices& service){
 using namespace ::port::race::detail;auto& m=m_;const auto first=m.word(address_+0xc),second=m.word(address_+0x10);contact_promote(m,first,second,service);contact_promote(m,second,first,service);
 const float backup=m.scalar(address_+8);contact_backup(m,first,backup);contact_backup(m,second,backup);::port::driving::Memory::Temporary positions(m,32);const auto p1=positions.address(),p2=p1+16;m.copy(p1,shared_collision_pointer(m,first,0x18),16);m.copy(p2,shared_collision_pointer(m,second,0x18),16);
 const auto crowbar=m.vector(address_+0x20);auto a=m.vector(p1),b=m.vector(p2);const auto inspect_correction=[&](float result,float position,float correction,unsigned lane,const char* pc){if(!std::isfinite(position)||!std::isfinite(correction)||!std::isfinite(result)){const auto bits=[](float value){std::uint32_t word;std::memcpy(&word,&value,4);return std::to_string(word);};const std::string why="Contact Crowbar correction producer first="+std::to_string(first)+" second="+std::to_string(second)+" lane="+std::to_string(lane)+" position_bits="+bits(position)+" correction_bits="+bits(correction)+" result_bits="+bits(result);hp2::not_implemented(pc,why.c_str());}return result;};if(contact_moveable(m,first))for(unsigned i=0;i<4;++i)a[i]=inspect_correction(::port::driving::ee_vsub_finite(a[i],crowbar[i],"0x001D19C0"),a[i],crowbar[i],i,"0x001D19C0");if(contact_moveable(m,second))for(unsigned i=0;i<4;++i)b[i]=inspect_correction(::port::driving::ee_vadd_finite(b[i],crowbar[i],"0x001D19F0"),b[i],crowbar[i],i,"0x001D19F0");m.vector(p1,a);m.vector(p2,b);contact_set_position(m,first,p1);contact_set_position(m,second,p2);
 contact_recompute(m,first,service);contact_recompute(m,second,service);CollisionImpulse(service.disable_smokeable_collision_sound);contact_recompute(m,first,service);contact_recompute(m,second,service);contact_move(m,first,backup,service);contact_move(m,second,backup,service);
}
}
