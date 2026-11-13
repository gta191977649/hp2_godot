#include "indep/src/speech/mgr.hpp"
#include "bware/inc/bMath.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace {
// PAL20F820: select within the remaining suffix, swap its first element,
// then advance. This consumes the existing bMath RNG, not another seed.
void ShuffleArray(uint32_t* values,uint32_t count)
{
    while (count>=2) {
        const uint32_t selected=vectorDefaultRandomBounded(count);
        std::swap(values[0],values[selected]);
        ++values;
        --count;
    }
}
}

// PAL20F888: cop identifiers and address forms have different permutations.
void worldSpeechShuffleCopIds(WorldSpeechIdentityMap* identities)
{
    if (!identities)throw std::invalid_argument("Speech identity-map owner is absent");
    constexpr std::array<uint32_t,4> cop{{2,4,8,32}};
    std::copy(cop.begin(),cop.end(),identities->copIds);
    ShuffleArray(identities->copIds,4);
    std::array<uint32_t,6> address{{1,2,4,8,16,32}};
    ShuffleArray(address.data(),static_cast<uint32_t>(address.size()));
    std::copy_n(address.begin(),4,identities->fromCopIds);
    std::copy_n(address.begin(),4,identities->toIds);
}

// PAL20F970..20FA8C: action calls resolve identifiers through the
// retained world speech context and its lookup tables.
uint32_t WorldSpeechIdView::CopId(const WorldSpeechContext& context) const
{ return context.fromId<4 ? map.copIds[context.fromId] : 0; }
uint32_t WorldSpeechIdView::TargetCopId(const WorldSpeechContext& context) const
{ return context.toId<4 ? map.copIds[context.toId] : 0; }
uint32_t WorldSpeechIdView::FromCopId(const WorldSpeechContext& context) const
{ return context.fromId<4 ? map.fromCopIds[context.fromId] : 0; }
uint32_t WorldSpeechIdView::TargetFromCopId(const WorldSpeechContext& context) const
{ return context.toId<4 ? map.fromCopIds[context.toId] : 0; }
uint32_t WorldSpeechIdView::ToId(const WorldSpeechContext& context) const
{ return context.fromId<4 ? map.toIds[context.fromId] : 0; }
uint32_t WorldSpeechIdView::TargetToId(const WorldSpeechContext& context) const
{ return context.toId<4 ? map.toIds[context.toId] : 0; }
uint32_t WorldSpeechIdView::Supervisor(const WorldSpeechContext& context) const
{ return context.variant!=0; }

// The remaining released context accessors read only the existing semantic
// fields populated by the individually proved Raise producers.
int32_t worldSpeechGetArrestType(const WorldSpeechContext* context)
{ return context->arrestType; }
int32_t worldSpeechGetExactVehicle(const WorldSpeechContext* context)
{ return context->exactVehicle; }
int32_t worldSpeechGetApproximateVehicle(const WorldSpeechContext* context)
{ return context->approximateVehicle; }
int32_t worldSpeechGetVehicleColor(const WorldSpeechContext* context)
{ return context->vehicleColor; }
int32_t worldSpeechGetRaceType(const WorldSpeechContext* context)
{ return context->racingVariant; }
