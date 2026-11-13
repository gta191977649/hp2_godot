#ifndef NFSHP2_CAR_LOADER_HPP
#define NFSHP2_CAR_LOADER_HPP

#include "CarInfo.hpp"
#include "bware/inc/bChunk.hpp"
#include <cstdint>
#include <exception>
#include <memory>
#include <string>

namespace hp2 {
class TexturePack;
extern "C++" {class LanguageManager;struct FrontendPlayerManager;}

// One native arrangement of the actual two retained lists. These types do
// not overlay the console pool's48-byte slots or assert beta class layout.
class LoadedCar final {
public:
    explicit LoadedCar(const char* name);
    std::uint8_t references{};
    bool loading{},loaded{};
    ResourceFile* file{};
    std::string filename;
};

class LoadedSkin final {
public:
    explicit LoadedSkin(const CarAppearanceSelection& ride);
    CarAppearanceSelection appearance{-1,0,0,0,0};
    std::uint8_t references{};
    bool loading{},loaded{};
};

class CarLoader final {
public:
    CarLoader();
    ~CarLoader();
    CarLoader(const CarLoader&)=delete;
    CarLoader& operator=(const CarLoader&)=delete;
    void Bind(ResourceLoader& files,const CarInfoResource& cars,TexturePack& textures,
        const LanguageManager& language,const CarTextureNamingPolicy& naming,
        PositionMarkerRegistry& markers,TerrainTopology* topology);
    void ReleaseResources();
    void BeginShutdown(ResourceLoader& files) noexcept;
    std::exception_ptr Shutdown(ResourceLoader& files) noexcept;
    bool ShuttingDown() const noexcept;
    // Stop terminal continuations before draining genuine pending car reads.
    void DetachLoadingCallbacks() noexcept;
    LoadedCar* FindLoadedCar(const char* filename) const;
    LoadedSkin* FindLoadedSkin(std::int32_t model,std::int32_t skin) const;
    std::int32_t Load(const CarAppearanceSelection& ride);
    bool IsLoaded(const CarAppearanceSelection& ride,bool includePending=false) const;
    void Unload(const CarAppearanceSelection& ride);
    void UnloadEverything();
    void RemoveUnusedCarsAndSkins();
    // This is the original void completion signature. Current concrete
    // preview callers passNULL; a game producer must supply its real method.
    void BeginLoading(void (*completion)()=nullptr);
    // Context for the original GameFlowLoadingTrackCommonPart2 callback.
    // It is retained on this same asynchronous CarLoader, not a global copy.
    void BeginLoading(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings);
    void LoadingDoneCallback();
    std::int32_t GetUsedTextures(LoadedSkin& skin,std::uint32_t* keys,std::int32_t count,
        std::int32_t capacity,std::int32_t frontend) const;
    bool singleSkinBatch{true};
    std::int32_t frontendTextures{};
private:
    friend class ResourceLoader;
    void ReleaseResourceLoader(ResourceLoader& files);
    struct Storage;
    std::unique_ptr<Storage> storage_;
};

// PAL313A38: one process CarLoader, initially empty, with no fabricated
// resource bindings. Preview selection and race consumers borrow this owner.
extern CarLoader carLoader;
extern "C++" {
void UnloadCurrentSelectedCars();
void CloseSelectCarScreen();
void PreloadCurrentSelectedCars(FrontendPlayerManager& frontend);
}
} // namespace hp2
#endif
