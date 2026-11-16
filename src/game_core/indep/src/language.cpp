#include "Language.hpp"
#include "Flags.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bFile.hpp"
#include "bware/inc/bWare.hpp"
#include "Fred.hpp"
#include "CarLoader.hpp"
#include "GameFlow.hpp"
#include "FlailerDB.hpp"
#include "indep/src/speech/SEDWedge.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <limits>
#include <utility>

namespace hp2 {

namespace {
constexpr std::array<const char*,8> languageFiles{{
    "LANGUAGES\\FE_ENGLISH.BIN","LANGUAGES\\FE_FRENCH.BIN",
    "LANGUAGES\\FE_GERMAN.BIN","LANGUAGES\\FE_ITALIAN.BIN",
    "LANGUAGES\\FE_SPANISH.BIN","LANGUAGES\\FE_SWEDISH.BIN",
    "LANGUAGES\\FE_LONGEST.BIN","LANGUAGES\\FE_ID.BIN"
}};

std::string SubstituteLocalizedArgument(const char* text,char token,
    const std::string& replacement,std::size_t capacity)
{
    Require(text && capacity,"Localized text has no source or output capacity");
    std::string result;
    for (const char* cursor=text;*cursor;) {
        if (cursor[0]=='%' && cursor[1]==token) {
            Require(replacement.size()<capacity-result.size(),
                "Localized argument exceeds its frontend text buffer");
            result.append(replacement);cursor+=2;
        } else {
            Require(result.size()<capacity-1,"Localized text exceeds its frontend text buffer");
            result.push_back(*cursor++);
        }
    }
    return result;
}
}

std::string FormatLocalizedButtonText(const char* text,std::uint32_t buttonCharacter,std::size_t capacity)
{
    Require(buttonCharacter<=255,"Localized button is not a font byte");
    return SubstituteLocalizedArgument(text,'c',std::string(1,static_cast<char>(buttonCharacter)),capacity);
}

std::string FormatLocalizedIntegerText(const char* text,std::int32_t value,std::size_t capacity)
{
    return SubstituteLocalizedArgument(text,'d',std::to_string(value),capacity);
}

LocalizedTextArgument::LocalizedTextArgument(std::int32_t value)
    :kind_(Kind::Integer),text_(std::to_string(value)) {}
LocalizedTextArgument::LocalizedTextArgument(const char* value):kind_(Kind::Text)
{
    Require(value,"Localized string argument has no text owner");
    text_=value;
}
LocalizedTextArgument::LocalizedTextArgument(double value):kind_(Kind::WholeFloat)
{
    // The two PAL19FB88 elevation arguments are promoted authored floats.
    // PAL2521D4..2522AC carries a decimal digit when the remainder is >=.5,
    // including an exact tie. Preserve that rule instead of host printf's
    // ties-to-even rounding. The caller supplies finite binary32 elevations.
    Require(std::isfinite(value),"Localized elevation is not finite");
    const double rounded=value<0.0?-std::floor(-value+0.5):std::floor(value+0.5);
    // Only this compiled numeric format reaches the host CRT; resource text
    // cannot change its type, precision, or number of arguments.
    char output[384];
    const auto count=std::snprintf(output,sizeof(output),"%.0f",rounded);
    Require(count>=0 && static_cast<std::size_t>(count)<sizeof(output),
        "Localized elevation exceeds its numeric representation");
    text_.assign(output,static_cast<std::size_t>(count));
}
std::string FormatLocalizedText(const char* text,
    std::initializer_list<LocalizedTextArgument> arguments,std::size_t capacity)
{
    Require(text && capacity,"Localized text has no source or output capacity");
    std::string result;auto argument=arguments.begin();
    for(const char* cursor=text;*cursor;) {
        std::size_t tokenBytes=0;
        auto kind=LocalizedTextArgument::Kind::Text;
        if(cursor[0]=='%') {
            if(cursor[1]=='d') {kind=LocalizedTextArgument::Kind::Integer;tokenBytes=2;}
            else if(cursor[1]=='s')tokenBytes=2;
            else if(cursor[1]=='.' && cursor[2]=='f') {
                kind=LocalizedTextArgument::Kind::WholeFloat;tokenBytes=3;
            }
        }
        if(tokenBytes) {
            Require(argument!=arguments.end() && argument->Type()==kind,
                "Localized placeholder does not match its declared consumer argument");
            const auto& replacement=argument->Text();
            Require(replacement.size()<capacity-result.size(),
                "Localized argument exceeds its frontend text buffer");
            result.append(replacement);++argument;cursor+=tokenBytes;
        } else {
            Require(result.size()<capacity-1,"Localized text exceeds its frontend text buffer");
            result.push_back(*cursor++);
        }
    }
    return result;
}

struct LanguageManager::Storage {
    struct Entry { std::int32_t key{};std::uint32_t stringOffset{}; };
    const std::uint8_t* authored{};
    std::vector<Entry> names;
    std::vector<Entry> ids;
    std::size_t count{};
    std::size_t stringStart{};
    std::size_t stringBytes{};
};

LanguageManager::LanguageManager()=default;
LanguageManager::LanguageManager(std::vector<std::uint8_t> authored)
{
    Require(Loader(std::move(authored)),"Language resource has a different chunk type");
}
LanguageManager::~LanguageManager()=default;

void LanguageManager::Bind(const LanguageBindings& bindings)
{
    Require(!bindings_,"Language application graph is already bound");
    bindings_=std::make_unique<LanguageBindings>(bindings);
}

void LanguageManager::Close()
{
    // PAL21E080 frees the single maximum-sized retained language allocation;
    // unloading speech is independent of whether a language file exists.
    if (maximumFileBytes_) {
        storage_.reset();
        fileImage_.clear();fileImage_.shrink_to_fit();
        maximumFileBytes_=0;
    }
    UnloadLocalizedSpeech();
}

void LanguageManager::LoadFileChunks()
{
    std::size_t offset=0;
    while (offset<loadedFileBytes_) {
        Require(loadedFileBytes_-offset>=8,"Language file has a truncated chunk header");
        const auto* header=fileImage_.data()+offset;
        const auto kind=ReadU32(header);
        const auto bytes=static_cast<std::size_t>(ReadU32(header+4));
        Require(bytes<=loadedFileBytes_-offset-8,"Language file chunk exceeds its image");
        if (kind==0x39000u) {
            Require(LoadChunk(offset,bytes+8,files_),"Language chunk publication failed");
        } else {
            // Final FE language resources contain their language chunk and
            // optional null padding. Unknown resources need a real loader,
            // never a success fallback or another language image owner.
            Require(kind==0,"Language file contains an unimplemented resource chunk");
        }
        offset+=bytes+8;
    }
}

const char* LocalizedSpeechLanguageName(std::int32_t language) noexcept
{
    // Final PAL table2B6228, including English speech for Swedish.
    constexpr std::array names{"English","French","German","Italian","Spanish","English"};
    return static_cast<std::uint32_t>(language)<names.size() ? names[language] : nullptr;
}

bool LanguageManager::CanSelect(std::int32_t language) const
{
    const auto* speech=LocalizedSpeechLanguageName(language);
    if(!speech || !files_)return false;
    const auto text=files_->TryOpenPhysical(languageFiles[language]);
    if(!text || !text->range.Size() || text->range.Size()>maximumFileBytes_)return false;
    const auto base=std::string("Sound\\Speech\\")+speech;
    return files_->TryOpenPhysical(base+"BAN.pck").has_value() &&
        files_->TryOpenPhysical(base+"DAT.pck").has_value();
}

void LanguageManager::SetLanguage(std::int32_t language)
{
    if (language==currentLanguage)return;
    Require(language>=-1 && language<static_cast<std::int32_t>(languageFiles.size()),
        "Language index exceeds the released file table");
    Require(files_ && bindings_,"Language file/application graph is not bound");
    auto previous=currentLanguage;
    if (previous!=-1) {
        UnloadCurrentSelectedCars();
        // The actual language unloader detaches the two indices before the
        // same physical image is overwritten by the next file.
        (void)Unloader(0x39000u);
        loadedFileBytes_=0;
        previous=currentLanguage;
    }
    currentLanguage=language;
    if (currentLanguage!=-1 && maximumFileBytes_) {
        const auto file=files_->TryOpenPhysical(languageFiles[static_cast<std::size_t>(currentLanguage)]);
        if (file) {
            const auto bytes=file->range.Size();
            loadedFileBytes_=bytes<=maximumFileBytes_ ? static_cast<std::size_t>(bytes) : 0;
            if (loadedFileBytes_)file->range.Read(0,fileImage_.data(),loadedFileBytes_);
            LoadFileChunks();
        }
    }
    HandleLanguageChange(currentLanguage,previous);
}

void LanguageManager::HandleLanguageChange(std::int32_t current,std::int32_t previous)
{
    Require(bindings_!=nullptr,"Language application graph is not bound");
    bindings_->frontend.currentLanguage=current;
    if (textEnabled && current!=previous) {
        UnloadLocalizedSpeech();
        LoadLocalizedSpeech();
    }
    GameFlowNotifyLanguageChange(bindings_->flow,bindings_->dynamicTextures,
        bindings_->resources,current,previous);
    FlailerUpdateLanguageText(bindings_->flailer);
}

void LanguageManager::LoadLocalizedSpeech()
{
    if (currentLanguage==-1 || textEnabled)return;
    Require(bindings_!=nullptr,"Language speech owner is not bound");
    textEnabled=1;
    const auto result=soundLoadLocalizedSpeech(bindings_->speech,currentLanguage);
    Require(result==WORLD_STREAM_AUDIO_OK,
        "Localized speech initialization failed (language="+std::to_string(currentLanguage)+
        ", result="+std::to_string(static_cast<int>(result))+
        "; 1=owner binding, 2=capacity, 3=EVT/BAN/DAT resource, 4=busy)");
}

void LanguageManager::UnloadLocalizedSpeech()
{
    if (!textEnabled)return;
    Require(bindings_!=nullptr,"Language speech owner is not bound");
    const auto oldEnabled=textEnabled;
    textEnabled=0;
    try {
        // PAL's release is synchronous. The native STOP retains this same
        // BAN/DAT owner until the real consumer acknowledges it. Continue
        // servicing that consumer at this source boundary, including queue
        // pressure before STOP can be submitted; never turn BUSY into OK.
        for (;;) {
            const auto result=soundUnloadLocalizedSpeech(bindings_->speech);
            if (result==WORLD_STREAM_AUDIO_OK)return;
            Require(result==WORLD_STREAM_AUDIO_BUSY || result==WORLD_STREAM_AUDIO_CAPACITY,
                "Localized speech cleanup failed on its actual owner");
            Require(bindings_->audioServiceContext && bindings_->serviceAudio,
                "Localized speech retirement has no actual audio completion service");
            bindings_->serviceAudio(bindings_->audioServiceContext);
        }
    } catch (...) {
        // A failed physical service leaves the still-retained unload
        // request retryable. Terminal Application shutdown joins the device
        // before cancelling its transport and releasing these inputs.
        textEnabled=oldEnabled;
        throw;
    }
}

void LanguageManager::Init(const GameResources& files)
{
    Require(!files_ && fileImage_.empty(),"Language manager is already initialized");
    std::size_t maximum=0;
    for (const auto* filename:languageFiles) {
        const auto resource=files.TryOpenPhysical(filename);
        if (!resource)continue;
        const auto bytes=resource->range.Size();
        Require(bytes<=static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()),
            "Language file exceeds the released signed file-size range");
        maximum=std::max(maximum,static_cast<std::size_t>(bytes));
    }
    fileImage_.resize(maximum);
    maximumFileBytes_=maximum;
    files_=&files;
}

