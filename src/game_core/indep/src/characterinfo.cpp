#include "indep/inc/CarDriver.hpp"
#include "indep/inc/CarDriver.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bChunk.hpp"
#include "bware/inc/bWare.hpp"

#include <algorithm>
#include <cstring>

int32_t carDriverAppearanceLoad(CarDriverAppearanceCatalogue* catalogue,
    const CarDriverAppearanceChunk* chunk)
{
    if (chunk->kind == 0x34301u) {
        const size_t count = chunk->authoredPayloadSize / 180u;
        if (count > chunk->availableRecords || (count && !chunk->records.appearances))
            return 0;
        catalogue->appearances = chunk->records.appearances;
        catalogue->appearanceCount = count;
        return 1;
    }
    if (chunk->kind == 0x34300u) {
        const size_t count = chunk->authoredPayloadSize / 68u;
        if (count > chunk->availableRecords || (count && !chunk->records.characters))
            return 0;
        catalogue->characters = chunk->records.characters;
        catalogue->characterCount = count;
        return 1;
    }
    return 0;
}

int32_t carDriverAppearanceUnload(CarDriverAppearanceCatalogue* catalogue,uint32_t kind)
{
    if (kind == 0x34301u) {
        catalogue->appearances = nullptr;
        catalogue->appearanceCount = 0;
        return 1;
    }
    if (kind == 0x34300u) {
        catalogue->characters = nullptr;
        catalogue->characterCount = 0;
        return 1;
    }
    return 0;
}

const CarDriverAppearance* carDriverAppearanceFind(
    const CarDriverAppearanceCatalogue* catalogue,const char* name)
{
    for (size_t index = 0; index < catalogue->appearanceCount; ++index)
        if (stringCompareInsensitive(name,catalogue->appearances[index].name) == 0)
            return catalogue->appearances + index;
    return nullptr;
}

const CarDriverCharacter* carDriverCharacterFind(
    const CarDriverAppearanceCatalogue* catalogue,const char* name)
{
    for (size_t index = 0; index < catalogue->characterCount; ++index)
        if (stringCompareInsensitive(name,catalogue->characters[index].name) == 0)
            return catalogue->characters + index;
    return nullptr;
}

const CarDriverCharacter* carDriverCharacterFindKey(
    const CarDriverAppearanceCatalogue* catalogue,uint32_t key)
{
    for (size_t index = 0; index < catalogue->characterCount; ++index)
        if (hashString(catalogue->characters[index].name) == key)
            return catalogue->characters + index;
    return nullptr;
}

int32_t carDriverCharacterCount(const CarDriverAppearanceCatalogue* catalogue)
{
    return static_cast<int32_t>(catalogue->characterCount);
}

const CarDriverCharacter* carDriverCharacterAt(
    const CarDriverAppearanceCatalogue* catalogue,int32_t index)
{
    if (index < 0 || static_cast<size_t>(index) >= catalogue->characterCount)
        return nullptr;
    return catalogue->characters + index;
}

namespace hp2 {
namespace {

template<std::size_t N> void readDriverName(char (&output)[N],const std::uint8_t* bytes)
{
    Require(std::find(bytes,bytes + N,std::uint8_t(0)) != bytes + N,
        "Driver appearance name is not terminated");
    std::memcpy(output,bytes,N);
}

std::int32_t readDriverSignedWord(const std::uint8_t* bytes)
{
    const auto bits = ReadU32(bytes);
    std::int32_t value;
    std::memcpy(&value,&bits,sizeof(value));
    return value;
}

} // namespace

CarDriverAppearanceResources::CarDriverAppearanceResources(const std::vector<std::uint8_t>& bundle)
{
    std::uint32_t appearanceBytes = 0,characterBytes = 0;
    const auto visit = [&](const auto& self,std::size_t begin,std::size_t size,unsigned depth)->void {
        Require(depth <= 32,"Driver appearance resource nesting exceeds 32 levels");
        for (const auto& chunk : ReadChunks(bundle,begin,size)) {
            if (chunk.id == 0x34301u) {
                Require(appearances_.empty() && chunk.size % 180 == 0,
                    "Driver appearance class table is repeated or truncated");
                appearanceBytes = chunk.size;
                for (std::size_t offset = 0; offset < chunk.size; offset += 180) {
                    const auto* bytes = bundle.data() + chunk.payloadOffset + offset;
                    CarDriverAppearance appearance{};
                    readDriverName(appearance.name,bytes);
                    for (int index = 0; index < 3; ++index)
                        appearance.modelKeys[index] = ReadU32(bytes + 24 + index * 4);
                    for (int index = 0; index < 2; ++index) {
                        appearance.materialKeys[index] = ReadU32(bytes + 36 + index * 4);
                        appearance.variantCounts[index] = readDriverSignedWord(bytes + 44 + index * 4);
                        Require(appearance.variantCounts[index] >= 0 && appearance.variantCounts[index] <= 16,
                            "Driver appearance variant count exceeds its authored array");
                        for (int variant = 0; variant < 16; ++variant)
                            appearance.textureKeys[index][variant] = ReadU32(bytes + 52 + index * 64 + variant * 4);
                    }
                    appearances_.push_back(appearance);
                }
            } else if (chunk.id == 0x34300u) {
                Require(characters_.empty() && chunk.size % 68 == 0,
                    "Driver character table is repeated or truncated");
                characterBytes = chunk.size;
                for (std::size_t offset = 0; offset < chunk.size; offset += 68) {
                    const auto* bytes = bundle.data() + chunk.payloadOffset + offset;
                    CarDriverCharacter character{};
                    character.id = readDriverSignedWord(bytes);
                    readDriverName(character.name,bytes + 4);
                    readDriverName(character.appearanceName,bytes + 36);
                    for (int index = 0; index < 2; ++index)
                        character.variants[index] = readDriverSignedWord(bytes + 60 + index * 4);
                    characters_.push_back(character);
                }
            } else if (chunk.IsContainer()) {
                self(self,chunk.payloadOffset,chunk.size,depth + 1);
            }
        }
    };
    visit(visit,0,bundle.size(),0);

    CarDriverAppearanceChunk appearances{};
    appearances.kind = 0x34301u;
    appearances.authoredPayloadSize = appearanceBytes;
    appearances.records.appearances = appearances_.data();
    appearances.availableRecords = appearances_.size();
    Require(carDriverAppearanceLoad(&catalogue_,&appearances) != 0,"Driver class table cannot be registered");
    CarDriverAppearanceChunk characters{};
    characters.kind = 0x34300u;
    characters.authoredPayloadSize = characterBytes;
    characters.records.characters = characters_.data();
    characters.availableRecords = characters_.size();
    Require(carDriverAppearanceLoad(&catalogue_,&characters) != 0,"Driver character table cannot be registered");

    for (const auto& character : characters_) {
        const auto* appearance = carDriverAppearanceFind(&catalogue_,character.appearanceName);
        Require(appearance != nullptr,"Driver character names an absent appearance class");
        for (int index = 0; index < 2; ++index)
            Require(character.variants[index] >= 0 && character.variants[index] < 16,
                "Driver character variant is outside its authored texture array");
    }
}

} // namespace hp2
