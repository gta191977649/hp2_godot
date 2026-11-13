#include "indep/inc/Player.hpp"
#include "indep/inc/CarDriver.hpp"
#include "bware/inc/bMath.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "indep/inc/Race.hpp"
#include "indep/inc/CameraCoordinator.hpp"

#include <cstring>
#include <stdexcept>
#include <utility>

namespace hp2 {
namespace {

class CameraReader final {
public:
    CameraReader(const std::uint8_t* bytes, std::size_t size) : bytes_(bytes), size_(size) {
        if (!bytes && size) throw std::invalid_argument("Missing camera description bytes");
    }

    std::uint32_t Word() {
        Need(4);
        const auto* word = bytes_ + position_;
        position_ += 4;
        return static_cast<std::uint32_t>(word[0]) |
            (static_cast<std::uint32_t>(word[1]) << 8) |
            (static_cast<std::uint32_t>(word[2]) << 16) |
            (static_cast<std::uint32_t>(word[3]) << 24);
    }

    float Scalar() {
        const auto word = Word();
        float value;
        static_assert(sizeof(value) == sizeof(word));
        std::memcpy(&value, &word, sizeof(value));
        return value;
    }

    std::string Name() {
        Need(32);
        std::size_t length = 0;
        while (length < 32 && bytes_[position_ + length]) ++length;
        std::string name(reinterpret_cast<const char*>(bytes_ + position_), length);
        position_ += 32;
        return name;
    }

    CameraVector4 Vector() {
        CameraVector4 result{};
        for (float& component : result) component = Scalar();
        return result;
    }

    std::vector<CameraVector4> Keys(std::uint32_t count) {
        Elements(count, 16);
        std::vector<CameraVector4> result(count);
        for (auto& key : result) key = Vector();
        return result;
    }

    void Elements(std::uint32_t count, std::size_t stride) const {
        if (count > (size_ - position_) / stride)
            throw std::invalid_argument("Camera stream exceeds its resource");
    }

    void Skip(std::size_t count) {
        Need(count);
        position_ += count;
    }

    std::size_t Position() const noexcept { return position_; }

private:
    void Need(std::size_t count) const {
        if (count > size_ - position_)
            throw std::invalid_argument("Truncated camera description");
    }

    const std::uint8_t* bytes_;
    std::size_t size_;
    std::size_t position_ = 0;
};

} // namespace

CameraDataItems::CameraDataItems(const std::uint8_t* bytes, std::size_t size) {
    CameraReader reader(bytes, size);
    auto decoded = std::make_shared<Data>();
    decoded->format = reader.Word();
    if (decoded->format > 2) throw std::invalid_argument("Unknown camera description format");
    decoded->name = reader.Name();

    if (decoded->format == 0) {
        for (float& coefficient : decoded->legacy.authoredCoefficients)
            coefficient = reader.Scalar();
        for (float& coefficient : decoded->legacy.start) coefficient = reader.Scalar();
        for (float& coefficient : decoded->legacy.end) coefficient = reader.Scalar();
        decoded->legacy.interpolate = reader.Word();
    }

    decoded->playback.eyeAdvanceRate = reader.Scalar();
    if (decoded->format != 0) decoded->playback.targetAdvanceRate = reader.Scalar();
    if (decoded->format == 2) decoded->playback.authoredParameter = reader.Scalar();
    decoded->playback.eyeUsesRouteFraction = reader.Word();
    if (decoded->format != 0) decoded->playback.targetUsesRouteFraction = reader.Word();
    decoded->playback.routeStart = reader.Scalar();
    decoded->playback.routeEnd = reader.Scalar();
    decoded->projection.usesWidth = reader.Word();
    decoded->projection.routeStart = reader.Scalar();
    decoded->projection.routeEnd = reader.Scalar();
    const auto vertexCount = reader.Word();
    const auto triangleCount = reader.Word();
    const auto eyeCount = reader.Word();
    std::uint32_t targetCount = 0;
    if (decoded->format == 0) {
        decoded->legacy.eyeProjectionGate = reader.Scalar();
        decoded->legacy.closestApproachGate = reader.Word();
    } else {
        targetCount = reader.Word();
    }
    decoded->nearClip = reader.Scalar();
    decoded->farClip = reader.Scalar();
    decoded->directionGate = reader.Word();
    if (decoded->format == 0) {
        decoded->legacy.placementSelector = reader.Word();
        reader.Skip(8);
    } else {
        reader.Skip(decoded->format == 1 ? 12 : 8);
    }

    for (auto& sample : decoded->projection.samples) sample = reader.Vector();
    decoded->eyeKeys = reader.Keys(eyeCount);
    decoded->rollKeys = reader.Keys(eyeCount);
    decoded->targetKeys = reader.Keys(targetCount);
    reader.Elements(triangleCount, 16);
    decoded->regionTriangles.resize(triangleCount);
    for (auto& triangle : decoded->regionTriangles) {
        for (auto& vertex : triangle.vertices) {
            vertex = reader.Word();
            if (vertex >= vertexCount)
                throw std::invalid_argument("Camera region references a missing vertex");
        }
        triangle.properties = reader.Word();
    }
    reader.Elements(vertexCount, 8);
    decoded->regionVertices.resize(vertexCount);
    for (auto& vertex : decoded->regionVertices) {
        vertex.x = reader.Scalar();
        vertex.y = reader.Scalar();
    }
    decoded->encodedBytes = reader.Position();
    data_ = std::move(decoded);
}

CameraDataItems::CameraDataItems(const CameraDataItems& other) noexcept : data_(other.data_) {}

} // namespace hp2

int32_t carViewRegionContains(const CarViewAuthoredCamera* camera,const CarMovement* movement)
{
    for (size_t index=0;index<camera->triangleCount;++index) {
        const CarViewRegionTriangle* triangle=camera->triangles+index;
        CarViewRegionVertex points[3];
        for (int corner=0;corner<3;++corner) points[corner]=camera->vertices[triangle->vertices[corner]];
        if (carViewPointInPolygon(movement->bindings.state->position,points,3)) return 1;
    }
    return 0;
}

