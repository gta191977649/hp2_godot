
#include "indep/inc/EasterEggs.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/WeatherMan.hpp"
#include "indep/inc/AnimatedDriver.hpp"
#include "indep/inc/CarInfo.hpp"

#include <array>
#include <cstring>
#include <memory>
#include <stdexcept>

namespace {
std::unique_ptr<Stomper> currentStomper;
// PAL mode 8 descriptor 0x31D410 consists of exactly one IntegerCallback
// leaf. 0x10F1F8 and beta RadialBlurOnOff__Fbi agree on the MIPS body.
// The target is the native camera owner, never the original EE pointer.
ScreenRadialBlur* activeRadialBlur{};
std::int32_t PalRadialBlurOnOff(std::int32_t enabled,std::int32_t seat)
{
    if(enabled && !activeRadialBlur)
        throw std::runtime_error("Radial render egg has no active track view owner");
    if (activeRadialBlur) {
        if (enabled) screenRadialBlurStart(activeRadialBlur,seat);
        else screenRadialBlurStop(activeRadialBlur,seat);
    }
    return seat; // The original 0x10F1F8 returns its second argument.
}
const StompEntry palRadialMode8[]={{StompKind::IntegerCallback,nullptr,0,nullptr,&PalRadialBlurOnOff,nullptr},
                                    {StompKind::End}};
// Original PAL31D588/31D618 descriptor leaves, including radial seat0.
// The screen-filter bytes feed the same live current-destination consumer.
const StompEntry palWarmRadialMode6[] = {
    {StompKind::Group,nullptr,0,palRadialMode8},
    {StompKind::Byte,&hp2::nativeRenderPolicy.screenFilters[0].enabled,1},
    {StompKind::Byte,&hp2::nativeRenderPolicy.screenFilters[0].blend[0],1},
    {StompKind::Byte,&hp2::nativeRenderPolicy.screenFilters[0].blend[1],0},
    {StompKind::Byte,&hp2::nativeRenderPolicy.screenFilters[0].blend[2],2},
    {StompKind::Byte,&hp2::nativeRenderPolicy.screenFilters[0].blend[3],1},
    {StompKind::Byte,&hp2::nativeRenderPolicy.screenFilters[0].fixedAlpha,122},
    {StompKind::Byte,&hp2::nativeRenderPolicy.screenFilters[0].color[0],193},
    {StompKind::Byte,&hp2::nativeRenderPolicy.screenFilters[0].color[1],128},
    {StompKind::Byte,&hp2::nativeRenderPolicy.screenFilters[0].color[2],23},
    {StompKind::Byte,&hp2::nativeRenderPolicy.screenFilters[0].color[3],255},
    {StompKind::End}
};
const StompEntry palGreenRadialMode7[] = {
    {StompKind::Group,nullptr,0,palRadialMode8},
    {StompKind::Byte,&hp2::nativeRenderPolicy.screenFilters[0].enabled,1},
    {StompKind::Byte,&hp2::nativeRenderPolicy.screenFilters[0].blend[0],1},
    {StompKind::Byte,&hp2::nativeRenderPolicy.screenFilters[0].blend[1],0},
    {StompKind::Byte,&hp2::nativeRenderPolicy.screenFilters[0].blend[2],2},
    {StompKind::Byte,&hp2::nativeRenderPolicy.screenFilters[0].blend[3],1},
    {StompKind::Byte,&hp2::nativeRenderPolicy.screenFilters[0].fixedAlpha,255},
    {StompKind::Byte,&hp2::nativeRenderPolicy.screenFilters[0].color[0],60},
    {StompKind::Byte,&hp2::nativeRenderPolicy.screenFilters[0].color[1],191},
    {StompKind::Byte,&hp2::nativeRenderPolicy.screenFilters[0].color[2],128},
    {StompKind::Byte,&hp2::nativeRenderPolicy.screenFilters[0].color[3],255},
    {StompKind::End}
};
const StompEntry palBlackAndWhiteMode9[] = {
    {StompKind::Word,&hp2::nativeRenderPolicy.blackAndWhite,1},
    {StompKind::End}
};
// PAL's static constructor fills 31D2E0 with these three leaves. The
// WeatherMan policy is the same live owner consumed by WeatherManQuery;
// a mode must change those inputs, not retain a disconnected cheat flag.
const StompEntry palNegativeFog[] = {
    {StompKind::Word,&weatherManPolicy.fixedOverride,1},
    {StompKind::Float,&weatherManPolicy.fixedFogStrength,0x3ffeb852u},
    {StompKind::Float,&weatherManPolicy.fixedFogDistance,0xc47a0000u},
    {StompKind::End}
};
const StompEntry palNegativeBlueMode3[] = {
    {StompKind::Group,nullptr,0,palNegativeFog},
    {StompKind::Word,&weatherManPolicy.fixedRed,0x40},
    {StompKind::Word,&weatherManPolicy.fixedGreen,0x32},
    {StompKind::Word,&weatherManPolicy.fixedBlue,0x51},
    {StompKind::End}
};
const StompEntry palNegativeCreamMode4[] = {
    {StompKind::Group,nullptr,0,palNegativeFog},
    {StompKind::Word,&weatherManPolicy.fixedRed,0x6e},
    {StompKind::Word,&weatherManPolicy.fixedGreen,0x68},
    {StompKind::Word,&weatherManPolicy.fixedBlue,0x51},
    {StompKind::End}
};
// PAL mode10 writes 330DB8=1. WeatherManQuery 1A84F4 reads that exact
// cell before the ordinary fixed/region path and produces animated fog.
const StompEntry palAnimatedWeatherMode10[] = {
    {StompKind::Word,&weatherManPolicy.animatedOverride,1},
    {StompKind::End}
};
const StompEntry palBigHeadDriver[] = {
    {StompKind::Word,&carDriverVisualForceAnimated,1},
    {StompKind::Word,&carRenderDriverVisibilityOverride,1},
    {StompKind::Word,&hp2::nBoneScale,15},
    {StompKind::Float,&hp2::vBoneScale[0],0x40b00000u},
    {StompKind::Float,&hp2::vBoneScale[1],0x40b00000u},
    {StompKind::Float,&hp2::vBoneScale[2],0x40b00000u},
    {StompKind::End}
};
// PAL31D508: half-size vehicle artwork, with the driver's authored bone15
// magnified5.5 times. Physics, collision shapes and the source skeleton stay
// owned by their existing systems; no guessed bounding-box scale is used.
const StompEntry palSmallCarBigHeadMode11[] = {
    {StompKind::Float,&CarScaleMatrix[0],0x3f000000u},
    {StompKind::Float,&CarScaleMatrix[5],0x3f000000u},
    {StompKind::Float,&CarScaleMatrix[10],0x3f000000u},
    {StompKind::Group,nullptr,0,palBigHeadDriver},
    {StompKind::End}
};

std::int32_t PalClampTopLeft(std::int32_t enabled,std::int32_t)
{
    // Complete PAL108CA0..108CBC (beta eClampTopLeft): the callback
    // returns0, toggles the clamp override, and ignores its second argument.
    // Its region is texels[0,1] on both axes, consumed by the native shader.
    hp2::nativeRenderPolicy.pixelClampOverride=enabled;
    return 0;
}
const StompEntry palClampTopLeft[] = {
    {StompKind::IntegerCallback,nullptr,1,nullptr,&PalClampTopLeft,nullptr},
    {StompKind::End}
};
// PAL's additional331E98 flag has no feature reader: the complete text has
// no GP/address reference, and its sole data pointer is this mode's leaf.
// Exclude that nonfunctional debug write from the native table; retain all
// original vehicle, driver and texture-clamp consumers below.
const StompEntry palCartoonMode1[] = {
    {StompKind::Group,nullptr,0,palSmallCarBigHeadMode11},
    {StompKind::Group,nullptr,0,palClampTopLeft},
    {StompKind::Group,nullptr,0,palBigHeadDriver},
    {StompKind::End}
};

const StompEntry palFlatShaded[] = {
    {StompKind::Word,&hp2::nativeRenderPolicy.texturing,0},
    {StompKind::End}
};
const StompEntry palWireframe[] = {
    {StompKind::Word,&hp2::nativeRenderPolicy.drawSky,0},
    {StompKind::Word,&hp2::nativeRenderPolicy.wireframe,1},
    {StompKind::End}
};
const StompEntry palNegativeOrange[] = {
    {StompKind::Group,nullptr,0,palNegativeFog},
    {StompKind::Word,&weatherManPolicy.fixedRed,0x32},
    {StompKind::Word,&weatherManPolicy.fixedGreen,0x20},
    {StompKind::Word,&weatherManPolicy.fixedBlue,0},
    {StompKind::End}
};
const StompEntry palFlatWireOrangeMode2[] = {
    {StompKind::Group,nullptr,0,palFlatShaded},
    {StompKind::Group,nullptr,0,palWireframe},
    {StompKind::Group,nullptr,0,palNegativeOrange},
    {StompKind::End}
};
const StompEntry palFlatOrangeMode5[] = {
    {StompKind::Group,nullptr,0,palFlatShaded},
    {StompKind::Group,nullptr,0,palNegativeOrange},
    {StompKind::End}
};
const StompEntry palReflectionBlendMode12[] = {
    {StompKind::Word,&hp2::nativeRenderPolicy.reflectionBlend[0],2},
    {StompKind::Word,&hp2::nativeRenderPolicy.reflectionBlend[1],2},
    {StompKind::Word,&hp2::nativeRenderPolicy.reflectionBlend[2],1},
    {StompKind::Word,&hp2::nativeRenderPolicy.reflectionBlend[3],0},
    {StompKind::Word,&hp2::nativeRenderPolicy.reflectionBlend[4],128},
    {StompKind::End}
};

void ValidateEntries(const StompEntry* entries,std::size_t& leaves,std::size_t depth)
{
    if(!entries || depth>16)
        throw std::runtime_error("Original Stomper group has no valid source data");
    // PAL uses kind=7 to terminate StompEntry arrays. Avoid native unbounded
    // reads in malformed tables while respecting the original 0x400-byte save.
    for(std::size_t index=0;index<=Stomper::OriginalSaveBytes/4;++index) {
        const StompEntry& entry=entries[index];
        if(entry.kind==StompKind::End)return;
        if(entry.kind==StompKind::Group) {
            ValidateEntries(entry.group,leaves,depth+1);
            continue;
        }
        if(static_cast<std::uint32_t>(entry.kind)>5 ||
            (entry.kind<=StompKind::Float && !entry.target) ||
            (entry.kind==StompKind::IntegerCallback && !entry.integerFn) ||
            (entry.kind==StompKind::FloatCallback && !entry.floatFn) ||
            ++leaves>Stomper::OriginalSaveBytes/4)
            throw std::runtime_error("Original Stomper entry is invalid or overflows its 1024-byte save");
    }
    throw std::runtime_error("Original Stomper entry group lacks a terminal entry");
}

float FloatFromBits(std::uint32_t bits) noexcept
{
    float value;std::memcpy(&value,&bits,sizeof(value));return value;
}
std::uint32_t FloatBits(float value) noexcept
{
    std::uint32_t bits;std::memcpy(&bits,&value,sizeof(bits));return bits;
}
}