bool LanguageManager::Loader(std::vector<std::uint8_t> authored,const GameResources* normalizationFiles)
{
    Require(authored.size()>=8,"Language chunk header is truncated");
    if (ReadU32(authored.data())!=0x39000u) return false;
    Require(!files_,"A bound language file cannot be replaced by a separate chunk owner");
    storage_.reset();
    fileImage_=std::move(authored);
    return LoadChunk(0,fileImage_.size(),normalizationFiles);
}

bool LanguageManager::LoadChunk(std::size_t offset,std::size_t bytes,const GameResources* normalizationFiles)
{
    Require(Inside(offset,bytes,fileImage_.size()) && bytes>=8,"Language chunk exceeds its file image");
    const auto* authored=fileImage_.data()+offset;
    if (ReadU32(authored)!=0x39000u)return false;
    Require(bytes>=108 && ReadU32(authored+4)==bytes-8,
        "Language chunk length is invalid");
    const auto nameCount=ReadU32(authored+84);
    const auto idCount=ReadU32(authored+88);
    const auto stringBytes=ReadU32(authored+92);
    const std::uint64_t idsStart=108u+static_cast<std::uint64_t>(nameCount)*8u;
    const std::uint64_t stringStart=idsStart+static_cast<std::uint64_t>(idCount)*8u;
    Require(nameCount==idCount && idCount<=1048576 && stringBytes>=8 &&
        Inside(stringStart,stringBytes,bytes),"Language arrays exceed their retained chunk");
    auto decoded=std::make_unique<Storage>();
    decoded->authored=authored;decoded->count=idCount;
    decoded->stringStart=static_cast<std::size_t>(stringStart);decoded->stringBytes=stringBytes;
    const auto readIndex=[&](std::vector<Storage::Entry>& entries,std::size_t start) {
        // PAL's search uses count as its inclusive upper bound. Preserve the
        // adjacent entry it can inspect; never read beyond retained storage.
        entries.resize(static_cast<std::size_t>(idCount)+1u);
        for (std::size_t index=0;index<entries.size();++index) {
            const auto* entry=decoded->authored+start+index*8u;
            const auto raw=ReadU32(entry);
            std::memcpy(&entries[index].key,&raw,sizeof(raw));
            entries[index].stringOffset=ReadU32(entry+4);
            if (index<idCount) {
                const auto offset=entries[index].stringOffset;
                Require(offset<stringBytes && std::memchr(decoded->authored+decoded->stringStart+offset,0,
                    stringBytes-offset),"Language string exceeds its authored pool");
                if (index) Require(entries[index-1].key<=entries[index].key,"Language index is not signed-key sorted");
            }
        }
    };
    readIndex(decoded->names,108);readIndex(decoded->ids,static_cast<std::size_t>(idsStart));
    if (normalizationFiles) {
        // Native resource adaptation, not an additional PAL lookup rule.
        // A translated file can retain stale name offsets after rebuilding
        // only its numeric-ID index. Infer no offsets from text or hashes:
        // every repaired ordinal must have the same hash and numeric ID in
        // every usable, independently loaded reference language image.
        std::vector<std::uint32_t> offsets;
        offsets.reserve(idCount);
        for (std::size_t i=0;i<idCount;++i)offsets.push_back(decoded->ids[i].stringOffset);
        std::sort(offsets.begin(),offsets.end());
        bool inconsistent=false;
        for (std::size_t i=0;i<nameCount;++i)
            inconsistent=inconsistent || !std::binary_search(offsets.begin(),offsets.end(),decoded->names[i].stringOffset);
        if (inconsistent) {
            for (std::size_t i=0;i<idCount;++i) {
                const auto& entry=decoded->ids[i];
                Require((!i || decoded->ids[i-1].key<entry.key) &&
                    (!entry.stringOffset || authored[stringStart+entry.stringOffset-1]==0),
                    "Language name-index repair requires unique numeric IDs and complete numeric strings");
            }
            std::vector<std::int32_t> mapping;
            bool verifiedReference=false;
            for (const auto* filename:languageFiles) {
                if (!normalizationFiles->TryOpenPhysical(filename))continue;
                const auto reference=normalizationFiles->Load(filename);
                if (reference.size()<108 || ReadU32(reference.data())!=0x39000u ||
                    ReadU32(reference.data()+4)!=reference.size()-8 ||
                    ReadU32(reference.data()+84)!=nameCount || ReadU32(reference.data()+88)!=idCount)continue;
                const auto referenceStringBytes=ReadU32(reference.data()+92);
                if (referenceStringBytes<8 || !Inside(stringStart,referenceStringBytes,reference.size()))continue;
                const auto* raw=reference.data();
                std::vector<std::pair<std::uint32_t,std::int32_t>> byOffset;
                byOffset.reserve(idCount);
                bool valid=true;
                std::int32_t previousId{};
                for (std::size_t i=0;i<idCount;++i) {
                    const auto rawId=ReadU32(raw+idsStart+i*8u);
                    std::int32_t id{};std::memcpy(&id,&rawId,sizeof(id));
                    const auto stringOffset=ReadU32(raw+idsStart+i*8u+4u);
                    if ((i && id<=previousId) || stringOffset>=referenceStringBytes ||
                        (stringOffset && raw[stringStart+stringOffset-1]!=0) ||
                        !std::memchr(raw+stringStart+stringOffset,0,referenceStringBytes-stringOffset)) {valid=false;break;}
                    byOffset.emplace_back(stringOffset,id);previousId=id;
                }
                if (!valid)continue;
                std::sort(byOffset.begin(),byOffset.end());
                for (std::size_t i=1;i<byOffset.size();++i)
                    if (byOffset[i-1].first==byOffset[i].first) {valid=false;break;}
                if (!valid)continue;
                std::vector<std::int32_t> candidate;
                candidate.reserve(nameCount);
                for (std::size_t i=0;i<nameCount;++i) {
                    const auto hash=ReadU32(raw+108u+i*8u);
                    std::int32_t signedHash{};std::memcpy(&signedHash,&hash,sizeof(signedHash));
                    const auto stringOffset=ReadU32(raw+108u+i*8u+4u);
                    if (signedHash!=decoded->names[i].key) {valid=false;break;}
                    const auto found=std::lower_bound(byOffset.begin(),byOffset.end(),stringOffset,
                        [](const auto& entry,std::uint32_t value){return entry.first<value;});
                    if (found==byOffset.end() || found->first!=stringOffset) {valid=false;break;}
                    const auto numeric=std::lower_bound(decoded->ids.begin(),decoded->ids.begin()+idCount,found->second,
                        [](const auto& entry,std::int32_t value){return entry.key<value;});
                    if (numeric==decoded->ids.begin()+idCount || numeric->key!=found->second) {valid=false;break;}
                    candidate.push_back(found->second);
                }
                if (!valid)continue;
                Require(mapping.empty() || mapping==candidate,
                    "Language name-index repair reference mappings disagree");
                if (mapping.empty())mapping=std::move(candidate);
                verifiedReference=true;
            }
            Require(verifiedReference && mapping.size()==nameCount,
                "Language name index is inconsistent and has no verified reference mapping");
            for (std::size_t i=0;i<nameCount;++i) {
                const auto numeric=std::lower_bound(decoded->ids.begin(),decoded->ids.begin()+idCount,mapping[i],
                    [](const auto& entry,std::int32_t value){return entry.key<value;});
                const auto repaired=numeric->stringOffset;
                decoded->names[i].stringOffset=repaired;
            }


        }
    }
    storage_=std::move(decoded);
    return true;
}

