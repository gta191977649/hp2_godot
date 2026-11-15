#include "graphic_menu.hpp"

#include "indep/inc/FEAnim.hpp"
#include "bware/inc/bWare.hpp"

#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace hp2 {
namespace {
constexpr std::uint32_t art = 0x8bc68340u;
constexpr std::uint32_t first = 0x52e039f3u;
constexpr std::uint32_t restoreRow = 0xaee77883u; // MENUITEM_10
constexpr std::size_t advancedIndex = 8;
constexpr std::size_t restoreIndex = 9;
constexpr std::uint32_t clip = 0xc67cfb4au;
constexpr std::uint32_t upper = 0xc671680cu;
constexpr std::uint32_t lower = 0x32c91daeu;
constexpr std::uint32_t left = 0xd1cda594u;
constexpr std::uint32_t right = 0x9345e4a7u;
constexpr std::uint32_t scrollUp = 0xedf2e4eeu;
constexpr std::uint32_t scrollDown = 0x20d73d01u;
constexpr std::array labels{"Render API", "Full Screen Mode", "Resolution", "Aspect Ratio",
    "Refresh Rate", "VSync", "Anti Aliasing", "Show FPS", "Advanced Setting", "Restore to Default"};
constexpr std::array help{"Takes effect after restarting the game.", "Set the window mode.",
    "Set the full screen rendering resolution.", "Set the game picture aspect ratio.", "Limit frames per second.",
    "Synchronize frames with the display.", "Smooth edges in the 3D view.",
    "Show the live frame rate.", "Open advanced graphic settings.", "Restore the default graphic settings."};
constexpr std::array<std::uint32_t, 7> masks{
    0x6e34e350u, 0x708a0bf1u, 0x72df3492u, 0x75345d33u, 0x778985d4u, 0x79deae75u, 0x7c33d716u};
constexpr std::array<std::uint32_t, 6> authoredValues{
    0x55cb09f2u, 0x2e931f1bu, 0x03d90172u, 0x5610f2f4u, 0x422313cbu, 0x8fe4086bu};
}

struct GraphicMenu::Page {
    struct SavedText {
        FlailerTextChunk* text;
        std::string value;
        std::uint32_t language;
    };
    struct SavedObject {
        FlailerObjectChunk* object;
        float y;
        std::uint32_t clipKey;
        std::uint8_t visible;
    };

    Page(MainMenuScreenServices& menus, const godot::Callable& settings)
        : screens(*menus.screens), ui(*menus.ui), resources(*ui.resources), screen(*resources.FindScreen(art)),
          settings(settings), slide(ui, art, first_row_x()), highlight(ui, art, 0xab671c9au,
          ui.animationSettings.highlightColor), valueHighlight(ui, art, 0x1042eac3u, -1),
          arrows(ui, art, left, right), blurb(ui, art, 0x75980cf0u, 0x75980cf0u, 0x70a404ecu)
    {
        for (auto* object = screen.objects.next; object != &screen.objects; object = object->next) {
            if (object->key == 0x6eb9985du && object->kind == 1 && object->topLeftY < 40.0f) {
                relabel(static_cast<FlailerTextChunk&>(*object), "Graphic");
                break;
            }
        }
        for (std::size_t i = 0; i < restoreIndex; ++i) {
            auto& label = text(first + static_cast<std::uint32_t>(i));
            save(label);
            relabel(label, labels[i]);
        }
        add_restore_row();
        for (std::size_t i = 0; i < authoredValues.size(); ++i) {
            auto& value = text(authoredValues[i]);
            save(value);
            relabel(value, "");
            values[i] = &value;
        }
        constexpr std::array extraValueNames{"HP2_GRAPHIC_AA_VALUE", "HP2_GRAPHIC_FPS_VALUE"};
        const auto& templateValue = *values[5];
        for (std::size_t i = 0; i < extraValues.size(); ++i) {
            const auto row = i + authoredValues.size();
            auto& value = extraValues[i];
            value = templateValue;
            value.key = hashString(extraValueNames[i]);
            value.strip = nullptr;
            value.textCapacity = 64;
            value.ownedText.assign(65, 0);
            value.text = value.ownedText.data();
            value.SetTopLeftY(templateValue.topLeftY + text(first + static_cast<std::uint32_t>(row)).topLeftY -
                text(first + 5).topLeftY);
            value.previous = screen.objects.previous;
            value.next = &screen.objects;
            value.previous->next = &value;
            screen.objects.previous = &value;
            ++screen.textCount;
            values[row] = &value;
        }
        for (auto key : {upper, lower, left, right, scrollUp, scrollDown}) save(*resources.FindObject(art, key));
        for (auto key : masks) save(*resources.FindObject(art, key));
        for (auto key : {left, right}) resources.FindObject(art, key)->SetClipToPoly(clip);
        selection.showOtherItems = 1;
        selection.AddItems(resources, art, masks.data(), static_cast<int>(masks.size()));
        scroll = std::make_unique<FlailerScrollingMenuList>(ui, art, first, first + 6, scrollUp, scrollDown, clip);
        for (std::size_t i = 0; i < restoreIndex; ++i) scroll->AddObject(first + static_cast<std::uint32_t>(i));
        scroll->AddObject(restoreRow);
        for (auto* value : values) scroll->AddObject(value->key);
        scroll->SetItemCount(static_cast<int>(labels.size()));
        screens.StartMovie("MOVIES\\options_movie_new.AVI");
        refresh(-1, 0);
    }