std::size_t StompEntry::Set(std::uint32_t* save,std::size_t capacity) const
{
    if(kind==StompKind::Group)return SetAll(group,save,capacity);
    if(kind==StompKind::End)return 0;
    if(capacity==0)throw std::runtime_error("Stomper save capacity exceeded");
    switch(kind) {
    case StompKind::Byte: {
        std::uint8_t previous;std::memcpy(&previous,target,sizeof(previous));
        *save=previous;
        const auto authored=static_cast<std::uint8_t>(valueBits);
        std::memcpy(target,&authored,sizeof(authored));break;
    }
    case StompKind::Half: {
        std::uint16_t previous;std::memcpy(&previous,target,sizeof(previous));
        *save=previous;
        const auto authored=static_cast<std::uint16_t>(valueBits);
        std::memcpy(target,&authored,sizeof(authored));break;
    }
    case StompKind::Word:
    case StompKind::Float:
        std::memcpy(save,target,sizeof(*save));
        std::memcpy(target,&valueBits,sizeof(valueBits));break;
    case StompKind::IntegerCallback:
        *save=static_cast<std::uint32_t>(integerFn(1,static_cast<std::int32_t>(valueBits)));break;
    case StompKind::FloatCallback:
        *save=FloatBits(floatFn(1,FloatFromBits(valueBits)));break;
    default:throw std::runtime_error("Unknown original Stomper entry kind");
    }
    return 1;
}

