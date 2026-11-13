#include "FlailerChunks.hpp"
#include "FlailerDB.hpp"
#include "indep/src/MoviePlayer/MV3Codec.h"
#include "Texture.hpp"
#include "Language.hpp"
#include "bware/inc/bWare.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/eMath.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bChunk.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bFile.hpp"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <memory>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace hp2 {

// PAL global constructor1464D8 initializes the clip window to644x480.
// The writable raster bytes33019C..3301A1 and help mapping2F09C0..DD
// remain one live state, rather than constants copied into each text object.
FlailerTextRasterState flailerTextRasterState{};
// The actual145768/1457D0 writers update2F09A0+14/+18: live strips and
// signed high-water count. All texture batches participate in the same cells.
FlailerRuntimeStatistics flailerRuntimeStatistics{};

void SetFlailerTextClipWindow(float left,float top,float right,float bottom)
{
    flailerTextRasterState.minimum={left,top};
    flailerTextRasterState.maximum={right,bottom};
}

void SetFlailerTextClipWindow(const float minimum[2],const float maximum[2])
{
    flailerTextRasterState.minimum[0]=minimum[0];
    flailerTextRasterState.minimum[1]=minimum[1];
    flailerTextRasterState.maximum[0]=maximum[0];
    flailerTextRasterState.maximum[1]=maximum[1];
}

void ResetFlailerTextClipWindow()
{
    flailerTextRasterState.maximum={644.0f,480.0f};
    flailerTextRasterState.minimum={0.0f,0.0f};
}

std::uint32_t next_power_of_2(std::uint32_t value)
{
    if (!value) return 0;
    value=(value-1u)>>1;
    std::uint32_t result=2;
    while (value) { result<<=1;value>>=1; }
    return result;
}

float Font::GetKerning(int nextCharacter,const tTTGlyph* glyph) const
{
    const auto next=static_cast<std::uint8_t>(nextCharacter);
    for (std::int32_t index=0;index<glyph->kerningCount;++index) {
        const auto& pair=kernings[glyph->firstKerning+static_cast<std::uint32_t>(index)];
        if (pair.nextCharacter==next) return pair.adjustment;
    }
    return 0.0f;
}

float Font::GetWidestCharacter(const char* text) const
{
    float widest=0.0f;
    for (const auto* cursor=reinterpret_cast<const unsigned char*>(text);*cursor;++cursor) {
        const tTTGlyph* glyph=characters[*cursor];
        if (glyph && widest<glyph->advance) widest=glyph->advance;
    }
    return widest;
}

void FlailerObjectChunk::SetScaleX(float value)
{
    if (scaleX!=value) { scaleX=value;updateKind=3; }
}

void FlailerObjectChunk::SetScaleY(float value)
{
    if (scaleY!=value) { scaleY=value;updateKind=3; }
}

void FlailerObjectChunk::SetTopLeftX(float value)
{
    if (topLeftX!=value) { topLeftX=value;updateKind=4; }
}

void FlailerObjectChunk::SetTopLeftY(float value)
{
    if (topLeftY!=value) { topLeftY=value;updateKind=4; }
}

// Complete original SetClipToPoly140A70..140AA8. The source clip word is
// shared by all object kinds; only text also enables the clipping format bit.
void FlailerObjectChunk::SetClipToPoly(std::uint32_t value)
{
    if(clipObjectKey==value)return;
    updateKind=4;clipObjectKey=value;
    if(kind==1)static_cast<FlailerTextChunk*>(this)->SetFormatOptions(0x10);
}

void FlailerTextChunk::SetFormatOptions(std::int32_t options)
{
    updateKind=5;
    formatOptions|=static_cast<std::uint32_t>(options);
}

void FlailerTextChunk::ClearFormatOptions(std::int32_t options)
{
    updateKind=5;
    formatOptions&=~static_cast<std::uint32_t>(options);
}

void FlailerTextChunk::SetColor(std::uint32_t rgb)
{
    const std::uint32_t replacement=(color&0xff000000u)|(rgb&0x00ffffffu);
    if (color!=replacement) { color=replacement;updateKind=7; }
}

void FlailerTextChunk::UpdateLanguageText()
{
    updateKind=5;
}

void FlailerScreenResource::UpdateLanguageText()
{
    for (auto* object=objects.next;object!=&objects;object=object->next)
        if (object->kind==1)static_cast<FlailerTextChunk*>(object)->UpdateLanguageText();
}

// Complete PAL140300..140374 DeleteAllFlailerStrips. A cached bitmap
// descriptor is borrowed from a texture pack, so deleting the strip must
// also clear that binding and mark the bitmap for the next loaded lookup.
void FlailerScreenResource::DeleteAllFlailerStrips()
{
    for (auto* object=objects.next;object!=&objects;object=object->next) {
        object->DeleteFlailerStrip();
        if (object->kind==2) {
            auto* bitmap=static_cast<FlailerBitmapChunk*>(object);
            bitmap->texture=nullptr;bitmap->textureStorage.reset();bitmap->updateKind=10;
        }
    }
}

void FlailerUpdateLanguageText(FlailerResources& resources)
{
    auto& root=resources.PackageRoot();
    for (auto* package=root.next;package!=&root;package=package->next)
        for (auto* screen=package->screenRoot.next;screen!=&package->screenRoot;screen=screen->next)
            screen->UpdateLanguageText();
    // PAL1416B4 shares the same global strip/binding retirement tail.
    resources.DeleteAllStrips();
}

void FlailerTextChunk::SetAlpha(std::int32_t alpha)
{
    const std::uint32_t replacement=(color&0x00ffffffu)|(static_cast<std::uint32_t>(alpha)<<24);
    if (color!=replacement) { color=replacement;updateKind=6; }
}

std::int32_t FlailerTextChunk::GetAlpha() const
{
    return static_cast<std::int32_t>(color>>24);
}

void FlailerBitmapChunk::SetAlpha(std::int32_t alpha,std::int32_t corner)
{
    updateKind=6;
    const std::uint32_t value=static_cast<std::uint32_t>(alpha)<<24;
    if (corner==-1) {
        for (auto& color:colors) color=(color&0x00ffffffu)|value;
    } else {
        auto& color=colors[static_cast<std::size_t>(corner)];
        color=(color&0x00ffffffu)|value;
    }
}

void FlailerBitmapChunk::SetRotation(float value)
{
    const std::int32_t replacement=convertFloatToInt32(value);
    if (rotation!=replacement) { rotation=replacement;updateKind=9; }
}

void FlailerBitmapChunk::SetColor(std::uint32_t value)
{
    if (colors[0]!=value) updateKind=7;
    colors[3]=value;colors[0]=value;colors[1]=value;colors[2]=value;
}

void FlailerBitmapChunk::ChangeTexture(const EditorMarkerTexture* value)
{
    const std::uint32_t textureHash=value->key;
    updateKind=10;textureKey=textureHash;texture=value;
    textureStorage=textureMetadataOwner(value);
}

void FlailerBitmapChunk::ChangeTexture(std::uint32_t textureHash)
{
    textureKey=textureHash;updateKind=10;texture=nullptr;
    textureStorage.reset();
}

std::int32_t FlailerBitmapChunk::GetAlpha(std::int32_t corner) const
{
    return static_cast<std::int32_t>(colors[static_cast<std::size_t>(corner)]>>24);
}

bool MaybeClipPoly(FlailerPoly* polygon,const float minimum[2],const float maximum[2])
{
    auto& vertices=polygon->vertices;
    const float oldRight=vertices[1].position[0];
    const float oldLeft=vertices[0].position[0];
    const float oldBottom=vertices[2].position[1];
    const float oldTop=vertices[0].position[1];
    if (oldRight<minimum[0] || maximum[0]<=oldLeft ||
        oldBottom<minimum[1] || maximum[1]<=oldTop) return true;
    if (oldLeft<minimum[0]) {
        const float amount=std::fabs((minimum[0]-oldLeft)/(oldRight-oldLeft));
        const float complement=1.0f-amount;
        const float right=amount*oldRight;
        const float newU=complement*vertices[0].textureCoordinate[0]+amount*vertices[1].textureCoordinate[0];
        const float newX=complement*oldLeft+right;
        vertices[0].textureCoordinate[0]=newU;
        vertices[0].position[0]=newX;vertices[3].position[0]=newX;
        vertices[3].textureCoordinate[0]=newU;
    }
    if (maximum[0]<=oldRight) {
        const float left=vertices[0].position[0];
        const float amount=std::fabs((maximum[0]-oldRight)/(oldRight-left));
        const float complement=1.0f-amount;
        const float leftContribution=amount*left;
        const float newU=amount*vertices[0].textureCoordinate[0]+complement*vertices[1].textureCoordinate[0];
        const float newX=leftContribution+complement*oldRight;
        vertices[1].textureCoordinate[0]=newU;
        vertices[1].position[0]=newX;vertices[2].position[0]=newX;
        vertices[2].textureCoordinate[0]=newU;
    }
    if (oldTop<minimum[1]) {
        const float amount=std::fabs((minimum[1]-oldTop)/(oldBottom-oldTop));
        const float complement=1.0f-amount;
        const float bottom=amount*oldBottom;
        const float newV=complement*vertices[0].textureCoordinate[1]+amount*vertices[2].textureCoordinate[1];
        const float newY=complement*oldTop+bottom;
        vertices[0].textureCoordinate[1]=newV;
        vertices[0].position[1]=newY;vertices[1].position[1]=newY;
        vertices[1].textureCoordinate[1]=newV;
    }
    if (maximum[1]<=oldBottom) {
        const float top=vertices[0].position[1];
        const float amount=std::fabs((maximum[1]-oldBottom)/(oldBottom-top));
        const float complement=1.0f-amount;
        const float topContribution=amount*top;
        const float newV=amount*vertices[0].textureCoordinate[1]+complement*vertices[2].textureCoordinate[1];
        const float newY=topContribution+complement*oldBottom;
        vertices[2].textureCoordinate[1]=newV;
        vertices[2].position[1]=newY;vertices[3].position[1]=newY;
        vertices[3].textureCoordinate[1]=newV;
    }
    return false;
}

float FlailerTextDistanceToNextChar(const char* text,char delimiter,const Font* font)
{
    float distance=0.0f;
    if (!text) return distance;
    for (const auto* cursor=reinterpret_cast<const unsigned char*>(text);
         *cursor && *cursor!=static_cast<unsigned char>(delimiter);++cursor) {
        if (const tTTGlyph* glyph=font->characters[*cursor])
            distance+=static_cast<float>(convertFloatToInt32(glyph->advance));
    }
    return distance;
}

bool IsHelpCharacter(std::uint8_t value)
{
    return static_cast<std::uint8_t>(value+0x80u)<0x1eu && value!=0x92u;
}

FlailerObjectChunk* FindFlailerObject(FlailerObjectChunk* root,std::uint32_t key)
{
    for (auto* object=root->previous;object!=root;object=object->previous)
        if (object->key==key) return object;
    return nullptr;
}

std::uint32_t GetEPolyChecksum(const FlailerPoly* polygon)
{
    std::uint32_t sum=0;
    for (const auto& vertex:polygon->vertices) {
        for (float value:vertex.position) { std::uint32_t bits;std::memcpy(&bits,&value,4);sum+=bits; }
        for (float value:vertex.textureCoordinate) { std::uint32_t bits;std::memcpy(&bits,&value,4);sum+=bits; }
        sum+=vertex.color;
    }
    for (std::uint32_t value:polygon->rasterState) sum+=value;
    return sum;
}

bool FlailerRenderManager::IsScreenVisible(std::uint32_t key) const
{
    for (const auto* screen:visibleScreens)
        if (screen && screen->key==key) return true;
    return false;
}

