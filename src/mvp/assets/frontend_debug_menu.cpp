#include "frontend_debug_menu.hpp"

#include "bware/inc/bWare.hpp"
#include "indep/inc/Flags.hpp"
#include "indep/inc/Fred.hpp"
#include "indep/inc/GameFlow.hpp"
#include "indep/inc/Language.hpp"
#include "indep/inc/Track.hpp"

#include <algorithm>
#include <utility>

namespace hp2 {
namespace {
constexpr std::uint32_t art = 0x95ad096bu; // Options artwork.
constexpr std::uint32_t demoArt = 0x8bc68340u; // Authored settings artwork with left/right arrows.
constexpr std::array<std::uint32_t, 3> items{0x52e039f3u, 0x52e039f4u, 0x52e039f5u};
constexpr std::array<std::uint32_t, 8> masks{0x6e34e350u, 0x708a0bf1u, 0x72df3492u, 0x75345d33u,
    0x778985d4u, 0x79deae75u, 0x7c33d716u, 0x7e88ffb7u};
constexpr std::array<std::uint32_t, 2> arrows{0xd1cda594u, 0x9345e4a7u};
constexpr std::uint32_t trackValue = 0x55cb09f2u;
constexpr std::array<std::uint32_t, 2> rails{0x32c91daeu, 0xc671680cu};
constexpr std::array<const char*, 3> labels{"Track Viewer", "Car Viewer", "Demo Scene"};
constexpr std::array<const char*, 3> descriptions{
    "Explore the original tracks.", "Inspect cars.", "Choose a track and watch the demo."};
}

FrontendDebugMenu* FrontendDebugMenu::active_{};

FrontendDebugMenu::FrontendDebugMenu(std::function<void()> openTrackViewer)
    : screen_{"Debug", callback, {{static_cast<std::int32_t>(art), 0, 0, 0}},
          {{-1, -1, -1, -1}}, 1, nullptr},
      demoScreen_{"Demo Scene", demoCallback, {{static_cast<std::int32_t>(demoArt), 0, 0, 0}},
          {{-1, -1, -1, -1}}, 1, nullptr},
      extension_{"Debug", "Open the Debug menu.", &screen_,
          [](MainMenuScreenServices& menus, void* context) {
              active_ = static_cast<FrontendDebugMenu*>(context);
              active_->menus_ = &menus;
          },
          [](void* context) { static_cast<FrontendDebugMenu*>(context)->service(); }, this, &demoScreen_},
      openTrackViewer_(std::move(openTrackViewer)) {}

FrontendDebugMenu::~FrontendDebugMenu() {
    if (active_ == this) active_ = nullptr;
}

void FrontendDebugMenu::callback(ScreenMessage message, ScreenCallbackData* data) {
    if (active_ && data) active_->handle(message, *data);
}

void FrontendDebugMenu::demoCallback(ScreenMessage message, ScreenCallbackData* data) {
    if (active_ && data) active_->handleDemo(message, *data);
}

void FrontendDebugMenu::relabel(FlailerTextChunk& text, const char* label) {
    originalText_.push_back({&text, text.languageId, text.text ? text.text : ""});
    text.SetString(label);
}

void FrontendDebugMenu::updateDemoTrackLabel() {
    auto& ui = *menus_->ui;
    const auto trackId = demoTracks_[demoTrackIndex_];
    std::string label = "Automatic";
    if (const auto* track = menus_->raceSetup->tracks->Find(trackId)) {
        const auto* name = ui.language->GetString(
            GetTheRightTrackLanguageID(track, menus_->raceSetup->applicationOptions->demoTrackDirection));
        label = std::to_string(trackId) + " " + (name && *name ? name : track->name);
    }
    ui.resources->SetLocalizedText(demoArt, trackValue, label.c_str());
}

void FrontendDebugMenu::restorePage(std::uint32_t pageArt) {
    auto& resources = *menus_->ui->resources;
    pending_ = Action::None;
    for (const auto& saved : originalText_) {
        saved.object->SetString(saved.text.c_str());
        if (saved.language) saved.object->SetLanguageID(saved.language);
    }
    originalText_.clear();
    for (const auto& [object, visible] : originalVisibility_)
        resources.SetObjectVisibility(pageArt, object->key, visible);
    originalVisibility_.clear();
    for (std::size_t i = 0; i < rails.size(); ++i)
        resources.FindObject(pageArt, rails[i])->SetTopLeftY(originalRails_[i]);
    highlight_.reset();
    blurb_.reset();
}

void FrontendDebugMenu::positionRails(std::uint32_t pageArt, std::uint32_t mask) {
    const auto& ui = *menus_->ui;
    std::int32_t left, top, right, bottom;
    if (ui.resources->GetBoundingBox(pageArt, mask, left, top, right, bottom, ui.language)) {
        ui.resources->FindObject(pageArt, rails[0])->SetTopLeftY(static_cast<float>(bottom));
        ui.resources->FindObject(pageArt, rails[1])->SetTopLeftY(static_cast<float>(top) - 6.0f);
    }
    selection_.SetCurrentItem(mask);
}

void FrontendDebugMenu::cycleDemoTrack(int direction) {
    demoTrackIndex_ = direction < 0 ?
        (demoTrackIndex_ == 0 ? demoTracks_.size() - 1 : demoTrackIndex_ - 1) :
        (demoTrackIndex_ + 1) % demoTracks_.size();
    updateDemoTrackLabel();
}

void FrontendDebugMenu::handle(ScreenMessage message, ScreenCallbackData& data) {
    auto& ui = *menus_->ui;
    auto& resources = *ui.resources;
    auto& screens = *menus_->screens;
    const auto found = std::find(items.begin(), items.end(), data.key);
    const auto index = static_cast<std::size_t>(found - items.begin());
    switch (message) {
    case ScreenMessage::ScreenActivate: {
        screens.StartMovie("MOVIES\\options_movie_new.AVI");
        selection_ = FlailerSelectionSet{};
        selection_.showOtherItems = 1;
        selection_.AddItems(resources, art, masks.data(), static_cast<int>(items.size()));
        for (std::size_t i = 0; i < items.size(); ++i) {
            auto* text = static_cast<FlailerTextChunk*>(resources.FindObject(art, items[i], 1));
            Require(text, "Debug menu requires the Options row artwork");
            relabel(*text, labels[i]);
        }
        auto* source = resources.FindScreen(art);
        // The Options header is the anonymous text in the top band.
        for (auto* object = source->objects.next; object != &source->objects; object = object->next) {
            if (object->key == 0x6eb9985du && object->kind == 1 && object->topLeftY < 29.0f) {
                relabel(*static_cast<FlailerTextChunk*>(object), screen_.name);
                break;
            }
        }
        for (std::size_t i = items.size(); i < masks.size(); ++i) {
            const auto key = items.front() + static_cast<std::uint32_t>(i);
            if (auto* item = screens.active->screen->menu->FindItem(key)) item->ClearMenuItemState(1);
            for (const auto objectKey : {key, masks[i]}) {
                if (auto* object = resources.FindObject(art, objectKey)) {
                    originalVisibility_.emplace_back(object, object->visible);
                    resources.SetObjectVisibility(art, objectKey, 0);
                }
            }
        }
        data.value = 1;
        for (std::size_t i = 0; i < rails.size(); ++i) {
            auto* rail = resources.FindObject(art, rails[i]);
            Require(rail, "Debug menu requires the Options highlight rails");
            originalRails_[i] = rail->topLeftY;
        }
        blurb_.emplace(ui, art, 0x75980cf0u, 0x75980cf0u, 0x70a404ecu);
        highlight_.emplace(ui, art, 0xab671c9au, ui.animationSettings.highlightColor);
        break;
    }
    case ScreenMessage::ScreenDeactivate:
        restorePage(art);
        break;
    case ScreenMessage::Highlight:
        if (index < items.size()) {
            positionRails(art, masks[index]);
            highlight_->SetCurrentMenuItem(data.key);
            blurb_->NewText(descriptions[index]);
        }
        break;
    case ScreenMessage::Tick:
        blurb_->Animate();
        highlight_->Animate();
        break;
    case ScreenMessage::Select:
        if (screens.IsAnyPopupActive() || pending_ != Action::None) break;
        if (index == 1) {
            FlailerPopupConfig popup;
            popup.sourceNode = screens.active;
            popup.screenKey = 0x74d2dfa2u; // Authored one-button HP2 dialog.
            popup.message = "Not Implemented";
            popup.buttonCount = 1;
            popup.buttonLanguageIds[0] = 0x24a55;
            popup.flags = 2;
            popup.joystickPort = -1;
            Require(screens.SummonPopup(popup) >= 0, "Debug menu could not open the HP2 dialog");
        } else if (index < items.size()) {
            pending_ = index == 0 ? Action::TrackViewer : Action::OpenDemo;
            pendingEpoch_ = screens.pointerEpoch;
        }
        break;
    case ScreenMessage::PopupActivate:
        data.sound = 5;
        break;
    case ScreenMessage::PopupDismiss:
        data.sound = 6;
        break;
    default:
        break;
    }
}

void FrontendDebugMenu::handleDemo(ScreenMessage message, ScreenCallbackData& data) {
    auto& ui = *menus_->ui;
    auto& resources = *ui.resources;
    auto& screens = *menus_->screens;
    switch (message) {
    case ScreenMessage::ScreenActivate: {
        screens.StartMovie("MOVIES\\options_movie_new.AVI");
        if (demoTracks_.empty()) {
            demoTracks_.push_back(-1);
            const auto& tracks = *menus_->raceSetup->tracks;
            for (std::size_t id = 1; id <= tracks.Catalog().scenarioCount; ++id) {
                const auto* track = tracks.Find(static_cast<std::int32_t>(id));
                if (track && track->routeKind == 0)
                    demoTracks_.push_back(static_cast<std::int32_t>(id));
            }
        }
        auto* source = resources.FindScreen(demoArt);
        Require(source && screens.active->screen->menu, "Demo Scene requires settings artwork and menu rows");
        const auto hide = [&](std::uint32_t key) {
            if (auto* object = resources.FindObject(demoArt, key)) {
                originalVisibility_.emplace_back(object, object->visible);
                resources.SetObjectVisibility(demoArt, key, 0);
            }
        };
        constexpr std::array valuesToHide{0x2e931f1bu, 0x03d90172u, 0x5610f2f4u, 0x422313cbu, 0x8fe4086bu,
            0xedf2e4eeu, 0x20d73d01u};
        for (auto key : valuesToHide) hide(key);
        for (std::size_t i = 2; i < masks.size(); ++i) {
            const auto key = items.front() + static_cast<std::uint32_t>(i);
            if (auto* item = screens.active->screen->menu->FindItem(key)) item->ClearMenuItemState(1);
            hide(key);
            hide(masks[i]);
        }
        const auto calibration = items.front() + 8;
        if (auto* item = screens.active->screen->menu->FindItem(calibration)) item->ClearMenuItemState(1);
        hide(calibration);
        const auto text = [&](std::uint32_t key) -> FlailerTextChunk& {
            auto* object = resources.FindObject(demoArt, key, 1);
            Require(object, "Demo Scene requires an authored text field");
            return *static_cast<FlailerTextChunk*>(object);
        };
        relabel(text(items[0]), "");
        resources.SetLocalizedText(demoArt, items[0], "Select Track");
        relabel(text(items[1]), "Start Scene");
        relabel(text(trackValue), "");
        for (auto* object = source->objects.next; object != &source->objects; object = object->next) {
            if (object->key == 0x6eb9985du && object->kind == 1 && object->topLeftY < 40.0f) {
                relabel(*static_cast<FlailerTextChunk*>(object), demoScreen_.name);
                break;
            }
        }
        for (std::size_t i = 0; i < arrows.size(); ++i) {
            auto* arrow = resources.FindObject(demoArt, arrows[i]);
            Require(arrow, "Demo Scene requires the authored track arrows");
            originalArrowY_[i] = arrow->topLeftY;
            originalVisibility_.emplace_back(arrow, arrow->visible);
        }
        for (std::size_t i = 0; i < rails.size(); ++i)
            originalRails_[i] = resources.FindObject(demoArt, rails[i])->topLeftY;
        selection_ = FlailerSelectionSet{};
        selection_.showOtherItems = 1;
        selection_.AddItems(resources, demoArt, masks.data(), 2);
        demoArrows_.emplace(ui, demoArt, arrows[0], arrows[1]);
        blurb_.emplace(ui, demoArt, 0x75980cf0u, 0x75980cf0u, 0x70a404ecu);
        highlight_.emplace(ui, demoArt, 0xab671c9au, ui.animationSettings.highlightColor);
        updateDemoTrackLabel();
        data.value = 1;
        break;
    }
    case ScreenMessage::ScreenDeactivate:
        demoArrows_.reset();
        for (std::size_t i = 0; i < arrows.size(); ++i)
            resources.FindObject(demoArt, arrows[i])->SetTopLeftY(originalArrowY_[i]);
        restorePage(demoArt);
        break;
    case ScreenMessage::Highlight:
        if (data.key == items[0] || data.key == items[1]) {
            const bool trackSelected = data.key == items[0];
            positionRails(demoArt, masks[trackSelected ? 0 : 1]);
            highlight_->SetCurrentMenuItem(data.key);
            demoArrows_->SetCurrentItem(trackSelected ? trackValue : 0);
            blurb_->NewText(trackSelected ? "Use the arrows to choose a track." : "Watch the demo on the selected track.");
        }
        break;
    case ScreenMessage::Tick:
        demoArrows_->Animate();
        blurb_->Animate();
        highlight_->Animate();
        break;
    case ScreenMessage::Left:
    case ScreenMessage::Right:
        if (data.key == items[0]) {
            cycleDemoTrack(message == ScreenMessage::Left ? -1 : 1);
            data.sound = message == ScreenMessage::Left ? 1 : 2;
        }
        break;
    case ScreenMessage::Select:
        if (data.key == items[1] && pending_ == Action::None) {
            pending_ = Action::StartDemo;
            pendingDemoTrack_ = demoTracks_[demoTrackIndex_];
            pendingEpoch_ = screens.pointerEpoch;
        }
        break;
    default:
        break;
    }
}

void FrontendDebugMenu::service() {
    auto& screens = *menus_->screens;
    const auto action = std::exchange(pending_, Action::None);
    if (action == Action::None || pendingEpoch_ != screens.pointerEpoch ||
        !screens.active || screens.active->configuration != (action == Action::StartDemo ? &demoScreen_ : &screen_) ||
        screens.IsAnyPopupActive() || menus_->raceSetup->flow->requestedState != 3)
        return;
    if (action == Action::TrackViewer) openTrackViewer_();
    else if (action == Action::OpenDemo) screens.ChangeToScreen(demoScreen_.name, true);
    else menus_->demoMode->RequestDemoMode(pendingDemoTrack_);
}

}
