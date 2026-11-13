#include "CarLoader.hpp"
#include "Texture.hpp"
#include "Language.hpp"
#include "Ecstasy.hpp"
#include "GameFlow.hpp"
#include "bware/inc/bWare.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <list>
#include <unordered_map>

namespace hp2 {

LoadedCar::LoadedCar(const char* name):filename(name ? name : "")
{Require(name && filename.size()<32,"LoadedCar filename exceeds its actual pool field");}
LoadedSkin::LoadedSkin(const CarAppearanceSelection& ride) {appearance=ride;}

// One physical texture acquisition per key per LoadTexturesFromFile call.
// A shared car texture can be requested by several skins in one batch, or
// by the same key in independent batches. Track both dimensions; a single
// deduplicated unload across all skins does not balance those references.
struct CarSkinTextureUse {
    LoadedSkin* skin{};
    std::vector<std::uint32_t> keys;
};
struct CarTextureBatch {
    std::size_t id{};
    std::vector<CarSkinTextureUse> uses;
    std::unordered_map<std::uint32_t,std::size_t> remainingUsers;
};

struct CarLoader::Storage {
    std::list<std::unique_ptr<LoadedCar>> cars;
    std::list<std::unique_ptr<LoadedSkin>> skins;
    std::list<CarTextureBatch> textureBatches;
    std::size_t nextTextureBatchId{};
    ResourceLoader* files{};
    const CarInfoResource* info{};
    TexturePack* textures{};
    const LanguageManager* language{};
    const CarTextureNamingPolicy* naming{};
    ResourceFileBindings bindings{};
    bool loading{};
    bool shuttingDown{};
    std::exception_ptr shutdownFailure;
    void (*completion)(){};
    ApplicationFrontendFlow* flow{};
    const GameFlowBindings* gameFlow{};
};

CarLoader::CarLoader():storage_(std::make_unique<Storage>()) {}
CarLoader::~CarLoader()=default;
CarLoader carLoader;

void CarLoader::Bind(ResourceLoader& files,const CarInfoResource& cars,TexturePack& textures,
    const LanguageManager& language,const CarTextureNamingPolicy& naming,
    PositionMarkerRegistry& markers,TerrainTopology* topology)
{
    auto& s=*storage_;
    Require(!s.files && s.cars.empty() && s.skins.empty() && !s.loading,
        "CarLoader must release its previous resource bindings first");
    Require(cars.CarsLoaded() && cars.SkinsLoaded() && &files.Registry().Models()==&textures.CatalogueRoot(),
        "CarLoader requires the actual shared car and model/texture owners");
    Require(!files.ShuttingDown(),"CarLoader cannot bind a retiring resource service");
    s.shuttingDown=false;s.shutdownFailure=nullptr;
    s.files=&files;s.info=&cars;s.textures=&textures;s.language=&language;s.naming=&naming;
    s.bindings.kind=ResourceFileKind::Car;s.bindings.positionMarkers=&markers;
    s.bindings.terrain=topology;s.bindings.carLoader=this;
}

void CarLoader::ReleaseResources()
{
    auto& s=*storage_;
    Require(!s.loading && s.cars.empty() && s.skins.empty() && s.textureBatches.empty(),
        "CarLoader still owns active car, skin or texture-batch resources");
    s.files=nullptr;s.info=nullptr;s.textures=nullptr;s.language=nullptr;s.naming=nullptr;s.bindings={};
    s.completion=nullptr;s.flow=nullptr;s.gameFlow=nullptr;
}

void CarLoader::DetachLoadingCallbacks() noexcept
{
    auto& s=*storage_;s.completion=nullptr;s.flow=nullptr;s.gameFlow=nullptr;
}

void CarLoader::ReleaseResourceLoader(ResourceLoader& files)
{
    if (storage_->files!=&files)return;
    DetachLoadingCallbacks();
    if (this==&carLoader)UnloadCurrentSelectedCars();
    UnloadEverything();ReleaseResources();
}

void CarLoader::BeginShutdown(ResourceLoader& files) noexcept
{
    auto& s=*storage_;if(s.files!=&files)return;
    s.shuttingDown=true;DetachLoadingCallbacks();
    if(s.textures)s.textures->BeginShutdown();
}

bool CarLoader::ShuttingDown() const noexcept {return storage_->shuttingDown;}

std::exception_ptr CarLoader::Shutdown(ResourceLoader& files) noexcept
{
    auto& s=*storage_;
    if(!s.files)return s.shutdownFailure;
    if(s.files!=&files)return {};
    BeginShutdown(files);
    const auto remember=[&]() noexcept {
        if(!s.shutdownFailure)s.shutdownFailure=std::current_exception();
    };
    if(this==&carLoader)try {UnloadCurrentSelectedCars();}catch(...) {remember();}
    // Pending skins already own their batch references. No loading marker
    // needs to run before retiring those references, only the actual write.
    if(s.textures)try {s.textures->WaitForReads();}catch(...) {remember();}
    for(auto& car:s.cars)if(car->file) {
        try {files.CancelResourceFile(car->file);}catch(...) {remember();}
        // ResourceLoader still owns any file whose provider release failed;
        // its following terminal pass will finish that retirement.
        car->file=nullptr;car->loading=false;car->loaded=false;
    }
    bool textureFailure=false;
    for(auto& batch:s.textureBatches)for(const auto& acquisition:batch.remainingUsers) {
        try {
            Require(s.textures && acquisition.second>0,
                "Terminal skin batch lost its physical texture acquisition");
            const auto key=acquisition.first;s.textures->UnloadTexturesFromFile(&key,1);
        } catch(...) {remember();textureFailure=true;}
    }
    // All application borrowers have stopped. A bad acquisition must retire
    // its real pack, not leak regions or conceal a corrupt reference count.
    if(textureFailure && s.textures) {
        const auto error=s.textures->Shutdown();
        if(!s.shutdownFailure)s.shutdownFailure=error;
    }
    s.textureBatches.clear();s.skins.clear();s.cars.clear();s.loading=false;
    s.files=nullptr;s.info=nullptr;s.textures=nullptr;s.language=nullptr;s.naming=nullptr;s.bindings={};
    return s.shutdownFailure;
}

LoadedCar* CarLoader::FindLoadedCar(const char* filename) const
{
    Require(filename!=nullptr,"LoadedCar lookup has no filename");
    for (const auto& car:storage_->cars)if (car->filename==filename)return car.get();
    return nullptr;
}
LoadedSkin* CarLoader::FindLoadedSkin(std::int32_t model,std::int32_t skin) const
{
    for (const auto& entry:storage_->skins)
        if (entry->appearance.modelCode==model && entry->appearance.skinNumber==skin)return entry.get();
    return nullptr;
}

std::int32_t CarLoader::Load(const CarAppearanceSelection& ride)
{
    auto& s=*storage_;Require(!s.shuttingDown && s.info,"CarLoader is unbound or shutting down");
    const auto& info=s.info->Car(static_cast<std::size_t>(ride.modelCode));
    auto* primary=FindLoadedCar(info.geometryCompressedPath.c_str());
    auto* dashboard=ride.dashboardTextures ? FindLoadedCar(info.dashboardGeometryPath.c_str()) : nullptr;
    auto* skin=FindLoadedSkin(ride.modelCode,ride.skinNumber);
    const bool sameGeometry=ride.dashboardTextures &&
        info.geometryCompressedPath==info.dashboardGeometryPath;
    // Validate and allocate the complete request before modifying any live
    // reference. A missing dashboard or skin must not leave primary retained
    // if a later allocation/path validation fails.
    const auto primaryIncrements=1u+(sameGeometry ? 1u : 0u);
    Require(!primary || primary->references<=255u-primaryIncrements,
        "CarLoader primary geometry reference counter would overflow");
    Require(!dashboard || sameGeometry || dashboard->references<255u,
        "CarLoader dashboard geometry reference counter would overflow");
    Require(!skin || skin->references<255u,
        "CarLoader skin reference counter would overflow");
    std::unique_ptr<LoadedCar> newPrimary;
    std::unique_ptr<LoadedCar> newDashboard;
    std::unique_ptr<LoadedSkin> newSkin;
    if(!primary)newPrimary=std::make_unique<LoadedCar>(info.geometryCompressedPath.c_str());
    if(ride.dashboardTextures && !dashboard && !sameGeometry)
        newDashboard=std::make_unique<LoadedCar>(info.dashboardGeometryPath.c_str());
    if(!skin)newSkin=std::make_unique<LoadedSkin>(ride);
    bool committedPrimary=false,committedDashboard=false;
    try {
        if(newPrimary) {
            s.cars.push_back(std::move(newPrimary));
            primary=s.cars.back().get();committedPrimary=true;
        }
        if(ride.dashboardTextures && !dashboard) {
            if(sameGeometry)dashboard=primary;
            else {
                s.cars.push_back(std::move(newDashboard));
                dashboard=s.cars.back().get();committedDashboard=true;
            }
        }
        if(newSkin) {
            s.skins.push_back(std::move(newSkin));
            skin=s.skins.back().get();
        }
    } catch(...) {
        if(committedDashboard)s.cars.pop_back();
        if(committedPrimary)s.cars.pop_back();
        throw;
    }
    ++primary->references;
    if(ride.dashboardTextures)++dashboard->references;
    if(ride.dashboardTextures)skin->appearance.dashboardTextures=1;
    ++skin->references;
    return static_cast<std::int32_t>(static_cast<std::int8_t>(primary->references))-1;
}

bool CarLoader::IsLoaded(const CarAppearanceSelection& ride,bool includePending) const
{
    auto& s=*storage_;Require(s.info,"CarLoader has no actual CarInfo owner");
    const auto& info=s.info->Car(static_cast<std::size_t>(ride.modelCode));
    const auto* car=FindLoadedCar(info.geometryCompressedPath.c_str());
    // A cancelled request can remain in the physical file/texture queue after
    // its final frontend borrower retires. Pending does not mean owned: a
    // stale Select Car preload must re-acquire its reference rather than
    // treating that zero-reference request as a live selected car.
    if (!car || car->references<=0 || (!includePending && !car->loaded))return false;
    if (ride.dashboardTextures) {
        car=FindLoadedCar(info.dashboardGeometryPath.c_str());
        if (!car || car->references<=0 || (!includePending && !car->loaded))return false;
    }
    const auto* skin=FindLoadedSkin(ride.modelCode,ride.skinNumber);
    return skin && skin->references>0 && (includePending || skin->loaded);
}

void CarLoader::Unload(const CarAppearanceSelection& ride)
{
    if (!IsLoaded(ride,true))return;
    auto& s=*storage_;const auto& info=s.info->Car(static_cast<std::size_t>(ride.modelCode));
    --FindLoadedCar(info.geometryCompressedPath.c_str())->references;
    if (ride.dashboardTextures)--FindLoadedCar(info.dashboardGeometryPath.c_str())->references;
    --FindLoadedSkin(ride.modelCode,ride.skinNumber)->references;
    // Destructors still balance their real references after admissions stop.
    // Fallible physical retirement belongs to the reporting terminal pass.
    if(!s.shuttingDown)RemoveUnusedCarsAndSkins();
}

std::int32_t CarLoader::GetUsedTextures(LoadedSkin& skin,std::uint32_t* keys,std::int32_t count,
    std::int32_t capacity,std::int32_t frontend) const
{
    auto& s=*storage_;
    Require(s.info && s.language && s.naming && keys && count>=0 && capacity>=count,
        "Skin texture enumeration requires its actual shared owners and output extent");
    auto& ride=skin.appearance;
    if (ride.skinNumber==-1) {
        try {
            for (std::int32_t index=0;index<s.info->AvailableSkinCount(static_cast<std::size_t>(ride.modelCode));++index) {
                ride.skinNumber=static_cast<std::int8_t>(s.info->AvailableSkinNumber(
                    static_cast<std::size_t>(ride.modelCode),index,s.language->currentLanguage!=0));
                count=GetUsedTextures(skin,keys,count,capacity,frontend);
            }
        } catch(...) {
            ride.skinNumber=-1;
            throw;
        }
        ride.skinNumber=-1;return count;
    }
    const auto info=BuildCarTextureInfo(*s.info,ride,frontend,*s.naming);
    for (std::size_t index=0;index<info.count;++index) {
        const auto key=info.usedKeys[index];
        if (std::find(keys,keys+count,key)!=keys+count)continue;
        if (count>=capacity)return count;
        keys[count++]=key;
    }
    return count;
}

void CarLoader::BeginLoading(void (*completion)())
{
    auto& s=*storage_;Require(!s.shuttingDown,"CarLoader is shutting down");
    if (completion)s.completion=completion;
    if (s.loading) {
        // A native read-start exception may leave a real texture marker but
        // no queued read to advance it. An explicit retry resumes that
        // service; only its genuine completion may finish the skin batch.
        if(s.textures && s.textures->HasCarCompletion(this) && !s.textures->HasPendingReads())
            s.textures->ServiceLoadingTextures();
        return;
    }
    Require(s.files && s.textures && s.info,"CarLoader loading has no actual shared resource bindings");
    std::array<std::uint32_t,512> keys{};std::int32_t count=0;bool found=false;
    std::vector<LoadedSkin*> requesting;
    std::size_t batchId=0;
    try {
        for (auto& skin:s.skins) {
            if (!skin->loading && !skin->loaded) {
                skin->loading=true;found=true;
                requesting.push_back(skin.get());
                count=GetUsedTextures(*skin,keys.data(),count,static_cast<std::int32_t>(keys.size()),frontendTextures);
            }
            if (found && singleSkinBatch)break;
        }
        if (found) {
            CarTextureBatch batch;
            batch.id=++s.nextTextureBatchId;
            batchId=batch.id;
            batch.uses.reserve(requesting.size());
            for (auto* skin:requesting) {
                std::array<std::uint32_t,512> ownKeys{};
                const auto ownCount=GetUsedTextures(*skin,ownKeys.data(),0,
                    static_cast<std::int32_t>(ownKeys.size()),frontendTextures);
                CarSkinTextureUse use;use.skin=skin;
                use.keys.reserve(static_cast<std::size_t>(ownCount));
                for(std::int32_t i=0;i<ownCount;++i) {
                    const auto key=ownKeys[i];
                    if(!key || std::find(keys.data(),keys.data()+count,key)==keys.data()+count ||
                       !s.textures->GetTexture(key))continue;
                    use.keys.push_back(key);
                    ++batch.remainingUsers[key];
                }
                batch.uses.push_back(std::move(use));
            }
            // Publish the acquisition ledger before the physical texture
            // service, which can complete synchronously for cached pixels.
            s.textureBatches.push_back(std::move(batch));
            s.loading=true;
            s.textures->LoadTexturesFromFile(keys.data(),count,this);return;
        }
    } catch(...) {
        s.completion=nullptr;s.flow=nullptr;s.gameFlow=nullptr;
        // A marker or completed borrower retains its genuine acquisition.
        // If the request failed before marker publication, TexturePack has
        // already rolled back pixel/palette references; remove only our ledger.
        const bool hasCompleted=std::any_of(requesting.begin(),requesting.end(),
            [](const LoadedSkin* skin){return skin->loaded;});
        if(!s.textures->HasCarCompletion(this) && !hasCompleted) {
            for(auto* skin:requesting)skin->loading=false;
            if(batchId) {
                const auto it=std::find_if(s.textureBatches.begin(),s.textureBatches.end(),
                    [batchId](const CarTextureBatch& b){return b.id==batchId;});
                if(it!=s.textureBatches.end())s.textureBatches.erase(it);
            }
            s.loading=false;
        }
        throw;
    }
    for (auto& car:s.cars)if (!car->loading && !car->loaded) {
        // Queue creation can fail before ResourceLoader publishes a file. No
        // completion runs inside this call, so commit the loading state only
        // after the real request exists; otherwise shutdown cannot drain it.
        try {
            car->file=s.files->LoadResourceFile(car->filename.c_str(),5,8,s.bindings,ResourceFileCompletion::CarLoaded);
        } catch (...) {
            s.completion=nullptr;s.flow=nullptr;s.gameFlow=nullptr;
            throw;
        }
        car->loading=true;s.loading=true;
        break;
    }
}

void CarLoader::LoadingDoneCallback()
{
    auto& s=*storage_;
    if(s.shuttingDown)return;
    for (auto& skin:s.skins)if (skin->loading) {skin->loading=false;skin->loaded=true;}
    for (auto& car:s.cars)if (car->loading) {car->loading=false;car->loaded=true;}
    s.loading=false;RemoveUnusedCarsAndSkins();BeginLoading();
    if (!s.loading && s.completion) {const auto done=s.completion;s.completion=nullptr;done();}
    if (!s.loading && s.flow) {
        auto* flow=s.flow;const auto* bindings=s.gameFlow;
        s.flow=nullptr;s.gameFlow=nullptr;
        Require(bindings!=nullptr,"CarLoader completion lost its application flow");
        GameFlowLoadingTrackCommonPart2(*flow,*bindings);
    }
}

void CarLoader::BeginLoading(ApplicationFrontendFlow& flow,
    const GameFlowBindings& bindings)
{
    auto& state=*storage_;
    Require(!state.shuttingDown,"CarLoader is shutting down");
    Require(!state.completion && !state.flow,"CarLoader already has an outstanding completion");
    state.flow=&flow;state.gameFlow=&bindings;
    BeginLoading();
}

void CarLoader::RemoveUnusedCarsAndSkins()
{
    auto& s=*storage_;
    for (auto it=s.cars.begin();it!=s.cars.end();) {
        auto& car=**it;
        if (!car.references) {
            if (car.loaded) {Require(s.files!=nullptr,"Car retirement lost its ResourceLoader");s.files->UnloadResourceFile(car.file);car.file=nullptr;car.loaded=false;}
            if (!car.loading) {it=s.cars.erase(it);continue;}
        }
        ++it;
    }
    for (auto it=s.skins.begin();it!=s.skins.end();) {
        auto& skin=**it;
        if (!skin.references) {
            if (skin.loaded) {
                Require(s.textures!=nullptr,"Skin retirement lost its actual texture pack");
                bool foundOwner=false;
                for(auto batch=s.textureBatches.begin();batch!=s.textureBatches.end();++batch) {
                    auto use=std::find_if(batch->uses.begin(),batch->uses.end(),
                        [&skin](const CarSkinTextureUse& entry){return entry.skin==&skin;});
                    if(use==batch->uses.end())continue;
                    foundOwner=true;
                    // Release one acquired key only after the last skin
                    // depending on it in this batch has retired. Another
                    // batch of the same texture owns a separate reference.
                    while(!use->keys.empty()) {
                        const auto key=use->keys.back();
                        auto count=batch->remainingUsers.find(key);
                        Require(count!=batch->remainingUsers.end() && count->second>0,
                            "Skin texture acquisition count is corrupted");
                        if(count->second==1) {
                            s.textures->UnloadTexturesFromFile(&key,1);
                            batch->remainingUsers.erase(count);
                        } else --count->second;
                        // Commit this key once, only after successful release.
                        // A retry begins at the first unconsumed key.
                        use->keys.pop_back();
                    }
                    batch->uses.erase(use);
                    if(batch->uses.empty())s.textureBatches.erase(batch);
                    break;
                }
                Require(foundOwner,"Loaded skin has no source texture-acquisition owner");
                skin.loaded=false;
            }
            if (!skin.loading) {it=s.skins.erase(it);continue;}
        }
        ++it;
    }
}

void CarLoader::UnloadEverything()
{
    auto& s=*storage_;
    if(s.shuttingDown) {
        if(s.files)if(const auto error=Shutdown(*s.files))std::rethrow_exception(error);
        return;
    }
    for (auto& car:s.cars)car->references=0;
    for (auto& skin:s.skins)skin->references=0;
    RemoveUnusedCarsAndSkins();
    while (!s.cars.empty() || !s.skins.empty()) {
        Require(s.files!=nullptr,"CarLoader cannot drain a missing resource service");
        // The ordinary file service cannot advance a texture request whose
        // first read failed before entering the physical queue.
        if(s.textures && s.textures->HasCarCompletion(this) && !s.textures->HasPendingReads())
            s.textures->ServiceLoadingTextures();
        s.files->ServiceResourceLoading();RemoveUnusedCarsAndSkins();
    }
}
} // namespace hp2
