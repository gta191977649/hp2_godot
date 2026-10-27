#include "port/runtime/frame_builder.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>
#include "game_core/port/not_implemented.hpp"

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
bool emit(render::Frame& out, render::Quad quad, const render::Rect& clip,
          render::FrontendStage stage,std::int8_t layer) {
    if (!render::crop(quad, clip)) return false;
    quad.frontend_layer=layer;
    const auto first=out.quads.size();
    out.quads.push_back(quad);
    if (!out.frontend_ranges.empty() && out.frontend_ranges.back().stage==stage &&
        out.frontend_ranges.back().first+out.frontend_ranges.back().count==first)
        ++out.frontend_ranges.back().count;
    else out.frontend_ranges.push_back({stage,first,1});
    return true;
}

// Debug00112C60, E-0098-F01: preserve the original view0 submission boundary.
// This metadata does not move the existing quads or implement the intervening3D stages.
render::FrontendStage frontend_stage(std::int8_t layer) {
    if (layer>=20 && layer<=24) return render::FrontendStage::background;
    if (layer>=0 && layer<=28) return render::FrontendStage::foreground;
    return render::FrontendStage::unspecified;
}

// Port projection of00140098, E-0039-F02: pairs belong to the CURRENT glyph,
// and their byte identifies the PREVIOUS character. This is deliberately not measurement.
float pair_adjustment(const hp2::frontend::FeFont& font,
                      const hp2::frontend::FeGlyph& glyph, unsigned char previous) {
    const auto count = static_cast<std::int16_t>(glyph.flags);
    for (int i = 0; i < count; ++i) {
        const auto index = static_cast<std::size_t>(glyph.field_0x1c) + i;
        if (index >= font.field_0x410.size())
            hp2::not_implemented("0x00140098", "font pair range outside original table");
        const auto& pair = font.field_0x410[index];
        if (pair.first == previous) return pair.second;
    }
    return 0;
}

// E-0039-F04,00141DB0/00141DD8 and table002F09C0. Byte92 remains ordinary text.
constexpr unsigned char kSub_00141DD8_002F09C0[] = {
    'a','b','d','e','f','g','i','j','h','k','n','o','l','m','3','w',
    'p','v',0x92,'u','q','r','t','s','c','x','2','y','z','1'};
bool inline_button(unsigned char code) { return code >= 0x80 && code <= 0x9d && code != 0x92; }