/* Canonical CameraLoader resource adapter, PAL199CD8. */
#include "bware/inc/bMath.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/AnimatedDriver.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::uint32_t word(const std::uint8_t* bytes,std::size_t size,std::size_t offset)
{
    if (offset>size || size-offset<4) throw std::runtime_error("Truncated camera resource word");
    return std::uint32_t(bytes[offset]) | (std::uint32_t(bytes[offset+1])<<8) |
        (std::uint32_t(bytes[offset+2])<<16) | (std::uint32_t(bytes[offset+3])<<24);
}
float scalar(const std::uint8_t* bytes,std::size_t size,std::size_t offset)
{
    const auto bits=word(bytes,size,offset);
    float value;
    std::memcpy(&value,&bits,sizeof(value));
    return value;
}
std::string name(const std::uint8_t* bytes,std::size_t size,std::size_t offset)
{
    if (offset>size || size-offset<32) throw std::runtime_error("Truncated camera resource name");
    const auto* first=reinterpret_cast<const char*>(bytes+offset);
    return std::string(first,std::find(first,first+32,'\0'));
}
bool equalName(const std::string& authored,const char* requested)
{
    const auto upper=[](unsigned char value) {
        return value>='a' && value<='z'?static_cast<unsigned char>(value&0x5f):value;
    };
    std::size_t index=0;
    for (;index<authored.size();++index) {
        if (!requested[index] || upper(static_cast<unsigned char>(authored[index]))!=
                upper(static_cast<unsigned char>(requested[index]))) return false;
    }
    return requested[index]=='\0';
}
struct Authored {
    hp2::CameraDataItems decoded;
    CarViewAuthoredCamera view{};
    std::vector<CarViewRegionVertex> vertices;
    std::vector<CarViewRegionTriangle> triangles;
    Authored(const std::uint8_t* bytes,std::size_t size):decoded(bytes,size)
    {
        const auto& data=decoded.Get();
        auto& items=view.items;
        items.format=data.format;items.name=data.name.c_str();
        items.playback={data.playback.eyeAdvanceRate,data.playback.targetAdvanceRate,
            data.playback.authoredParameter,data.playback.eyeUsesRouteFraction,
            data.playback.targetUsesRouteFraction,data.playback.routeStart,data.playback.routeEnd};
        items.projection={data.projection.usesWidth,data.projection.routeStart,
            data.projection.routeEnd,reinterpret_cast<const CarCameraPoint*>(data.projection.samples.data())};
        items.nearClip=data.nearClip;items.farClip=data.farClip;items.directionGate=data.directionGate;
        std::copy(data.legacy.authoredCoefficients.begin(),data.legacy.authoredCoefficients.end(),items.legacy.authoredCoefficients);
        std::copy(data.legacy.start.begin(),data.legacy.start.end(),items.legacy.start);
        std::copy(data.legacy.end.begin(),data.legacy.end.end(),items.legacy.end);
        items.legacy.interpolate=data.legacy.interpolate;
        items.legacy.eyeProjectionGate=data.legacy.eyeProjectionGate;
        items.legacy.closestApproachGate=data.legacy.closestApproachGate;
        items.legacy.placementSelector=data.legacy.placementSelector;
        items.eyeKeys=reinterpret_cast<const CarCameraPoint*>(data.eyeKeys.data());
        items.rollKeys=reinterpret_cast<const CarCameraPoint*>(data.rollKeys.data());
        items.eyeCount=static_cast<int32_t>(data.eyeKeys.size());
        items.targetKeys=reinterpret_cast<const CarCameraPoint*>(data.targetKeys.data());
        items.targetCount=static_cast<int32_t>(data.targetKeys.size());
        for (const auto& vertex:data.regionVertices) vertices.push_back({vertex.x,vertex.y});
        for (const auto& triangle:data.regionTriangles)
            triangles.push_back({{triangle.vertices[0],triangle.vertices[1],triangle.vertices[2]},triangle.properties});
        view.vertices=vertices.data();view.vertexCount=vertices.size();
        view.triangles=triangles.data();view.triangleCount=triangles.size();
    }
};
struct Take {
    std::string title;
    std::vector<CarViewRecordedPose> poses;
    CarViewRecordedTake view{};
    Take(const std::uint8_t* bytes,std::size_t size):title(name(bytes,size,0))
    {
        const std::uint32_t count=word(bytes,size,32);
        if (!count || size<36 || count>(size-36)/48)
            throw std::runtime_error("Invalid recorded camera pose count");
        poses.resize(count);
        for (std::size_t index=0;index<count;++index) {
            auto& pose=poses[index];const std::size_t base=36+index*48;
            for (std::size_t axis=0;axis<3;++axis) pose.position[axis]=scalar(bytes,size,base+axis*4);
            for (std::size_t axis=0;axis<4;++axis) pose.quaternion[axis]=scalar(bytes,size,base+16+axis*4);
            pose.fieldOfView=static_cast<std::uint16_t>(word(bytes,size,base+32));
        }
        view={title.c_str(),poses.data(),poses.size()};
    }
};
}

struct CarViewCameraResources {
    std::array<std::vector<std::shared_ptr<Authored>>,4> groups;
    std::vector<std::shared_ptr<Take>> pendingTakes;
    std::array<std::vector<std::shared_ptr<Take>>,2> replayGroups;
    const void* pendingAssetOwner{};
    std::string pendingFileIdentity;
    std::array<const void*,2> replayAssetOwners{};
    std::array<std::string,2> replayFileIdentities;
    std::shared_ptr<std::vector<CarCameraOffsetTrack>> currentOffsets;
    std::uint32_t revision=0;
    CarViewMoverRegistry* movers=nullptr;
    struct ReplayFile {
        const void* assetOwner;
        std::string identity;
        std::size_t encodedSize;
        std::vector<std::shared_ptr<Take>> takes;
    };
    /* Immutable file identities come from one retained GameResources owner
     * and the finite authored NIS catalogue. Every re-publication borrows
     * these same decoded takes; repeated playback does not append pose data. */
    std::vector<ReplayFile> replayFiles;

    void LoadRange(const std::uint8_t* bytes,std::size_t size,std::size_t first,std::size_t end,unsigned depth=0)
    {
        if (depth>32) throw std::runtime_error("Camera resource nesting exceeds 32 levels");
        while (first<end) {
            if (end-first<8) throw std::runtime_error("Truncated camera chunk header");
            const auto kind=word(bytes,size,first),length=word(bytes,size,first+4);
            const std::size_t payload=first+8;
            if (length>end-payload) throw std::runtime_error("Camera chunk exceeds its container");
            const std::size_t stop=payload+length;
            int group=-1;std::uint32_t leaf=0;
            switch (kind) {
            case 0x80034405u:group=0;leaf=0x34450u;break;
            case 0x80034425u:group=1;leaf=0x34490u;break;
            case 0x80034410u:group=2;leaf=0x34460u;break;
            case 0x80034415u:group=3;leaf=0x34470u;break;
            default:break;
            }
            if (group>=0 || kind==0x80034420u) {
                if (group>=0) groups[static_cast<std::size_t>(group)].clear();
                for (std::size_t child=payload;child<stop;) {
                    if (stop-child<8) throw std::runtime_error("Truncated nested camera chunk");
                    const auto childKind=word(bytes,size,child),childLength=word(bytes,size,child+4);
                    if (childLength>stop-child-8) throw std::runtime_error("Nested camera chunk exceeds parent");
                    const std::size_t last=child+8+childLength;
                    const std::size_t aligned=(child+23)&~std::size_t(15);
                    if (aligned>last) throw std::runtime_error("Camera alignment exceeds chunk");
                    if (group>=0 && childKind==leaf) {
                        auto record=std::make_shared<Authored>(bytes+aligned,last-aligned);
                        auto& destination=groups[static_cast<std::size_t>(group)];
                        const std::size_t maximum=group==0?512:32;
                        if (destination.size()>=maximum) throw std::runtime_error("Camera catalogue capacity exceeded");
                        destination.push_back(std::move(record));
                    } else if (group==0 && childKind==0x34440u) {
                        revision=word(bytes,size,aligned+16);
                    } else if (kind==0x80034420u && childKind==0x34480u) {
                        if (pendingTakes.size()>=8) throw std::runtime_error("Replay camera take capacity exceeded");
                        auto take=std::make_shared<Take>(bytes+aligned,last-aligned);
                        pendingTakes.push_back(std::move(take));
                    }
                    child=last;
                }
            } else if (kind==0x34492u) {
                const std::size_t aligned=(payload+15)&~std::size_t(15),count=length/560;
                if (aligned>stop || count>(stop-aligned)/560)
                    throw std::runtime_error("Truncated camera offset track array");
                auto records=std::make_shared<std::vector<CarCameraOffsetTrack>>(count);
                for (std::size_t index=0;index<count;++index) {
                    const std::size_t base=aligned+index*560;auto& record=(*records)[index];
                    const auto title=name(bytes,size,base);
                    std::copy(title.begin(),title.end(),record.name);
                    record.flags=word(bytes,size,base+32);record.advanceRate=scalar(bytes,size,base+36);
                    record.count=static_cast<int32_t>(word(bytes,size,base+40));record.properties=word(bytes,size,base+44);
                    if (record.count<1 || record.count>32) throw std::runtime_error("Invalid camera offset sample count");
                    for (std::size_t point=0;point<32;++point)
                        for (std::size_t axis=0;axis<4;++axis)
                            record.keys[point][axis]=scalar(bytes,size,base+48+point*16+axis*4);
                }
                currentOffsets=std::move(records);
            } else if (kind&0x80000000u) LoadRange(bytes,size,payload,stop,depth+1);
            first=stop;
        }
    }
};

