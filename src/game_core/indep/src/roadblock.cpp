#include "indep/inc/Pursuit.hpp"
#include "indep/inc/Car.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/CarInfo.hpp"

#include <cmath>
#include <cstring>
#include <limits>

// Actual source tuning at GP-39E0; the authored initial value is 3.0f.
float pursuitRoadBlockFillWidth=3.0f;

// PAL250068..250084. Source does not clear unused queue payload words.
PursuitWordQueue::PursuitWordQueue() noexcept : read_(0),write_(0) {}

// PAL2500B0..2500FC. A full queue leaves storage and both indices intact.
bool PursuitWordQueue::Store(uint32_t value) noexcept
{
    const uint32_t next=(write_+1u)%30u;
    if(next==read_)return false;
    words_[write_]=value;write_=next;return true;
}

// PAL250100..250150. Empty removal leaves the caller's output unchanged.
bool PursuitWordQueue::Remove(uint32_t& value) noexcept
{
    if(read_==write_)return false;
    value=words_[read_];read_=(read_+1u)%30u;return true;
}

bool PursuitWordQueue::CanStore(size_t count) const noexcept
{
    // One of the 30 cells is reserved to distinguish empty from full.
    return count<=static_cast<size_t>((read_+29u-write_)%30u);
}

// PAL24FAE8..24FB50. The original reads the manager cache, not the
// target car's dimensions or a fabricated sample hull.
float pursuitProjectedCopWidth(const PursuitManager* manager,uint16_t heading)
{
    const float cosine=std::fabs(vectorCosineAngle(heading));
    const float sine=std::fabs(rotationSineAngle(heading));
    const float longitudinal=bScalarMultiply(cosine,manager->copDimensions[1]);
    const float lateral=bScalarMultiply(sine,manager->copDimensions[0]);
    return bScalarAdd(longitudinal,lateral);
}

namespace {
uint32_t floatWord(float value)
{
    uint32_t word;std::memcpy(&word,&value,sizeof(word));return word;
}
}

