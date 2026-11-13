#ifndef NFSHP2_CLANS_HPP
#define NFSHP2_CLANS_HPP

// Individual path evidence: ExpandBoundingBox beta1A8B90 -> PAL1AB510,
// full PDR/N_SOL definition ../indep/inc/Clans.hpp; names only from beta.
#include "indep/inc/Skids.hpp"
#include "indep/inc/Timer.hpp"
#include "indep/inc/Ecstasy.hpp"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

struct WheelTrailClanOwner;
struct WheelTrailClan {
    WheelTrailClanOwner* owner{};
    WheelTrailClan* previous{};
    WheelTrailClan* next{};
    WheelTrail* first{};
    WheelTrail* last{};
    bool occupied{};
    std::uint32_t key{};
    std::int32_t lastTouched{};
    float position[3]{};
    float minimum[3]{};
    float maximum[3]{};
};

namespace hp2 {
struct SkidRenderVertex {
    std::array<float,4> position{};
    std::array<float,3> textureCoordinate{};
    std::uint32_t color{};
};
struct SkidRenderQuad {
    std::array<SkidRenderVertex,4> vertices{};
    const EditorMarkerTexture* texture{};
    std::array<std::uint32_t,4> rasterState{};
    std::size_t trail{};
    std::uint64_t revision{};
    std::uint8_t fade{};
};
// Init/Flush/CloseClans1AB618..1AB6D0. Bind after SkidResources; release
// before SkidResources and after Cars. Groups own actual SkidSet membership.
class ClanResources final {
public:
    static constexpr std::size_t Capacity=40;
    explicit ClanResources(const GameTime& clock);
    ClanResources(SkidResources& skids,const GameTime& clock);
    ~ClanResources();
    ClanResources(const ClanResources&)=delete;
    ClanResources& operator=(const ClanResources&)=delete;
    void BindSkids(SkidResources& skids);
    void Flush() noexcept;
    WheelTrailClan* Get(const float position[3]);
    const WheelTrailClan* Find(const float position[3]) const noexcept;
    std::size_t Count() const noexcept;
    std::size_t TrailCount(const WheelTrailClan& clan) const noexcept;
    void Bounds(const WheelTrailClan& clan,float minimum[3],float maximum[3]) const;
    // RenderClans plus RenderSkids visibility/order and source ePoly geometry.
    // DisplayFrame consumes this geometry at the original PAL1132C4 call site.
    void PrepareRender(const float matrix[16],const CarCinematicScreen& screen,
        std::vector<SkidRenderQuad>& output);
private:
    std::unique_ptr<WheelTrailClanOwner> owner_;
};
std::uint32_t ClanPositionKey(const float position[3]) noexcept;
}

// Same-owner integration used by the original SkidSet lifecycle. No callback
// registry, second trail pool, or synthetic success implementation exists.
void wheelTrailClanAttach(WheelTrailStore* store,WheelTrail* trail,const float position[3]);
void wheelTrailClanDetach(WheelTrailStore* store,WheelTrail* trail) noexcept;
inline void wheelTrailClanExpand(WheelTrailStore* store,WheelTrail* trail) noexcept
{
    auto* clan=trail->clan;assert(store && store->clans && clan && clan->owner==store->clans);
    (void)store;
    // Clan::ExpandBoundingBox PAL1AB510 -> bUpdateBoundingBox2596D0.
    for(std::size_t axis=0;axis<3;++axis) {
        if(trail->minimum[axis]<clan->minimum[axis])clan->minimum[axis]=trail->minimum[axis];
        if(clan->maximum[axis]<trail->maximum[axis])clan->maximum[axis]=trail->maximum[axis];
    }
}
void wheelTrailPrepareRender(hp2::SkidResources& skids,WheelTrail* first,
    const float matrix[16],const CarCinematicScreen& screen,std::vector<hp2::SkidRenderQuad>& output);

#endif
