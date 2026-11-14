#ifndef NFSHP2_TEXTURE_HPP
#define NFSHP2_TEXTURE_HPP

#include "indep/inc/Ecstasy.hpp"
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace hp2 {
class GameResources;
class TexturePack;
class TextureAnimPack;
class TextureMetadataStorage;
class CarLoader;
extern "C++" { class SelectTrackSlideshow; struct ApplicationFrontendFlow; struct GameFlowBindings; }

class TexturePixelStorage final {
public:
    TexturePixelStorage(std::size_t bytes,std::size_t alignment);
    ~TexturePixelStorage();
    TexturePixelStorage(const TexturePixelStorage&)=delete;
    TexturePixelStorage& operator=(const TexturePixelStorage&)=delete;
    const std::uint8_t* Data() const noexcept {return data_;}
    // A writable pointer can outlive the call which returned it. Once one
    // escapes, all aliases of this allocation keep exact content validation
    // for the rest of its lifetime, including writes through an old pointer.
    std::uint8_t* Data() noexcept {mutableAddressExposed_=true;return data_;}
    std::size_t Size() const noexcept {return size_;}
    bool PublishedReadOnly() const noexcept {return published_ && !mutableAddressExposed_;}
private:
    friend class TexturePack;
    // Only the pack's private initial copy / completed queued read can
    // publish immutable contents. Publication never forgets an escaped
    // writable address obtained while that read was still pending.
    std::uint8_t* LoadingData() noexcept {return data_;}
    void PublishReadOnly() noexcept {published_=true;}
    std::uint8_t* data_{};
    std::size_t size_{};
    bool published_{},mutableAddressExposed_{};
};

struct TextureByteView {
    std::shared_ptr<TexturePixelStorage> owner;
    std::size_t offset{};
    std::size_t size{};
    // MoviePlayer binds its actual DCT plane here while the pack retains its
    // loaded allocation. Close detaches this borrow before freeing the image.
    // There is one selected pixel address, never a copied decoded image.
    std::uint8_t* borrowed{};
    // Generated PC palettes are already linear. The tag belongs to the view:
    // animation copies carry it, while a resource reload reconstructs a
    // default authored-format view. It does not alter the asset descriptor.
    bool nativeLinearPalette{};
    const std::uint8_t* Data() const noexcept {return borrowed ? borrowed :
        (owner ? static_cast<const TexturePixelStorage&>(*owner).Data()+offset : nullptr);}
    std::uint8_t* MutableData() const noexcept {return borrowed ? borrowed : (owner ? owner->Data()+offset : nullptr);}
};

// The descriptor is the same object published to the model/Flailer texture
// catalogue. Pixel and palette storage are retained region views, not copies
// of a console texture object or fabricated GPU handles.
struct TextureInfo {
    EditorMarkerTexture* descriptor{};
    TexturePack* pack{};
    TextureInfo* linkedTexture{};
    std::int32_t pixelFileOffset{};
    std::int32_t paletteFileOffset{};
    std::int32_t pixelBytes{};
    std::int32_t paletteBytes{};
    std::uint32_t linkedTextureKey{};
    TextureByteView pixels;
    TextureByteView palette;
    // Exact RenderStrips predicates over this same retained descriptor.
    // No colour/alpha-mode meaning is inferred for the separate flag4 lane.
    bool requiresDepthSort{};
    bool drawFlag4Enabled{};
    static float GetScroll(float time,float rate,std::int32_t mode,float interval);
    void GetAlphaChannel(std::uint8_t* output,std::size_t outputBytes) const;
    void GetColourChannel(std::uint8_t* output,std::size_t outputBytes) const;
    void SetColourChannel(const std::uint8_t* input,std::size_t inputBytes);
};

// Actual native upload decoder, also usable by CPU-side resource checks.
// This is the one implementation used by Vulkan; no second test renderer.
std::vector<std::uint8_t> DecodeTextureMipRgba(const TextureInfo& texture,
    std::uint32_t level,std::size_t sourceOffset,std::size_t sourceBytes,
    std::uint32_t& width,std::uint32_t& height,bool repeatedPadding=true);

// Typed delivery for frontend.cpp's two original static callbacks. The
// marker remains in this same ordered texture-read queue.
enum class FrontendLegalTextureCompletion { None,Legal,AntiPiracy };

struct LoadedTextureData {
    std::int32_t begin{};
    std::int32_t end{};
    std::uint32_t references{};
    std::uint32_t allocationFlags{0x40};
    std::shared_ptr<TexturePixelStorage> data;
    bool loaded{};
    bool loading{};
    SelectTrackSlideshow* completion{};
    CarLoader* carCompletion{};
    FrontendLegalTextureCompletion legalCompletion{FrontendLegalTextureCompletion::None};
    ApplicationFrontendFlow* gameFlow{};
    const GameFlowBindings* gameFlowBindings{};
};

struct TexturePackRegistry {
    std::vector<TexturePack*> packs;
    std::vector<TextureAnimPack*> animationPacks;
    // The loaded branch of PAL1ABF48, shared by model lookup and InitAnims.
    TextureInfo* ResolveLoadedTexture(EditorMarkerResources& root,std::uint32_t key);
    void UpdateAnimations(EditorMarkerResources& root,std::int32_t worldFrames,std::int32_t realFrames);
    void ServiceReads();
    // Native PC index for the source descriptor -> TextureInfo association.
    // Rebuilt only when a TexturePack publishes or retires a table, NOT for
    // every material strip during scene construction. Original pack/texture
    // objects remain the sole owners. This is not a second resource table.
    TextureInfo* LookupDescriptor(const EditorMarkerTexture* descriptor) noexcept;
    void InvalidateDescriptorIndex() noexcept { descriptorIndexValid_=false; }
    // Source global330E38; zero selects each animation's authored rate.
    std::int32_t animationRateOverride{};
private:
    std::unordered_map<const EditorMarkerTexture*,TextureInfo*> descriptorIndex_;
    bool descriptorIndexValid_{};
};
extern TexturePackRegistry texturePackRegistry;
bool SwapTextureName(TexturePackRegistry& registry,std::uint32_t first,std::uint32_t second);

// Sole owner of one authored B0300100 animation pack. Each entry retains its
// original pixel/palette views while the same TextureInfo receives the current
// frame. Native vectors replace pointer fixups, not the source frame selection.
class TextureAnimPack final {
public:
    TextureAnimPack(TexturePackRegistry& registry,EditorMarkerResources& root,
        const std::vector<std::uint8_t>& bundle,std::size_t chunkOffset);
    ~TextureAnimPack();
    TextureAnimPack(const TextureAnimPack&)=delete;
    TextureAnimPack& operator=(const TextureAnimPack&)=delete;
    EditorMarkerResources& CatalogueRoot() const noexcept;
    // Port-side read-only snapshot for uploading the original frames to Godot.
    bool ReadFrames(const EditorMarkerTexture& descriptor, std::vector<TextureInfo>& frames,
        int& rate, bool& realClock) const;
    void Update(std::int32_t worldFrames,std::int32_t realFrames,std::int32_t rateOverride);
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};

class TexturePack final {
public:
    TexturePack(TexturePackRegistry& registry,const GameResources& resources,
        EditorMarkerResources& catalogue,const char* name,const char* filename=nullptr);
    TexturePack(TexturePackRegistry& registry,EditorMarkerResources& catalogue,
        const std::vector<std::uint8_t>& bundle,std::size_t chunkOffset);
    ~TexturePack();
    TexturePack(const TexturePack&)=delete;
    TexturePack& operator=(const TexturePack&)=delete;
    TexturePack(TexturePack&&)=delete;
    TexturePack& operator=(TexturePack&&)=delete;
    bool LoadTextureTableFromFile(const char* filename);
    TextureInfo* GetLoadedTexture(std::uint32_t key);
    TextureInfo* GetTexture(std::uint32_t key);
    TextureInfo* GetNthTexture(std::int32_t index);
    LoadedTextureData* FindLoadedTextureData(std::int32_t begin,std::int32_t bytes);
    LoadedTextureData* FindNotYetLoadedNeighbourTextureData(std::int32_t begin,
        std::int32_t bytes,std::int32_t unused=0);
    void AddLoadedTextureData(std::int32_t begin,std::int32_t bytes);
    void DeleteLoadedTextureData(LoadedTextureData* data);
    void LoadTextureFromFile(TextureInfo* texture);
    void UnloadTextureFromFile(TextureInfo* texture);
    void UnloadTexturesFromFile(const std::uint32_t* keys,std::int32_t count);
    bool RemoveLoadedTextureData(std::int32_t begin,std::int32_t bytes);
    void LoadTexturesFromFile(const std::uint32_t* keys,std::int32_t count,
        SelectTrackSlideshow* completion=nullptr);
    void LoadTexturesFromFile(const std::uint32_t* keys,std::int32_t count,CarLoader* completion);
    void LoadTexturesFromFile(const std::uint32_t* keys,std::int32_t count,FrontendLegalTextureCompletion completion);
    void LoadTexturesFromFile(const std::uint32_t* keys,std::int32_t count,
        ApplicationFrontendFlow& flow,const GameFlowBindings& bindings);
    void ServiceLoadingTextures();
    void ReadCallback();
    void UnloadAllTexturesFromFile();
    void BeginShutdown() noexcept;
    std::exception_ptr Shutdown() noexcept;
    std::uint32_t GetLoadedSize() const;
    // Native I/O event delivery. These methods service real file reads;
    // they are platform adaptation helpers, zero additional ELF bodies.
    void ServiceRead();
    void WaitForReads();
    bool HasPendingReads() const noexcept;
    // Drop only the retiring application's HUD/flow marker continuation.
    // Pixel reads, ownership and normal slideshow/car callbacks are unchanged.
    void DetachFlowCompletions(const ApplicationFrontendFlow& flow) noexcept;
    std::size_t TextureCount() const;
    const std::string& Name() const;
    TexturePackRegistry& Registry() const noexcept;
    EditorMarkerResources& CatalogueRoot() const noexcept;
    std::shared_ptr<const TextureMetadataStorage> MetadataOwnerFor(const EditorMarkerTexture* descriptor) const;
    // CarRenderInfo writes the original descriptor byte29. Keep the write in
    // the same retained metadata owner; no copied sampler/texture state.
    void SetDescriptorByte29(EditorMarkerTexture* descriptor,std::uint8_t value);
    // OverlayRVMImages sets the actual RVM_MASK descriptor's full-alpha lane.
    void SetDescriptorByte4F(EditorMarkerTexture* descriptor,std::uint8_t value);
    // PAL pack+90/node+14 are allocator flags. Flag40 requests128-byte
    // alignment; treating the value64 as a byte alignment was misleading.
    std::uint32_t readAllocationFlags{0x40};
private:
    friend bool SwapTextureName(TexturePackRegistry&,std::uint32_t,std::uint32_t);
    friend class QueuedFileService;
    friend class CarLoader;
    // Native exception boundary: a published marker still owns the real
    // loading completion, even if its first physical read could not start.
    bool HasCarCompletion(const CarLoader* car) const noexcept;
    void QueuedReadDone(bool failed);
    void LoadTextureKeys(const std::uint32_t* keys,std::int32_t count,
        SelectTrackSlideshow* slideshow,CarLoader* car,
        FrontendLegalTextureCompletion legal=FrontendLegalTextureCompletion::None,
        ApplicationFrontendFlow* flow=nullptr,const GameFlowBindings* bindings=nullptr);
    void AttachTextureTable(std::vector<std::uint8_t> bytes);
    void AssignTextureData(const std::shared_ptr<TexturePixelStorage>& bytes,
        std::int32_t begin,std::int32_t count);
    void UnAssignTextureData(std::int32_t begin,std::int32_t count);
    struct Storage;
    std::unique_ptr<Storage> storage_;
};

std::shared_ptr<const TextureMetadataStorage> textureMetadataOwner(const EditorMarkerTexture* descriptor);
// A lease keeps its descriptors alive after unpublication. Mutable borrowers
// must check publication before writing into them.
bool textureMetadataPublished(const std::shared_ptr<const TextureMetadataStorage>& metadata) noexcept;
TextureInfo* textureInfoForDescriptor(const EditorMarkerTexture* descriptor) noexcept;

} // namespace hp2
#endif
