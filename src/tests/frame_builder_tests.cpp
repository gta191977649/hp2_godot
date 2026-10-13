// TASK-0018 / D008: the port runtime must produce the front end's frames with no backend and
// no Godot. A fake Resources supplies original-shaped texture dimensions and a fake Backend
// records what a device would have received.
#include "port/runtime/frame_builder.hpp"
#include <iostream>
#include <map>
#include <stdexcept>

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

hp2::frontend::FeWidget quad_widget(const char* name, float x, float y, std::uint32_t texture,
                                    std::uint8_t layer) {
    hp2::frontend::FeWidget w;
    w.chunk_id = hp2::frontend::kQuadWidgetId;
    w.type = 2;
    w.visible = 1;
    w.layer = layer;
    w.name = name;
    w.name_hash = hp2::frontend::StringHash_002534A8(name);
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
    w.name_hash = hp2::frontend::StringHash_002534A8(name);
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

        // Invisible widgets are not drawn, and a frame is a fresh list each time.
        screen.widgets[0].visible = 0;
        stats = runtime::build_frame(screen, sizes, {}, frame);
        check(stats.quads == 1 && frame.quads.size() == 1, "invisible widget skipped");
        check(frame.sequence == 2, "sequence advances per frame");
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
        check(frame.quads[0].source.x == 0 && frame.quads[2].source.x == 20, "glyph atlas uv");
        check(frame.quads[0].destination.height == 12, "glyph height");
        check(frame.quads[0].texture == kFont, "glyphs sample the font atlas");

        // align bit 0x04 centres the run on the authored x: 3 glyphs x 10 advance = 30 wide.
        text_screen.widgets = {text_widget("CENTRED", 320, 20, kFont, "ABC", 4)};
        runtime::build_frame(text_screen, sizes, {}, frame);
        check(frame.quads[0].destination.x == 305, "centred text origin");

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
        check(frame.quads[0].destination.x == 80, "text without bit 0x10 is unclipped");
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