struct FlailerResources::Storage {
    struct FontStorage {
        Font font{};
        std::vector<tTTGlyph> glyphs;
        std::vector<FontKerning> kernings;
    };
    struct Bundle {
        std::uint64_t token{};
        std::size_t offset{},size{};
        std::vector<FontStorage*> fonts;
        std::vector<FlailerPackageResource*> packages;
        std::vector<FlailerScreenResource*> screens;
        std::vector<FlailerTextChunk*> text;
        std::vector<FlailerBitmapChunk*> bitmaps;
        std::vector<TrackRouteEditorResources*> models;
    };
    std::vector<Bundle> bundles;
    std::uint64_t nextBundle{1};
    std::vector<std::uint8_t> authored;
    std::vector<std::unique_ptr<FontStorage>> fonts;
    FlailerPackageResource packageRoot{};
    std::vector<std::unique_ptr<FlailerPackageResource>> packages;
    std::vector<std::unique_ptr<FlailerScreenResource>> screens;
    std::vector<std::unique_ptr<FlailerTextChunk>> text;
    std::vector<std::unique_ptr<FlailerBitmapChunk>> bitmaps;
    FlailerStripPool stripPool;
    EditorMarkerResources ownedModelRegistry{};
    EditorMarkerResources* modelRegistry{};
    std::vector<std::unique_ptr<TrackRouteEditorResources>> modelAssets;
    const EditorMarkerTexture* textures{};
    std::size_t textureCount{};
    mutable bool missingFontReported{};
};

namespace {
std::vector<std::vector<std::uint8_t>> readFrontendBundles(GameResources& resources)
{
    std::vector<std::vector<std::uint8_t>> bundles;
    bundles.push_back(resources.Load("GLOBAL/GLOBALB.LZC"));
    bundles.push_back(resources.Load("FRONTEND/FRONTB.LZC"));
    return bundles;
}

std::vector<std::vector<std::uint8_t>> oneFrontendBundle(std::vector<std::uint8_t> authored)
{
    std::vector<std::vector<std::uint8_t>> bundles;
    bundles.push_back(std::move(authored));return bundles;
}

float authoredFloat(const std::uint8_t* bytes)
{
    const std::uint32_t bits=ReadU32(bytes);
    float value;
    std::memcpy(&value,&bits,sizeof(value));
    Require(std::isfinite(value),"Flailer resource contains a non-finite metric");
    return value;
}

void decodeObject(FlailerObjectChunk& object,const std::uint8_t* bytes)
{
    object.key=ReadU32(bytes+8);object.parentKey=ReadU32(bytes+12);
    std::memcpy(object.name.data(),bytes+16,object.name.size());
    object.kind=bytes[40];object.visible=bytes[41];
    object.authoredFlags=bytes[42];object.updateKind=bytes[43];
    object.topLeftX=authoredFloat(bytes+48);object.topLeftY=authoredFloat(bytes+52);
    object.scaleX=authoredFloat(bytes+56);object.scaleY=authoredFloat(bytes+60);
    const std::uint32_t rotation=ReadU32(bytes+64);
    std::memcpy(&object.rotation,&rotation,sizeof(rotation));
    object.authoredMetrics={ReadU32(bytes+68),ReadU32(bytes+72)};
}

void appendObject(FlailerScreenResource& screen,FlailerObjectChunk& object)
{
    auto* previous=screen.objects.previous;
    previous->next=&object;screen.objects.previous=&object;
    object.previous=previous;object.next=&screen.objects;
}
} // namespace

FlailerResources::FlailerResources(EditorMarkerResources& modelRegistry)
    : FlailerResources(std::vector<std::vector<std::uint8_t>>{},nullptr,0,&modelRegistry)
{
}

FlailerResources::FlailerResources(GameResources& resources,
    const EditorMarkerTexture* textures,std::size_t textureCount,EditorMarkerResources* modelRegistry)
    : FlailerResources(readFrontendBundles(resources),textures,textureCount,modelRegistry)
{
}

FlailerResources::FlailerResources(std::vector<std::uint8_t> authored,
    const EditorMarkerTexture* textures,std::size_t textureCount,EditorMarkerResources* modelRegistry)
    : FlailerResources(oneFrontendBundle(std::move(authored)),textures,textureCount,modelRegistry)
{
}

FlailerResources::FlailerResources(std::vector<std::vector<std::uint8_t>> bundles,
    const EditorMarkerTexture* textures,std::size_t textureCount,EditorMarkerResources* modelRegistry)
    : storage_(std::make_unique<Storage>())
{
    Require(textures || !textureCount,"Flailer texture range has no owner");
    storage_->modelRegistry=modelRegistry ? modelRegistry : &storage_->ownedModelRegistry;
    for (const auto& bundle:bundles) {
        Require(bundle.size()<=storage_->authored.max_size()-storage_->authored.size(),
            "Frontend bundle set exceeds native capacity");
        storage_->authored.insert(storage_->authored.end(),bundle.begin(),bundle.end());
        // The resource loader aligns each file independently. A concatenated
        // GLOBALB/FRONTB byte view is valid for the unaligned Flailer chunks,
        // but would change the origin of every16-aligned eSolid/texture.
        // Decode each retained file once into the same live model catalogue.
        storage_->modelAssets.push_back(std::make_unique<TrackRouteEditorResources>(bundle,
            storage_->modelRegistry));
    }
    storage_->packageRoot.next=storage_->packageRoot.previous=&storage_->packageRoot;
    const auto& bytes=storage_->authored;
    storage_->textures=textures;storage_->textureCount=textureCount;
    const auto visit=[&](const auto& self,std::size_t start,std::size_t size,unsigned depth,
                         FlailerPackageResource* package)->void {
        Require(depth<=32,"Flailer chunk nesting exceeds 32 levels");
        for (const auto& chunk:ReadChunks(bytes,start,size)) {
            if (chunk.id==0x80010200u) {
                auto created=std::make_unique<FlailerPackageResource>();
                created->screenRoot.next=created->screenRoot.previous=&created->screenRoot;
                created->previous=storage_->packageRoot.previous;created->next=&storage_->packageRoot;
                created->previous->next=created.get();storage_->packageRoot.previous=created.get();
                auto* current=created.get();storage_->packages.push_back(std::move(created));
                self(self,chunk.payloadOffset,chunk.size,depth+1,current);
            } else if (chunk.id==0x15010u) {
                const auto offset=chunk.headerOffset;
                const auto length=chunk.size+8u;
                Require(length>=28,"Flailer font header is truncated");
                const auto* record=bytes.data()+offset;
                const auto glyphCount=ReadU32(record+12),kerningCount=ReadU32(record+16);
                const auto glyphOffset=ReadU32(record+20),kerningOffset=ReadU32(record+24);
                Require(glyphCount<=256 && kerningCount<=1048576,"Flailer font count exceeds its authored domain");
                Require(Inside(glyphOffset,static_cast<std::uint64_t>(glyphCount)*32u,length) &&
                        (!kerningCount || Inside(kerningOffset,static_cast<std::uint64_t>(kerningCount)*8u,length)),
                        "Flailer font arrays exceed their chunk");
                auto font=std::make_unique<Storage::FontStorage>();
                font->font.key=ReadU32(record+8);
                font->glyphs.resize(glyphCount);font->kernings.resize(kerningCount);
                for (std::size_t index=0;index<kerningCount;++index) {
                    const auto* pair=record+kerningOffset+index*8;
                    font->kernings[index]={pair[0],authoredFloat(pair+4)};
                }
                for (std::size_t index=0;index<glyphCount;++index) {
                    const auto* raw=record+glyphOffset+index*32;
                    auto& glyph=font->glyphs[index];glyph.character=raw[0];
                    const std::uint16_t count=ReadU16(raw+2);
                    std::memcpy(&glyph.kerningCount,&count,sizeof(count));
                    glyph.firstKerning=ReadU32(raw+28);glyph.advance=authoredFloat(raw+20);
                    glyph.atlasPosition={authoredFloat(raw+4),authoredFloat(raw+8)};
                    glyph.size={authoredFloat(raw+12),authoredFloat(raw+16)};
                    const auto height=authoredFloat(raw+16);
                    if (font->font.lineHeight<height) font->font.lineHeight=height;
                    Require(glyph.kerningCount<=0 || Inside(glyph.firstKerning,
                        static_cast<std::uint16_t>(glyph.kerningCount),kerningCount),
                        "Flailer glyph kerning range exceeds its font");
                    font->font.characters[glyph.character]=&glyph;
                }
                font->font.kernings=font->kernings.data();font->font.kerningCount=kerningCount;
                font->font.fixedAdvance=font->font.GetWidestCharacter("1234567890:- ");
                storage_->fonts.push_back(std::move(font));
            } else if (chunk.id==0x80010201u) {
                Require(package,"Flailer screen has no package owner");
                FlailerScreenResource* screen=nullptr;
                for (const auto& child:ReadChunks(bytes,chunk.payloadOffset,chunk.size)) {
                    const auto* record=bytes.data()+child.payloadOffset;
                    if (child.id==0x10203u) {
                        Require(child.size>=40 && !screen,"Flailer screen header is absent or repeated");
                        auto created=std::make_unique<FlailerScreenResource>();
                        created->key=ReadU32(record+8);
                        created->sharedScreenKey=ReadU32(record+20);
                        const auto visibility=ReadU16(record+30);
                        std::memcpy(&created->visibility,&visibility,sizeof(visibility));
                        created->objects.next=created->objects.previous=&created->objects;
                        created->next=&package->screenRoot;created->previous=package->screenRoot.previous;
                        created->previous->next=created.get();package->screenRoot.previous=created.get();
                        screen=created.get();storage_->screens.push_back(std::move(created));
                    } else if (child.id==0x10212u) {
                        Require(screen && child.size>=100,"Flailer text precedes its screen or is truncated");
                        auto text=std::make_unique<FlailerTextChunk>();decodeObject(*text,record);
                        Require(text->kind==1,"Flailer text chunk has a different object kind");
                        text->clipObjectKey=ReadU32(record+76);text->fontKey=ReadU32(record+80);
                        text->textCapacity=ReadU32(record+84);text->formatOptions=ReadU32(record+88);
                        text->color=ReadU32(record+92);
                        text->languageId=ReadU32(record+96);
                        Require(text->textCapacity && Inside(100,text->textCapacity,child.size),
                            "Flailer text buffer exceeds its chunk");
                        const auto* value=reinterpret_cast<const char*>(record+100);
                        const auto* end=static_cast<const char*>(std::memchr(value,0,text->textCapacity));
                        // Several shipped fixed-width values (for example
                        // four-byte "help") occupy the entire authored span.
                        // Give their exact counted bytes a host terminator;
                        // never read the following chunk as part of a string.
                        const auto length=end?static_cast<std::size_t>(end-value):text->textCapacity;
                        text->ownedText.resize(std::max<std::size_t>(static_cast<std::size_t>(text->textCapacity)+1u,5u),0);
                        std::memcpy(text->ownedText.data(),value,length);
                        text->text=text->ownedText.data();
                        appendObject(*screen,*text);++screen->textCount;
                        storage_->text.push_back(std::move(text));
                    } else if (child.id==0x10210u) {
                        Require(screen && child.size>=144,"Flailer bitmap precedes its screen or is truncated");
                        auto bitmap=std::make_unique<FlailerBitmapChunk>();decodeObject(*bitmap,record);
                        Require(bitmap->kind==2,"Flailer bitmap chunk has a different object kind");
                        bitmap->clipObjectKey=ReadU32(record+76);
                        bitmap->textureKey=ReadU32(record+80);
                        for (std::size_t corner=0;corner<4;++corner)
                            for (std::size_t axis=0;axis<2;++axis)
                                bitmap->textureCoordinates[corner][axis]=authoredFloat(record+112+corner*8+axis*4);
                        for (std::size_t corner=0;corner<4;++corner) bitmap->colors[corner]=ReadU32(record+88+corner*4);
                        bitmap->texture=FindTexture(bitmap->textureKey);
                        appendObject(*screen,*bitmap);++screen->bitmapCount;
                        storage_->bitmaps.push_back(std::move(bitmap));
                    }
                }
                Require(screen,"Flailer screen container has no screen header");
            } else if (chunk.IsContainer()) self(self,chunk.payloadOffset,chunk.size,depth+1,package);
        }
    };
    visit(visit,0,bytes.size(),0,nullptr);
}

FlailerResources::~FlailerResources()=default;