extern "C" CarViewCameraResources* carViewCameraResourcesCreate(void)
{ try { return new CarViewCameraResources; } catch (...) { return nullptr; } }
extern "C" void carViewCameraResourcesDispose(CarViewCameraResources* resources) { delete resources; }
extern "C" int carViewCameraResourcesBindMovers(CarViewCameraResources* resources,CarViewMoverRegistry* registry)
{
    if (!resources || (registry && resources->movers && resources->movers!=registry)) return 0;
    resources->movers=registry;return 1;
}
extern "C" int carViewCameraResourcesLoadBundle(CarViewCameraResources* resources,const uint8_t* bytes,size_t size)
{
    if (!resources || (!bytes&&size)) return 0;
    try {
        /* A failed decode leaves the live catalogue and its borrowed views
         * intact; copies here share decoded records, not wire memory. */
        auto prepared=*resources;prepared.LoadRange(bytes,size,0,size);*resources=std::move(prepared);
        if (resources->movers) carViewMoversInvalidate(resources->movers);
        return 1;
    } catch (...) { return 0; }
}
extern "C" int carViewCameraResourcesCommitReplayGroup(CarViewCameraResources* resources,int32_t group)
{
    if (!resources || group<0 || group>1) return 0;
    const auto index=static_cast<std::size_t>(group);
    resources->replayGroups[index]=std::move(resources->pendingTakes);
    resources->replayAssetOwners[index]=resources->pendingAssetOwner;
    resources->replayFileIdentities[index]=std::move(resources->pendingFileIdentity);
    resources->pendingTakes.clear();resources->pendingAssetOwner=nullptr;
    resources->pendingFileIdentity.clear();return 1;
}

extern "C" void carViewCameraResourcesRetireReplayFile(CarViewCameraResources* resources,
    const void* assetOwner,const char* immutableFileIdentity,int32_t group) noexcept
{
    if (!resources || !assetOwner || !immutableFileIdentity || group<0 || group>1)return;
    const auto index=static_cast<std::size_t>(group);
    if (resources->replayAssetOwners[index]==assetOwner &&
        resources->replayFileIdentities[index]==immutableFileIdentity) {
        resources->replayGroups[index].clear();resources->replayAssetOwners[index]=nullptr;
        resources->replayFileIdentities[index].clear();
    }
    if (resources->pendingAssetOwner==assetOwner && resources->pendingFileIdentity==immutableFileIdentity) {
        resources->pendingTakes.clear();resources->pendingAssetOwner=nullptr;
        resources->pendingFileIdentity.clear();
    }
}

extern "C" int carViewCameraResourcesUnloadTrackGroups(CarViewCameraResources* resources,uint32_t mask)
{
    if (!resources || (mask&~31u))return 0;
    // PAL19A028..19A080 resets exactly the selected source group count.
    // A live CarViewChild retains only its own immutable record; replacing
    // this publication cannot destroy its borrowed spline spans. Unused
    // records now retire here, instead of accumulating for the process life.
    for (std::size_t group=0;group<4;++group)
        if (mask&(1u<<group))resources->groups[group].clear();
    if (mask&16u)resources->currentOffsets.reset();
    if (resources->movers)carViewMoversInvalidate(resources->movers);
    return 1;
}
extern "C" int carViewCameraResourcesLoadReplayFile(CarViewCameraResources* resources,
    const void* assetOwner,const char* immutableFileIdentity,const uint8_t* bytes,size_t size)
{
    if (!resources || !assetOwner || !immutableFileIdentity || !*immutableFileIdentity ||
        !bytes || !size || !resources->pendingTakes.empty()) return 0;
    try {
        for (const auto& file : resources->replayFiles) {
            if (file.assetOwner == assetOwner && file.identity == immutableFileIdentity) {
                if (file.encodedSize != size) return 0;
                auto pending=file.takes;
                std::string identity=immutableFileIdentity;
                resources->pendingTakes=std::move(pending);
                resources->pendingFileIdentity=std::move(identity);
                resources->pendingAssetOwner=assetOwner;
                return 1;
            }
        }
        auto prepared = *resources;
        prepared.LoadRange(bytes,size,0,size);
        if (prepared.pendingTakes.empty()) return 0;
        prepared.pendingFileIdentity=immutableFileIdentity;
        prepared.pendingAssetOwner=assetOwner;
        prepared.replayFiles.push_back({assetOwner,immutableFileIdentity,size,prepared.pendingTakes});
        *resources = std::move(prepared);
        if (resources->movers) carViewMoversInvalidate(resources->movers);
        return 1;
    } catch (...) {
        return 0;
    }
}

extern "C" size_t carViewCameraResourcesReplayCacheCount(const CarViewCameraResources* resources)
{
    return resources ? resources->replayFiles.size() : 0;
}
extern "C" uint32_t carViewCameraResourcesRevision(const CarViewCameraResources* resources)
{ return resources?resources->revision:0; }
extern "C" size_t carViewCameraResourcesRegionCount(const CarViewCameraResources* resources)
{ return resources?resources->groups[0].size():0; }
extern "C" const CarViewAuthoredCamera* carViewCameraResourcesRegionAt(const CarViewCameraResources* resources,size_t index)
{ return resources&&index<resources->groups[0].size()?&resources->groups[0][index]->view:nullptr; }
extern "C" const CarViewAuthoredCamera* carViewCameraResourcesFind(const CarViewCameraResources* resources,const char* title)
{
    if (resources&&title) for (const auto& group:resources->groups)
        for (const auto& record:group) if (equalName(record->decoded.Get().name,title)) return &record->view;
    return nullptr;
}
extern "C" const CarCameraOffsetTrack* carViewCameraResourcesChooseOffset(const CarViewCameraResources* resources,uint32_t selector)
{
    if (!resources || !selector || !resources->currentOffsets) return nullptr;
    std::uint32_t count=0;
    for (const auto& record:*resources->currentOffsets) if (record.flags==selector) ++count;
    if (!count) return nullptr;
    auto chosen=vectorDefaultRandomBounded(count);
    for (const auto& record:*resources->currentOffsets) if (record.flags==selector) {
        if (!chosen) return &record;
        --chosen;
    }
    return nullptr;
}
extern "C" size_t carViewCameraResourcesTakeCount(const CarViewCameraResources* resources,int32_t group)
{ return resources&&group>=0&&group<2?resources->replayGroups[static_cast<std::size_t>(group)].size():0; }
extern "C" const CarViewRecordedTake* carViewCameraResourcesTakeAt(const CarViewCameraResources* resources,int32_t group,size_t index)
{
    if (!resources || group<0 || group>1) return nullptr;
    const auto& records=resources->replayGroups[static_cast<std::size_t>(group)];
    return index<records.size()?&records[index]->view:nullptr;
}

