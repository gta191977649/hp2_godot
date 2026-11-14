#include "Texture.hpp"
#include "CarLoader.hpp"
#include "Fred.hpp"
#include "FMenus.hpp"
#include "GameFlow.hpp"
#include "bware/inc/bChunk.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bFile.hpp"
#include "bware/inc/bMath.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <exception>
#include <limits>
#include <malloc.h>
#include <cstdlib>
#include <new>
#include <stdexcept>

namespace hp2 {

TexturePackRegistry texturePackRegistry;

TextureInfo* TexturePackRegistry::ResolveLoadedTexture(EditorMarkerResources& catalogue,std::uint32_t key)
{
    auto* root=catalogue.catalogueRoot ? catalogue.catalogueRoot:&catalogue;
    // PAL1ABF48 loaded lookup, move-to-front and used byte. Render-target
    // textures and the optional missing-texture branch have separate callers.
    for(std::size_t index=0;index<packs.size();++index) {
        auto* pack=packs[index];if(&pack->CatalogueRoot()!=root)continue;
        if(auto* texture=pack->GetLoadedTexture(key)) {
            packs.erase(packs.begin()+static_cast<std::ptrdiff_t>(index));packs.insert(packs.begin(),pack);
            texture->descriptor->used=1;return texture;
        }
    }
    return nullptr;
}

struct TextureAnimPack::Storage {
    struct Entry {
        std::uint32_t key{};TextureInfo* texture{};TextureByteView pixels,palette;
        std::shared_ptr<const TextureMetadataStorage> metadata;
    };
    struct Animation {
        std::uint32_t key{};
        std::int32_t rate{},clock{},currentFrame{};
        bool enabled{};
        std::vector<Entry> entries;
    };
    TexturePackRegistry* registry{};
    EditorMarkerResources* root{};
    std::vector<Animation> animations;
};

TextureAnimPack::TextureAnimPack(TexturePackRegistry& registry,EditorMarkerResources& catalogue,
    const std::vector<std::uint8_t>& bytes,std::size_t offset):storage_(std::make_unique<Storage>())
{
    // LoaderTexturePack100900..1009A8: version1 has52-byte animations and
    //16-byte entries. The constructor1ABBE8 walks the packed entry sequence.
    Require(offset<=bytes.size() && bytes.size()-offset>=8 && ReadU32(bytes.data()+offset)==0xb0300100u,
        "Texture animation pack has no authored B0300100 header");
    const auto extent=ReadU32(bytes.data()+offset+4);
    Require(extent<=bytes.size()-offset-8,"Texture animation pack leaves its retained bundle");
    const auto children=ReadChunks(bytes,offset+8,extent);
    Require(children.size()==3 && children[0].id==0x30300101u && children[0].size>=4 &&
        ReadU32(bytes.data()+children[0].payloadOffset)==1 && children[1].id==0x30300102u &&
        children[2].id==0x30300103u && children[1].size%52u==0 && children[2].size%16u==0,
        "Texture animation tables are missing or truncated");
    auto& state=*storage_;state.registry=&registry;
    state.root=catalogue.catalogueRoot ? catalogue.catalogueRoot:&catalogue;
    std::size_t entryIndex{};
    for(std::size_t at=children[1].payloadOffset;at<children[1].endOffset;at+=52) {
        const auto* record=bytes.data()+at;Storage::Animation animation;
        animation.key=ReadU32(record+0x18);
        const auto count=ReadU32(record+0x1c);
        Require(count>0 && count<=static_cast<std::uint32_t>(INT32_MAX) &&
            entryIndex<=children[2].size/16 && count<=children[2].size/16-entryIndex,
            "Texture animation entries leave their authored sequence");
        std::memcpy(&animation.rate,record+0x20,4);std::memcpy(&animation.clock,record+0x24,4);
        animation.entries.resize(count);animation.enabled=true;
        // Complete InitAnims1ABC78: missing real frames disable this authored
        // animation, with no synthetic replacement. Snapshot original views
        // before rotating live TextureInfo pixel/palette fields.
        for(auto& entry:animation.entries) {
            entry.key=ReadU32(bytes.data()+children[2].payloadOffset+entryIndex++*16);
            entry.texture=registry.ResolveLoadedTexture(*state.root,entry.key);
            if(entry.texture) {
                entry.metadata=entry.texture->pack->MetadataOwnerFor(entry.texture->descriptor);
                Require(entry.metadata!=nullptr,"Texture animation has no retained descriptor owner");
                entry.pixels=entry.texture->pixels;entry.palette=entry.texture->palette;
            }
            else animation.enabled=false;
        }
        state.animations.push_back(std::move(animation));
    }
    registry.animationPacks.push_back(this);
}

TextureAnimPack::~TextureAnimPack()
{
    auto& packs=storage_->registry->animationPacks;
    const auto found=std::find(packs.begin(),packs.end(),this);
    if(found!=packs.end())packs.erase(found);
}

EditorMarkerResources& TextureAnimPack::CatalogueRoot() const noexcept {return *storage_->root;}

bool TextureAnimPack::ReadFrames(const EditorMarkerTexture& descriptor, std::vector<TextureInfo>& frames,
    int& rate, bool& realClock) const
{
    for (const auto& animation : storage_->animations) {
        if (!animation.enabled) continue;
        const auto& entries = animation.entries;
        const auto destination = std::find_if(entries.begin(), entries.end(), [&descriptor](const auto& entry) {
            return textureMetadataPublished(entry.metadata) && entry.texture->descriptor == &descriptor;
        });
        if (destination == entries.end() || std::any_of(entries.begin(), entries.end(), [](const auto& entry) {
                return !textureMetadataPublished(entry.metadata);
            })) continue;
        rate = storage_->registry->animationRateOverride ? storage_->registry->animationRateOverride : animation.rate;
        realClock = animation.clock == 1;
        frames.reserve(entries.size());
        for (std::size_t i = 0; i < entries.size(); ++i) {
            const auto& source = entries[(destination - entries.begin() + i) % entries.size()];
            auto& frame = frames.emplace_back(*destination->texture);
            frame.pixels = source.pixels;
            frame.palette = source.palette;
        }
        return true;
    }
    return false;
}

void TextureAnimPack::Update(std::int32_t worldFrames,std::int32_t realFrames,std::int32_t rateOverride)
{
    // Complete frame/pointer operation of PAL1ABD80..1ABEE8. Real clock is
    // selected only for authored mode1; every other mode uses WorldTimeFrames.
    const float worldSeconds=static_cast<float>(worldFrames)*0.01666666753590107f;
    const float realSeconds=static_cast<float>(realFrames)*0.01666666753590107f;
    for(auto& animation:storage_->animations) {
        if(!animation.enabled)continue;
        if(std::any_of(animation.entries.begin(),animation.entries.end(),
            [](const auto& entry){return !textureMetadataPublished(entry.metadata);})) {
            // Cross-file destinations stay alive through their metadata
            // leases, but must never receive writes after unpublication.
            animation.enabled=false;
            for(auto& entry:animation.entries) {
                entry.texture=nullptr;entry.pixels={};entry.palette={};entry.metadata.reset();
            }
            continue;
        }
        const auto count=static_cast<std::int32_t>(animation.entries.size());
        const auto rate=rateOverride ? rateOverride:animation.rate;
        const auto sample=convertFloatToInt32(static_cast<float>(rate)*
            (animation.clock==1 ? realSeconds:worldSeconds))%count;
        Require(sample>=0,"Texture animation time precedes its authored frame sequence");
        if(sample==animation.currentFrame)continue;
        animation.currentFrame=sample;
        for(std::int32_t index=0;index<count;++index) {
            auto& destination=*animation.entries[static_cast<std::size_t>(index)].texture;
            const auto& source=animation.entries[static_cast<std::size_t>((index+sample)%count)];
            destination.pixels=source.pixels;destination.palette=source.palette;
        }
    }
}

void TexturePackRegistry::UpdateAnimations(EditorMarkerResources& catalogue,
    std::int32_t worldFrames,std::int32_t realFrames)
{
    const auto* root=catalogue.catalogueRoot ? catalogue.catalogueRoot:&catalogue;
    for(auto* pack:animationPacks)if(&pack->CatalogueRoot()==root)
        pack->Update(worldFrames,realFrames,animationRateOverride);
}

TexturePixelStorage::TexturePixelStorage(std::size_t bytes,std::size_t alignment):size_(bytes)
{
    Require(bytes && alignment>=sizeof(void*) && !(alignment&(alignment-1)),
        "Texture read allocation has invalid size/alignment");
#ifdef _WIN32
    data_=static_cast<std::uint8_t*>(_aligned_malloc(bytes,alignment));
#else
    void* allocation=nullptr;
    if (posix_memalign(&allocation,alignment,bytes)!=0)allocation=nullptr;
    data_=static_cast<std::uint8_t*>(allocation);
#endif
    if (!data_)throw std::bad_alloc();
}

TexturePixelStorage::~TexturePixelStorage()
{
#ifdef _WIN32
    _aligned_free(data_);
#else
    std::free(data_);
#endif
}

class TextureMetadataStorage final {
public:
    std::vector<std::uint8_t> authored;
    std::vector<EditorMarkerTexture> descriptors;
    std::vector<TextureInfo> textures;
    // Native index is metadata-owned and maps authored keys to retained
    // record ordinals in original file order. Multiple identical keys are
    // allowed: GetLoadedTexture must skip unready entries, exactly as the
    // original linear search did. SwapTextureName invalidates this cache.
    std::unordered_map<std::uint32_t,std::vector<std::size_t>> byKey;
    bool keyIndexValid{};
    bool published{true};
    void EnsureKeyIndex() noexcept {
        if(keyIndexValid)return;
        try {
            decltype(byKey) rebuilt;
            rebuilt.reserve(textures.size());
            for(std::size_t i=0;i<textures.size();++i)
                rebuilt[textures[i].descriptor->key].push_back(i);
            byKey.swap(rebuilt);
            keyIndexValid=true;
        } catch (...) {
            // A failed optional index does not change original lookups.
        }
    }
};

struct TexturePack::Storage {
    TexturePackRegistry* registry{};
    const GameResources* resources{};
    EditorMarkerResources* root{};
    EditorMarkerResources catalogue{};
    std::string name;
    std::string filename;
    std::uint32_t nameKey{};
    std::shared_ptr<TextureMetadataStorage> metadata;
    std::shared_ptr<TexturePixelStorage> inlineData;
    std::vector<std::unique_ptr<LoadedTextureData>> regions;
    std::unique_ptr<PhysicalResource> file;
    std::uint64_t dataOffset{};
    LoadedTextureData* activeRead{};
    QueuedFileService* queued{};
    std::int32_t queuedId{};
    std::exception_ptr failure;
    std::int32_t serviceCounter{};
    bool shuttingDown{},shutdownComplete{};
    std::exception_ptr shutdownFailure;
};

namespace {
std::int32_t Signed(std::uint32_t value)
{
    std::int32_t result;std::memcpy(&result,&value,4);return result;
}

std::int32_t Add(std::int32_t a,std::int32_t b)
{
    return Signed(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));
}

bool BeginsInside(std::int32_t value,std::int32_t begin,std::int32_t count)
{
    return value>=begin && value<Add(begin,count);
}
}