std::size_t StompEntry::Clear(const std::uint32_t* save,std::size_t capacity) const
{
    if(kind==StompKind::Group)return ClearAll(group,save,capacity);
    if(kind==StompKind::End)return 0;
    if(capacity==0)throw std::runtime_error("Stomper restore capacity exceeded");
    switch(kind) {
    case StompKind::Byte: {
        const auto previous=static_cast<std::uint8_t>(*save);
        std::memcpy(target,&previous,sizeof(previous));break;
    }
    case StompKind::Half: {
        const auto previous=static_cast<std::uint16_t>(*save);
        std::memcpy(target,&previous,sizeof(previous));break;
    }
    case StompKind::Word:
    case StompKind::Float:std::memcpy(target,save,sizeof(*save));break;
    case StompKind::IntegerCallback:(void)integerFn(0,static_cast<std::int32_t>(*save));break;
    case StompKind::FloatCallback:(void)floatFn(0,FloatFromBits(*save));break;
    default:throw std::runtime_error("Unknown original Stomper entry kind");
    }
    return 1;
}

std::size_t StompEntry::SetAll(const StompEntry* entries,std::uint32_t* save,std::size_t capacity)
{
    // Each callback/parameter is part of one PAL graphics-mode transition.
    // A resource failure half way through must not leave the renderer with
    // a mixture of two modes. Keep the original ordered leaf writes and
    // saved words; roll back successfully applied leaves in reverse order.
    struct Applied { const StompEntry* entry; std::size_t word; };
    std::array<Applied,Stomper::OriginalSaveBytes/4> applied{};
    std::size_t count=0;
    const auto apply=[&](auto&& visit,const StompEntry* list,std::size_t depth)->void {
        if (!list || depth>16)throw std::runtime_error("Invalid Stomper parameter group");
        for(std::size_t index=0;index<=Stomper::OriginalSaveBytes/4;++index) {
            const auto& entry=list[index];
            if(entry.kind==StompKind::End)return;
            if(entry.kind==StompKind::Group) {visit(visit,entry.group,depth+1);continue;}
            if(count>=capacity || count>=applied.size())
                throw std::runtime_error("Stomper save capacity exceeded");
            // Set() returns the original 32-bit value from callbacks before
            // publishing the leaf as applied. A callback that throws before
            // returning is responsible for not committing its own state.
            const auto used=entry.Set(save+count,capacity-count);
            if(used!=1)throw std::runtime_error("Stomper leaf did not save one word");
            applied[count]=Applied{&entry,count};
            ++count;
        }
        throw std::runtime_error("Stomper parameter group lacks its end marker");
    };
    try {apply(apply,entries,0);}
    catch (...) {
        bool rollbackFailure=false;
        while(count) {
            const auto& leaf=applied[--count];
            try {(void)leaf.entry->Clear(save+leaf.word,capacity-leaf.word);}
            catch (...) {rollbackFailure=true;}
        }
        if (rollbackFailure)
            throw std::runtime_error("Stomper effect failed and renderer parameter rollback was incomplete");
        throw;
    }
    return count;
}