bool LanguageManager::Unloader(std::uint32_t chunkId)
{
    if (chunkId!=0x39000u) return false;
    storage_.reset();
    return true;
}

const char* LanguageManager::GetString(std::int32_t key,bool byName) const
{
    if (!storage_) return nullptr;
    const auto& index=byName ? storage_->names : storage_->ids;
    std::int32_t lower=0;
    auto upper=static_cast<std::int32_t>(storage_->count);
    auto selected=upper/2;
    if (index[static_cast<std::size_t>(selected)].key!=key && upper>=0) {
        do {
            const auto found=index[static_cast<std::size_t>(selected)].key;
            if (found<key) lower=selected+1;
            else if (key<found) upper=selected-1;
            selected=(lower+upper)/2;
            if (index[static_cast<std::size_t>(selected)].key==key) break;
        } while (lower<=upper);
    }
    const auto& found=index[static_cast<std::size_t>(selected)];
    if (found.key!=key) return nullptr;
    Require(found.stringOffset<storage_->stringBytes &&
        std::memchr(storage_->authored+storage_->stringStart+found.stringOffset,0,
            storage_->stringBytes-found.stringOffset),"Language lookup points outside its authored string pool");
    return reinterpret_cast<const char*>(storage_->authored+storage_->stringStart+found.stringOffset);
}

const char* LanguageManager::GetString(const char* name) const
{
    const auto raw=hashString(name);
    std::int32_t key{};
    std::memcpy(&key,&raw,sizeof(key));
    return GetString(key,true);
}

// Complete final wrapper21E508..21E55C. The raw name lookup above retains
// its own miss=nullptr contract; the wrapper preserves untranslated text.
const char* GetTranslatedString(const LanguageManager& language,const char* text)
{
    const auto* translated=language.GetString(text);
    return translated?translated:text;
}

std::size_t LanguageManager::StringCount() const noexcept
{
    return storage_ ? storage_->count : 0;
}

} // namespace hp2