    ~Page()
    {
        scroll.reset();
        if (page->menu == menu) {
            if (menu->current == restoreItem.get()) menu->current = restoreItem->previous;
            restoreItem->previous->next = restoreItem->next;
            restoreItem->next->previous = restoreItem->previous;
        }
        restoreItem.reset();
        remove_text(restoreText);
        for (auto& value : extraValues) {
            remove_text(value);
        }
        for (const auto& entry : savedText) {
            entry.text->SetString(entry.value.c_str());
            if (entry.language) entry.text->SetLanguageID(entry.language);
        }
        for (const auto& entry : savedObjects) {
            entry.object->SetClipToPoly(entry.clipKey);
            entry.object->SetTopLeftY(entry.y);
            resources.SetObjectVisibility(art, entry.object->key, entry.visible);
        }
    }

    void add_restore_row()
    {
        page = screens.active->FindFlailerScreen(static_cast<std::int32_t>(art));
        Require(page && page->menu, "Graphic menu has no authored item ring");
        menu = page->menu;
        auto* previous = menu->FindItem(first + advancedIndex);
        Require(previous && !menu->FindItem(restoreRow), "Graphic menu cannot append its restore row");
        const auto& advanced = text(first + advancedIndex);
        restoreText = advanced;
        restoreText.key = restoreRow;
        restoreText.strip = nullptr;
        restoreText.textCapacity = 64;
        restoreText.ownedText.assign(65, 0);
        restoreText.text = restoreText.ownedText.data();
        restoreText.SetString(labels[restoreIndex]);
        restoreText.SetTopLeftY(advanced.topLeftY * 2.0f - text(first + advancedIndex - 1).topLeftY);
        restoreText.previous = screen.objects.previous;
        restoreText.next = &screen.objects;
        restoreText.previous->next = &restoreText;
        screen.objects.previous = &restoreText;
        ++screen.textCount;
        restoreItem = std::make_unique<FlailerMenuItem>(menu, restoreRow, 10);
        restoreItem->object = &restoreText;
        restoreItem->previous = previous;
        restoreItem->next = previous->next;
        previous->next->previous = restoreItem.get();
        previous->next = restoreItem.get();
    }

    void remove_text(FlailerTextChunk& object)
    {
        object.DeleteFlailerStrip();
        object.previous->next = object.next;
        object.next->previous = object.previous;
        --screen.textCount;
    }

    FlailerTextChunk& text(std::uint32_t key)
    {
        auto* object = resources.FindObject(art, key, 1);
        Require(object, "Graphic menu requires the authored Display text");
        return static_cast<FlailerTextChunk&>(*object);
    }

    float first_row_x()
    {
        float x{}, y{};
        resources.GetTopLeft(*resources.FindObject(art, masks.front()), x, y, ui.language);
        return x;
    }

    void save(FlailerObjectChunk& object)
    {
        savedObjects.push_back({&object, object.topLeftY, object.clipObjectKey, object.visible});
    }

    void relabel(FlailerTextChunk& object, const char* value)
    {
        savedText.push_back({&object, object.text ? object.text : "", object.languageId});
        resources.SetLocalizedText(art, object.key, value);
    }

    void refresh(int index, int direction)
    {
        const godot::PackedStringArray next = settings.call(index, direction);
        if (next.size() < static_cast<std::int64_t>(values.size())) return;
        resolutionAdjustable = next.size() > static_cast<std::int64_t>(values.size() + 1) &&
            next[values.size() + 1] == "1";
        for (std::size_t i = 0; i < values.size(); ++i) {
            const auto label = next[static_cast<std::int64_t>(i)].utf8();
            if (values[i]->text && std::string_view(values[i]->text) == label.get_data()) continue;
            resources.SetLocalizedText(art, values[i]->key, label.get_data());
        }
        if (next.size() > static_cast<std::int64_t>(values.size()) && !next[values.size()].is_empty()) {
            const auto message = next[values.size()].utf8();
            show_popup(message.get_data());
        }
    }

    void position_rails(std::uint32_t mask)
    {
        std::int32_t x, top, width, bottom;
        Require(resources.GetBoundingBox(art, mask, x, top, width, bottom, ui.language),
            "Graphic menu requires the authored Display mask");
        auto offset = scroll->CurrentMenuOffsetY();
        resources.FindObject(art, upper)->SetTopLeftY(static_cast<float>(top - 6) + offset);
        resources.FindObject(art, lower)->SetTopLeftY(static_cast<float>(bottom) + offset);
    }

