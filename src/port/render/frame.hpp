#pragma once
#include <cstdint>
#include <cstddef>
#include <array>
#include <memory>
#include <vector>

// Port-side presentation contract, not a recovered PS2 ABI. No backend types.
namespace port::render {
using TextureId = std::uint64_t;
struct Vec2 { float x=0,y=0; };
struct Rect { float x=0,y=0,width=0,height=0; bool has_area() const {return width>0&&height>0;} };
struct Color { float r=1,g=1,b=1,a=1; };
struct Bitmap { int width=0,height=0; std::vector<std::uint8_t> rgba; std::uint64_t revision=1; };
// texture==0 is a solid-colour rect: `color` fills `destination` and `source` is unused. The
// recovered event-tree connectors (tLineSegment) are axis-aligned, so they need no other
// primitive. Every other quad samples `source` from `texture`.
struct Quad {
 TextureId texture=0; Rect destination,source; Color color;
 // Port-side packet corners TL, TR, BR, BL. UVs are bitmap pixel coordinates.
 // E-0039-Q02/Q03/Q05: optional independent original packet data; rectangle callers remain valid.
 bool has_vertex_data=false;
 std::array<Vec2,4> positions{}, source_points{};
 std::array<Color,4> colors{};
 // E-0098-F04: authored Flailer layer survives glyph expansion/clipping and
 // document concatenation. -1 denotes legacy/untagged port quads.
 std::int8_t frontend_layer=-1;
 bool bypass_clip=false; // E-0039-Q05: original rotated quads skip CPU rectangle clipping.
};
// E-0098-F01: view0 Flailer layers24..20 are submitted in eDisplayFrame stage1;
// layers15..0,19..16,28..25 are submitted in stage7. These are port packet tags,
// not an original enum or a compositor: stages0/2/4 still require shared 3D state.
enum class FrontendStage : std::uint8_t { unspecified=0, background=1, foreground=7 };
struct FrontendQuadRange {
 FrontendStage stage=FrontendStage::unspecified;
 std::size_t first=0,count=0;
};
struct Frame { Vec2 logical_size{640,480}; Color clear{0,0,0,1}; std::vector<Quad> quads; std::uint64_t sequence=0;
 // Optional metadata from the widget builder. Consumers that append quads must
 // propagate/rebase their ranges before treating this as complete frame coverage.
 // Empty stages have no range; existing flat-quad backends remain compatible.
 std::vector<FrontendQuadRange> frontend_ranges;
 // Port display-device translation after logical clipping, separate from widgets.
 Vec2 display_offset{};
};
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
// Port packet utilities. Coverage requires known stages and a contiguous partition
// of every quad; legacy appends that omit metadata therefore remain detectable.
bool frontend_ranges_complete(const Frame& frame) noexcept;
// Appends without reordering quads; rebases and merges only adjacent equal-stage
// ranges. Partial metadata stays partial. Invalid/overlapping ranges are rejected.
void append_frontend_frame(Frame& destination,const Frame& source);
// Applies original clip rectangles without knowing the rendering device.
bool crop(Quad& quad,const Rect& clip);
} // namespace port::render