namespace hp2 {
std::shared_ptr<const void> RetainCameraResource(const CarViewCameraResources& resources,
    const CarViewAuthoredCamera* camera)
{
    if(camera)for(const auto& group:resources.groups)for(const auto& record:group)
        if(&record->view==camera)return record;
    return {};
}
std::shared_ptr<const void> RetainCameraResource(const CarViewCameraResources& resources,
    const CarViewRecordedTake* take)
{
    if(take)for(const auto& group:resources.replayGroups)for(const auto& record:group)
        if(&record->view==take)return record;
    return {};
}
std::shared_ptr<const void> RetainCameraResource(const CarViewCameraResources& resources,
    const CarCameraOffsetTrack* offset)
{
    if(offset && resources.currentOffsets)for(const auto& record:*resources.currentOffsets)
        if(&record==offset)return resources.currentOffsets;
    return {};
}
}

struct CarViewResourceLease {
    std::shared_ptr<const void> record;
};
namespace {
template<class Record>
std::unique_ptr<CarViewResourceLease> retainChildResource(const CarViewCameraResources& resources,
    const Record* record)
{
    auto owner=hp2::RetainCameraResource(resources,record);
    if(!owner)return {};
    return std::unique_ptr<CarViewResourceLease>(new(std::nothrow) CarViewResourceLease{std::move(owner)});
}
}

// The same settings owner holds both mutable source camera-control cells.
void carViewMoverSettingsInitialize(CarViewMoverSettings* settings)
{
    if (!settings)return;
    settings->projectionAngle=9102;
    settings->loopPreRaceCameras=0; // PAL32FEAC authored word is zero.
}

CarViewMover* carViewMoverCreateForFlyby(CarViewMoverRegistry* registry,int32_t viewNumber,
    CarViewMoverResult* result)
{
    if (result)*result=CAR_VIEW_MOVER_INVALID_BINDING;
    if (!registry || !registry->environment.resources)return nullptr;
    auto* mover=carViewMoverAllocate(registry,viewNumber,CAR_VIEW_MOVER_FLYBY);
    if (!mover) {
        if(result)*result=CAR_VIEW_MOVER_ALLOCATION_FAILURE;
        return nullptr;
    }
    mover->state.flyby.activeIndex=-1;
    mover->state.flyby.nextIndex=0;
    mover->state.flyby.resourceRevision=carViewCameraResourcesRevision(registry->environment.resources);
    if(result)*result=CAR_VIEW_MOVER_OK;
    return mover;
}

int32_t carViewMoverFlybyFinished(const CarViewMover* mover)
{
    if (!mover || mover->kind!=CAR_VIEW_MOVER_FLYBY || !mover->registry ||
        !mover->registry->environment.resources)return 0;
    // This source query reads the global *catalogue count*, not child time.
    return mover->registry->environment.resources->groups[1].empty()?1:0;
}

CarViewMoverResult carViewMoverUpdateFlyby(CarViewMover* mover)
{
    if (!mover || mover->kind!=CAR_VIEW_MOVER_FLYBY || !mover->registry)
        return CAR_VIEW_MOVER_INVALID_BINDING;
    auto& state=mover->state.flyby;
    auto& environment=mover->registry->environment;
    bool change=state.activeIndex==-1 || mover->dirty!=0;
    if(change)mover->dirty=0;
    else {
        const auto* base=carViewChildBase(&mover->child);
        change=!base || base->kind!=9 || mover->child.kind!=CAR_VIEW_CHILD_PATH ||
            carPathCameraFinished(mover->child.owner.path)!=0;
    }
    if (!change)return CAR_VIEW_MOVER_OK;
    if (!environment.resources || !environment.services || !environment.race)
        return CAR_VIEW_MOVER_INVALID_BINDING;
    const auto& group=environment.resources->groups[1];
    // The PAL caller skips this selector when the real catalogue is empty.
    // Invalid host invocation is an explicit resource error, never a camera.
    if(group.empty())return CAR_VIEW_MOVER_MISSING_RESOURCE;
    if(state.nextIndex<0 || static_cast<std::size_t>(state.nextIndex)>=group.size())state.nextIndex=0;
    const auto& authored=group[static_cast<std::size_t>(state.nextIndex)]->view;
    auto lease=retainChildResource(*environment.resources,&authored);
    if(!lease)return CAR_VIEW_MOVER_ALLOCATION_FAILURE;
    ++state.resourceRevision;
    carViewChildDispose(&mover->child);
    CarPathCameraContext context{};
    context.services=environment.services;
    context.race=environment.race;
    context.items=&authored.items;
    context.viewNumber=mover->viewNumber;
    CarCinematicResult result;
    mover->child.owner.path=carPathCameraCreate(&context,&result);
    if(!mover->child.owner.path)return CAR_VIEW_MOVER_CAMERA_FAILURE;
    mover->child.kind=CAR_VIEW_CHILD_PATH;
    mover->child.resourceLease=lease.release();
    state.activeIndex=state.nextIndex++;
    return CAR_VIEW_MOVER_OK;
}

CarViewMover* carViewMoverCreateForPreRace(CarViewMoverRegistry* registry,int32_t viewNumber,
    int32_t specialPursuit,CarViewMoverResult* result)
{
    if(result)*result=CAR_VIEW_MOVER_INVALID_BINDING;
    if(!registry || !registry->environment.resources || !registry->environment.simulation ||
        !registry->environment.simulation->players || !registry->environment.simulation->race)
        return nullptr;
    const auto& simulation=*registry->environment.simulation;
    const auto& players=*simulation.players;
    const auto& race=*simulation.race;
    if(players.count<1 || players.count>2 || !race.entries || !race.entryCount)
        return nullptr;
    auto movement=[](const RaceCarEntry* entry)->CarMovement* {
        if(!entry || !entry->car)return nullptr;
        auto* value=static_cast<CarMovement*>(entry->car->movement);
        return value && value->bindings.owner==entry->car?value:nullptr;
    };
    CarMovement* selected[4]{};
    int32_t count=4;
    if(specialPursuit) {
        if(players.count==2) {
            if(!players.playersByNumber[0] || !players.playersByNumber[1])return nullptr;
            selected[0]=players.playersByNumber[0]->car;
            selected[1]=players.playersByNumber[1]->car;
            count=2;
        } else {
            for(std::size_t i=0;i<4;++i)selected[i]=movement(race.entries[i%race.entryCount]);
        }
    } else {
        selected[0]=movement(race.entries[0]);
        if(players.count==2) {
            if(!players.playersByNumber[0] || !players.playersByNumber[1])return nullptr;
            selected[1]=players.playersByNumber[0]->car;
            selected[2]=players.playersByNumber[1]->car;
            selected[3]=players.playersByNumber[0]->car;
        } else {
            if(!players.players[0])return nullptr;
            selected[1]=selected[2]=selected[3]=players.players[0]->car;
        }
    }
    for(int32_t i=0;i<count;++i)
        if(!selected[i] || !selected[i]->bindings.owner)return nullptr;
    auto* mover=carViewMoverAllocate(registry,viewNumber,CAR_VIEW_MOVER_PRE_RACE);
    if(!mover) {
        if(result)*result=CAR_VIEW_MOVER_ALLOCATION_FAILURE;
        return nullptr;
    }
    auto& state=mover->state.preRace;
    state.activeIndex=-1;
    state.cameraIndex=-1;
    state.resourceRevision=carViewCameraResourcesRevision(registry->environment.resources);
    state.numberToShow=state.carCount=count;
    std::copy(selected,selected+count,state.cars);
    if(result)*result=CAR_VIEW_MOVER_OK;
    return mover;
}

