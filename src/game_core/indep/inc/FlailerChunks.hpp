#pragma once

#include "indep/inc/Ecstasy.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace hp2 {class TextureMetadataStorage;}
namespace hp2 {class MoviePlayer;}
namespace hp2 {

struct FlailerScreenResource;
struct FlailerStrip;
class FlailerResources;
class FlailerStripPool;
class LanguageManager;

struct tTTGlyph {
    std::uint8_t character{};
    std::int16_t kerningCount{};
    std::uint32_t firstKerning{};
    float advance{};
    std::array<float,2> atlasPosition{};
    std::array<float,2> size{};
};

struct FontKerning {
    std::uint8_t nextCharacter{};
    float adjustment{};
};

struct Font {
    std::uint32_t key{};
    std::array<const tTTGlyph*,256> characters{};
    const FontKerning* kernings{};
    std::size_t kerningCount{};
    float lineHeight{};
    float fixedAdvance{};
    mutable const EditorMarkerTexture* texture{};
    mutable std::shared_ptr<const ::hp2::TextureMetadataStorage> textureStorage;
    mutable float textureWidth{};
    mutable float textureHeight{};

    float GetKerning(int nextCharacter,const tTTGlyph* glyph) const;
    float GetWidestCharacter(const char* text) const;
    const EditorMarkerTexture* GetTextureInfo(FlailerResources& resources) const;
};

struct FlailerObjectChunk {
    FlailerObjectChunk* next{};
    FlailerObjectChunk* previous{};
    std::uint32_t key{};
    std::uint32_t parentKey{};
    std::array<char,24> name{};
    std::uint8_t kind{};
    std::uint8_t visible{};
    std::uint8_t authoredFlags{};
    std::uint8_t updateKind{};
    float topLeftX{};
    float topLeftY{};
    float scaleX{1.0f};
    float scaleY{1.0f};
    std::int32_t rotation{};
    FlailerStrip* strip{};
    // Two authored scalar words copied by CopyFlailerText. Their render-side
    // interpretation is not needed by the CPU menu navigation.
    std::array<std::uint32_t,2> authoredMetrics{};
    // Shared original object word+4C, used by every object kind.
    std::uint32_t clipObjectKey{};

    void SetScaleX(float value);
    void SetScaleY(float value);
    void SetTopLeftX(float value);
    void SetTopLeftY(float value);
    void SetClipToPoly(std::uint32_t key);
    void SetAlpha(std::int32_t alpha);
    void ScaleAboutCenter(FlailerResources& resources,float scale,const LanguageManager* language);
    bool DoWeNeedToBuildFlailerStrip(float alphaScale);
    void CreateFlailerStrip(FlailerStripPool& pool,const EditorMarkerTexture* texture,float alphaScale);
    void DeleteFlailerStrip();
};

struct FlailerTextChunk : FlailerObjectChunk {
    std::uint32_t fontKey{};
    std::uint32_t textCapacity{};
    std::uint32_t formatOptions{};
    std::uint32_t color{};
    const char* text{};
    std::uint32_t languageId{};
    std::vector<char> ownedText;