TexturePack::TexturePack(TexturePackRegistry& registry,const GameResources& resources,
    EditorMarkerResources& catalogue,const char* name,const char* filename)
    : storage_(std::make_unique<Storage>())
{
    Require(name && std::strlen(name)<32,"Texture pack name exceeds the original valid field");
    Require(!filename || std::strlen(filename)<64,"Texture pack filename exceeds the original valid field");
    auto& s=*storage_;
    s.registry=&registry;s.resources=&resources;s.root=&catalogue;
    if(s.root->catalogueRoot)s.root=s.root->catalogueRoot;
    s.name=name;s.filename=filename ? filename : "";s.nameKey=hashString(name);
    registry.packs.push_back(this);
    registry.InvalidateDescriptorIndex();
    // Source25D398/25D490 only publishes the CodeineInventoryTextures
    // diagnostic packet. No gameplay control or loaded texture is fabricated.
}

TexturePack::~TexturePack()
{
    (void)Shutdown();
}

TexturePack::TexturePack(TexturePackRegistry& registry,EditorMarkerResources& catalogue,
    const std::vector<std::uint8_t>& bundle,std::size_t chunkOffset)
    :storage_(std::make_unique<Storage>())
{
    Require(chunkOffset<=bundle.size() && bundle.size()-chunkOffset>=16,
        "Inline texture pack header is truncated");
    const auto* bytes=bundle.data();
    Require(ReadU32(bytes+chunkOffset)==0xb0300001u,"Inline texture pack has a different chunk type");
    const auto end=chunkOffset+8ull+ReadU32(bytes+chunkOffset+4);
    Require(end<=bundle.size(),"Inline texture pack exceeds its retained bundle");
    const auto nameHeader=chunkOffset+8;
    const auto tableHeader=nameHeader+8ull+ReadU32(bytes+nameHeader+4);
    Require(tableHeader+8<=end && ReadU32(bytes+tableHeader)==0x30300003u,
        "Inline texture pack table is missing");
    const auto tableBytes=ReadU32(bytes+tableHeader+4);
    const auto dataHeader=tableHeader+8ull+tableBytes;
    Require(dataHeader+8<=end && ReadU32(bytes+dataHeader)==0x30300004u,
        "Inline texture pack pixel chunk is missing");
    const auto dataBegin=(dataHeader+0x87ull)&~std::uint64_t{127};
    const auto dataEnd=dataHeader+8ull+ReadU32(bytes+dataHeader+4);
    Require(dataBegin<=dataEnd && dataEnd<=end,"Inline texture bytes are truncated");
    const auto* name=reinterpret_cast<const char*>(bytes+nameHeader+8);
    const auto nameBytes=static_cast<std::size_t>(tableHeader-nameHeader-8);
    const auto* terminator=static_cast<const char*>(std::memchr(name,0,nameBytes));
    Require(terminator && terminator!=name,"Inline texture pack has no name");
    auto& s=*storage_;s.registry=&registry;s.root=&catalogue;
    if(s.root->catalogueRoot)s.root=s.root->catalogueRoot;
    s.name.assign(name,terminator);s.nameKey=hashString(s.name.c_str());
    registry.packs.push_back(this);
    registry.InvalidateDescriptorIndex();
    try {
        AttachTextureTable(std::vector<std::uint8_t>(bytes+tableHeader+8,bytes+dataHeader));
        const auto size=static_cast<std::size_t>(dataEnd-dataBegin);
        if (size) {
            s.inlineData=std::make_shared<TexturePixelStorage>(size,128);
            std::memcpy(s.inlineData->LoadingData(),bytes+dataBegin,size);
            Require(size<=static_cast<std::size_t>(INT32_MAX),"Inline texture bytes exceed source signed extent");
            s.inlineData->PublishReadOnly();
            AssignTextureData(s.inlineData,0,static_cast<std::int32_t>(size));
        }
    } catch (...) {
        if(s.metadata) {
            s.metadata->published=false;
            for(auto& texture:s.metadata->textures)texture.pack=nullptr;
        }
        editorMarkerResourcesDetach(&s.catalogue);
        registry.InvalidateDescriptorIndex();
        registry.packs.erase(std::find(registry.packs.begin(),registry.packs.end(),this));
        throw;
    }
}