CarViewMoverResult carViewMoverUpdatePreRace(CarViewMover* mover)
{
    if(!mover || mover->kind!=CAR_VIEW_MOVER_PRE_RACE || !mover->registry)
        return CAR_VIEW_MOVER_INVALID_BINDING;
    auto& state=mover->state.preRace;
    auto& environment=mover->registry->environment;
    if(state.carIndex<0 || state.carIndex>=state.carCount || state.carCount>32)
        return CAR_VIEW_MOVER_INVALID_BINDING;
    CarMovement* selected=state.cars[state.carIndex];
    bool change=state.activeIndex==-1 || mover->dirty!=0;
    if(change)mover->dirty=0;
    else change=mover->child.kind==CAR_VIEW_CHILD_NONE ||
        (mover->child.kind==CAR_VIEW_CHILD_BUMPER && carBumperCameraFinished(mover->child.owner.bumper));
    if(!change)return CAR_VIEW_MOVER_OK;
    if(mover->child.kind!=CAR_VIEW_CHILD_NONE && mover->child.kind!=CAR_VIEW_CHILD_BUMPER)
        return CAR_VIEW_MOVER_INVALID_BINDING;
    if(!environment.settings || !environment.resources || !selected ||
        !selected->bindings.owner || !environment.chaseSettings || !environment.simulation ||
        !environment.simulation->race)
        return CAR_VIEW_MOVER_INVALID_BINDING;
    carViewChildDispose(&mover->child);
    if(state.shownCount==state.carCount && !environment.settings->loopPreRaceCameras) {
        state.finished=1;
        return CAR_VIEW_MOVER_OK;
    }
    if(++state.carIndex==state.carCount)state.carIndex=0;
    const auto& group=environment.resources->groups[2];
    if(group.empty())return CAR_VIEW_MOVER_MISSING_RESOURCE;
    if(++state.cameraIndex>=static_cast<int32_t>(group.size()))state.cameraIndex=0;
    if(auto* visual=selected->bindings.owner->visual) {
        carDriverVisualBeginPreRaceAnimation(visual);
        if(visual->result!=CAR_DRIVER_VISUAL_OK)return CAR_VIEW_MOVER_CAMERA_FAILURE;
    }
    const auto& authored=group[static_cast<std::size_t>(state.cameraIndex)]->view;
    auto lease=retainChildResource(*environment.resources,&authored);
    if(!lease)return CAR_VIEW_MOVER_ALLOCATION_FAILURE;
    ++state.resourceRevision;
    CarBumperCameraContext context{};
    context.services=environment.services;
    context.terrain=environment.terrain;
    context.movement=selected;
    context.race=environment.race;
    context.raceFrame=environment.simulation->race;
    context.items=&authored.items;
    context.ignoredTerrainMaterials=environment.chaseSettings->ignoredTerrainMaterials;
    context.viewNumber=mover->viewNumber;
    CarCinematicResult result;
    mover->child.owner.bumper=carBumperCameraCreate(&context,&result);
    if(!mover->child.owner.bumper)return CAR_VIEW_MOVER_CAMERA_FAILURE;
    mover->child.kind=CAR_VIEW_CHILD_BUMPER;
    mover->child.resourceLease=lease.release();
    ++state.shownCount;
    state.activeIndex=state.cameraIndex;
    return CAR_VIEW_MOVER_OK;
}

int32_t carViewMoverPreRaceFinished(const CarViewMover* mover)
{
    return mover && mover->kind==CAR_VIEW_MOVER_PRE_RACE?mover->state.preRace.finished:0;
}

CarViewMoverResult carViewMoversAdvance(CarViewMoverRegistry* registry,float elapsed)
{
    (void)elapsed; // Actual selector updates consume their existing clock owner.
    if(!registry)return CAR_VIEW_MOVER_INVALID_BINDING;
    for(auto* mover=registry->first;mover;mover=mover->next) {
        if(mover->updateSuspended)continue;
        CarViewMoverResult result=CAR_VIEW_MOVER_OK;
        switch(mover->kind) {
        case CAR_VIEW_MOVER_CAR:result=carViewMoverUpdateCar(mover);break;
        case CAR_VIEW_MOVER_COUNTDOWN:result=carViewMoverUpdateCountdown(mover);break;
        case CAR_VIEW_MOVER_FLYBY:result=carViewMoverUpdateFlyby(mover);break;
        case CAR_VIEW_MOVER_PRE_RACE:result=carViewMoverUpdatePreRace(mover);break;
        case CAR_VIEW_MOVER_REPLAY:break; // Source19B658 is an actual empty update.
        default:return CAR_VIEW_MOVER_INVALID_BINDING;
        }
        if(result!=CAR_VIEW_MOVER_OK)return result;
    }
    return CAR_VIEW_MOVER_OK;
}

/* Coordinator and NIS views. */
CarCinematicBase* carViewChildBase(const CarViewChild* child)
{
    switch (child->kind) {
    case CAR_VIEW_CHILD_CHASE:return carChaseCameraBase(child->owner.chase);
    case CAR_VIEW_CHILD_BUMPER:return carBumperCameraBase(child->owner.bumper);
    case CAR_VIEW_CHILD_PATH:return carPathCameraBase(child->owner.path);
    case CAR_VIEW_CHILD_AIRBORNE:return carAirborneCameraBase(child->owner.airborne);
    case CAR_VIEW_CHILD_RECORDED:return carRecordedCameraBase(child->owner.recorded);
    case CAR_VIEW_CHILD_NONE:return NULL;
    }
    return NULL;
}

void carViewChildDispose(CarViewChild* child)
{
    switch (child->kind) {
    case CAR_VIEW_CHILD_CHASE:carChaseCameraDispose(child->owner.chase);break;
    case CAR_VIEW_CHILD_BUMPER:carBumperCameraDispose(child->owner.bumper);break;
    case CAR_VIEW_CHILD_PATH:carPathCameraDispose(child->owner.path);break;
    case CAR_VIEW_CHILD_AIRBORNE:carAirborneCameraDispose(child->owner.airborne);break;
    case CAR_VIEW_CHILD_RECORDED:carRecordedCameraDispose(child->owner.recorded);break;
    case CAR_VIEW_CHILD_NONE:break;
    }
    delete child->resourceLease;
    memset(child,0,sizeof(*child));
}