    void SetFormatOptions(std::int32_t options);
    void ClearFormatOptions(std::int32_t options);
    void SetColor(std::uint32_t rgb);
    void UpdateLanguageText();
    void SetAlpha(std::int32_t alpha);
    std::int32_t GetAlpha() const;
    void SetString(const char* value);
    void SetLanguageID(std::uint32_t value);
    void CopyAttributesFrom(const FlailerTextChunk& source);
    void RenderCharacter(const tTTGlyph& glyph,const Font& font,const float position[3],
        const float minimum[2],const float maximum[2],float alphaScale);
    float RenderHelpCharacter(FlailerResources& resources,float x,float y,std::uint8_t character,float alphaScale);
    void Render(FlailerResources& resources,const LanguageManager* language,
        const float* minimum,const float* maximum,float alphaScale);
    void RenderStringFixedWidth(FlailerResources& resources,const char* first,const char* end,float alphaScale);
    void RenderDarkBackground(FlailerResources& resources,const LanguageManager* language);
    float GetFixedWidthStringWidth(FlailerResources& resources,const char* first,const char* end) const;
};

struct FlailerBitmapChunk : FlailerObjectChunk {
    std::uint32_t textureKey{};
    const EditorMarkerTexture* texture{};
    // Retains the same dynamic TexturePack metadata if this bitmap borrows
    // it. Static bundle descriptors already belong to FlailerResources.
    std::shared_ptr<const ::hp2::TextureMetadataStorage> textureStorage;
    std::array<std::uint32_t,4> colors{};
    std::array<std::array<float,2>,4> textureCoordinates{};
    // Borrowed original bitmap+6C fan, retained by the owning Player.
    const struct FlailerTachometerPoint* tachometerPoints{};
    // Title/legal movie layout only. Other frontend movie panels retain
    // their authored rectangle, texture coordinates and clipping behavior.
    bool nativeFullscreenMovie{};