// PAL24FB78..250068. This produces the actual typed layout command words.
// Element numbers0..6 and all arithmetic come from the two final jump
// tables. The explicit span validates authored storage, not a new sentinel.
// Preserve each EE operation before the width/count decisions: the same
// rounded gap and barrel half-span feed the real spike and officer placement.
WorldServicesResult pursuitBuildRoadBlockLayout(PursuitWordQueue& queue,
    const int32_t* elements,size_t elementCount,const PursuitRoadBlock* block,
    const PursuitManager* manager,float spacing)
{
    if(!elements || !elementCount || !block || !block->roadPose || !manager)
        return WORLD_SERVICES_INVALID_BINDING;
    constexpr uint16_t orientation=0x2000u; // Actual250150 static producer.
    const float projected=pursuitProjectedCopWidth(manager,orientation);
    float used=0.0f,leftSpace=0.0f,rightSpace=0.0f;
    int32_t spaces=0,barrelGroups=0,cars=0,barrels=8,fillCount=0;
    bool fill=false;size_t index=0;
    for(;index<elementCount && elements[index]!=6;++index) {
        switch(elements[index]) {
        case 0:leftSpace=spacing;used=bScalarAdd(used,spacing);++spaces;break;
        case 1:rightSpace=spacing;used=bScalarAdd(used,spacing);++spaces;break;
        case 2:used=bScalarAdd(used,bScalarMultiply(8.0f,0.6499999761581421f));++barrelGroups;break;
        case 3:used=bScalarAdd(used,projected);++cars;break;
        case 4:used=bScalarAdd(used,0.800000011920929f);break;
        case 5:fill=true;break;
        }
    }
    if(index==elementCount)return WORLD_SERVICES_INVALID_BINDING;
    if(projected<bScalarAdd(used,-block->width) && cars>=2){used=bScalarAdd(used,-projected);--cars;}
    // Complete PickBestCopOrientation24FB50 always writes2000 and returns
    // that projection; its count/available-width arguments are unused.
    const float selected=pursuitProjectedCopWidth(manager,orientation);
    used=bScalarAdd(used,bScalarMultiply(static_cast<float>(cars),bScalarAdd(selected,-projected)));
    if(fill && pursuitRoadBlockFillWidth<bScalarAdd(block->width,-used)) {
        const float count=bScalarDivide(bScalarAdd(block->width,-used),pursuitRoadBlockFillWidth);
        if(!std::isfinite(count) || count<0.0f || count>=2147483648.0f)
            return WORLD_SERVICES_INVALID_BINDING;
        fillCount=static_cast<int32_t>(count);
        if(fillCount>10)fillCount=10;
        used=bScalarAdd(used,bScalarMultiply(static_cast<float>(fillCount),pursuitRoadBlockFillWidth));
    }
    if(barrelGroups) {
        const float extra=bScalarMultiply(bScalarAdd(block->width,-used),1.5384615659713745f);
        if(!std::isfinite(extra) || extra< -2147483648.0f || extra>=2147483648.0f)
            return WORLD_SERVICES_INVALID_BINDING;
        const int32_t integer=static_cast<int32_t>(extra);
        if(integer) {
            barrels=integer+8;
            if(barrels<3*barrelGroups)barrels=3*barrelGroups;
            if(20*barrelGroups<barrels)barrels=20*barrelGroups;
            used=bScalarAdd(used,bScalarMultiply(static_cast<float>(barrels-8),0.6499999761581421f));
        }
    }
    // Real patterns provide a spacing element. Zero would be an invalid
    // authored division domain, rather than a fabricated success layout.
    if(!spaces)return WORLD_SERVICES_INVALID_BINDING;
    const float remaining=bScalarDivide(bScalarAdd(block->width,-used),static_cast<float>(spaces));
    if(leftSpace!=0.0f && rightSpace!=0.0f) {
        leftSpace=bScalarAdd(leftSpace,remaining);rightSpace=bScalarAdd(rightSpace,remaining);
        float imbalance=bScalarAdd(block->roadPose->halfWidths[0],-block->roadPose->halfWidths[1]);
        if(0.0f<imbalance) {
            if(imbalance<rightSpace){leftSpace=bScalarAdd(leftSpace,imbalance);rightSpace=bScalarAdd(rightSpace,-imbalance);}
            else rightSpace=0.0f;
        } else {
            imbalance=std::fabs(imbalance);
            if(imbalance<leftSpace){rightSpace=bScalarAdd(rightSpace,imbalance);leftSpace=bScalarAdd(leftSpace,-imbalance);}
            else {leftSpace=0.0f;rightSpace=bScalarAdd(rightSpace,rightSpace);}
        }
    } else {leftSpace=bScalarAdd(leftSpace,remaining);rightSpace=bScalarAdd(rightSpace,remaining);}
    // The source emits typed pairs into a fixed 29-word ring; Store returns
    // false on full. Count the exact emitted words before mutating the queue.
    // All original valid patterns take the identical emission path below.
    size_t wordsNeeded=0;
    int32_t carsToWrite=cars;
    for(size_t at=0;at<index;++at) {
        switch(elements[at]) {
        case 0:case 1:case 2:wordsNeeded+=2;break;
        case 3:if(carsToWrite>0){wordsNeeded+=2;--carsToWrite;}break;
        case 4:++wordsNeeded;break;
        case 5:wordsNeeded+=static_cast<size_t>(fillCount);break;
        }
        if(wordsNeeded>29u)return WORLD_SERVICES_CAPACITY;
    }
    if(!queue.CanStore(wordsNeeded))return WORLD_SERVICES_CAPACITY;
    for(index=0;index<elementCount && elements[index]!=6;) {
        const uint32_t element=static_cast<uint32_t>(elements[index]);
        switch(element) {
        case 0:queue.Store(element);queue.Store(floatWord(leftSpace));++index;break;
        case 1:queue.Store(element);queue.Store(floatWord(rightSpace));++index;break;
        case 2: {
            if(!barrelGroups)return WORLD_SERVICES_INVALID_BINDING;
            queue.Store(element);
            const int32_t count=barrels/barrelGroups;
            --barrelGroups;barrels-=count;queue.Store(static_cast<uint32_t>(count));++index;break;
        }
        case 3:
            if(cars){queue.Store(element);queue.Store(orientation);--cars;}
            ++index;break;
        case 4:queue.Store(element);++index;break;
        case 5:
            // Source increments the authored index once before its repeat
            // loop; only the local emitted-filler counter then advances.
            for(int32_t i=0;i<fillCount;++i)queue.Store(element);
            ++index;
            break;
        default:++index;break;
        }
    }
    return index<elementCount ? WORLD_SERVICES_OK : WORLD_SERVICES_INVALID_BINDING;
}
