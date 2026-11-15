#pragma once

#include "indep/inc/FMenus.hpp"

#include <godot_cpp/variant/callable.hpp>
#include <memory>

namespace hp2 {

// Port-side Flailer page for host graphics preferences.
class GraphicMenu final {
public:
    explicit GraphicMenu(godot::Callable settings);
    ~GraphicMenu();
    const FrontendMenuExtension& extension() const { return extension_; }

private:
    struct Page;
    static void callback(ScreenMessage message, ScreenCallbackData* data);

    static GraphicMenu* active_;
    FlailerScreenConfig screen_;
    FrontendMenuExtension extension_;
    MainMenuScreenServices* menus_{};
    godot::Callable settings_;
    std::unique_ptr<Page> page_;
};

}
