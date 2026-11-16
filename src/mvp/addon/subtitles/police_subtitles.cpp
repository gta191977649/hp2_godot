#include "police_subtitles.hpp"

#include "bware/inc/bFile.hpp"
#include "indep/inc/FlailerDB.hpp"
#include "indep/inc/Texture.hpp"
#include "indep/src/speech/SEDWedge.hpp"
#include "bware/inc/bWare.hpp"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/hashing_context.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/rendering_server.hpp>

#include <algorithm>
#include <array>
#include <utility>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <future>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace hp2 {
using namespace godot;
namespace {
using Clock = std::chrono::steady_clock;
constexpr auto catalogPath = "res://addons/hp2_subtitles/subtitles.en.json";
constexpr auto englishPath = "Sound/Speech/EnglishDAT.pck";
constexpr auto englishHash = "27e26947be223b653c7dade002afd964de8bd9792bb6dd3e3de6df4e677b71fe";
// Original radio FSM action event IDs from the recovered speech actions.
constexpr std::uint16_t radioEvents[]{
    0,6,7,8,9,10,11,12,13,14,15,16,17,19,20,21,22,23,24,25,28,
    31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,50,51,52,53,54,
    56,57,58,61,62,63,65,67,71,72,73,81,83,84,85,86,91,92,93,96,97,98,100,
    101,109,112,121,170,171,172,173,174,175,177,178,179,180,181,185,186,187,188,
    189,194,195,198,199,201,202,203
};
// The first split value carries the completion percentage.
constexpr std::uint32_t completionValue = 0x6486623bu;
constexpr std::uint32_t timeLeftLabel = 0x0390d3e1u;
constexpr std::array<std::uint32_t,3> hudScreens{0xf1f1f8e5u,0xa8eb7af0u,0x6283be92u};
constexpr std::array<std::uint32_t,3> gaugeFaces{0x9309e75cu, 0xf446d30eu, 0x01223abdu};

bool verify_english(const FileRange& source, const std::atomic_bool& cancel)
{
    constexpr std::size_t chunk = 1024 * 1024;
    Ref<HashingContext> hash;
    hash.instantiate();
    if (hash->start(HashingContext::HASH_SHA256) != OK) return false;
    PackedByteArray bytes;
    try {
        for (std::uint64_t offset = 0; offset < source.Size(); offset += chunk) {
            if (cancel.load(std::memory_order_relaxed)) return false;
            const auto size = static_cast<std::size_t>(std::min<std::uint64_t>(chunk, source.Size() - offset));
            bytes.resize(static_cast<std::int64_t>(size));
            source.Read(offset, bytes.ptrw(), size);
            if (hash->update(bytes) != OK) return false;
        }
    } catch (const std::exception&) { return false; }
    return hash->finish().hex_encode() == englishHash;
}

std::string byte_font_text(String text)
{
    text = text.replace("’", "'").replace("‘", "'").replace("“", "\"").replace("”", "\"")
        .replace("–", "-").replace("—", "-").replace("…", "...").replace("\n", " ");
    std::string converted;
    for (int64_t i = 0; i < text.length(); ++i) {
        const auto ch = text[i];
        converted += ch >= 32 && ch <= 255 ? static_cast<char>(ch) : '?';
    }
    return converted;
}

float width(const Font& font, std::string_view text)
{
    float result = 0;
    const tTTGlyph* previous = nullptr;
    for (unsigned char code : text) {
        const auto* glyph = font.characters[code];
        if (!glyph) glyph = font.characters['?'];
        if (!glyph) continue;
        if (previous) result += font.GetKerning(previous->character, glyph);
        result += glyph->advance;
        previous = glyph;
    }
    return result;
}
}

struct PoliceSubtitles::State {
    Node2D& canvas;
    const GameResources& files;
    Dictionary entries;
    bool loaded{}, verified{};
    std::atomic_bool cancel{false};
    std::future<bool> digest;
    std::uint64_t world{}, replay{}, sequence{}, suppressed{};
    Clock::time_point finishTime{};
    std::string line, visible;
    Vector2 renderedViewport;
    Ref<ImageTexture> atlas;
    std::array<Rect2,256> regions{};
    std::array<float,256> inkBottom{};
    std::unordered_map<std::uint32_t, float> gaugeTextureBottoms;
    std::uint32_t atlasFont{};
    Color renderedTint;
    std::vector<std::string> renderedRows;
    float renderedBottom{};
    Vector2 renderedTextScale;

