#include "FMenus.hpp"
#include "Language.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bWare.hpp"

#include <cstring>

namespace hp2 {

FlailerHelpMenu::FlailerHelpMenu(FlailerUiState& ui) : ui_(&ui)
{
    Require(ui.resources && ui.sound && ui.render && ui.language && ui.clock,
        "Help menu has an incomplete frontend binding");
    tCubic1DInitialize(&slide_);
    slide_.value=slide_.desiredValue=650.0f;
    slide_.derivative=slide_.desiredDerivative=0.0f;
    slide_.duration=0.5f;slide_.threshold=1.0f;slide_.mode=0;slide_.continuous=0;
    fadeObject_=ui.resources->FindObject(screenKey_,0x7830fd99u);
    if (fadeObject_) {
        for (std::int32_t corner=0;corner<4;++corner) {
            std::int32_t alpha=0;
            if (fadeObject_->kind==2) alpha=static_cast<FlailerBitmapChunk*>(fadeObject_)->GetAlpha(corner);
            else if (fadeObject_->kind==1) alpha=static_cast<FlailerTextChunk*>(fadeObject_)->GetAlpha();
            initialAlpha_[static_cast<std::size_t>(corner)]=static_cast<float>(alpha);
        }
    }
}

bool FlailerHelpMenu::IsActive() const
{
    return slide_.desiredValue<640.0f;
}

void FlailerHelpMenu::PlaySelect()
{
    auto& sound=*ui_->sound;
    Require(sound.enabled && sound.audio,"Help sound has no audio owner");
    if (!*sound.enabled) return;
    sound.result=worldAudioPlayVoice(sound.audio,sound.events[5].voice,1.0f,0.0f,0.0f);
}

void FlailerHelpMenu::SplitHelp(std::int32_t languageId,char prefix[128],char description[128],std::uint32_t capacity) const
{
    const auto* string=ui_->language->GetString(languageId);
    Require(string && capacity>=2 && capacity<=128,"Help text is missing or exceeds its retained buffers");
    const auto* cursor=reinterpret_cast<const unsigned char*>(string);
    std::size_t written=0;
    for (;;) {
        const auto character=*cursor;
        const bool whitespace=character==0 || character==9 || character==32;
        if (!IsHelpCharacter(character) && !whitespace && character!='+') break;
        // PAL's space predicate also accepts NUL. A valid help string must
        // contain its description; reject malformed input before reading
        // beyond this language-owned C string.
        Require(character!=0,"Help text contains no description");
        ++cursor;
        if (whitespace) continue;
        if (character=='+') {
            Require(written+3<128,"Help icon prefix exceeds its retained buffer");
            prefix[written++]=' ';prefix[written++]='+';prefix[written++]=' ';
        } else {
            Require(written+1<128,"Help icon prefix exceeds its retained buffer");
            prefix[written++]=static_cast<char>(character);
        }
    }
    prefix[written]='\0';
    written=0;
    while (*cursor && written<capacity-2u) description[written++]=static_cast<char>(*cursor++);
    description[written]='\0';
}

void FlailerHelpMenu::Summon(const std::int32_t* items)
{
    if (!ui_->helpEnabled) return;
    ++summonCount_;
    if (!items) return;
    PlaySelect();
    slide_.desiredValue=53.0f;
    if (slide_.value!=53.0f) slide_.mode=2;
    static constexpr std::array<std::uint32_t,8> descriptions{{0xeacf0330u,0xeacf0331u,0xeacf0332u,0xeacf0333u,
        0xeacf0334u,0xeacf0335u,0xeacf0336u,0xeacf0337u}};
    static constexpr std::array<std::uint32_t,8> icons{{0x8c92aa9au,0x8c92aa9bu,0x8c92aa9cu,0x8c92aa9du,
        0x8c92aa9eu,0x8c92aa9fu,0x8c92aaa0u,0x8c92aaa1u}};
    bool shown=true;
    for (std::size_t index=0;index<8;++index) {
        // The zero terminator fixes all remaining rows as hidden. Avoid
        // PAL's unused reads into adjacent globals beyond a native short list.
        if (shown && items[index]) {
            char prefix[128]{},description[128]{};
            SplitHelp(items[index],prefix,description,128);
            // Source passes these completed strings to libc formatting with
            // no additional values. Help resources must contain no consuming
            // conversion; literal percent escapes remain supported.
            const auto validFormat=[](const char* value) {
                for (const char* at=value;*at;++at) {
                    if (*at=='%') { if (at[1]!='%') return false;++at; }
                }
                return true;
            };
            Require(validFormat(prefix) && validFormat(description),"Help format requires unavailable arguments");
            ui_->resources->Printf(screenKey_,icons[index],prefix);
            ui_->resources->Printf(screenKey_,descriptions[index],description);
            ui_->resources->SetObjectVisibility(screenKey_,descriptions[index],1);
            ui_->resources->SetObjectVisibility(screenKey_,icons[index],1);
        } else {
            ui_->resources->SetObjectVisibility(screenKey_,descriptions[index],0);
            shown=false;
            ui_->resources->SetObjectVisibility(screenKey_,icons[index],0);
        }
    }
}

void FlailerHelpMenu::Dismiss()
{
    slide_.desiredValue=650.0f;
    if (slide_.value!=650.0f) slide_.mode=2;
    std::int32_t signedCount;
    std::memcpy(&signedCount,&summonCount_,sizeof(signedCount));
    if (signedCount>0) PlaySelect();
}

void FlailerHelpMenu::Tick()
{
    const auto visible=slide_.value<640.0f ? 1 : 0;
    if (visible!=wasVisible_) {
        if (summonCount_) ui_->render->SetScreenVisibility(*ui_->resources,screenKey_,visible?0:1);
        wasVisible_=visible;
    }
    if (auto* anchor=ui_->resources->FindObject(screenKey_,0x49b30391u)) {
        const auto x=convertFloatToInt32(slide_.value);
        const auto y=convertFloatToInt32(anchor->topLeftY);
        ui_->resources->MoveGroupTo(screenKey_,0x3c86db48u,0x49b30391u,static_cast<float>(x),static_cast<float>(y));
    }
    float alpha=1.0f;
    if (slide_.value>=320.0f) alpha=1.0f-(slide_.value-320.0f)*0.003125f;
    if (fadeObject_) {
        if (fadeObject_->kind==2) {
            for (std::int32_t corner=0;corner<4;++corner)
                static_cast<FlailerBitmapChunk*>(fadeObject_)->SetAlpha(
                    convertFloatToInt32(initialAlpha_[static_cast<std::size_t>(corner)]*alpha),corner);
        } else if (fadeObject_->kind==1)
            static_cast<FlailerTextChunk*>(fadeObject_)->SetAlpha(convertFloatToInt32(initialAlpha_[0]*alpha));
    }
    tCubic1DUpdate(&slide_,ui_->clock->realTimeElapsed,0.0f,0.0f);
}

} // namespace hp2
