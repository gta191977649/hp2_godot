#pragma once
// TASK-0112 E-0112-U01. Read-only port projection of live source Flailer nodes.
// Layout provenance: E-0099-W52 and the independently parsed original assets.
// This does not manufacture HUD objects or replace missing source producers.
#include "game_core/port/driving/memory.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/indep/inc/Language.hpp"
#include "game_core/port/race/player_hud_render.hpp"
#include <set>
#include <stdexcept>

namespace port::race {
inline std::vector<hp2::frontend::FeScreen> snapshot_visible_flailer(const driving::Memory& m,std::uint32_t only_screen=0,bool visible_only=true,bool source_geometry=false,std::uint32_t only_address=0){
 std::vector<hp2::frontend::FeScreen> result;
 std::set<std::uint32_t> projects,screens,objects;
 const auto text=[&](std::uint32_t at,std::uint32_t limit){std::string value;for(std::uint32_t i=0;i<limit;++i){const auto c=m.byte(at+i);if(!c)break;value.push_back(char(c));}return value;};
 for(auto project=m.word(0x3271c0);project!=0x3271c0;project=m.word(project)){
  if(!projects.insert(project).second||projects.size()>1024)throw std::logic_error("Malformed source Flailer project ring");
  const auto head=project+0xc;
  for(auto screen=m.word(head);screen!=head;screen=m.word(screen)){
   if(!screens.insert(screen).second||screens.size()>4096)throw std::logic_error("Malformed source Flailer screen ring");
   if(only_address&&screen!=only_address)continue;
   if(only_screen&&m.word(screen+8)!=only_screen)continue;
   if(visible_only&&(m.half(screen+0x1e)&1))continue;
   hp2::frontend::FeScreen doc;doc.name_hash_0x08=m.word(screen+8);doc.width=m.half(screen+0x18);doc.height=m.half(screen+0x1a);
   const auto list=screen+0x20;
   for(auto object=m.word(list);object!=list;object=m.word(object)){
    if(!objects.insert(object).second||objects.size()>65536)throw std::logic_error("Malformed source Flailer object ring");
    if(visible_only&&!m.byte(object+0x29))continue;
    hp2::frontend::FeWidget w;w.port_source_address=object;const auto type=m.byte(object+0x28);
    if(!visible_only&&type!=1&&type!=2)continue;
    if(type!=1&&type!=2)throw std::logic_error("Visible source Flailer object type is outside the current frame transport");
    w.type=type;w.chunk_id=type==1?hp2::frontend::kTextWidgetId:hp2::frontend::kQuadWidgetId;
    w.name_hash=m.word(object+8);w.class_hash=m.word(object+0xc);w.name=text(object+0x10,24);w.visible=m.byte(object+0x29);w.layer=m.byte(object+0x2a);
    w.x=m.scalar(object+0x30);w.y=m.scalar(object+0x34);w.scale_x=m.scalar(object+0x38);w.scale_y=m.scalar(object+0x3c);w.clip_hash_0x4c=m.word(object+0x4c);w.texture_hash=m.word(object+0x50);
    const auto colour=object+(type==1?0x5cu:0x58u);w.r=m.byte(colour);w.g=m.byte(colour+1);w.b=m.byte(colour+2);w.a=m.byte(colour+3);
    if(type==1){w.text_capacity=m.word(object+0x54);w.text_align=m.word(object+0x58);w.string_id_0x60=m.word(object+0x60);const auto extent=m.word(object-4);if(extent<0x64)throw std::logic_error("Live Flailer text extent exceeds its source chunk: hash="+std::to_string(w.name_hash)+" extent="+std::to_string(extent)+" capacity="+std::to_string(w.text_capacity)+" language="+std::to_string(w.string_id_0x60));w.text=text(object+0x64,extent-0x64);if(m.word(0x328a94)&&w.string_id_0x60)w.text=text(hp2::debug::GetTranslatedString(m,int(w.string_id_0x60)),4096);}
    else{w.field_0x40=m.signed_word(object+0x40);w.field_0x44=m.word(object+0x44);for(unsigned i=0;i<3;++i)w.field_0x5c[i]=m.word(object+0x5c+4*i);for(unsigned i=0;i<8;++i)w.field_0x70[i]=m.scalar(object+0x70+4*i);}
    if(type==2&&source_geometry&&w.texture_hash){
     std::vector<::port::race::HudTachometerQuad> packets;
     if(m.word(object+0x6c))packets=::port::race::snapshot_source_tachometer(m,object);
     else packets.push_back(::port::race::snapshot_source_bitmap_geometry(m,object));
     for(const auto& packet:packets){auto copy=w;copy.port_quad_corners.emplace();for(unsigned i=0;i<4;++i){(*copy.port_quad_corners)[i]={{packet.points[i][0],packet.points[i][1]}};copy.field_0x70[i*2]=packet.points[i][3];copy.field_0x70[i*2+1]=packet.points[i][4];if(i)copy.field_0x5c[i-1]=packet.colours[i];}const auto c=packet.colours[0];copy.r=std::uint8_t(c);copy.g=std::uint8_t(c>>8);copy.b=std::uint8_t(c>>16);copy.a=std::uint8_t(c>>24);doc.widgets.push_back(std::move(copy));}
    }else doc.widgets.push_back(std::move(w));
   }
   result.push_back(std::move(doc));
  }
 }return result;
}
}
