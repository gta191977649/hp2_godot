#pragma once
#include "game_core/indep/src/fmenus.hpp"
#include "port/render/frame.hpp"
#include <functional>
#include <string>
#include <vector>

// TASK-0018 / D008: turns the live authored front-end screen into a typed render::Frame.
// This is the port's presentation step, not recovered code: the original wrote GS packets.
// What is preserved here is the observable arrangement the recovered records describe --
// authored 640x480 positions, authored scales, the layer draw order, the 0x80-is-opaque
// colour bytes, the per-glyph advances of the original bitmap fonts and the original clip
// rectangles (GetWidgetClip_00145F30). No backend, device or Godot type appears.
namespace port::runtime {

// Port-side texture ids for live host surfaces that have no original TexturePack entry. They sit
// above the 32-bit entry-hash space, so they can never collide with an original texture hash and
// an original-asset catalog reports them as absent.
inline constexpr render::TextureId kIntroMovieTexture = 0x100000001ull;

// The background movie, unlike the intro, does have an authored slot: screens reference it by
// this TexturePack entry hash, which no bundle contains, so the host supplies the live frame.
inline constexpr render::TextureId kBackgroundMovieTexture = 0xc3f0caefull;

// Authored widget size at scale 1, measured from the original font tables and the original
// TexturePack entry dimensions. This replaces the former host measurement round trip: the
// sizes come from the same original data the renderer samples.
class WidgetSizes {
public:
    // Redirects a measurement to another widget, for the recovered highlight ghost that
    // copies its target's font and string every tick (FUN_001410A0). The callback returns the
    // widget to measure -- the argument itself when nothing is redirected -- or nullptr when a
    // redirect target is absent, which measures as zero rather than falling back to the
    // template's authored placeholder text. An empty callback measures every widget directly.
    using Redirect = std::function<const hp2::frontend::FeWidget*(const hp2::frontend::FeWidget&)>;

    WidgetSizes(render::Resources& resources, const std::vector<hp2::frontend::FeFont>& fonts,
                Redirect redirect = {})
        : resources_(resources), fonts_(fonts), redirect_(std::move(redirect)) {}

    const hp2::frontend::FeFont* font(std::uint32_t texture_hash) const;

    // Sum of the original per-glyph advances; characters absent from the font contribute
    // nothing, exactly as the glyph walk in the original draw path does.
    static float text_advance(const hp2::frontend::FeFont& font, const std::string& text);

    // Authored size at scale 1: texture dimensions for a quad, (advance sum, line height)
    // for text. Zero when the texture or font is not available.
    hp2::frontend::WidgetMetrics base(const hp2::frontend::FeWidget& widget) const;

    // base() multiplied by the widget's current scale. Matches the measurement seam the
    // recovered animation and viewport arithmetic consumes.
    hp2::frontend::WidgetMetrics scaled(const hp2::frontend::FeWidget& widget) const;

    // Measurement seam for recovered code (hp2::frontend::WidgetMeasure).
    hp2::frontend::WidgetMeasure measure() const;

private:
    render::Resources& resources_;
    const std::vector<hp2::frontend::FeFont>& fonts_;
    Redirect redirect_;
};

// Counts of what the authored screen asked for and what the original data could supply.
// Reported rather than hidden: a missing texture or font is an unresolved asset, not a
// reason to substitute art.
struct FrameStats {
    int quads = 0;       // textured quads emitted
    int glyphs = 0;      // glyph quads emitted
    int texts = 0;       // text widgets that produced at least one glyph
    int missing = 0;     // widgets skipped because their texture or font is unavailable
    int clipped_out = 0; // widgets fully outside their original clip rectangle
};

struct FrameOptions {
    // Startup title override: while the intro movie owns the screen, the original shows only
    // the movie widget. The named widget is drawn at the movie's authored 512x256 source size
    // and everything else is suppressed. Empty disables the override.
    std::string exclusive_widget;
    render::TextureId exclusive_texture = 0; // a live host surface id, so 64-bit
    float exclusive_width = 0, exclusive_height = 0;
    // Widget suppressed while the startup sequence runs but the movie is not on screen.
    std::string suppressed_widget;
};

// Emits `screen`'s visible widgets into `out`, ordered by descending authored layer and then
// by authored position within a layer. `screen` is non-const because the recovered clip query
// walks the live widget list.
FrameStats build_frame(hp2::frontend::FeScreen& screen, const WidgetSizes& sizes,
                       const FrameOptions& options, render::Frame& out);

} // namespace port::runtime