bool TexturePack::LoadTextureTableFromFile(const char* filename)
{
    Require(filename && std::strlen(filename)<64,"Texture pack filename exceeds the original valid field");
    auto& s=*storage_;Require(!s.shuttingDown,"Texture pack is shutting down");s.filename=filename;
    Require(s.resources!=nullptr,"Texture physical read has no actual file-system binding");
    std::unique_ptr<PhysicalResource> file;
    try {file=std::make_unique<PhysicalResource>(s.resources->OpenPhysical(s.filename));}
    catch (const std::runtime_error&) {return false;}
    const auto count=static_cast<std::size_t>(std::min<std::uint64_t>(file->range.Size(),0x20000));
    auto bytes=file->range.Read(0,count);
    if (bytes.size()<16)return false;
    // Final TPK header: outer header, name header,164-byte texture table,
    // then the data chunk padded to128 bytes. Offsets remain file-relative.
    const auto second=8ull+ReadU32(bytes.data()+12);
    if (second+16>bytes.size())return false;
    const auto tableHeader=second+8;
    const auto tableBytes=ReadU32(bytes.data()+tableHeader+4);
    if (tableHeader+8ull+tableBytes>bytes.size())return false;
    Require(tableBytes%164==0,"Texture table has a partial164-byte record");
    const auto dataHeader=tableHeader+tableBytes;
    s.dataOffset=(dataHeader+0x8full)&~std::uint64_t{127};
    std::vector<std::uint8_t> table(bytes.begin()+static_cast<std::ptrdiff_t>(tableHeader+8),
        bytes.begin()+static_cast<std::ptrdiff_t>(tableHeader+8+tableBytes));
    UnloadAllTexturesFromFile();WaitForReads();
    editorMarkerResourcesDetach(&s.catalogue);
    s.file=std::move(file);
    AttachTextureTable(std::move(table));
    return true;
}