    void SetAlpha(std::int32_t alpha,std::int32_t corner=-1);
    void SetRotation(float value);
    void SetColor(std::uint32_t color);
    void SetWidth(FlailerResources& resources,float width);
    void SetHeight(FlailerResources& resources,float height);
    void ChangeTexture(const EditorMarkerTexture* value);
    void ChangeTexture(std::uint32_t key);
    std::int32_t GetAlpha(std::int32_t corner) const;
    void SetNativeFullscreenMovie(bool enabled);
};

struct FlailerTachometerPoint {
    std::array<float,3> position{};
    std::array<float,2> textureCoordinate{};
};

struct FlailerPolyVertex {
    std::array<float,4> position{};
    std::array<float,3> textureCoordinate{};
    std::uint32_t color{};
};

struct FlailerPoly {
    std::array<FlailerPolyVertex,4> vertices{};
    // The checksum also consumes these four CPU raster-state words. Their
    // contents remain uninterpreted; no console rendering packet is emitted.
    std::array<std::uint32_t,4> rasterState{};
    // PC sampling transport for actual font glyphs only. The source UVs
    // include the texel immediately after the glyph; native bilinear
    // filtering must not borrow a neighbouring glyph from the same atlas.
    // Bounds are the first/last authored texel centres, before clipping.
    // All zero selects the unchanged ordinary bitmap/world sampling path.
    std::array<float,4> glyphSamplingBounds{};
};

// Original bitmap composition borrows the actual five eRenderSystem targets.
// No replacement image is generated when a target has not been rendered.
struct FlailerBitmapComposite {
    // PAL3301A4 EnableFlailerMovie5Pass is the same 32-bit owner read by
    // MoviePlayer::Update238410 and RenderFlailerBitmap14307C.
    std::int32_t enabled{1};
    std::array<bool,5> stages{true,true,true,true,true};
    std::array<const EditorMarkerTexture*,5> textures{};
    bool targetsReady{};
    // Borrow the canonical player whose PAL+440 validity suppresses closed
    // TITLE/PANELMOVIE planes. Native RGBA conversion preserves that lifetime.
    const MoviePlayer* movie{};
};

struct FlailerRenderManager {
    // Port-side animation inputs. Flailer lays out the echo at its start
    // pose and the host owns its playback.
    struct Highlight {
        FlailerStrip* layout{};
        std::uint32_t screen{}, echo{}, target{}, revision{};
        float scaleRatio{}, duration{}, period{}, endAlpha{};
    };
    std::vector<Highlight> highlights;
    // Event panels reuse one authored bitmap template, so each submission
    // carries its own event number and projected bounds for the pointer host.
    struct PointerEventPanel {
        std::uint32_t number{};
        std::array<float,4> bounds{};
    };
    std::vector<PointerEventPanel> pointerEventPanels;
    struct PointerArrowRow {
        std::uint32_t screen{}, value{};
    };
    std::vector<PointerArrowRow> pointerArrowRows;
    std::array<const FlailerScreenResource*,8> visibleScreens{};
    // The original manager has29 depth lists. These retain the same object
    // caches until the frontend command consumer completes its frame.
    std::array<std::vector<FlailerStrip*>,29> layers;
    std::uint32_t submittingScreen{};
    FlailerBitmapComposite bitmapComposite;
    void BeginFrame(FlailerResources& resources,const LanguageManager* language,float textureTime);
    std::int32_t RenderObjectList(FlailerResources& resources,FlailerObjectChunk& list,
        const LanguageManager* language,float textureTime);
    void RenderFlailerText(FlailerResources& resources,FlailerTextChunk& text,
        const LanguageManager* language,const float* minimum,const float* maximum,float alphaScale);
    void RenderSingleFlailerBitmap(FlailerResources& resources,FlailerBitmapChunk& bitmap,
        float alphaScale,float textureTime);
    void RenderFlailerBitmap(FlailerResources& resources,FlailerBitmapChunk& bitmap,
        float alphaScale,float textureTime);
    std::int32_t RenderLayer(std::int32_t layer,const CarCinematicScreen& view,EcstasyFrame& frame);
    void EndFrame();
    static std::int32_t SortFlailerObjectFunction(const FlailerObjectChunk* first,const FlailerObjectChunk* second);
    bool IsScreenVisible(std::uint32_t key) const;
    void SetScreenVisibility(FlailerResources& resources,std::uint32_t key,std::int32_t visibility);
    void SetScreenVisibility(FlailerScreenResource* screen,std::int32_t visibility);
    void AddFlailerStrip(FlailerObjectChunk& object);
    // Native ePoly submission to the same retained frontend strip owner.
    void SubmitPolygon(FlailerStripPool& pool,const EditorMarkerTexture* texture,
        const FlailerPoly& polygon,std::int32_t depth);
    void RenderTextIntoRectangle(FlailerResources& resources,const char* message,FlailerTextChunk& text,
        const float minimum[2],const float maximum[2],float alphaScale);
    void RenderTextIntoRectangle(FlailerResources& resources,std::uint32_t screenKey,
        std::uint32_t textKey,std::uint32_t clipKey,const char* message,float alphaScale);
    void CompleteSubmissions() noexcept;
};

struct FlailerTextRasterState {
    std::array<float,2> minimum{};
    std::array<float,2> maximum{644.0f,480.0f};
    std::array<std::uint8_t,6> rasterBytes{1,0,1,0,1,128};
    float wrapLeft{},wrapRight{640.0f},wrapLineHeight{20.0f};
    std::array<float,2> lastRenderDistance{};
    const EditorMarkerTexture* darkBackground{};
    std::uint32_t darkBackgroundColor{0xb0000000u};
    bool highlightUntranslated{};
    std::array<std::uint8_t,30> helpGlyphs{
        'a','b','d','e','f','g','i','j','h','k','n','o','l','m','3','w',
        'p','v',0x92,'u','q','r','t','s','c','x','2','y','z','1'};
};
extern FlailerTextRasterState flailerTextRasterState;
struct FlailerRuntimeStatistics {
    std::int32_t liveStrips{};
    std::int32_t maximumStrips{};
};
extern FlailerRuntimeStatistics flailerRuntimeStatistics;
void SetFlailerTextClipWindow(float left,float top,float right,float bottom);
void SetFlailerTextClipWindow(const float minimum[2],const float maximum[2]);
void ResetFlailerTextClipWindow();

std::uint32_t next_power_of_2(std::uint32_t value);
bool MaybeClipPoly(FlailerPoly* polygon,const float minimum[2],const float maximum[2]);
float FlailerTextDistanceToNextChar(const char* text,char delimiter,const Font* font);
bool IsHelpCharacter(std::uint8_t value);
FlailerObjectChunk* FindFlailerObject(FlailerObjectChunk* root,std::uint32_t key);
std::uint32_t GetEPolyChecksum(const FlailerPoly* polygon);

} // namespace hp2
