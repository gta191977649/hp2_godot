#pragma once
#include "profile_debug_ui.hpp"
#include "ps2_frontend_layout.hpp"
#include "ps2_movie_player.hpp"
#include "port/render/frame.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <map>
#include <godot_cpp/classes/input_event.hpp>
#include "game_core/extensions/viewer_session.hpp"
#if HP2_DEBUG_BUILD
#include "game_core/extensions/debug_console.hpp"
#include "game_core/extensions/viewer_ui.hpp"
#endif

// TASK-0018 / D008: the Godot rendering backend. This is the only place in the port that knows
// about a graphics device: it owns the RenderingServer RIDs (one canvas item, one 2D texture per
// original TexturePack entry actually drawn) and consumes port::render::Frame, which carries no
// Godot or device type. Replacing this file replaces the renderer; nothing above it changes.
//
// It makes no drawing decisions. Draw order, geometry, colour, glyph placement and the original
// clip rectangles are all resolved natively in port::runtime::build_frame before a frame arrives.
namespace port::godot_bridge {

// Resolves the ids in a frame: original TexturePack entries come from the Godot-free catalog,
// the two live movie surfaces from the host decoders. Lives for one draw call.
class LiveResources final : public port::render::Resources {
public:
    LiveResources(port::render::Resources* catalog, std::shared_ptr<const port::render::Bitmap> intro,
                  std::shared_ptr<const port::render::Bitmap> background)
        : catalog_(catalog), intro_(std::move(intro)), background_(std::move(background)) {}
    std::shared_ptr<const port::render::Bitmap> texture(port::render::TextureId id) override;

private:
    port::render::Resources* catalog_;
    std::shared_ptr<const port::render::Bitmap> intro_, background_;
};

class Ps2RenderHost : public godot::RefCounted {
    GDCLASS(Ps2RenderHost, godot::RefCounted)
protected:
    static void _bind_methods();

public:
    ~Ps2RenderHost() override;
    bool is_debug_build() const { return HP2_DEBUG_BUILD != 0; }
    bool console_available() const { return HP2_ENABLE_DEBUG_CONSOLE != 0; }
    bool console_input(const godot::Ref<godot::InputEvent>& event);
    godot::Dictionary console_frame(double width, double height, double delta,
                                    const godot::Dictionary& status);
    double get_ui_scale()const;
    void console_init(const godot::String& settings, bool info);
    void console_log(const godot::String& text, bool error);
    void console_execute(const godot::String& command);
    void console_focus_lost();
    bool console_visible() const;
    godot::Dictionary viewer_request(const godot::String& action, const godot::String& source);
    godot::Dictionary viewer_next();
    godot::Dictionary viewer_activate(const godot::String& kind);
    godot::Dictionary viewer_apply(const godot::String& action, const godot::String& source);
    godot::Dictionary viewer_state() const;
    void viewer_camera(const godot::String& description);
    godot::Dictionary viewer_loaded(bool success, const godot::String& diagnostic, const godot::String& rendering);
    void viewer_rendering(const godot::String& description);
    void viewer_ui_snapshot(const godot::Dictionary& snapshot);
    godot::Dictionary viewer_ui_next();
    godot::Dictionary viewer_ui_targets()const;
    bool viewer_ui_capture()const;
    bool viewer_keyboard_capture()const;

    // Creates the canvas item under an existing CanvasItem RID, i.e. a Node2D's or Control's
    // get_canvas_item(). Godot keeps owning the window, viewport and canvas; this owns the item.
    void attach(const godot::RID& parent_canvas_item);

    // Window scaling is a backend concern: frames stay in the original 640x480 authored space.
    void set_output_scale(double scale);
    double get_output_scale() const { return output_scale_; }

    // The host's whole per-frame entry point. Pulls one native frame from the runtime and
    // submits it. `intro_movie` and `background_movie` may be null.
    // Returns {"ok": bool, "quads": int, "glyphs": int, "texts": int, "missing": int,
    //          "clipped_out": int, "sequence": int, "textures": int, "movie_quads": int,
    //          "error": String}. "movie_quads" counts the quads that sampled a live movie
    //          surface this frame, which is how a host check confirms the movie reached the
    //          renderer without reading widgets.
    godot::Dictionary draw(godot::Object* runtime, godot::Object* intro_movie,
                           godot::Object* background_movie);

    // Frees every RID this host owns. Called from the destructor as well.
    void release();

    int get_texture_count() const { return static_cast<int>(textures_.size()); }
    godot::String get_last_error() const { return last_error_; }

private:
    port::extensions::ViewerSession viewers_{HP2_DEBUG_BUILD != 0};
#if HP2_DEBUG_BUILD
    std::unique_ptr<port::extensions::DebugConsole> console_;
    port::extensions::ViewerUi viewer_ui_;
    ProfileDebugUi profile_debug_ui_;
    std::string clipboard_text_;
#endif
    godot::RID console_parent_, console_font_;
    std::uint64_t console_font_revision_=0;
    std::vector<godot::RID> console_items_;
    std::vector<godot::RID> console_draw_items_;
    // One device texture per drawn original entry, re-uploaded only when the bitmap revision
    // changes, which is what makes the per-frame movie surfaces cheap.
    struct Uploaded {
        godot::RID rid;
        int width = 0, height = 0;
        std::uint64_t revision = 0;
    };
    godot::RID texture_for(port::render::TextureId id, port::render::Resources& resources);
    // Returns how many quads sampled a live movie surface.
    int submit(const port::render::Frame& frame, port::render::Resources& resources);

    godot::RID item_;
    std::map<port::render::TextureId, Uploaded> textures_;
    double output_scale_ = 1.0;
    godot::String last_error_;
};

} // namespace port::godot_bridge
