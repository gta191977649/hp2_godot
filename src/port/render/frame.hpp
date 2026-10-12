#pragma once
#include <cstdint>
#include <memory>
#include <vector>

// Port-side presentation contract, not a recovered PS2 ABI. No backend types.
namespace port::render {
using TextureId = std::uint64_t;
struct Vec2 { float x=0,y=0; };
struct Rect { float x=0,y=0,width=0,height=0; bool has_area() const {return width>0&&height>0;} };
struct Color { float r=1,g=1,b=1,a=1; };
struct Bitmap { int width=0,height=0; std::vector<std::uint8_t> rgba; std::uint64_t revision=1; };
struct Quad { TextureId texture=0; Rect destination,source; Color color; };
struct Frame { Vec2 logical_size{640,480}; Color clear{0,0,0,1}; std::vector<Quad> quads; std::uint64_t sequence=0; };
class Resources {
public:
 virtual ~Resources()=default;
 virtual std::shared_ptr<const Bitmap> texture(TextureId id)=0;
};
class Backend {
public:
 virtual ~Backend()=default;
 virtual void submit(const Frame& frame,Resources& resources)=0;
 virtual void reset()=0;
};
// Applies original clip rectangles without knowing the rendering device.
bool crop(Quad& quad,const Rect& clip);
} // namespace port::render
