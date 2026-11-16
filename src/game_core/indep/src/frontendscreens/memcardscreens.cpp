#include "FMenus.hpp"
#include "Fred.hpp"
#include "Language.hpp"
#include "Flags.hpp"
#include "Timer.hpp"
#include "FEAnim.hpp"
#include "inc/dbFiles.hpp"
#include "bware/inc/bWare.hpp"
#include <algorithm>
#include <cstring>
#include <random>
#include <string_view>
#include <utility>

namespace hp2 {
namespace {
constexpr const char* ProfileFilename="profile.json";
constexpr std::uint32_t ManualScreen=0x9829bfabu;
constexpr std::uint32_t PickerScreen=0x8bc68340u; // Borrow the authored Display rows and masks.
constexpr std::uint32_t FirstRow=0x52e039f3u;
constexpr std::uint32_t FirstProfileRow=0x48525000u;
constexpr std::size_t VisibleRows=7;
constexpr std::array<std::uint32_t,VisibleRows> RowMasks{
    0x6e34e350u,0x708a0bf1u,0x72df3492u,0x75345d33u,0x778985d4u,0x79deae75u,0x7c33d716u};
constexpr std::uint32_t UpperRail=0xc671680cu,LowerRail=0x32c91daeu;
constexpr std::uint32_t ScrollUp=0xedf2e4eeu,ScrollDown=0x20d73d01u;
constexpr std::uint32_t TransferScreen=0xfa2ac5c6u;
constexpr std::uint32_t BootStatusPopup=0x0ce3f7ceu;
constexpr std::uint32_t ContinuePopup=0x74d2dfa2u;
constexpr std::array<std::int32_t,4> LoadSaveHelp{{0xc3546,0xc3547,0xc3548,0}};
MemcardScreens* boundMemcards{};
// PAL245640..2457C0 SavegameData owns the detached transfer, while the
// original live FRED owns progress. PC files carry named UTF-8 JSON.
class SavegameData final {
public:
    explicit SavegameData(FrontendPlayerManager& frontend):frontend_(frontend) {}
    void* SetupForSave() {text_=frontend_.SerializeNativeProfileText();return text_.data();}
    std::int32_t Size() const {return static_cast<std::int32_t>(text_.size());}
    const std::string& Text() const {return text_;}
private:
    FrontendPlayerManager& frontend_;
    std::string text_;
};

// A port page borrows its art for one activation, just like the other
// frontend extensions. The original Flailer ring still handles focus.
class ProfileMenuArt final {
public:
    ProfileMenuArt(MainMenuScreenServices& menus,std::uint32_t screen)
        :resources(*menus.ui->resources),art(*resources.FindScreen(screen)),
         page(*menus.screens->active->FindFlailerScreen(static_cast<std::int32_t>(screen))),menu(*page.menu) {}

    ~ProfileMenuArt()
    {
        if(pickerViewport) {
            if(page.menu==&menu) {
                menu.root.next=oldFirst;menu.root.previous=oldLast;menu.current=oldCurrent;
            }
            pickerItems.clear();
        }
        if(extraItem) {
            if(page.menu==&menu) {
                if(menu.current==extraItem.get())menu.current=extraItem->previous;
                extraItem->previous->next=extraItem->next;
                extraItem->next->previous=extraItem->previous;
            }
            extraItem.reset();
            extraText.DeleteFlailerStrip();
            extraText.previous->next=extraText.next;
            extraText.next->previous=extraText.previous;
            --art.textCount;
        }
        for(auto& saved:texts) {
            Write(*saved.object,saved.value);
            if(saved.language)saved.object->SetLanguageID(saved.language);
        }
        for(const auto& saved:objects) {
            saved.object->SetTopLeftY(saved.y);
            saved.object->SetClipToPoly(saved.clip);
            resources.SetObjectVisibility(art.key,saved.object->key,saved.visible);
        }
    }