void TexturePack::AttachTextureTable(std::vector<std::uint8_t> bytes)
{
    Require(!storage_->shuttingDown,"Texture pack is shutting down");
    Require(bytes.size()%164==0,"Texture table has a partial record");
    auto& s=*storage_;auto metadata=std::make_shared<TextureMetadataStorage>();
    metadata->authored=std::move(bytes);
    const auto count=metadata->authored.size()/164;
    metadata->descriptors.resize(count);metadata->textures.resize(count);
    for (std::size_t i=0;i<count;++i) {
        const auto* wire=metadata->authored.data()+i*164;
        auto& descriptor=metadata->descriptors[i];auto& texture=metadata->textures[i];
        descriptor={ReadU32(wire+0x20),wire,164,0};
        texture.descriptor=&descriptor;texture.pack=this;
        texture.pixelFileOffset=Signed(ReadU32(wire+0x30));
        texture.paletteFileOffset=Signed(ReadU32(wire+0x34));
        texture.pixelBytes=Signed(ReadU32(wire+0x38));
        texture.paletteBytes=Signed(ReadU32(wire+0x3c));
        texture.linkedTextureKey=ReadU32(wire+0x40);
        texture.requiresDepthSort=wire[0x4d]!=0;
        texture.drawFlag4Enabled=wire[0x74]!=0;
        Require(texture.pixelFileOffset>=0 && texture.paletteFileOffset>=0 &&
            texture.pixelBytes>=0 && texture.paletteBytes>=0,"Texture data has a negative authored range");
    }
    // Build after all authored keys are populated. This also avoids a
    // quadratic linear search while resolving source linked texture records.
    metadata->EnsureKeyIndex();
    if(s.metadata) {
        s.metadata->published=false;
        for(auto& texture:s.metadata->textures)texture.pack=nullptr;
    }
    s.metadata=metadata;
    s.registry->InvalidateDescriptorIndex();
    // The full table is installed before any linked-record lookup, matching
    // source AttachTextureTable's reference resolution and first-key order.
    for (auto& texture:metadata->textures)
        texture.linkedTexture=texture.linkedTextureKey ? GetTexture(texture.linkedTextureKey) : &texture;
    s.catalogue.textures=metadata->descriptors.data();s.catalogue.textureCount=count;
    Require(editorMarkerResourcesAttach(s.root,&s.catalogue)!=0,"Texture catalogue cannot join its live model registry");
    ++s.root->bindingRevision;
    s.root->texturesChanged=1;
}

TextureInfo* TexturePack::GetTexture(std::uint32_t key)
{
    auto* metadata=storage_->metadata.get();
    if(!metadata || !metadata->published)return nullptr;
    metadata->EnsureKeyIndex();
    if(metadata->keyIndexValid) {
        const auto found=metadata->byKey.find(key);
        if(found==metadata->byKey.end())return nullptr;
        return &metadata->textures[found->second.front()];
    }
    for(auto& texture:metadata->textures)
        if(texture.descriptor->key==key)return &texture;
    return nullptr;
}

TextureInfo* TexturePack::GetLoadedTexture(std::uint32_t key)
{
    if(storage_->failure)std::rethrow_exception(storage_->failure);
    auto* metadata=storage_->metadata.get();
    if(!metadata || !metadata->published)return nullptr;
    metadata->EnsureKeyIndex();
    if(metadata->keyIndexValid) {
        const auto found=metadata->byKey.find(key);
        if(found==metadata->byKey.end())return nullptr;
        for(const auto index:found->second) {
            auto& texture=metadata->textures[index];
            if(texture.pixels.owner && (!texture.paletteBytes || texture.palette.owner))return &texture;
        }
        return nullptr;
    }
    for(auto& texture:metadata->textures)
        if(texture.descriptor->key==key && texture.pixels.owner &&
            (!texture.paletteBytes || texture.palette.owner))return &texture;
    return nullptr;
}

TextureInfo* TexturePack::GetNthTexture(std::int32_t index)
{
    if (!storage_->metadata || !storage_->metadata->published || index<0 || static_cast<std::size_t>(index)>=storage_->metadata->textures.size())return nullptr;
    return &storage_->metadata->textures[static_cast<std::size_t>(index)];
}

void TexturePack::SetDescriptorByte29(EditorMarkerTexture* descriptor,std::uint8_t value)
{
    Require(storage_->metadata!=nullptr,"Texture descriptor has no retained metadata");
    auto& metadata=*storage_->metadata;
    for (std::size_t index=0;index<metadata.descriptors.size();++index)
        if (&metadata.descriptors[index]==descriptor) {
            metadata.authored[index*164+0x29]=value;
            return;
        }
    throw std::runtime_error("Texture descriptor belongs to a different pack");
}

void TexturePack::SetDescriptorByte4F(EditorMarkerTexture* descriptor,std::uint8_t value)
{
    Require(storage_->metadata!=nullptr,"Texture descriptor has no retained metadata");
    auto& metadata=*storage_->metadata;
    for (std::size_t index=0;index<metadata.descriptors.size();++index)
        if (&metadata.descriptors[index]==descriptor) {
            metadata.authored[index*164+0x4f]=value;
            return;
        }
    throw std::runtime_error("Texture descriptor belongs to a different pack");
}

LoadedTextureData* TexturePack::FindLoadedTextureData(std::int32_t begin,std::int32_t bytes)
{
    const auto end=Add(begin,bytes);
    for (auto& region:storage_->regions)
        if (begin>=region->begin && region->end>=end)return region.get();
    return nullptr;
}