CarViewMoverResult carViewChildAdvance(CarViewChild* child)
{
    CarCinematicResult result;
    switch (child->kind) {
    case CAR_VIEW_CHILD_CHASE:result=carChaseCameraAdvance(child->owner.chase);break;
    case CAR_VIEW_CHILD_BUMPER:result=carBumperCameraAdvance(child->owner.bumper);break;
    case CAR_VIEW_CHILD_PATH:result=carPathCameraAdvance(child->owner.path);break;
    case CAR_VIEW_CHILD_AIRBORNE:result=carAirborneCameraAdvance(child->owner.airborne);break;
    case CAR_VIEW_CHILD_RECORDED:result=carRecordedCameraAdvance(child->owner.recorded);break;
    case CAR_VIEW_CHILD_NONE:return CAR_VIEW_MOVER_OK;
    default:return CAR_VIEW_MOVER_INVALID_BINDING;
    }
    return result==CAR_CINEMATIC_OK?CAR_VIEW_MOVER_OK:CAR_VIEW_MOVER_CAMERA_FAILURE;
}

CarViewMover* carViewMoverAllocate(CarViewMoverRegistry* registry,int32_t viewNumber,CarViewMoverKind kind)
{
    if (!registry || viewNumber<0 || viewNumber>3 || !registry->environment.services->screens[viewNumber].view) return NULL;
    CarViewMover* mover=(CarViewMover*)calloc(1,sizeof(*mover));
    if (!mover) return NULL;
    mover->registry=registry;mover->viewNumber=viewNumber;mover->kind=kind;
    mover->previous=registry->last;
    if (registry->last) registry->last->next=mover;else registry->first=mover;
    registry->last=mover;return mover;
}

CarViewMoverRegistry* carViewMoverRegistryCreate(const CarViewMoverEnvironment* environment,CarViewMoverResult* result)
{
    if (result) *result=CAR_VIEW_MOVER_INVALID_BINDING;
    if (!environment || !environment->services || !environment->services->clock ||
        !environment->simulation || !environment->simulation->world || !environment->simulation->race ||
        !environment->simulation->drivers || !environment->resources || !environment->race || !environment->terrain ||
        !environment->vehicles || environment->vehicleCount>WORLD_MAX_CARS || !environment->settings ||
        !environment->chaseSettings || !environment->airborneSettings || !environment->recordedSettings ||
        !environment->shake || !environment->defaultCatalogue || !environment->simulation->world->carList.next ||
        !environment->simulation->world->carList.previous) return NULL;
    CarViewMoverRegistry* registry=(CarViewMoverRegistry*)calloc(1,sizeof(*registry));
    if (!registry) { if (result) *result=CAR_VIEW_MOVER_ALLOCATION_FAILURE;return NULL; }
    registry->environment=*environment;
    for (size_t index=0;index<environment->vehicleCount;++index) {
        const CarChaseCameraVehicle* vehicle=environment->vehicles+index;
        if (!vehicle->movement || !vehicle->movement->bindings.owner || !vehicle->movement->bindings.state ||
            !vehicle->movement->bindings.ownerMotion) {
            free(registry);return NULL;
        }
        registry->presentations[index].car=vehicle->movement->bindings.owner;
        registry->presentations[index].state.motion=vehicle->movement->bindings.ownerMotion;
    }
    registry->presentationCount=environment->vehicleCount;
    if (!carViewCameraResourcesBindMovers(environment->resources,registry)) { free(registry);return NULL; }
    if (result) *result=CAR_VIEW_MOVER_OK;
    return registry;
}

void carViewMoverRegistryDispose(CarViewMoverRegistry* registry)
{
    if (!registry) return;
    while (registry->first) carViewMoverDispose(registry->first);
    (void)carViewCameraResourcesBindMovers(registry->environment.resources,NULL);
    free(registry);
}

CarReplayPresentationState* carViewMoverPresentation(CarViewMoverRegistry* registry,const Car* car)
{
    if (!registry || !car) return NULL;
    for (size_t index=0;index<registry->presentationCount;++index)
        if (registry->presentations[index].car==car) return &registry->presentations[index].state;
    return NULL;
}

CarViewMover* carViewMoverCreateForCar(CarViewMoverRegistry* registry,int32_t viewNumber,
    CarMovement* car,CarViewMoverResult* result)
{
    if (result) *result=CAR_VIEW_MOVER_INVALID_BINDING;
    if (!registry || !car || !carViewMoverPresentation(registry,car->bindings.owner)) return NULL;
    CarViewMover* mover=carViewMoverAllocate(registry,viewNumber,CAR_VIEW_MOVER_CAR);
    if (!mover) { if (result) *result=CAR_VIEW_MOVER_ALLOCATION_FAILURE;return NULL; }
    mover->state.car.car=car;mover->state.car.regionIndex=-1;
    mover->state.car.resourceRevision=carViewCameraResourcesRevision(registry->environment.resources);
    if (result) *result=CAR_VIEW_MOVER_OK;
    return mover;
}

CarViewMover* carViewMoverCreateForCountdown(CarViewMoverRegistry* registry,int32_t viewNumber,
    CarFramePlayerState* player,CarViewMoverResult* result)
{
    if (result) *result=CAR_VIEW_MOVER_INVALID_BINDING;
    if (!registry || !player || !player->car || !carViewMoverPresentation(registry,player->car->bindings.owner)) return NULL;
    CarViewMover* mover=carViewMoverAllocate(registry,viewNumber,CAR_VIEW_MOVER_COUNTDOWN);
    if (!mover) { if (result) *result=CAR_VIEW_MOVER_ALLOCATION_FAILURE;return NULL; }
    mover->state.countdown.player=player;mover->state.countdown.stage=-1;
    const CarViewMoverResult update=carViewMoverUpdateCountdown(mover);
    if (update!=CAR_VIEW_MOVER_OK) { carViewMoverDispose(mover);if (result) *result=update;return NULL; }
    if (result) *result=CAR_VIEW_MOVER_OK;
    return mover;
}

void carViewMoverDispose(CarViewMover* mover)
{
    if (!mover) return;
    carViewChildDispose(&mover->child);
    CarViewMoverRegistry* registry=mover->registry;
    if (mover->previous) mover->previous->next=mover->next;else registry->first=mover->next;
    if (mover->next) mover->next->previous=mover->previous;else registry->last=mover->previous;
    free(mover);
}

void carViewMoverDeactivate(CarViewMover* mover)
{
    if (!mover || mover->updateSuspended) return;
    mover->updateSuspended=1;
    CarCinematicBase* base=carViewChildBase(&mover->child);
    if (base) carCinematicBaseDeactivate(base);
}

void carViewMoverActivate(CarViewMover* mover)
{
    if (!mover || !mover->updateSuspended) return;
    mover->updateSuspended=0;
    CarCinematicBase* base=carViewChildBase(&mover->child);
    if (base) carCinematicBaseActivate(base);
}

void carViewMoversInvalidate(CarViewMoverRegistry* registry)
{
    if (registry) for (CarViewMover* mover=registry->first;mover;mover=mover->next) mover->dirty=1;
}

CarCinematicBase* carViewMoverCurrentBase(CarViewMover* mover)
{ return mover?carViewChildBase(&mover->child):NULL; }