std::uint64_t FlailerResources::AppendBundle(const std::vector<std::uint8_t>& authored)
{
    auto& root=*storage_;
    Require(root.nextBundle!=0,"Flailer bundle identity exhausted");
    // Parse once with its original file alignment. Until this succeeds the
    // package and font lists in the live root are untouched. Its eSolid/TPK
    // owners already use the same model registry and retire on any exception.
    FlailerResources decoded(oneFrontendBundle(authored),root.textures,root.textureCount,root.modelRegistry);
    auto& added=*decoded.storage_;
    Storage::Bundle block;
    block.token=root.nextBundle;block.offset=root.authored.size();block.size=authored.size();
    const auto prepare=[](auto& destination,auto& members,const auto& incoming) {
        destination.reserve(destination.size()+incoming.size());
        members.reserve(incoming.size());
        for (const auto& value:incoming)members.push_back(value.get());
    };
    prepare(root.fonts,block.fonts,added.fonts);
    prepare(root.packages,block.packages,added.packages);
    prepare(root.screens,block.screens,added.screens);
    prepare(root.text,block.text,added.text);
    prepare(root.bitmaps,block.bitmaps,added.bitmaps);
    prepare(root.modelAssets,block.models,added.modelAssets);
    Require(authored.size()<=root.authored.max_size()-root.authored.size(),"Flailer bundle exceeds native capacity");
    root.authored.reserve(root.authored.size()+authored.size());
    root.bundles.reserve(root.bundles.size()+1);
    root.authored.insert(root.authored.end(),authored.begin(),authored.end());
    if (added.packageRoot.next!=&added.packageRoot) {
        auto* first=added.packageRoot.next;
        auto* last=added.packageRoot.previous;
        root.packageRoot.previous->next=first;first->previous=root.packageRoot.previous;
        last->next=&root.packageRoot;root.packageRoot.previous=last;
        added.packageRoot.next=added.packageRoot.previous=&added.packageRoot;
    }
    const auto transfer=[](auto& destination,auto& incoming) {
        for (auto& value:incoming)destination.push_back(std::move(value));
        incoming.clear();
    };
    transfer(root.fonts,added.fonts);transfer(root.packages,added.packages);
    transfer(root.screens,added.screens);transfer(root.text,added.text);
    transfer(root.bitmaps,added.bitmaps);transfer(root.modelAssets,added.modelAssets);
    const auto token=block.token;
    root.bundles.push_back(std::move(block));++root.nextBundle;
    return token;
}

bool FlailerResources::CanRemoveBundle(std::uint64_t token) const
{
    auto& root=*storage_;
    const auto found=std::find_if(root.bundles.begin(),root.bundles.end(),
        [token](const auto& value){return value.token==token;});
    if (found==root.bundles.end())return false;
    const auto& block=*found;
    // A hidden/rebuilt object can already have detached its retained strip.
    // Other packages also borrow this bundle's font/texture descriptors.
    // Resource retirement is a frame boundary: retain the whole canonical
    // UI owner until both queued layers and submitted frames have drained.
    if (root.stripPool.HasPendingSubmissions())return false;
    // Live eModels retain these actual solid addresses. Their owners must
    // dispose them before a global/frontend file can release its geometry.
    for (const auto* model=root.modelRegistry->first;model;model=model->next)
        for (auto* assets:block.models) {
            const auto& catalogue=assets->Native();
            for (std::size_t i=0;i<catalogue.solidCount;++i)
                if (model->solid==&catalogue.solids[i])return false;
        }
    return true;
}

bool FlailerResources::RemoveBundle(std::uint64_t token)
{
    if (!CanRemoveBundle(token))return false;
    auto& root=*storage_;
    const auto found=std::find_if(root.bundles.begin(),root.bundles.end(),
        [token](const auto& value){return value.token==token;});
    const auto& block=*found;
    // Retained frames have drained above. Cached strips, bitmap descriptors
    // and font atlases in other bundles can still borrow this file, so detach
    // those caches before its texture/model providers are unpublished.
    DeleteAllStrips();
    for (auto* screen:block.screens) {
        screen->previous->next=screen->next;screen->next->previous=screen->previous;
    }
    for (auto* package:block.packages) {
        package->previous->next=package->next;package->next->previous=package->previous;
    }
    const auto erase=[](auto& objects,const auto& members) {
        objects.erase(std::remove_if(objects.begin(),objects.end(),[&](const auto& value) {
            return std::find(members.begin(),members.end(),value.get())!=members.end();
        }),objects.end());
    };
    erase(root.text,block.text);erase(root.bitmaps,block.bitmaps);
    erase(root.screens,block.screens);erase(root.packages,block.packages);erase(root.fonts,block.fonts);
    erase(root.modelAssets,block.models);
    const auto offset=block.offset,size=block.size;
    root.authored.erase(root.authored.begin()+offset,root.authored.begin()+offset+size);
    for (auto& remaining:root.bundles)if (remaining.offset>offset)remaining.offset-=size;
    root.bundles.erase(found);
    return true;
}

const Font* FlailerResources::FindFont(std::uint32_t key) const noexcept
{
    for (const auto& font:storage_->fonts) if (font->font.key==key) return &font->font;
    storage_->missingFontReported=true;
    return nullptr;
}

bool FlailerResources::MissingFontReported() const noexcept { return storage_->missingFontReported; }

std::size_t FlailerResources::FontCount() const noexcept { return storage_->fonts.size(); }
const Font& FlailerResources::FontAt(std::size_t index) const { return storage_->fonts.at(index)->font; }
std::size_t FlailerResources::ScreenCount() const noexcept { return storage_->screens.size(); }
FlailerScreenResource& FlailerResources::ScreenAt(std::size_t index) { return *storage_->screens.at(index); }
std::size_t FlailerResources::PackageCount() const noexcept { return storage_->packages.size(); }
FlailerPackageResource& FlailerResources::PackageRoot() noexcept { return storage_->packageRoot; }
const std::vector<std::uint8_t>& FlailerResources::Authored() const noexcept { return storage_->authored; }

FlailerScreenResource* FlailerResources::FindScreen(std::uint32_t key)
{
    std::int32_t missed=0;
    auto& root=storage_->packageRoot;
    for (auto* package=root.next;package!=&root;package=package->next) {
        auto& screens=package->screenRoot;
        for (auto* screen=screens.next;screen!=&screens;screen=screen->next,++missed) {
            if (screen->key!=key) continue;
            if (missed>=2) {
                screen->previous->next=screen->next;screen->next->previous=screen->previous;
                screen->next=screens.next;screen->previous=&screens;
                screens.next->previous=screen;screens.next=screen;
                package->previous->next=package->next;package->next->previous=package->previous;
                package->next=root.next;package->previous=&root;
                root.next->previous=package;root.next=package;
            }
            return screen;
        }
    }
    return nullptr;
}

FlailerObjectChunk* FlailerResources::FindObject(std::uint32_t screenKey,std::uint32_t objectKey,std::int32_t kind)
{
    auto* screen=FindScreen(screenKey);
    if (!screen) return nullptr;
    for (auto* object=screen->objects.next;object!=&screen->objects;object=object->next)
        if (object->key==objectKey && (!kind || static_cast<std::int32_t>(object->kind)==kind)) return object;
    return nullptr;
}

void FlailerResources::SetTextColor(std::uint32_t screenKey,std::uint32_t objectKey,std::uint32_t rgb)
{
    if (auto* object=FindObject(screenKey,objectKey,1))
        static_cast<FlailerTextChunk*>(object)->SetColor(rgb);
}

void FlailerTextChunk::SetString(const char* value)
{
    Require(value && text && textCapacity && ownedText.size()>textCapacity,
        "Flailer text has no retained counted buffer");
    if (std::strcmp(value,text)==0) return;
    updateKind=5;
    // PAL uses a counted memcpy and may read allocation bytes beyond the
    // input terminator. Native C-string storage preserves visible characters,
    // including forward aliases in this buffer, and zeroes private tail
    // padding instead of reading an unknown caller allocation tail.
    bool ended=false;
    for (std::size_t index=0;index<textCapacity;++index) {
        const char character=ended ? '\0' : value[index];
        ownedText[index]=character;
        if (!character) ended=true;
    }
    ownedText[static_cast<std::size_t>(textCapacity)-1]='\0';
    languageId=0;
}

void FlailerTextChunk::SetLanguageID(std::uint32_t value)
{
    if (languageId==value) return;
    Require(!ownedText.empty(),"Flailer language text has no retained buffer");
    languageId=value;updateKind=5;ownedText[0]='\0';
    UpdateLanguageText();
}

std::int32_t FlailerResources::Printf(std::uint32_t screenKey,std::uint32_t objectKey,const char* format,...)
{
    auto* object=FindObject(screenKey,objectKey,1);
    if (!object) return 0;
    Require(format,"Flailer formatted text has no format owner");
    char buffer[128];
    va_list arguments;
    va_start(arguments,format);
    const auto length=std::vsnprintf(buffer,sizeof(buffer),format,arguments);
    va_end(arguments);
    Require(length>=0,"Flailer text formatting failed");
    if(static_cast<std::size_t>(length)<sizeof(buffer)) {
        static_cast<FlailerTextChunk*>(object)->SetString(buffer);
    } else {
        // Keep the hot short-text path unchanged. Localized output is not
        // limited by this scratch array; the actual UI field owns its size.
        std::string text;
        va_start(arguments,format);
        try {text=FormatTextV(format,arguments);}
        catch(...) {va_end(arguments);throw;}
        va_end(arguments);
        static_cast<FlailerTextChunk*>(object)->SetString(text.c_str());
    }
    return length;
}

std::int32_t FlailerResources::SetLocalizedText(std::uint32_t screenKey,std::uint32_t objectKey,const char* value)
{
    auto* object=FindObject(screenKey,objectKey,1);
    if(!object)return 0;
    Require(value,"Localized frontend field has no completed text");
    auto& field=*static_cast<FlailerTextChunk*>(object);
    const auto length=std::strlen(value);
    Require(length<static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()),
        "Localized frontend field exceeds its native counted extent");
    if(length>=field.textCapacity) {
        // Own aliases before vector growth can retire their backing storage.
        const std::string retained(value);
        field.ownedText.resize(length+2u,0);
        field.text=field.ownedText.data();
        field.textCapacity=static_cast<std::uint32_t>(length+1u);
        field.SetString(retained.c_str());
    } else field.SetString(value);
    return static_cast<std::int32_t>(length);
}

void FlailerResources::SetObjectVisibility(std::uint32_t screenKey,std::uint32_t objectKey,std::int32_t value)
{
    auto* object=FindObject(screenKey,objectKey);
    if (!object) return;
    object->visible=static_cast<std::uint8_t>(value);
    if (!value && object->strip) object->strip->pool->Discard(*object);
}

void FlailerResources::SetTopLeftPosition(std::uint32_t screenKey,std::uint32_t objectKey,
    std::int32_t x,std::int32_t y)
{
    if (auto* object=FindObject(screenKey,objectKey)) {
        object->SetTopLeftX(static_cast<float>(x));
        object->SetTopLeftY(static_cast<float>(y));
    }
}

void FlailerResources::SetTopLeftX(std::uint32_t screenKey,std::uint32_t objectKey,std::int32_t x)
{
    if (auto* object=FindObject(screenKey,objectKey)) object->SetTopLeftX(static_cast<float>(x));
}

void FlailerResources::GetTopLeft(const FlailerObjectChunk& object,float& x,float& y,
    const LanguageManager* language) const
{
    if (object.kind==1) {
        const auto& text=static_cast<const FlailerTextChunk&>(object);
        const float width=ScaledTextWidth(text,language);
        if (text.formatOptions&4u) {
            x=object.topLeftX-width*0.5f;
            y=object.topLeftY;
            return;
        }
        x=(text.formatOptions&1u) ? object.topLeftX-width : object.topLeftX;
    } else x=object.topLeftX;
    y=object.topLeftY;
}

