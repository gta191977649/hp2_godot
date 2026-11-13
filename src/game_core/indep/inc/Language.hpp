#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

struct WorldStreamAudio;
namespace hp2 { class GameResources; class TexturePack; class ResourceLoader; }
namespace hp2 {
struct FrontendPlayerManager;
struct ApplicationFrontendFlow;
class FlailerResources;

// Borrow one live frontend/resource/speech graph. In particular FRED's
// language cell and the speech queue are not mirrored in a locale service.
struct LanguageBindings {
    FrontendPlayerManager& frontend;
    ApplicationFrontendFlow& flow;
    TexturePack& dynamicTextures;
    ResourceLoader& resources;
    FlailerResources& flailer;
    WorldStreamAudio& speech;
    // Native completion service borrows the same ApplicationRuntime owner.
    // It services actual device requests; it must not manufacture readiness.
    void* audioServiceContext{};
    void (*serviceAudio)(void*){};
};

// One retained language image supplies both the name-hash and numeric-ID
// indices. Returned strings borrow this owner until unload/replacement.
class LanguageManager final {
public:
    std::int32_t currentLanguage{-1};
    // The live manager's text-enable cell (PAL manager+14), independent of
    // whether a retained language image happens to be loaded.
    std::int32_t textEnabled{};
    LanguageManager();
    explicit LanguageManager(std::vector<std::uint8_t> authored);
    ~LanguageManager();
    LanguageManager(const LanguageManager&)=delete;
    LanguageManager& operator=(const LanguageManager&)=delete;
    void Init(const GameResources& files);
    void Bind(const LanguageBindings& bindings);
    void Close();
    void SetLanguage(std::int32_t language);
    void HandleLanguageChange(std::int32_t current,std::int32_t previous);
    void LoadLocalizedSpeech();
    void UnloadLocalizedSpeech();
    const char* GetString(std::int32_t key,bool byName=false) const;
    const char* GetString(const char* name) const;
    // Raw fixture callers can omit the borrowed reference resource set. The
    // live file loader passes its own GameResources for verified index repair.
    bool Loader(std::vector<std::uint8_t> authored,const GameResources* normalizationFiles=nullptr);
    bool Unloader(std::uint32_t chunkId);
    std::size_t StringCount() const noexcept;
    std::size_t MaximumFileSize() const noexcept { return maximumFileBytes_; }
private:
    bool LoadChunk(std::size_t offset,std::size_t bytes,const GameResources* normalizationFiles=nullptr);
    void LoadFileChunks();
    struct Storage;
    const GameResources* files_{};
    // The maximum-sized physical file image is also the string owner.
    // Decoded indices borrow this allocation; they never copy its text.
    std::vector<std::uint8_t> fileImage_;
    std::size_t maximumFileBytes_{};
    std::size_t loadedFileBytes_{};
    std::unique_ptr<LanguageBindings> bindings_;
    std::unique_ptr<Storage> storage_;
};

// Native byte-text substitutions for the two declared frontend arguments.
// Other percent bytes remain authored text, including French event percentages
// and the incomplete German calibration title. Capacity includes the final NUL.
// These functions do not interpret arbitrary resource text as a CRT format.
std::string FormatLocalizedButtonText(const char* text,std::uint32_t buttonCharacter,std::size_t capacity);
std::string FormatLocalizedIntegerText(const char* text,std::int32_t value,std::size_t capacity);

// Explicit argument types for the remaining released localized consumers:
// event/reward integers, translated names/times, and track elevation values.
// Resource bytes are never passed to the host variadic formatter. This is a
// deliberately bounded vocabulary (%d, %s, %.f), not a PS2 printf emulator.
class LocalizedTextArgument final {
public:
    enum class Kind { Integer,Text,WholeFloat };
    LocalizedTextArgument(std::int32_t value);
    LocalizedTextArgument(const char* value);
    LocalizedTextArgument(double value);
    Kind Type() const noexcept { return kind_; }
    const std::string& Text() const noexcept { return text_; }
private:
    Kind kind_;
    std::string text_;
};
// Capacity includes NUL. An omitted placeholder (FE_ID, for example) leaves
// its argument unused. Unrecognised percent bytes remain literal resource
// text; recognised placeholders must match a supplied argument's exact type.
std::string FormatLocalizedText(const char* text,
    std::initializer_list<LocalizedTextArgument> arguments,
    std::size_t capacity=std::string::npos);

// Original PAL21E508: a successful name lookup, otherwise the input string.
// The explicit reference borrows the same manager as the original global.
const char* GetTranslatedString(const LanguageManager& language,const char* text);

} // namespace hp2
