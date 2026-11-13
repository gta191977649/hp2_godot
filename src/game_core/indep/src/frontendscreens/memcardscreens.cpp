#include "FMenus.hpp"
#include "Fred.hpp"
#include "Language.hpp"
#include "Flags.hpp"
#include "inc/dbFiles.hpp"
#include "bware/inc/bWare.hpp"
#include <cstring>
#include <utility>

namespace hp2 {
namespace {
constexpr const char* ProfileFilename="profile.json";
constexpr const char* ProfileBackupFilename="profile.backup.json";
constexpr const char* LegacyProfileFilename="settings.db";
constexpr std::uint32_t ManualScreen=0x9829bfabu;
constexpr std::uint32_t TransferScreen=0xfa2ac5c6u;
constexpr std::array<std::int32_t,4> LoadSaveHelp{{0xc3546,0xc3547,0xc3548,0}};
MemcardScreens* boundMemcards{};
// PAL245640..2457C0 SavegameData owns the detached transfer, while the
// original live FRED owns progress. PC files now carry named UTF-8 JSON;
// the legacy binary image is accepted only by the migration reader.
class SavegameData final {
public:
    explicit SavegameData(FrontendPlayerManager& frontend):frontend_(frontend) {}
    void* SetupForSave() {text_=frontend_.SerializeNativeProfileText();return text_.data();}
    std::int32_t Size() const {return static_cast<std::int32_t>(text_.size());}
private:
    FrontendPlayerManager& frontend_;
    std::string text_;
};
}

struct MemcardScreens::Storage {
    FrontendPlayerManager& frontend;
    cDBFileSys files;
    SavegameData data;
    std::unique_ptr<cMC> legacyFiles;
    MainMenuScreenServices* menus{};
    std::function<void()> completion;
    std::string error,popupMessage;
    enum class Operation {None,Save,Load,Autosave};
    Operation operation{Operation::None};
    Operation retryOperation{Operation::None};
    bool awaitingCallback{},showingError{},returnAfterTick{},creatingBootProfile{},blockedByLoadFailure{};
    LoadResult lastLoad{LoadResult::Absent};
    Storage(FrontendPlayerManager& owner,const char* directory,const char* legacyDirectory):frontend(owner),files(directory),data(owner) {
        if(legacyDirectory && *legacyDirectory)legacyFiles=std::make_unique<cMC>(legacyDirectory);
    }
    bool PrepareSave() {
        if(blockedByLoadFailure) {
            error="The previous profile load failed. Restore or correct that file and load it successfully before saving; existing progress was retained.";
            return false;
        }
        try {
            std::vector<std::uint8_t> previous;const auto read=files.ReadFile(ProfileFilename,previous);
            if(read!=cMC::Ok && read!=cMC::NotFound) {
                error=files.LastErrorMessage();
                return false;
            }
            if(read==cMC::Ok && !frontend.ValidateNativeProfileText(previous.data(),previous.size(),error)) {
                error="Existing profile.json was retained because it failed validation: "+error;
                return false;
            }
            // Serialize before changing either durable file. The I/O owner
            // borrows this detached string until its one completion callback.
            void* transfer=data.SetupForSave();
            files.SetBackupFile(read==cMC::Ok?ProfileBackupFilename:nullptr,std::move(previous));
            files.SetFileData(ProfileFilename,transfer,data.Size());return true;
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

MemcardScreens::MemcardScreens(FrontendPlayerManager& frontend,const char* directory,const char* legacyDirectory)
    :storage_(std::make_unique<Storage>(frontend,directory,legacyDirectory)) {}
MemcardScreens::~MemcardScreens()
{
    // A destructor may finish an already authorized atomic write, but never
    // dispatch a game transition through an owner which is being retired.
    storage_->completion={};storage_->files.WaitUntilFunctionCompletes();
    if(boundMemcards==this)boundMemcards=nullptr;
}

MemcardScreens::LoadResult MemcardScreens::LoadProfile()
{
    auto& state=*storage_;state.error.clear();state.retryOperation=Storage::Operation::Load;
    const auto failed=[&](const std::string& reason) {
        state.error=reason;state.blockedByLoadFailure=true;state.lastLoad=LoadResult::Failed;
        return state.lastLoad;
    };
    if(state.files.Busy())return failed("A profile transfer is already active");
    std::vector<std::uint8_t> bytes;auto read=state.files.ReadFile(ProfileFilename,bytes);
    if(read==cMC::NotFound) {
        // Only a genuinely absent primary may recover its previous durable
        // text version. An existing malformed file is reported and retained;
        // it must never be mistaken for a fresh profile or silently rolled back.
        read=state.files.ReadFile(ProfileBackupFilename,bytes);
        if(read==cMC::Ok) {
            if(!state.frontend.ValidateNativeProfileText(bytes.data(),bytes.size(),state.error))return failed(state.error);
            if(state.files.WriteNewFile(ProfileFilename,bytes.data(),bytes.size())!=cMC::Ok)return failed(state.files.LastErrorMessage());
        } else if(read==cMC::NotFound) {
            read=state.files.ReadFile(LegacyProfileFilename,bytes,GAME_SAVE_SIZE);
            if(read==cMC::NotFound && state.legacyFiles) {
                read=state.legacyFiles->MCReadFile(LegacyProfileFilename,bytes,GAME_SAVE_SIZE);
                if(read!=cMC::Ok && read!=cMC::NotFound)return failed(state.legacyFiles->LastError());
            }
            if(read==cMC::NotFound) {
                state.blockedByLoadFailure=false;state.lastLoad=LoadResult::Absent;
                return state.lastLoad;
            }
            if(read!=cMC::Ok)return failed(state.files.LastErrorMessage());
            std::string text;
            if(!state.frontend.ConvertLegacyProfileToText(bytes.data(),bytes.size(),text,state.error))return failed(state.error);
            if(state.files.WriteNewFile(ProfileFilename,text.data(),text.size())!=cMC::Ok)return failed(state.files.LastErrorMessage());
            bytes.assign(text.begin(),text.end());
        } else return failed(state.files.LastErrorMessage());
    } else if(read!=cMC::Ok)return failed(state.files.LastErrorMessage());
    if(!state.frontend.DeserializeNativeProfileText(bytes.data(),bytes.size(),state.error))return failed(state.error);
    state.blockedByLoadFailure=false;

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
    state.SetCaption();state.lastLoad=LoadResult::Loaded;return state.lastLoad;
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

    auto completion=std::move(state.completion);state.operation=Storage::Operation::None;
    if(completion)completion();
}

bool MemcardScreens::Busy() const noexcept
{return storage_->files.Busy() || storage_->operation!=Storage::Operation::None;}
const std::string& MemcardScreens::LastError() const noexcept {return storage_->error;}

void MemcardScreens::HandleEvent(ScreenMessage message,ScreenCallbackData& payload)
{
    auto& state=*storage_;
    Require(state.menus && state.menus->screens,"Memcard screen has no actual frontend screen owner");
    auto& screens=*state.menus->screens;
    const auto* name=screens.active && screens.active->configuration?screens.active->configuration->name:nullptr;
    Require(name,"Memcard callback has no original screen configuration");
    const bool boot=!std::strcmp(name,"BootAutoload"),automatic=!std::strcmp(name,"Autosave");
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
        } else state.SetCaption();
        return;
    }
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
            } else if(state.retryOperation==Storage::Operation::Load) {
                const auto loaded=LoadProfile();
                if(loaded==LoadResult::Absent)state.error="No saved native profile exists";
                if(loaded!=LoadResult::Loaded)state.ShowError();
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
        if(payload.key==0x52e039f3u) {if(!SaveProfile())state.ShowError();}
        else if(payload.key==0x52e039f4u) {
            const auto loaded=LoadProfile();
            if(loaded==LoadResult::Absent)state.error="No saved native profile exists";
            if(loaded!=LoadResult::Loaded)state.ShowError();
        }
    } else if((message==ScreenMessage::Left || message==ScreenMessage::Right) && payload.key==0x52e039f5u) {
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
void UnbindMemcardScreens() {boundMemcards=nullptr;}

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

std::array<FlailerScreenConfig,3>& MemcardScreenConfigurations()
{
    // Actual PAL247DE8 constructor arguments, callbacks and final strings.
    // PC I/O completion replaces the Sony polling states, never fake success.
    static std::array<FlailerScreenConfig,3> configurations{{
        {"Phils Load Save",MemcardScreenCallback,{{static_cast<std::int32_t>(ManualScreen),0,0,0}},{{-1,-1,-1,-1}},1,LoadSaveHelp.data()},
        {"BootAutoload",MemcardScreenCallback,{{static_cast<std::int32_t>(TransferScreen),0,0,0}},{{-1,-1,-1,-1}},1,nullptr},
        {"Autosave",MemcardScreenCallback,{{static_cast<std::int32_t>(TransferScreen),0,0,0}},{{-1,-1,-1,-1}},1,nullptr}
    }};
    return configurations;
}
void MemcardScreenCallback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(boundMemcards && data,"Memcard callback has no retained process owner/payload");
    boundMemcards->HandleEvent(message,*data);
}
} // namespace hp2
