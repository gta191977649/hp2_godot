#include "indep/inc/Clans.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bWare.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>

// Names/path: beta Clans.obj and individual PDRs. All branches, limits and
// clocks below follow final PAL1AB618..1AB9F0; native pointers own no PS2 state.
struct WheelTrailClanOwner {
    hp2::SkidResources* skids{};
    const GameTime* clock{};
    std::array<WheelTrailClan,hp2::ClanResources::Capacity> slots{};
    WheelTrailClan* first{};
    WheelTrailClan* last{};
    std::size_t count{};
};

namespace {
void removeClan(WheelTrailClanOwner& owner,WheelTrailClan& clan) noexcept
{
    if(clan.previous)clan.previous->next=clan.next;else owner.first=clan.next;
    if(clan.next)clan.next->previous=clan.previous;else owner.last=clan.previous;
    clan.next=nullptr;clan.previous=nullptr;
}
void prependClan(WheelTrailClanOwner& owner,WheelTrailClan& clan) noexcept
{
    clan.previous=nullptr;clan.next=owner.first;
    if(owner.first)owner.first->previous=&clan;else owner.last=&clan;
    owner.first=&clan;
}
void destroyClan(WheelTrailClanOwner& owner,WheelTrailClan& clan) noexcept
{
    removeClan(owner,clan);
    while(clan.first)wheelTrailDelete(&owner.skids->Trails(),clan.first);
    clan.occupied=false;--owner.count;
}
WheelTrailClan* getClan(WheelTrailClanOwner& owner,const float position[3])
{
    const auto key=hp2::ClanPositionKey(position);
    for(auto* clan=owner.first;clan;clan=clan->next)if(clan->key==key) {
        removeClan(owner,*clan);prependClan(owner,*clan);
        clan->lastTouched=owner.clock->worldLoopCounter;return clan;
    }
    if(owner.count==owner.slots.size())destroyClan(owner,*owner.last);
    for(auto& clan:owner.slots)if(!clan.occupied) {
        // GetClan calls bOMalloc before the constructor. Its default pool
        // flags clear the complete slot on every allocation, including reuse.
        // Keep that allocation boundary separate from the constructor fields.
        clan=WheelTrailClan{};
        clan.owner=&owner;clan.occupied=true;clan.key=key;
        std::copy_n(position,3,clan.position);
        std::copy_n(position,3,clan.minimum);std::copy_n(position,3,clan.maximum);
        prependClan(owner,clan);++owner.count;return &clan;
    }
    throw std::logic_error("ClanSlotPool occupancy is corrupt");
}
}

namespace hp2 {
std::uint32_t ClanPositionKey(const float position[3]) noexcept
{
    const auto x=convertFloatToInt32(position[0]*65536.0f);
    const auto y=convertFloatToInt32(position[1]*65536.0f);
    // Explicit arithmetic shift of source signed words, independent of host
    // signed-shift rules. The final key has low16 X and high16 Y.
    const auto cell=[](std::int32_t v) {
        const auto bits=static_cast<std::uint32_t>(v);
        return (bits>>22)|(v<0?0xfffffc00u:0u);
    };
    return (cell(x)&0xffffu)|(cell(y)<<16);
}
ClanResources::ClanResources(const GameTime& clock)
    :owner_(std::make_unique<WheelTrailClanOwner>())
{
    owner_->clock=&clock;
}
ClanResources::ClanResources(SkidResources& skids,const GameTime& clock):ClanResources(clock)
{
    BindSkids(skids);
}
void ClanResources::BindSkids(SkidResources& skids)
{
    Require(!owner_->skids,"ClanResources already has its actual Skid owner");
    Require(skids.Trails().clans==nullptr,"SkidResources already has a Clan owner");
    // InitClans owns40 slots. World construction binds it before any trails.
    for(std::size_t i=0;i<skids.Trails().capacity;++i)
        Require(skids.Trails().storage[i].creation==0,"Clans must bind before SkidSet creation");
    owner_->skids=&skids;
    skids.Trails().clans=owner_.get();
}
ClanResources::~ClanResources()
{
    Flush();if(owner_->skids)owner_->skids->Trails().clans=nullptr;
}
void ClanResources::Flush() noexcept
{
    while(owner_->first)destroyClan(*owner_,*owner_->first);
}
WheelTrailClan* ClanResources::Get(const float position[3])
{Require(owner_->skids,"GetClan requires the actual initialized Skids pool");return getClan(*owner_,position);}
const WheelTrailClan* ClanResources::Find(const float position[3]) const noexcept
{
    const auto key=ClanPositionKey(position);
    for(auto* clan=owner_->first;clan;clan=clan->next)if(clan->key==key)return clan;
    return nullptr;
}
std::size_t ClanResources::Count() const noexcept {return owner_->count;}
std::size_t ClanResources::TrailCount(const WheelTrailClan& clan) const noexcept
{
    assert(clan.owner==owner_.get() && clan.occupied);
    std::size_t count=0;for(auto* trail=clan.first;trail;trail=trail->clanNext)++count;return count;
}
void ClanResources::Bounds(const WheelTrailClan& clan,float minimum[3],float maximum[3]) const
{
    Require(clan.owner==owner_.get() && clan.occupied,"Clan belongs to another or released World");
    std::copy_n(clan.minimum,3,minimum);std::copy_n(clan.maximum,3,maximum);
}
void ClanResources::PrepareRender(const float matrix[16],const CarCinematicScreen& screen,
    std::vector<SkidRenderQuad>& output)
{
    Require(matrix && screen.view,"RenderClans requires the live view and clip matrix");
    for(auto* clan=owner_->first;clan;) {
        // Save the next original node: visible groups may move to list head.
        auto* next=clan->next;
        if(ecstasyViewBoundsPixelSize(&screen,clan->minimum,clan->maximum)>=11 &&
           ecstasyClassifyBounds(clan->minimum,clan->maximum,matrix)!=0) {
            const std::uint32_t elapsedBits=static_cast<std::uint32_t>(owner_->clock->worldLoopCounter)-
                static_cast<std::uint32_t>(clan->lastTouched);
            std::int32_t elapsed;std::memcpy(&elapsed,&elapsedBits,sizeof(elapsed));
            if(elapsed>=301) {
                clan->lastTouched=owner_->clock->worldLoopCounter;
                removeClan(*owner_,*clan);prependClan(*owner_,*clan);
            }
            wheelTrailPrepareRender(*owner_->skids,clan->first,matrix,screen,output);
        }
        clan=next;
    }
}
}

void wheelTrailClanAttach(WheelTrailStore* store,WheelTrail* trail,const float position[3])
{
    assert(store && store->clans && trail && !trail->clan);
    auto* clan=getClan(*store->clans,position);
    trail->clan=clan;trail->clanPrevious=nullptr;trail->clanNext=clan->first;
    if(clan->first)clan->first->clanPrevious=trail;else clan->last=trail;
    clan->first=trail;
}
void wheelTrailClanDetach(WheelTrailStore* store,WheelTrail* trail) noexcept
{
    auto* clan=trail->clan;
    if(!clan)return;
    assert(store && store->clans && clan->owner==store->clans);
    (void)store;
    if(trail->clanPrevious)trail->clanPrevious->clanNext=trail->clanNext;
    else clan->first=trail->clanNext;
    if(trail->clanNext)trail->clanNext->clanPrevious=trail->clanPrevious;
    else clan->last=trail->clanPrevious;
    trail->clan=nullptr;trail->clanNext=nullptr;trail->clanPrevious=nullptr;
}
