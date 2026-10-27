// TASK-0018 / D008: the port runtime must produce the front end's frames with no backend and
// no Godot. A fake Resources supplies original-shaped texture dimensions and a fake Backend
// records what a device would have received.
#include "port/runtime/frame_builder.hpp"
#include <iostream>
#include <map>
#include <stdexcept>
#include <cstring>
#include <algorithm>

namespace {

using namespace port;

class FakeResources final : public render::Resources {
public:
    std::map<render::TextureId, std::shared_ptr<render::Bitmap>> bitmaps;
    void add(render::TextureId id, int width, int height) {
        auto bitmap = std::make_shared<render::Bitmap>();
        bitmap->width = width;
        bitmap->height = height;
        bitmap->rgba.assign(static_cast<std::size_t>(width) * height * 4, 0xff);
        bitmaps[id] = std::move(bitmap);
    }
    std::shared_ptr<const render::Bitmap> texture(render::TextureId id) override {
        auto it = bitmaps.find(id);
        return it == bitmaps.end() ? nullptr : it->second;
    }
};

// Stands in for a device backend. Records submissions and the revision it last uploaded, which
// is how a real backend decides whether to re-upload a texture.
class FakeBackend final : public render::Backend {
public:
    std::vector<render::Frame> submitted;
    std::map<render::TextureId, std::uint64_t> uploaded;
    int resets = 0, solid_rects = 0;
    void submit(const render::Frame& frame, render::Resources& resources) override {
        for (const auto& quad : frame.quads) {
            if (quad.texture == 0) { ++solid_rects; continue; } // solid-colour rect
            const auto bitmap = resources.texture(quad.texture);
            if (!bitmap) throw std::runtime_error("backend received a quad with no bitmap");
            uploaded[quad.texture] = bitmap->revision;
        }
        submitted.push_back(frame);
    }
    void reset() override { ++resets; uploaded.clear(); }
};

void check(bool condition, const char* what) {
    if (!condition) throw std::runtime_error(what);
}

void check_stage_coverage(const render::Frame& frame) {
    check(render::frontend_ranges_complete(frame),"widget frame has incomplete frontend stage metadata");
    std::size_t next=0;
    for (const auto& range:frame.frontend_ranges) {
        check(range.count>0 && range.first==next,"frontend stage ranges overlap or leave a gap");
        check(range.first<=frame.quads.size() && range.count<=frame.quads.size()-range.first,
              "frontend stage range exceeds quads");
        next+=range.count;
    }
    check(next==frame.quads.size(),"frontend stage ranges do not cover all widget quads");
}

hp2::frontend::FeWidget quad_widget(const char* name, float x, float y, std::uint32_t texture,
                                    std::uint8_t layer) {
    hp2::frontend::FeWidget w;
    w.chunk_id = hp2::frontend::kQuadWidgetId;
    w.type = 2;
    w.visible = 1;
    w.layer = layer;
    w.name = name;
    w.name_hash = hp2::frontend::bStringHash(name);
    w.x = x;
    w.y = y;
    w.texture_hash = texture;
    return w;
}

hp2::frontend::FeWidget text_widget(const char* name, float x, float y, std::uint32_t font_texture,
                                    const char* text, std::uint32_t align) {
    hp2::frontend::FeWidget w;
    w.chunk_id = hp2::frontend::kTextWidgetId;
    w.type = 1;
    w.visible = 1;
    w.layer = 0;
    w.name = name;
    w.name_hash = hp2::frontend::bStringHash(name);
    w.x = x;
    w.y = y;
    w.texture_hash = font_texture;
    w.text = text;
    w.text_align = align;
    return w;
}

// Three-glyph monospaced font: 10x12 cells advancing 10, so expected geometry is exact.
hp2::frontend::FeFont make_font(std::uint32_t texture_hash) {
    hp2::frontend::FeFont font;
    font.texture_hash = texture_hash;
    font.line_height = 12;
    for (int i = 0; i < 3; ++i) {
        hp2::frontend::FeGlyph glyph;
        glyph.code = static_cast<std::uint16_t>(65 + i); // 'A', 'B', 'C'
        glyph.u = static_cast<float>(i * 10);
        glyph.v = 0;
        glyph.w = 10;
        glyph.h = 12;
        glyph.advance = 10;
        font.glyphs.push_back(glyph);
    }
    return font;
}

} // namespace

