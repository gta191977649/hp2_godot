#include "port/runtime/frame_builder.hpp"
#include <algorithm>
#include <numeric>

namespace port::runtime {
namespace {

// PS2 colour bytes: 0x80 is the neutral/opaque value, the same conversion the earlier host
// draw path used.
render::Color widget_colour(const hp2::frontend::FeWidget& w) {
    return {static_cast<float>(w.r) / 128.0f, static_cast<float>(w.g) / 128.0f,
            static_cast<float>(w.b) / 128.0f, static_cast<float>(w.a) / 128.0f};
}

render::Rect clip_of(hp2::frontend::FeScreen& screen, const hp2::frontend::FeWidget& w,
                     const hp2::frontend::WidgetMeasure& measure) {
    const auto clip = hp2::frontend::GetWidgetClip_00145F30(screen, w, measure);
    if (!clip.valid) return {};
    return {clip.x0, clip.y0, clip.x1 - clip.x0, clip.y1 - clip.y0};
}

// Appends `quad` after applying the original clip rectangle. Returns false when the clip
// removes it entirely.
bool emit(render::Frame& out, render::Quad quad, const render::Rect& clip) {
    if (!render::crop(quad, clip)) return false;
    out.quads.push_back(quad);
    return true;
}

} // namespace

const hp2::frontend::FeFont* WidgetSizes::font(std::uint32_t texture_hash) const {
    for (const auto& font : fonts_) {
        if (font.texture_hash == texture_hash) return &font;
    }
    return nullptr;
}

float WidgetSizes::text_advance(const hp2::frontend::FeFont& font, const std::string& text) {
    float total = 0;
    for (const unsigned char code : text) {
        for (const auto& glyph : font.glyphs) {
            if (glyph.code == code) { total += glyph.advance; break; }
        }
    }
    return total;
}

hp2::frontend::WidgetMetrics WidgetSizes::base(const hp2::frontend::FeWidget& widget) const {
    const hp2::frontend::FeWidget* measured = &widget;
    if (redirect_) {
        if (const auto* target = redirect_(widget)) measured = target;
        else return {};
    }
    if (measured->chunk_id == hp2::frontend::kTextWidgetId) {
        const auto* font = this->font(measured->texture_hash);
        if (font == nullptr) return {};
        return {text_advance(*font, measured->text), static_cast<float>(font->line_height)};
    }
    if (measured->texture_hash == 0) return {};
    const auto bitmap = resources_.texture(measured->texture_hash);
    if (!bitmap) return {};
    return {static_cast<float>(bitmap->width), static_cast<float>(bitmap->height)};
}

hp2::frontend::WidgetMetrics WidgetSizes::scaled(const hp2::frontend::FeWidget& widget) const {
    const auto b = base(widget);
    return {b.width * widget.scale_x, b.height * widget.scale_y};
}

hp2::frontend::WidgetMeasure WidgetSizes::measure() const {
    return [this](const hp2::frontend::FeWidget& widget) { return scaled(widget); };
}

FrameStats build_frame(hp2::frontend::FeScreen& screen, const WidgetSizes& sizes,
                       const FrameOptions& options, render::Frame& out) {
    FrameStats stats;
    out.quads.clear();
    out.logical_size = {static_cast<float>(screen.width), static_cast<float>(screen.height)};
    ++out.sequence;

    if (!options.exclusive_widget.empty()) {
        // The intro movie owns the screen: the original leaves only the movie widget visible,
        // at the authored transform of that widget and the movie's own source size.
        for (const auto& w : screen.widgets) {
            if (w.name != options.exclusive_widget) continue;
            render::Quad quad;
            quad.texture = options.exclusive_texture;
            quad.destination = {w.x, w.y, options.exclusive_width * w.scale_x,
                                options.exclusive_height * w.scale_y};
            quad.source = {0, 0, options.exclusive_width, options.exclusive_height};
            quad.color = {1, 1, 1, 1};
            if (emit(out, quad, {})) ++stats.quads;
        }
        return stats;
    }

    // The authored layer byte counts down toward the viewer (grid/blueprint backdrops carry the
    // larger values, menu text carries 0), so paint descending, preserving authored order
    // within a layer.
    std::vector<std::size_t> order(screen.widgets.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::stable_sort(order.begin(), order.end(), [&screen](std::size_t a, std::size_t b) {
        return screen.widgets[a].layer > screen.widgets[b].layer;
    });

    const auto measure = sizes.measure();
    for (const std::size_t slot : order) {
        const auto& w = screen.widgets[slot];
        if (w.visible == 0) continue;
        if (!options.suppressed_widget.empty() && w.name == options.suppressed_widget) continue;

        const auto clip = clip_of(screen, w, measure);
        const auto colour = widget_colour(w);

        if (w.chunk_id == hp2::frontend::kTextWidgetId) {
            const auto* font = sizes.font(w.texture_hash);
            if (font == nullptr) { ++stats.missing; continue; }
            if (w.text.empty()) continue;
            float pen_x = w.x;
            // align bit 0x04 centres the string on the authored x (verified against
            // PRESS_START_1 at x=320 and the right-hand stat column at x=494).
            if ((w.text_align & 4) != 0)
                pen_x -= WidgetSizes::text_advance(*font, w.text) * w.scale_x * 0.5f;
            int emitted = 0;
            for (const unsigned char code : w.text) {
                const hp2::frontend::FeGlyph* glyph = nullptr;
                for (const auto& candidate : font->glyphs) {
                    if (candidate.code == code) { glyph = &candidate; break; }
                }
                if (glyph == nullptr) continue;
                render::Quad quad;
                quad.texture = w.texture_hash;
                quad.destination = {pen_x, w.y, glyph->w * w.scale_x, glyph->h * w.scale_y};
                quad.source = {glyph->u, glyph->v, glyph->w, glyph->h};
                quad.color = colour;
                if (emit(out, quad, clip)) { ++stats.glyphs; ++emitted; }
                pen_x += glyph->advance * w.scale_x;
            }
            if (emitted > 0) ++stats.texts;
            else if (!w.text.empty()) ++stats.clipped_out;
            continue;
        }

        if (w.texture_hash == 0) continue;
        const auto bitmap = sizes.base(w);
        if (bitmap.width <= 0 || bitmap.height <= 0) { ++stats.missing; continue; }
        render::Quad quad;
        quad.texture = w.texture_hash;
        quad.destination = {w.x, w.y, bitmap.width * w.scale_x, bitmap.height * w.scale_y};
        quad.source = {0, 0, bitmap.width, bitmap.height};
        quad.color = colour;
        if (emit(out, quad, clip)) ++stats.quads;
        else ++stats.clipped_out;
    }
    return stats;
}

} // namespace port::runtime