bool FlailerResources::GetBoundingBox(std::uint32_t screenKey,std::uint32_t objectKey,
    std::int32_t& left,std::int32_t& top,std::int32_t& right,std::int32_t& bottom,
    const LanguageManager* language)
{
    auto* object=FindObject(screenKey,objectKey);
    if (!object) return false;
    std::int32_t width=0,height=0;
    if (object->kind==1) {
        const auto& text=static_cast<const FlailerTextChunk&>(*object);
        width=convertFloatToInt32(ScaledTextWidth(text,language));
        height=convertFloatToInt32(ScaledTextHeight(text));
    } else if (object->kind==2) {
        auto& image=static_cast<FlailerBitmapChunk&>(*object);
        width=convertFloatToInt32(ScaledBitmapWidth(image));
        height=convertFloatToInt32(ScaledBitmapHeight(image));
    }
    float x=0.0f,y=0.0f;
    if (auto* positioned=FindObject(screenKey,objectKey)) GetTopLeft(*positioned,x,y,language);
    const auto originX=convertFloatToInt32(x),originY=convertFloatToInt32(y);
    const auto add=[](std::int32_t first,std::int32_t second) {
        const auto bits=static_cast<std::uint32_t>(first)+static_cast<std::uint32_t>(second);
        std::int32_t result;std::memcpy(&result,&bits,sizeof(result));return result;
    };
    // Keep the original four stores, including when output references alias.
    left=originX;
    right=add(originX,width);
    top=originY;
    bottom=add(originY,height);
    return true;
}

void FlailerResources::SetGroupVisibility(std::uint32_t screenKey,std::uint32_t groupKey,std::int32_t value)
{
    // The first lookup's result is deliberately discarded in the original.
    // Keep its resource-cache access ahead of the ordered traversal.
    (void)FindObjectByGroup(screenKey,groupKey,0);
    FlailerObjectChunk* previous=nullptr;
    while ((previous=FindObjectByGroup(screenKey,groupKey,0,previous))!=nullptr)
        previous->visible=static_cast<std::uint8_t>(value);
}

void FlailerObjectChunk::SetAlpha(std::int32_t alpha)
{
    if (kind==1)static_cast<FlailerTextChunk*>(this)->SetAlpha(alpha);
    else if (kind==2)static_cast<FlailerBitmapChunk*>(this)->SetAlpha(alpha,-1);
}

void FlailerResources::SetGroupAlpha(std::uint32_t screenKey,std::uint32_t groupKey,std::int32_t alpha)
{
    auto* object=FindObjectByGroup(screenKey,groupKey,0);
    while (object) {
        object->SetAlpha(alpha);
        object=FindObjectByGroup(screenKey,groupKey,0,object);
    }
}

void FlailerResources::MoveGroupTo(std::uint32_t screenKey,std::uint32_t groupKey,std::uint32_t anchorKey,float x,float y)
{
    auto* anchor=FindObject(screenKey,anchorKey);
    if (!anchor) return;
    const float offsetX=x-anchor->topLeftX;
    const float offsetY=y-anchor->topLeftY;
    auto* screen=FindScreen(screenKey);
    if (!screen) return;
    for (auto* object=screen->objects.previous;object!=&screen->objects;object=object->previous) {
        if (object->parentKey!=groupKey) continue;
        object->SetTopLeftX(object->topLeftX+offsetX);
        object->SetTopLeftY(object->topLeftY+offsetY);
    }
}

FlailerObjectChunk* FlailerResources::FindObjectByGroup(std::uint32_t screenKey,std::uint32_t groupKey,
    std::int32_t kind,FlailerObjectChunk* previous)
{
    auto* screen=FindScreen(screenKey);
    if (!screen) return nullptr;
    auto* first=previous ? previous->next : screen->objects.next;
    for (auto* object=first;object!=&screen->objects;object=object->next)
        if (object->parentKey==groupKey && (!kind || static_cast<std::int32_t>(object->kind)==kind)) return object;
    return nullptr;
}

float FlailerResources::ScaledTextHeight(const FlailerTextChunk& text) const
{
    const auto* font=FindFont(text.fontKey);
    if (!font) return 0.0f;
    return text.scaleY*static_cast<float>(convertFloatToInt32(font->lineHeight));
}

namespace {
float unscaledFlailerTextWidth(const FlailerResources& resources,const FlailerTextChunk& text,
    const LanguageManager* language)
{
    const auto* font=resources.FindFont(text.fontKey);
    if (!font) return 0.0f;
    const char* value=text.text;
    Require(value,"Flailer text width has no string owner");
    if (language && language->textEnabled && text.languageId) {
        const auto* translated=language->GetString(static_cast<std::int32_t>(text.languageId));
        // PAL's GetLanguageString supplies an empty global string on a miss.
        value=translated ? translated : "";
    }
    float width=0.0f;
    for (const auto* cursor=reinterpret_cast<const unsigned char*>(value);*cursor;++cursor)
        if (const auto* glyph=font->characters[*cursor]) width+=glyph->advance;
    return width;
}
}

float FlailerResources::ScaledTextWidth(const FlailerTextChunk& text,const LanguageManager* language) const
{
    return text.scaleX*unscaledFlailerTextWidth(*this,text,language);
}

void FlailerResources::ScaleTextAboutCenter(FlailerTextChunk& text,float scale,const LanguageManager* language) const
{
    text.updateKind=3;
    // NIS/HUD envelopes can begin at scale zero on PC. Use the unscaled
    // authored font extent at that endpoint instead of producing 0/0 and
    // permanently poisoning the retained position with NaN. Keep the
    // released arithmetic unchanged for every nonzero source scale.
    const auto* font=(text.scaleX==0.0f || text.scaleY==0.0f)?FindFont(text.fontKey):nullptr;
    const float halfWidth=text.scaleX!=0.0f ? ScaledTextWidth(text,language)/(2.0f*text.scaleX):
        unscaledFlailerTextWidth(*this,text,language)*0.5f;
    const float halfHeight=text.scaleY!=0.0f ? ScaledTextHeight(text)/(2.0f*text.scaleY):
        (font?static_cast<float>(convertFloatToInt32(font->lineHeight))*0.5f:0.0f);
    if (text.formatOptions&2u) {
        text.topLeftX+=halfWidth*(text.scaleX-scale);
        text.topLeftY+=halfHeight*(text.scaleY-scale);
    } else if (text.formatOptions&4u) text.topLeftY+=halfHeight*(text.scaleY-scale);
    else if (text.formatOptions&1u) {
        text.topLeftX-=halfWidth*(text.scaleX-scale);
        text.topLeftY+=halfHeight*(text.scaleX-scale);
    }
    text.scaleX=scale;text.scaleY=scale;
}

// PAL141588: raw signed texture dimensions keep the center fixed even when
// the old scale is zero. Dividing ScaledBitmapWidth by scale is not equivalent.
void FlailerResources::ScaleBitmapAboutCenter(FlailerBitmapChunk& bitmap,float scale) const
{
    bitmap.updateKind=3;
    if (!bitmap.texture) {
        bitmap.texture=FindTexture(bitmap.textureKey);
        bitmap.textureStorage=textureMetadataOwner(bitmap.texture);
    }
    Require(bitmap.texture && bitmap.texture->authoredRecord && bitmap.texture->authoredRecordSize>=40,
        "Flailer bitmap has no retained texture dimensions");
    const auto rawWidth=ReadU16(bitmap.texture->authoredRecord+36);
    const auto rawHeight=ReadU16(bitmap.texture->authoredRecord+38);
    std::int16_t width,height;
    std::memcpy(&width,&rawWidth,sizeof(width));
    std::memcpy(&height,&rawHeight,sizeof(height));
    const float halfWidth=static_cast<float>(width)*0.5f;
    const float halfHeight=static_cast<float>(height)*0.5f;
    bitmap.topLeftX+=halfWidth*(bitmap.scaleX-scale);
    bitmap.topLeftY+=halfHeight*(bitmap.scaleY-scale);
    bitmap.scaleX=scale;bitmap.scaleY=scale;
}

// PAL140820: other authored object kinds deliberately have no scale branch.
void FlailerObjectChunk::ScaleAboutCenter(FlailerResources& resources,float scale,
    const LanguageManager* language)
{
    if (kind==1) resources.ScaleTextAboutCenter(static_cast<FlailerTextChunk&>(*this),scale,language);
    else if (kind==2) resources.ScaleBitmapAboutCenter(static_cast<FlailerBitmapChunk&>(*this),scale);
}

// PAL1445B0: the normal ordered object lookup retains its missing-object case.
void FlailerResources::ScaleObjectAboutCenter(std::uint32_t screen,std::uint32_t object,
    float scale,const LanguageManager* language)
{
    if (auto* found=FindObject(screen,object)) found->ScaleAboutCenter(*this,scale,language);
}

void FlailerTextChunk::CopyAttributesFrom(const FlailerTextChunk& source)
{
    // The source memcpy copies its semantic attribute region, including four
    // inline string bytes, then restores the destination's geometry-cache
    // pointer. Neither a console pointer nor a native owning vector is copied.
    Require(ownedText.size()>=4 && source.ownedText.size()>=4,
        "Flailer attribute copy has no four-byte inline string prefix");
    std::array<char,4> prefix{};std::memcpy(prefix.data(),source.ownedText.data(),4);
    // Authored echo objects can have a smaller original text capacity than
    // the selected label (for example 5 versus18). The source only copies
    // four inline bytes here; capacity equality is not its precondition.
    // Grow native backing storage, retaining its existing terminator/tail,
    // before assigning that copied logical capacity.
    if (ownedText.size()<=source.textCapacity)
        ownedText.resize(static_cast<std::size_t>(source.textCapacity)+1u,0);
    text=ownedText.data();
    kind=source.kind;visible=source.visible;authoredFlags=source.authoredFlags;
    topLeftX=source.topLeftX;topLeftY=source.topLeftY;scaleX=source.scaleX;scaleY=source.scaleY;
    rotation=source.rotation;authoredMetrics=source.authoredMetrics;clipObjectKey=source.clipObjectKey;
    fontKey=source.fontKey;textCapacity=source.textCapacity;formatOptions=source.formatOptions;
    color=source.color;languageId=source.languageId;
    std::memcpy(ownedText.data(),prefix.data(),4);updateKind=8;
}

const EditorMarkerTexture* FlailerResources::FindTexture(std::uint32_t key) const
{
    for (std::size_t index=0;index<storage_->textureCount;++index)
        if (storage_->textures[index].key==key) return &storage_->textures[index];
    auto* root=storage_->modelRegistry;
    if (root->catalogueRoot) root=root->catalogueRoot;
    for (auto* catalogue=root;catalogue;catalogue=catalogue->nextCatalogue)
        for (std::size_t index=0;index<catalogue->textureCount;++index)
            if (catalogue->textures[index].key==key) return &catalogue->textures[index];
    return nullptr;
}

EditorMarkerResources& FlailerResources::Models() noexcept
{
    return *storage_->modelRegistry;
}

// Complete PAL1414C8/141528. The original missing-texture branch returns
// without changing scale; dimensions are signed authored16 values.
void FlailerBitmapChunk::SetWidth(FlailerResources& resources,float width)
{
    if(!texture) {texture=resources.FindTexture(textureKey);textureStorage=textureMetadataOwner(texture);}
    if(!texture)return;
    Require(texture->authoredRecord && texture->authoredRecordSize>=40,
        "Flailer bitmap width has no retained texture dimensions");
    const auto raw=ReadU16(texture->authoredRecord+36);
    std::int16_t dimension;std::memcpy(&dimension,&raw,sizeof(dimension));
    SetScaleX(width/static_cast<float>(dimension));
}
void FlailerBitmapChunk::SetHeight(FlailerResources& resources,float height)
{
    if(!texture) {texture=resources.FindTexture(textureKey);textureStorage=textureMetadataOwner(texture);}
    if(!texture)return;
    Require(texture->authoredRecord && texture->authoredRecordSize>=40,
        "Flailer bitmap height has no retained texture dimensions");
    const auto raw=ReadU16(texture->authoredRecord+38);
    std::int16_t dimension;std::memcpy(&dimension,&raw,sizeof(dimension));
    SetScaleY(height/static_cast<float>(dimension));
}