std::size_t StompEntry::ClearAll(const StompEntry* entries,const std::uint32_t* save,std::size_t capacity)
{
    // Explicit PC correction: PAL24BB78 restores forward. Mode1 contains
    // the same group31D4B0 twice, so the second saved intermediate value
    // would overwrite the first restored original. Unwind actual leaf
    // writes in reverse, as SetAll already does when a transaction fails.
    std::size_t count=0;ValidateEntries(entries,count,0);
    if(count>capacity)throw std::runtime_error("Stomper restore capacity exceeded");
    std::array<const StompEntry*,Stomper::OriginalSaveBytes/4> leaves{};
    std::size_t position=0;
    const auto collect=[&](auto&& self,const StompEntry* list)->void {
        for(const auto* entry=list;entry->kind!=StompKind::End;++entry) {
            if(entry->kind==StompKind::Group)self(self,entry->group);
            else leaves[position++]=entry;
        }
    };
    collect(collect,entries);
    while(position) {
        --position;
        (void)leaves[position]->Clear(save+position,capacity-position);
    }
    return count;
}

void Stomper::BindMode(std::uint32_t mode,const StompEntry* entries)
{
    if(mode<1 || mode>FinalModeCount || this->mode_==mode)
        throw std::runtime_error("Cannot replace the active or invalid Stomper mode");
    std::size_t leaves=0;ValidateEntries(entries,leaves,0);
    if(!leaves)throw std::runtime_error("Original Stomper mode has no source render parameters");
    tables_[mode]=entries;
}

