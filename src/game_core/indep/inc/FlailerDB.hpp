#pragma once

#include "FlailerChunks.hpp"

#include <memory>
#include <vector>

namespace hp2 { class GameResources; }
namespace hp2 {
class LanguageManager;

class FlailerStripPool;
struct FlailerStrip {
    FlailerStrip();
    ~FlailerStrip();
    FlailerStripPool* pool{};
    std::vector<FlailerPoly> polygons;
    std::uint32_t retentionCount{};
    bool deleteRequested{};
    const EditorMarkerTexture* texture{};
    float alphaScale{};
    std::uint32_t renderedFrames{};
    EcstasyUiPlacement uiPlacement{EcstasyUiPlacement::OriginalCanvas};
    // Authored object identity for the external presentation.
    std::uint32_t sourceObject{}, sourceScreen{};
    std::unique_ptr<FlailerStrip> nextTexture;
    FlailerPoly& AddPoly(const EditorMarkerTexture* texture);
    void RemovePoly(FlailerPoly& polygon);
    void Render(const CarCinematicScreen& view,EcstasyFrame& frame,std::int32_t layer);
};

// CPU geometry caches are retained across backend submissions. Hiding a
// screen detaches its object caches; referenced caches retire on completion.
class FlailerStripPool final {
public:
    FlailerStrip* Create(FlailerObjectChunk& object,std::vector<FlailerPoly> polygons);
    void Retain(FlailerStrip& strip);
    void Complete(FlailerStrip& strip);
    void Discard(FlailerObjectChunk& object);
    std::size_t Size() const noexcept { return strips_.size(); }
    bool HasPendingSubmissions() const noexcept;
private:
    void Release(FlailerStrip* strip);
    std::vector<std::unique_ptr<FlailerStrip>> strips_;
};

struct FlailerScreenResource {
    std::uint32_t key{};
    std::uint32_t sharedScreenKey{};
    FlailerObjectChunk objects{};
    std::size_t textCount{};
    std::size_t bitmapCount{};
    FlailerScreenResource* next{};
    FlailerScreenResource* previous{};
    std::int16_t visibility{};
    void NotifyVisibilityChange(std::int32_t value);
    void UpdateLanguageText();
    void DeleteAllFlailerStrips();
};

struct FlailerPackageResource {
    FlailerPackageResource* next{};
    FlailerPackageResource* previous{};
    FlailerScreenResource screenRoot{};
};

// Host resource adaptation only. This owns the retained final-game bundle and
// native decoded records. It is not an additional original ELF function.
class FlailerResources final {
public:
    // The application publishes GLOBALB and FRONTB into this same root as
    // their physical files complete. Existing decoded object addresses stay
    // stable. A bundle must retire its UI users before RemoveBundle.
    explicit FlailerResources(EditorMarkerResources& modelRegistry);
    std::uint64_t AppendBundle(const std::vector<std::uint8_t>& authored);
    bool CanRemoveBundle(std::uint64_t token) const;
    bool RemoveBundle(std::uint64_t token);
    explicit FlailerResources(GameResources& resources,
        const EditorMarkerTexture* textures=nullptr,std::size_t textureCount=0,
        EditorMarkerResources* modelRegistry=nullptr);
    explicit FlailerResources(std::vector<std::uint8_t> authored,
        const EditorMarkerTexture* textures=nullptr,std::size_t textureCount=0,
        EditorMarkerResources* modelRegistry=nullptr);
    ~FlailerResources();
    FlailerResources(const FlailerResources&)=delete;
    FlailerResources& operator=(const FlailerResources&)=delete;
    const Font* FindFont(std::uint32_t key) const noexcept;
    bool MissingFontReported() const noexcept;
    std::size_t FontCount() const noexcept;
    const Font& FontAt(std::size_t index) const;
    std::size_t ScreenCount() const noexcept;
    FlailerScreenResource& ScreenAt(std::size_t index);
    // These searches preserve the original ordered package/screen cache.
    FlailerScreenResource* FindScreen(std::uint32_t key);
    FlailerObjectChunk* FindObject(std::uint32_t screenKey,std::uint32_t objectKey,std::int32_t kind=0);
    void SetTextColor(std::uint32_t screenKey,std::uint32_t objectKey,std::uint32_t rgb);
    std::int32_t Printf(std::uint32_t screenKey,std::uint32_t objectKey,const char* format,...);
    // A completed localized field may exceed the console's authored byte
    // allocation (custom English campaign titles and French result titles).
    // Grow this field's native owner explicitly; ordinary SetString, wrapping
    // capacities, layout and debug fields keep their existing contracts.
    std::int32_t SetLocalizedText(std::uint32_t screenKey,std::uint32_t objectKey,const char* text);
    void SetObjectVisibility(std::uint32_t screenKey,std::uint32_t objectKey,std::int32_t value);
    void SetTopLeftPosition(std::uint32_t screenKey,std::uint32_t objectKey,std::int32_t x,std::int32_t y);
    void SetTopLeftX(std::uint32_t screenKey,std::uint32_t objectKey,std::int32_t x);
    void GetTopLeft(const FlailerObjectChunk& object,float& x,float& y,const LanguageManager* language) const;
    bool GetBoundingBox(std::uint32_t screenKey,std::uint32_t objectKey,
        std::int32_t& left,std::int32_t& top,std::int32_t& right,std::int32_t& bottom,
        const LanguageManager* language);
    void SetGroupVisibility(std::uint32_t screenKey,std::uint32_t groupKey,std::int32_t value);
    void SetGroupAlpha(std::uint32_t screenKey,std::uint32_t groupKey,std::int32_t alpha);
    void MoveGroupTo(std::uint32_t screenKey,std::uint32_t groupKey,std::uint32_t anchorKey,float x,float y);
    FlailerObjectChunk* FindObjectByGroup(std::uint32_t screenKey,std::uint32_t groupKey,
        std::int32_t kind,FlailerObjectChunk* previous=nullptr);
    float ScaledTextHeight(const FlailerTextChunk& text) const;
    float ScaledTextWidth(const FlailerTextChunk& text,const LanguageManager* language) const;
    void ScaleTextAboutCenter(FlailerTextChunk& text,float scale,const LanguageManager* language) const;
    void ScaleBitmapAboutCenter(FlailerBitmapChunk& bitmap,float scale) const;
    void ScaleObjectAboutCenter(std::uint32_t screen,std::uint32_t object,float scale,const LanguageManager* language);
    const EditorMarkerTexture* FindTexture(std::uint32_t key) const;
    EditorMarkerResources& Models() noexcept;
    float ScaledBitmapWidth(FlailerBitmapChunk& bitmap) const;
    float ScaledBitmapHeight(FlailerBitmapChunk& bitmap) const;
    void SetBitmapCenter(FlailerBitmapChunk& bitmap,float x,float y) const;
    std::int32_t RenderTextIntoGroup(std::uint32_t screenKey,std::uint32_t groupKey,const char* message);
    void SetLanguageID(std::uint32_t screenKey,std::uint32_t objectKey,std::uint32_t value);
    std::size_t PackageCount() const noexcept;
    FlailerPackageResource& PackageRoot() noexcept;
    void DeleteAllStrips();
    FlailerStripPool& StripPool() noexcept;
    const std::vector<std::uint8_t>& Authored() const noexcept;

private:
    FlailerResources(std::vector<std::vector<std::uint8_t>> bundles,
        const EditorMarkerTexture* textures,std::size_t textureCount,
        EditorMarkerResources* modelRegistry);
    struct Storage;
    std::unique_ptr<Storage> storage_;
};

void FlailerUpdateLanguageText(FlailerResources& resources);
void InitializeFlailerTachometer(FlailerResources& resources,FlailerBitmapChunk& bitmap,
    std::array<FlailerTachometerPoint,9>& points);
void RenderFlailerTachometer(FlailerBitmapChunk& bitmap,const FlailerTachometerPoint* points);

// Complete PAL146600; beta definition is this header, not a new sort facade.
inline std::int32_t FlailerRenderManager::SortFlailerObjectFunction(
    const FlailerObjectChunk* first,const FlailerObjectChunk* second)
{
    return static_cast<std::int8_t>(first->authoredFlags)<=static_cast<std::int8_t>(second->authoredFlags);
}

} // namespace hp2