CarCameraReference carViewMoverCurrentCamera(const CarViewMover* mover)
{
    CarCameraReference reference;memset(&reference,0,sizeof(reference));
    if (mover && mover->child.kind==CAR_VIEW_CHILD_CHASE) {
        reference.kind=CAR_CAMERA_REFERENCE_CHASE;reference.owner.chase=mover->child.owner.chase;
    } else {
        reference.kind=CAR_CAMERA_REFERENCE_BASE;
        reference.owner.base=mover?carViewChildBase(&mover->child):NULL;
    }
    return reference;
}

CarViewMoverResult carViewMoverAdvanceCamera(CarViewMover* mover)
{ return mover?carViewChildAdvance(&mover->child):CAR_VIEW_MOVER_INVALID_BINDING; }

static const CarChaseCameraVehicle* coordinatorVehicleForMovement(const CarViewMoverEnvironment* environment,
    const CarMovement* movement)
{
    for (size_t index=0;index<environment->vehicleCount;++index)
        if (environment->vehicles[index].movement==movement) return environment->vehicles+index;
    return NULL;
}

static const CarDriver* coordinatorDriverForMovement(const CarViewMoverEnvironment* environment,const CarMovement* movement)
{
    const World* world=environment->simulation->world;
    for (int32_t index=0;index<world->totalCars;++index)
        if (world->carTable[index]==movement->bindings.owner)
            return (size_t)index<environment->simulation->frameCount?environment->simulation->drivers[index]:NULL;
    return NULL;
}

static CarChaseCameraContext coordinatorChaseContext(CarViewMover* mover,CarMovement* movement,int32_t viewIndex,
    int32_t dynamicResponse,int32_t rearFacing,float facingRate,int32_t extendedVisibility)
{
    const CarViewMoverEnvironment* environment=&mover->registry->environment;
    CarChaseCameraContext context{};
    context.services=environment->services;context.terrain=environment->terrain;
    context.movement=movement;context.vehicles=environment->vehicles;context.vehicleCount=environment->vehicleCount;
    context.defaultCatalogue=environment->defaultCatalogue;context.simulation=environment->simulation;
    context.shake=environment->shake;context.settings=environment->chaseSettings;
    context.viewNumber=mover->viewNumber;context.viewIndex=viewIndex;context.dynamicResponse=dynamicResponse;
    context.initiallyFacingBack=rearFacing;context.facingRate=facingRate;context.extendedVisibility=extendedVisibility;
    return context;
}

static CarViewMoverResult coordinatorCreateBumper(CarViewMover* mover,CarMovement* movement,
    const CarViewAuthoredCamera* authored,CarViewChild* child)
{
    const CarViewMoverEnvironment* environment=&mover->registry->environment;
    auto lease=retainChildResource(*environment->resources,authored);
    if(!lease)return CAR_VIEW_MOVER_ALLOCATION_FAILURE;
    CarBumperCameraContext context{};
    context.services=environment->services;context.terrain=environment->terrain;context.movement=movement;
    context.race=environment->race;context.raceFrame=environment->simulation->race;context.items=&authored->items;
    context.ignoredTerrainMaterials=environment->chaseSettings->ignoredTerrainMaterials;context.viewNumber=mover->viewNumber;
    CarCinematicResult result;child->owner.bumper=carBumperCameraCreate(&context,&result);
    if (!child->owner.bumper) return CAR_VIEW_MOVER_CAMERA_FAILURE;
    child->kind=CAR_VIEW_CHILD_BUMPER;child->resourceLease=lease.release();return CAR_VIEW_MOVER_OK;
}

int32_t carViewMoverSelectRegion(const CarViewMover* mover,const CarMovement* car,const CarDriver* driver)
{
    const CarViewMoverEnvironment* environment=&mover->registry->environment;
    const size_t count=carViewCameraResourcesRegionCount(environment->resources);
    for (size_t index=0;index<count;++index) {
        const CarViewAuthoredCamera* camera=carViewCameraResourcesRegionAt(environment->resources,index);
        const int reversed=environment->race->reverse==1;
        int desiredDirection=0;
        if (!carViewMoverOpposesRoad(car,driver)) desiredDirection=reversed;
        if (carViewMoverOpposesRoad(car,driver) && !reversed) desiredDirection=1;
        if ((desiredDirection!=0)==(camera->items.directionGate!=0) && carViewRegionContains(camera,car))
            return (int32_t)index;
    }
    return -1;
}

static int coordinatorCarNearEye(const World* world,const float eye[4],float distance)
{
    const Car* root=&world->carList;
    for (const Car* car=root->next;car!=root;car=car->next) {
        const float x=eye[0]-car->position[0],y=eye[1]-car->position[1],z=eye[2]-car->position[2];
        if (sqrtf((x*x+y*y)+z*z)<distance) return 1;
    }
    return 0;
}

static CarViewMoverResult coordinatorCreateAuthored(CarViewMover* mover,const CarViewAuthoredCamera* camera,
    CarViewChild* child)
{
    const CarViewMoverEnvironment* environment=&mover->registry->environment;
    CarMovement* movement=mover->state.car.car;
    if(camera->items.format==1)return coordinatorCreateBumper(mover,movement,camera,child);
    auto lease=retainChildResource(*environment->resources,camera);
    if(!lease)return CAR_VIEW_MOVER_ALLOCATION_FAILURE;
    CarCinematicResult result;
    if (camera->items.format==0) {
        /* The original producer performs this random selection but then
         * initializes a separate zero offset track. Retain the RNG effect. */
        (void)carViewCameraResourcesChooseOffset(environment->resources,camera->items.legacy.placementSelector);
        CarCameraOffsetTrack offset{};offset.count=2;
        CarAirborneCameraContext context{};
        context.services=environment->services;context.movement=movement;context.race=environment->race;
        context.items=&camera->items;context.offset=&offset;context.settings=environment->airborneSettings;
        context.viewNumber=mover->viewNumber;context.extendedVisibility=1;
        child->owner.airborne=carAirborneCameraCreate(&context,&result);
        if (!child->owner.airborne) return CAR_VIEW_MOVER_CAMERA_FAILURE;
        child->kind=CAR_VIEW_CHILD_AIRBORNE;
    } else if (camera->items.format==2) {
        CarPathCameraContext context{};
        context.services=environment->services;context.movement=movement;context.race=environment->race;
        context.items=&camera->items;context.viewNumber=mover->viewNumber;
        child->owner.path=carPathCameraCreate(&context,&result);
        if (!child->owner.path) return CAR_VIEW_MOVER_CAMERA_FAILURE;
        child->kind=CAR_VIEW_CHILD_PATH;
    } else return CAR_VIEW_MOVER_MISSING_RESOURCE;
    child->resourceLease=lease.release();
    return CAR_VIEW_MOVER_OK;
}

