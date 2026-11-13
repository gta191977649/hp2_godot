#include "FMenus.hpp"
#include "FEAnim.hpp"
#include "Fred.hpp"
#include "Timer.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bWare.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>

namespace hp2 {
namespace {
// Final PAL31D750/31D780. Fourteen characters and a terminator per row.
constexpr char DefaultMap[3][15]={"abcdefghijklmn","opqrstuvwxyz!.","0123456789# ()"};
constexpr char UpperMap[3][15]={"ABCDEFGHIJKLMN","OPQRSTUVWXYZ!.","0123456789# ()"};
constexpr std::int32_t NameHelp[]={800073,800074,803010,800075,0};
constexpr std::int32_t KeyboardHelp[]={803001,803007,803008,803004,803006,803005,0};
constexpr std::uint32_t NameObjects[]={0x71450a81u,0x71572302u};
constexpr std::uint32_t GenderObjects[]={0xd6f9d954u,0xc33bae35u};
MainMenuScreenServices* nameServices{};

void CopyNameBytes(char* destination,const char* source,std::size_t limit)
{
    // bStrNCpy2535A0: stop after copying the first NUL, do not pad the tail.
    for (std::size_t i=0;i<limit;++i) {
        destination[i]=source[i];
        if (!source[i])break;
    }
}
void SnapCurve(tCubic1D& curve)
{
    curve.mode=0;curve.value=curve.desiredValue;curve.derivative=curve.desiredDerivative;
}
float ObjectHeight(FlailerUiState& ui,FlailerObjectChunk& object)
{
    if (object.kind==1)return ui.resources->ScaledTextHeight(static_cast<FlailerTextChunk&>(object));
    if (object.kind==2)return ui.resources->ScaledBitmapHeight(static_cast<FlailerBitmapChunk&>(object));
    return 0.0f;
}
void ObjectColor(FlailerObjectChunk* object,std::uint32_t color)
{
    if (!object)return;
    if (object->kind==1)static_cast<FlailerTextChunk*>(object)->SetColor(color);
    else if (object->kind==2)static_cast<FlailerBitmapChunk*>(object)->SetColor(color);
}

class tKeyboard final {
public:
    tKeyboard(MainMenuScreenServices& services,std::uint32_t screen)
        : services_(services),screen_(screen),pulse_(0.0f,128.0f,10.0f)
    {
        for (auto& axis:cursor_.axes)tCubic1DInitialize(&axis);
        tCubic1DInitialize(&slide_);
        if (auto* anchor=Ui().resources->FindObject(screen_,0x35b2f256u))
            Ui().resources->GetTopLeft(*anchor,savedX_,savedY_,Ui().language);
        SetVisible(player_,0);
        for (auto& axis:cursor_.axes) {axis.duration=0.10000000149011612f;axis.continuous=0;}
    }
    void SetVisible(std::int32_t player,std::int32_t visible)
    {
        auto& resources=*Ui().resources;
        resources.SetGroupVisibility(screen_,0x670b6fddu,visible);
        if (player==0 || player==1) {
            resources.SetGroupVisibility(screen_,player==0?0x7b95e55cu:0x7deb0dfdu,visible);
            resources.SetObjectVisibility(screen_,NameObjects[player],visible==1?0:1);
        }
        resources.SetObjectVisibility(screen_,0xc01086b5u,visible);
        if (!visible)resources.MoveGroupTo(screen_,0x670b6fddu,0x35b2f256u,
            static_cast<float>(convertFloatToInt32(savedX_)),static_cast<float>(convertFloatToInt32(savedY_)));
    }
    void Activate(std::int32_t player)
    {
        Require(player==0 || player==1,"Name keyboard has no authored player slot");
        player_=player;SetVisible(player_,1);nameObject_=NameObjects[player_];
        UpdateCursor();for (auto& axis:cursor_.axes)SnapCurve(axis);
        editPosition_=0;
        slide_.desiredValue=650.0f;slide_.duration=0.5f;slide_.continuous=0;
        SnapCurve(slide_);slide_.desiredValue=savedX_;
        if (slide_.value!=slide_.desiredValue)slide_.mode=2;
        Tick();map_=DefaultMap;edit_.fill(0);
        CopyNameBytes(edit_.data(),Profile().playerName,7);
        SyncEditWithDisplay();RedrawCharSet();
    }
    void Unactivate() {SetVisible(player_,0);}
    float GetColumnX(std::int32_t column) {return GetCenter("CHAR_0%X",column,true);}
    float GetRowY(std::int32_t row) {return GetCenter("CHAR_%X0",row,false);}
    void UpdateCursor()
    {
        bool wrapped=false;
        if (column_<0) {column_=13;wrapped=true;}
        if (column_>13) {column_=0;wrapped=true;}
        if (row_<0) {row_=2;wrapped=true;}
        if (row_>2) {row_=0;wrapped=true;}
        const float desired[]={GetColumnX(column_),GetRowY(row_)};
        tCubic2DSetValDesired(&cursor_,desired);
        if (wrapped)for (auto& axis:cursor_.axes)SnapCurve(axis);
    }
    void Tick()
    {
        auto& ui=Ui();
        if (slide_.mode) {UpdateCursor();for (auto& axis:cursor_.axes)SnapCurve(axis);}
        tCubic2DUpdate(&cursor_,ui.clock->realTimeElapsed);
        if (auto* cursor=ui.resources->FindObject(screen_,0x249db7b7u)) {
            // Both original SetCenter wrappers1446E0/144760 use GetHeight.
            const float half=static_cast<float>(convertFloatToInt32(ObjectHeight(ui,*cursor)))*0.5f;
            cursor->SetTopLeftX(static_cast<float>(convertFloatToInt32(cursor_.axes[0].value))-half);
            cursor->SetTopLeftY(static_cast<float>(convertFloatToInt32(cursor_.axes[1].value))-half);
        }
        tCubic1DUpdate(&slide_,ui.clock->realTimeElapsed,0.0f,0.0f);
        ui.resources->MoveGroupTo(screen_,0x670b6fddu,0x35b2f256u,
            static_cast<float>(convertFloatToInt32(slide_.value)),static_cast<float>(convertFloatToInt32(savedY_)));
        pulse_.Step(ui.clock->realTimeElapsed);
        if (auto* caret=ui.resources->FindObject(screen_,0xc01086b5u))caret->SetAlpha(pulse_.GetValInt());
    }
    void MoveEditText(std::int32_t direction)
    {
        editPosition_=std::clamp(editPosition_+direction,0,6);SyncEditWithDisplay();
    }
    char GetChar(std::int32_t row,std::int32_t column) const
    {
        Require(row>=0 && row<3 && column>=0 && column<14,"Name keyboard cursor is outside its actual character map");
        return map_[row][column];
    }
    bool SelectChar(ScreenCallbackData& data)
    {
        const auto character=GetChar(row_,column_);
        if (character==')') {data.sound=29;return false;}
        if (character=='(') {data.sound=26;DeleteCharacter();return true;}
        edit_[static_cast<std::size_t>(editPosition_)]=character;
        Ui().resources->Printf(screen_,nameObject_,"%s",edit_.data());
        MoveEditText(1);data.sound=25;return true;
    }
    void DeleteCharacter()
    {
        for (auto i=editPosition_;i<6;++i)edit_[static_cast<std::size_t>(i)]=edit_[static_cast<std::size_t>(i+1)];
        edit_[6]=' ';edit_[7]=0;
        Ui().resources->Printf(screen_,nameObject_,"%s",edit_.data());
        SyncEditWithDisplay();MoveEditText(-1);
    }
    void SyncEditWithDisplay()
    {
        auto& ui=Ui();
        for (std::int32_t i=0;i<7;++i) {
            char objectName[16];std::snprintf(objectName,sizeof(objectName),"P%dC%d",player_+1,i+1);
            const auto key=hashString(objectName);
            ui.resources->Printf(screen_,key,"%c",edit_[static_cast<std::size_t>(i)]);
            auto* object=ui.resources->FindObject(screen_,key);
            ObjectColor(object,i==editPosition_?0x80004080u:0x80808080u);
            if (i==editPosition_ && object) {
                float x,y;ui.resources->GetTopLeft(*object,x,y,ui.language);
                if (auto* caret=ui.resources->FindObject(screen_,0xc01086b5u)) {
                    caret->SetTopLeftX(x);
                    caret->SetTopLeftY(y+static_cast<float>(convertFloatToInt32(ObjectHeight(ui,*object)))+2.0f);
                }
            }
        }
    }
    void SwitchCharSet() {map_=map_==DefaultMap?UpperMap:DefaultMap;RedrawCharSet();}
    void RedrawCharSet()
    {
        for (std::int32_t row=0;row<3;++row)for (std::int32_t column=0;column<14;++column) {
            char name[20];std::snprintf(name,sizeof(name),"CHAR_%X%X",row,column);
            Ui().resources->Printf(screen_,hashString(name),"%c",GetChar(row,column));
        }
    }
    bool PlayerRight(std::int32_t port) const {return (player_==0 || player_==1) && player_==port;}
    bool FocusPointerCharacter(std::uint32_t key)
    {
        if (!PlayerRight(0)) return false;
        for (int row=0;row<3;++row)for (int column=0;column<14;++column) {
            char name[20];std::snprintf(name,sizeof(name),"CHAR_%X%X",row,column);
            if (hashString(name)!=key) continue;
            column_=column;row_=row;UpdateCursor();return true;
        }
        return false;
    }
    bool HandleEvent(ScreenMessage message,ScreenCallbackData& data)
    {
        // Closed PAL2BF270 table: ScreenEvent13/15 move the edit caret;
        // LeaveScreen commits the same seven-character buffer and consumes Back.
        if (message==ScreenMessage::Tick) {Tick();return true;}
        if (message==ScreenMessage::LeaveScreen) {data.value=0;data.sound=29;Commit();return false;}
        if (message!=ScreenMessage::Select && message!=ScreenMessage::Left && message!=ScreenMessage::Right &&
            message!=ScreenMessage::Previous && message!=ScreenMessage::Next && message!=ScreenMessage::ItemEvent &&
            message!=ScreenMessage::ScreenEvent13 && message!=ScreenMessage::ScreenEvent15)return true;
        if (!PlayerRight(data.joystickPort)) {data.sound=0;return true;}
        switch (message) {
        case ScreenMessage::Left:--column_;data.sound=28;data.value=1;UpdateCursor();return true;
        case ScreenMessage::Right:++column_;data.sound=28;data.value=1;UpdateCursor();return true;
        case ScreenMessage::Previous:--row_;Ui().sound->Play(28);data.value=1;UpdateCursor();return true;
        case ScreenMessage::Next:++row_;Ui().sound->Play(28);data.value=1;UpdateCursor();return true;
        case ScreenMessage::ScreenEvent13:MoveEditText(-1);data.sound=1;return true;
        case ScreenMessage::ScreenEvent15:MoveEditText(1);data.sound=2;return true;
        case ScreenMessage::ItemEvent:SwitchCharSet();data.sound=27;return true;
        case ScreenMessage::Select:if (SelectChar(data))return true;Commit();return false;
        default:return true;
        }
    }
private:
    FlailerUiState& Ui() const {return *services_.ui;}
    PlayerSettings& Profile() const {return services_.frontend->players[static_cast<std::size_t>(player_)]->settings;}
    float GetCenter(const char* format,std::int32_t number,bool horizontal)
    {
        char name[20];std::snprintf(name,sizeof(name),format,number);
        std::int32_t left,top,right,bottom;
        Require(Ui().resources->GetBoundingBox(screen_,hashString(name),left,top,right,bottom,Ui().language),
            "Name keyboard lacks its authored row/column anchor");
        return static_cast<float>(horizontal ? left+(right-left)/2 : top+(bottom-top)/2);
    }
    void Commit() {CopyNameBytes(Profile().playerName,edit_.data(),11);Unactivate();}
    MainMenuScreenServices& services_;
    std::uint32_t screen_;
    std::int32_t player_{},row_{},column_{},editPosition_{};
    std::uint32_t nameObject_{NameObjects[0]};
    tCubic2D cursor_{};
    tCubic1D slide_{};
    std::array<char,8> edit_{};
    float savedX_{},savedY_{};
    const char (*map_)[15]{DefaultMap};
    FlailerSineWave pulse_;
};

struct tNameEntryData {
    std::array<std::array<char,12>,2> names{};
    std::array<std::uint8_t,2> genders{};
    void Init(const FrontendPlayerManager& frontend)
    {
        for (std::size_t i=0;i<2;++i) {
            CopyNameBytes(names[i].data(),frontend.players[i]->settings.playerName,12);
            // Final profile+64B, cPlayerSettings+7 (PreferredGender in the
            // individually retained owner schema). Existing native field.
            genders[i]=frontend.players[i]->settings.authoredTimeDisplayOption;
        }
    }
    bool Different(const FrontendPlayerManager& frontend) const
    {
        for (std::size_t i=0;i<2;++i)
            if (std::strncmp(names[i].data(),frontend.players[i]->settings.playerName,12)!=0)return true;
        for (std::size_t i=0;i<2;++i)
            if (genders[i]!=frontend.players[i]->settings.authoredTimeDisplayOption)return true;
        return false;
    }
};

class NameEntry final {
public:
    bool FocusPointerCharacter(std::uint32_t key)
    {
        return editing_ && keyboard_.FocusPointerCharacter(key);
    }
    NameEntry(MainMenuScreenServices& services,std::uint32_t screen)
        : services_(services),screen_(screen),keyboard_(services,screen),
          highlight_(*services.ui,screen,0xab671c9au,services.ui->animationSettings.highlightColor),
          selection_(*services.ui,screen,0x1042eac3u,-1),
          arrows_(*services.ui,screen,0xd1cda594u,0x9345e4a7u),
          blurb_(*services.ui,screen,0x75980cf0u,0x75980cf0u,0x70a404ecu)
    {
        keyboard_.SetVisible(0,0);keyboard_.SetVisible(1,0);
        for (std::size_t i=0;i<2;++i)
            services_.ui->resources->Printf(screen_,NameObjects[i],"%s",services_.frontend->players[i]->settings.playerName);
        RedrawGenders();snapshot_.Init(*services_.frontend);
    }
    bool MaybeActivateKeyboard(ScreenCallbackData& data)
    {
        const std::int32_t player=data.key==0x52e039f5u?1:0;
        if (data.joystickPort!=player)return false;
        keyboard_.Activate(player);editing_=true;services_.screens->SetHelpOverride(KeyboardHelp);return true;
    }
    void ChangeGender(ScreenMessage message,ScreenCallbackData& data)
    {
        const std::size_t player=data.key==0x52e039f6u?1u:0u;
        const bool allowed=data.joystickPort==static_cast<std::int32_t>(player);
        if (allowed) {
            auto& gender=services_.frontend->players[player]->settings.authoredTimeDisplayOption;
            gender=gender==0?1:0;
        }
        RedrawGenders();
        if (!allowed) {data.sound=0;return;}
        if (message==ScreenMessage::Left)data.sound=1;
        if (message==ScreenMessage::Right)data.sound=2;
        selection_.SetCurrentMenuItem(GenderObjects[player]);
    }
    void RedrawGenders()
    {
        // PAL retains the first ID if the second malformed value is neither
        // zero nor one; ordinary mutable profile values are zero/one.
        std::uint32_t id=0;
        for (std::size_t i=0;i<2;++i) {
            const auto gender=services_.frontend->players[i]->settings.authoredTimeDisplayOption;
            if (gender==0)id=160004;
            else if (gender==1)id=160005;
            else if (!i)id=0;
            if (auto* object=services_.ui->resources->FindObject(screen_,GenderObjects[i],1))
                static_cast<FlailerTextChunk*>(object)->SetLanguageID(id);
        }
    }
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data)
    {
        if (message==ScreenMessage::Tick) {highlight_.Animate();selection_.Animate();blurb_.Animate();}
        if (editing_) {
            if (!keyboard_.HandleEvent(message,data)) {editing_=false;services_.screens->SetHelpOverride(nullptr);}
            return;
        }
        arrows_.HandleEvent(message,data);
        if (message==ScreenMessage::LeaveScreen) {
            if (snapshot_.Different(*services_.frontend) && services_.frontend->autoSave)
                services_.screens->stack.Push("Autosave");
        } else if (message==ScreenMessage::Select) {
            if (data.key==0x52e039f3u || data.key==0x52e039f5u) {
                if (!MaybeActivateKeyboard(data))data.sound=0;
            } else if (data.key==0x52e039f4u || data.key==0x52e039f6u)data.sound=0;
        } else if (message==ScreenMessage::Left || message==ScreenMessage::Right) {
            if (data.key==0x52e039f4u || data.key==0x52e039f6u)ChangeGender(message,data);
        } else if (message==ScreenMessage::Highlight) {
            highlight_.SetCurrentMenuItem(data.key);
            if (data.key>=0x52e039f3u && data.key<=0x52e039f6u) {
                const auto index=data.key-0x52e039f3u;
                const auto target=(index&1u)?GenderObjects[index/2]:0u;
                selection_.SetCurrentMenuItem(target);arrows_.SetCurrentItem(target);
                blurb_.NewText(static_cast<std::int32_t>(150800u+index));
            }
        }
    }
private:
    MainMenuScreenServices& services_;
    std::uint32_t screen_;
    tKeyboard keyboard_;
    tNameEntryData snapshot_;
    FlailerHighlightAnimation highlight_,selection_;
    FlailerLeftRightArrowScroll arrows_;
    FlailerAnimatedBlurb blurb_;
    bool editing_{};
};
std::unique_ptr<NameEntry> nameOwner;
}

