// TASK-0022: authentic catalog IDs; port tests do not establish PS2 equivalence.
#include "port/assets/event_name_catalog.hpp"
#include "platform/ps2_asset_root.hpp"
#include <cstdio>
#include <fstream>
#include <stdexcept>

int main() {
    try {
        using port::assets::EventNameCatalog;
        if (EventNameCatalog::car_name_id(0) != 0xa08c ||
            EventNameCatalog::car_name_id(1) != 0xa0f0 ||
            EventNameCatalog::car_name_id(22) != 0xa0fb ||
            EventNameCatalog::car_name_id(999) != 0xa08c)
            throw std::runtime_error("car catalog variant or recovered fallback mismatch");
        const auto root = hp2::platform::resolve_zzdata_root(".");
        if (!root) { std::puts("SKIP: authentic track catalog not configured; car IDs passed"); return 0; }
        std::ifstream input(*root + "/GLOBAL/GLOBALB.BUN", std::ios::binary);
        if (!input) throw std::runtime_error("configured GLOBALB.BUN cannot be opened");
        std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)), {});
        EventNameCatalog catalog;
        catalog.load(bytes);
        if (catalog.track_name_id(11) != 0xc359 || catalog.track_name_id(11, true) != 0xc741 ||
            catalog.track_name_id(66) != 0xc367)
            throw std::runtime_error("track catalog names differ from authentic asset rows");
        bool missing = false;
        try { (void)catalog.track_name_id(54); }
        catch (const hp2::NotImplemented&) { missing = true; }
        if (!missing) throw std::runtime_error("cleared track 54 must remain absent");
        std::puts("PASS: authentic track IDs, car variants, cleared track and fallback");
        return 0;
    } catch (const std::exception& error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
}