CarViewMoverResult carViewMoverUpdateCar(CarViewMover* mover)
{
    const CarViewMoverEnvironment* environment=&mover->registry->environment;
    CarMovement* car=mover->state.car.car;
    const CarDriver* driver=coordinatorDriverForMovement(environment,car);
    if (!driver) return CAR_VIEW_MOVER_INVALID_BINDING;
    const int32_t now=environment->services->clock->worldTimeFrames;
    const int32_t age=(int32_t)((uint32_t)now-(uint32_t)mover->state.car.lastSelectionFrame);
    int32_t region=-1,changed=0;
    const int32_t dirty=mover->dirty;mover->dirty=0;
    if (mover->child.kind==CAR_VIEW_CHILD_NONE || dirty) {
        region=carViewMoverSelectRegion(mover,car,driver);changed=1;
    } else if (age>=56) {
        const int32_t previous=mover->state.car.regionIndex;
        const CarViewAuthoredCamera* authored=previous>=0?
            carViewCameraResourcesRegionAt(environment->resources,(size_t)previous):NULL;
        if (!authored || !carViewRegionContains(authored,car)) {
            region=carViewMoverSelectRegion(mover,car,driver);changed=1;
            if (previous==-1 && region==-1) changed=0;
        }
    }
    int32_t blocked=0;
    CarCinematicBase* current=carViewChildBase(&mover->child);
    if (mover->state.car.regionIndex!=-1 && current && age>=56) {
        blocked=coordinatorCarNearEye(environment->simulation->world,current->view->current.eye,2.0f);
        if (carViewCrossesNearPlane(current->view,car,environment->settings->projectionAngle)) blocked=1;
    }
    if (!changed && !blocked) return CAR_VIEW_MOVER_OK;
    carViewChildDispose(&mover->child);
    if (region!=-1 && !blocked) {
        const CarViewAuthoredCamera* authored=carViewCameraResourcesRegionAt(environment->resources,(size_t)region);
        if (!authored) return CAR_VIEW_MOVER_MISSING_RESOURCE;
        ++mover->state.car.resourceRevision;
        CarViewChild child{};
        CarViewMoverResult result=coordinatorCreateAuthored(mover,authored,&child);
        if (result!=CAR_VIEW_MOVER_OK) return result;
        CarCinematicBase* base=carViewChildBase(&child);
        carCinematicBaseDeactivate(base);
        result=carViewChildAdvance(&child);
        if (result!=CAR_VIEW_MOVER_OK) { carViewChildDispose(&child);return result; }
        int valid=!coordinatorCarNearEye(environment->simulation->world,base->view->current.eye,5.0f);
        if (carViewCrossesNearPlane(base->view,car,environment->settings->projectionAngle)) valid=0;
        if (valid) { mover->child=child;carCinematicBaseActivate(base); }
        else { carViewChildDispose(&child);blocked=1; }
    }
    if (region==-1 || blocked) {
        region=-1;
        const CarChaseCameraVehicle* vehicle=coordinatorVehicleForMovement(environment,car);
        if (!vehicle || !vehicle->catalogue) return CAR_VIEW_MOVER_INVALID_BINDING;
        const int layout=environment->services->presentationLayout==3;
        const int32_t count=vehicle->catalogue->viewCount[layout];
        int32_t selected=count>0?(int32_t)vectorDefaultRandomBounded((uint32_t)count):0;
        if (selected==mover->state.car.viewIndex && count>0) selected=(selected+1)%count;
        mover->state.car.viewIndex=selected;mover->state.car.rearFacing=!mover->state.car.rearFacing;
        CarChaseCameraContext context=coordinatorChaseContext(mover,car,selected,1,mover->state.car.rearFacing,300.0f,0);
        CarCinematicResult result;
        mover->child.owner.chase=carChaseCameraCreate(&context,&result);
        if (!mover->child.owner.chase) return CAR_VIEW_MOVER_CAMERA_FAILURE;
        mover->child.kind=CAR_VIEW_CHILD_CHASE;
    }
    mover->state.car.regionIndex=region;mover->state.car.lastSelectionFrame=now;
    return CAR_VIEW_MOVER_OK;
}

CarViewMoverResult carViewMoverUpdateCountdown(CarViewMover* mover)
{
    const CarViewMoverEnvironment* environment=&mover->registry->environment;
    const int32_t stage=raceFrameCountdownStep(environment->simulation->race,environment->race->specialPursuit);
    if (stage==mover->state.countdown.stage) return CAR_VIEW_MOVER_OK;
    carViewChildDispose(&mover->child);
    char name[32];(void)snprintf(name,sizeof(name),"COUNTDOWN_%d",stage);
    const CarViewAuthoredCamera* camera=carViewCameraResourcesFind(environment->resources,name);
    if (camera) {
        CarViewMoverResult result=coordinatorCreateBumper(mover,mover->state.countdown.player->car,camera,&mover->child);
        if (result!=CAR_VIEW_MOVER_OK) return result;
    }
    mover->state.countdown.stage=stage;return CAR_VIEW_MOVER_OK;
}

CarViewMover* carViewMoverCreateForReplay(CarViewMoverRegistry* registry,int32_t viewNumber,
    int32_t groupIndex,CarMovement* firstCar,CarMovement* secondCar,int32_t takeIndex,
    float timeOffset,int32_t startWorldTicks,CarViewMoverResult* result)
{
    if (result) *result=CAR_VIEW_MOVER_INVALID_BINDING;
    if (!registry || !firstCar || groupIndex<0 || groupIndex>1 || takeIndex<0 ||
        !carViewMoverPresentation(registry,firstCar->bindings.owner) ||
        (secondCar && !carViewMoverPresentation(registry,secondCar->bindings.owner))) return NULL;
    CarViewMover* mover=carViewMoverAllocate(registry,viewNumber,CAR_VIEW_MOVER_REPLAY);
    if (!mover) { if (result) *result=CAR_VIEW_MOVER_ALLOCATION_FAILURE;return NULL; }
    mover->state.replay.group=groupIndex;
    mover->state.replay.first=firstCar;mover->state.replay.second=secondCar;
    const CarViewMoverEnvironment* environment=&registry->environment;
    const size_t count=carViewCameraResourcesTakeCount(environment->resources,groupIndex);
    CarCinematicResult created;
    if (!count) {
        CarChaseCameraContext context=coordinatorChaseContext(mover,firstCar,2,0,0,300.8999938964844f,1);
        mover->child.owner.chase=carChaseCameraCreate(&context,&created);
        if (mover->child.owner.chase) mover->child.kind=CAR_VIEW_CHILD_CHASE;
    } else {
        const size_t index=(size_t)takeIndex<count?(size_t)takeIndex:count-1;
        CarRecordedCameraContext context{};
        context.services=environment->services;context.movement=firstCar;
        context.presentation=carViewMoverPresentation(registry,firstCar->bindings.owner);
        context.take=carViewCameraResourcesTakeAt(environment->resources,groupIndex,index);
        auto lease=retainChildResource(*environment->resources,context.take);
        if(!lease) {
            carViewMoverDispose(mover);if(result)*result=CAR_VIEW_MOVER_ALLOCATION_FAILURE;return NULL;
        }
        context.settings=environment->recordedSettings;context.viewNumber=viewNumber;
        context.startWorldTicks=startWorldTicks;context.timeOffset=-timeOffset;
        mover->child.owner.recorded=carRecordedCameraCreate(&context,&created);
        if (mover->child.owner.recorded) {
            mover->child.kind=CAR_VIEW_CHILD_RECORDED;
            mover->child.resourceLease=lease.release();
        }
    }
    if (mover->child.kind==CAR_VIEW_CHILD_NONE) {
        carViewMoverDispose(mover);if (result) *result=CAR_VIEW_MOVER_CAMERA_FAILURE;return NULL;
    }
    if (result) *result=CAR_VIEW_MOVER_OK;
    return mover;
}
