// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\collisionbody.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): CarCollisionBody::Move() [0x002B8700]
#include "game_core/indep/inc/CollisionBody.hpp"
#include "game_core/port/driving/ee_scalar.hpp"
#include "game_core/indep/src/copmgr.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/NumberAllocator.hpp"
#include "game_core/inc/bSlotPool.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/indep/src/voronoiclip.hpp"
#include "game_core/indep/src/collision.hpp"
#include "game_core/indep/inc/Polyhedra.hpp"
#include "game_core/indep/inc/Smokeable.hpp"
#include "game_core/port/race/smokeables.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/port/race/collision_resources.hpp"
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace hp2::debug {
// Debug001D2758/001D27F0/001D2830/001D28F0/001D5910, E-0099-W19.
// Preserve floor(max/8), including the original nonmultiple-of-eight tail gap;
// the port allocator does not fabricate accessible original heap padding.
void NumberAllocator::SetMaxNumbers(int maximum){
    auto& m=memory_;if(m.word(address_)!=0){m.word(address_,0);if(auto field=m.word(address_+4))m.release(field);m.word(address_+4,0);m.word(address_+8,0);}
    if(maximum!=0){m.word(address_,std::uint32_t(maximum));
        if(maximum<8)hp2::not_implemented("0x001D2758","original NumberAllocator zero/negative byte allocation is not represented by port Memory");
        const auto size=std::uint32_t(maximum>>3),field=m.allocate(size);m.word(address_+4,field);
        std::memset(m.view(field,size).data,0,size);m.word(address_+8,0);}
}
int NumberAllocator::AllocateNumber(){const int number=memory_.signed_word(address_+8);if(number>=memory_.signed_word(address_))return -1;AllocateNumber(number);return number;}
void NumberAllocator::AllocateNumber(int number){
    auto& m=memory_;const auto field=m.word(address_+4);
    const auto read=[&](int candidate){try{return m.byte(field+std::uint32_t(candidate>>3));}
        catch(const std::out_of_range&){hp2::not_implemented("0x001D2830","original NumberAllocator access requires unrepresented heap tail bytes");}};
    if(m.signed_word(address_+8)==number){int next=number+1;
        while(next<m.signed_word(address_) && (read(next)&(1u<<(std::uint32_t(next)&7))))++next;
        m.word(address_+8,std::uint32_t(next));}
    const auto value=read(number);m.byte(field+std::uint32_t(number>>3),std::uint8_t(value|(1u<<(std::uint32_t(number)&7))));
}
void NumberAllocator::FreeNumber(int number){auto& m=memory_;const auto at=m.word(address_+4)+std::uint32_t(number>>3);
    try{m.byte(at,std::uint8_t(m.byte(at)&~(1u<<(std::uint32_t(number)&7))));}
    catch(const std::out_of_range&){hp2::not_implemented("0x001D28F0","original NumberAllocator release requires unrepresented heap tail bytes");}
    if(number<m.signed_word(address_+8))m.word(address_+8,std::uint32_t(number));}
void NumberAllocator::Sub_001D5910(int in_chrg){SetMaxNumbers(0);if(in_chrg&1)memory_.release(address_);}
// Debug001D2938/001D2A58/001D3A58, E-0099-W19. Explicit original lifecycle.
void InitCollisionBody(::port::driving::Memory& m){
    m.word(0x32842c,bNewSlotPool(m,0x150,14,0x2b01d0,0x10));m.word(0x328428,bNewSlotPool(m,0x438,160,0x2b01f0,0x10));
    m.word(0x328430,bNewSlotPool(m,0x70,256,0x2b0218,0x10));const auto allocator=m.allocate(12);
    m.word(allocator,0);m.word(allocator+4,0);m.word(allocator+8,0);NumberAllocator(m,allocator).SetMaxNumbers(206);m.word(0x3283fc,allocator);
    const auto table=m.allocate(0x338);m.word(0x3283d0,table);std::memset(m.view(table,0x338).data,0,0x338);
    m.matrix(0x3115e0,hp2::recovered::bIdentity());std::memset(m.view(0x311620,0x40).data,0,0x40);
    const auto witnesses=m.allocate(0x14b8);m.word(0x328424,witnesses);std::memset(m.view(witnesses,0x14b8).data,0,0x14b8);
}
void CloseCollisionBody(::port::driving::Memory& m){
    bDeleteSlotPool(m,m.word(0x328428));bDeleteSlotPool(m,m.word(0x32842c));bDeleteSlotPool(m,m.word(0x328430));
    m.word(0x328428,0);m.word(0x32842c,0);m.word(0x328430,0);
    if(auto allocator=m.word(0x3283fc))NumberAllocator(m,allocator).Sub_001D5910(3);m.word(0x3283fc,0);
    if(auto table=m.word(0x3283d0))m.release(table);m.word(0x3283d0,0);
    if(auto field=m.word(0x328424))m.release(field);m.word(0x328424,0);
}
std::uint16_t GetNextCollisionBodyID(const ::port::driving::Memory& m){return std::uint16_t(m.word(m.word(0x3283fc)+8));}
// Debug001D58C8, E-0099-W15. Explicit original process-static initialization;
// CollisionStats counters are original initialized data, not reset by this call.
void Sub_001D58C8(::port::driving::Memory& m){
    m.word(0x3283dc,0x3283d8);m.scalar(0x3115d8,0);m.scalar(0x3115d0,0);
    m.word(0x328404,0x328400);m.scalar(0x3115d4,0);
    m.word(0x3283d8,0x3283d8);m.word(0x328400,0x328400);
}
// Debug001D3520, E-0099-W15/W19. Negative IDs bypass NumberAllocator;
// positive IDs reserve the original allocator bit and body table entry.
void CollisionBody::Sub_001D3520(int id,CollisionBodyType type){
    auto& m=memory_;const auto at=address_;m.word(at+0x50,0x2b0830);m.byte(at+0xc,std::uint8_t(type));m.half(at+0xe,std::uint16_t(id));
    if(id>=0){NumberAllocator(m,m.word(0x3283fc)).AllocateNumber(std::int16_t(id));m.word(m.word(0x3283d0)+4*std::uint32_t(std::int16_t(id)),at);}
    const auto count=m.word(0x311588)+1;m.word(0x311588,count);
    if(m.signed_word(0x3115ac)<std::int32_t(count))m.word(0x3115ac,count);
    m.byte(at+0x24,0);const auto next=m.word(0x3283d8);const auto initial=m.word(0x325ad4);
    if(!next)hp2::not_implemented("0x001D58C8","original CollisionBody process static initialization has not run");
    m.word(0x3283d8,at);m.word(next+4,at);m.word(at+0x20,initial);m.word(at+4,0x3283d8);m.word(at,next);
    m.word(at+0x4c,0);m.word(at+0x14,initial);m.word(at+0x18,initial);m.word(at+0x1c,initial);
    m.byte(at+0x11,0);m.byte(at+0x12,0);m.byte(at+0x10,0);m.byte(at+0x13,0);
}
// Debug001D5310, E-0099-W15. Dynamic is only set after the base constructor.
void StaticCollisionBody::Sub_001D5310(int id,CollisionBodyType type){
    Sub_001D3520(id,type);auto& m=memory_;const auto count=m.word(0x31158c)+1;m.word(0x31158c,count);
    m.word(address_+0x50,0x2b0760);if(m.signed_word(0x3115b0)<std::int32_t(count))m.word(0x3115b0,count);
    m.byte(address_+0xd,0);
}
// Debug001D3788, E-0099-W15. Keep separate float comparisons (including NaNs).
int CollisionBodyFlipFunction(const ::port::driving::Memory& m,std::uint32_t before,std::uint32_t after){
    const float a=m.scalar(after+0x1c),b=m.scalar(before+0x1c);
    if(a<b)return 1;if(a<=b)return m.signed_half(after+0xe)<m.signed_half(before+0xe);return 0;
}
// Debug001D37D0, E-0099-W15. On shrinking the maximum, exclude this body;
// the recovered scan does not insert new_width as an extra candidate.
void CollisionBody::UpdateMaxCollisionBodyWidth(float width){
    auto& m=memory_;const float previous=m.scalar(address_+0x1c)-m.scalar(address_+0x14);
    if(width!=previous){
        if(previous<width && m.scalar(0x3283e0)<width){m.scalar(0x3283e0,width);return;}
        if(width<previous && previous==m.scalar(0x3283e0)){
            m.scalar(0x3283e0,0);
            for(auto body=m.word(0x3283d8);body!=0x3283d8;body=m.word(body))if(body!=address_){
                const float candidate=m.scalar(body+0x1c)-m.scalar(body+0x14);
                if(m.scalar(0x3283e0)<candidate)m.scalar(0x3283e0,candidate);
            }
        }
    }
}
// Debug001D3888, E-0099-W15. Original incremental sorted-list repositioning.
void CollisionBody::SetBBoxMinMax(const std::array<float,2>& minimum,const std::array<float,2>& maximum){
    auto& m=memory_;const auto at=address_;UpdateMaxCollisionBodyWidth(maximum[0]-minimum[0]);
    m.scalar(at+0x18,minimum[1]);m.scalar(at+0x14,minimum[0]);const float previous=m.scalar(at+0x1c);
    m.scalar(at+0x1c,maximum[0]);m.scalar(at+0x20,maximum[1]);
    if(previous!=maximum[0])while(true){
        while(true){const auto before=m.word(at+4);if(before==0x3283d8 || !CollisionBodyFlipFunction(m,before,at))break;
            const auto next=m.word(at),prev=m.word(at+4);m.word(prev,next);m.word(next+4,prev);
            const auto preceding=m.word(before+4);m.word(preceding,at);m.word(before+4,at);m.word(at+4,preceding);m.word(at,before);
        }
        const auto after=m.word(at);if(after==0x3283d8 || !CollisionBodyFlipFunction(m,at,after))break;
        const auto next=m.word(at),prev=m.word(at+4);m.word(prev,next);m.word(next+4,prev);
        const auto following=m.word(after);m.word(after,at);m.word(following+4,at);m.word(at+4,after);m.word(at,following);
    }
}
// Debug001D3A68 and bStrNCpy0024C9A8, E-0099-W15. No tail zero-fill.
void CollisionBody::SetDebugName(const std::string& name){
    for(std::size_t i=0;i<40;++i){const auto c=i<name.size()?std::uint8_t(name[i]):0;memory_.byte(address_+0x24+std::uint32_t(i),c);if(!c)break;}
    memory_.byte(address_+0x4b,0);
}
// Debug001D3A48, E-0099-W15. The listing is addiu/jr/addiu only.
void CollisionBody::PlotToCaffeine(const std::array<float,16>&,const std::array<float,3>&){}
// Debug001D35E8, E-0099-W15/W19. Release matching real witnesses and positive
// IDs; reached active witness dependencies retain their exact missing callees.
void CollisionBody::Sub_001D35E8(int in_chrg){
    auto& m=memory_;m.word(address_+0x50,0x2b0830);m.word(0x311588,m.word(0x311588)-1);
    UpdateMaxCollisionBodyWidth(0);const auto next=m.word(address_),previous=m.word(address_+4);m.word(previous,next);m.word(next+4,previous);
    if(m.byte(address_+0x10)!=0)for(auto witness=m.word(0x328400);witness!=0x328400;){const auto next_witness=m.word(witness);
        if(m.word(witness+0x60)==address_ || m.word(witness+0x64)==address_)CollisionBodyWitness(m,witness).Sub_001D2BF0(3);witness=next_witness;}
    if(const int id=m.signed_half(address_+0xe);id>=0){m.word(m.word(0x3283d0)+4*std::uint32_t(id),0);NumberAllocator(m,m.word(0x3283fc)).FreeNumber(id);}
    if(in_chrg&1)m.release(address_);
}
// Debug001D5370, E-0099-W15.
void StaticCollisionBody::Sub_001D5370(int in_chrg){memory_.word(address_+0x50,0x2b0760);memory_.word(0x31158c,memory_.word(0x31158c)-1);Sub_001D35E8(in_chrg);}
// Debug001D3BE0/001D3C60, E-0099-W19. Geometry remains the derived ctor's job.
void DynamicCollisionBody::Sub_001D3BE0(int id,CollisionBodyType type,std::uint32_t topology){
    Sub_001D3520(id,type);auto& m=memory_;const auto count=m.word(0x311590)+1;m.word(0x311590,count);
    m.word(address_+0x50,0x2b05c0);m.word(address_+0x58,topology);
    if(m.signed_word(0x3115b4)<std::int32_t(count))m.word(0x3115b4,count);
    m.byte(address_+0xd,1);m.word(address_+0x5c,0);m.word(address_+0x60,0);m.word(address_+0x54,0);
}
void DynamicCollisionBody::Sub_001D3C60(int in_chrg){memory_.word(address_+0x50,0x2b05c0);memory_.word(0x311590,memory_.word(0x311590)-1);Sub_001D35E8(in_chrg);}
// Debug001D3D90, E-0099-W23. Original bounds include zero before the scan.
void DynamicCollisionBody::BuildBoundingVolumes(){
    auto& m=memory_;const auto poly=m.word(address_+8);float radius=0,cylinder=0;std::array<float,3> low{},high{};
    for(int i=0;i<m.signed_half(poly+0x10);++i){const auto vertex=m.word(poly+0x20)+20*std::uint32_t(i);
        const float x=m.scalar(vertex+4),y=m.scalar(vertex+8),z=m.scalar(vertex+12),xy=x*x+y*y;
        const float rxy=std::sqrt(xy),rxyz=std::sqrt(xy+z*z);if(radius<rxyz)radius=rxyz;if(cylinder<rxy)cylinder=rxy;
        const std::array<float,3> p{{x,y,z}};for(unsigned j=0;j<3;++j){if(p[j]<=high[j]){if(p[j]<low[j])low[j]=p[j];}else high[j]=p[j];}
    }
    const float padding=m.scalar(0x325ad8);m.scalar(address_+0xdc,radius);m.scalar(address_+0xd0,padding-low[2]);
    m.scalar(address_+0xd4,high[2]+padding);m.scalar(address_+0xd8,cylinder+padding);
    for(unsigned j=0;j<3;++j){m.scalar(address_+0xe0+4*j,low[j]);m.scalar(address_+0xf0+4*j,high[j]);}
}
// Debug001D3F28; E-0099-W23/W76. Original signed descriptor18/1C dispatch;
// actual Car001D5B68 and Smokeable0021C928 getters only. Other targets throw.
void DynamicCollisionBody::UpdatePosition(){
    auto& m=memory_;const auto table=m.word(address_+0x50),target=m.word(table+0x1c),self=address_+std::int32_t(m.signed_half(table+0x18));std::uint32_t position;
    if(target==0x1d5b68)position=CarCollisionBody(m,self).GetPosition();
    else if(target==0x21c928)position=Smokeable(m,self).GetPosition();
    else hp2::not_implemented("0x001D3F28","original dynamic-body GetPosition virtual target is not reconstructed");
    const float x=m.scalar(position),y=m.scalar(position+4),radius=m.scalar(address_+0xd8);
    SetBBoxMinMax({{x-radius,y-radius}},{{x+radius,y+radius}});
}
namespace {
// Port-only bounded string extraction; the original field/concatenation/hash
// come from CarCollisionBody001D4A80/001D4BD8 and captured CarTypeInfo metadata.
std::string port_collision_volume_name(const ::port::driving::Memory& m,std::uint32_t car){
    const auto name=m.word(car+0xc)+0x20;std::string result;bool terminated=false;
    for(unsigned i=0;i<32;++i){const auto c=m.byte(name+i);if(!c){terminated=true;break;}result.push_back(char(c));}
    if(!terminated)throw std::out_of_range("Original CarTypeInfo BaseModelName lacks terminator");return result+"_CV";
}
}
// Debug001D4A80, E-0099-W23. Genuine authored lookup first; only an actual
// complete-source miss takes the original six-constant Rect fallback.
void CarCollisionBody::Sub_001D4A80(std::uint32_t car,const ::port::race::CollisionResources& resources){
    auto& m=memory_;if(!resources.source_data_loaded)hp2::not_implemented("0x00213C98","complete original vehicle collision sources are not loaded");
    Sub_001D3BE0(GetNextCollisionBodyID(m),CAR_COLLISION_BODY,car+0x520);
    m.word(address_+0x104,0);m.word(address_+0x108,0);m.word(address_+0x10c,0);
    const auto count=m.word(0x311598)+1;m.word(0x311598,count);m.word(address_+0x50,0x2b04f0);
    if(m.signed_word(0x3115bc)<std::int32_t(count))m.word(0x3115bc,count);
    m.word(address_+0x100,car);m.byte(address_+0x13,1);m.word(address_+0x5c,0);m.word(address_+0x60,0);
    m.word(address_+0x104,m.word(0x327cf8));m.word(address_+0x108,m.word(0x327cf8));m.word(address_+0x10c,m.word(0x327cf8));
    SetDebugName("Car"+std::to_string(m.signed_half(car+0xa)));const auto type=m.word(car+0xc);
    const std::array<float,3> cog{{m.scalar(type+0x110),m.scalar(type+0x114),m.scalar(type+0x118)}};
    const auto name=port_collision_volume_name(m,car);const auto hash=hp2::frontend::bStringHash(name.c_str());auto poly=FindPipelinePolyhedron(m,hash,&cog);
    if(!poly){poly=m.allocate(0xaf0);hp2::recovered::RectPolyhedron::Sub_00212EB0(m,poly,m.scalar(0x325ae4),m.scalar(0x325ae8),m.scalar(0x325aec),m.scalar(0x325af0),m.scalar(0x325af4),m.scalar(0x325af8));}
    m.word(address_+8,poly);BuildBoundingVolumes();UpdatePosition();
}
// Debug001D4BD8, E-0099-W23. Shared authored polyhedra outlive each car; only
// a genuine missing authored volume is individually deleted by this destructor.
void CarCollisionBody::Sub_001D4BD8(int in_chrg){
    auto& m=memory_;const auto car=m.word(address_+0x100);m.word(0x311598,m.word(0x311598)-1);m.word(address_+0x50,0x2b04f0);
    const auto name=port_collision_volume_name(m,car);
    if(!FindPipelinePolyhedron(m,hp2::frontend::bStringHash(name.c_str()),nullptr))if(auto poly=m.word(address_+8))hp2::recovered::VCPolyhedron::Sub_001D00E8(m,poly,3);
    Sub_001D3C60(0);if(in_chrg&1)SlotPool(m,m.word(0x32842c)).Free(address_);
}
// Debug001D5B68, E-0099-W23. RigidBody.State+10 and State.X+0 are independently
// confirmed imported Debug layouts; no physics integration is synthesized.
std::uint32_t CarCollisionBody::GetPosition()const{return memory_.word(address_+0x54)==0?
    memory_.word(address_+0x100)+0x60:memory_.word(address_+0x60)+0x10;}
// Debug001D2BF0/001D3480, E-0099-W19. Reached active collision/track/Caffeine
// dependencies retain their exact missing callee addresses.
void CollisionBodyWitness::Sub_001D2BF0(int in_chrg){
    auto& m=memory_;const auto at=address_;m.word(0x311580,m.word(0x311580)-1);
    if(m.word(at+0x68))EndCollision();
    const auto next=m.word(at),previous=m.word(at+4),b1=m.word(at+0x60),b2=m.word(at+0x64);m.word(previous,next);m.word(next+4,previous);
    m.byte(b1+0x10,std::uint8_t(m.byte(b1+0x10)-1));m.byte(b2+0x10,std::uint8_t(m.byte(b2+0x10)-1));
    for(const auto pair:{std::array<std::uint32_t,2>{{b1,b2}},std::array<std::uint32_t,2>{{b2,b1}}}){
        const int index=m.signed_half(pair[0]+0xe)*206+m.signed_half(pair[1]+0xe);const auto cell=m.word(0x328424)+std::uint32_t(index>>3);
        m.byte(cell,std::uint8_t(m.byte(cell)&~(1u<<(std::uint32_t(index)&7))));}
    if(m.byte(b1+0xc)==TRACK_POLYGON_COLLISION_BODY && m.byte(b1+0x10)==0){TrackPolygonCollisionBody(m,b1).Sub_001D54B8(3);m.word(at+0x60,0);}
    if(m.byte(b2+0xc)==TRACK_POLYGON_COLLISION_BODY && m.byte(b2+0x10)==0){TrackPolygonCollisionBody(m,b2).Sub_001D54B8(3);m.word(at+0x64,0);}
    hp2::debug::Sub_001CD8B0(m,at+0x10,0);if(in_chrg&1)SlotPool(m,m.word(0x328430)).Free(at);
}
void EliminateAllWitnesses(::port::driving::Memory& m){while(m.word(0x328400)!=0x328400)CollisionBodyWitness(m,m.word(0x328400)).Sub_001D2BF0(3);}
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): DynamicCollisionBody::Move() [0x002B8658]
// Diagnostic text (not a callable signature): DynamicCollisionBody::MaybeCreateWitnesses() [0x002B8678]
// Diagnostic text (not a callable signature): DynamicCollisionBody::CreateTrackPolygonCollisionBodies() [0x002B86A8]
#include "game_core/indep/inc/CollisionBody.hpp"
#include "game_core/indep/src/copmgr.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/NumberAllocator.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): TrackPolygonCollisionBody::TrackPolygonCollisionBody() [0x002B8720]
#include "game_core/indep/inc/CollisionBody.hpp"
#include "game_core/indep/src/copmgr.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001D7FA0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d7fa0.md.
void CarCollisionBody::Sub_001D7FA0() {
    hp2::not_implemented("0x001D7FA0", "CarCollisionBody::Sub_001D7FA0 (signature and full body unresolved)");
}

