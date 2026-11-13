#include "indep/inc/Ecstasy.hpp"
#include "bware/inc/bChunk.hpp"
#include "bware/inc/bWare.hpp"

#include <cmath>
#include <cstring>
#include <stdexcept>
#include <utility>

void positionMarkersInitialize(PositionMarkerRegistry* registry)
{
    if (registry) {
        registry->first=nullptr;
        registry->last=nullptr;
    }
}

int32_t positionMarkersLoad(PositionMarkerRegistry* registry,uint32_t chunkKind,
    PositionMarker* records,size_t count)
{
    if (chunkKind!=0x34013u)return 0;
    if (!registry || (count && !records))return 0;
    for (size_t index=0;index<count;++index)
        if (records[index].registry)return 0;
    for (size_t index=0;index<count;++index) {
        PositionMarker& marker=records[index];
        marker.next=nullptr;
        marker.previous=registry->last;
        if (registry->last)registry->last->next=&marker;
        else registry->first=&marker;
        registry->last=&marker;
        marker.registry=registry;
    }
    return 1;
}

int32_t positionMarkersUnload(PositionMarkerRegistry* registry,uint32_t chunkKind,
    PositionMarker* records,size_t count)
{
    if (chunkKind!=0x34013u)return 0;
    if (!registry || (count && !records))return 0;
    for (size_t index=0;index<count;++index)
        if (records[index].registry!=registry)return 0;
    // Source walks the file's records in forward order and unlinks each
    // actual node. Other loaded catalogues keep their relative order.
    for (size_t index=0;index<count;++index) {
        PositionMarker& marker=records[index];
        if (marker.previous)marker.previous->next=marker.next;
        else registry->first=marker.next;
        if (marker.next)marker.next->previous=marker.previous;
        else registry->last=marker.previous;
        marker.next=marker.previous=nullptr;
        marker.registry=nullptr;
    }
    return 1;
}

const PositionMarker* positionMarkerFind(const PositionMarkerRegistry* registry,
    uint32_t key,uint32_t groupKey,const PositionMarker* previous)
{
    if (!key && !groupKey)return nullptr;
    if (!registry || (previous && previous->registry!=registry))return nullptr;
    const PositionMarker* marker=previous ? previous->next:registry->first;
    for (;marker;marker=marker->next) {
        if (!key) {
            if (marker->groupKey==groupKey)return marker;
        } else if (marker->key==key && (!groupKey || marker->groupKey==groupKey)) {
            return marker;
        }
    }
    return nullptr;
}

namespace hp2 {

PositionMarkerResources::PositionMarkerResources(const std::vector<std::uint8_t>& bundle,
    PositionMarkerRegistry& registry)
    : registry_(registry),authored_(bundle)
{
    const auto visit=[&](const auto& self,std::size_t begin,std::size_t size,unsigned depth)->void {
        Require(depth<=32,"Position marker resource nesting exceeds32 levels");
        for (const auto& chunk:ReadChunks(authored_,begin,size)) {
            if (chunk.id==0x34013u) {
                const std::size_t start=(chunk.payloadOffset+15u)&~std::size_t(15u);
                Require(start<=chunk.endOffset,"Position marker alignment exceeds chunk extent");
                Block block;
                // LoaderPositionMarkers subtracts the alignment gap and
                // divides by32. Bytes beyond the last full record are not
                // semantic markers and are never indexed by the source.
                block.count=(chunk.endOffset-start)/32u;
                block.records=std::make_unique<PositionMarker[]>(block.count);
                for (std::size_t index=0;index<block.count;++index) {
                    PositionMarker& marker=block.records[index];
                    const auto* record=authored_.data()+start+32u*index;
                    marker.key=ReadU32(record+8);
                    marker.groupKey=ReadU32(record+12);
                    for (int axis=0;axis<3;++axis) {
                        const uint32_t bits=ReadU32(record+16+4u*axis);
                        std::memcpy(&marker.position[axis],&bits,sizeof(bits));
                        Require(std::isfinite(marker.position[axis]),"Position marker coordinate is not finite");
                    }
                    marker.tags=ReadU32(record+28);
                    marker.authoredRecord=record;
                }
                blocks_.push_back(std::move(block));
            } else if (chunk.IsContainer()) {
                self(self,chunk.payloadOffset,chunk.size,depth+1);
            }
        }
    };
    visit(visit,0,authored_.size(),0);
    try {
        for (auto& block:blocks_)
            Require(positionMarkersLoad(&registry_,0x34013u,block.records.get(),block.count)!=0,
                "Position marker block is already registered");
    } catch (...) {
        for (auto& block:blocks_)
            if (block.count && block.records[0].registry==&registry_)
                positionMarkersUnload(&registry_,0x34013u,block.records.get(),block.count);
        throw;
    }
}

PositionMarkerResources::~PositionMarkerResources()
{
    for (auto& block:blocks_)
        positionMarkersUnload(&registry_,0x34013u,block.records.get(),block.count);
}

std::size_t PositionMarkerResources::Count() const noexcept
{
    std::size_t count=0;
    for (const auto& block:blocks_)count+=block.count;
    return count;
}

const PositionMarker* PositionMarkerResources::At(std::size_t index) const noexcept
{
    for (const auto& block:blocks_) {
        if (index<block.count)return &block.records[index];
        index-=block.count;
    }
    return nullptr;
}

}
