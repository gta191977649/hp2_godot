#include "nis_render_packet.hpp"
#include "car_render_resources.hpp"
#include "player_construction.hpp"
#include "game_core/indep/inc/NIS.hpp"
#include "game_core/indep/src/track.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cmath>
#include <functional>
#include <set>
namespace port::race {
namespace {
using Matrix=std::array<float,16>;using Vector=std::array<float,4>;
Matrix identity(){return hp2::recovered::bIdentity();}
Matrix multiply(const Matrix& a,const Matrix& b){Matrix out{};hp2::recovered::eMulMatrix(out,a,b);return out;}
Vector transform(const Matrix& a,const Vector& b){Vector out{};for(unsigned lane=0;lane<4;++lane){float f=a[lane]*b[0];f=f+a[4+lane]*b[1];f=f+a[8+lane]*b[2];out[lane]=f+a[12+lane]*b[3];}return out;}
// Debug002532D8, exact scalar evaluation order, no quaternion normalization.
Matrix quaternion_matrix(const Vector& q){Matrix m{};const float x=q[0],y=q[1],z=q[2],twox=x+x,twow=q[3]+q[3],yy=(y+y)*y,yz=(y+y)*z,xx=1.f-twox*x,zz=(z+z)*z;m[15]=1;m[4]=twox*y+twow*z;m[8]=twox*z-twow*y;m[9]=yz+twow*x;m[10]=xx-yy;m[0]=(1.f-yy)-zz;m[5]=xx-zz;m[1]=twox*y-twow*z;m[2]=twox*z+twow*y;m[6]=yz-twow*x;return m;}
// Debug0011BF28 source affine inverse: transpose basis, negative translation.
Matrix inverse_transform(const Matrix& in){auto basis=identity(),translation=identity();for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)basis[4*row+col]=in[4*col+row];for(unsigned i=0;i<3;++i)translation[12+i]=-in[12+i];return multiply(translation,basis);}
Vector translation(driving::Memory& m,std::uint32_t solid,float frame){if(!std::isfinite(frame)||frame>2147483520.f||frame< -2147483648.f)hp2::not_implemented("0x001A50C0","unsupported original float-to-integer frame");int index=int(frame);if(index<0||index>=m.half(solid+0x44))index=0;const auto key=m.word(solid+0x54)+16*unsigned(index);return {{m.scalar(key),m.scalar(key+4),m.scalar(key+8),0}};}
struct Pose {std::uint32_t tree=0,nodes=0;std::vector<Matrix> matrices;Vector root{};};
// Debug001A4230/001A4360/001A45E0: NIS SetAnimation always Weight1=1.
Pose pose(driving::Memory& m,std::uint32_t animation){Pose result;const auto solid=m.word(animation+0x28),model=m.word(animation+0x14);if(!solid||!model)return result;result.tree=model+0xa0;const auto count=m.word(result.tree);if(count!=m.half(solid+0x46))return {};if(count>42)hp2::not_implemented("0x001A4230","source quaternion stack has42 elements");if(m.scalar(animation+0x38)!=1.f)hp2::not_implemented("0x001A4230","NIS packet requires original single-animation path");result.nodes=m.word(result.tree+4);result.root=translation(m,solid,m.scalar(animation+0x30));result.matrices.resize(count);if(!count)return result;const int frame=int(m.scalar(animation+0x30));if(frame<0||frame>=m.half(solid+0x44))hp2::not_implemented("0x001A5010","source rotation key frame exceeds authored keys");const auto keys=m.word(solid+0x4c);std::vector<Vector> rotations(count);for(unsigned i=0;i<count;++i)for(unsigned j=0;j<4;++j)rotations[i][j]=float(m.signed_half(keys+8*(m.half(solid+0x44)*i+unsigned(frame))+2*j))*m.scalar(0x3257ac);
 std::set<unsigned> active,visited;std::function<void(unsigned,const Matrix&)> visit=[&](unsigned i,const Matrix& parent){if(i>=count)return;if(!active.insert(i).second||!visited.insert(i).second)hp2::not_implemented("0x001A45E0","malformed source animation tree cycle/repeated child");auto local=quaternion_matrix(rotations[i]);for(unsigned j=0;j<3;++j)local[12+j]=m.scalar(result.nodes+0x60*i+0x40+4*j);auto& out=result.matrices[i];out=multiply(local,parent);if(int(i)==m.signed_word(0x327da0))for(unsigned row=0;row<3;++row)for(unsigned j=0;j<4;++j)out[4*row+j]=out[4*row+j]*m.scalar(0x3087e0+4*row);for(int child=m.signed_byte(result.nodes+0x60*i+0x4e);child;child=m.signed_byte(result.nodes+0x60*unsigned(child)+0x4f)){if(child<0||unsigned(child)>=count)hp2::not_implemented("0x001A45E0","source child index outside actual tree");visit(unsigned(child),out);}active.erase(i);};visit(0,identity());if(visited.size()!=count)hp2::not_implemented("0x001A4360","uninitialized source bone matrix from disconnected tree");for(unsigned i=0;i<count;++i)result.matrices[i]=multiply(m.matrix(result.nodes+0x60*i),result.matrices[i]);return result;
}
}
NisDrawPacket snapshot_source_animation(driving::Memory& m,std::uint32_t animation,const Matrix& world){auto value=pose(m,animation);auto final_world=world;const auto offset=transform(world,value.root);for(unsigned i=0;i<3;++i)final_world[12+i]=final_world[12+i]+offset[i];final_world[15]=1;return {m.word(animation+0x10),final_world,std::move(value.matrices)};}
// Render001B0138 and RenderHandProp001B0538, E-0112-N01. Frame allocator and
// GPU strip consumers are replaced only by immutable copied transport objects.
std::vector<NisDrawPacket> snapshot_source_nis(driving::Memory& m,std::uint32_t manager,unsigned view){std::vector<NisDrawPacket> out;if((!m.word(manager+0x80)&&!m.word(manager+0xb0))||!(m.word(manager+0x60)&(1u<<(view&31)))||!m.word(manager+0xa0))return out;hp2::debug::NisManager(m,manager).SetupForRendering();const auto info=hp2::debug::GetNisInfo(m,m.signed_word(manager+0x4c)),car=m.word(manager+0xa0);const bool apply=m.word(info+0x24)!=0,override=(m.word(car+0x510)&(1u<<(view&31)))!=0;Matrix world=m.matrix(car+(override?0x4c0:0x90));const auto position=car+(override?0x500:0x50);for(unsigned i=0;i<3;++i)world[12+i]=m.scalar(position+4*i);world[15]=1;auto flat=world;flat[8]=0;flat[9]=0;flat[10]=1;flat[11]=0;const Vector up{{0,0,1,0}},forward{{world[0],world[1],world[2],world[3]}};auto cross=[](const Vector& a,const Vector& b){return Vector{{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0],0}};};const auto right=cross(up,forward),front=cross(right,up);for(unsigned i=0;i<4;++i){flat[4+i]=right[i];flat[i]=front[i];}
 for(unsigned actor=0;actor<2;++actor){const auto animation=m.word(manager+(actor?0xb0:0x80));if(!animation)continue;auto model_world=actor||!apply?flat:world;const auto offset=transform(world,m.vector(manager+(actor?0xc0:0x90)));for(unsigned i=0;i<4;++i)model_world[12+i]=model_world[12+i]+offset[i];auto posed=pose(m,animation);if(posed.matrices.empty())continue;
  if(actor||!apply){auto root=posed.root;root[3]=1;const auto point=transform(model_world,root);const float elevation=hp2::debug::TopologyCoordinate(m,car+0x520).GetElevation({{point[0],point[1],point[2]}});model_world[14]=model_world[14]+(elevation-m.scalar(manager+0x70))+m.scalar(actor?0x32584c:0x325848);}
  auto final_world=model_world;const auto root_offset=transform(model_world,posed.root);for(unsigned i=0;i<3;++i)final_world[12+i]=final_world[12+i]+root_offset[i];final_world[15]=1;out.push_back({m.word(animation+0x10),final_world,posed.matrices});
  for(unsigned hand=0;hand<2;++hand){const unsigned node=hand?0x26:0x19;if(node>=posed.matrices.size())hp2::not_implemented("0x001B0538","source hand node exceeds actual animation tree");const auto prop=manager+(actor?0x110:0xe0)+24*hand;const auto hash=m.word(prop+8);if(!hash)continue;auto inverse=inverse_transform(m.matrix(posed.nodes+node*0x60));std::array<float,3> hand_offset{};for(unsigned i=0;i<3;++i)hand_offset[i]=m.scalar((hand?0x32b1e0:0x32b1d0)+4*i);Matrix shifted{};hp2::recovered::eTranslate(shifted,inverse,hand_offset);out.push_back({hash,multiply(multiply(shifted,posed.matrices[node]),final_world),{}});}
 }
 return out;
}
}