float FlailerResources::ScaledBitmapWidth(FlailerBitmapChunk& bitmap) const
{
    if (!bitmap.texture) {
        bitmap.texture=FindTexture(bitmap.textureKey);
        bitmap.textureStorage=textureMetadataOwner(bitmap.texture);
    }
    Require(bitmap.texture && bitmap.texture->authoredRecord && bitmap.texture->authoredRecordSize>=40,
        "Flailer bitmap has no retained texture dimensions");
    const auto raw=ReadU16(bitmap.texture->authoredRecord+36);
    std::int16_t width;std::memcpy(&width,&raw,sizeof(width));
    return static_cast<float>(width)*bitmap.scaleX;
}

float FlailerResources::ScaledBitmapHeight(FlailerBitmapChunk& bitmap) const
{
    if (!bitmap.texture) {
        bitmap.texture=FindTexture(bitmap.textureKey);
        bitmap.textureStorage=textureMetadataOwner(bitmap.texture);
    }
    Require(bitmap.texture && bitmap.texture->authoredRecord && bitmap.texture->authoredRecordSize>=40,
        "Flailer bitmap has no retained texture dimensions");
    const auto raw=ReadU16(bitmap.texture->authoredRecord+38);
    std::int16_t height;std::memcpy(&height,&raw,sizeof(height));
    return static_cast<float>(height)*bitmap.scaleY;
}

void FlailerResources::SetLanguageID(std::uint32_t screenKey,std::uint32_t objectKey,std::uint32_t value)
{
    if (auto* object=FindObject(screenKey,objectKey,1)) static_cast<FlailerTextChunk*>(object)->SetLanguageID(value);
}

std::int32_t FlailerResources::RenderTextIntoGroup(std::uint32_t screenKey,std::uint32_t groupKey,const char* message)
{
    std::array<FlailerTextChunk*,33> lines{};
    std::size_t count=0;
    FlailerObjectChunk* previous=nullptr;
    while (count<32) {
        previous=FindObjectByGroup(screenKey,groupKey,1,previous);
        if (!previous) break;
        lines[count++]=static_cast<FlailerTextChunk*>(previous);
    }
    if (!count) return 0;
    Require(message,"Flailer message group has no string owner");
    const char* const end=message+std::strlen(message);
    const char* cursor=message;
    const char* start=message;
    const char* lastBoundary=message;
    std::size_t line=0,nextLine=1;
    std::int32_t characters=0;
    bool hasBoundary=false;
    const auto writeLine=[](FlailerTextChunk& text,const char* first,const char* last) {
        Require(first<=last,"Flailer line has an invalid text span");
        const auto size=static_cast<std::size_t>(last-first);
        text.visible=1;
        if(size<256) {
            char buffer[256]{};
            std::memcpy(buffer,first,size);text.SetString(buffer);
        } else {
            const std::string line(first,size);text.SetString(line.c_str());
        }
    };
    while (cursor<end && *cursor) {
        const auto character=*cursor;
        if (character==' ' || character=='\t' || character=='@') {
            lastBoundary=cursor;hasBoundary=true;
        }
        const auto capacity=static_cast<std::int32_t>(lines[line]->textCapacity);
        if (hasBoundary && (characters>=capacity-2 || character=='@')) {
            writeLine(*lines[line],start,lastBoundary);
            characters=0;line=nextLine;
            cursor=lastBoundary+1;start=cursor;
            if (!lines[line]) return 0;
            hasBoundary=false;nextLine=line+1;
        } else ++characters;
        // Preserve PAL's post-break scan step: the first byte remains in the
        // next copied line while its boundary scan starts one byte later.
        ++cursor;
    }
    // An authored terminal '@' can advance the scan beyond the terminator.
    // Its visible final line is empty; keep the host span inside the string.
    writeLine(*lines[line],start,cursor>end ? end : cursor);
    auto* heightLine=lines[nextLine] ? lines[nextLine] : lines[nextLine-1];
    const auto bottom=convertFloatToInt32(heightLine->topLeftY+ScaledTextHeight(*heightLine));
    for (std::size_t unused=nextLine;unused<count;++unused) lines[unused]->visible=0;
    return bottom;
}

void FlailerScreenResource::NotifyVisibilityChange(std::int32_t value)
{
    if (value==1) {
        for (auto* object=objects.next;object!=&objects;object=object->next)
            if (object->strip) object->strip->pool->Discard(*object);
    }
    const auto bits=static_cast<std::uint16_t>(value);
    std::memcpy(&visibility,&bits,sizeof(visibility));
}

void FlailerRenderManager::SetScreenVisibility(FlailerScreenResource* screen,std::int32_t visibility)
{
    if (screen->visibility==visibility) return;
    for (auto& visible:visibleScreens) {
        if (visible==screen) { visible=nullptr;break; }
    }
    if (!visibility) {
        for (auto& visible:visibleScreens) {
            if (!visible) { visible=screen;break; }
        }
    }
    screen->NotifyVisibilityChange(visibility);
}

void FlailerRenderManager::SetScreenVisibility(FlailerResources& resources,std::uint32_t key,std::int32_t visibility)
{
    if (auto* screen=resources.FindScreen(key)) SetScreenVisibility(screen,visibility);
}

FlailerStrip* FlailerStripPool::Create(FlailerObjectChunk& object,std::vector<FlailerPoly> polygons)
{
    Discard(object);
    auto strip=std::make_unique<FlailerStrip>();
    strip->pool=this;strip->polygons=std::move(polygons);
    // The released Player fills only this authored race-over text group
    // (PAL181558 -> RenderTextIntoGroup). Keep all ordinary HUD/menu and
    // immediate polygons on their established canvas and edge anchors.
    if (object.kind==1 && object.parentKey==0xC707E093u)
        strip->uiPlacement=EcstasyUiPlacement::CenteredCanvas;
    // Publish only after the pool accepted ownership. A failed vector growth
    // must not leave object.strip pointing into the destroyed local owner.
    auto* result=strip.get();strips_.push_back(std::move(strip));
    object.strip=result;return result;
}

void FlailerStripPool::Retain(FlailerStrip& strip)
{
    Require(strip.pool==this && !strip.deleteRequested && strip.retentionCount!=UINT32_MAX,
        "Flailer strip cannot be retained");
    ++strip.retentionCount;
}

bool FlailerStripPool::HasPendingSubmissions() const noexcept
{
    for (const auto& root:strips_)
        for (const auto* strip=root.get();strip;strip=strip->nextTexture.get())
            if (strip->retentionCount!=0) return true;
    return false;
}

void FlailerStripPool::Complete(FlailerStrip& strip)
{
    Require(strip.pool==this && strip.retentionCount,"Flailer strip completion has no retained owner");
    --strip.retentionCount;
    if (!strip.retentionCount && strip.deleteRequested) Release(&strip);
}

void FlailerStripPool::Discard(FlailerObjectChunk& object)
{
    auto* strip=object.strip;
    if (!strip) return;
    Require(strip->pool==this,"Flailer strip belongs to another cache owner");
    if (strip->retentionCount) strip->deleteRequested=true;
    else Release(strip);
    object.strip=nullptr;
}

void FlailerStripPool::Release(FlailerStrip* strip)
{
    const auto found=std::find_if(strips_.begin(),strips_.end(),
        [strip](const auto& owned){return owned.get()==strip;});
    Require(found!=strips_.end(),"Flailer strip cache owner is absent");
    strips_.erase(found);
}

// PAL1416C0..141734 walks every screen even when it currently owns no
// strip: metadata lookup may already have cached its bitmap descriptor.
void FlailerResources::DeleteAllStrips()
{
    // Font atlases are a native lookup cache too. Retaining metadata prevents
    // UAF but must not keep a retired pack selected after resource replacement.
    for (const auto& storage:storage_->fonts) {
        auto& font=storage->font;
        font.texture=nullptr;font.textureStorage.reset();
        font.textureWidth=font.textureHeight=0.0f;
    }
    auto& packages=PackageRoot();
    for (auto* package=packages.next;package!=&packages;package=package->next) {
        auto& screens=package->screenRoot;
        for (auto* screen=screens.next;screen!=&screens;screen=screen->next)
            screen->DeleteAllFlailerStrips();
    }
}

FlailerStripPool& FlailerResources::StripPool() noexcept
{
    return storage_->stripPool;
}

const EditorMarkerTexture* Font::GetTextureInfo(FlailerResources& resources) const
{
    if (texture) return texture;
    auto* registry=resources.Models().resourceRegistry;
    texture=registry ? registry->ResolveTexture(key,false) : resources.FindTexture(key);
    if (!texture) return nullptr;
    Require(texture->authoredRecord && texture->authoredRecordSize>=40,
        "Font atlas has no retained texture dimensions");
    textureStorage=textureMetadataOwner(texture);
    const auto dimension=[](const std::uint8_t* bytes) {
        const auto raw=ReadU16(bytes);std::int16_t signedValue;
        std::memcpy(&signedValue,&raw,2);
        return static_cast<float>(next_power_of_2(static_cast<std::uint32_t>(signedValue)));
    };
    textureWidth=dimension(texture->authoredRecord+36);
    textureHeight=dimension(texture->authoredRecord+38);
    return texture;
}

bool FlailerObjectChunk::DoWeNeedToBuildFlailerStrip(float alphaScale)
{
    if (!strip) return true;
    if (!updateKind && alphaScale==strip->alphaScale) return false;
    DeleteFlailerStrip();
    return true;
}

void FlailerObjectChunk::CreateFlailerStrip(FlailerStripPool& pool,
    const EditorMarkerTexture* texture,float alphaScale)
{
    auto* created=pool.Create(*this,{});
    created->texture=texture;created->alphaScale=alphaScale;
    updateKind=0;
}

void FlailerObjectChunk::DeleteFlailerStrip()
{
    if (strip) strip->pool->Discard(*this);
}

FlailerPoly& FlailerStrip::AddPoly(const EditorMarkerTexture* requestedTexture)
{
    auto* batch=this;
    while (batch->texture!=requestedTexture) {
        if (!batch->nextTexture) {
            batch->nextTexture=std::make_unique<FlailerStrip>();
            batch->nextTexture->pool=pool;
            batch->nextTexture->texture=requestedTexture;
            batch->nextTexture->uiPlacement=uiPlacement;
        }
        batch=batch->nextTexture.get();
    }
    auto& polygon=batch->polygons.emplace_back();
    // PAL10F968 supplies a64x64 square as well as W=0,Q=1. The partial
    // tachometer fan intentionally retains its third default vertex.
    constexpr std::array<std::array<float,2>,4> corners{{{0,0},{1,0},{1,1},{0,1}}};
    for (std::size_t index=0;index<polygon.vertices.size();++index) {
        auto& vertex=polygon.vertices[index];
        vertex.position[0]=64.0f*corners[index][0];
        vertex.position[1]=64.0f*corners[index][1];
        vertex.textureCoordinate[0]=corners[index][0];
        vertex.textureCoordinate[1]=corners[index][1];
        vertex.textureCoordinate[2]=1.0f;
        vertex.color=0x80808080u;
    }
    polygon.rasterState={0x01000100u,0x01000080u,0,0};
    return polygon;
}

FlailerStrip::FlailerStrip()
{
    Require(flailerRuntimeStatistics.liveStrips!=INT32_MAX,"Flailer strip statistics overflow");
    ++flailerRuntimeStatistics.liveStrips;
    flailerRuntimeStatistics.maximumStrips=std::max(flailerRuntimeStatistics.maximumStrips,
        flailerRuntimeStatistics.liveStrips);
}

FlailerStrip::~FlailerStrip()
{
    --flailerRuntimeStatistics.liveStrips;
}

void FlailerStrip::RemovePoly(FlailerPoly& polygon)
{
    for (auto* batch=this;batch;batch=batch->nextTexture.get())
        for (auto it=batch->polygons.begin();it!=batch->polygons.end();++it)
            if (&*it==&polygon) {batch->polygons.erase(it);return;}
    Require(false,"Flailer polygon has no owning texture batch");
}