LoadedTextureData* TexturePack::FindNotYetLoadedNeighbourTextureData(std::int32_t begin,
    std::int32_t bytes,std::int32_t unused)
{
    (void)unused;const auto end=Add(begin,bytes);
    for (auto& owned:storage_->regions) {
        auto& region=*owned;
        if (region.data)continue;
        if (!(end<Add(region.begin,-1024)) && !(region.begin<end))return &region;
        if (!(Add(region.end,1024)<begin) && !(begin<region.end))return &region;
    }
    return nullptr;
}

void TexturePack::AddLoadedTextureData(std::int32_t begin,std::int32_t bytes)
{
    auto& s=*storage_;
    Require(!s.shuttingDown,"Texture pack is shutting down");
    if (auto* existing=FindLoadedTextureData(begin,bytes)) {++existing->references;return;}
    auto* region=FindNotYetLoadedNeighbourTextureData(begin,bytes);
    if (region) {
        region->begin=std::min(region->begin,begin);region->end=std::max(region->end,Add(begin,bytes));
        ++region->references;
        auto* neighbour=FindNotYetLoadedNeighbourTextureData(region->begin,region->end-region->begin);
        if (neighbour) {
            Require(neighbour!=region,"Texture coalescing selected its own nonempty range");
            region->begin=std::min(region->begin,neighbour->begin);region->end=std::max(region->end,neighbour->end);
            region->references+=neighbour->references;
            const auto it=std::find_if(s.regions.begin(),s.regions.end(),[&](const auto& p){return p.get()==neighbour;});
            s.regions.erase(it);s.serviceCounter=Add(s.serviceCounter,-1);
        }
        return;
    }
    auto owned=std::make_unique<LoadedTextureData>();owned->begin=begin;owned->end=Add(begin,bytes);
    owned->references=1;owned->allocationFlags=readAllocationFlags;
    s.regions.push_back(std::move(owned));s.serviceCounter=Add(s.serviceCounter,1);
    // Exact source250550 stable adjacent-swap sort. Callback markers are
    // barriers; do not give std::sort the source's non-strict predicate.
    bool changed;
    do {
        changed=false;
        for (std::size_t i=1;i<s.regions.size();++i) {
            const auto& a=*s.regions[i-1];const auto& b=*s.regions[i];
            if (!a.completion && !a.carCompletion && !a.gameFlow && a.legalCompletion==FrontendLegalTextureCompletion::None &&
                !b.completion && !b.carCompletion && !b.gameFlow && b.legalCompletion==FrontendLegalTextureCompletion::None && b.begin<a.begin) {
                std::swap(s.regions[i-1],s.regions[i]);changed=true;
            }
        }
    } while (changed);
}

void TexturePack::AssignTextureData(const std::shared_ptr<TexturePixelStorage>& data,
    std::int32_t begin,std::int32_t count)
{
    auto& s=*storage_;
    if (s.metadata)for (auto& texture:s.metadata->textures) {
        if (texture.pixelBytes && BeginsInside(texture.pixelFileOffset,begin,count))
            texture.pixels={data,static_cast<std::size_t>(texture.pixelFileOffset-begin),static_cast<std::size_t>(texture.pixelBytes)};
        if (texture.paletteBytes && BeginsInside(texture.paletteFileOffset,begin,count))
            texture.palette={data,static_cast<std::size_t>(texture.paletteFileOffset-begin),static_cast<std::size_t>(texture.paletteBytes)};
    }
    ++s.root->bindingRevision;
    s.root->texturesChanged=1;
}

void TexturePack::UnAssignTextureData(std::int32_t begin,std::int32_t count)
{
    auto& s=*storage_;
    if (s.metadata)for (auto& texture:s.metadata->textures) {
        if (texture.pixelBytes && BeginsInside(texture.pixelFileOffset,begin,count))texture.pixels={};
        if (texture.paletteBytes && BeginsInside(texture.paletteFileOffset,begin,count))texture.palette={};
    }
    ++s.root->bindingRevision;
    s.root->texturesChanged=1;
}

void TexturePack::DeleteLoadedTextureData(LoadedTextureData* region)
{
    Require(region && region!=storage_->activeRead,"Cannot dispose an active texture read");
    auto& s=*storage_;
    if (region->loaded)UnAssignTextureData(region->begin,region->end-region->begin);
    else s.serviceCounter=Add(s.serviceCounter,-1);
    const auto it=std::find_if(s.regions.begin(),s.regions.end(),[&](const auto& p){return p.get()==region;});
    Require(it!=s.regions.end(),"Texture region is not owned by this pack");
    // Source111DD8 waits for rendering before freeing memory. Native byte
    // views retain the actual allocation until their final consumer releases.
    s.regions.erase(it);
}

void TexturePack::LoadTextureFromFile(TextureInfo* texture)
{
    Require(!storage_->shuttingDown,"Texture pack is shutting down");
    Require(texture && texture->pack==this,"Texture request has a different pack owner");
    bool pixelsAdded=false;
    try {
        if (texture->pixelBytes) {
            AddLoadedTextureData(texture->pixelFileOffset,texture->pixelBytes);
            pixelsAdded=true;
        }
        if (texture->paletteBytes)AddLoadedTextureData(texture->paletteFileOffset,texture->paletteBytes);
    } catch(...) {
        // A native allocation failure between pixels and palette has not
        // published a request. Retire only the reference acquired here.
        if(pixelsAdded)RemoveLoadedTextureData(texture->pixelFileOffset,texture->pixelBytes);
        throw;
    }
}

bool TexturePack::RemoveLoadedTextureData(std::int32_t begin,std::int32_t bytes)
{
    auto* region=FindLoadedTextureData(begin,bytes);
    if (!region)return false;
    Require(region->references>0,"Texture region reference counter would underflow");
    --region->references;
    if (!region->references && !region->loading)DeleteLoadedTextureData(region);
    return true;
}