void Stomper::Set(std::uint32_t mode)
{
    // PAL 24BD20: ignore mode values outside 1..12. It does not clear an
    // earlier selection; callers changing selections use ClearSet instead.
    if(mode<1 || mode>FinalModeCount)return;
    if(!tables_[mode])throw std::runtime_error("Requested PAL Stomper effect has no native renderer mapping");
    std::size_t leaves=0;ValidateEntries(tables_[mode],leaves,0);
    if(savedWords_ && !suspended_)
        throw std::runtime_error("Stomper::Set requires ClearSet for a live effect");
    savedWords_=StompEntry::SetAll(tables_[mode],saved_.data(),saved_.size());
    mode_=mode;suspended_=false;
}

void Stomper::Clear()
{
    // PAL 24BCD0: only modes 1..12 have a saved entry table; clear mode=0.
    if(mode_>=1 && mode_<=FinalModeCount && !suspended_) {
        if(!tables_[mode_])throw std::runtime_error("PAL Stomper clear has no renderer table");
        StompEntry::ClearAll(tables_[mode_],saved_.data(),savedWords_);
    }
    mode_=0;savedWords_=0;suspended_=false;
}

void Stomper::ClearSet(std::uint32_t mode)
{
    // Preserve the previous effect when the requested PAL renderer mode is
    // unbound or one of its callbacks rejects an allocation. An exception
    // must not silently disable a previously working graphics cheat.
    const auto previous=mode_;
    const bool wasSuspended=suspended_;
    if(mode>=1 && mode<=FinalModeCount) {
        if(!tables_[mode])throw std::runtime_error("Requested PAL Stomper effect has no native renderer mapping");
        std::size_t leaves=0;ValidateEntries(tables_[mode],leaves,0);
    }
    Clear();
    try {Set(mode);}
    catch (...) {
        if(previous && wasSuspended) {mode_=previous;suspended_=true;}
        else if(previous)Set(previous);
        throw;
    }
}

void Stomper::Suspend()
{
    // PAL 24BD78: remember the selected mode, clear underlying render state,
    // then retain the mode ID for a subsequent PAL 24BDB0 Resume.
    const auto selected=mode_;
    if(!suspended_)Clear();
    mode_=selected;suspended_=selected!=0;
}

void Stomper::Resume()
{
    if(!suspended_)return;
    const auto selected=mode_;
    mode_=0;suspended_=false;
    Set(selected);
}

void Stomper::Disable()
{
    // PAL 24C000: Clear THEN set enabled=0, including already inactive modes.
    Clear();enabled_=false;
}