    void Show(std::uint32_t key,bool visible)
    {
        if(auto* object=resources.FindObject(art.key,key)) {
            Remember(*object);
            resources.SetObjectVisibility(art.key,key,visible?1:0);
        }
    }
    void Relabel(std::uint32_t key,const std::string& value)
    {
        if(auto* object=resources.FindObject(art.key,key,1))
            Relabel(static_cast<FlailerTextChunk&>(*object),value);
    }
    void Relabel(FlailerTextChunk& text,const std::string& value)
    {
        if(std::none_of(texts.begin(),texts.end(),[&](const auto& saved){return saved.object==&text;}))
            texts.push_back({&text,text.text?text.text:"",text.languageId});
        Write(text,value);
    }
    void Place(std::uint32_t key,float y)
    {
        if(auto* object=resources.FindObject(art.key,key)) {
            Remember(*object);
            object->SetTopLeftY(y);
        }
    }
    void AddCreateRow()
    {
        auto* previous=menu.FindItem(FirstRow+2);
        Require(previous && previous->object,"Save/Load requires the original Autosave row");
        extraText=static_cast<const FlailerTextChunk&>(*previous->object);
        extraText.key=FirstRow+3;
        extraText.strip=nullptr;
        extraText.clipObjectKey=0;
        extraText.languageId=0;
        extraText.textCapacity=64;
        extraText.ownedText.assign(65,0);
        extraText.text=extraText.ownedText.data();
        extraText.SetString("Create Profile");
        const auto* above=resources.FindObject(art.key,FirstRow+1,1);
        const auto* first=resources.FindObject(art.key,FirstRow,1);
        Require(first && above,"Save/Load requires the original Save and Load rows");
        // Fit four rows within the original memory-card panel.
        const auto top=first->topLeftY;
        const auto spacing=(extraText.topLeftY-above->topLeftY)*0.75f;
        Place(FirstRow+1,top+spacing);
        Place(FirstRow+2,top+2*spacing);
        extraText.SetTopLeftY(top+3*spacing);
        extraText.previous=art.objects.previous;extraText.next=&art.objects;
        extraText.previous->next=&extraText;art.objects.previous=&extraText;++art.textCount;
        extraItem=std::make_unique<FlailerMenuItem>(&menu,extraText.key,4);
        extraItem->object=&extraText;extraItem->previous=previous;extraItem->next=previous->next;
        previous->next->previous=extraItem.get();previous->next=extraItem.get();
    }
    void SetPickerTitle()
    {
        for(auto* object=art.objects.next;object!=&art.objects;object=object->next) {
            if(object->kind==1 && object->key==0x6eb9985du && object->topLeftY<40) {
                Relabel(static_cast<FlailerTextChunk&>(*object),"Select Profile");
                break;
            }
        }
        for(auto key:{0x55cb09f2u,0x2e931f1bu,0x03d90172u,0x5610f2f4u,0x422313cbu,0x8fe4086bu,
                      0xd1cda594u,0x9345e4a7u,FirstRow+7,FirstRow+8})Show(key,false);
    }

    void BuildPicker(const std::vector<std::string>& captions,std::size_t selected)
    {
        SetPickerTitle();
        const auto& model=*static_cast<const FlailerTextChunk*>(resources.FindObject(art.key,FirstRow,1));
        const auto& next=*resources.FindObject(art.key,FirstRow+1,1);
        auto* mask=static_cast<FlailerBitmapChunk*>(resources.FindObject(art.key,RowMasks[0],2));
        const auto pitch=next.topLeftY-model.topLeftY;
        Require(mask && pitch>0.0f,"Profile scrolling requires the authored Display rows");
        pickerY=model.topLeftY;
        railY=mask->topLeftY;
        railHeight=resources.ScaledBitmapHeight(*mask);
        pickerViewport=std::make_unique<FlailerListViewport>(resources,art,*mask,
            -4.0f,railY-6.0f,648.0f,pitch*(VisibleRows-1)+railHeight+14.0f,pitch,VisibleRows);
        for(auto* object=art.objects.next;object!=&art.objects;object=object->next) {
            if(object->kind==1 && object->topLeftY>=pickerY && object->topLeftY<343.0f) {
                Remember(*object);object->visible=0;object->DeleteFlailerStrip();
            }
        }
        for(const auto key:RowMasks)Show(key,false);
        for(const auto key:{UpperRail,LowerRail})if(auto* rail=resources.FindObject(art.key,key)) {
            Remember(*rail);pickerViewport->Clip(*rail);
        }
        oldFirst=menu.root.next;oldLast=menu.root.previous;oldCurrent=menu.current;
        menu.root.next=menu.root.previous=&menu.root;menu.current=nullptr;
        for(std::size_t i=0;i<captions.size();++i) {
            const auto key=FirstProfileRow+static_cast<std::uint32_t>(i);
            auto& text=pickerViewport->CloneText(model,key);
            text.visible=1;
            Write(text,captions[i]);
            const auto width=resources.ScaledTextWidth(text,nullptr);
            if(width>475.0f) {text.SetScaleX(text.scaleX*475.0f/width);text.SetScaleY(text.scaleY*475.0f/width);}
            auto item=std::make_unique<FlailerMenuItem>(&menu,key,static_cast<std::uint32_t>(i));
            item->object=&text;item->previous=menu.root.previous;item->next=&menu.root;
            item->previous->next=item.get();menu.root.previous=item.get();
            pickerItems.push_back(std::move(item));
        }
        SelectProfileRow(selected);
        pickerViewport->Tick(1.0f);
        PositionPickerRows();
    }

    void SelectProfileRow(std::size_t row)
    {
        pickerSelection=row;
        pickerViewport->Reveal(row);
        Show(ScrollUp,pickerViewport->Top()>0);
        Show(ScrollDown,pickerViewport->Top()+VisibleRows<pickerItems.size());
        PositionPickerRails();
    }

    bool AnimatePicker(float elapsed)
    {
        if(pickerViewport->Tick(elapsed))PositionPickerRows();
        return pickerViewport->Moving();
    }

    FlailerResources& resources;
    FlailerScreenResource& art;
    FlailerScreen& page;
    FlailerMenu& menu;
private:
    struct Text {FlailerTextChunk* object;std::string value;std::uint32_t language;};
    struct Object {FlailerObjectChunk* object;float y;std::uint8_t visible;std::uint32_t clip;};
    std::vector<Text> texts;
    std::vector<Object> objects;
    FlailerTextChunk extraText;
    std::unique_ptr<FlailerMenuItem> extraItem;
    std::unique_ptr<FlailerListViewport> pickerViewport;
    std::vector<std::unique_ptr<FlailerMenuItem>> pickerItems;
    FlailerMenuItem *oldFirst{},*oldLast{},*oldCurrent{};
    float pickerY{},railY{},railHeight{};
    std::size_t pickerSelection{};

