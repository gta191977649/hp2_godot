// TASK-0039 port packet regression; static evidence E-0039-Q02/Q03/Q05/Q06.
#include "port/render/frame.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

int main() {
 try {
  using namespace port::render;
  const auto require=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
  const auto near=[](float a,float b){return std::abs(a-b)<0.0001f;};
  Quad contained;
  contained.destination={10,10,20,20};contained.has_vertex_data=true;
  contained.source_points={Vec2{0,0},Vec2{10,1},Vec2{8,9},Vec2{2,7}};
  require(crop(contained,{0,0,100,100})&&contained.source_points[2].x==8&&
          contained.source_points[3].y==7,"containing clip altered untouched authored UV corners");
  Quad q;
  q.destination={0,0,100,100};q.source={0,0,200,200};q.has_vertex_data=true;
  q.positions={Vec2{0,0},Vec2{100,0},Vec2{100,100},Vec2{0,100}};
  q.source_points={Vec2{10,20},Vec2{210,20},Vec2{210,420},Vec2{10,420}};
  q.colors={Color{1,0,0,1},Color{0,1,0,0.75f},Color{0,0,1,0.5f},Color{1,1,0,0.25f}};
  require(crop(q,{25,20,50,60}),"partial vertex clip rejected");
  require(q.positions[0].x==25&&q.positions[2].x==75&&q.positions[0].y==20&&q.positions[2].y==80,
          "clipped vertex positions differ");
  require(near(q.source_points[0].x,60)&&near(q.source_points[1].x,160)&&near(q.source_points[0].y,100)&&near(q.source_points[2].y,340),
          "clipped vertex UVs differ");
  require(q.colors[0].r==1&&q.colors[1].g==1&&q.colors[2].b==1&&q.colors[3].a==0.25f,
          "CPU clipping changed original corner colours");
  require(!crop(q,{200,200,10,10}),"outside vertex quad accepted");
  q.bypass_clip=true;
  q.positions={Vec2{-10,0},Vec2{0,-10},Vec2{10,0},Vec2{0,10}};
  require(crop(q,{200,200,10,10})&&q.positions[0].x==-10&&q.positions[1].y==-10,
          "rotated packet did not bypass rectangle clipping");
  Quad flipped;
  flipped.destination={0,0,100,100};flipped.source={0,0,100,100};flipped.has_vertex_data=true;
  flipped.source_points={Vec2{100,100},Vec2{0,100},Vec2{0,0},Vec2{100,0}};
  require(crop(flipped,{25,25,50,50})&&near(flipped.source_points[0].x,75)&&near(flipped.source_points[2].x,25)&&
          near(flipped.source_points[0].y,75)&&near(flipped.source_points[2].y,25),"flipped UV clip lost direction");
  std::cout<<"FRONTEND_QUAD_TESTS_OK\n";return 0;
 } catch(const std::exception& e) {std::cerr<<"frontend_quad_tests failed: "<<e.what()<<'\n';return 1;}
}
