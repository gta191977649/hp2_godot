#pragma once

#include "indep/inc/FMenus.hpp"

#include <godot_cpp/classes/global_constants.hpp>

#include <memory>

namespace hp2 {

class ControlSettings;

class ControlsMenu final {
public:
    explicit ControlsMenu(ControlSettings& settings);
    ~ControlsMenu();
    const FrontendMenuExtension& extension() const { return extension_; }
    bool capturing() const;
    void capture_key(godot::Key key);

private:
    enum class View { Hub, Keyboard, Controller };
    struct Page;
    static void callback(View view, ScreenMessage message, ScreenCallbackData* data);

    static ControlsMenu* active_;
    ControlSettings& settings_;
    MainMenuScreenServices* menus_{};
    FlailerScreenConfig hub_, keyboard_, controller_;
    FrontendMenuExtension extension_;
    std::unique_ptr<Page> page_;
};

}
