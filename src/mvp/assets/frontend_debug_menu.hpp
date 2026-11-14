#pragma once

#include "indep/inc/FEAnim.hpp"
#include "indep/inc/FMenus.hpp"

#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace hp2 {

// One Debug frontend extension for the live ApplicationRuntime.
class FrontendDebugMenu final {
public:
    explicit FrontendDebugMenu(std::function<void()> openTrackViewer);
    ~FrontendDebugMenu();
    const FrontendMenuExtension& extension() const { return extension_; }

private:
    enum class Action { None, TrackViewer, Demo };
    static void callback(ScreenMessage message, ScreenCallbackData* data);
    void handle(ScreenMessage message, ScreenCallbackData& data);
    void service();
    void relabel(FlailerTextChunk& text, const char* label);

    struct SavedText {
        FlailerTextChunk* object;
        std::uint32_t language;
        std::string text;
    };

    static FrontendDebugMenu* active_;
    MainMenuScreenServices* menus_{};
    FlailerScreenConfig screen_;
    FrontendMenuExtension extension_;
    std::function<void()> openTrackViewer_;
    std::optional<FlailerAnimatedBlurb> blurb_;
    std::optional<FlailerHighlightAnimation> highlight_;
    FlailerSelectionSet selection_;
    std::array<float, 2> originalRails_{};
    std::vector<SavedText> originalText_;
    std::vector<std::pair<FlailerObjectChunk*, std::uint8_t>> originalVisibility_;
    Action pending_{Action::None};
    std::uint64_t pendingEpoch_{};
};

}
