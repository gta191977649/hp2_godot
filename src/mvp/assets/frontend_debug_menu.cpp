#include "frontend_debug_menu.hpp"

#include "bware/inc/bWare.hpp"
#include "indep/inc/Fred.hpp"
#include "indep/inc/GameFlow.hpp"

#include <algorithm>
#include <utility>

namespace hp2 {
namespace {
constexpr std::uint32_t art = 0x95ad096bu; // Options artwork.
constexpr std::array<std::uint32_t, 3> items{0x52e039f3u, 0x52e039f4u, 0x52e039f5u};
constexpr std::array<std::uint32_t, 3> masks{0x6e34e350u, 0x708a0bf1u, 0x72df3492u};
constexpr std::array<std::uint32_t, 5> unusedMasks{
    0x75345d33u, 0x778985d4u, 0x79deae75u, 0x7c33d716u, 0x7e88ffb7u};
constexpr std::array<std::uint32_t, 2> rails{0x32c91daeu, 0xc671680cu};
constexpr std::array<const char*, 3> labels{"Track Viewer", "Car Viewer", "View Demo Scene"};
constexpr std::array<const char*, 3> descriptions{
    "Explore the original tracks.", "Inspect cars.", "Watch the original attract demo."};
}

FrontendDebugMenu* FrontendDebugMenu::active_{};

FrontendDebugMenu::FrontendDebugMenu(std::function<void()> openTrackViewer)
    : screen_{"Debug", callback, {{static_cast<std::int32_t>(art), 0, 0, 0}},
          {{-1, -1, -1, -1}}, 1, nullptr},
      extension_{"Debug", "Open the Debug menu.", &screen_,
          [](MainMenuScreenServices& menus, void* context) {
              active_ = static_cast<FrontendDebugMenu*>(context);
              active_->menus_ = &menus;
          },
          [](void* context) { static_cast<FrontendDebugMenu*>(context)->service(); }, this},
      openTrackViewer_(std::move(openTrackViewer)) {}

FrontendDebugMenu::~FrontendDebugMenu() {
    if (active_ == this) active_ = nullptr;
}

void FrontendDebugMenu::callback(ScreenMessage message, ScreenCallbackData* data) {
    if (active_ && data) active_->handle(message, *data);
}

void FrontendDebugMenu::relabel(FlailerTextChunk& text, const char* label) {
    originalText_.push_back({&text, text.languageId, text.text ? text.text : ""});
    text.SetString(label);
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
        selection_.AddItems(resources, art, masks.data(), static_cast<int>(masks.size()));
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
        for (std::size_t i = 0; i < unusedMasks.size(); ++i) {
            const auto key = items.back() + 1 + i;
            if (auto* item = screens.active->screen->menu->FindItem(key)) item->ClearMenuItemState(1);
            for (const auto objectKey : {static_cast<std::uint32_t>(key), unusedMasks[i]}) {
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
        pending_ = Action::None;
        for (const auto& saved : originalText_) {
            saved.object->SetString(saved.text.c_str());
            if (saved.language) saved.object->SetLanguageID(saved.language);
        }
        originalText_.clear();
        for (const auto& [object, visible] : originalVisibility_)
            resources.SetObjectVisibility(art, object->key, visible);
        originalVisibility_.clear();
        for (std::size_t i = 0; i < rails.size(); ++i)
            resources.FindObject(art, rails[i])->SetTopLeftY(originalRails_[i]);
        highlight_.reset();
        blurb_.reset();
        break;
    case ScreenMessage::Highlight:
        if (index < items.size()) {
            std::int32_t left, top, right, bottom;
            if (resources.GetBoundingBox(art, masks[index], left, top, right, bottom, ui.language)) {
                resources.FindObject(art, rails[0])->SetTopLeftY(static_cast<float>(bottom));
                resources.FindObject(art, rails[1])->SetTopLeftY(static_cast<float>(top) - 6.0f);
            }
            selection_.SetCurrentItem(masks[index]);
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
            pending_ = index == 0 ? Action::TrackViewer : Action::Demo;
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

void FrontendDebugMenu::service() {
    auto& screens = *menus_->screens;
    const auto action = std::exchange(pending_, Action::None);
    if (action == Action::None || pendingEpoch_ != screens.pointerEpoch ||
        !screens.active || screens.active->configuration != &screen_ ||
        screens.IsAnyPopupActive() || menus_->raceSetup->flow->requestedState != 3)
        return;
    if (action == Action::TrackViewer) openTrackViewer_();
    else menus_->demoMode->RequestDemoMode();
}

}