    void PositionPickerRails()
    {
        const auto y=railY+pickerViewport->Displacement(pickerSelection);
        Place(UpperRail,y-6.0f);Place(LowerRail,y+railHeight);
    }

    void PositionPickerRows()
    {
        for(std::size_t i=0;i<pickerItems.size();++i)
            pickerItems[i]->object->SetTopLeftY(pickerY+pickerViewport->Displacement(i));
        PositionPickerRails();
    }

    void Remember(FlailerObjectChunk& object)
    {
        if(std::none_of(objects.begin(),objects.end(),[&](const auto& saved){return saved.object==&object;}))
            objects.push_back({&object,object.topLeftY,object.visible,object.clipObjectKey});
    }
    static void Write(FlailerTextChunk& text,const std::string& value)
    {
        if(value.size()>=text.textCapacity) {
            text.textCapacity=static_cast<std::uint32_t>(value.size()+1);
            text.ownedText.resize(text.textCapacity+1);
            text.text=text.ownedText.data();
        }
        text.SetString(value.c_str());
        text.languageId=0;
    }
};

bool IsProfileFile(std::string_view name)
{
    return name==ProfileFilename || (name.size()>13 && name.substr(0,8)=="profile-" &&
        name.substr(name.size()-5)==".json" && name.substr(name.size()-12)!=".backup.json");
}

std::string NewProfileFilename()
{
    std::random_device random;
    constexpr std::string_view hex="0123456789abcdef";
    std::string name="profile-";
    for(int i=0;i<32;++i)name+=hex[random()&15u];
    return name+".json";
}
}

struct MemcardScreens::Storage {
    FrontendPlayerManager& frontend;
    cDBFileSys files;
    SavegameData data;
    MainMenuScreenServices* menus{};
    std::function<void()> completion;
    std::string error,popupMessage;
    enum class Operation {None,Save,Load,Autosave};
    enum class BootStage {Idle,Checking,Loading,Missing,LoadError,Choosing,Complete};
    enum class Creation {None,Boot,StartupPicker,OptionsPicker,Options};
    struct Profile {std::string file,name;bool valid{};};
    Operation operation{Operation::None};
    Operation retryOperation{Operation::None};
    BootStage bootStage{BootStage::Idle};
    Creation creation{Creation::None},pendingCreation{Creation::None};
    std::string activeFile,savedProfile,defaults,pendingFile;
    std::vector<Profile> profiles;
    std::unique_ptr<ProfileMenuArt> page;
    std::unique_ptr<FlailerHighlightAnimation> highlight;
    std::size_t pickerSelection{};
    bool pickerAtBoot{},switchPopup{};
    int switchDecision{};
    bool requireNamedProfile{};
    std::int32_t bootPopup{-1};
    std::uint32_t bootTime{};
    bool awaitingCallback{},showingError{},returnAfterTick{},creatingBootProfile{},blockedByLoadFailure{};
    LoadResult lastLoad{LoadResult::Absent};
    Storage(FrontendPlayerManager& owner,const char* directory):frontend(owner),files(directory),data(owner) {}
    bool ScanProfiles() {
        profiles.clear();
        std::vector<std::string> names;
        const auto result=files.ListFiles(names);
        if(result==cMC::NotFound)return true;
        if(result!=cMC::Ok){error=files.LastErrorMessage();return false;}
        for(const auto& file:names) {
            if(!IsProfileFile(file))continue;
            Profile profile{file,{},false};
            std::vector<std::uint8_t> bytes;
            std::string reason;
            profile.valid=files.ReadFile(file.c_str(),bytes)==cMC::Ok &&
                frontend.ValidateNativeProfileText(bytes.data(),bytes.size(),reason,true,&profile.name);
            if(profile.name.empty())profile.name=profile.valid?"Unnamed Profile":file;
            profiles.push_back(std::move(profile));
        }
        std::sort(profiles.begin(),profiles.end(),[](const auto& left,const auto& right){
            if(left.name!=right.name)return left.name<right.name;
            return left.file<right.file;
        });
        return true;
    }
    std::vector<std::string> ProfileCaptions() const {
        std::vector<std::string> captions;
        captions.reserve(profiles.size()+1);
        for(const auto& profile:profiles) {
            std::string caption=profile.name;
            if(profile.valid && std::count_if(profiles.begin(),profiles.end(),[&](const Profile& other){
                return other.valid && other.name==profile.name;
            })>1)caption+=" ("+(profile.file==ProfileFilename?std::string("legacy"):profile.file.substr(8,6))+")";
            if(profile.file==activeFile)caption+=" (Active)";
            if(!profile.valid)caption+=" (Unavailable)";
            captions.push_back(std::move(caption));
        }
        captions.emplace_back("Create Profile");
        return captions;
    }
    void PositionPicker(std::size_t row) {
        if(row>profiles.size())return;
        pickerSelection=row;
        page->SelectProfileRow(row);
        highlight->SetCurrentMenuItem(FirstProfileRow+static_cast<std::uint32_t>(row));
        page->Relabel(0x75980cf0u,row==profiles.size()?"Create a new profile.":
            profiles[row].valid?"Load this profile.":"This profile is unavailable.");
    }
    void ShowNotice(const std::string& message) {
        popupMessage=message;
        FlailerPopupConfig popup;
        popup.sourceNode=menus->screens->active;popup.screenKey=ContinuePopup;
        popup.message=popupMessage.c_str();popup.buttonCount=1;popup.flags=2;popup.joystickPort=-1;
        popup.buttonLanguageIds[0]=0x24a55;
        Require(menus->screens->SummonPopup(popup)>=0,"Profile notice has no popup owner");
    }
    void RequestSwitch(std::string filename,Creation origin=Creation::None) {
        pendingFile=std::move(filename);pendingCreation=origin;
        if(activeFile.empty() || savedProfile==frontend.SerializeNativeProfileText()) {
            switchDecision=2;
            return;
        }
        constexpr std::uint32_t dialog=0x41c2fdc3u; // Authored three-choice car selection dialog.
        popupMessage="Save Profile Changes?";
        FlailerPopupConfig popup;
        popup.sourceNode=menus->screens->active;popup.screenKey=dialog;
        popup.message=popupMessage.c_str();popup.buttonCount=3;popup.flags=3;popup.joystickPort=-1;
        popup.buttonResults={{1,2,0}};popup.initialButton=2;
        Require(menus->screens->SummonPopup(popup)>=0,"Profile switch has no popup owner");
        constexpr std::array labels{"Save","Discard","Cancel"};
        for(std::size_t i=0;i<labels.size();++i)
            menus->ui->resources->SetLocalizedText(dialog,0xe0105663u+static_cast<std::uint32_t>(i),labels[i]);
        switchPopup=true;
    }
    void ShowBootPopup(std::uint32_t screen,std::string message,bool button) {
        auto& screens=*menus->screens;
        if(bootPopup>=0) {
            screens.DismissPopup(bootPopup);
            bootPopup=-1;
        }
        popupMessage=std::move(message);
        FlailerPopupConfig popup;
        popup.sourceNode=screens.active;popup.screenKey=screen;popup.message=popupMessage.c_str();
        popup.joystickPort=-1;
        if(button) {
            popup.buttonCount=1;
            popup.buttonLanguageIds[0]=0x24a55;
        } else {
            popup.flags=9; // Source status popup ignores confirm and cancel.
            popup.footerLanguageId=0x249f4;
        }
        bootPopup=screens.SummonPopup(popup);
        Require(bootPopup>=0,"Boot profile status has no original popup owner");
    }
    void ShowBootStatus(std::int32_t languageId) {
        const auto* text=menus->ui->language->GetString(languageId);
        Require(text,"Boot profile status has no authored text");
        std::string message=text;
        // The authored status displays memory-card slot 1 for the PC profile.
        for(auto slot=message.find("%d");slot!=std::string::npos;slot=message.find("%d",slot+1))
            message.replace(slot,2,"1");
        bootTime=static_cast<std::uint32_t>(menus->ui->clock->realTimer);
        ShowBootPopup(BootStatusPopup,std::move(message),false);
    }
    bool BootStatusElapsed() const {
        return static_cast<std::uint32_t>(menus->ui->clock->realTimer)-bootTime>=
            static_cast<std::uint32_t>(gameTimeQuantizeElapsed(2.0f));
    }
    bool PrepareSave() {
        if(activeFile.empty()){error="Create or load a profile before saving.";return false;}
        if(blockedByLoadFailure) {
            error="The previous profile load failed. Restore or correct that file and load it successfully before saving; existing progress was retained.";
            return false;
        }
        try {
            std::vector<std::uint8_t> previous;const auto read=files.ReadFile(activeFile.c_str(),previous);
            if(read!=cMC::Ok && read!=cMC::NotFound) {
                error=files.LastErrorMessage();
                return false;
            }
            if(read==cMC::Ok && !frontend.ValidateNativeProfileText(previous.data(),previous.size(),error)) {
                error="Existing profile was retained because it failed validation: "+error;
                return false;
            }
            // Serialize before changing either durable file. The I/O owner
            // borrows this detached string until its one completion callback.
            void* transfer=data.SetupForSave();
            const auto backup=activeFile.substr(0,activeFile.size()-5)+".backup.json";
            files.SetBackupFile(read==cMC::Ok?backup.c_str():nullptr,std::move(previous));
            files.SetFileData(activeFile.c_str(),transfer,data.Size());return true;
        } catch(const std::exception& e) {
            error=e.what();return false;
        }
    }
    static void Completed(void* argument) {static_cast<Storage*>(argument)->awaitingCallback=false;}
    void SetCaption() {
        if(menus && menus->ui && menus->ui->resources)
            menus->ui->resources->SetLanguageID(ManualScreen,0x52e039f5u,frontend.autoSave?0x249f3:0x249f2);
    }
    void ShowError() {
        if(!menus || !menus->screens || !menus->ui || showingError)return;
        popupMessage=std::string(retryOperation==Operation::Load ? "Cannot load the native profile. " :
            "Cannot write the native profile. ")+error+"\nRetry?";
        FlailerPopupConfig popup;popup.sourceNode=menus->screens->active;popup.screenKey=0x037f3133;
        popup.buttonCount=2;popup.flags=3;popup.message=popupMessage.c_str();
        popup.buttonLanguageIds={{0x38277,0x38278,0}};popup.buttonResults={{1,0,0}};
        popup.initialButton=1;popup.joystickPort=-1;
        Require(menus->screens->SummonPopup(popup)>=0,"Profile I/O error has no actual popup owner");showingError=true;
    }
};

MemcardScreens::MemcardScreens(FrontendPlayerManager& frontend,const char* directory,bool requireNamedProfile)
    :storage_(std::make_unique<Storage>(frontend,directory))
{
    storage_->requireNamedProfile=requireNamedProfile;
    if(!requireNamedProfile)storage_->activeFile=ProfileFilename; // Isolated diagnostic startup.
}
MemcardScreens::~MemcardScreens()
{
    // A destructor may finish an already authorized atomic write, but never
    // dispatch a game transition through an owner which is being retired.
    storage_->completion={};storage_->files.WaitUntilFunctionCompletes();
    if(boundMemcards==this)boundMemcards=nullptr;
}

MemcardScreens::LoadResult MemcardScreens::LoadProfile(const char* filename)
{
    auto& state=*storage_;state.error.clear();state.retryOperation=Storage::Operation::Load;
    const std::string file=filename?filename:state.activeFile;
    const auto failed=[&](const std::string& reason) {
        state.error=reason;
        if(file==state.activeFile)state.blockedByLoadFailure=true;
        state.lastLoad=LoadResult::Failed;
        return state.lastLoad;
    };
    if(state.files.Busy())return failed("A profile transfer is already active");
    if(file.empty())return LoadResult::Absent;
    std::vector<std::uint8_t> bytes;const auto read=state.files.ReadFile(file.c_str(),bytes);
    if(read==cMC::NotFound) {
        state.lastLoad=LoadResult::Absent;
        return state.lastLoad;
    } else if(read!=cMC::Ok)return failed(state.files.LastErrorMessage());
    if(!state.frontend.DeserializeNativeProfileText(bytes.data(),bytes.size(),state.error))return failed(state.error);
    state.blockedByLoadFailure=false;
    state.activeFile=file;
    state.savedProfile=state.frontend.SerializeNativeProfileText();
    ApplyProfileOptions();
    state.lastLoad=LoadResult::Loaded;
    return state.lastLoad;
}

void MemcardScreens::ApplyProfileOptions()
{
    auto& state=*storage_;
    if(state.menus && state.menus->ui && state.menus->ui->language &&
       state.menus->ui->language->currentLanguage!=state.frontend.currentLanguage)
        state.menus->ui->language->SetLanguage(state.frontend.currentLanguage);
    if(state.menus && state.menus->sounds) {
        Require(state.menus->raceSetup && state.menus->raceSetup->parameters &&
            state.menus->ui && state.menus->ui->sound,
            "Loaded audio options have no canonical race or speech-volume settings");
        state.frontend.ApplyAudioOptions(*state.menus->sounds,*state.menus->raceSetup->parameters,*state.menus->ui->sound);
    }
    // Final245D30 load commit restores both saved FRED+178/+17C choices
    // into the existing input router, independently of player inputProfile.
    if(state.menus) {
        Require(state.menus->screens && state.menus->screens->input && state.menus->screens->input->router,
            "Loaded joystick configurations have no canonical input router");
        for(std::int32_t port=0;port<2;++port)
            joystickSetUserConfiguration(state.menus->screens->input->router,port,
                state.frontend.joystickConfigurations[static_cast<std::size_t>(port)]);
    }
    if(state.menus) {
        Require(state.menus->ui && state.menus->ui->clock,
            "Loaded display calibration has no canonical platform clock");
        ApplyDisplayCalibration(state.frontend,*state.menus->ui->clock);
    }
    state.SetCaption();
}

bool MemcardScreens::SaveProfile()
{
    auto& state=*storage_;state.error.clear();state.retryOperation=Storage::Operation::Save;
    if(state.files.Busy()){state.error="A profile transfer is already active";return false;}
    if(!state.PrepareSave())return false;
    Require(state.files.SetState(HP2_BMC_FS_SAVE),"Profile saver cannot schedule its idle file owner");
    state.files.Service();
    if(state.files.lastError!=cMC::Ok) {
        state.error=state.files.LastErrorMessage();return false;
    }

    state.savedProfile=state.data.Text();
    state.SetCaption();return true;
}

bool MemcardScreens::RequestAutosave(std::function<void()> completed)
{
    auto& state=*storage_;
    if(state.files.Busy() || state.operation!=Storage::Operation::None)return false;
    state.error.clear();state.completion=std::move(completed);state.operation=Storage::Operation::Autosave;
    state.retryOperation=Storage::Operation::Autosave;
    if(!state.PrepareSave())return true;
    state.awaitingCallback=true;
    if(!state.files.SetState(HP2_BMC_FS_SAVE,Storage::Completed,&state)) {
        state.awaitingCallback=false;state.error="The file owner rejected the autosave transaction";
    }
    return true;
}

void MemcardScreens::Service()
{
    auto& state=*storage_;
    if(state.files.Busy())state.files.Service();
    if(state.operation!=Storage::Operation::Autosave || state.awaitingCallback)return;
    if(!state.error.empty() || state.files.lastError!=cMC::Ok) {
        if(state.error.empty()) {
            state.error=state.files.LastErrorMessage();
        }
        state.ShowError();return;
    }
    // Completion is detached before the original transition. It may retire
    // a screen or this entire application owner; no member is read afterward.

    state.savedProfile=state.data.Text();
    auto completion=std::move(state.completion);state.operation=Storage::Operation::None;
    if(completion)completion();
}

bool MemcardScreens::Busy() const noexcept
{return storage_->files.Busy() || storage_->operation!=Storage::Operation::None;}
const std::string& MemcardScreens::LastError() const noexcept {return storage_->error;}

bool MemcardScreens::BootNameEntryActive() const noexcept
{
    return storage_->creation!=Storage::Creation::None;
}

bool MemcardScreens::CreateBootProfile(const char* name)
{
    auto& state=*storage_;
    Require(BootNameEntryActive(),"Boot profile creation has no active name editor");
    Require(name && *name && std::strlen(name)<=7,"Boot profile name exceeds the authored keyboard buffer");
    state.error.clear();
    if(Busy()){state.error="A profile transfer is already active";return false;}
    std::string previous, text, filename;
    try {
        previous=state.frontend.SerializeNativeProfileText();
        Require(!state.defaults.empty(),"Profile creation requires the startup defaults");
        if(!state.frontend.DeserializeNativeProfileText(state.defaults.data(),state.defaults.size(),state.error))return false;
        auto& playerName=state.frontend.players[0]->settings.playerName;
        std::fill(std::begin(playerName),std::end(playerName),'\0');
        std::copy_n(name,std::strlen(name),playerName);
        state.frontend.autoSave=1;
        text=state.frontend.SerializeNativeProfileText();
        filename=NewProfileFilename();
        if(state.files.WriteNewFile(filename.c_str(),text.data(),text.size())!=cMC::Ok) {
            state.error=state.files.LastErrorMessage();
        }
    } catch(const std::exception& error) {
        state.error=error.what();
    }
    if(!state.error.empty()) {
        std::string rollbackError;
        if(!previous.empty())Require(state.frontend.DeserializeNativeProfileText(previous.data(),previous.size(),rollbackError),
            "Unable to restore the live profile after a failed creation");
        return false;
    }
    state.activeFile=std::move(filename);
    state.savedProfile=std::move(text);
    state.blockedByLoadFailure=false;
    ApplyProfileOptions();
    return true;
}

void MemcardScreens::FinishBootNameEntry(bool saved)
{
    auto& state=*storage_;
    Require(BootNameEntryActive(),"Boot profile name has no active continuation");
    using Creation=Storage::Creation;
    using Stage=Storage::BootStage;
    const auto origin=std::exchange(state.creation,Creation::None);
    if(origin==Creation::Boot)state.bootStage=saved?Stage::Complete:Stage::Missing;
    if(origin==Creation::StartupPicker && saved)state.bootStage=Stage::Complete;
    const bool skipPicker=saved && (origin==Creation::StartupPicker || origin==Creation::OptionsPicker);
    Require(state.menus->screens->MoveToPreviousScreen(skipPicker?2:1,false,-1),
        "Profile name editor has no retained caller");
}

MemcardScreens* GetBootNameEntryOwner()
{
    return boundMemcards && boundMemcards->BootNameEntryActive()?boundMemcards:nullptr;
}

void MemcardScreens::HandleEvent(ScreenMessage message,ScreenCallbackData& payload)
{
    auto& state=*storage_;
    Require(state.menus && state.menus->screens,"Memcard screen has no actual frontend screen owner");
    auto& screens=*state.menus->screens;
    const auto* name=screens.active && screens.active->configuration?screens.active->configuration->name:nullptr;
    Require(name,"Memcard callback has no original screen configuration");
    const bool boot=!std::strcmp(name,"BootAutoload"),automatic=!std::strcmp(name,"Autosave");
    const bool picker=!std::strcmp(name,"Select Profile");
    const bool manual=!boot && !automatic && !picker;
    using Stage=Storage::BootStage;
    using Creation=Storage::Creation;

    if(message==ScreenMessage::ScreenActivate && boot && state.defaults.empty())
        state.defaults=state.frontend.SerializeNativeProfileText();
    if(message==ScreenMessage::PopupDismiss && state.switchPopup) {
        state.switchPopup=false;
        state.switchDecision=payload.value;
        return;
    }
    if(message==ScreenMessage::Tick && (picker || manual) && state.switchDecision) {
        const auto decision=std::exchange(state.switchDecision,0);
        const auto creation=std::exchange(state.pendingCreation,Creation::None);
        auto filename=std::move(state.pendingFile);
        if(decision==1 && !SaveProfile()) {
            state.ShowNotice("Cannot save the active profile. "+state.error);
        } else if(creation!=Creation::None) {
            state.creation=creation;
            screens.ChangeToScreen("NameEntry",true);
        } else if(LoadProfile(filename.c_str())==LoadResult::Loaded) {
            if(state.pickerAtBoot)state.bootStage=Stage::Loading;
            Require(screens.MoveToPreviousScreen(1,false,-1),"Profile picker has no retained caller");
        } else {
            state.ShowNotice(state.error.empty()?"This profile is no longer available.":"Cannot load the profile. "+state.error);
        }
        return;
    }
    if(picker) {
        const auto row=static_cast<std::size_t>(payload.key-FirstProfileRow);
        if(message==ScreenMessage::ScreenActivate) {
            state.page=std::make_unique<ProfileMenuArt>(*state.menus,PickerScreen);
            state.pickerSelection=std::min(state.pickerSelection,state.profiles.size());
            state.page->BuildPicker(state.ProfileCaptions(),state.pickerSelection);
            state.highlight=std::make_unique<FlailerHighlightAnimation>(*state.menus->ui,PickerScreen,0xab671c9au,
                state.menus->ui->animationSettings.highlightColor);
            payload.value=static_cast<std::int32_t>(state.pickerSelection+1);
        } else if(message==ScreenMessage::ScreenDeactivate) {
            state.highlight.reset();
            state.page.reset();
        } else if(message==ScreenMessage::Tick) {
            if(!state.page->AnimatePicker(state.menus->ui->clock->realTimeElapsed))state.highlight->Animate();
        } else if(message==ScreenMessage::Highlight) {
            state.PositionPicker(row);
        } else if((message==ScreenMessage::Previous && row==0) ||
                  (message==ScreenMessage::Next && row==state.profiles.size())) {
            payload.value=1;
        } else if(message==ScreenMessage::LeaveScreen && (state.pickerAtBoot || Busy())) {
            payload.value=0;payload.sound=0;
        } else if(message==ScreenMessage::Select && row<=state.profiles.size() && !Busy()) {
            const auto index=row;
            if(index==state.profiles.size()) {
                state.RequestSwitch({},state.pickerAtBoot?Creation::StartupPicker:Creation::OptionsPicker);
            } else if(index<state.profiles.size()) {
                const auto& profile=state.profiles[index];
                if(profile.valid)state.RequestSwitch(profile.file);
                else state.ShowNotice("This profile could not be read. Its file has been kept.");
            }
        }
        return;
    }
    if(boot && state.requireNamedProfile) {
        if(message==ScreenMessage::ScreenActivate) {
            screens.StopMovie();
            if(state.bootStage==Stage::Missing)
                state.ShowBootPopup(ContinuePopup,"No saved profile was found. Create a profile to continue.",true);
            else if(state.bootStage==Stage::Loading)
                state.ShowBootStatus(0x24dd9);
            else if(state.bootStage!=Stage::Complete) {
                state.bootStage=Stage::Checking;
                state.ShowBootStatus(0x24de5);
            }
        } else if(message==ScreenMessage::Tick) {
            if(state.bootStage==Stage::Checking && state.BootStatusElapsed()) {
                const auto scanned=state.ScanProfiles();
                if(scanned && state.profiles.size()==1 && state.profiles.front().valid &&
                   LoadProfile(state.profiles.front().file.c_str())==LoadResult::Loaded) {
                    state.bootStage=Stage::Loading;
                    state.ShowBootStatus(0x24dd9);
                } else if(scanned && state.profiles.empty()) {
                    state.bootStage=Stage::Missing;
                    state.ShowBootPopup(ContinuePopup,"No saved profile was found. Create a profile to continue.",true);
                } else if(scanned) {
                    screens.DismissPopup(state.bootPopup);
                    state.bootStage=Stage::Choosing;
                    state.pickerSelection=0;
                    state.pickerAtBoot=true;
                    screens.ChangeToScreen("Select Profile",true);
                } else {
                    state.bootStage=Stage::LoadError;
                    state.ShowBootPopup(ContinuePopup,"Cannot check the saved profiles. "+state.error+
                        "\nPress Continue to retry.",true);
                }
            } else if(state.bootStage==Stage::Missing && state.bootPopup<0) {
                state.creation=Creation::Boot;
                screens.ChangeToScreen("NameEntry",true);
            } else if(state.bootStage==Stage::LoadError && state.bootPopup<0) {
                state.bootStage=Stage::Checking;
                state.ShowBootStatus(0x24de5);
            } else if(state.bootStage==Stage::Complete ||
                (state.bootStage==Stage::Loading && state.BootStatusElapsed())) {
                state.bootStage=Stage::Idle;
                if(state.bootPopup>=0)screens.DismissPopup(state.bootPopup);
                state.bootPopup=-1;
                Require(screens.MoveToPreviousScreen(1,false,-1),"Boot autoload has no retained startup screen");
            }
        } else if(message==ScreenMessage::PopupDismiss)state.bootPopup=-1;
        else if(message==ScreenMessage::LeaveScreen){payload.value=0;payload.sound=0;}
        return;
    }
    // Explicit native-PC first-run policy: only a genuinely absent profile
    // enables autosave. Existing preferences and invalid files are retained.
    const auto loadBootProfile=[this,&state] {
        const auto loaded=LoadProfile();state.returnAfterTick=loaded==LoadResult::Loaded;
        if(loaded==LoadResult::Failed) {state.ShowError();return;}
        if(loaded==LoadResult::Absent) {
            state.creatingBootProfile=true;state.frontend.autoSave=1;
            Require(RequestAutosave([this] {
                storage_->creatingBootProfile=false;storage_->returnAfterTick=true;
            }),"First-run profile cannot schedule its actual file owner");
        }
    };
    if(message==ScreenMessage::ScreenActivate) {
        if(boot) {
            loadBootProfile();
        } else if(automatic && !Busy()) {
            Require(RequestAutosave([this] {
                auto* menus=storage_->menus;
                Require(menus && menus->screens->MoveToPreviousScreen(1,false,-1),"Autosave has no previous original screen");
            }),"Autosave screen could not schedule its transfer");
        } else if(manual) {
            state.page=std::make_unique<ProfileMenuArt>(*state.menus,ManualScreen);
            state.page->AddCreateRow();
            if(!state.activeFile.empty()) {
                const auto& player=state.frontend.players[0]->settings;
                state.page->Relabel(FirstRow,"Save ("+std::string(player.playerName)+")");
            }
            state.SetCaption();
        }
        return;
    }
    if(message==ScreenMessage::ScreenDeactivate && manual) {state.page.reset();return;}
    if(message==ScreenMessage::Tick) {
        if(state.returnAfterTick) {
            state.returnAfterTick=false;
            Require(screens.MoveToPreviousScreen(1,false,-1),"Boot autoload has no next original startup screen");return;
        }
        Service();return;
    }
    if(message==ScreenMessage::PopupDismiss && state.showingError) {
        state.showingError=false;
        if(payload.value) {
            if(boot && !state.creatingBootProfile)loadBootProfile();
            else if(automatic || (boot && state.creatingBootProfile)) {
                auto continuation=std::move(state.completion);state.operation=Storage::Operation::None;
                Require(RequestAutosave(std::move(continuation)),"Retry cannot schedule its actual file owner");
            } else if(!SaveProfile())state.ShowError();
        } else {
            // Explicit user cancellation carries no save success and cannot
            // run a deferred coordinator action. Return to the prior screen.
            state.completion={};state.operation=Storage::Operation::None;
            if(state.creatingBootProfile) {state.creatingBootProfile=false;state.frontend.autoSave=0;}
            if(automatic || boot)Require(screens.MoveToPreviousScreen(1,false,-1),"Cancelled profile transfer has no prior screen");
        }
        return;
    }
    if(message==ScreenMessage::LeaveScreen && Busy()){payload.value=0;payload.sound=0;return;}
    if(automatic || boot)return;
    if(message==ScreenMessage::Select) {
        if(payload.key==FirstRow) {if(!SaveProfile())state.ShowError();}
        else if(payload.key==FirstRow+1) {
            if(!state.ScanProfiles())state.ShowNotice("Cannot check the saved profiles. "+state.error);
            else {
                state.pickerSelection=0;
                state.pickerAtBoot=false;
                screens.ChangeToScreen("Select Profile",true);
            }
        } else if(payload.key==FirstRow+3)state.RequestSwitch({},Creation::Options);
    } else if((message==ScreenMessage::Left || message==ScreenMessage::Right) && payload.key==FirstRow+2) {
        state.frontend.autoSave=!state.frontend.autoSave;state.SetCaption();payload.sound=2;
        if(state.frontend.autoSave && !SaveProfile())state.ShowError();
    }
}

void BindMemcardScreens(MemcardScreens& owner,MainMenuScreenServices& services)
{
    Require(!boundMemcards || boundMemcards==&owner,"Another MemcardScreens owner is already bound");
    Require(services.frontend==&owner.storage_->frontend && services.ui && services.screens,
        "Memcard screen binding is not the live application profile/UI");
    owner.storage_->menus=&services;boundMemcards=&owner;
}
void UnbindMemcardScreens()
{
    if(boundMemcards) {
        boundMemcards->storage_->highlight.reset();
        boundMemcards->storage_->page.reset();
    }
    boundMemcards=nullptr;
}

bool RequestProfileAutosave(std::function<void()> completed,std::function<bool()> ownerAlive)
{
    Require(boundMemcards,"Autosave has no actual native profile/file owner");
    auto* owner=boundMemcards;auto* menus=owner->storage_->menus;
    Require(menus && menus->screens,"Autosave continuation has no actual original screen stack");
    const auto queued=owner->RequestAutosave([menus,completed=std::move(completed),ownerAlive=std::move(ownerAlive)]() mutable {
        // An authorised write may finish after its phase retired. Guard the
        // old screen stack before any access, not just the final continuation.
        if(ownerAlive && !ownerAlive())return;
        const auto* node=menus->screens->active;
        if(node && node->configuration && !std::strcmp(node->configuration->name,"Autosave"))
            Require(menus->screens->MoveToPreviousScreen(1,false,-1),"Completed autosave has no retained previous screen");
        if(completed)completed();
    });
    if(queued)menus->screens->ChangeToScreen("Autosave",true);
    return queued;
}

std::array<FlailerScreenConfig,4>& MemcardScreenConfigurations()
{
    // Actual PAL247DE8 constructor arguments, callbacks and final strings.
    // PC I/O completion replaces the Sony polling states, never fake success.
    static std::array<FlailerScreenConfig,4> configurations{{
        {"Phils Load Save",MemcardScreenCallback,{{static_cast<std::int32_t>(ManualScreen),0,0,0}},{{-1,-1,-1,-1}},1,LoadSaveHelp.data()},
        {"BootAutoload",MemcardScreenCallback,{{static_cast<std::int32_t>(TransferScreen),0,0,0}},{{-1,-1,-1,-1}},1,nullptr},
        {"Autosave",MemcardScreenCallback,{{static_cast<std::int32_t>(TransferScreen),0,0,0}},{{-1,-1,-1,-1}},1,nullptr},
        {"Select Profile",MemcardScreenCallback,{{static_cast<std::int32_t>(PickerScreen),0,0,0}},{{-1,-1,-1,-1}},1,LoadSaveHelp.data()}
    }};
    return configurations;
}
void MemcardScreenCallback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(boundMemcards && data,"Memcard callback has no retained process owner/payload");
    boundMemcards->HandleEvent(message,*data);
}
} // namespace hp2