bool FocusNamePointerCharacter(std::uint32_t key)
{
    return nameOwner && nameOwner->FocusPointerCharacter(key);
}

void BindNameEntryScreen(MainMenuScreenServices& services)
{
    Require(!nameOwner && services.ui && services.ui->resources && services.ui->clock && services.ui->sound &&
        services.ui->language && services.screens && services.screens->ui==services.ui && services.frontend &&
        services.frontend->players[0] && services.frontend->players[1],
        "NameEntry bindings do not share the actual UI/profile owners");
    nameServices=&services;
}
void UnbindNameEntryScreen()
{
    nameOwner.reset();
    if (nameServices && nameServices->screens->overrideHelpItems==KeyboardHelp)
        nameServices->screens->SetHelpOverride(nullptr);
    nameServices=nullptr;
}
void NameEntryScreenCallback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(nameServices && data,"NameEntry callback has no actual bindings/data");
    if (message==ScreenMessage::ScreenActivate) {
        Require(!nameOwner,"NameEntry received duplicate activation");
        nameOwner=std::make_unique<NameEntry>(*nameServices,data->key);
    } else if (message==ScreenMessage::ScreenDeactivate) {
        nameOwner.reset();
        if (nameServices->screens->overrideHelpItems==KeyboardHelp)nameServices->screens->SetHelpOverride(nullptr);
    } else {
        Require(nameOwner!=nullptr,"NameEntry callback precedes activation");
        nameOwner->HandleEvent(message,*data);
    }
}
FlailerScreenConfig fNameEntry{"NameEntry",NameEntryScreenCallback,
    {{static_cast<std::int32_t>(0x617a4e0au),0,0,0}},{{-1,-1,-1,-1}},1,NameHelp};
}