// ELF 0x001D8058; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8058.md.
void CarCollisionBody::DoSnapshot() {
    hp2::not_implemented("0x001D8058", "CarCollisionBody::DoSnapshot (signature and full body unresolved)");
}

// ELF 0x001D8138; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8138.md.
void CarCollisionBody::SetupPhysicsPointers() {
    hp2::not_implemented("0x001D8138", "CarCollisionBody::SetupPhysicsPointers (signature and full body unresolved)");
}

// ELF 0x001D8198; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8198.md.
void CarCollisionBody::SetToPhysicsMode() {
    hp2::not_implemented("0x001D8198", "CarCollisionBody::SetToPhysicsMode (signature and full body unresolved)");
}

// ELF 0x001D81F0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d81f0.md.
void CarCollisionBody::Move() {
    hp2::not_implemented("0x001D81F0", "CarCollisionBody::Move (signature and full body unresolved)");
}

// ELF 0x001D8270; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8270.md.
void CarCollisionBody::AreCollisionsEnabled() {
    hp2::not_implemented("0x001D8270", "CarCollisionBody::AreCollisionsEnabled (signature and full body unresolved)");
}

// ELF 0x001D8298; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8298.md.
void CarCollisionBody::GetMassInverse() {
    hp2::not_implemented("0x001D8298", "CarCollisionBody::GetMassInverse (signature and full body unresolved)");
}