    State(Node2D& node, const GameResources& assets) : canvas(node), files(assets) {}
    ~State() { cancel.store(true, std::memory_order_relaxed); if (digest.valid()) digest.wait(); }

    bool ready()
    {
        if (!loaded) {
            loaded = true;
            const auto parsed = JSON::parse_string(FileAccess::get_file_as_string(catalogPath));
            if (parsed.get_type() != Variant::DICTIONARY) return false;
            const Dictionary catalog = parsed;
            if (int(catalog.get("version", 0)) != 1 || String(catalog.get("language", "")) != "en" ||
                Variant(catalog.get("entries", {})).get_type() != Variant::DICTIONARY) return false;
            entries = catalog["entries"];
            if (const auto source = files.TryOpenPhysical(englishPath))
                digest = std::async(std::launch::async, [range = source->range, this] {
                    return verify_english(range, cancel);
                });
        }
        if (digest.valid() && digest.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            verified = digest.get();
        return verified;
    }

    std::string assemble(const SpeechCaptionState& caption) const
    {
        if (!caption.english || !caption.count || caption.count > std::size(caption.offsets)) return {};
        std::vector<String> parts;
        for (unsigned i = 0; i < caption.count; ++i) {
            if (!std::binary_search(std::begin(radioEvents), std::end(radioEvents), caption.events[i])) return {};
            char key[48];
            std::snprintf(key, sizeof(key), "assets/voice/native_%08x.wav", caption.offsets[i]);
            const Variant item = entries.get(key, Variant());
            if (item.get_type() != Variant::DICTIONARY) return {};
            const Dictionary fragment = item;
            if (bool(fragment.get("silent", false))) continue;
            auto text = String(fragment.get("text", "")).strip_edges();
            if (text.is_empty()) return {};
            parts.push_back(std::move(text));
        }
        String sentence;
        for (size_t i = 0; i < parts.size(); ++i) {
            auto text = parts[i];
            if (i + 1 < parts.size()) {
                while (!text.is_empty() && String(".!?").contains(text.right(1))) text = text.left(-1);
            }
            if (i && !text.is_empty()) {
                const auto first = text.get_slice(" ", 0);
                if (first != first.to_upper() && !first.begins_with("I'") &&
                    !first.begins_with("I’") && first != "County")
                    text = text.left(1).to_lower() + text.substr(1);
                sentence += " ";
            }
            sentence += text;
        }
        return byte_font_text(sentence);
    }

    void clear()
    {
        if (visible.empty()) return;
        visible.clear();
        RenderingServer::get_singleton()->canvas_item_clear(canvas.get_canvas_item());
    }

    static const FlailerTextChunk* hud_text(FlailerResources& resources, std::uint32_t key)
    {
        for (const auto screen : hudScreens) {
            const auto* text = static_cast<const FlailerTextChunk*>(resources.FindObject(screen, key, 1));
            if (text && text->scaleX > 0 && text->scaleY > 0) return text;
        }
        return nullptr;
    }

    bool prepare_atlas(const Font& font, FlailerResources& resources)
    {
        if (atlas.is_valid() && atlasFont == font.key) return true;
        const auto* source = font.GetTextureInfo(resources);
        const auto* info = source ? textureInfoForDescriptor(source) : nullptr;
        if (!info || !info->pixels.Data() || (info->paletteBytes && !info->palette.Data())) return false;
        std::uint32_t w{}, h{};
        const auto rgba = DecodeTextureMipRgba(*info, 0, 0, info->pixels.size, w, h);
        int cellW = 0, cellH = 0;
        for (const auto* glyph : font.characters) {
            if (!glyph) continue;
            cellW = std::max(cellW, int(glyph->size[0]));
            cellH = std::max(cellH, int(glyph->size[1]));
        }
        cellW += 2;
        cellH += 2;
        const int wOut = cellW * 16, hOut = cellH * 16;
        PackedByteArray pixels;
        pixels.resize(static_cast<int64_t>(wOut) * hOut * 4);
        auto* destination = pixels.ptrw();
        std::fill_n(destination, pixels.size(), 0);
        regions = {};
        inkBottom = {};
        for (const auto* glyph : font.characters) {
            if (!glyph || glyph->size[0] <= 0 || glyph->size[1] <= 0) continue;
            const int glyphW = int(glyph->size[0]), glyphH = int(glyph->size[1]);
            const int sx = int(glyph->atlasPosition[0]), sy = int(glyph->atlasPosition[1]);
            const int dx = (glyph->character % 16) * cellW + 1;
            const int dy = (glyph->character / 16) * cellH + 1;
            // Preserve the authored glyph and its baked edge. Duplicate the
            // border texels so linear filtering stays within each glyph.
            for (int y = -1; y <= glyphH; ++y) {
                for (int x = -1; x <= glyphW; ++x) {
                    const int sourceX = sx + std::clamp(x, 0, glyphW - 1);
                    const int sourceY = sy + std::clamp(y, 0, glyphH - 1);
                    if (sourceX < 0 || sourceX >= int(w) || sourceY < 0 || sourceY >= int(h)) continue;
                    const auto* texel = rgba.data() + (std::size_t(sourceY) * w + sourceX) * 4;
                    auto* pixel = destination + ((dy + y) * wOut + dx + x) * 4;
                    std::copy_n(texel, 3, pixel);
                    pixel[3] = static_cast<std::uint8_t>(std::min(255, texel[3] * 2));
                    if (pixel[3] && y < glyphH) inkBottom[glyph->character] = float(y + 1);
                }
            }
            regions[glyph->character] = Rect2(dx, dy, glyphW, glyphH);
        }
        atlas = ImageTexture::create_from_image(Image::create_from_data(wOut, hOut, false, Image::FORMAT_RGBA8, pixels));
        atlasFont = font.key;
        return atlas.is_valid();
    }

    float gauge_texture_bottom(const EditorMarkerTexture* texture)
    {
        if (!texture) return 1;
        const auto cached = gaugeTextureBottoms.find(texture->key);
        if (cached != gaugeTextureBottoms.end()) return cached->second;
        const auto* info = textureInfoForDescriptor(texture);
        if (!info || !info->pixels.Data() || (info->paletteBytes && !info->palette.Data())) return 1;
        std::uint32_t width{}, height{};
        const auto rgba = DecodeTextureMipRgba(*info, 0, 0, info->pixels.size, width, height);
        for (std::uint32_t y = height; y > 0; --y) {
            for (std::uint32_t x = 0; x < width; ++x) {
                if (rgba[(std::size_t(y - 1) * width + x) * 4 + 3] > 8) {
                    const float bottom = float(y) / height;
                    gaugeTextureBottoms.emplace(texture->key, bottom);
                    return bottom;
                }
            }
        }
        gaugeTextureBottoms.emplace(texture->key, 0);
        return 0;
    }

    std::vector<Rect2> hud_bounds(const FlailerRenderManager& hud, Vector2 viewport, float scale,
        float& gaugeBottom)
    {
        const auto origin = (viewport - Vector2(640, 480) * scale) * 0.5f;
        std::vector<Rect2> result;
        for (const auto& layer : hud.layers) for (const auto* first : layer) {
            for (auto* strip = first; strip; strip = strip->nextTexture.get()) {
                if (std::find(hudScreens.begin(), hudScreens.end(), strip->sourceScreen) == hudScreens.end()) continue;
                Rect2 bounds;
                bool found = false;
                for (const auto& polygon : strip->polygons) {
                    if (std::none_of(polygon.vertices.begin(), polygon.vertices.end(),
                        [](const auto& vertex) { return (vertex.color >> 24) != 0; })) continue;
                    for (const auto& vertex : polygon.vertices) {
                        const Vector2 point = origin + Vector2(vertex.position[0], vertex.position[1]) * scale;
                        if (!found) { bounds.position = point; found = true; }
                        else bounds.expand_to(point);
                    }
                }
                if (!bounds.has_area()) continue;
                if (bounds.get_center().x > viewport.x * 0.5f &&
                    std::find(gaugeFaces.begin(), gaugeFaces.end(), strip->sourceObject) != gaugeFaces.end()) {
                    // The artwork leaves blank rows inside the authored gauge quad.
                    const float textureBottom = gauge_texture_bottom(strip->texture);
                    const auto& quad = strip->polygons.front().vertices;
                    const float topV = quad[0].textureCoordinate[1], bottomV = quad[2].textureCoordinate[1];
                    if (bottomV > topV)
                        bounds.size.y *= std::clamp((textureBottom - topV) / (bottomV - topV), 0.0f, 1.0f);
                    if (bounds.has_area()) gaugeBottom = std::max(gaugeBottom, bounds.get_end().y);
                }
                if (bounds.has_area()) result.push_back(bounds.grow(8 * scale));
            }
        }
        return result;
    }

    static std::vector<std::string> wrap(const Font& font, const std::string& text, float limit)
    {
        std::vector<std::string> rows;
        std::string line, word;
        std::istringstream words(text);
        while (words >> word) {
            const auto candidate = line.empty() ? word : line + " " + word;
            if (width(font, candidate) <= limit) { line = candidate; continue; }
            if (!line.empty()) rows.push_back(std::exchange(line, {}));
            for (char code : word) {
                if (!line.empty() && width(font, line + code) > limit)
                    rows.push_back(std::exchange(line, {}));
                line += code;
            }
        }
        if (!line.empty()) rows.push_back(std::move(line));
        return rows;
    }

    void draw(const std::string& text, FlailerResources& resources, const FlailerRenderManager& hud, Vector2 viewport)
    {
        if (text.empty()) { clear(); return; }
        const auto* style = hud_text(resources, completionValue);
        const auto* font = style ? resources.FindFont(style->fontKey) : nullptr;
        if (!font || font->lineHeight <= 0 || !prepare_atlas(*font, resources)) { clear(); return; }

        const float hudScale = std::min(viewport.x / 640.0f, viewport.y / 480.0f);
        if (hudScale <= 0) { clear(); return; }
        const auto* timeLabel = hud_text(resources, timeLeftLabel);
        const auto* timeFont = timeLabel ? resources.FindFont(timeLabel->fontKey) : nullptr;
        const float size = timeFont && timeFont->lineHeight > 0
            ? timeFont->lineHeight * timeLabel->scaleY / (font->lineHeight * style->scaleY) : 1.0f;
        const Vector2 textScale = Vector2(style->scaleX, style->scaleY) * (hudScale * size);
        const auto channel = [&](int shift) { return std::min(1.0f, ((style->color >> shift) & 255u) / 128.0f); };
        const Color tint(channel(0), channel(8), channel(16), channel(24));
        const float lineHeight = font->lineHeight * textScale.y;
        const float center = viewport.x * 0.5f;
        float gaugeBottom = 0;
        const auto protectedBounds = hud_bounds(hud, viewport, hudScale, gaugeBottom);
        const float preferredWidth = std::min(viewport.x * 0.60f, viewport.y * 0.70f);
        float selectedBottom = 0;
        std::vector<std::string> selectedRows;
        const float preferredBottom = gaugeBottom > 0 ? std::min(gaugeBottom, viewport.y) : viewport.y * 0.91f;
        for (float bottom : {preferredBottom, viewport.y * 0.85f, viewport.y * 0.78f, viewport.y * 0.72f}) {
            float bandWidth = preferredWidth;
            while (bandWidth >= 116.0f * hudScale) {
                auto rows = wrap(*font, text, bandWidth / textScale.x);
                const float top = bottom - rows.size() * lineHeight;
                if (top < viewport.y * 0.55f) break;
                float safe = bandWidth;
                for (const auto& obstacle : protectedBounds) {
                    if (obstacle.position.y >= bottom || obstacle.get_end().y <= top) continue;
                    safe = std::min(safe, 2 * std::max(obstacle.position.x - center, center - obstacle.get_end().x));
                }
                if (safe >= bandWidth - 0.01f) {
                    selectedBottom = bottom;
                    selectedRows = std::move(rows);
                    break;
                }
                bandWidth = safe;
            }
            if (!selectedRows.empty()) break;
        }
        if (selectedRows.empty()) { clear(); return; }
        if (text == visible && viewport == renderedViewport && selectedRows == renderedRows &&
            selectedBottom == renderedBottom && textScale == renderedTextScale && tint == renderedTint) return;

        auto* server = RenderingServer::get_singleton();
        const auto canvasId = canvas.get_canvas_item();
        server->canvas_item_clear(canvasId);
        visible = text;
        renderedViewport = viewport;
        renderedBottom = selectedBottom;
        renderedTextScale = textScale;
        renderedTint = tint;
        renderedRows = std::move(selectedRows);
        float lastRowInk = 0;
        for (unsigned char code : renderedRows.back()) {
            const auto* glyph = font->characters[code] ? font->characters[code] : font->characters['?'];
            if (glyph) lastRowInk = std::max(lastRowInk, inkBottom[glyph->character]);
        }
        float y = selectedBottom - (renderedRows.size() - 1) * lineHeight - lastRowInk * textScale.y;
        for (const auto& row : renderedRows) {
            float x = center - width(*font, row) * textScale.x * 0.5f;
            const tTTGlyph* previous = nullptr;
            for (unsigned char code : row) {
                const auto* glyph = font->characters[code];
                if (!glyph) glyph = font->characters['?'];
                if (!glyph) continue;
                if (previous) x += font->GetKerning(previous->character, glyph) * textScale.x;
                const auto& region = regions[glyph->character];
                if (region.has_area())
                    server->canvas_item_add_texture_rect_region(canvasId,
                        Rect2(Vector2(x, y), region.size * textScale), atlas->get_rid(), region, tint);
                x += glyph->advance * textScale.x;
                previous = glyph;
            }
            y += lineHeight;
        }
    }
};

PoliceSubtitles::PoliceSubtitles(Node2D& canvas, const GameResources& files)
    : state_(std::make_unique<State>(canvas, files)) {}
PoliceSubtitles::~PoliceSubtitles() = default;

const std::string& PoliceSubtitles::visible_text() const noexcept { return state_->visible; }
bool PoliceSubtitles::verified_source() const noexcept { return state_->verified; }

void PoliceSubtitles::update(bool enabled, bool racing, bool paused, std::uint64_t world, std::uint64_t replay,
    const SpeechCaptionState& caption, FlailerResources& resources, const FlailerRenderManager& hud, const Vector2& viewport)
{
    auto& s = *state_;
    if (world != s.world || replay != s.replay) {
        // A playback left over from the previous scene must not appear after a seek or restart.
        if (s.world || s.replay) s.suppressed = caption.sequence;
        s.world = world;
        s.replay = replay;
        s.sequence = 0;
        s.line.clear();
        s.clear();
    }
    const bool available = enabled && s.ready();
    if (paused && caption.finished) s.suppressed = caption.sequence;
    if (!available || !racing || !caption.sequence || caption.sequence == s.suppressed) {
        if (!available || !paused) {
            // Keep a live line through a pause, but never pick up post-race speech.
            if (!racing && !paused && caption.sequence) s.suppressed = caption.sequence;
            s.sequence = 0;
            s.line.clear();
            s.finishTime = {};
        }
        s.clear();
        return;
    }
    if (caption.sequence != s.sequence) {
        s.sequence = caption.sequence;
        s.finishTime = {};
        s.line = caption.finished ? std::string{} : s.assemble(caption);
    }
    if (caption.finished) {
        if (s.finishTime == Clock::time_point{}) s.finishTime = Clock::now();
        if (Clock::now() - s.finishTime >= std::chrono::milliseconds(500)) s.line.clear();
    }
    s.draw(s.line, resources, hud, viewport);
}
}
