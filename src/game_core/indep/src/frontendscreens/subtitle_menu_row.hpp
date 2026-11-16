#pragma once

#include "FMenus.hpp"
#include "FlailerDB.hpp"

namespace hp2 {
// Port-side row that uses the owning screen's authored text and native menu ring.
class SubtitleMenuRow final {
public:
    static constexpr std::uint32_t ValueKey=0x83ab872cu;

    SubtitleMenuRow(MainMenuScreenServices& services,std::uint32_t screenKey,
        std::uint32_t rowKey,std::uint32_t previousKey,std::uint32_t sourceValue,float spacing)
        : services_(services),art_(*services.ui->resources->FindScreen(screenKey)),
          screen_(*services.screens->active->FindFlailerScreen(static_cast<std::int32_t>(screenKey)))
    {
        auto* previous=screen_.menu->FindItem(previousKey);
        auto& resources=*services.ui->resources;
        auto* source=static_cast<FlailerTextChunk*>(resources.FindObject(screenKey,sourceValue,1));
        Prepare(label_,*static_cast<FlailerTextChunk*>(previous->object),rowKey,spacing);
        label_.SetString("Subtitle");
        Prepare(value_,*source,ValueKey,spacing);
        Draw(services.subtitleSetting(services.subtitleContext,-1));
        item_=std::make_unique<FlailerMenuItem>(screen_.menu,rowKey,previous->ordinal+1);
        Link(label_);Link(value_);
        item_->object=&label_;
        item_->ResetSavedParameters();
        item_->previous=previous;
        item_->next=previous->next;
        previous->next->previous=item_.get();
        previous->next=item_.get();
    }

    ~SubtitleMenuRow()
    {
        if(screen_.menu) {
            if(screen_.menu->current==item_.get())screen_.menu->current=item_->previous;
            item_->previous->next=item_->next;
            item_->next->previous=item_->previous;
        }
        item_.reset();
        for(auto* text:{&label_,&value_}) {
            text->DeleteFlailerStrip();
            text->previous->next=text->next;
            text->next->previous=text->previous;
            --art_.textCount;
        }
        for(const auto& [object,y]:shiftedRows_)object->SetTopLeftY(y);
    }

    void ShiftRow(FlailerObjectChunk& object,float spacing)
    {
        shiftedRows_.emplace_back(&object,object.topLeftY);
        object.SetTopLeftY(object.topLeftY+spacing);
    }

    void Toggle()
    {
        const bool enabled=services_.subtitleSetting(services_.subtitleContext,-1);
        Draw(services_.subtitleSetting(services_.subtitleContext,enabled?0:1));
    }

private:
    static void Prepare(FlailerTextChunk& target,const FlailerTextChunk& source,std::uint32_t key,float spacing)
    {
        target=source;
        target.key=key;
        target.strip=nullptr;
        target.languageId=0;
        target.textCapacity=64;
        target.ownedText.assign(65,0);
        target.text=target.ownedText.data();
        target.SetTopLeftY(source.topLeftY+spacing);
        target.visible=1;
    }

    void Link(FlailerTextChunk& text)
    {
        text.previous=art_.objects.previous;
        text.next=&art_.objects;
        text.previous->next=&text;
        art_.objects.previous=&text;
        ++art_.textCount;
    }

    void Draw(bool enabled)
    {
        value_.SetLanguageID(enabled?0x22352u:0x22353u);
    }

    MainMenuScreenServices& services_;
    FlailerScreenResource& art_;
    FlailerScreen& screen_;
    FlailerTextChunk label_,value_;
    std::unique_ptr<FlailerMenuItem> item_;
    std::vector<std::pair<FlailerObjectChunk*,float>> shiftedRows_;
};
}