void FlailerTextChunk::RenderCharacter(const tTTGlyph& glyph,const Font& font,
    const float position[3],const float minimum[2],const float maximum[2],float alphaScale)
{
    Require(strip && font.texture && font.textureWidth!=0.0f && font.textureHeight!=0.0f,
        "Text glyph has no actual strip or atlas binding");
    float left=glyph.atlasPosition[0]/font.textureWidth;
    float top=glyph.atlasPosition[1]/font.textureHeight;
    float right=left+glyph.size[0]/font.textureWidth;
    float bottom=top+glyph.size[1]/font.textureHeight;
    const float halfU=0.5f/font.textureWidth,halfV=0.5f/font.textureHeight;
    const float oneU=1.0f/font.textureWidth,oneV=1.0f/font.textureHeight;
    float width=scaleX*glyph.size[0];
    const float height=scaleY*glyph.size[1];
    auto& polygon=strip->AddPoly(font.texture);
    if(glyph.size[0]>0 && glyph.size[1]>0)
        polygon.glyphSamplingBounds={left+halfU,top+halfV,right-halfU,bottom-halfV};
    left+=halfU;right-=halfU;bottom-=halfV;top+=halfV;
    right+=oneU;bottom+=oneV;width*=scaleX;
    const std::array<std::array<float,2>,4> uv{{{left,top},{right,top},{right,bottom},{left,bottom}}};
    const float rightX=position[0]+width,bottomY=position[1]+height;
    const std::array<std::array<float,3>,4> xyz{{
        {position[0],position[1],position[2]},{rightX,position[1],position[2]},
        {rightX,bottomY,position[2]},{position[0],bottomY,position[2]}}};
    for (std::size_t i=0;i<4;++i) {
        auto& vertex=polygon.vertices[i];
        std::copy(xyz[i].begin(),xyz[i].end(),vertex.position.begin());
        vertex.textureCoordinate[0]=uv[i][0];vertex.textureCoordinate[1]=uv[i][1];
        const auto alpha=static_cast<std::uint8_t>(convertFloatToInt32(
            static_cast<float>(color>>24)*alphaScale));
        vertex.color=(color&0x00ffffffu)|(static_cast<std::uint32_t>(alpha)<<24);
    }
    const auto& state=flailerTextRasterState.rasterBytes;
    polygon.rasterState={static_cast<std::uint32_t>(state[1])|
        (static_cast<std::uint32_t>(state[2])<<8)|(static_cast<std::uint32_t>(state[3])<<16)|
        (static_cast<std::uint32_t>(state[4])<<24),static_cast<std::uint32_t>(state[5])|
        (static_cast<std::uint32_t>(state[0])<<8)|0x01000000u,1,0};
    if (MaybeClipPoly(&polygon,minimum,maximum)) strip->RemovePoly(polygon);
}

float FlailerTextChunk::RenderHelpCharacter(FlailerResources& resources,float x,float y,
    std::uint8_t character,float alphaScale)
{
    Require(IsHelpCharacter(character),"Help glyph is outside the authored mapping");
    // BUTTONSOUT is the actual font key consumed by141E20, not the text font.
    const auto* font=resources.FindFont(hashString("BUTTONSOUT"));
    if (!font || !font->GetTextureInfo(resources)) return 0.0f;
    const auto mapped=flailerTextRasterState.helpGlyphs[character-0x80u];
    const auto* glyph=font->characters[mapped];
    if (!glyph) return 0.0f;
    const float position[3]{x,y,static_cast<float>(static_cast<std::int8_t>(authoredFlags))};
    RenderCharacter(*glyph,*font,position,flailerTextRasterState.minimum.data(),
        flailerTextRasterState.maximum.data(),alphaScale);
    return glyph->advance;
}

void FlailerRenderManager::AddFlailerStrip(FlailerObjectChunk& object)
{
    auto* strip=object.strip;
    if (!strip) return;
    const auto depth=static_cast<std::int8_t>(object.authoredFlags);
    Require(depth>=0 && static_cast<std::size_t>(depth)<layers.size(),
        "Flailer strip depth is outside the original29 layers");
    auto& destination=layers[static_cast<std::size_t>(depth)];
    std::vector<FlailerStrip*>* source=nullptr;
    std::size_t sourceIndex=0;
    for (auto& layer:layers) {
        const auto it=std::find(layer.begin(),layer.end(),strip);
        if (it!=layer.end()) {source=&layer;sourceIndex=static_cast<std::size_t>(it-layer.begin());break;}
    }
    // Grow the destination before changing either the old queue or its
    // retention; preserve vector's geometric growth on ordinary submissions.
    destination.push_back(strip);
    if (source)source->erase(source->begin()+sourceIndex);
    else {
        try {strip->pool->Retain(*strip);}
        catch (...) {destination.pop_back();throw;}
    }
}

void FlailerRenderManager::SubmitPolygon(FlailerStripPool& pool,const EditorMarkerTexture* texture,
    const FlailerPoly& polygon,std::int32_t depth)
{
    Require(texture && depth>=0 && static_cast<std::size_t>(depth)<layers.size(),
        "Immediate frontend polygon has no texture or valid depth");
    FlailerObjectChunk submission;
    submission.authoredFlags=static_cast<std::uint8_t>(depth);
    submission.CreateFlailerStrip(pool,texture,1.0f);
    submission.strip->AddPoly(texture)=polygon;
    AddFlailerStrip(submission);
    submission.DeleteFlailerStrip();
}

void FlailerRenderManager::CompleteSubmissions() noexcept
{
    for (auto& layer:layers) {
        for (auto* strip:layer)strip->pool->Complete(*strip);
        layer.clear();
    }
}

void FlailerRenderManager::RenderTextIntoRectangle(FlailerResources& resources,const char* message,
    FlailerTextChunk& text,const float minimum[2],const float maximum[2],float alphaScale)
{
    if (!text.DoWeNeedToBuildFlailerStrip(alphaScale)) {AddFlailerStrip(text);return;}
    const auto* font=resources.FindFont(text.fontKey);
    Require(font,"Rectangle text has no actual font");
    const auto* atlas=font->GetTextureInfo(resources);
    if (!atlas) return;
    text.CreateFlailerStrip(resources.StripPool(),atlas,alphaScale);
    const float initialX=text.topLeftX;
    float x=initialX,y=text.topLeftY;
    const float available=maximum[0]-initialX;
    const auto delimiter=[](std::int32_t value){return value==' ' || value=='\t' || value==0;};
    const auto* first=reinterpret_cast<const unsigned char*>(message);
    auto* cursor=first;
    while (*cursor) {
        const auto character=*cursor++;
        if (IsHelpCharacter(character)) {
            x+=text.RenderHelpCharacter(resources,x,y,character,alphaScale);
            continue;
        }
        const auto* glyph=font->characters[character];
        float position[3]{};
        if (glyph) {
            float kerning=0.0f;
            if (cursor>first+1)kerning=font->GetKerning(static_cast<std::int8_t>(cursor[-2]),glyph)*text.scaleX;
            position[0]=x+kerning;position[1]=y;
            position[2]=static_cast<float>(static_cast<std::int8_t>(text.authoredFlags));
            x+=static_cast<float>(convertFloatToInt32(text.scaleX*(glyph->advance+kerning)));
        }
        bool newline=false;
        if (delimiter(static_cast<std::int8_t>(character))) {
            float width=0.0f;std::size_t count=0;
            for (auto* next=cursor;*next && !delimiter(static_cast<std::int8_t>(*next));++next) {
                Require(++count<256,"Next word exceeds the original256-byte scratch domain");
                if (const auto* nextGlyph=font->characters[*next])width+=nextGlyph->advance;
            }
            width*=text.scaleX;
            if (available<(x+width)-initialX) {
                x=initialX;y+=static_cast<float>(convertFloatToInt32(font->lineHeight));newline=true;
            }
        }
        if (character=='@' && !newline) {
            x=initialX;y+=static_cast<float>(convertFloatToInt32(font->lineHeight));newline=true;
        }
        if (!newline && glyph)text.RenderCharacter(*glyph,*font,position,minimum,maximum,alphaScale);
        else {
            // The PAL loop can walk past its terminator for a final missing
            // glyph/newline. Stop at the C-string boundary in that undefined
            // source domain; defined authored strings keep the same cursor.
            while (*cursor && delimiter(static_cast<std::int8_t>(*cursor)))++cursor;
        }
    }
    AddFlailerStrip(text);
}

void FlailerRenderManager::RenderTextIntoRectangle(FlailerResources& resources,
    std::uint32_t screenKey,std::uint32_t textKey,std::uint32_t clipKey,const char* message,float alphaScale)
{
    auto* clip=static_cast<FlailerBitmapChunk*>(resources.FindObject(screenKey,clipKey,2));
    auto* text=static_cast<FlailerTextChunk*>(resources.FindObject(screenKey,textKey,1));
    if (!clip) return;
    Require(text,"Text clip rectangle has no actual text template");
    const float minimum[2]{clip->topLeftX,clip->topLeftY};
    const float width=static_cast<float>(convertFloatToInt32(resources.ScaledBitmapWidth(*clip)));
    const float height=static_cast<float>(convertFloatToInt32(resources.ScaledBitmapHeight(*clip)));
    const float maximum[2]{clip->topLeftX+width,clip->topLeftY+height};
    text->SetString(message);
    RenderTextIntoRectangle(resources,message,*text,minimum,maximum,alphaScale);
}

// PAL1434A0..143858. The unused first source argument was a needle bitmap;
// the authored tachometer bitmap owns only a borrowed reference to this fan.
void InitializeFlailerTachometer(FlailerResources& resources,FlailerBitmapChunk& bitmap,
    std::array<FlailerTachometerPoint,9>& points)
{
    if (!bitmap.texture) bitmap.ChangeTexture(resources.FindTexture(bitmap.textureKey));
    Require(bitmap.texture && bitmap.texture->authoredRecord && bitmap.texture->authoredRecordSize>=40,
        "Tachometer has no final texture metadata");
    const auto width=static_cast<std::int16_t>(ReadU16(bitmap.texture->authoredRecord+36));
    const auto height=static_cast<std::int16_t>(ReadU16(bitmap.texture->authoredRecord+38));
    Require(width>0 && height>0,"Tachometer texture dimensions are outside their defined domain");
    const auto scaledWidth=convertFloatToInt32(static_cast<float>(width)*bitmap.scaleX);
    const auto scaledHeight=convertFloatToInt32(static_cast<float>(height)*bitmap.scaleY);
    const float left=bitmap.topLeftX,top=bitmap.topLeftY;
    const float right=left+static_cast<float>(scaledWidth),bottom=top+static_cast<float>(scaledHeight);
    const float centerX=left+static_cast<float>(scaledWidth/2);
    const float centerY=top+static_cast<float>(scaledHeight/2);
    const float u=static_cast<float>(width)/static_cast<float>(next_power_of_2(static_cast<uint32_t>(width)));
    const float v=static_cast<float>(height)/static_cast<float>(next_power_of_2(static_cast<uint32_t>(height)));
    const float z=static_cast<float>(static_cast<std::int8_t>(bitmap.authoredFlags));
    points={{{{centerX,centerY,z},{u*0.5f,v*0.5f}},{{centerX,bottom,z},{u*0.5f,v}},
        {{left,bottom,z},{0,v}},{{left,centerY,z},{0,v*0.5f}},{{left,top,z},{0,0}},
        {{centerX,top,z},{u*0.5f,0}},{{right,top,z},{u,0}},
        {{right,centerY,z},{u,v*0.5f}},{{right,bottom,z},{u,v}}}};
    bitmap.tachometerPoints=points.data();
}