    void show_popup(const char* message)
    {
        if (screens.IsAnyPopupActive()) return;
        popupMessage = message;
        FlailerPopupConfig popup;
        popup.sourceNode = screens.active;
        popup.screenKey = 0x74d2dfa2u; // Same authored dialog as Debug > Car Viewer.
        popup.message = popupMessage.c_str();
        popup.buttonCount = 1;
        popup.buttonLanguageIds[0] = 0x24a55;
        popup.flags = 2;
        popup.joystickPort = -1;
        Require(screens.SummonPopup(popup) >= 0, "Graphic menu could not open the HP2 dialog");
    }

    void open_advanced_settings() { show_popup("Comming Soon!"); }

    void handle(ScreenMessage message, ScreenCallbackData& data)
    {
        scroll->HandleEvent(message, data);
        const auto index = data.key == restoreRow ? restoreIndex : static_cast<std::size_t>(data.key - first);
        const bool adjustable = index < values.size() && (index != 2 || resolutionAdjustable);
        switch (message) {
        case ScreenMessage::Highlight:
            if (index < labels.size()) {
                activeMask = scroll->GetCurrentMenuMask();
                selection.SetCurrentItem(activeMask);
                position_rails(activeMask);
                highlight.SetCurrentMenuItem(data.key);
                valueHighlight.SetCurrentMenuItem(adjustable ? values[index]->key : 0);
                arrows.SetCurrentItem(adjustable ? values[index]->key : 0);
                blurb.NewText(index == 2 && !adjustable ? "Current window resolution." : help[index]);
            }
            break;
        case ScreenMessage::Tick:
            slide.Animate();
            if (activeMask) position_rails(activeMask);
            arrows.Animate();
            highlight.Animate();
            valueHighlight.Animate();
            blurb.Animate();
            refresh(-1, 0);
            break;
        case ScreenMessage::Left:
        case ScreenMessage::Right:
            if (adjustable) {
                refresh(static_cast<int>(index), message == ScreenMessage::Left ? -1 : 1);
                valueHighlight.ForceStart();
                data.sound = message == ScreenMessage::Left ? 1 : 2;
            } else data.sound = 0;
            break;
        case ScreenMessage::Previous:
            if (index == 0) data.value = 1;
            break;
        case ScreenMessage::Next:
            if (index == labels.size() - 1) data.value = 1;
            break;
        case ScreenMessage::Select:
            if (index == advancedIndex) open_advanced_settings();
            else if (index == restoreIndex) refresh(static_cast<int>(restoreIndex), 1);
            else data.sound = 0;
            break;
        case ScreenMessage::PopupActivate:
            data.sound = 5;
            break;
        case ScreenMessage::PopupDismiss:
            data.sound = 6;
            break;
        default: break;
        }
    }

    FlailerScreenList& screens;
    FlailerUiState& ui;
    FlailerResources& resources;
    FlailerScreenResource& screen;
    godot::Callable settings;
    FlailerSlideInAnimation slide;
    FlailerHighlightAnimation highlight, valueHighlight;
    FlailerLeftRightArrowScroll arrows;
    FlailerAnimatedBlurb blurb;
    FlailerSelectionSet selection;
    std::unique_ptr<FlailerScrollingMenuList> scroll;
    std::array<FlailerTextChunk*, 8> values{};
    std::array<FlailerTextChunk, 2> extraValues{};
    FlailerScreen* page{};
    FlailerMenu* menu{};
    FlailerTextChunk restoreText;
    std::unique_ptr<FlailerMenuItem> restoreItem;
    std::vector<SavedText> savedText;
    std::vector<SavedObject> savedObjects;
    std::string popupMessage;
    std::uint32_t activeMask{};
    bool resolutionAdjustable{};
};

GraphicMenu* GraphicMenu::active_{};

GraphicMenu::GraphicMenu(godot::Callable settings)
    : screen_{"Graphic", callback, {{static_cast<std::int32_t>(art), 0, 0, 0}},
          {{-1, -1, -1, -1}}, 1, nullptr},
      extension_{"Graphic", "Set PC graphics and window options.", &screen_,
          [](MainMenuScreenServices& menus, void* context) {
              active_ = static_cast<GraphicMenu*>(context);
              active_->menus_ = &menus;
          }, nullptr, this}, settings_(std::move(settings)) {}

GraphicMenu::~GraphicMenu()
{
    if (active_ == this) active_ = nullptr;
}

void GraphicMenu::callback(ScreenMessage message, ScreenCallbackData* data)
{
    if (!active_ || !data) return;
    auto& menu = *active_;
    if (message == ScreenMessage::ScreenActivate)
        menu.page_ = std::make_unique<Page>(*menu.menus_, menu.settings_);
    else if (message == ScreenMessage::ScreenDeactivate)
        menu.page_.reset();
    else if (menu.page_)
        menu.page_->handle(message, *data);
}

}