// ELF 0x001D82D0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d82d0.md.
void CarCollisionBody::GetInertiaTensorInverse() {
    hp2::not_implemented("0x001D82D0", "CarCollisionBody::GetInertiaTensorInverse (signature and full body unresolved)");
}

// ELF 0x001D8318; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8318.md.
void CarCollisionBody::ApplyImpulse() {
    hp2::not_implemented("0x001D8318", "CarCollisionBody::ApplyImpulse (signature and full body unresolved)");
}

// ELF 0x001D8E58; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8e58.md.
void CarCollisionBody::GetFriction() {
    hp2::not_implemented("0x001D8E58", "CarCollisionBody::GetFriction (signature and full body unresolved)");
}

// ELF 0x001D8E68; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8e68.md.
void CarCollisionBody::GetBounciness() {
    hp2::not_implemented("0x001D8E68", "CarCollisionBody::GetBounciness (signature and full body unresolved)");
}

// ELF 0x001D8E78; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8e78.md.
void CarCollisionBody::GetTerrainType() {
    hp2::not_implemented("0x001D8E78", "CarCollisionBody::GetTerrainType (signature and full body unresolved)");
}

// ELF 0x001D8E80; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8e80.md.
void CarCollisionBody::IsBlinking() {
    hp2::not_implemented("0x001D8E80", "CarCollisionBody::IsBlinking (signature and full body unresolved)");
}

// ELF 0x001D8ED8; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8ed8.md.
void CarCollisionBody::IsMoveable() {
    hp2::not_implemented("0x001D8ED8", "CarCollisionBody::IsMoveable (signature and full body unresolved)");
}

// ELF 0x001D8EE0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8ee0.md.
void CarCollisionBody::ConsiderImpulse() {
    hp2::not_implemented("0x001D8EE0", "CarCollisionBody::ConsiderImpulse (signature and full body unresolved)");
}

// ELF 0x001D8EF0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8ef0.md.
void CarCollisionBody::GetRotation() {
    hp2::not_implemented("0x001D8EF0", "CarCollisionBody::GetRotation (signature and full body unresolved)");
}

// ELF 0x001D8F10; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8f10.md.
void CarCollisionBody::GetVelocity() {
    hp2::not_implemented("0x001D8F10", "CarCollisionBody::GetVelocity (signature and full body unresolved)");
}