// PAL143858..143D50. Retain the final lookup rows and the source partial
// fan's default vertex; a generic pie chart changes its actual geometry.
void RenderFlailerTachometer(FlailerBitmapChunk& bitmap,const FlailerTachometerPoint* points)
{
    const int32_t remaining=convertFloatToInt32(360.0f-static_cast<float>(bitmap.rotation));
    const int32_t sector=remaining/90;
    float remainder=static_cast<float>(remaining%90);
    int32_t split=2;
    if (remainder>=45.0f) {remainder-=45.0f;split=1;}
    const float fraction=1.0f-remainder*0.02222222276031971f;
    if (!points) return;
    Require(bitmap.strip && bitmap.texture && sector>=0 && sector<=4,
        "Tachometer render has no actual strip/texture or a rotation outside its source domain");
    constexpr int32_t indices[5][4]={{0,7,8,1},{0,5,6,7},{0,3,4,5},{0,1,2,3},{0,0,0,0}};
    auto copyPoint=[&](FlailerPolyVertex& vertex,int32_t index) {
        vertex.position[0]=points[index].position[0];
        vertex.position[1]=points[index].position[1];
        vertex.textureCoordinate[0]=points[index].textureCoordinate[0];
        vertex.textureCoordinate[1]=points[index].textureCoordinate[1];
    };
    auto configure=[&](FlailerPoly& polygon,bool complete) {
        const auto& state=flailerTextRasterState.rasterBytes;
        polygon.rasterState={static_cast<uint32_t>(state[1])|(static_cast<uint32_t>(state[2])<<8)|
            (static_cast<uint32_t>(state[3])<<16)|(static_cast<uint32_t>(state[4])<<24),
            static_cast<uint32_t>(state[5])|(static_cast<uint32_t>(state[0])<<8)|0x01000000u,1,0};
        for (std::size_t index=0;index<4;++index) {
            polygon.vertices[index].position[2]=static_cast<float>(static_cast<std::int8_t>(bitmap.authoredFlags));
            polygon.vertices[index].color=complete ? bitmap.colors[index] : 0x80808080u;
        }
    };
    int32_t row=1;
    for (;row<sector;++row) {
        auto& polygon=bitmap.strip->AddPoly(bitmap.texture);
        for (int32_t vertex=0;vertex<4;++vertex) copyPoint(polygon.vertices[vertex],indices[row][vertex]);
        configure(polygon,true);
    }
    auto& polygon=bitmap.strip->AddPoly(bitmap.texture);
    copyPoint(polygon.vertices[0],indices[row][0]);
    const auto& first=points[indices[row][split]];
    const auto& second=points[indices[row][split+1]];
    for (std::size_t axis=0;axis<2;++axis) {
        polygon.vertices[1].position[axis]=first.position[axis]+fraction*(second.position[axis]-first.position[axis]);
        polygon.vertices[1].textureCoordinate[axis]=first.textureCoordinate[axis]+
            fraction*(second.textureCoordinate[axis]-first.textureCoordinate[axis]);
    }
    for (int32_t vertex=split+1;vertex<4;++vertex) copyPoint(polygon.vertices[vertex],indices[row][vertex]);
    configure(polygon,false);
}


// PAL145D90: shared-screen objects precede each visible screen's own list.
void FlailerRenderManager::BeginFrame(FlailerResources& resources,const LanguageManager* language,float textureTime)
{
    for (const auto* screen:visibleScreens)if (screen) {
        if (auto* shared=resources.FindScreen(screen->sharedScreenKey))
            RenderObjectList(resources,shared->objects,language,textureTime);
        RenderObjectList(resources,const_cast<FlailerObjectChunk&>(screen->objects),language,textureTime);
    }
}

// PAL145F30: stable ascending depth sort, followed by the source reverse walk.
std::int32_t FlailerRenderManager::RenderObjectList(FlailerResources& resources,
    FlailerObjectChunk& root,const LanguageManager* language,float textureTime)
{
    bool changed;
    do {
        changed=false;
        for (auto* first=root.next;first!=&root && first->next!=&root;) {
            auto* second=first->next;
            if (SortFlailerObjectFunction(first,second))first=second;
            else {
                second->previous->next=second->next;second->next->previous=second->previous;
                second->previous=first->previous;second->next=first;
                first->previous->next=second;first->previous=second;changed=true;
            }
        }
    } while (changed);
    const auto find=[&](std::uint32_t key)->FlailerObjectChunk* {
        for (auto* object=root.next;object!=&root;object=object->next)if (object->key==key)return object;
        return nullptr;
    };
    const auto dimensions=[&](FlailerObjectChunk& object) {
        std::array<float,2> size{};
        if (object.kind==1) {
            const auto& text=static_cast<const FlailerTextChunk&>(object);
            size={resources.ScaledTextWidth(text,language),resources.ScaledTextHeight(text)};
        } else if (object.kind==2) {
            auto& bitmap=static_cast<FlailerBitmapChunk&>(object);
            size={resources.ScaledBitmapWidth(bitmap),resources.ScaledBitmapHeight(bitmap)};
        }
        return size;
    };
    std::int32_t count=0;
    for (auto* object=root.previous;object!=&root;object=object->previous) {
        if (object->visible!=1)continue;
        ++count;
        if (object->kind==1) {
            auto& text=static_cast<FlailerTextChunk&>(*object);
            auto* clip=text.clipObjectKey && (text.formatOptions&0x10u) ? find(text.clipObjectKey):nullptr;
            if (clip) {
                const auto size=dimensions(*clip);
                const float minimum[2]={clip->topLeftX+4.0f,clip->topLeftY};
                const float maximum[2]={clip->topLeftX+static_cast<float>(convertFloatToInt32(size[0])),
                    clip->topLeftY+static_cast<float>(convertFloatToInt32(size[1]))};
                RenderFlailerText(resources,text,language,minimum,maximum,1.0f);
            } else RenderFlailerText(resources,text,language,nullptr,nullptr,1.0f);
        } else if (object->kind==2) {
            auto& bitmap=static_cast<FlailerBitmapChunk&>(*object);
            if (auto* clip=bitmap.clipObjectKey ? find(bitmap.clipObjectKey):nullptr) {
                float left,top;resources.GetTopLeft(*clip,left,top,language);
                const auto size=dimensions(*clip);SetFlailerTextClipWindow(left,top,left+size[0],top+size[1]);
            }
            RenderFlailerBitmap(resources,bitmap,1.0f,textureTime);
            if (bitmap.clipObjectKey)ResetFlailerTextClipWindow();
        }
    }
    return count;
}

void FlailerRenderManager::RenderFlailerText(FlailerResources& resources,FlailerTextChunk& text,
    const LanguageManager* language,const float* minimum,const float* maximum,float alphaScale)
{
    if (text.DoWeNeedToBuildFlailerStrip(alphaScale))text.Render(resources,language,minimum,maximum,alphaScale);
    AddFlailerStrip(text);
}

namespace {
void configureFlailerPolygon(FlailerPoly& polygon,const std::array<std::uint32_t,4>& colors,
    float alphaScale,bool scaleCoordinates)
{
    const auto& state=flailerTextRasterState.rasterBytes;
    polygon.rasterState={static_cast<std::uint32_t>(state[1])|(static_cast<std::uint32_t>(state[2])<<8)|
        (static_cast<std::uint32_t>(state[3])<<16)|(static_cast<std::uint32_t>(state[4])<<24),
        static_cast<std::uint32_t>(state[5])|(static_cast<std::uint32_t>(state[0])<<8)|
        (scaleCoordinates ? 0x01000000u:0u),1u,0u};
    for (std::size_t index=0;index<4;++index) {
        const auto alpha=static_cast<std::uint8_t>(convertFloatToInt32(static_cast<float>(colors[index]>>24)*alphaScale));
        polygon.vertices[index].color=(colors[index]&0x00ffffffu)|(static_cast<std::uint32_t>(alpha)<<24);
    }
}
}

float FlailerTextChunk::GetFixedWidthStringWidth(FlailerResources& resources,const char* first,const char* end) const
{
    const auto* font=resources.FindFont(fontKey);if (!font)return 0.0f;
    float width=0.0f;
    for (auto* cursor=first;cursor<end && *cursor;++cursor)
        if (font->characters[static_cast<unsigned char>(*cursor)])width+=font->fixedAdvance;
    return width;
}

// PAL141F70; source RenderCharacter retains its own distinct width arithmetic.
void FlailerTextChunk::RenderStringFixedWidth(FlailerResources& resources,
    const char* first,const char* end,float alphaScale)
{
    const auto* font=resources.FindFont(fontKey);
    if (!font || !font->GetTextureInfo(resources))return;
    const float width=scaleX*GetFixedWidthStringWidth(resources,first,end);
    float position[3]={topLeftX,topLeftY,static_cast<float>(static_cast<std::int8_t>(authoredFlags))};
    if (formatOptions&1u)position[0]-=width;
    else if (formatOptions&4u)position[0]-=width*0.5f;
    for (auto* cursor=first;cursor<end && *cursor;++cursor) {
        const auto* glyph=font->characters[static_cast<unsigned char>(*cursor)];
        if (!glyph)continue;
        float centered[3]={position[0]+(font->fixedAdvance-glyph->advance)*0.5f,position[1],position[2]};
        RenderCharacter(*glyph,*font,centered,flailerTextRasterState.minimum.data(),
            flailerTextRasterState.maximum.data(),alphaScale);
        position[0]+=scaleX*font->fixedAdvance;
    }
}

void FlailerTextChunk::RenderDarkBackground(FlailerResources& resources,const LanguageManager* language)
{
    const auto* texture=flailerTextRasterState.darkBackground;if (!texture)return;
    const float left=topLeftX-2.0f,top=topLeftY-2.0f;
    float width=resources.ScaledTextWidth(*this,language)+4.0f;
    const float height=resources.ScaledTextHeight(*this)+4.0f;
    if (formatOptions&0x40u)
        width=GetFixedWidthStringWidth(resources,text,text+std::strlen(text))+4.0f;
    const auto depth=static_cast<std::int8_t>(authoredFlags);
    const float z=static_cast<float>(depth<28 ? depth+1:depth);
    auto& polygon=strip->AddPoly(texture);
    const float x[4]={left,left+width,left+width,left},y[4]={top,top,top+height,top+height};
    for (std::size_t index=0;index<4;++index) {
        polygon.vertices[index].position={x[index],y[index],z,0.0f};
        polygon.vertices[index].color=flailerTextRasterState.darkBackgroundColor;
    }
    polygon.rasterState[2]=1;
}

