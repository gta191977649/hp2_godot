// TASK-0046 E-0046-S01/S02/S03. Synthetic boundary fixtures, not PS2 certification.
#include "game_core/indep/src/skids.hpp"
#include "game_core/indep/src/wheel.hpp"
#include "game_core/src/ecstasy/ecstasye.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/port/driving/projections.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace port::driving;
using hp2::recovered::SkidSegment;
using hp2::recovered::SkidMaker;
using hp2::recovered::SkidSet;
namespace {
void check(bool b,const char*m){if(!b)throw std::runtime_error(m);}
bool close(float a,float b){return std::abs(a-b)<0.00001f;}
struct Fixture {
    Memory m;SkidArena arena;SkidServices services;std::vector<SkidQuad> quads;
    Fixture(){m.map(0x300000,0x40000);
        // Original bits read at Debug003257C8..D8 and00325774/78.
        m.word(0x3257c8,0x3e4ccccd);m.word(0x3257cc,0x3b03126f);m.word(0x3257d0,0x3d4ccccd);m.word(0x3257d4,0x3b808081);m.word(0x3257d8,0x422aaaab);
        services.arena=&arena;services.emit=[this](const SkidQuad&q){quads.push_back(q);};
        hp2::recovered::InitSkids(m,services,[](const std::string&name,std::uint32_t hash){return SkidTexture{name,hash,true,true,false};});
        arena.textures[1]={"synthetic-test-binding",123,true,true,false};
    }
    unsigned maker(){return m.allocate(4);}
};
void segment_codec(){Fixture f;auto seg=f.m.allocate(16);SkidSegment s(f.m,seg);s.SetPoints({{10,20,30}},{{0.30f,-0.30f,0.02f}});
    check(f.m.signed_byte(seg+12)==19&&f.m.signed_byte(seg+13)==-19&&f.m.byte(seg+14)==1,"original compressed truncation");
    SkidVector p,d,l,r;s.GetPoints(p,&d);s.GetEndPoints(l,r);check(close(d[0],19.0f/64)&&close(l[1],20-19.0f/64)&&close(r[2],30-1.0f/64),"signed compressed endpoints");
    check(hp2::recovered::bNormalize({{0,0,0}},2)==SkidVector{{2,0,0}},"original zero normalization branch");
}
void maker_geometry(){Fixture f;const auto maker=f.maker();SkidMaker sm(f.m,maker);const SkidVector delta{{0,0.25f,0}};
    sm.MakeSkid(0,{{0,0,0}},delta,1,1,f.services);const auto first=f.m.word(maker);
    sm.MakeSkid(0,{{1,0,0}},delta,1,0.5f,f.services);check(f.m.word(first+0xf0)==2,"second original segment");
    sm.MakeSkid(0,{{2,0,0}},delta,1,1,f.services);check(f.m.word(first+0xf0)==2,"collinear segment merges endpoint");
    SkidSet(f.m,first).Render(0,f.services);check(f.quads.size()==1,"one original quad");const auto&q=f.quads[0];
    check(close(q.vertices[0][1],0.25f)&&close(q.vertices[3][1],-0.25f)&&close(q.vertices[1][0],2),"original quad winding/endpoints");
    check(close(q.vertices[0][2],0.05f)&&q.colours[0][0]==128&&q.colours[0][3]==127,"original zoffset and raw colour");
    check(q.uv[0]==std::array<float,2>{{0,0}}&&q.uv[2]==std::array<float,2>{{1,1}},"original ePoly UVs");
    sm.MakeSkid(0,{{3,0,0}},delta,2,1,f.services);const auto second=f.m.word(maker);
    check(second!=first&&f.m.word(first+0x30)==0&&f.m.word(second+0xf0)==2,"terrain transition bridges/detaches original sets");
    sm.MakeNoSkid();check(!f.m.word(maker)&&!f.m.word(second+0x30),"no skid terminates maker");
}
void limits_and_lru(){Fixture f;const auto maker=f.maker();SkidMaker sm(f.m,maker);const SkidVector d{{0,0.25f,0}};
    for(unsigned i=0;i<9;++i)sm.MakeSkid(0,{{4.0f*i,0,0}},d,1,1,f.services);
    const auto second=f.m.word(maker),first=f.m.word(f.arena.list+4);
    check(first!=second&&f.m.word(first+0xf0)==8&&f.m.word(second+0xf0)==2,"eight segment rollover with bridge");
    sm.MakeNoSkid();for(unsigned i=0;i<126;++i){const auto mk=f.maker();SkidMaker(f.m,mk).MakeSkid(0,{{float(i),2,0}},d,1,1,f.services);}
    auto old=f.m.word(f.arena.list+4);const auto mk=f.maker();SkidMaker(f.m,mk).MakeSkid(0,{{0,5,0}},d,1,1,f.services);
    check(f.m.word(mk)==old,"128 slot pool replaces tail");
    unsigned count=0;for(unsigned i=0;i<128;++i)count+=f.m.byte(f.arena.used+i);check(count==128,"pool remains bounded");
}
void cull_fade(){Fixture f;const auto mk=f.maker();SkidMaker sm(f.m,mk);sm.MakeSkid(0,{{0,0,0}},{{0,0.25f,0}},1,1,f.services);sm.MakeSkid(0,{{1,0,0}},{{0,0.25f,0}},1,1,f.services);
    SkidView v;v.clip_matrix[0]=v.clip_matrix[5]=v.clip_matrix[10]=0.01f;v.clip_matrix[15]=1;v.position={{0.5f,0,10}};v.H=105;
    hp2::recovered::RenderSkids(f.m,v,f.services);check(f.quads.size()==1&&f.quads[0].colours[0][3]==127,"original ten pixel branch");
    f.quads.clear();v.H=65;hp2::recovered::RenderSkids(f.m,v,f.services);check(f.quads.size()==1&&f.quads[0].colours[0][3]==42,"original six pixel intensity reduction");
    f.quads.clear();v.H=45;hp2::recovered::RenderSkids(f.m,v,f.services);check(f.quads.empty(),"four pixel cull");
    v.H=105;v.nis_playing=true;hp2::recovered::RenderSkids(f.m,v,f.services);check(f.quads.empty(),"NIS suppresses skid rendering");
    check(hp2::recovered::ClassifyAABB({{-0.5f,-0.5f,-0.5f}},{{0.5f,0.5f,0.5f}},init::bIdentity())==2,"inside AABB");
    check(!hp2::recovered::IsBoundingBoxVisible({{2,2,2}},{{3,3,3}},init::bIdentity()),"outside AABB");
}
void render_promotes_lru(){
    Fixture f;const auto first_maker=f.maker(),second_maker=f.maker();const SkidVector d{{0,0.25f,0}};
    SkidMaker first(f.m,first_maker),second(f.m,second_maker);
    first.MakeSkid(0,{{0,0,0}},d,1,1,f.services);first.MakeSkid(0,{{1,0,0}},d,1,1,f.services);
    second.MakeSkid(0,{{1000,0,0}},d,1,1,f.services);second.MakeSkid(0,{{1001,0,0}},d,1,1,f.services);
    const auto a=f.m.word(first_maker),b=f.m.word(second_maker);check(f.m.word(f.arena.list)==b,"new set initially at global head");
    SkidView view;view.clip_matrix[0]=view.clip_matrix[5]=view.clip_matrix[10]=0.01f;view.clip_matrix[15]=1;view.position={{1,0,10}};view.H=105;
    hp2::recovered::RenderSkids(f.m,view,f.services);
    check(f.m.word(f.arena.list)==a&&f.m.word(f.arena.list+4)==b,"only visible rendered set moves to LRU head");
    f.quads.clear();view.nis_playing=true;hp2::recovered::RenderSkids(f.m,view,f.services);
    check(f.m.word(f.arena.list)==a&&f.quads.empty(),"NIS render does not advance LRU");
}
struct Elevation:ContactProvider{int count=0;std::array<SkidVector,2> points{};float GetElevation(Memory&m,std::uint32_t,std::uint32_t p,std::uint32_t,std::uint32_t,std::uint32_t)override{points[count%2]={{m.scalar(p),m.scalar(p+4),m.scalar(p+8)}};++count;return m.scalar(p+4)*0.1f;}};
void debug_draw_context(){
    SkidTexture t;t.present=true;
    auto state=hp2::recovered::AttachSolidPolys(t,true);check(state.draw_context==2&&state.write_depth&&state.alpha_test==6&&state.alpha_reference==48,"Debug opaque context");
    t.semitransparent=true;t.rendering_order=2;state=hp2::recovered::AttachSolidPolys(t,true);check(state.draw_context==3&&state.write_depth&&state.alpha_test==5,"Debug early semi writes depth");
    t.rendering_order=3;state=hp2::recovered::AttachSolidPolys(t,true);check(state.draw_context==1&&!state.write_depth,"Debug late semi disables depth");
    t.gloss_map_in_alpha=true;state=hp2::recovered::AttachSolidPolys(t,true);check(state.draw_context==3&&state.write_depth,"Debug solid gloss context");
    state=hp2::recovered::AttachAlphaPolys(t,true);check(state.draw_context==1&&!state.write_depth,"Debug alpha route omits gloss override");
    state=hp2::recovered::AttachSolidPolys(t,false);check(state.draw_context==1&&!state.write_depth,"global depth gate clears bit1");
}
void wheel_eligibility(){Fixture f;Elevation provider;f.services.contact=&provider;auto w=f.m.allocate(0x190),wi=f.m.allocate(0x20),cs=f.m.allocate(0x410),car=f.m.allocate(0x900);
    f.m.word(w+0xc,wi);f.m.word(w+0x18,cs);f.m.scalar(wi+0x10,0.5f);f.m.word(wi+0x14,2);f.m.word(car+0x514,0xffffffffu);
    f.m.matrix(w+0x70,init::bIdentity());f.m.scalar(w+0xf0,2);f.m.byte(w+0x155,1);f.m.byte(w+0x154,1);f.m.word(w+0x150,1);f.m.scalar(0x3102e8+0x90+0x14,1);
    hp2::recovered::Wheel wheel(f.m,w);wheel.MaybeMakeSkids(0.016f,car,f.services);check(provider.count==2&&f.m.word(w+0x180),"wheel calls two topology elevations");
    const auto set=f.m.word(w+0x180);check(close(f.m.scalar(set+0x70+8),0)&&close(provider.points[0][1],0.25f),"original width/height endpoint queries");
    f.m.byte(w+0x155,0);wheel.MaybeMakeSkids(0.016f,car,f.services);check(!f.m.word(w+0x180),"airborne skid detaches");
    f.m.byte(w+0x155,1);f.m.byte(car+0x848,4);wheel.MaybeMakeSkids(0.016f,car,f.services);check(!f.m.word(w+0x180)&&provider.count==2,"blown wheel excluded");
    f.m.byte(car+0x848,0);f.m.byte(w+0x154,0);f.m.scalar(w+0x158,8);f.m.scalar(w+0x168,1);wheel.MaybeMakeSkids(0.016f,car,f.services);check(!f.m.word(w+0x180),"strict force threshold");
    f.m.byte(w+0x154,1);f.m.word(w+0x150,4);f.m.scalar(0x3102e8+4*0x90+0x14,1);wheel.MaybeMakeSkids(0.016f,car,f.services);check(f.m.byte(f.m.word(w+0x180)+0x7f)==0,"puddle intensity zero");
}
}
int main(){try{segment_codec();maker_geometry();limits_and_lru();cull_fade();render_promotes_lru();debug_draw_context();wheel_eligibility();std::cout<<"DRIVING_SKID_TESTS_OK\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