void TexturePack::UnloadTextureFromFile(TextureInfo* texture)
{
    Require(texture && texture->pack==this,"Texture unload has a different pack owner");
    auto* pixels=texture->pixelBytes ? FindLoadedTextureData(texture->pixelFileOffset,texture->pixelBytes):nullptr;
    auto* palette=texture->paletteBytes ? FindLoadedTextureData(texture->paletteFileOffset,texture->paletteBytes):nullptr;
    // Validate both acquisitions before consuming either one. This also
    // covers a merged region which owns the pixel and palette references.
    Require(!pixels || pixels->references>0,"Texture pixels have no acquired reference");
    Require(!palette || palette->references>(pixels==palette ? 1u:0u),
        "Texture palette has no acquired reference");
    if (texture->pixelBytes)RemoveLoadedTextureData(texture->pixelFileOffset,texture->pixelBytes);
    if (texture->paletteBytes)RemoveLoadedTextureData(texture->paletteFileOffset,texture->paletteBytes);
}

void TexturePack::UnloadTexturesFromFile(const std::uint32_t* keys,std::int32_t count)
{
    Require(count<=0 || keys,"Texture unload has no input key span");
    for (std::int32_t index=0;index<count;++index)
        if (keys[index])if (auto* texture=GetTexture(keys[index]))UnloadTextureFromFile(texture);
}

bool SwapTextureName(TexturePackRegistry& registry,std::uint32_t first,std::uint32_t second)
{
    const auto find=[&](std::uint32_t key) -> TextureInfo* {
        for (auto* pack:registry.packs)
            if (auto* texture=pack->GetTexture(key))return texture;
        return nullptr;
    };
    auto* a=find(first);auto* b=find(second);
    if (!a || !b)return false;
    const auto nameBytes=[](TextureInfo& texture) {
        auto& metadata=*texture.pack->storage_->metadata;
        const auto index=static_cast<std::size_t>(&texture-metadata.textures.data());
        Require(index<metadata.textures.size(),"Texture name is outside its metadata owner");
        auto* bytes=metadata.authored.data()+index*164+8;
        Require(std::memchr(bytes,0,24)!=nullptr,"Authored texture name exceeds its24-byte field");
        return bytes;
    };
    auto* an=nameBytes(*a);auto* bn=nameBytes(*b);
    if (an==bn)return true;
    std::array<std::uint8_t,24> saved{};
    std::memcpy(saved.data(),an,saved.size());
    std::memcpy(an,bn,saved.size());
    a->descriptor->key=hashString(reinterpret_cast<const char*>(an));
    PutU32(an+24,a->descriptor->key);
    std::memcpy(bn,saved.data(),saved.size());
    b->descriptor->key=hashString(reinterpret_cast<const char*>(bn));
    PutU32(bn+24,b->descriptor->key);
    // Pack keys are writable authored metadata; rebuild both owners only
    // after this atomic two-record name swap has completed.
    a->pack->storage_->metadata->keyIndexValid=false;
    b->pack->storage_->metadata->keyIndexValid=false;
    return true;
}

void TexturePack::LoadTexturesFromFile(const std::uint32_t* keys,std::int32_t count,
    SelectTrackSlideshow* completion)
{LoadTextureKeys(keys,count,completion,nullptr);}

void TexturePack::LoadTexturesFromFile(const std::uint32_t* keys,std::int32_t count,CarLoader* completion)
{LoadTextureKeys(keys,count,nullptr,completion);}

void TexturePack::LoadTexturesFromFile(const std::uint32_t* keys,std::int32_t count,FrontendLegalTextureCompletion completion)
{LoadTextureKeys(keys,count,nullptr,nullptr,completion);}

void TexturePack::LoadTexturesFromFile(const std::uint32_t* keys,std::int32_t count,
    ApplicationFrontendFlow& flow,const GameFlowBindings& bindings)
{LoadTextureKeys(keys,count,nullptr,nullptr,FrontendLegalTextureCompletion::None,&flow,&bindings);}

void TexturePack::LoadTextureKeys(const std::uint32_t* keys,std::int32_t count,
    SelectTrackSlideshow* completion,CarLoader* car,FrontendLegalTextureCompletion legal,
    ApplicationFrontendFlow* flow,const GameFlowBindings* bindings)
{
    Require(!storage_->shuttingDown,"Texture pack is shutting down");
    Require(count<=0 || keys,"Texture request has no input key span");
    std::vector<TextureInfo*> acquired;
    if(count>0)acquired.reserve(static_cast<std::size_t>(count));
    try {
        for (std::int32_t i=0;i<count;++i)
            if (keys[i])if (auto* texture=GetTexture(keys[i])) {
                LoadTextureFromFile(texture);
                acquired.push_back(texture); // capacity was reserved before acquiring references
            }
        if (completion || car || legal!=FrontendLegalTextureCompletion::None || flow) {
            auto marker=std::make_unique<LoadedTextureData>();marker->allocationFlags=readAllocationFlags;marker->completion=completion;
            marker->carCompletion=car;
            marker->legalCompletion=legal;
            marker->gameFlow=flow;marker->gameFlowBindings=bindings;
            storage_->regions.push_back(std::move(marker));storage_->serviceCounter=Add(storage_->serviceCounter,1);
        }
    } catch(...) {
        // Before publication, every successfully acquired pixel/palette
        // reference belongs to this call. A failed marker allocation must
        // not strand them without the consumer's loading completion.
        for(auto it=acquired.rbegin();it!=acquired.rend();++it)UnloadTextureFromFile(*it);
        throw;
    }
    // After publication the marker owns the real completion. Keep its
    // references on a read-start exception so shutdown/retry can drain it.
    ServiceLoadingTextures();
}

bool TexturePack::HasCarCompletion(const CarLoader* car) const noexcept
{
    for(const auto& region:storage_->regions)if(region->carCompletion==car)return true;
    return false;
}