// PAL1422F0, ordinary, fixed-width, translated, wrapped and clipped text.
void FlailerTextChunk::Render(FlailerResources& resources,const LanguageManager* language,
    const float* minimum,const float* maximum,float alphaScale)
{
    const auto* font=resources.FindFont(fontKey);
    if (!font)return;
    const auto* atlas=font->GetTextureInfo(resources);if (!atlas)return;
    CreateFlailerStrip(resources.StripPool(),atlas,alphaScale);
    if (!minimum) {minimum=flailerTextRasterState.minimum.data();maximum=flailerTextRasterState.maximum.data();}
    Require(maximum && text,"Flailer text render has no string or clip bounds");
    const char* first=text;const char* end=text+std::min<std::size_t>(textCapacity,std::strlen(text));
    flailerTextRasterState.lastRenderDistance={0.0f,0.0f};
    if (language && language->textEnabled) {
        if (languageId) {
            if (const auto* translated=language->GetString(static_cast<std::int32_t>(languageId))) {
                first=translated;end=translated+std::strlen(translated);
            }
        } else if (flailerTextRasterState.highlightUntranslated)color=0xffde5454u;
    }
    if (formatOptions&8u)RenderDarkBackground(resources,language);
    if (formatOptions&0x40u) {RenderStringFixedWidth(resources,first,end,alphaScale);return;}
    float x=topLeftX,y=topLeftY;
    const float width=resources.ScaledTextWidth(*this,language);
    if (formatOptions&1u)x-=width;else if (formatOptions&4u)x-=width*0.5f;
    const float initialX=x,initialY=y;bool afterSpace=false;
    const float z=static_cast<float>(static_cast<std::int8_t>(authoredFlags));
    for (auto* cursor=first;cursor<end && *cursor;++cursor) {
        if ((formatOptions&0x20u) && afterSpace) {
            const float nextWordWidth=FlailerTextDistanceToNextChar(cursor,' ',font)*scaleX;
            if (flailerTextRasterState.wrapRight<=x+nextWordWidth) {
                x=flailerTextRasterState.wrapLeft;y+=flailerTextRasterState.wrapLineHeight;
            }
            afterSpace=false;
        }
        const auto character=static_cast<unsigned char>(*cursor);
        if (character==' ')afterSpace=true;
        if (IsHelpCharacter(character)) {x+=RenderHelpCharacter(resources,x,y,character,alphaScale);continue;}
        const auto* glyph=font->characters[character];if (!glyph)continue;
        float kerning=cursor!=first ? font->GetKerning(static_cast<std::int8_t>(cursor[-1]),glyph)*scaleX:0.0f;
        if (std::fabs(kerning)>80.0f)kerning=0.0f;
        const float authoredU=glyph->atlasPosition[0]/font->textureWidth;
        const float authoredV=glyph->atlasPosition[1]/font->textureHeight;
        const float halfU=0.5f/font->textureWidth,halfV=0.5f/font->textureHeight;
        const float left=authoredU+halfU,top=authoredV+halfV;
        const float right=((authoredU+glyph->size[0]/font->textureWidth)-halfU)+1.0f/font->textureWidth;
        const float bottom=((authoredV+glyph->size[1]/font->textureHeight)-halfV)+1.0f/font->textureHeight;
        auto& polygon=strip->AddPoly(atlas);
        if(glyph->size[0]>0 && glyph->size[1]>0)
            polygon.glyphSamplingBounds={left,top,
                authoredU+glyph->size[0]/font->textureWidth-halfU,
                authoredV+glyph->size[1]/font->textureHeight-halfV};
        const float vx[4]={x+kerning,x+scaleX*glyph->size[0]+kerning,x+scaleX*glyph->size[0]+kerning,x+kerning};
        const float vy[4]={y,y,y+scaleY*glyph->size[1],y+scaleY*glyph->size[1]};
        const float u[4]={left,right,right,left},v[4]={top,top,bottom,bottom};
        for (std::size_t index=0;index<4;++index) {
            polygon.vertices[index].position={vx[index],vy[index],z,0.0f};
            polygon.vertices[index].textureCoordinate={u[index],v[index],1.0f};
        }
        if ((formatOptions&0x10u) && MaybeClipPoly(&polygon,minimum,maximum))strip->RemovePoly(polygon);
        else configureFlailerPolygon(polygon,{color,color,color,color},alphaScale,true);
        x+=scaleX*(glyph->advance+kerning);
    }
    flailerTextRasterState.lastRenderDistance={x-initialX,y-initialY};
}

// PAL1428F0. Authored UVs and the source signed integer pixel rounding are
// retained; rotation is the original angle-table eRotateZ operation.
void FlailerBitmapChunk::SetNativeFullscreenMovie(bool enabled)
{
    if(nativeFullscreenMovie!=enabled) {nativeFullscreenMovie=enabled;updateKind=3;}
}

void FlailerRenderManager::RenderSingleFlailerBitmap(FlailerResources& resources,
    FlailerBitmapChunk& bitmap,float alphaScale,float textureTime)
{
    if (!bitmap.DoWeNeedToBuildFlailerStrip(alphaScale)) {AddFlailerStrip(bitmap);return;}
    if (!bitmap.texture) {
        auto* registry=resources.Models().resourceRegistry;
        bitmap.ChangeTexture(registry ? registry->ResolveTexture(bitmap.textureKey,true):resources.FindTexture(bitmap.textureKey));
    }
    bitmap.CreateFlailerStrip(resources.StripPool(),bitmap.texture,alphaScale);
    if (bitmap.authoredMetrics[0]==10) {
        Require(bitmap.tachometerPoints,"Tachometer bitmap has no owning Player fan");
        RenderFlailerTachometer(bitmap,bitmap.tachometerPoints);AddFlailerStrip(bitmap);return;
    }
    Require(bitmap.texture && bitmap.texture->authoredRecord && bitmap.texture->authoredRecordSize>=0x88,
        "Bitmap texture has no actual descriptor");
    const auto* record=bitmap.texture->authoredRecord;
    float scroll[2]{};
    const auto scrollMode=static_cast<std::int8_t>(record[0x79]);
    if (scrollMode) {
        for (std::size_t axis=0;axis<2;++axis)
            scroll[axis]=TextureInfo::GetScroll(textureTime,authoredFloat(record+0x7c+axis*4),
                scrollMode,authoredFloat(record+0x84));
        bitmap.updateKind=1;
    }
    const auto rawWidth=static_cast<std::int16_t>(ReadU16(record+0x24));
    const auto rawHeight=static_cast<std::int16_t>(ReadU16(record+0x26));
    Require(rawWidth>0 && rawHeight>0,"Bitmap texture dimensions are not positive");
    const float width=static_cast<float>(rawWidth),height=static_cast<float>(rawHeight);
    const float paddedWidth=static_cast<float>(next_power_of_2(static_cast<std::uint32_t>(rawWidth)));
    const float paddedHeight=static_cast<float>(next_power_of_2(static_cast<std::uint32_t>(rawHeight)));
    const bool fullMovie=bitmap.nativeFullscreenMovie &&
        (bitmap.texture->key==0xc3f0caefu || bitmap.texture->key==0xf9117c29u);
    const auto drawWidth=fullMovie?640:convertFloatToInt32(width*bitmap.scaleX);
    const auto drawHeight=fullMovie?480:convertFloatToInt32(height*bitmap.scaleY);
    const float left=fullMovie?0.0f:bitmap.topLeftX,top=fullMovie?0.0f:bitmap.topLeftY;
    auto& polygon=bitmap.strip->AddPoly(bitmap.texture);
    for (std::size_t index=0;index<4;++index) {
        const bool right=index==1 || index==2,bottom=index>=2;
        polygon.vertices[index].textureCoordinate={scroll[0]+width*bitmap.textureCoordinates[index][0]/paddedWidth+
            (right?-0.5f:0.5f)/paddedWidth,scroll[1]+height*bitmap.textureCoordinates[index][1]/paddedHeight+
            (bottom?-0.5f:0.5f)/paddedHeight,1.0f};
        polygon.vertices[index].position={left+(right?static_cast<float>(drawWidth):0.0f),
            top+(bottom?static_cast<float>(drawHeight):0.0f),
            static_cast<float>(static_cast<std::int8_t>(bitmap.authoredFlags)),0.0f};
    }
    const bool rotated=!fullMovie && bitmap.authoredMetrics[0]==2 && bitmap.rotation!=0;
    if (rotated) {
        float rotation[16],translation[16],transform[16];eIdentity(rotation);
        const auto angle=static_cast<std::uint16_t>(convertFloatToInt32(static_cast<float>(bitmap.rotation)*65536.0f)/360);
        motionMatrixRotateYaw(rotation,rotation,angle);
        const float center[3]={bitmap.topLeftX+static_cast<float>(drawWidth)*0.5f,
            bitmap.topLeftY+static_cast<float>(drawHeight)*0.5f,0.0f};
        motionMatrixTranslation(translation,center);eMulMatrix(transform,rotation,translation);
        for (std::size_t index=0;index<4;++index) {
            const float local[3]={(index==1 || index==2?0.5f:-0.5f)*static_cast<float>(drawWidth),
                (index>=2?0.5f:-0.5f)*static_cast<float>(drawHeight),0.0f};
            float position[4];eMulVector3(position,transform,local);
            polygon.vertices[index].position[0]=position[0];polygon.vertices[index].position[1]=position[1];
        }
    }
    // Single bitmap leaves ePoly+97's constructor value1 unchanged.
    configureFlailerPolygon(polygon,bitmap.colors,alphaScale,true);
    if (!fullMovie && !rotated && MaybeClipPoly(&polygon,flailerTextRasterState.minimum.data(),flailerTextRasterState.maximum.data()))
        bitmap.strip->RemovePoly(polygon);
    if(fullMovie)bitmap.strip->uiPlacement=EcstasyUiPlacement::OriginalCanvas;
    AddFlailerStrip(bitmap);
}

// PAL143078: the two composite images reuse actual scene target textures.
void FlailerRenderManager::RenderFlailerBitmap(FlailerResources& resources,
    FlailerBitmapChunk& bitmap,float alphaScale,float textureTime)
{
    const auto* original=bitmap.texture;
    const bool movieTexture=original && (original->key==0xc3f0caefu || original->key==0xf9117c29u);
    // Retail PAL143148/14314C suppresses movie planes once Close clears
    // MoviePlayer+440. Preserve that source lifetime in native CPU RGBA,
    // even though its conversion uses the source common-zero branch.
    if (movieTexture && bitmapComposite.movie && !bitmapComposite.movie->ValidFrame())return;
    if (!bitmapComposite.enabled || !movieTexture) {
        RenderSingleFlailerBitmap(resources,bitmap,alphaScale,textureTime);return;
    }
    if (!bitmapComposite.targetsReady)return;
    for (const auto* texture:bitmapComposite.textures)if (!texture)return;
    const auto dimension=[](const EditorMarkerTexture& texture) {
        Require(texture.authoredRecord && texture.authoredRecordSize>=40,"Composite target has no descriptor");
        return static_cast<float>(next_power_of_2(static_cast<std::uint32_t>(static_cast<std::int16_t>(ReadU16(texture.authoredRecord+38)))));
    };
    const auto coordinates=bitmap.textureCoordinates;const float scaleX=bitmap.scaleX,scaleY=bitmap.scaleY;
    const auto alphaByte=flailerTextRasterState.rasterBytes[0];
    for (std::size_t pass=0;pass<5;++pass) {
        if (pass<2) {
            const float offset=0.5f/dimension(*bitmapComposite.textures[pass]);
            for (std::size_t corner=0;corner<4;++corner)
                bitmap.textureCoordinates[corner][1]=1.0f-coordinates[corner][1]+(corner<2?offset:-offset);
        }
        if (bitmapComposite.stages[pass]) {
            bitmap.SetScaleX(scaleX*(pass==0?1.0f:2.0f));bitmap.SetScaleY(scaleY*(pass==0?1.0f:2.0f));
            bitmap.ChangeTexture(bitmapComposite.textures[pass]);RenderSingleFlailerBitmap(resources,bitmap,alphaScale,textureTime);
        }
    }
    bitmap.ChangeTexture(original);bitmap.textureCoordinates=coordinates;
    bitmap.SetScaleX(scaleX);bitmap.SetScaleY(scaleY);flailerTextRasterState.rasterBytes[0]=alphaByte;
}

// PAL145950 visits the next texture batch before this batch's polygons.
void FlailerStrip::Render(const CarCinematicScreen& view,EcstasyFrame& frame,std::int32_t layer)
{
    if (nextTexture)nextTexture->Render(view,frame,layer);
    if (texture)for (const auto& polygon:polygons)
        frame.AddPolygon(polygon,*texture,view,layer,1.0f,uiPlacement);
    ++renderedFrames;
}

// PAL145E10 detaches the layer immediately. Native destruction waits for the
// device fence instead of freeing the still-borrowed GPU input in this call.
std::int32_t FlailerRenderManager::RenderLayer(std::int32_t layer,const CarCinematicScreen& view,EcstasyFrame& frame)
{
    Require(layer>=0 && static_cast<std::size_t>(layer)<layers.size(),"Flailer layer is outside the original29 lists");
    auto& pending=layers[static_cast<std::size_t>(layer)];std::int32_t count=0;
    Require(pending.size()<=static_cast<std::size_t>(INT32_MAX),"Flailer layer exceeds its draw-count domain");
    try {
        for (auto* strip:pending) {
            strip->Render(view,frame,layer);frame.RetireStripAfterCompletion(*strip);++count;
        }
    } catch (...) {
        // Only the successfully transferred prefix belongs to the frame.
        // The current strip and suffix remain queued for normal cancellation.
        pending.erase(pending.begin(),pending.begin()+count);throw;
    }
    pending.clear();return count;
}

// The complete PAL145ED0 body is JR RA,NOP. Frame resource retirement belongs
// to RenderLayer and the real GPU completion owner, not this source hook.
void FlailerRenderManager::EndFrame() {}

} // namespace hp2