// ELF 0x001D8F30; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8f30.md.
void CarCollisionBody::GetPosition() {
    hp2::not_implemented("0x001D8F30", "CarCollisionBody::GetPosition (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/src/collisionbody.hpp"

namespace hp2::recovered {
// ELF 0x001D69B0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d69b0.md.
void CollisionBody::Sub_001D69B0() {
    hp2::not_implemented("0x001D69B0", "CollisionBody::Sub_001D69B0 (signature and full body unresolved)");
}

// ELF 0x001D6A98; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d6a98.md.
void CollisionBody::DoSnapshot() {
    hp2::not_implemented("0x001D6A98", "CollisionBody::DoSnapshot (signature and full body unresolved)");
}

// ELF 0x001D91D0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d91d0.md.
void CollisionBody::IsBlinking() {
    hp2::not_implemented("0x001D91D0", "CollisionBody::IsBlinking (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001D7028; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d7028.md.
void DynamicCollisionBody::Sub_001D7028() {
    hp2::not_implemented("0x001D7028", "DynamicCollisionBody::Sub_001D7028 (signature and full body unresolved)");
}

// ELF 0x001D7050; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d7050.md.
void DynamicCollisionBody::DoSnapshot() {
    hp2::not_implemented("0x001D7050", "DynamicCollisionBody::DoSnapshot (signature and full body unresolved)");
}

// ELF 0x001D7408; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d7408.md.
void DynamicCollisionBody::Interpolate() {
    hp2::not_implemented("0x001D7408", "DynamicCollisionBody::Interpolate (signature and full body unresolved)");
}

// ELF 0x001D7660; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d7660.md.
void DynamicCollisionBody::Backup() {
    hp2::not_implemented("0x001D7660", "DynamicCollisionBody::Backup (signature and full body unresolved)");
}

// ELF 0x001D7710; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d7710.md.
void DynamicCollisionBody::RecomputeAuxilliaries() {
    hp2::not_implemented("0x001D7710", "DynamicCollisionBody::RecomputeAuxilliaries (signature and full body unresolved)");
}

// ELF 0x001D7740; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d7740.md.
void DynamicCollisionBody::GetCurrentVelocities() {
    hp2::not_implemented("0x001D7740", "DynamicCollisionBody::GetCurrentVelocities (signature and full body unresolved)");
}

// ELF 0x001D7760; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d7760.md.
void DynamicCollisionBody::GetTestImpulseVelocities() {
    hp2::not_implemented("0x001D7760", "DynamicCollisionBody::GetTestImpulseVelocities (signature and full body unresolved)");
}

// ELF 0x001D7E08; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d7e08.md.
void DynamicCollisionBody::ChangeInEnergy() {
    hp2::not_implemented("0x001D7E08", "DynamicCollisionBody::ChangeInEnergy (signature and full body unresolved)");
}

// ELF 0x001D8F98; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8f98.md.
void DynamicCollisionBody::ApplyImpulse() {
    hp2::not_implemented("0x001D8F98", "DynamicCollisionBody::ApplyImpulse (signature and full body unresolved)");
}

// ELF 0x001D8FA0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8fa0.md.
void DynamicCollisionBody::GetInertiaTensorInverse() {
    hp2::not_implemented("0x001D8FA0", "DynamicCollisionBody::GetInertiaTensorInverse (signature and full body unresolved)");
}

// ELF 0x001D8FB0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8fb0.md.
void DynamicCollisionBody::GetMassInverse() {
    hp2::not_implemented("0x001D8FB0", "DynamicCollisionBody::GetMassInverse (signature and full body unresolved)");
}

// ELF 0x001D8FC0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8fc0.md.
void DynamicCollisionBody::SetToPhysicsMode() {
    hp2::not_implemented("0x001D8FC0", "DynamicCollisionBody::SetToPhysicsMode (signature and full body unresolved)");
}

// ELF 0x001D8FC8; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8fc8.md.
void DynamicCollisionBody::GetMaxBackupTime() {
    hp2::not_implemented("0x001D8FC8", "DynamicCollisionBody::GetMaxBackupTime (signature and full body unresolved)");
}

// ELF 0x001D8FD0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8fd0.md.
void DynamicCollisionBody::SetPosition() {
    hp2::not_implemented("0x001D8FD0", "DynamicCollisionBody::SetPosition (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001D5BF8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d5bf8.md.
void NumberAllocator::AllocateNumber() {
    hp2::not_implemented("0x001D5BF8", "NumberAllocator::AllocateNumber (signature and full body unresolved)");
}

// ELF 0x001D5CB8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d5cb8.md.
void NumberAllocator::FreeNumber() {
    hp2::not_implemented("0x001D5CB8", "NumberAllocator::FreeNumber (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001D9048; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9048.md.
void StaticCollisionBody::ChangeInEnergy() {
    hp2::not_implemented("0x001D9048", "StaticCollisionBody::ChangeInEnergy (signature and full body unresolved)");
}

// ELF 0x001D9058; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9058.md.
void StaticCollisionBody::IsMoveable() {
    hp2::not_implemented("0x001D9058", "StaticCollisionBody::IsMoveable (signature and full body unresolved)");
}

// ELF 0x001D9060; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9060.md.
void StaticCollisionBody::AreCollisionsEnabled() {
    hp2::not_implemented("0x001D9060", "StaticCollisionBody::AreCollisionsEnabled (signature and full body unresolved)");
}

// ELF 0x001D9070; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9070.md.
void StaticCollisionBody::ApplyImpulse() {
    hp2::not_implemented("0x001D9070", "StaticCollisionBody::ApplyImpulse (signature and full body unresolved)");
}

// ELF 0x001D9078; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9078.md.
void StaticCollisionBody::ConsiderImpulse() {
    hp2::not_implemented("0x001D9078", "StaticCollisionBody::ConsiderImpulse (signature and full body unresolved)");
}

// ELF 0x001D9088; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9088.md.
void StaticCollisionBody::SetToPhysicsMode() {
    hp2::not_implemented("0x001D9088", "StaticCollisionBody::SetToPhysicsMode (signature and full body unresolved)");
}

// ELF 0x001D9090; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9090.md.
void StaticCollisionBody::GetFriction() {
    hp2::not_implemented("0x001D9090", "StaticCollisionBody::GetFriction (signature and full body unresolved)");
}

// ELF 0x001D90A0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d90a0.md.
void StaticCollisionBody::GetBounciness() {
    hp2::not_implemented("0x001D90A0", "StaticCollisionBody::GetBounciness (signature and full body unresolved)");
}

// ELF 0x001D90B0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d90b0.md.
void StaticCollisionBody::GetInertiaTensorInverse() {
    hp2::not_implemented("0x001D90B0", "StaticCollisionBody::GetInertiaTensorInverse (signature and full body unresolved)");
}

// ELF 0x001D90C0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d90c0.md.
void StaticCollisionBody::GetMassInverse() {
    hp2::not_implemented("0x001D90C0", "StaticCollisionBody::GetMassInverse (signature and full body unresolved)");
}

// ELF 0x001D90D0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d90d0.md.
void StaticCollisionBody::GetTestImpulseVelocities() {
    hp2::not_implemented("0x001D90D0", "StaticCollisionBody::GetTestImpulseVelocities (signature and full body unresolved)");
}

// ELF 0x001D90F0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d90f0.md.
void StaticCollisionBody::GetCurrentVelocities() {
    hp2::not_implemented("0x001D90F0", "StaticCollisionBody::GetCurrentVelocities (signature and full body unresolved)");
}

// ELF 0x001D9110; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9110.md.
void StaticCollisionBody::RecomputeAuxilliaries() {
    hp2::not_implemented("0x001D9110", "StaticCollisionBody::RecomputeAuxilliaries (signature and full body unresolved)");
}

// ELF 0x001D9118; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9118.md.
void StaticCollisionBody::Interpolate() {
    hp2::not_implemented("0x001D9118", "StaticCollisionBody::Interpolate (signature and full body unresolved)");
}

// ELF 0x001D9158; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9158.md.
void StaticCollisionBody::Backup() {
    hp2::not_implemented("0x001D9158", "StaticCollisionBody::Backup (signature and full body unresolved)");
}

// ELF 0x001D9160; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9160.md.
void StaticCollisionBody::Move() {
    hp2::not_implemented("0x001D9160", "StaticCollisionBody::Move (signature and full body unresolved)");
}

// ELF 0x001D9168; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9168.md.
void StaticCollisionBody::GetMaxBackupTime() {
    hp2::not_implemented("0x001D9168", "StaticCollisionBody::GetMaxBackupTime (signature and full body unresolved)");
}

// ELF 0x001D9178; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9178.md.
void StaticCollisionBody::SetPosition() {
    hp2::not_implemented("0x001D9178", "StaticCollisionBody::SetPosition (signature and full body unresolved)");
}

// ELF 0x001D9180; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9180.md.
void StaticCollisionBody::GetRotation() {
    hp2::not_implemented("0x001D9180", "StaticCollisionBody::GetRotation (signature and full body unresolved)");
}

// ELF 0x001D9190; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9190.md.
void StaticCollisionBody::GetVelocity() {
    hp2::not_implemented("0x001D9190", "StaticCollisionBody::GetVelocity (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001D8880; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8880.md.
void TrackPolygonCollisionBody::Sub_001D8880() {
    hp2::not_implemented("0x001D8880", "TrackPolygonCollisionBody::Sub_001D8880 (signature and full body unresolved)");
}

// ELF 0x001D8918; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8918.md.
void TrackPolygonCollisionBody::DoSnapshot() {
    hp2::not_implemented("0x001D8918", "TrackPolygonCollisionBody::DoSnapshot (signature and full body unresolved)");
}

// ELF 0x001D8B18; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8b18.md.
void TrackPolygonCollisionBody::GetPosition() {
    hp2::not_implemented("0x001D8B18", "TrackPolygonCollisionBody::GetPosition (signature and full body unresolved)");
}

// ELF 0x001D8B28; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8b28.md.
void TrackPolygonCollisionBody::GetTerrainType() {
    hp2::not_implemented("0x001D8B28", "TrackPolygonCollisionBody::GetTerrainType (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include <cmath>
// TASK-0048 E-0048-T02. Debug NFSHP2.ELF SHA-256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
namespace hp2::recovered {
// Debug 0x001D4920; original DynamicCollisionBody::TrackPolygonCheck.
bool DynamicCollisionBody::TrackPolygonCheck(const port::track::CollisionCylinder& body,const port::track::Polygon& polygon){
    const auto& n=polygon.plane;
    const float axis_dot=(body.axis[0]*n[0]+body.axis[1]*n[1])+body.axis[2]*n[2];
    const float radial=std::sqrt(1.0f-axis_dot*axis_dot);
    const float distance=((body.center[0]*n[0]+body.center[1]*n[1])+body.center[2]*n[2])-n[3];
    const float height=axis_dot*distance<0?body.top_height:body.base_height;
    return std::abs(distance)<std::abs(axis_dot)*height+radial*body.radius;
}
}

#include "game_core/indep/src/rigidbody.hpp"
#include <cmath>
namespace hp2::recovered {
// Debug001D4ED0; E-0048-D04.
float CarCollisionBody::GetMassInverse(port::driving::Memory& m,std::uint32_t body){return body?std::min(m.scalar(body+0x11c),m.scalar(0x00325afc)):0;}
// Debug001D4F08; E-0048-D04. Original scales I_body_inv rows0..2 and copies row3.
std::array<float,16> CarCollisionBody::GetInertiaTensorInverse(port::driving::Memory& m,std::uint32_t body){auto result=m.matrix(body+0x120);for(unsigned i=0;i<12;++i)result[i]*=m.scalar(0x00325b00);return result;}
// Debug001D4F50; E-0048-D04. Native playerless/no-damage fixture branch of side effects only.
void CarCollisionBody::ApplyImpulse(port::driving::Memory& m,std::uint32_t body,std::uint32_t car,const std::array<float,4>& impulse,const std::array<float,4>& arm){
    float factor=1;const float mass_inverse=m.scalar(body+0x11c);if(m.scalar(0x00325b04)<mass_inverse)factor=m.scalar(0x00325b04)/mass_inverse;
    auto scaled=impulse;for(auto& f:scaled)f*=factor;
    RigidBody(m,body).ApplyImpulseWithScaledInverseInertiaTensor(scaled,arm,m.scalar(0x00325b08));
    // Debug001D5008 AddDamage and001D5188 AnimateCollisionReaction: exact null branches.
    if(m.word(car+0x844)||m.word(car+0x18)||m.word(0x00328c1c))hp2::not_implemented("0x001D5008","collision damage/player/police effects are outside the native playerless fixture");
    if(m.word(car+0x4b4))hp2::not_implemented("0x001D5188","animated collision reaction is outside the native playerless fixture");
}
}
#include "game_core/port/driving/static_collisions.hpp"
// TASK-0048 E-0048-T06. Debug original backup/broad-phase member bindings.
namespace hp2::recovered {
// Debug001D3D90. Original cylinder extent construction from the loaded collision vertices.
port::track::CollisionCylinder DynamicCollisionBody::BuildBoundingVolumes(port::driving::Memory& m,const port::track::VoronoiPolyhedron& shape){
    port::track::CollisionCylinder cylinder;float radius=0,low=0,high=0;
    for(const auto& v:shape.vertices){const float radial=std::sqrt(v.position[0]*v.position[0]+v.position[1]*v.position[1]);if(radius<radial)radius=radial;
        if(high<v.position[2])high=v.position[2];else if(v.position[2]<low)low=v.position[2];}
    const float margin=m.scalar(0x00325ad8);cylinder.radius=radius+margin;cylinder.base_height=margin-low;cylinder.top_height=high+margin;return cylinder;
}
// Debug001D3FA0.
port::driving::CollisionMotion DynamicCollisionBody::BeginMove(const port::driving::Memory& m,std::uint32_t body,float timestep){
    port::driving::CollisionMotion frame;frame.previous=port::driving::collision_pose(m,body);const auto v=m.vector(body+0x90);frame.velocity={{v[0],v[1],v[2]}};frame.timestep=timestep;return frame;
}
// Debug001D4040. Rotation is interpolated componentwise, without normalization.
port::track::VoronoiPose DynamicCollisionBody::Interpolate(const port::driving::Memory& m,std::uint32_t body,const port::driving::CollisionMotion& frame,float backup){
    const auto current=port::driving::collision_pose(m,body);if(frame.timestep<m.scalar(0x00325adc))return current;
    const float fraction=(frame.timestep-backup)/frame.timestep;auto result=frame.previous;
    for(unsigned j=0;j<3;++j){result.position[j]+=(current.position[j]-result.position[j])*fraction;for(unsigned i=0;i<3;++i)result.basis[i][j]+=(current.basis[i][j]-result.basis[i][j])*fraction;}return result;
}
// Debug001D4298/001D41E8. Only X/R/P are restored; angular momentum remains current.
void DynamicCollisionBody::Backup(port::driving::Memory& m,std::uint32_t body,const port::driving::CollisionMotion& frame,float backup){
    const auto pose=Interpolate(m,body,frame,backup);const auto current=m.vector(body+0x90);std::array<float,4> velocity=current;
    if(!(frame.timestep<m.scalar(0x00325ae0))){const float fraction=(frame.timestep-backup)/frame.timestep;for(unsigned j=0;j<3;++j)velocity[j]=frame.velocity[j]+(current[j]-frame.velocity[j])*fraction;}
    auto position=m.vector(body+0x10);auto matrix=m.matrix(body+0x20);for(unsigned j=0;j<3;++j){position[j]=pose.position[j];for(unsigned i=0;i<3;++i)matrix[i*4+j]=pose.basis[i][j];}
    m.vector(body+0x10,position);m.matrix(body+0x20,matrix);for(unsigned j=0;j<4;++j)velocity[j]*=m.scalar(body+0x118);m.vector(body+0x60,velocity);
}
}

// Debug001D4D70, TASK-0099 E-0099-W33. Original collisionbody.cpp.
namespace hp2::debug {
void CarCollisionBody::SetupPhysicsPointers(){
 auto& m=memory_;m.word(address_+0x5c,0);m.word(address_+0x60,0);
 if(!m.word(address_+0x54))return;
 const auto car=m.word(address_+0x100),mode=m.word(car+0x24);
 if(mode!=1&&mode!=4)return;
 const auto mover=m.word(car+0x30);
 if(!mover)hp2::not_implemented("0x001D4D70","Original physics movement requires an existing CarMover");
 m.word(address_+0x5c,mover+8);m.word(address_+0x60,m.word(mover+0x18));
}
}

#include "game_core/indep/inc/Car.hpp"
namespace hp2::debug {
// Debug001D4DD0, E-0099-W36. A car hitter owns its Car pointer at+100.
void CarCollisionBody::SetToPhysicsMode(std::uint32_t hitter,::port::race::PhysicsConstructionServices& services){
 const auto collision=hitter&&memory_.byte(hitter+0xc)==0?memory_.word(hitter+0x100):0;
 hp2::recovered::Car(memory_,memory_.word(address_+0x100)).SetToPhysicsMode(collision,services);
 SetupPhysicsPointers();memory_.byte(address_+0x13,1);
}
}

// TASK-0099 E-0099-W38. Exact shared collision witness lifecycle leaves.
namespace port::race::detail {
// Port virtual dispatch binding; targets/adjustments are read from actual vtables.
int shared_collision_enabled(::port::driving::Memory& m,std::uint32_t body){
 const auto table=m.word(body+0x50),target=m.word(table+0x8c),adjusted=body+static_cast<std::int32_t>(m.signed_half(table+0x88));
 switch(target){case 0x1d4ea8:return hp2::debug::CarCollisionBody(m,adjusted).AreCollisionsEnabled();
  case 0x1d5c98:return hp2::debug::StaticCollisionBody(m,adjusted).AreCollisionsEnabled();
  case 0x21c8b0:return hp2::debug::Smokeable(m,adjusted).AreCollisionsEnabled();
  default:hp2::not_implemented("0x001D30F0","Unrecovered collision-enable virtual target");}
}
}
namespace hp2::debug {
// Debug001D4EA8, E-0099-W38. Original camera/AI tuning gates precede disabled byte.
int CarCollisionBody::AreCollisionsEnabled()const{return !memory_.word(0x32711c)&&!memory_.word(0x328e08)&&!memory_.byte(address_+0x11);}
// Debug001D5C98/001D5DB8, E-0099-W38; inline CollisionBody.hpp.
int StaticCollisionBody::AreCollisionsEnabled()const{return memory_.byte(address_+0x11)==0;}
std::uint32_t StaticCollisionBody::GetRotation()const{return 0x3115e0;}
// Debug001D5B28, E-0099-W38; inline CollisionBody.hpp, both movement families.
std::uint32_t CarCollisionBody::GetRotation()const{return memory_.word(address_+0x54)?memory_.word(address_+0x60)+0x20:memory_.word(address_+0x100)+0x90;}
// Debug001D2AC8, E-0099-W38. Original list publication, symmetric bitfield and stats.
void CollisionBodyWitness::Sub_001D2AC8(std::uint32_t b1,std::uint32_t b2){
 auto& m=memory_;const auto p=address_;Sub_001CD860(m,p+0x10,m.word(b1+8),m.word(b2+8));
 m.word(0x311580,m.word(0x311580)+1);if(m.signed_word(0x3115a4)<m.signed_word(0x311580))m.word(0x3115a4,m.word(0x311580));
 const auto tail=m.word(0x328404);m.word(p+0x60,b1);m.word(tail,p);m.word(0x328404,p);m.word(p+0x64,b2);m.word(p+4,tail);m.word(p,0x328400);m.word(p+0x68,0);
 for(const auto pair:{std::array<std::uint32_t,2>{{b1,b2}},std::array<std::uint32_t,2>{{b2,b1}}}){
  const int a=m.signed_half(pair[0]+0xe),b=m.signed_half(pair[1]+0xe);if(a<0||a>=206||b<0||b>=206)throw std::out_of_range("Original witness requires allocated206-body IDs");
  const auto index=static_cast<std::uint32_t>(a*206+b),cell=m.word(0x328424)+(index>>3);m.byte(cell,static_cast<std::uint8_t>(m.byte(cell)|(1u<<(index&7))));
 }
 m.byte(b1+0x10,static_cast<std::uint8_t>(m.byte(b1+0x10)+1));m.byte(b2+0x10,static_cast<std::uint8_t>(m.byte(b2+0x10)+1));
}
// Debug001D34D0, E-0099-W38. Negative IDs return0 without reading the bitfield.
int CollisionBodyWitness::DoesWitnessExist(const ::port::driving::Memory& m,std::uint32_t b1,std::uint32_t b2){
 const int a=m.signed_half(b1+0xe),b=m.signed_half(b2+0xe);if(a<0||b<0)return 0;if(a>=206||b>=206)throw std::out_of_range("Original witness ID exceeds206-body table");
 const auto index=static_cast<std::uint32_t>(a*206+b);return m.byte(m.word(0x328424)+(index>>3))&(1u<<(index&7));
}
// Debug001D30F0, E-0099-W38. Active Collision keeps witness; XY overlap is inclusive.
int CollisionBodyWitness::HasExpired()const{
 auto& m=memory_;const auto p=address_;if(m.word(p+0x68))return 0;const auto a=m.word(p+0x60),b=m.word(p+0x64);
 if(!(m.scalar(a+0x14)<=m.scalar(b+0x1c)&&m.scalar(b+0x14)<=m.scalar(a+0x1c)&&m.scalar(a+0x18)<=m.scalar(b+0x20)&&m.scalar(b+0x18)<=m.scalar(a+0x20)))return 1;
 if(!m.byte(a+0x13)&&!m.byte(b+0x13))return 1;
 if(!::port::race::detail::shared_collision_enabled(m,a))return 1;
 return ::port::race::detail::shared_collision_enabled(m,b)?0:1;
}
}

namespace port::race::detail {
// Port binding of original pointer-returning collision virtuals, with exact this delta.
std::uint32_t shared_collision_pointer(::port::driving::Memory& m,std::uint32_t body,unsigned slot){
 const auto table=m.word(body+0x50),target=m.word(table+slot+4),adjusted=body+static_cast<std::int32_t>(m.signed_half(table+slot));
 switch(target){
 case 0x1d5b68:return hp2::debug::CarCollisionBody(m,adjusted).GetPosition();
 case 0x1d5b28:return hp2::debug::CarCollisionBody(m,adjusted).GetRotation();
 case 0x1d5b48:return hp2::debug::CarCollisionBody(m,adjusted).GetVelocity();
 case 0x1d5750:return hp2::debug::TrackPolygonCollisionBody(m,adjusted).GetPosition();
 case 0x1d5db8:return hp2::debug::StaticCollisionBody(m,adjusted).GetRotation();
 case 0x1d5dc8:return hp2::debug::StaticCollisionBody(m,adjusted).GetVelocity();
 // E-0099-W116: actual Smokeable.hpp pointer leaves, same signed descriptor delta.
 case 0x21c908:return hp2::debug::Smokeable(m,adjusted).GetRotation();
 case 0x21c918:return hp2::debug::Smokeable(m,adjusted).GetVelocity();
 case 0x21c928:return hp2::debug::Smokeable(m,adjusted).GetPosition();
 default:hp2::not_implemented("0x001D3FA0","Unrecovered collision pose virtual target");
 }
}
}
namespace hp2::debug {
// Debug001D5B48/001D5DC8/001D5C00/001D5DA0; E-0099-W38, CollisionBody.hpp.
std::uint32_t CarCollisionBody::GetVelocity()const{return memory_.word(address_+0x54)?memory_.word(address_+0x60)+0x90:memory_.word(address_+0x100)+0x70;}
std::uint32_t StaticCollisionBody::GetVelocity()const{return 0x3115d0;}
float DynamicCollisionBody::GetMaxBackupTime()const{return memory_.scalar(address_+0x64);}
float StaticCollisionBody::GetMaxBackupTime()const{return 1000;}
// Debug001D3FA0; E-0099-W38, original collisionbody.cpp. Whole16-lane matrix snapshot.
void DynamicCollisionBody::BeginMove(float dt){
 auto& m=memory_;const auto p=address_;
 const auto rotation=::port::race::detail::shared_collision_pointer(m,p,0x28);m.copy(p+0x70,rotation,64);
 const auto position=::port::race::detail::shared_collision_pointer(m,p,0x18);m.copy(p+0xb0,position,16);
 const auto velocity=::port::race::detail::shared_collision_pointer(m,p,0x20);m.copy(p+0xc0,velocity,16);m.scalar(p+0x64,dt);
}
// TASK0100 port diagnostic: observe the first native nonfinite producer;
// this does not saturate, accept bad inputs or alter original arithmetic.
} // hp2::debug
namespace port::race::detail {
float inspect_collision_interpolation(float result,float left,float right,std::uint32_t body,unsigned lane,const char* stage,const char* pc,float time,float backup){if(!std::isfinite(result)||!std::isfinite(left)||!std::isfinite(right)){const auto bits=[](float v){std::uint32_t word;std::memcpy(&word,&v,4);return std::to_string(word);};const std::string why=std::string("Collision interpolation producer stage=")+stage+" body="+std::to_string(body)+" lane="+std::to_string(lane)+" left_bits="+bits(left)+" right_bits="+bits(right)+" result_bits="+bits(result)+" dt_bits="+bits(time)+" backup_bits="+bits(backup);hp2::not_implemented(pc,why.c_str());}return result;}
}
namespace hp2::debug {
// Debug001D4040; E-0099-W38. Elementwise interpolation; no clamping/orthonormalization.
void DynamicCollisionBody::Interpolate(std::uint32_t pos,std::uint32_t mat,float backup){
 auto& m=memory_;const auto p=address_;
 const auto current_position=::port::race::detail::shared_collision_pointer(m,p,0x18),current_rotation=::port::race::detail::shared_collision_pointer(m,p,0x28);
 const float dt=m.scalar(p+0x64);
 if(dt<m.scalar(0x325adc)){m.copy(pos,current_position,16);m.copy(mat,current_rotation,64);return;}
 auto position=m.vector(current_position);const auto previous=m.vector(p+0xb0);auto rotation=m.matrix(current_rotation);const auto old=m.matrix(p+0x70);const float fraction=(dt-backup)/dt;
 for(unsigned i=0;i<4;++i){const float difference=::port::race::detail::inspect_collision_interpolation(position[i]-previous[i],position[i],previous[i],p,i,"position-sub","0x001D40F0",dt,backup);const float product=::port::race::detail::inspect_collision_interpolation(difference*fraction,difference,fraction,p,i,"position-mul","0x001D4154",dt,backup);position[i]=::port::race::detail::inspect_collision_interpolation(product+previous[i],product,previous[i],p,i,"position-add","0x001D415C",dt,backup);}m.vector(pos,position);
 for(unsigned i=0;i<16;++i)rotation[i]=old[i]+(rotation[i]-old[i])*fraction;m.matrix(mat,rotation);
}
}
#include "game_core/indep/src/track.hpp"
namespace hp2::debug {
// Debug001D53A0/001D54B8, E-0099-W38. Actual embedded wafer and topology lease.
void TrackPolygonCollisionBody::Sub_001D53A0(int id,std::uint32_t source){
 auto& m=memory_;const auto p=address_;
 Sub_001D5310(id,TRACK_POLYGON_COLLISION_BODY);m.word(p+0x50,0x2b0690);
 hp2::recovered::VCPolyhedron::Sub_001CFFA8(m,p+0x58);
 m.word(0x311594,m.word(0x311594)+1);if(m.signed_word(0x3115b8)<m.signed_word(0x311594))m.word(0x3115b8,m.word(0x311594));m.word(p+8,p+0x58);
 const auto first=m.word(0x2f0d20);if(source<first||(source-first)%32)throw std::out_of_range("Original topology polygon index");
 SetDebugName("TrackPolygon"+std::to_string((source-first)/32));
 const auto tp=TopologyTree(m,0x2f0d10).AssignTrackPolygon(source);m.word(p+0x54,tp);m.word(tp+0x10,p);
 hp2::recovered::WaferPolyhedron::Init(m,p+0x58,tp,{{0,0,0}});
 SetBBoxMinMax({{m.scalar(tp+0x14),m.scalar(tp+0x18)}},{{m.scalar(tp+0x1c),m.scalar(tp+0x20)}});
 // PlotToCaffeine001D3A48 is instruction-confirmed empty.
}
void TrackPolygonCollisionBody::Sub_001D54B8(int flags){
 auto& m=memory_;const auto p=address_;m.word(p+0x50,0x2b0690);const auto tp=m.word(p+0x54);m.word(tp+0x10,0);
 TopologyTree(m,0x2f0d10).UnAssignTrackPolygon(tp);m.word(0x311594,m.word(0x311594)-1);
 hp2::recovered::VCPolyhedron::Sub_001D00E8(m,p+0x58,0);Sub_001D5370(0);if(flags&1)SlotPool(m,m.word(0x328428)).Free(p);
}
// Debug001D5750/001D5760, inline CollisionBody.hpp; E-0099-W38.
std::uint32_t TrackPolygonCollisionBody::GetPosition()const{return 0x3115d0;}
int TrackPolygonCollisionBody::GetTerrainType()const{return memory_.signed_byte(memory_.word(address_+0x54)+0xb);}
// Debug001D4920, E-0099-W38. Actual body rotation axis and signed plane side.
int DynamicCollisionBody::TrackPolygonCheck(std::uint32_t tp){
 auto& m=memory_;const auto rotation=::port::race::detail::shared_collision_pointer(m,address_,0x28);
 const std::array<float,3> axis{{m.scalar(rotation+0x20),m.scalar(rotation+0x24),m.scalar(rotation+0x28)}},plane{{m.scalar(tp+0x70),m.scalar(tp+0x74),m.scalar(tp+0x78)}};
 const auto bits=[](float value){std::uint32_t word;std::memcpy(&word,&value,4);return std::to_string(word);};
 for(unsigned i=0;i<3;++i)if(!std::isfinite(axis[i])||!std::isfinite(plane[i])){const std::string why="Track polygon dot input body="+std::to_string(address_)+" rotation="+std::to_string(rotation)+" polygon="+std::to_string(tp)+" lane="+std::to_string(i)+" axis_bits="+bits(axis[i])+" plane_bits="+bits(plane[i]);hp2::not_implemented("0x001D496C",why.c_str());}
 const float raw_dot=(axis[0]*plane[0]+axis[1]*plane[1])+axis[2]*plane[2],raw_square=1.f-raw_dot*raw_dot;
 if(!std::isfinite(raw_square)){const std::string why="Track polygon native radicand producer body="+std::to_string(address_)+" polygon="+std::to_string(tp)+" dot_bits="+bits(raw_dot)+" square_bits="+bits(raw_square);::port::diagnostics::warn("0x001D4990",why,"captured before source VMUL/ADD/MUL.S/SUB.S finite carriers; strict mode still rejects native-range overflow");}
 // E-0100-TP01: exact1D496C/4978/497C VU dot,4990 MUL.S,4994 SUB.S.
 const float x=::port::driving::ee_vmul_finite(axis[0],plane[0],"0x001D496C"),y=::port::driving::ee_vmul_finite(axis[1],plane[1],"0x001D496C"),z=::port::driving::ee_vmul_finite(axis[2],plane[2],"0x001D496C");
 float dot=::port::driving::ee_vadd_finite(::port::driving::ee_vadd_finite(x,y,"0x001D4978"),z,"0x001D497C");
 const float square=::port::driving::ee_sub_s_finite(1.f,::port::driving::ee_mul_s_finite(dot,dot,"0x001D4990"),"0x001D4994");
 // E-0099-W115: caller-only Boolean consumer, never a numerical SQRT value.
 // Original1D5B68 is loads/branches/pointer-add/return only in both modes;
 // no stores/FCR31 reads. Only that proved descriptor can be inspected early.
 // FCR31 and cross-function CPU/VU state are outside this native model.
 std::uint32_t square_bits;std::memcpy(&square_bits,&square,4);
 if(square_bits==0xb5000000u){
  const auto table=m.word(address_+0x50);
  if(m.word(table+0x1c)==0x1d5b68&&m.signed_half(table+0x18)==0){
   const auto position=::port::race::detail::shared_collision_pointer(m,address_,0x18);
   const float distance=((m.scalar(position)*m.scalar(tp+0x70)+m.scalar(position+4)*m.scalar(tp+0x74))+m.scalar(position+8)*m.scalar(tp+0x78))-m.scalar(tp+0x7c);
   const auto height_offset=dot*distance<0?0xd4:0xd0;
   const float radius=m.scalar(address_+0xd8),absolute_dot=std::fabs(dot),absolute_distance=std::fabs(distance);
   // sqrt(2^-21) lies in the broad normal dyadic bin(2^-11,2^-10).
   // Base<=5/16, transverse term<=1/256: a loose final RHS<3/8<1/2
   // leaves ample margin for unmodelled normal post-op last-bit differences.
   // This predicate is conditional on these source consumer operands, not on
   // original hardware reproducing a particular native World frame/tuple.
   if(std::isnormal(dot)&&absolute_dot>=1&&absolute_dot<=1.25f&&
      std::isnormal(distance)&&absolute_distance>=.5f&&absolute_distance<=0x1p40f&&
      m.word(address_+height_offset)==0x3e800000u&&
      !std::signbit(radius)&&(radius==0||std::isnormal(radius))&&radius<=4)
    return 0;
  }
 }
 // E-0099-W54: actual SQRT.S magnitude/sign semantics, not a geometric clamp.
 // This recovered body does not read FCR31; primitive status is independently tested.
 const float transverse=::port::driving::ee_sqrt_s(square,false,"0x001D4998").value;
 const auto position=::port::race::detail::shared_collision_pointer(m,address_,0x18);
 const float distance=((m.scalar(position)*m.scalar(tp+0x70)+m.scalar(position+4)*m.scalar(tp+0x74))+m.scalar(position+8)*m.scalar(tp+0x78))-m.scalar(tp+0x7c);
 const float height=m.scalar(address_+(dot*distance<0?0xd4:0xd0));return std::fabs(distance)<std::fabs(dot)*height+transverse*m.scalar(address_+0xd8);
}
// Debug001D46D8, E-0099-W38. Preserve first48 selection before raw-address sort.
int DynamicCollisionBody::CreateTrackPolygonCollisionBodies(){
 auto& m=memory_;std::array<std::uint32_t,48> found{};unsigned count=0;const auto tc=m.word(address_+0x58);const int n=m.signed_word(tc+0x2a0);
 for(int i=0;i<n;++i){const auto tp=m.word(tc+0x20+4*std::uint32_t(i));if(m.word(tp+0x10))continue;
  if(m.scalar(address_+0x14)<=m.scalar(tp+0x1c)&&m.scalar(tp+0x14)<=m.scalar(address_+0x1c)&&m.scalar(address_+0x18)<=m.scalar(tp+0x20)&&m.scalar(tp+0x18)<=m.scalar(address_+0x20))if(TrackPolygonCheck(tp)&&count<48)found[count++]=tp;
 }
 bool changed;do{changed=false;for(unsigned i=1;i<count;++i)if(m.word(found[i]+0xc)<m.word(found[i-1]+0xc)){std::swap(found[i],found[i-1]);changed=true;}}while(changed);
 for(unsigned i=0;i<count;++i){const auto pool=m.word(0x328428);if(m.word(pool+0x18)==m.word(pool+0x2c))return static_cast<int>(i);
  const auto source=m.word(found[i]+0xc);const auto id=GetNextCollisionBodyID(m);const auto body=bOMalloc(m,pool);TrackPolygonCollisionBody(m,body).Sub_001D53A0(id,source);
  // DoesWitnessExist's diagnostic branch calls DebugPrint001D3A98, confirmed empty.
  (void)CollisionBodyWitness::DoesWitnessExist(m,address_,body);
 }return static_cast<int>(count);
}
// Debug001D4468, E-0099-W38. Saved successor survives original object conversion.
void DynamicCollisionBody::MaybeCreateWitnesses(const ::port::race::PhysicsConstructionServices* services){
 auto& m=memory_;const auto self=address_;if(!::port::race::detail::shared_collision_enabled(m,self))return;CreateTrackPolygonCollisionBodies();
 auto before=m.word(self+4);const float minimum=m.scalar(self+0x14);if(before!=0x3283d8&&!(m.scalar(before+0x1c)<minimum)){before=m.word(before+4);while(before!=0x3283d8&&!(m.scalar(before+0x1c)<minimum))before=m.word(before+4);}
 auto end=m.word(self);const float limit=m.scalar(self+0x1c)+m.scalar(0x3283e0);while(end!=0x3283d8&&m.scalar(end+0x1c)<=limit)end=m.word(end);
 auto body=m.word(before==0x3283d8?0x3283d8:before);
 const auto overlap=[&](std::uint32_t b){return m.scalar(self+0x14)<=m.scalar(b+0x1c)&&m.scalar(b+0x14)<=m.scalar(self+0x1c)&&m.scalar(self+0x18)<=m.scalar(b+0x20)&&m.scalar(b+0x18)<=m.scalar(self+0x20);};
 while(body!=end){const auto next=m.word(body);if(body!=self&&overlap(body)&&!CollisionBodyWitness::DoesWitnessExist(m,self,body)&&::port::race::detail::shared_collision_enabled(m,body)){
  if(m.byte(body+0xc)==VIRGIN_SMOKEABLE_COLLISION_BODY){
   if(!services)hp2::not_implemented("0x0021A9A8","Virgin witness conversion requires actual physics resources and CPU lane policy");
   body=ConvertToSmokeable(m,body,*services);
  }
  // Source001D4624 checks the conversion return before touching its bounds.
  if(body&&overlap(body)){const auto witness=bOMalloc(m,m.word(0x328430));CollisionBodyWitness(m,witness).Sub_001D2AC8(self,body);}
 }body=next;}
}
}

namespace hp2::debug {
// Debug001D2FB8/001D2FF0/001D3080, E-0099-W38; original collisionbody.cpp.
std::uint32_t CollisionBodyWitness::GetCollision(std::uint32_t point){if(!memory_.word(address_+0x68))BeginCollision(point);return memory_.word(address_+0x68);}
void CollisionBodyWitness::BeginCollision(std::uint32_t point){
 auto& m=memory_;const auto first=m.word(address_+0x60),second=m.word(address_+0x64);m.byte(first+0x12,static_cast<std::uint8_t>(m.byte(first+0x12)+1));m.byte(second+0x12,static_cast<std::uint8_t>(m.byte(second+0x12)+1));
 m.word(0x311584,m.word(0x311584)+1);if(m.signed_word(0x3115a8)<m.signed_word(0x311584))m.word(0x3115a8,m.word(0x311584));
 const auto collision=bOMalloc(m,m.word(0x328b20));Collision(m,collision).Sub_00219230(address_,point);m.word(address_+0x68,collision);
}
void CollisionBodyWitness::EndCollision(){
 auto& m=memory_;if(const auto collision=m.word(address_+0x68))Collision(m,collision).Sub_00219308(3);m.word(address_+0x68,0);m.word(0x311584,m.word(0x311584)-1);
 for(const auto body:{m.word(address_+0x60),m.word(address_+0x64)})m.byte(body+0x12,static_cast<std::uint8_t>(m.byte(body+0x12)-1));
}
}

namespace port::race::detail {void contact_interpolate(::port::driving::Memory&,std::uint32_t,std::uint32_t,std::uint32_t,float);}
namespace hp2::debug {
// Debug001D41E8, E-0099-W38. Full original velocity vector, including W.
void DynamicCollisionBody::InterpolateVelocity(std::uint32_t out,float backup){
 auto& m=memory_;const auto current=::port::race::detail::shared_collision_pointer(m,address_,0x20);const float time=m.scalar(address_+0x64);
 if(time<m.scalar(0x325ae0)){m.copy(out,current,16);return;}auto velocity=m.vector(current);const auto previous=m.vector(address_+0xc0);const float fraction=(time-backup)/time;for(unsigned i=0;i<4;++i){const float difference=::port::race::detail::inspect_collision_interpolation(velocity[i]-previous[i],velocity[i],previous[i],address_,i,"velocity-sub","0x001D425C",time,backup);const float product=::port::race::detail::inspect_collision_interpolation(difference*fraction,difference,fraction,address_,i,"velocity-mul","0x001D4274",time,backup);velocity[i]=::port::race::detail::inspect_collision_interpolation(product+previous[i],product,previous[i],address_,i,"velocity-add","0x001D4278",time,backup);}m.vector(out,velocity);
}
// Debug001D4298, E-0099-W38. Preserve current angular momentum; original writes
// X/R/P only, deriving all4 momentum lanes from interpolated velocity*Mass.
void DynamicCollisionBody::Backup(float backup){
 auto& m=memory_;::port::driving::Memory::Temporary temp(m,96);const auto pos=temp.address(),rotation=pos+16,velocity=rotation+64;
 ::port::race::detail::contact_interpolate(m,address_,pos,rotation,backup);InterpolateVelocity(velocity,backup);const auto body=m.word(address_+0x60);
 auto momentum=m.vector(velocity);for(unsigned lane=0;lane<4;++lane)momentum[lane]=::port::race::detail::inspect_collision_interpolation(momentum[lane]*m.scalar(body+0x118),momentum[lane],m.scalar(body+0x118),address_,lane,"backup-momentum","0x001D42EC",m.scalar(address_+0x64),backup);m.copy(body+0x10,pos,16);m.copy(body+0x20,rotation,64);m.vector(body+0x60,momentum);
}
// Debug001D5C08/001D4378, E-0099-W38. These are actual logical pointers, not
// the historical host car/body projection's guessed object offsets.
void DynamicCollisionBody::SetPosition(std::uint32_t source){const auto destination=::port::race::detail::shared_collision_pointer(memory_,address_,0x18);memory_.copy(destination,source,16);}
void DynamicCollisionBody::GetCurrentVelocities(std::uint32_t velocity,std::uint32_t angular){const auto body=memory_.word(address_+0x60);memory_.copy(velocity,body+0x90,16);memory_.copy(angular,body+0xa0,16);}
}

#include "game_core/port/race/physics_construction.hpp"
#include "game_core/port/race/car_construction.hpp"
#include "game_core/port/race/traffic_construction.hpp"
namespace hp2::debug {
// Debug001D4348, E-0099-W38. Actual system virtual target and signed this delta.
void DynamicCollisionBody::RecomputeAuxilliaries(::port::race::PhysicsConstructionServices& services){
 const auto source_system=memory_.word(address_+0x5c),source_table=memory_.word(source_system+0xc),source_target=memory_.word(source_table+0x14),source_receiver=source_system+static_cast<std::int32_t>(memory_.signed_half(source_table+0x10));
 // E-0110-E03. Original Smokeable PhysicsSystem dispatch needs its own
 // receiver, not a Car-only persistent topology adaptation.
 if(source_target==0x21c300){Smokeable(memory_,source_receiver).ComputeAuxiliaries(services);return;}
 if(memory_.byte(address_+0xc)!=CAR_COLLISION_BODY)hp2::not_implemented("0x001D4348","Unrecovered non-Car PhysicsSystem/contact binding");
 // Port provider adapts THIS original Car's persistent TC, never a caller's
 // player/contact singleton. Primitive clock/padding inputs retain their source.
 const auto car=memory_.word(address_+0x100);::port::race::SharedTopologyContact own_contact(memory_,car+0x520);auto actual=services;actual.contact=&own_contact;
 const auto system=memory_.word(address_+0x5c),table=memory_.word(system+0xc),target=memory_.word(table+0x14),adjusted=system+static_cast<std::int32_t>(memory_.signed_half(table+0x10));
 switch(target){case 0x137748:PhysicsCar(memory_,adjusted).ComputeAuxiliaries(actual);return;case 0x214ae8:MellowMover(memory_,adjusted).ComputeAuxiliaries(actual);return;
 default:hp2::not_implemented("0x001D4348","Unrecovered PhysicsSystem ComputeAuxiliaries virtual target");}
}
}
#include "game_core/indep/inc/Contact.hpp"
#include "game_core/src/bList.hpp"
namespace hp2::debug {
// Debug001D31E8, E-0099-W38. Original signed type switch and TrackPolygon flags.
int IsYawControlInducingCollisionBody(const ::port::driving::Memory& m,std::uint32_t body){const int type=m.signed_byte(body+0xc);if(type>=0&&type<3)return 1;if(type==3)return (m.byte(m.word(body+0x54)+0xa)&2)!=0;return 0;}
// Debug001D5AB8, E-0099-W38. IsPoofed001332C8 is a full-word load, so a
// nonzero upper byte still disables dynamic-dynamic collision generation.
int CarCollisionBody::IsBlinking(){const auto car=memory_.word(address_+0x100);return hp2::recovered::Car(memory_,car).IsBlinking()||memory_.word(car+0x868);}
// Debug001D5BF8; E-0110-E03. Exact original DynamicCollisionBody empty
// implementation (JR RA/NOP), including the inherited Smokeable virtual slot.
void DynamicCollisionBody::SetToPhysicsMode(std::uint32_t){}
}
namespace port::race::detail {
int shared_collision_blinking(::port::driving::Memory& m,std::uint32_t body){const auto table=m.word(body+0x50),target=m.word(table+0xbc),adjusted=body+static_cast<std::int32_t>(m.signed_half(table+0xb8));
 // E-0110-E02: original Smokeable table002B4670 has getter0021C8D0;
 // preserve the virtual signed-halfword receiver adjustment and signed byte.
 switch(target){case 0x1d5ab8:return hp2::debug::CarCollisionBody(m,adjusted).IsBlinking();case 0x1d5e08:return 0;case 0x21c8d0:return hp2::debug::Smokeable(m,adjusted).IsBlinking();
 default:hp2::not_implemented("0x001D3230","Unrecovered IsBlinking collision virtual target");}
}
}
namespace hp2::debug {
// Debug001D3230, E-0099-W38. Actual shared witness query, source timer effects
// and original Contact allocation/sort. No collision response is hidden here.
void CollisionBodyWitness::TestForContact(std::uint32_t contacts,std::optional<std::uint32_t> padding){
 auto& m=memory_;const auto first=m.word(address_+0x60),second=m.word(address_+0x64);
 if(!::port::race::detail::shared_collision_enabled(m,first))return;if(!::port::race::detail::shared_collision_enabled(m,second))return;
 const auto r1=::port::race::detail::shared_collision_pointer(m,first,0x28),p1=::port::race::detail::shared_collision_pointer(m,first,0x18),r2=::port::race::detail::shared_collision_pointer(m,second,0x28),p2=::port::race::detail::shared_collision_pointer(m,second,0x18);
 const auto status=hp2::recovered::VCWitness::VClip(m,address_+0x10,r1,p1,r2,p2,padding);if(m.word(0x328408))m.word(0x328408,0);if(status!=2)return;
 if((::port::race::detail::shared_collision_blinking(m,first)||::port::race::detail::shared_collision_blinking(m,second))&&m.byte(first+0xd)&&m.byte(second+0xd)){
  if(!m.byte(first+0xc))m.word(first+0x108,m.word(0x327cf8));if(!m.byte(second+0xc))m.word(second+0x108,m.word(0x327cf8));return;
 }
 if(!m.byte(first+0xc)){if(IsYawControlInducingCollisionBody(m,second))m.word(first+0x10c,m.word(0x327cf8));m.word(first+0x104,m.word(0x327cf8));}
 if(!m.byte(second+0xc)){if(IsYawControlInducingCollisionBody(m,first))m.word(second+0x10c,m.word(0x327cf8));m.word(second+0x104,m.word(0x327cf8));}
 const auto contact=bOMalloc(m,m.word(0x328394));Contact(m,contact).Sub_001D13C8(first,second,address_,padding);bList(m,contacts).AddSorted(0x1d13a0,contact);
}
}

namespace hp2::debug {
// Debug001D4ED0, E-0099-W38. Retain explicit <= selection including its original
// operand order rather than substituting the host std::min NaN behavior.
float CarCollisionBody::GetMassInverse()const{const auto body=memory_.word(address_+0x60);if(!body)return 0;const float value=memory_.scalar(body+0x11c),maximum=memory_.scalar(0x325afc);return value<=maximum?value:maximum;}
// Debug001D4F08, E-0099-W38. Actual persistent cache, first3 rows scaled and
// fourth copied unchanged; not the historical transient host array result.
std::uint32_t CarCollisionBody::GetInertiaTensorInverse(){const auto body=memory_.word(address_+0x60);auto tensor=memory_.matrix(body+0x120);for(unsigned i=0;i<12;++i)tensor[i]*=memory_.scalar(0x325b00);memory_.matrix(address_+0x110,tensor);return address_+0x110;}
}

#include "game_core/indep/src/rigidbody.hpp"
#include "game_core/indep/inc/AnimatedDriver.hpp"
#include "game_core/indep/src/rumble.hpp"
#include "game_core/port/race/damage_resources.hpp"
namespace hp2::debug {
// Debug001D3B70, E-0099-W38. Original signed-byte count and active-witness scan.
std::uint32_t CollisionBody::GetCollision(int index)const{if(index>=memory_.signed_byte(address_+0x12))return 0;int count=0;for(auto w=memory_.word(0x328400);w!=0x328400;w=memory_.word(w))if(const auto collision=memory_.word(w+0x68))if(memory_.word(w+0x60)==address_||memory_.word(w+0x64)==address_){if(count==index)return collision;++count;}return 0;}
// Debug001D4F50, E-0099-W38. Actual shared rigid body impulse before damage
// and animation side effects, preserving all4 scaled impulse lanes.
void CarCollisionBody::ApplyImpulse(std::uint32_t impulse,std::uint32_t arm){auto& m=memory_;const auto body=m.word(address_+0x60);const float inverse=m.scalar(body+0x11c),limit=m.scalar(0x325b04),scale=limit<inverse?limit/inverse:1;
 auto scaled=m.vector(impulse);for(auto& lane:scaled)lane*=scale;hp2::recovered::RigidBody(m,body).ApplyImpulseWithScaledInverseInertiaTensor(scaled,m.vector(arm),m.scalar(0x325b08));
 ::port::driving::Memory::Temporary temp(m,16);m.vector(temp.address(),scaled);const float magnitude=std::sqrt((scaled[0]*scaled[0]+scaled[1]*scaled[1])+scaled[2]*scaled[2]);AddDamage(arm,magnitude);AnimateCollisionReaction(temp.address(),magnitude);
}
// Debug001D5008, E-0099-W38. Original damage/rumble/police order. Required
// live rumble and police callees are not replaced by port notification events.
void CarCollisionBody::AddDamage(std::uint32_t arm,float magnitude){auto& m=memory_;auto car=m.word(address_+0x100);
 if(const auto damage=m.word(car+0x844)){const auto body=m.word(address_+0x60);::port::driving::SolverMatrix transpose;hp2::recovered::bTransposeMatrix(transpose,m.matrix(body+0x20));::port::driving::SolverVector local;hp2::recovered::bMulMatrix(local,transpose,m.vector(arm),::port::driving::SolverMathContext{m.vector(0x317b20)});::port::driving::Memory::Temporary point(m,16);m.vector(point.address(),local);CarDamage(m,damage).AddDamage(point.address(),magnitude*m.scalar(0x325b0c));car=m.word(address_+0x100);}
 const auto player=m.word(car+0x18),coordinator=m.word(0x327290);if(player&&coordinator){const auto state=m.word(coordinator);if(state!=4&&state!=7&&state!=5&&state!=9&&state-8u>=2)PortRumbleImpact(m,m.signed_word(player+8),magnitude);}
 if(m.word(0x328c1c)&&1<=magnitude){bool two_car=false;for(int i=0;i<m.signed_byte(address_+0x12);++i){two_car=Collision(m,GetCollision(i)).IsTwoCarCollision();if(two_car)break;}tCopManager(m,m.word(0x328c1c)).MonitorImpact(car,magnitude,two_car);}
}
// Debug001D5188, E-0099-W38. Exact axis/tie choices and actual animated-driver
// queue; a player force-impact call remains required at its own original entry.
void CarCollisionBody::AnimateCollisionReaction(std::uint32_t impulse,float magnitude){auto& m=memory_;auto car=m.word(address_+0x100);
 const auto local_impulse=[&](){::port::driving::SolverMatrix transpose;hp2::recovered::bTransposeMatrix(transpose,m.matrix(m.word(address_+0x60)+0x20));::port::driving::SolverVector result;hp2::recovered::bMulMatrix(result,transpose,m.vector(impulse),::port::driving::SolverMathContext{m.vector(0x317b20)});return result;};
 if(const auto driver=m.word(car+0x4b4)){if(4<magnitude){DriverAnimationType type=DRIVER_ANIMATION_TYPE_COLLISION_LIGHT;int priority=2;if(20<magnitude){const auto local=local_impulse();priority=3;if(std::fabs(local[1])<std::fabs(local[0]))type=local[0]>0?DRIVER_ANIMATION_TYPE_COLLISION_BACK:DRIVER_ANIMATION_TYPE_COLLISION_FRONT;else type=local[1]>0?DRIVER_ANIMATION_TYPE_COLLISION_LEFT:DRIVER_ANIMATION_TYPE_COLLISION_RIGHT;}AnimatedDriver(m,driver).BeginCollision(type,priority);}car=m.word(address_+0x100);}
 if(m.word(car+0x18)&&4<magnitude){const auto local=local_impulse();::port::driving::Memory::Temporary impulse_copy(m,16);m.vector(impulse_copy.address(),local);hp2::recovered::Car(m,m.word(address_+0x100)).StartForceImpact(impulse_copy.address(),magnitude);}
}
}

namespace hp2::debug {
// Debug001D4E28, E-0099-W38. Original wrapper retains returned partial time,
// actual virtual promotion and second Car.Move before updating broad-phase box.
void CarCollisionBody::Move(float timestep,::port::race::PhysicsConstructionServices& services){BeginMove(timestep);const auto car=memory_.word(address_+0x100);const float moved=hp2::recovered::Car(memory_,car).Move(timestep,services);
 if(moved<timestep){const auto table=memory_.word(address_+0x50),target=memory_.word(table+0x74),adjusted=address_+static_cast<std::int32_t>(memory_.signed_half(table+0x70));if(target!=0x1d4dd0)hp2::not_implemented("0x001D4E28","Unrecovered collision body SetToPhysicsMode virtual target");CarCollisionBody(memory_,adjusted).SetToPhysicsMode(0,services);hp2::recovered::Car(memory_,car).Move(timestep-moved,services);}UpdatePosition();}
}

#include "game_core/indep/inc/Replay.hpp"
namespace hp2::debug {
// Debug001D36D0, E-0099-W76. Packed byte stores preserve actual source nibble semantics.
void CollisionBody::DoSnapshot(std::uint32_t snapshot){auto& m=memory_;ReplaySnapshot s(m,snapshot);::port::driving::Memory::Temporary temp(m,1);const auto p=temp.address();if(!m.byte(snapshot+0x3d))m.byte(p,std::uint8_t(m.byte(address_+0x11)+(std::uint32_t(m.byte(address_+0x13))<<4)));s.MoveData(p,1,1,0);if(m.byte(snapshot+0x3d)){const auto value=m.byte(p);m.byte(address_+0x11,value&15);m.byte(address_+0x13,value>>4);}s.VerifyData(address_+0xe,2,2,1);s.VerifyData(address_+0xc,1,1,1);s.VerifyData(address_+0xd,1,1,1);}
// Debug001D5398, E-0099-W76. Source inlines exactly this same base snapshot body.
void StaticCollisionBody::DoSnapshot(std::uint32_t snapshot){CollisionBody::DoSnapshot(snapshot);}
// Debug001D3C88, E-0099-W76. Taking snapshots has actual cache/backup side effects.
void DynamicCollisionBody::DoSnapshot(std::uint32_t snapshot){auto& m=memory_;ReplaySnapshot s(m,snapshot);s.BeginSection(address_,0x2b0388);CollisionBody::DoSnapshot(snapshot);s.MoveData(address_+0x54,1,4,1);const bool mutate=m.byte(snapshot+0x3c)||m.byte(snapshot+0x3d);if(mutate)TopologyCoordinate(m,m.word(address_+0x58)).ClearInterestBBox();for(auto offset:{0xd0u,0xd4u,0xd8u})s.VerifyData(address_+offset,4);s.VerifyData(address_+0xe0,12);s.VerifyData(address_+0xf0,12);if(mutate){std::memset(m.view(address_+0x64,4).data,0,4);std::memset(m.view(address_+0x70,64).data,0,64);std::memset(m.view(address_+0xb0,16).data,0,16);std::memset(m.view(address_+0xc0,16).data,0,16);}s.EndSection();}
// Debug001D4C90, E-0099-W76. Source restores pointers before rebuilding current bounds.
void CarCollisionBody::DoSnapshot(std::uint32_t snapshot){auto& m=memory_;ReplaySnapshot s(m,snapshot);s.BeginSection(address_,0x2b0430);DynamicCollisionBody::DoSnapshot(snapshot);::port::driving::Memory::Temporary driver(m,1);m.byte(driver.address(),m.byte(m.word(address_+0x100)+0xa));s.VerifyData(driver.address(),1,1,1);if(m.byte(snapshot+0x3d))SetupPhysicsPointers();if(m.byte(snapshot+0x3c)||m.byte(snapshot+0x3d))UpdatePosition();s.VerifyData(address_+0x14,8);s.VerifyData(address_+0x1c,8);for(auto offset:{0x104u,0x108u,0x10cu})s.MoveData(address_+offset,4);s.EndSection();}
// Debug001D5550, E-0099-W76.
void TrackPolygonCollisionBody::DoSnapshot(std::uint32_t snapshot){ReplaySnapshot s(memory_,snapshot);s.BeginSection(address_,0x2b04b0);StaticCollisionBody::DoSnapshot(snapshot);s.VerifyData(address_+0x14,8);s.VerifyData(address_+0x1c,8);s.EndSection();}
// Debug001D3B20, E-0099-W76. Actual negative-ID null and positive table lookup.
std::uint32_t CollisionBody::FindCollisionBody(::port::driving::Memory& m,int id){if(id<0)return 0;if(id>=206)hp2::not_implemented("0x001D3B20","original collision body ID outside allocated206-slot table");return m.word(m.word(0x3283d0)+4*std::uint32_t(id));}
// Debug001D3AA8, E-0099-W76. Verify IDs in current actual intrusive order.
void CollisionBody::SnapshotEverything(::port::driving::Memory& m,std::uint32_t snapshot){ReplaySnapshot s(m,snapshot);s.BeginSection(0x2b0370);for(auto body=m.word(0x3283d8);body!=0x3283d8;body=m.word(body))s.VerifyData(body+0xe,2,2,0);s.EndSection();}
// Debug001D55B8, E-0099-W76. Actual polygon index/ID pairs followed by FFFF;
// restoration constructs each real pooled body, preserving original registration.
void TrackPolygonCollisionBody::SnapshotEverything(::port::driving::Memory& m,std::uint32_t snapshot){ReplaySnapshot s(m,snapshot);s.BeginSection(0x2b04d0);s.VerifyData(0x311594,4,4,1);::port::driving::Memory::Temporary temp(m,4);const auto p=temp.address();if(!m.byte(snapshot+0x3d)){for(auto body=m.word(0x3283d8);body!=0x3283d8;body=m.word(body))if(m.signed_byte(body+0xc)==3){const auto polygon=m.word(m.word(body+0x54)+0xc),base=m.word(0x2f0d20);m.half(p,std::uint16_t((polygon-base)>>5));m.half(p+2,m.half(body+0xe));s.MoveData(p,2,2,0);s.MoveData(p+2,2,2,0);TrackPolygonCollisionBody(m,body).DoSnapshot(snapshot);}m.half(p,0xffff);s.MoveData(p,2,2,0);}else{for(;;){s.MoveData(p,2,2,0);if(m.half(p)==0xffff)break;const auto polygon=m.word(0x2f0d20)+32*std::uint32_t(m.half(p));s.MoveData(p+2,2,2,0);const auto body=bOMalloc(m,m.word(0x328428));TrackPolygonCollisionBody object(m,body);object.Sub_001D53A0(m.half(p+2),polygon);object.DoSnapshot(snapshot);}}s.EndSection();}
// Debug001D2DA0, E-0099-W76. Source recreates positive collision before moving its fields.
void CollisionBodyWitness::DoSnapshot(std::uint32_t snapshot){auto& m=memory_;ReplaySnapshot s(m,snapshot);s.BeginSection(address_,0x2b0308);hp2::recovered::VCWitness::DoSnapshot(m,address_+0x10,snapshot);::port::driving::Memory::Temporary flag(m,1);m.byte(flag.address(),m.word(address_+0x68)!=0);s.MoveData(flag.address(),1,1,1);if(m.byte(flag.address())){if(m.byte(snapshot+0x3d))BeginCollision(0);Collision(m,m.word(address_+0x68)).DoSnapshot(snapshot);}s.EndSection();}
// Debug001D2E40, E-0099-W76. Original signed2-byte count/IDs, real pooled witnesses.
void CollisionBodyWitness::SnapshotEverything(::port::driving::Memory& m,std::uint32_t snapshot){ReplaySnapshot s(m,snapshot);s.BeginSection(0x2b0320);::port::driving::Memory::Temporary temp(m,12);const auto p=temp.address();if(!m.byte(snapshot+0x3d)){s.MoveData(0x311580,2,4,1);for(auto w=m.word(0x328400);w!=0x328400;w=m.word(w)){m.word(p,std::uint32_t(std::int32_t(m.signed_half(m.word(w+0x60)+0xe))));m.word(p+4,std::uint32_t(std::int32_t(m.signed_half(m.word(w+0x64)+0xe))));s.MoveData(p,2,4,1);s.MoveData(p+4,2,4,1);CollisionBodyWitness(m,w).DoSnapshot(snapshot);}}else{s.MoveData(p+8,2,4,1);const int count=m.signed_word(p+8);for(int i=0;i<count;++i){s.MoveData(p,2,4,1);s.MoveData(p+4,2,4,1);const auto b1=CollisionBody::FindCollisionBody(m,m.signed_word(p)),b2=CollisionBody::FindCollisionBody(m,m.signed_word(p+4));if(!b1||!b2)hp2::not_implemented("0x001D2E40","restored witness requires both original registered collision body IDs");const auto w=bOMalloc(m,m.word(0x328430));CollisionBodyWitness object(m,w);object.Sub_001D2AC8(b1,b2);object.DoSnapshot(snapshot);}}s.EndSection();}
}

namespace hp2::debug {
// Debug001D3B40/001D3B50, E-0099-W96. Source byte counter, not a Boolean assignment.
void CollisionBody::DisableCollisions(){memory_.byte(address_+0x11,static_cast<std::uint8_t>(memory_.byte(address_+0x11)+1u));}
void CollisionBody::EnableCollisions(){const auto before=memory_.byte(address_+0x11);if(before)memory_.byte(address_+0x11,static_cast<std::uint8_t>(before-1u));}
}

namespace hp2::debug {
// Debug001D5BE8/001D5CF8,E-0110-F09, original collisionbody.cpp source leaves.
float DynamicCollisionBody::GetMassInverse()const{return memory_.scalar(memory_.word(address_+0x60)+0x11c);}
float StaticCollisionBody::GetMassInverse()const{return 0;}
}

namespace hp2::debug {
// Debug001D39A0; source XY overlap and Types1/2 only.
bool CollisionBody::AnySmokeablesInBox(::port::driving::Memory& m,const std::array<float,4>& low,const std::array<float,4>& high){const auto head=0x3283d8u;for(auto body=m.word(head);body!=head;body=m.word(body)){if(!body)hp2::not_implemented("0x001D39A0","actual CollisionBodyList must be initialized");const bool overlapping=m.scalar(body+0x14)<high[0]&&low[0]<m.scalar(body+0x1c)&&m.scalar(body+0x18)<high[1]&&low[1]<m.scalar(body+0x20);if(overlapping&&(m.signed_byte(body+0xc)==1||m.signed_byte(body+0xc)==2))return true;}return false;}
}
