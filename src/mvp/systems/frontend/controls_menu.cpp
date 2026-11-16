#include "controls_menu.hpp"
#include "control_settings.hpp"

#include "bware/inc/bWare.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/FEAnim.hpp"
#include "indep/inc/Fred.hpp"
#include "indep/inc/Timer.hpp"

#include <algorithm>
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace hp2 {
using namespace godot;

namespace {
constexpr std::uint32_t art = 0x8bc68340u; // Display Options art, also borrowed by Graphic.
constexpr std::uint32_t first = 0x52e039f3u;
constexpr std::uint32_t popupArt = 0x037f3133u;
constexpr std::size_t visibleRows = 5;
constexpr std::size_t resetRow = ControlSettings::bindings.size();
constexpr std::size_t firstTab = resetRow + 1;
constexpr std::array tabNames{"Frontend", "Race Mode", "Cop Mode"};
// Display Options' authored movie rectangle in the 640×480 canvas.
constexpr float panelLeft = 141.0f;
constexpr float panelWidth = 498.0f;
constexpr float contentInset = 12.0f;
constexpr float labelX = panelLeft + contentInset;
constexpr float valueX = panelLeft + panelWidth - contentInset;
constexpr float firstRowY = 80.0f;
constexpr float tabY = firstRowY;
constexpr float tabWidth = panelWidth / tabNames.size();
constexpr float resetY = 311.0f;
constexpr std::uint32_t echo = 0xab671c9au;
constexpr std::uint32_t upper = 0xc671680cu;
constexpr std::uint32_t lower = 0x32c91daeu;
constexpr std::uint32_t leftArrow = 0xd1cda594u;
constexpr std::uint32_t rightArrow = 0x9345e4a7u;
constexpr std::uint32_t scrollUp = 0xedf2e4eeu;
constexpr std::uint32_t scrollDown = 0x20d73d01u;
constexpr std::uint32_t firstMask = 0x6e34e350u;
constexpr std::uint32_t footer = 0x70a404ecu;
constexpr std::uint32_t footerClip = 0x5e11b599u;
constexpr std::array hiddenArt{leftArrow, rightArrow, scrollUp, scrollDown, 0xaa5d7d3fu,
    firstMask, 0x708a0bf1u, 0x72df3492u, 0x75345d33u, 0x778985d4u, 0x79deae75u, 0x7c33d716u,
    0x7e88ffb7u, 0x80de2858u, 0xc87ada20u, 0xcad002c1u};
}

struct ControlsMenu::Page {
    enum class Decision { None, Swap, ResetKeyboard, ResetController };
    struct Row {
        FlailerTextChunk* title{};
        FlailerTextChunk* value{};
        FlailerBitmapChunk* mask{};
        std::unique_ptr<FlailerMenuItem> item;
    };

    Page(MainMenuScreenServices& services, ControlSettings& settings, View view)
        : screens(*services.screens), resources(*services.ui->resources), language(services.ui->language),
          settings(settings), frontend(*services.frontend), clock(*services.ui->clock), view(view),
          source(*screens.active->FindFlailerScreen(static_cast<int>(art))),
          menu(*source.menu), resource(*resources.FindScreen(art)),
          highlight(*services.ui, art, echo, services.ui->animationSettings.highlightColor)
    {
        const auto& label = text(first);
        const auto& value = text(0x55cb09f2u);
        const auto& heading = text(0x6eb9985du);
        for (auto* object = resource.objects.next; object != &resource.objects; object = object->next) {
            if ((object->kind == 1 && object->key != echo) ||
                std::find(hiddenArt.begin(), hiddenArt.end(), object->key) != hiddenArt.end()) {
                visibility.emplace_back(object, object->visible);
                object->visible = 0;
                object->DeleteFlailerStrip();
            }
        }
        maskModel = static_cast<FlailerBitmapChunk*>(resources.FindObject(art, firstMask, 2));
        const auto* secondMask = resources.FindObject(art, 0x708a0bf1u, 2);
        Require(maskModel && secondMask, "Controls needs the authored Options row masks");
        rowPitch = secondMask->topLeftY - maskModel->topLeftY;
        maskHeight = resources.ScaledBitmapHeight(*maskModel);
        labelOffset = label.topLeftY - maskModel->topLeftY;
        valueOffset = value.topLeftY - maskModel->topLeftY;
        bindingY = tabY + rowPitch;
        Require(rowPitch > maskHeight, "Options row masks need their authored clearance");
        for (const auto key : {upper, lower, footer, footerClip}) {
            if (auto* bitmap = static_cast<FlailerBitmapChunk*>(resources.FindObject(art, key, 2))) {
                frame.push_back({bitmap, bitmap->topLeftX, bitmap->topLeftY, bitmap->scaleX, bitmap->clipObjectKey});
                bitmap->SetTopLeftX(panelLeft);
                bitmap->SetWidth(resources, panelWidth);
            }
        }
        const auto& caption = text(0x75980cf0u);
        auto& description = add_text(caption, "HP2_CONTROLS_DESCRIPTION", "", labelX, caption.topLeftY,
            caption.formatOptions | 0x10u);
        description.SetClipToPoly(footerClip);
        blurb.emplace(*services.ui, art, description.key, description.key, footer);
        for (const auto key : {leftArrow, rightArrow, scrollUp, scrollDown}) {
            if (auto* arrow = resources.FindObject(art, key))
                arrows.push_back({arrow, arrow->topLeftX, arrow->topLeftY, arrow->clipObjectKey});
        }
        oldFirst = menu.root.next;
        oldLast = menu.root.previous;
        oldCurrent = menu.current;
        menu.root.next = menu.root.previous = &menu.root;
        menu.current = nullptr;

        add_text(heading, "HP2_CONTROLS_TITLE",
            view == View::Hub ? "Controls" : view == View::Keyboard ? "Keyboard Settings" : "Controller Settings",
            labelX, 13.0f);
        if (view == View::Hub) {
            add_mask("DEVICE", panelLeft, firstRowY, panelWidth);
            add_text(label, "HP2_CONTROLS_DEVICE_LABEL", "Current Input Device", labelX, firstRowY + labelOffset);
            device = &add_text(value, "HP2_CONTROLS_DEVICE_VALUE", "", valueX, firstRowY + valueOffset, 1);
        }
        if (view == View::Hub) {
            add_row(label, value, "Keyboard Settings", "");
            add_row(label, value, "Controller Settings", "");
        } else if (view == View::Keyboard) {
            const auto* model = static_cast<const FlailerBitmapChunk*>(resources.FindObject(art, leftArrow, 2));
            Require(model, "Keyboard Settings needs an authored bitmap for its list clip");
            viewport = std::make_unique<FlailerListViewport>(resources, resource, *model,
                panelLeft - 4.0f, bindingY, panelWidth + 4.0f, rowPitch * (visibleRows - 1) + maskHeight,
                rowPitch, visibleRows);
            for (const auto& binding : ControlSettings::bindings) {
                const auto group = static_cast<std::size_t>(binding.group);
                groupRows[group].push_back(rows.size());
                if (std::string_view(binding.action) == "hp2_horn") sirenRow = rows.size();
                add_row(label, value, binding.label, "");
            }
            add_row(label, value, "Restore Defaults", "");
            for (std::size_t i = 0; i < tabNames.size(); ++i) {
                add_row(label, value, tabNames[i], "");
                rows.back().title->SetTopLeftX(labelX + static_cast<float>(i) * tabWidth);
            }
            tabColor = rows[firstTab].item->savedColor;
            const auto cop = static_cast<std::size_t>(ControlSettings::Group::Cop);
            const float sirenY = bindingY + rowPitch * groupRows[cop].size();
            sirenLabel = &add_text(label, "HP2_CONTROLS_SIREN_LABEL", "Siren / Target Speeders", labelX, sirenY + labelOffset);
            sirenValue = &add_text(value, "HP2_CONTROLS_SIREN_VALUE", "", valueX, sirenY + valueOffset, 1);
            sirenMask = &add_mask("SIREN", panelLeft, sirenY, panelWidth);
            show_group(0);
        } else {
            add_row(label, value, "Controller Layout", "");
            add_row(label, value, "Steering Dead Zone", "");
            add_row(label, value, "Steering Sensitivity", "");
            add_row(label, value, "Restore Defaults", "");
            for (const auto index : {1, 2}) rows[index].value->SetTopLeftX(valueX - 20.0f);
        }
        if (view != View::Keyboard) rebuild_selection();
        place_rows();
        update_overflow();
        refresh();
        screens.StartMovie("MOVIES\\options_movie_new.AVI");
    }

    ~Page() {
        highlight.SetCurrentMenuItem(0);
        if (source.menu == &menu) {
            menu.root.next = oldFirst;
            menu.root.previous = oldLast;
            menu.current = oldCurrent;
        }
        rows.clear();
        blurb.reset();
        for (auto& bitmap : masks) {
            bitmap.DeleteFlailerStrip();
            bitmap.previous->next = bitmap.next;
            bitmap.next->previous = bitmap.previous;
            --resource.bitmapCount;
        }
        for (auto& entry : texts) {
            entry.DeleteFlailerStrip();
            entry.previous->next = entry.next;
            entry.next->previous = entry.previous;
            --resource.textCount;
        }
        for (const auto& [bitmap, x, y, scale, clip] : frame) {
            bitmap->SetTopLeftX(x);
            bitmap->SetTopLeftY(y);
            bitmap->SetScaleX(scale);
            bitmap->SetClipToPoly(clip);
        }
        for (const auto& [arrow, x, y, clip] : arrows) {
            arrow->SetTopLeftX(x);
            arrow->SetTopLeftY(y);
            arrow->SetClipToPoly(clip);
        }
        for (const auto& [object, visible] : visibility) {
            object->visible = visible;
            if (!visible) object->DeleteFlailerStrip();
        }
    }

    const FlailerTextChunk& text(std::uint32_t key) const {
        const auto* object = resources.FindObject(art, key, 1);
        Require(object, "Controls menu requires existing HP2 Options fonts");
        return *static_cast<const FlailerTextChunk*>(object);
    }

    FlailerTextChunk& add_text(const FlailerTextChunk& model, const std::string& id,
        const char* initial, float x, float y, std::uint32_t alignment = 0) {
        auto& entry = texts.emplace_back(model);
        entry.key = hashString(id.c_str());
        entry.parentKey = 0;
        entry.strip = nullptr;
        entry.textCapacity = 256;
        entry.ownedText.assign(257, 0);
        entry.text = entry.ownedText.data();
        entry.languageId = 0;
        entry.visible = 1;
        entry.formatOptions = alignment;
        entry.SetClipToPoly(0);
        entry.SetTopLeftX(x);
        entry.SetTopLeftY(y);
        entry.SetString(initial);
        entry.next = &resource.objects;
        entry.previous = resource.objects.previous;
        entry.previous->next = &entry;
        resource.objects.previous = &entry;
        ++resource.textCount;
        return entry;
    }

    void add_row(const FlailerTextChunk& label, const FlailerTextChunk& value,
        const char* title, const char* initial) {
        const auto number = rows.size();
        const auto suffix = std::to_string(number);
        auto& left = add_text(label, "HP2_CONTROLS_ROW_" + suffix, title, labelX, firstRowY);
        auto& right = add_text(value, "HP2_CONTROLS_VALUE_" + suffix, initial, valueX, firstRowY, 1);
        const bool tab = viewport && number >= firstTab;
        auto& mask = add_mask(suffix, tab ? panelLeft + tabWidth * (number - firstTab) : panelLeft,
            firstRowY, tab ? tabWidth - 4.0f : panelWidth);
        if (viewport && number < resetRow) {
            viewport->Clip(left);
            viewport->Clip(right);
            viewport->Clip(mask);
        }
        auto item = std::make_unique<FlailerMenuItem>(&menu, left.key, static_cast<std::uint32_t>(number));
        item->object = &left;
        rows.push_back({&left, &right, &mask, std::move(item)});
        link_row(number);
    }

    void link_row(std::size_t row) {
        auto& item = *rows[row].item;
        item.next = &menu.root;
        item.previous = menu.root.previous;
        item.previous->next = &item;
        menu.root.previous = &item;
    }

    FlailerBitmapChunk& add_mask(const std::string& suffix, float x, float y, float width) {
        auto& bitmap = masks.emplace_back(*maskModel);
        bitmap.key = hashString(("HP2_CONTROLS_MASK_" + suffix).c_str());
        bitmap.parentKey = 0;
        bitmap.strip = nullptr;
        bitmap.visible = 1;
        bitmap.SetClipToPoly(0);
        bitmap.SetTopLeftX(x);
        bitmap.SetTopLeftY(y);
        bitmap.SetWidth(resources, width);
        bitmap.previous = maskModel->previous;
        bitmap.next = maskModel;
        bitmap.previous->next = &bitmap;
        maskModel->previous = &bitmap;
        ++resource.bitmapCount;
        return bitmap;
    }

    void rebuild_selection() {
        selection = {};
        selection.showOtherItems = 1;
        for (auto* item = menu.root.next; item != &menu.root; item = item->next)
            selection.AddItem(rows[item->ordinal].mask);
        if (menu.current) selection.SetCurrentItem(rows[menu.current->ordinal].mask->key);
    }

    void set_visible(FlailerObjectChunk& object, bool visible) {
        object.visible = visible;
        if (!visible) object.DeleteFlailerStrip();
    }

    void show_group(std::size_t next) {
        groupTop[group] = viewport->Top();
        group = next;
        viewport->Reset(groupTop[group]);
        menu.root.next = menu.root.previous = &menu.root;
        const auto& tint = menu.ui->highlightedRgb;
        const auto activeColor = (tabColor & 0xff000000u) | tint[0] | (tint[1] << 8) | (tint[2] << 16);
        for (std::size_t i = 0; i < tabNames.size(); ++i) {
            link_row(firstTab + i);
            auto& tab = rows[firstTab + i];
            tab.item->savedColor = i == group ? activeColor : tabColor;
            if (tab.item.get() != menu.current) tab.title->SetColor(tab.item->savedColor);
        }
        for (const auto& indexes : groupRows) {
            for (const auto row : indexes) {
                set_visible(*rows[row].title, &indexes == &groupRows[group]);
                set_visible(*rows[row].value, &indexes == &groupRows[group]);
                set_visible(*rows[row].mask, &indexes == &groupRows[group]);
            }
        }
        for (const auto row : groupRows[group]) link_row(row);
        link_row(resetRow);
        const bool cop = group == static_cast<std::size_t>(ControlSettings::Group::Cop);
        set_visible(*sirenLabel, cop);
        set_visible(*sirenValue, cop);
        set_visible(*sirenMask, cop);
        rebuild_selection();
        place_rows();
        update_overflow();
        refresh();
    }

    void choose_group(std::size_t next) {
        if (next != group) show_group(next);
        menu.SetActiveMenuItem(rows[firstTab + next].title->key);
        position_rails(firstTab + next);
    }

    void set_text(FlailerTextChunk& text, const String& value, float maxWidth) {
        const auto utf8 = value.utf8();
        if (value == String::utf8(text.text)) return;
        text.SetString(utf8.get_data());
        text.SetScaleX(1.0f);
        text.SetScaleY(1.0f);
        const auto width = resources.ScaledTextWidth(text, language);
        if (width > maxWidth) {
            text.SetScaleX(maxWidth / width);
            text.SetScaleY(maxWidth / width);
        }
    }

    void place_row(std::size_t index, float y) {
        auto& row = rows[index];
        row.title->SetTopLeftY(y + labelOffset);
        row.value->SetTopLeftY(y + valueOffset);
        row.mask->SetTopLeftY(y);
    }

    void place_rows() {
        if (viewport) {
            for (std::size_t i = 0; i < groupRows[group].size(); ++i)
                place_row(groupRows[group][i], bindingY + viewport->Displacement(i));
            for (std::size_t i = firstTab; i < rows.size(); ++i) place_row(i, tabY);
            place_row(resetRow, resetY);
            return;
        }
        const auto top = view == View::Hub ? firstRowY + rowPitch : firstRowY;
        for (std::size_t i = 0; i < rows.size(); ++i)
            place_row(i, top + static_cast<float>(i) * rowPitch);
    }

    void update_overflow() {
        if (!viewport) return;
        const auto top = viewport->Top();
        for (const auto& arrow : arrows) {
            const auto key = arrow.object->key;
            if (key != scrollUp && key != scrollDown) continue;
            const bool up = key == scrollUp;
            const bool visible = up ? top > 0 : top + visibleRows < groupRows[group].size();
            resources.SetObjectVisibility(art, key, visible);
            if (visible) {
                arrow.object->SetClipToPoly(0);
                arrow.object->SetTopLeftX(valueX - (up ? 50.0f : 18.0f));
                arrow.object->SetTopLeftY(293.0f);
            }
        }
    }

    void position_rails(std::size_t row) {
        auto& mask = *rows[row].mask;
        const auto rowY = mask.topLeftY;
        const auto y = viewport && row < resetRow
            ? std::clamp(rowY, bindingY, bindingY + rowPitch * (visibleRows - 1))
            : rowY;
        for (const auto& entry : frame) {
            if (entry.bitmap->key != upper && entry.bitmap->key != lower) continue;
            entry.bitmap->SetTopLeftX(mask.topLeftX);
            entry.bitmap->SetWidth(resources, resources.ScaledBitmapWidth(mask));
            entry.bitmap->SetClipToPoly(0);
        }
        OptionsScreenBase::PositionHighlightBars(*menu.ui, art, mask.key, y - rowY);
    }

    const char* description_for(std::size_t row) const {
        if (view == View::Hub)
            return row == 0 ? "Set the keyboard controls." : "Set the controller options.";
        if (view == View::Controller) {
            constexpr std::array descriptions{"Choose a controller layout.", "Set the steering stick dead zone.",
                "Set the steering sensitivity.", "Restore the controller defaults."};
            return descriptions[row];
        }
        if (row == resetRow) return "Restore the default keyboard controls.";
        if (row >= firstTab) {
            constexpr std::array descriptions{"Set the menu controls.", "Set the driving controls.",
                "Set the police controls."};
            return descriptions[row - firstTab];
        }
        return "Set the key for this action.";
    }

    void refresh() {
        if (device) set_text(*device, settings.deviceName, 265.0f);
        if (view == View::Keyboard) {
            for (std::size_t i = 0; i < ControlSettings::bindings.size(); ++i)
                set_text(*rows[i].value, capturingRow == static_cast<int>(i)
                    ? String(rejectedKey ? "Key unavailable" : "Press a key...") : settings.key_name(i), 158.0f);
            if (sirenValue->visible) set_text(*sirenValue, settings.key_name(sirenRow), 158.0f);
        }
        if (view == View::Controller) {
            const auto& input = *screens.input->router;
            const int layout = joystickChooseConfiguration(JOYSTICK_PROFILE_GAMEPAD, joystickGetUserConfiguration(&input, 0));
            const char name[]{static_cast<char>('A' + layout - 30), '\0'};
            set_text(*rows[0].value, String("Layout ") + name, 170.0f);
            set_text(*rows[1].value, String::num_int64(settings.dead_zone()) + "%", 170.0f);
            set_text(*rows[2].value, String::num_int64(settings.sensitivity()) + "%", 170.0f);
        }
    }

    void select(std::size_t row) {
        if (view == View::Hub) {
            screens.ChangeToScreen(row == 0 ? "Keyboard Settings" : "Controller Settings", true);
            return;
        }
        if (view == View::Controller) {
            if (row == 0) screens.ChangeToScreen("Joystick PreConfig", true);
            if (row == 3) show_decision(Decision::ResetController, "Restore the default layout, vibration and stick settings?", "Restore");
            return;
        }
        if (row >= firstTab) {
            choose_group(row - firstTab);
            return;
        }
        if (row == resetRow) {
            show_decision(Decision::ResetKeyboard, "Restore all default keyboard bindings?", "Restore");
            return;
        }
        capturingRow = static_cast<int>(row);
        rejectedKey = false;
        refresh();
    }

    void show_decision(Decision pending, const std::string& message, const char* confirm) {
        decision = pending;
        dialog = message;
        FlailerPopupConfig popup;
        popup.sourceNode = screens.active;
        popup.screenKey = popupArt;
        popup.message = dialog.c_str();
        popup.buttonCount = 2;
        popup.flags = 3;
        popup.joystickPort = -1;
        popup.buttonResults = {1, 0, 0};
        popup.initialButton = 1;
        Require(screens.SummonPopup(popup) >= 0, "Controls menu could not open the HP2 dialog");
        resources.SetLocalizedText(popupArt, 0xe0105663u, confirm);
        resources.SetLocalizedText(popupArt, 0xe0105664u, "Cancel");
    }

    void show_save_error() {
        FlailerPopupConfig popup;
        popup.sourceNode = screens.active;
        popup.screenKey = 0x74d2dfa2u; // Authored one-button message, also used by Graphic.
        popup.message = "Could not save the controls settings.";
        popup.buttonCount = 1;
        popup.buttonLanguageIds[0] = 0x24a55;
        popup.flags = 2;
        popup.joystickPort = -1;
        Require(screens.SummonPopup(popup) >= 0, "Controls menu could not open the HP2 message");
    }

    void finish_capture(Key key) {
        if (key == KEY_ESCAPE) {
            capturingRow = -1;
            return;
        }
        if (!settings.can_bind(static_cast<std::size_t>(capturingRow), key)) {
            rejectedKey = true;
            return;
        }
        const int row = std::exchange(capturingRow, -1);
        const int other = settings.conflict(row, key);
        if (other >= 0) {
            swapRow = row;
            swapKey = key;
            const auto keyName = ControlSettings::key_name(key).utf8();
            show_decision(Decision::Swap, std::string(keyName.get_data()) + " is assigned to " +
                ControlSettings::bindings[other].label + ". Swap bindings?", "Swap");
        } else if (!settings.bind(row, key)) saveError = true;
    }

    bool restore_controller() {
        if (!settings.tune_stick(ControlSettings::defaultDeadZone, ControlSettings::defaultSensitivity)) return false;
        auto& layout = frontend.joystickConfigurations[0];
        auto& vibration = frontend.players[0]->settings.authoredOption;
        const auto defaultLayout = joystickChooseConfiguration(JOYSTICK_PROFILE_GAMEPAD, 0);
        controllerSavePending = frontend.autoSave && (layout != defaultLayout || vibration != 1);
        layout = defaultLayout;
        vibration = 1;
        joystickSetUserConfiguration(screens.input->router, 0, layout);
        return true;
    }

    void handle(ScreenMessage message, ScreenCallbackData& data) {
        const auto row = menu.current ? menu.current->ordinal : 0;
        switch (message) {
        case ScreenMessage::Tick:
            if (controllerSavePending && RequestProfileAutosave([] {})) return;
            if (pendingKey) {
                const auto key = *pendingKey;
                pendingKey.reset();
                finish_capture(key);
            }
            if (saveError && !screens.IsAnyPopupActive()) {
                saveError = false;
                show_save_error();
            }
            if (viewport && viewport->Tick(clock.realTimeElapsed)) {
                place_rows();
                position_rails(row);
            }
            if (!viewport || !viewport->Moving()) highlight.Animate();
            blurb->Animate();
            refresh();
            break;
        case ScreenMessage::Highlight: {
            if (viewport && row < resetRow) {
                const auto& items = groupRows[group];
                const auto position = std::find(items.begin(), items.end(), row);
                if (position != items.end() && viewport->Reveal(position - items.begin())) update_overflow();
            }
            selection.SetCurrentItem(rows[row].mask->key);
            position_rails(row);
            blurb->NewText(description_for(row));
            for (const auto& arrow : arrows) {
                if (arrow.object->key != leftArrow && arrow.object->key != rightArrow) continue;
                const bool adjust = view == View::Controller && (row == 1 || row == 2);
                resources.SetObjectVisibility(art, arrow.object->key, adjust ? 1 : 0);
                if (adjust) {
                    arrow.object->SetClipToPoly(0);
                    arrow.object->SetTopLeftX(valueX - (arrow.object->key == leftArrow ? 82.0f : 8.0f));
                    const auto& value = *rows[row].value;
                    auto& bitmap = *static_cast<FlailerBitmapChunk*>(arrow.object);
                    arrow.object->SetTopLeftY(value.topLeftY +
                        (resources.ScaledTextHeight(value) - resources.ScaledBitmapHeight(bitmap)) * 0.5f);
                }
            }
            highlight.SetCurrentMenuItem(data.key);
            // The borrowed pulse grows around the text center. Keep it inside the content inset.
            const float labelWidth = resources.ScaledTextWidth(*rows[row].title, language);
            if (labelWidth > 0.0f) {
                const float maxScale = 1.0f + (contentInset - 2.0f) * 2.0f / labelWidth;
                highlight.startScale = std::min(highlight.startScale, maxScale);
                highlight.peakScale = std::min(highlight.peakScale, maxScale);
            }
            break;
        }
        case ScreenMessage::Select:
            if (capturingRow < 0) select(row);
            break;
        case ScreenMessage::Previous:
        case ScreenMessage::Next:
            if (viewport) {
                const bool previous = message == ScreenMessage::Previous;
                const auto& items = groupRows[group];
                if (row >= firstTab || (previous && row == items.front()) || (!previous && row == resetRow)) {
                    data.value = 1;
                    if (row >= firstTab && !previous) {
                        const auto position = std::min(viewport->Top(), items.size() - 1);
                        menu.SetActiveMenuItem(rows[items[position]].title->key);
                    } else if (previous && row < resetRow) {
                        menu.SetActiveMenuItem(rows[firstTab + group].title->key);
                    }
                }
            }
            break;
        case ScreenMessage::Left:
        case ScreenMessage::Right:
            if (viewport && row >= firstTab) {
                const int direction = message == ScreenMessage::Left ? -1 : 1;
                const auto next = std::clamp(static_cast<int>(row - firstTab) + direction, 0,
                    static_cast<int>(tabNames.size()) - 1);
                choose_group(static_cast<std::size_t>(next));
            }
            if (view == View::Controller && (row == 1 || row == 2)) {
                const int direction = message == ScreenMessage::Left ? -1 : 1;
                if (!settings.tune_stick(settings.dead_zone() + (row == 1 ? direction * 2 : 0),
                    settings.sensitivity() + (row == 2 ? direction * 10 : 0)))
                    saveError = true;
                else data.sound = direction < 0 ? 1 : 2;
                refresh();
            }
            break;
        case ScreenMessage::PopupDismiss: {
            const auto pending = std::exchange(decision, Decision::None);
            if (!data.value || pending == Decision::None) break;
            const bool saved = pending == Decision::Swap ? settings.bind(swapRow, swapKey)
                : pending == Decision::ResetKeyboard ? settings.restore_keyboard()
                : restore_controller();
            if (!saved) saveError = true;
            refresh();
            break;
        }
        default: break;
        }
    }

    FlailerScreenList& screens;
    FlailerResources& resources;
    const LanguageManager* language;
    ControlSettings& settings;
    FrontendPlayerManager& frontend;
    const GameTime& clock;
    View view;
    FlailerScreen& source;
    FlailerMenu& menu;
    FlailerScreenResource& resource;
    FlailerHighlightAnimation highlight;
    FlailerMenuItem *oldFirst{}, *oldLast{}, *oldCurrent{};
    std::deque<FlailerTextChunk> texts;
    std::deque<FlailerBitmapChunk> masks;
    FlailerBitmapChunk *maskModel{}, *sirenMask{};
    float rowPitch{}, maskHeight{}, labelOffset{}, valueOffset{}, bindingY{};
    FlailerSelectionSet selection;
    std::optional<FlailerAnimatedBlurb> blurb;
    std::vector<Row> rows;
    std::vector<std::pair<FlailerObjectChunk*, std::uint8_t>> visibility;
    struct PositionedBitmap { FlailerBitmapChunk* bitmap; float x, y, scale; std::uint32_t clip; };
    std::vector<PositionedBitmap> frame;
    struct PositionedArrow { FlailerObjectChunk* object; float x, y; std::uint32_t clip; };
    std::vector<PositionedArrow> arrows;
    FlailerTextChunk *device{}, *sirenLabel{}, *sirenValue{};
    std::unique_ptr<FlailerListViewport> viewport;
    std::array<std::vector<std::size_t>, tabNames.size()> groupRows;
    std::array<std::size_t, tabNames.size()> groupTop{};
    std::size_t group{}, sirenRow{};
    std::uint32_t tabColor{};
    bool controllerSavePending{};
    bool rejectedKey{}, saveError{};
    int capturingRow{-1};
    std::optional<Key> pendingKey;
    Decision decision{Decision::None};
    int swapRow{};
    Key swapKey{KEY_NONE};
    std::string dialog;
};

ControlsMenu* ControlsMenu::active_{};

ControlsMenu::ControlsMenu(ControlSettings& settings)
    : settings_(settings),
      hub_{"Controls", [](ScreenMessage message, ScreenCallbackData* data) { callback(View::Hub, message, data); },
          {{static_cast<int>(art), 0, 0, 0}}, {{-1, -1, -1, -1}}, 1, nullptr},
      keyboard_{"Keyboard Settings", [](ScreenMessage message, ScreenCallbackData* data) { callback(View::Keyboard, message, data); },
          hub_.screenIds, hub_.joystickPorts, 1, nullptr},
      controller_{"Controller Settings", [](ScreenMessage message, ScreenCallbackData* data) { callback(View::Controller, message, data); },
          hub_.screenIds, hub_.joystickPorts, 1, nullptr},
      extension_{"Controls", nullptr, &hub_,
          [](MainMenuScreenServices& menus, void* context) {
              active_ = static_cast<ControlsMenu*>(context);
              active_->menus_ = &menus;
          }, nullptr, this, &keyboard_, &controller_} {}

ControlsMenu::~ControlsMenu() { if (active_ == this) active_ = nullptr; }

bool ControlsMenu::capturing() const { return page_ && page_->capturingRow >= 0; }

void ControlsMenu::capture_key(Key key) {
    if (capturing() && !page_->pendingKey) page_->pendingKey = key;
}

void ControlsMenu::callback(View view, ScreenMessage message, ScreenCallbackData* data) {
    if (!active_ || !data) return;
    if (message == ScreenMessage::ScreenActivate) {
        active_->page_ = std::make_unique<Page>(*active_->menus_, active_->settings_, view);
        if (view == View::Keyboard) data->value = 1;
    } else if (message == ScreenMessage::ScreenDeactivate) {
        active_->page_.reset();
    } else if (active_->page_) {
        active_->page_->handle(message, *data);
    }
}

}