void TexturePack::ServiceLoadingTextures()
{
    auto& s=*storage_;
    if(s.shuttingDown)return;
    if (s.failure)std::rethrow_exception(s.failure);
    if (s.activeRead || !s.serviceCounter)return;
    for (const auto& owned:s.regions) {
        auto* region=owned.get();
        if (region->data)continue;
        if (region->begin==region->end) {
            auto* completion=region->completion;
            auto* car=region->carCompletion;
            const auto legal=region->legalCompletion;
            auto* flow=region->gameFlow;
            const auto* flowBindings=region->gameFlowBindings;
            s.serviceCounter=Add(s.serviceCounter,1);
            const auto it=std::find_if(s.regions.begin(),s.regions.end(),[&](const auto& q){return q.get()==region;});
            s.regions.erase(it);
            if (completion)completion->TextureLoadedCallback();
            if (car)car->LoadingDoneCallback();
            if (legal==FrontendLegalTextureCompletion::Legal)CallbackFinishedLegalScreen();
            if (legal==FrontendLegalTextureCompletion::AntiPiracy)CallbackFinishedAntiPiracyScreen();
            if (flow) {
                Require(flowBindings!=nullptr,"HUD texture completion lost its application flow");
                GameFlowLoadingTrackSpecificPart3(*flow,*flowBindings);
            }
            ServiceLoadingTextures();
            return;
        }
        Require(s.file && region->begin>=0 && region->end>region->begin,"Texture read has no valid retained file/range");
        const auto bytes=static_cast<std::size_t>(region->end-region->begin);
        const auto offset=s.dataOffset+static_cast<std::uint32_t>(region->begin);
        Require(offset<=s.file->range.Size() && bytes<=s.file->range.Size()-offset,
            "Texture read exceeds its actual file");
        Require(s.root->resourceRegistry && s.root->resourceRegistry->QueuedFiles(),
            "Texture physical read requires the actual shared queued-file owner");
        s.queued=s.root->resourceRegistry->QueuedFiles();
        Require(&s.queued->Files()==s.resources,"Texture and queue must borrow the same physical file owner");
        Require(offset<=static_cast<std::uint64_t>(INT32_MAX) && bytes<=static_cast<std::size_t>(INT32_MAX),
            "Texture queued read exceeds its signed source extent");
        const auto flags=region->allocationFlags;
        const std::size_t alignment=flags&0x40u ? 128u : flags&0x20u ? 64u : flags&0x10u ? 32u : 4u;
        region->data=std::make_shared<TexturePixelStorage>(bytes,std::max(alignment,sizeof(void*)));
        try {
            s.queuedId=s.queued->Add(region->data->LoadingData(),s.filename.c_str(),static_cast<std::int32_t>(offset),
                static_cast<std::int32_t>(bytes),{QueuedFileCompletionKind::TextureRegion,nullptr,this});
        } catch (...) {region->data.reset();throw;}
        region->loading=true;s.activeRead=region;s.serviceCounter=Add(s.serviceCounter,-1);
        return;
    }
}

void TexturePack::QueuedReadDone(bool failed)
{
    auto& s=*storage_;
    Require(s.queuedId && s.activeRead,"Texture completion has no actual queued read");
    s.queuedId=0;
    if (failed) {
        s.activeRead->loading=false;s.activeRead->data.reset();s.activeRead=nullptr;
        s.failure=std::make_exception_ptr(std::runtime_error("Texture physical transfer failed"));
        return;
    }
    ReadCallback();
}

void TexturePack::ReadCallback()
{
    auto& s=*storage_;auto* region=s.activeRead;
    try {
        Require(region && region->data,"Texture completion has no actual active read");
        s.activeRead=nullptr;
        region->data->PublishReadOnly();
        AssignTextureData(region->data,region->begin,region->end-region->begin);
        auto* root=s.root->catalogueRoot?s.root->catalogueRoot:s.root;
        Require(root->resourceRegistry!=nullptr,"Texture callback has no actual shared CPU refresh owner");
        root->resourceRegistry->ResourceRefresh();
        region->loading=false;region->loaded=true;
        if (!region->references)DeleteLoadedTextureData(region);
    } catch (...) {
        for (auto& current:s.regions)if (current.get()==region)current->loading=false;
        if (s.activeRead==region)s.activeRead=nullptr;
        s.failure=std::current_exception();throw;
    }
    // A following consumer may fail while requesting a different resource.
    // Preserve that exception without marking this completed texture failed.
    ServiceLoadingTextures();
}

void TexturePack::UnloadAllTexturesFromFile()
{
    auto& s=*storage_;
    std::size_t i=0;
    while (i<s.regions.size()) {
        auto* region=s.regions[i].get();region->references=0;
        if (!region->loading)DeleteLoadedTextureData(region);
        else ++i;
    }
    s.serviceCounter=0;
}

std::uint32_t TexturePack::GetLoadedSize() const
{
    std::uint32_t result=0;
    for (const auto& region:storage_->regions)
        result+=static_cast<std::uint32_t>(region->end)-static_cast<std::uint32_t>(region->begin);
    return result;
}

void TexturePack::ServiceRead()
{
    auto& s=*storage_;
    if (s.queued)s.queued->Service();
    ServiceLoadingTextures();
}

void TexturePack::WaitForReads()
{
    auto& s=*storage_;
    if(s.shuttingDown) {
        if(s.queuedId) {
            Require(s.queued!=nullptr,"Texture read lost its queued owner");
            s.queued->CancelAndWait(s.queuedId);s.queuedId=0;
        }
        if(s.activeRead) {s.activeRead->loading=false;s.activeRead=nullptr;}
        return;
    }
    while (s.activeRead) {Require(s.queued!=nullptr,"Texture read lost its queued owner");s.queued->Service();}
    if (s.failure)std::rethrow_exception(s.failure);
}

