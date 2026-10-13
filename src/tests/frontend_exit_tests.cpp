#include "game_core/indep/src/frontendscreens/options.hpp"
#include <iostream>
#include <stdexcept>

int main() {
    using namespace hp2::frontend;
    try {
        auto check = [](bool value) { if (!value) throw std::runtime_error("exit motion mismatch"); };
        FeScreen screen;
        const auto group = StringHash_002534A8("MENUITEM_1_GROUP");
        FeWidget mask; mask.name_hash = StringHash_002534A8("MI_1_MASK");
        mask.class_hash = group; mask.x = 10; mask.y = 20.75f;
        FeWidget text; text.name_hash = StringHash_002534A8("MENUITEM_1");
        text.class_hash = group; text.x = 15; text.y = 22.75f;
        FeWidget underline; underline.name_hash = 0x32C91DAE;
        underline.class_hash = 0x97BF4B25; underline.x = 10; underline.y = 30.75f;
        FeWidget unrelated; unrelated.x = 99; unrelated.y = 88;
        screen.widgets = {mask, text, underline, unrelated};
        Type_002490D8 exit;
        exit.Construct_002490D8(screen);
        check(screen.widgets[0].y == 20 && screen.widgets[1].y == 22);
        check(!exit.Update_00249128(0.05f)); // item 1 duration=0.1; Hermite midpoint
        check(screen.widgets[0].x == 330 && screen.widgets[1].x == 335);
        check(screen.widgets[2].x == 330);
        check(screen.widgets[3].x == 99 && screen.widgets[3].y == 88);
        check(screen.widgets[0].dirty_0x2b == 4);
        check(exit.Update_00249128(0.05f));
        check(screen.widgets[0].x == 650 && screen.widgets[1].x == 655);
        check(screen.widgets[2].x == 650);
        // Sparse item index 7 keeps its original index-dependent duration (0.4), not ordinal.
        screen.widgets[0].name_hash = StringHash_002534A8("MI_7_MASK");
        screen.widgets[0].class_hash = StringHash_002534A8("MENUITEM_7_GROUP");
        screen.widgets[0].x = 10;
        screen.widgets[1].class_hash = 0;
        screen.widgets[2].x = 10;
        exit.Construct_002490D8(screen);
        check(!exit.Update_00249128(0.2f));
        check(screen.widgets[0].x == 330 && screen.widgets[2].x == 330);
        check(exit.Update_00249128(0.3f)); // overshoot clamps to endpoint
        check(screen.widgets[0].x == 650);
        std::cout << "FRONTEND_EXIT_OK\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