Stomper* GetStomper() noexcept {return currentStomper.get();}
void InitStomper()
{
    if(currentStomper)throw std::runtime_error("InitStomper called twice without CloseStomper");
    currentStomper=std::make_unique<Stomper>();
    currentStomper->BindMode(1,palCartoonMode1);
    currentStomper->BindMode(2,palFlatWireOrangeMode2);
    currentStomper->BindMode(3,palNegativeBlueMode3);
    currentStomper->BindMode(4,palNegativeCreamMode4);
    currentStomper->BindMode(5,palFlatOrangeMode5);
    currentStomper->BindMode(6,palWarmRadialMode6);
    currentStomper->BindMode(7,palGreenRadialMode7);
    currentStomper->BindMode(9,palBlackAndWhiteMode9);
    currentStomper->BindMode(10,palAnimatedWeatherMode10);
    currentStomper->BindMode(11,palSmallCarBigHeadMode11);
    currentStomper->BindMode(12,palReflectionBlendMode12);
}
void CloseStomper()
{
    if(currentStomper)currentStomper->Clear();
    currentStomper.reset();
    activeRadialBlur=nullptr;
}
void BindPalRadialRenderEgg(ScreenRadialBlur* radial)
{
    if(!radial || !radial->cameras)
        throw std::runtime_error("PAL radial cheat requires its live native view and texture");
    if(activeRadialBlur && activeRadialBlur!=radial)
        throw std::runtime_error("PAL radial cheat still borrows a previous track");
    auto* stomper=GetStomper();
    if(!stomper)throw std::runtime_error("PAL radial cheat has no Stomper owner");
    if(!stomper->ModeBound(8))stomper->BindMode(8,palRadialMode8);
    activeRadialBlur=radial;
}
void UnbindPalRadialRenderEgg(const ScreenRadialBlur* radial)
{
    if(activeRadialBlur!=radial)return;
    if(auto* stomper=GetStomper();stomper &&
        stomper->Mode()>=6 && stomper->Mode()<=8)stomper->Clear();
    activeRadialBlur=nullptr;
}
void ResetRenderEggs(){if(currentStomper)currentStomper->Clear();}
void SuspendRenderEggs(){if(currentStomper)currentStomper->Suspend();}
void ResumeRenderEggs(){if(currentStomper)currentStomper->Resume();}
void DisableRenderEggs(){if(currentStomper)currentStomper->Disable();}
void EnableRenderEggs(){if(currentStomper)currentStomper->Enable();}
// PAL 24BDB8 (beta 245310) is physically JR RA/NOP.
void UpdateStomper() noexcept {}

// Final PAL 24BF18..24BFB8. The member at each original authored table
// offset 0x30+index*0x38 is precisely EasterEggCode::unlocked. The source
// branches in this priority order; record 4 intentionally shares mode 9
// with record 1. This is not a substitute for the actual effect descriptors.
std::uint32_t SelectedRenderEggMode(const hp2::EasterEggCode* records,std::size_t count)
{
    if(!records || count<9)
        throw std::runtime_error("PAL render eggs require the live EasterEgg record bank");
    if(records[0].unlocked)return 8;
    if(records[1].unlocked)return 9;
    if(records[2].unlocked)return 10;
    if(records[3].unlocked)return 11;
    if(records[4].unlocked)return 9;
    if(records[8].unlocked)return 12;
    return 0;
}

void ActivateRenderEggs(const hp2::EasterEggCode* records,std::size_t count)
{
    auto* stomper=GetStomper();
    if(!stomper)return;
    const auto selected=SelectedRenderEggMode(records,count);
    // Original mode selection remains in the EasterEgg record bank. Until
    // its descriptor has a proven native Vulkan target, do not install a
    // fake effect or crash the unrelated race constructor. Bound modes still
    // follow the exact Stomper transaction with its normal restore path.
    if(selected && stomper->ModeBound(selected))stomper->Set(selected);
}