int main() {
    try {
        constexpr std::uint32_t kPanel = 0x11111111;
        constexpr std::uint32_t kBack = 0x22222222;
        constexpr std::uint32_t kFont = 0x33333333;
        constexpr std::uint32_t kMovie = 0xc3f0caef; // live movie slot, as the screens author it

        FakeResources resources;
        resources.add(kPanel, 64, 32);
        resources.add(kBack, 640, 480);
        resources.add(kFont, 30, 12);
        std::vector<hp2::frontend::FeFont> fonts{make_font(kFont)};
        runtime::WidgetSizes sizes(resources, fonts);

        // --- layer order, geometry, colour -------------------------------------------------
        hp2::frontend::FeScreen screen;
        screen.width = 640;
        screen.height = 480;
        auto backdrop = quad_widget("BACKDROP", 0, 0, kBack, 9);
        auto panel = quad_widget("PANEL", 100, 50, kPanel, 0);
        panel.scale_x = 2;
        panel.scale_y = 0.5f;
        panel.r = 64; panel.g = 128; panel.b = 0; panel.a = 128;
        screen.widgets = {panel, backdrop};

        render::Frame frame;
        auto stats = runtime::build_frame(screen, sizes, {}, frame);
        check(stats.quads == 2 && stats.glyphs == 0 && stats.missing == 0, "quad counts");
        check(frame.logical_size.x == 640 && frame.logical_size.y == 480, "logical size");
        // Descending authored layer: the backdrop (layer 9) paints before the panel (layer 0),
        // even though the panel comes first in the authored list.
        check(frame.quads[0].texture == kBack && frame.quads[1].texture == kPanel, "layer order");
        const auto& drawn = frame.quads[1];
        check(drawn.destination.x == 100 && drawn.destination.y == 50, "quad position");
        check(drawn.destination.width == 128 && drawn.destination.height == 16, "authored scale");
        check(drawn.source.width == 64 && drawn.source.height == 32, "full source rect");
        // PS2 colour bytes: 0x80 is opaque, so 64/128/0/128 is (0.5, 1, 0, 1).
        check(drawn.color.r == 0.5f && drawn.color.g == 1.0f && drawn.color.b == 0.0f &&
                  drawn.color.a == 1.0f, "0x80-is-opaque colour");
        // E-0095-N04: equal-layer keyboard panels follow glyphs in the asset;
        // reverse traversal must draw their backdrop first, preserving glyph visibility.
        screen.widgets[1].layer=0;
        runtime::build_frame(screen,sizes,{},frame);
        check(frame.quads[0].texture==kBack && frame.quads[1].texture==kPanel,
              "equal-layer draw must reverse authored order");
        screen.widgets[1].layer=9;
        hp2::frontend::FeScreen phases;
        for(auto layer:{0,16,20,25,19,28,24,15})
            phases.widgets.push_back(quad_widget("PHASE",static_cast<float>(layer),0,kPanel,layer));
        runtime::build_frame(phases,sizes,{},frame);
        const int expected_phases[]{24,20,15,0,19,16,28,25};
        for(std::size_t i=0;i<8;++i)
            check(frame.quads[i].destination.x==expected_phases[i],
                  "final bucket phases must overlay keyboard after ordinary frontend");
        check_stage_coverage(frame);
        check(frame.frontend_ranges.size()==2 && frame.frontend_ranges[0].first==0 &&
              frame.frontend_ranges[0].count==2 &&
              frame.frontend_ranges[0].stage==render::FrontendStage::background &&
              frame.frontend_ranges[1].first==2 && frame.frontend_ranges[1].count==6 &&
              frame.frontend_ranges[1].stage==render::FrontendStage::foreground,
              "view0 stage1/stage7 split differs from original layer groups");
        // E-0098-F01: preserve every authored layer's existing ordering, with an
        // empty camera-dependent/widget stage represented by no packet range.
        hp2::frontend::FeScreen all_layers;
        for(int layer=0;layer<29;++layer)
            all_layers.widgets.push_back(quad_widget("LAYER",float(layer),0,kPanel,layer));
        runtime::build_frame(all_layers,sizes,{},frame);
        check_stage_coverage(frame);
        const int expected_all[]{24,23,22,21,20,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,
                                 19,18,17,16,28,27,26,25};
        check(frame.quads.size()==29 && frame.frontend_ranges[0].count==5 &&
              frame.frontend_ranges[1].count==24,"all-layer stage counts differ");
        for(std::size_t i=0;i<29;++i)
            check(frame.quads[i].destination.x==expected_all[i],"stage tags changed flat quad order");
        for(auto& w:all_layers.widgets)if(w.layer>=20 && w.layer<=24)w.visible=0;
        runtime::build_frame(all_layers,sizes,{},frame);check_stage_coverage(frame);
        check(frame.frontend_ranges.size()==1 && frame.frontend_ranges[0].first==0 &&
              frame.frontend_ranges[0].stage==render::FrontendStage::foreground,
              "empty background stage left a stale range");
        for(auto& w:all_layers.widgets)w.visible=w.layer>=20 && w.layer<=24;
        runtime::build_frame(all_layers,sizes,{},frame);check_stage_coverage(frame);
        check(frame.frontend_ranges.size()==1 && frame.frontend_ranges[0].count==5 &&
              frame.frontend_ranges[0].stage==render::FrontendStage::background,
              "empty foreground stage left a stale range");
        for(auto& w:all_layers.widgets)w.visible=0;
        runtime::build_frame(all_layers,sizes,{},frame);check_stage_coverage(frame);
        check(frame.frontend_ranges.empty(),"empty frame retained previous stage metadata");
        auto staged_text=text_widget("GLYPHS",10,20,kFont,"ABC",2);staged_text.layer=20;
        all_layers.widgets={staged_text,quad_widget("MISSING",0,0,0xdeadbeef,24),
                           quad_widget("OVERLAY",0,0,kPanel,0)};
        runtime::build_frame(all_layers,sizes,{},frame);check_stage_coverage(frame);
        check(frame.quads[0].frontend_layer==20 && frame.quads[1].frontend_layer==20 && frame.quads[2].frontend_layer==20
                && frame.quads[3].frontend_layer==0,"glyph expansion lost original Flailer layer");
        check(frame.frontend_ranges.size()==2 && frame.frontend_ranges[0].count==3 &&
              frame.frontend_ranges[1].first==3 && frame.frontend_ranges[1].count==1,
              "glyphs or missing assets corrupted emitted-quad range counts");
        {
            const auto original=frame;
            render::Frame combined;render::append_frontend_frame(combined,original);
            render::append_frontend_frame(combined,original);check_stage_coverage(combined);
            check(combined.frontend_ranges.size()==4 && combined.frontend_ranges[2].first==4 &&
                  combined.frontend_ranges[3].first==7,"append failed to rebase stage ranges");
            for(std::size_t i=0;i<combined.quads.size();++i)
                check(combined.quads[i].destination.x==original.quads[i%4].destination.x &&
                      combined.quads[i].texture==original.quads[i%4].texture &&
                        combined.quads[i].frontend_layer==original.quads[i%4].frontend_layer,
                      "append changed original flat quad sequence");
            auto foreground=original;foreground.quads={original.quads.back()};
            foreground.frontend_ranges={{render::FrontendStage::foreground,0,1}};
            render::append_frontend_frame(combined,foreground);check_stage_coverage(combined);
            check(combined.frontend_ranges.size()==4 && combined.frontend_ranges.back().count==2,
                  "adjacent equal-stage ranges did not merge");
            render::append_frontend_frame(foreground,foreground);check_stage_coverage(foreground);
            check(foreground.quads.size()==2 && foreground.frontend_ranges.size()==1,
                  "self append corrupted frame/ranges");
            combined.quads.push_back(original.quads.front());
            check(!render::frontend_ranges_complete(combined),"legacy untagged append reported complete");
            render::append_frontend_frame(combined,foreground);
            check(!render::frontend_ranges_complete(combined),"tagged append concealed a legacy gap");
            auto unknown=foreground;unknown.frontend_ranges[0].stage=render::FrontendStage::unspecified;
            check(!render::frontend_ranges_complete(unknown),"unspecified stage reported complete");
            auto invalid=original;invalid.frontend_ranges[1].first=1;
            check(!render::frontend_ranges_complete(invalid),"overlapping ranges reported complete");
            const auto size=combined.quads.size();bool rejected=false;
            try{render::append_frontend_frame(combined,invalid);}catch(const std::invalid_argument&){rejected=true;}
            check(rejected && combined.quads.size()==size,"invalid range append mutated destination");
        }
        // E-0039-Q02/Q03: distinct authored corners survive the presentation boundary.
        screen.widgets[0].field_0x5c = {0x40008000u,0x20800000u,0x10808000u};
        screen.widgets[0].field_0x70 = {0.25f,0,0.75f,0,0.75f,1,0.25f,1};
        runtime::build_frame(screen,sizes,{},frame);
        const auto& corners = frame.quads[1];
        check(corners.has_vertex_data && corners.colors[1].g==1 && corners.colors[1].a==0.5f &&
              corners.colors[2].b==1 && corners.colors[3].a==0.125f,
              "four independent corner colours retained");
        check(corners.source_points[0].x==16.5f && corners.source_points[1].x==47.5f &&
              corners.source_points[2].y==31.5f,"authored UVs and half-texel inset retained");
        hp2::frontend::SetAlpha(screen.widgets[0],64);
        runtime::build_frame(screen,sizes,{},frame);
        for (const auto& c : frame.quads[1].colors) check(c.a==0.5f,"001411D8 quad alpha updates all corners");
        screen.widgets[0].visible=2;
        runtime::build_frame(screen,sizes,{},frame);
        check(frame.quads.size()==1,"00145F30 visibility must equal1");
        screen.widgets[0]=panel;

        // Invisible widgets are not drawn, and a frame is a fresh list each time.
        const auto previous_sequence = frame.sequence;
        screen.widgets[0].visible = 0;
        stats = runtime::build_frame(screen, sizes, {}, frame);
        check(stats.quads == 1 && frame.quads.size() == 1, "invisible widget skipped");
        check(frame.sequence == previous_sequence + 1, "sequence advances per frame");
        screen.widgets[0].visible = 1;

        // A widget whose texture is absent from the original data is reported, never faked.
        screen.widgets.push_back(quad_widget("ABSENT", 0, 0, 0xdeadbeef, 0));
        stats = runtime::build_frame(screen, sizes, {}, frame);
        check(stats.missing == 1 && stats.quads == 2, "missing texture reported, not substituted");
        screen.widgets.pop_back();

        // --- text: per-glyph advance and the align bit -------------------------------------
        hp2::frontend::FeScreen text_screen;
        text_screen.widgets = {text_widget("LEFT", 10, 20, kFont, "ABC", 2)};
        stats = runtime::build_frame(text_screen, sizes, {}, frame);
        check(stats.glyphs == 3 && stats.texts == 1, "glyph count");
        check(frame.quads[0].destination.x == 10 && frame.quads[1].destination.x == 20 &&
                  frame.quads[2].destination.x == 30, "glyph advance");
        check(frame.quads[0].source.x == 0.5f && frame.quads[2].source.x == 20.5f,
              "001422F0 half-texel glyph UVs");
        check(frame.quads[0].destination.height == 12, "glyph height");
        check(frame.quads[0].texture == kFont, "glyphs sample the font atlas");

        // align bit 0x04 centres the run on the authored x: 3 glyphs x 10 advance = 30 wide.
        text_screen.widgets = {text_widget("CENTRED", 320, 20, kFont, "ABC", 4)};
        runtime::build_frame(text_screen, sizes, {}, frame);
        check(frame.quads[0].destination.x == 305, "centred text origin");

        text_screen.widgets[0].text_align = 5; // right must win over centre
        runtime::build_frame(text_screen, sizes, {}, frame);
        check(frame.quads[0].destination.x == 290, "001424F4 right alignment takes precedence");

        // E-0039-F01/F02: synthetic serialized font with header-relative table offsets,
        // current-glyph pair slices, a nonzero first pair index and a misleading offset28.
        std::vector<std::uint8_t> font_bytes(28 + 3 * 32 + 2 * 8);
        const auto put = [&](std::size_t off, std::uint32_t value) {
            for (int i = 0; i < 4; ++i) font_bytes[off + i] = static_cast<std::uint8_t>(value >> (8*i));
        };
        const auto put_float = [&](std::size_t off, float value) {
            std::uint32_t bits; std::memcpy(&bits, &value, 4); put(off, bits);
        };
        put(0, hp2::frontend::kFontChunkId); put(4, static_cast<std::uint32_t>(font_bytes.size()-8));
        put(8, kFont); put(12, 3); put(16, 2); put(20, 28); put(24, 124);
        for (int i = 0; i < 3; ++i) {
            const auto off = 28 + i*32;
            put(off, static_cast<std::uint32_t>(65+i) | (i==1 ? 0x10000u : 0));
            put_float(off+12, 10); put_float(off+16, 98); put_float(off+20, 10);
            put(off+28, i==1 ? 1 : 0);
        }
        put(124, 'Z'); put_float(128, -7); put(132, 'A'); put_float(136, -2);
        auto decoded = hp2::frontend::parse_fonts(font_bytes);
        check(decoded.size()==1 && decoded[0].line_height==98,
              "0013FFA8 derives maximum height rather than using table offset28");
        check(decoded[0].glyphs[1].field_0x1c==1 && decoded[0].field_0x410.size()==2,
              "font pair slices and table offsets retained");
        runtime::WidgetSizes kern_sizes(resources, decoded);
        text_screen.widgets = {text_widget("PAIR", 100, 0, kFont, "ABC", 4)};
        text_screen.widgets[0].scale_x = 2;
        runtime::build_frame(text_screen, kern_sizes, {}, frame);
        check(frame.quads[0].destination.x==70 && frame.quads[1].destination.x==86 &&
              frame.quads[2].destination.x==102,
              "measure excludes kerning; current glyph shifts then scaled adjustment advances pen");
        decoded[0].field_0x410[1].second = 41;
        runtime::build_frame(text_screen, kern_sizes, {}, frame);
        check(frame.quads[1].destination.x==90 && frame.quads[2].destination.x==110,
              "scaled kerning over80 is discarded");
        decoded[0].field_0x410[1].second = 40;
        runtime::build_frame(text_screen, kern_sizes, {}, frame);
        check(frame.quads[1].destination.x==170 && frame.quads[2].destination.x==270,
              "scaled kerning exactly80 is retained");
        put(24, 0xfffffff0u);
        check(hp2::frontend::parse_fonts(font_bytes).empty(), "invalid pair offset rejected");
        put(16,0); // shipped BUTTONSOUT has an out-of-bounds offset but zero pair count
        check(hp2::frontend::parse_fonts(font_bytes).size()==1,
              "zero-pair font does not dereference its unused serialized offset");

        //00141DD8: embedded byte80 draws BUTTONSOUT 'a', returns its unscaled advance.
        auto icon_font = make_font(hp2::frontend::bStringHash("BUTTONSOUT"));
        icon_font.glyphs[0].code = 'a'; icon_font.glyphs[0].advance = 7;
        std::vector<hp2::frontend::FeFont> icon_fonts{make_font(kFont),icon_font};
        runtime::WidgetSizes icon_sizes(resources,icon_fonts);
        auto hint = text_widget("HINT",0,0,kFont,"",2);
        hint.text = std::string(1,static_cast<char>(0x80))+"A";
        hint.scale_x=2;
        text_screen.widgets={hint};
        runtime::build_frame(text_screen,icon_sizes,{},frame);
        check(frame.quads.size()==2 && frame.quads[0].texture==icon_font.texture_hash &&
              frame.quads[0].destination.width==40 && frame.quads[1].destination.x==7,
              "embedded button font, squared helper scale and unscaled advance");

        //00141F70: fixed-cell origin correction is unscaled, unlike the cell advance.
        auto fixed_font=make_font(kFont);
        fixed_font.glyphs[0].code='1'; fixed_font.glyphs[0].advance=6;
        fixed_font.glyphs[1].code='2'; fixed_font.glyphs[1].advance=10;
        std::vector<hp2::frontend::FeFont> fixed_fonts{fixed_font};
        runtime::WidgetSizes fixed_sizes(resources,fixed_fonts);
        auto digits=text_widget("DIGITS",100,0,kFont,"12",0x44);
        digits.scale_x=2;
        text_screen.widgets={digits};
        runtime::build_frame(text_screen,fixed_sizes,{},frame);
        check(frame.quads[0].destination.x==82 && frame.quads[1].destination.x==100 &&
              frame.quads[0].destination.width==40,"fixed-cell draw uses numeral max and helper scaling");

        // Scale multiplies both the advance and the glyph cell.
        auto scaled = text_widget("SCALED", 0, 0, kFont, "AB", 2);
        scaled.scale_x = 2;
        scaled.scale_y = 3;
        text_screen.widgets = {scaled};
        runtime::build_frame(text_screen, sizes, {}, frame);
        check(frame.quads[1].destination.x == 20, "scaled advance");
        check(frame.quads[0].destination.width == 20 && frame.quads[0].destination.height == 36,
              "scaled glyph cell");

        // Characters the original font has no glyph for contribute nothing.
        text_screen.widgets = {text_widget("GAP", 0, 0, kFont, "AZB", 2)};
        runtime::build_frame(text_screen, sizes, {}, frame);
        check(frame.quads.size() == 2 && frame.quads[1].destination.x == 10,
              "unmapped character emits nothing and does not advance");

        // A text widget whose font atlas is not among the original tables is reported.
        text_screen.widgets = {text_widget("NOFONT", 0, 0, 0xabcdabcd, "ABC", 2)};
        stats = runtime::build_frame(text_screen, sizes, {}, frame);
        check(stats.missing == 1 && frame.quads.empty(), "missing font reported");

        // --- original clip rectangles ------------------------------------------------------
        // A clip widget named by clip_hash_0x4c scissors the quad and shifts its uvs together,
        // which is what keeps a partially scrolled glyph partial instead of stretched.
        hp2::frontend::FeScreen clipped;
        auto mask = quad_widget("MASK", 100, 100, kPanel, 0); // 64x32 at (100,100)
        auto inside = quad_widget("INSIDE", 80, 90, kPanel, 0);
        inside.clip_hash_0x4c = mask.name_hash;
        clipped.widgets = {inside, mask};
        stats = runtime::build_frame(clipped, sizes, {}, frame);
        check(stats.quads == 2, "clipped quad still drawn");
        // ELF 00145F30 gives a quad the mask's own bounds as its scissor (a text widget instead
        // gets mask.x + 4 and needs text_align bit 0x10), so the 64x32 panel authored at (80,90)
        // keeps only the part inside (100,100)-(164,132).
        const render::Quad* cropped = nullptr;
        for (const auto& quad : frame.quads)
            if (quad.destination.x == 100 && quad.destination.width == 44) cropped = &quad;
        check(cropped != nullptr, "quad cropped to the mask left edge");
        check(cropped->destination.y == 100 && cropped->destination.height == 22,
              "quad cropped to the mask top edge");
        check(cropped->source.x == 20 && cropped->source.y == 10, "uv shifted with the crop");
        check(cropped->source.width == 44 && cropped->source.height == 22,
              "crop preserves the 1:1 panel mapping");

        // The same widget as text is only clipped when text_align carries bit 0x10.
        auto clipped_text = text_widget("LABEL", 80, 90, kFont, "ABC", 2);
        clipped_text.clip_hash_0x4c = mask.name_hash;
        clipped.widgets = {clipped_text, mask};
        runtime::build_frame(clipped, sizes, {}, frame);
        const auto unclipped=std::find_if(frame.quads.begin(),frame.quads.end(),
            [](const auto& q){return q.texture==kFont;});
        check(unclipped!=frame.quads.end() && unclipped->destination.x==80,
              "text without bit 0x10 is unclipped");
        clipped.widgets[0].text_align = 2 | 0x10;
        runtime::build_frame(clipped, sizes, {}, frame);
        for (const auto& quad : frame.quads)
            if (quad.texture == kFont) check(quad.destination.x >= 104, "clipped text left edge");

        // Fully outside the clip: nothing is emitted and it is counted, not silently dropped.
        clipped.widgets[0] = inside;
        clipped.widgets[0].x = -500;
        clipped.widgets[0].y = -500;
        stats = runtime::build_frame(clipped, sizes, {}, frame);
        check(stats.quads == 1 && stats.clipped_out == 1, "quad outside its clip reported");
        check_stage_coverage(frame);
        check(frame.frontend_ranges.size()==1 && frame.frontend_ranges[0].count==1,
              "clipped-away quad left a stage packet entry");

        // --- startup movie override --------------------------------------------------------
        hp2::frontend::FeScreen title;
        title.widgets = {quad_widget("TITLE_LETTERBOX", 64, 112, 0, 0),
                         text_widget("PRESS_START_1", 320, 400, kFont, "ABC", 4)};
        runtime::FrameOptions options;
        options.exclusive_widget = "TITLE_LETTERBOX";
        options.exclusive_texture = kMovie;
        options.exclusive_width = 512;
        options.exclusive_height = 256;
        stats = runtime::build_frame(title, sizes, options, frame);
        check_stage_coverage(frame);
        check(frame.frontend_ranges.size()==1 &&
              frame.frontend_ranges[0].stage==render::FrontendStage::foreground,
              "exclusive startup quad did not retain its authored stage");
        check(stats.quads == 1 && frame.quads.size() == 1, "movie override draws only the movie");
        check(frame.quads[0].texture == kMovie, "movie slot texture id preserved");
        check(frame.quads[0].destination.width == 512 && frame.quads[0].destination.height == 256,
              "movie source size, not an authored texture size");

        // Without the override, the suppressed widget is the only thing held back.
        runtime::FrameOptions suppress;
        suppress.suppressed_widget = "TITLE_LETTERBOX";
        stats = runtime::build_frame(title, sizes, suppress, frame);
        check(stats.glyphs == 3 && stats.quads == 0, "suppressed widget withheld");

        // E-0022-R04: detail wrapping and explicit '@' rows use the font's line height.
        hp2::frontend::FeScreen detail;
        detail.widgets = {text_widget("TRACK_LINE_1", 0, 0, kFont, "AB C@A", 2)};
        runtime::FrameOptions wrap;
        wrap.wrapped_text_hash = detail.widgets[0].name_hash;
        wrap.wrapped_text_bounds = {0, 0, 25, 60};
        render::Frame wrapped_frame;
        runtime::build_frame(detail, sizes, wrap, wrapped_frame);
        check(wrapped_frame.quads.size() == 4, "wrapped text keeps all letters and removes break markers");
        check(wrapped_frame.quads[2].destination.y == 12 &&
              wrapped_frame.quads[3].destination.y == 24, "word lookahead and explicit break use successive rows");

        // --- the frame crosses a backend boundary with no device type ----------------------
        FakeBackend backend;
        resources.add(kMovie, 512, 256);
        backend.submit(frame, resources);
        title.widgets[1].text = "AB";
        runtime::build_frame(title, sizes, suppress, frame);
        backend.submit(frame, resources);
        check(backend.submitted.size() == 2, "backend received both frames");
        check(backend.submitted[0].quads.size() == 3 && backend.submitted[1].quads.size() == 2,
              "backend sees the live text change");
        check(backend.uploaded.count(kFont) == 1 && backend.uploaded[kFont] == 1,
              "backend tracks texture revision");

        std::cout << "FRAME_BUILDER_TESTS_OK\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "frame_builder_tests failed: " << error.what() << "\n";
        return 1;
    }
}