void TexturePack::BeginShutdown() noexcept
{
    auto& s=*storage_;s.shuttingDown=true;
    for(auto& region:s.regions) {
        region->completion=nullptr;region->carCompletion=nullptr;
        region->legalCompletion=FrontendLegalTextureCompletion::None;
        region->gameFlow=nullptr;region->gameFlowBindings=nullptr;
    }
}

std::exception_ptr TexturePack::Shutdown() noexcept
{
    BeginShutdown();auto& s=*storage_;
    if(s.shutdownComplete)return s.shutdownFailure;
    if(!s.shutdownFailure && s.failure)s.shutdownFailure=s.failure;
    try {WaitForReads();}
    catch(...) {
        if(!s.shutdownFailure)s.shutdownFailure=std::current_exception();
        return s.shutdownFailure;
    }
    if(s.metadata) {
        s.metadata->published=false;
        for(auto& texture:s.metadata->textures) {
            texture.pack=nullptr;texture.pixels={};texture.palette={};
        }
    }
    editorMarkerResourcesDetach(&s.catalogue);
    s.catalogue.textures=nullptr;s.catalogue.textureCount=0;
    s.registry->InvalidateDescriptorIndex();
    auto& packs=s.registry->packs;
    const auto found=std::find(packs.begin(),packs.end(),this);
    if(found!=packs.end())packs.erase(found);
    ++s.root->bindingRevision;s.root->texturesChanged=1;
    s.regions.clear();s.inlineData.reset();s.metadata.reset();s.file.reset();
    s.serviceCounter=0;s.queued=nullptr;s.shutdownComplete=true;
    return s.shutdownFailure;
}

bool TexturePack::HasPendingReads() const noexcept {return storage_->activeRead!=nullptr;}

void TexturePack::DetachFlowCompletions(const ApplicationFrontendFlow& flow) noexcept
{
    for (const auto& region:storage_->regions)if (region->gameFlow==&flow) {
        region->gameFlow=nullptr;region->gameFlowBindings=nullptr;
    }
}

void TexturePackRegistry::ServiceReads()
{
    // Callbacks may remove a pack. Advance through a stable snapshot of
    // borrowed identities and recheck membership before delivering an event.
    const auto pending=packs;
    std::vector<QueuedFileService*> services;
    for (auto* pack:pending) {
        auto* registry=pack->CatalogueRoot().resourceRegistry;
        auto* service=registry ? registry->QueuedFiles() : nullptr;
        if (service && std::find(services.begin(),services.end(),service)==services.end())services.push_back(service);
    }
    for (auto* service:services)service->Service();
    for (auto* pack:pending)
        if (std::find(packs.begin(),packs.end(),pack)!=packs.end())pack->ServiceLoadingTextures();
}

std::size_t TexturePack::TextureCount() const
{
    return storage_->metadata ? storage_->metadata->textures.size() : 0;
}

const std::string& TexturePack::Name() const {return storage_->name;}
TexturePackRegistry& TexturePack::Registry() const noexcept {return *storage_->registry;}
EditorMarkerResources& TexturePack::CatalogueRoot() const noexcept
{ return storage_->root->catalogueRoot?*storage_->root->catalogueRoot:*storage_->root; }

std::shared_ptr<const TextureMetadataStorage> TexturePack::MetadataOwnerFor(const EditorMarkerTexture* descriptor) const
{
    if (storage_->metadata)for (const auto& entry:storage_->metadata->descriptors)
        if (&entry==descriptor)return storage_->metadata;
    return {};
}

std::shared_ptr<const TextureMetadataStorage> textureMetadataOwner(const EditorMarkerTexture* descriptor)
{
    for (auto* pack:texturePackRegistry.packs)
        if (auto owner=pack->MetadataOwnerFor(descriptor))return owner;
    return {};
}

bool textureMetadataPublished(const std::shared_ptr<const TextureMetadataStorage>& metadata) noexcept
{
    return metadata && metadata->published;
}

TextureInfo* TexturePackRegistry::LookupDescriptor(const EditorMarkerTexture* descriptor) noexcept
{
    if (!descriptor)return nullptr;
    if (!descriptorIndexValid_) {
        // Keep the previous published map intact until the replacement is
        // completely built. An allocation failure uses the proven linear
        // path for this frame instead of losing material/texture ownership.
        try {
            std::size_t count=0;
            for(const auto* pack:packs)count+=pack->TextureCount();
            std::unordered_map<const EditorMarkerTexture*,TextureInfo*> rebuilt;
            rebuilt.reserve(count);
            for(auto* pack:packs)for(std::size_t index=0;index<pack->TextureCount();++index) {
                auto* texture=pack->GetNthTexture(static_cast<std::int32_t>(index));
                if(texture && texture->descriptor)
                    rebuilt.emplace(texture->descriptor,texture);
            }
            descriptorIndex_.swap(rebuilt);
            descriptorIndexValid_=true;
        } catch (...) {
            // The caller's interface is noexcept. A failed PC optimization
            // must preserve the original resource resolution behavior.
        }
    }
    if (descriptorIndexValid_) {
        const auto hit=descriptorIndex_.find(descriptor);
        return hit==descriptorIndex_.end() ? nullptr:hit->second;
    }
    for(auto* pack:packs)for(std::size_t index=0;index<pack->TextureCount();++index) {
        auto* texture=pack->GetNthTexture(static_cast<std::int32_t>(index));
        if(texture && texture->descriptor==descriptor)return texture;
    }
    return nullptr;
}

TextureInfo* textureInfoForDescriptor(const EditorMarkerTexture* descriptor) noexcept
{
    return texturePackRegistry.LookupDescriptor(descriptor);
}

} // namespace hp2