float fixed_cell(const hp2::frontend::FeFont& font) {
    //0013FFA8 ->001401D8 over the verbatim string at002AF1A0.
    float width = 0;
    for (const unsigned char code : std::string("1234567890:- "))
        for (const auto& glyph : font.glyphs)
            if (glyph.code == code && width < glyph.advance) width = glyph.advance;
    return width;
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
    out.frontend_ranges.clear();
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
            if (emit(out, quad, {}, frontend_stage(static_cast<std::int8_t>(w.layer)),static_cast<std::int8_t>(w.layer))) ++stats.quads;
        }
        return stats;
    }

    // E-0095-N04:0013FBA0 appends authored nodes;00145F30 traverses
    // their stable ascending layer list backwards INTO layer buckets. Actual
    // submission00112858 consumes24..20,15..0,19..16,28..25 in separate phases.
    // Keyboard layers16..19 therefore overlay ordinary frontend layers0..15.
    const auto phase=[](std::int8_t layer) {
        if(layer>=20 && layer<=24)return 0;
        if(layer>=0 && layer<=15)return 1;
        if(layer>=16 && layer<=19)return 2;
        if(layer>=25 && layer<=28)return 3;
        hp2::not_implemented("0x00145D38","frontend layer bucket outside recovered render phases");
    };
    std::vector<std::size_t> order;
    for(std::size_t i=0;i<screen.widgets.size();++i)if(screen.widgets[i].visible==1) {
        phase(static_cast<std::int8_t>(screen.widgets[i].layer));
        order.push_back(i);
    }
    std::stable_sort(order.begin(), order.end(), [&screen,&phase](std::size_t a, std::size_t b) {
        const auto first=static_cast<std::int8_t>(screen.widgets[a].layer);
        const auto second=static_cast<std::int8_t>(screen.widgets[b].layer);
        const int first_phase=phase(first),second_phase=phase(second);
        if(first_phase!=second_phase)return first_phase<second_phase;
        return first!=second ? first>second : a>b;
    });

    const auto measure = sizes.measure();
    for (const std::size_t slot : order) {
        const auto& w = screen.widgets[slot];
        const auto stage=frontend_stage(static_cast<std::int8_t>(w.layer));
        if (w.visible != 1) continue; //00145F30, E-0039-Q08
        if (!options.suppressed_widget.empty() && w.name == options.suppressed_widget) continue;

        auto clip = clip_of(screen, w, measure);
        const bool wrapped = options.wrapped_text_hash != 0 && w.name_hash == options.wrapped_text_hash;
        if (wrapped) clip = options.wrapped_text_bounds;
        const auto colour = widget_colour(w);

        if (w.chunk_id == hp2::frontend::kTextWidgetId) {
            const auto* font = sizes.font(w.texture_hash);
            if (font == nullptr) { ++stats.missing; continue; }
            if (w.text.empty()) continue;
            const bool fixed = !wrapped && (w.text_align & 0x40) != 0;
            const float cell = fixed ? fixed_cell(*font) : 0;
            float align_width = WidgetSizes::text_advance(*font, w.text);
            if (fixed) {
                align_width = 0;
                for (const unsigned char code : w.text)
                    for (const auto& glyph : font->glyphs) if (glyph.code == code) { align_width += cell; break; }
            }
            float pen_x = w.x;
            //001424F4..00142524: right(bit1) takes precedence over centre(bit4).
            //00140D30 measures advances WITHOUT kerning, unlike the subsequent draw walk.
            if ((w.text_align & 1) != 0)
                pen_x -= align_width * w.scale_x;
            else if ((w.text_align & 4) != 0)
                pen_x -= align_width * w.scale_x * 0.5f;
            int emitted = 0;
            float pen_y = w.y;
            for (std::size_t character = 0; character < w.text.size(); ++character) {
                const unsigned char code = static_cast<unsigned char>(w.text[character]);
                if (wrapped) {
                    bool next_line = code == '@';
                    if (code == ' ' || code == '\t') {
                        std::size_t end = character + 1;
                        while (end < w.text.size() && w.text[end] != ' ' && w.text[end] != '\t') ++end;
                        const float word_width = WidgetSizes::text_advance(*font,
                            w.text.substr(character + 1, end - character - 1)) * w.scale_x;
                        float space_width = 0;
                        for (const auto& glyph : font->glyphs) if (glyph.code == code)
                            space_width = glyph.advance * w.scale_x;
                        next_line = pen_x + space_width + word_width > clip.x + clip.width;
                    }
                    if (next_line) {
                        pen_x = w.x;
                        pen_y += static_cast<float>(font->line_height);
                        while (character + 1 < w.text.size() &&
                               (w.text[character + 1] == ' ' || w.text[character + 1] == '\t')) ++character;
                        continue;
                    }
                }
                if (!fixed && inline_button(code)) {
                    const auto* buttons = sizes.font(hp2::frontend::bStringHash("BUTTONSOUT"));
                    if (buttons == nullptr) { ++stats.missing; continue; }
                    const auto mapped = kSub_00141DD8_002F09C0[code-0x80];
                    for (const auto& icon : buttons->glyphs) if (icon.code == mapped) {
                        render::Quad quad;
                        quad.texture = buttons->texture_hash;
                        //00141A98 helper width uses scale twice;00141DD8 returns UNSCALED advance.
                        quad.destination = {pen_x,pen_y,icon.w*w.scale_x*w.scale_x,icon.h*w.scale_y};
                        quad.source = {icon.u+0.5f,icon.v+0.5f,icon.w,icon.h};
                        quad.color = colour;
                        if (emit(out,quad,clip,stage,static_cast<std::int8_t>(w.layer))) { ++stats.glyphs; ++emitted; }
                        pen_x += icon.advance;
                        break;
                    }
                    continue;
                }
                const hp2::frontend::FeGlyph* glyph = nullptr;
                for (const auto& candidate : font->glyphs) {
                    if (candidate.code == code) { glyph = &candidate; break; }
                }
                if (glyph == nullptr) continue;
                float adjustment = fixed || character == 0 ? 0 : pair_adjustment(*font, *glyph,
                    static_cast<unsigned char>(w.text[character - 1])) * w.scale_x;
                //0014261C..00142640 clamps the SCALED offset; wrapped00144ED0 has no clamp.
                if (!wrapped && std::abs(adjustment) > 80.0f) adjustment = 0;
                render::Quad quad;
                quad.texture = w.texture_hash;
                //00141A98 (wrapped helper) multiplies X scale twice; normal001422F0 once.
                quad.destination = {pen_x + adjustment + (fixed ? (cell-glyph->advance)*0.5f : 0), pen_y,
                    glyph->w * w.scale_x * (wrapped || fixed ? w.scale_x : 1.0f), glyph->h * w.scale_y};
                //00142694..001426D4: equal half-texel offsets at both UV edges, same extent.
                quad.source = {glyph->u + 0.5f, glyph->v + 0.5f, glyph->w, glyph->h};
                quad.color = colour;
                if (emit(out, quad, clip,stage,static_cast<std::int8_t>(w.layer))) { ++stats.glyphs; ++emitted; }
                // Preserve the original double application of scale to kerning.00142844..54.
                const float step = (fixed ? cell : glyph->advance + adjustment) * w.scale_x;
                pen_x += wrapped ? std::nearbyint(step) : step;
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
        //001428F0, E-0039-Q02/Q03: preserve all four authored UVs and colours.
        // Original FCR31 conversion is unresolved; retain existing fractional dimensions.
        if (w.field_0x44 == 2 && w.field_0x40 != 0)
            hp2::not_implemented("0x001428F0", "frontend rotated quad matrix not translated");
        quad.has_vertex_data = true;
        const auto& d = quad.destination;
        quad.positions = {render::Vec2{d.x,d.y}, render::Vec2{d.x+d.width,d.y},
            render::Vec2{d.x+d.width,d.y+d.height}, render::Vec2{d.x,d.y+d.height}};
        quad.colors[0] = colour;
        for (std::size_t i = 0; i < 4; ++i) {
            quad.source_points[i] = {bitmap.width*w.field_0x70[i*2] + (i==0 || i==3 ? 0.5f : -0.5f),
                bitmap.height*w.field_0x70[i*2+1] + (i<2 ? 0.5f : -0.5f)};
            if (i==0) continue;
            const auto c = w.field_0x5c[i-1];
            quad.colors[i] = {static_cast<float>(c&255)/128, static_cast<float>((c>>8)&255)/128,
                static_cast<float>((c>>16)&255)/128, static_cast<float>(c>>24)/128};
        }
        if (emit(out, quad, clip,stage,static_cast<std::int8_t>(w.layer))) ++stats.quads;
        else ++stats.clipped_out;
    }
    return stats;
}

} // namespace port::runtime
