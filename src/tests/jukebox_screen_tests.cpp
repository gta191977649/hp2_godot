#include "game_core/indep/src/frontendscreens/options.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/indep/src/frontendscreens/memcardscreens.hpp"
#include <cstdio>
#include <stdexcept>

namespace {
void require(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
using State=port::frontend::JukeboxScreenState;
State fixture() {
    std::array<std::uint8_t,0xb0> bytes{};
    bytes[0x9c]=9;bytes[0xaf]=0xff;
    std::vector<port::frontend::JukeboxSongMetadata> metadata;
    for(int i=0;i<9;++i) {
        bytes[0x1c+i*4]=2;bytes[0x1d+i*4]=static_cast<std::uint8_t>(i);
        bytes[0x1e + i*4]=4;
        metadata.push_back({"fixture artist","fixture title",i==1 || i==8 ? "second" : "first"});
    }
    State state;hp2::frontend::Sub_001F02D8(state,bytes,metadata);return state;
}
}
int main() {
    auto state=fixture();
    require(state.groups.size()==2 && state.groups[0].songs==std::vector<std::size_t>({0,2,3,4,5,6,7}),
            "grouping must retain first-seen categories and authored song order");
    require(state.snapshot[0x23]==1 && state.snapshot[0x3f]==1,
            "grouping must publish per-song genre indexes into the snapshot");
    auto rows=hp2::frontend::Sub_001F0478(state);
    require(rows.rows.size()==5 && rows.rows[1].song==2 && rows.category_arrows && !rows.up && !rows.down,
            "initial category focus and five-row presentation differ");
    int previews=0,stops=0,commits=0,sound=99;
    std::uint8_t preview_song=255;
    port::frontend::JukeboxScreenServices services{
        [&](std::uint8_t category,std::uint8_t song,bool resume) {
            require(category==2 && !resume,"preview operands differ");++previews;preview_song=song;
        },[&]{++stops;},[&](const State& current){require(current.snapshot[0xaf]==0xfe,"commit snapshot differs");++commits;}};
    const auto message=[&](int id,float elapsed=0,bool playing=true) {
        hp2::frontend::Sub_001F0D10(state,id,sound,1234,elapsed,playing,services);
    };
    message(1);require(!state.list_active && sound==0,"Select must not fabricate a list-entry action");
    message(9);require(state.list_active && state.preview_pending && state.selection_tick==1234,
                       "Down must enter enabled category and schedule preview");
    message(18,0.5f);require(previews==0,"half-second preview threshold must be strict");
    message(18,0.5001f);require(previews==1 && preview_song==0 && !state.preview_pending,"delayed preview differs");
    message(18,30.0f,false);require(previews==1,"thirty-second restart threshold must be strict");
    message(18,30.001f,false);require(previews==2,"inactive preview restart missing");
    for(int i=0;i<6;++i)message(9);
    rows=hp2::frontend::Sub_001F0478(state);
    require(state.cursor==6 && state.first_row==2 && rows.rows[4].song==7 && rows.rows[4].highlighted && rows.up && !rows.down,
            "scrolling final row and arrow limits differ");
    message(9);require(state.cursor==6 && sound==0,"Down at last song must not wrap");
    message(7);require(state.snapshot[0x3a]==5 && sound==2,"Right must change the selected song mode");
    message(7);require(state.snapshot[0x3a]==0,"mode5 must wrap to mode0");
    message(6);require(state.snapshot[0x3a]==5 && sound==1,"mode0 must wrap left to mode5");
    for(int i=0;i<6;++i)message(8);
    sound=77;message(8);
    require(!state.list_active && stops==1 && sound==77,"Up at first row must stop preview and preserve sound");
    message(6);require(state.snapshot[0xae]==1 && state.cursor==0,"category Left wrap differs");
    rows=hp2::frontend::Sub_001F0478(state);require(rows.rows.size()==2,"short category must hide unused rows");
    message(7);message(9);message(15);
    require(!state.list_active && state.cursor==0 && state.first_row==0 && state.snapshot[0xaf]==0xfe && sound==5,
            "disabling category must leave song focus and reset scroll");
    rows=hp2::frontend::Sub_001F0478(state);require(rows.rows[0].mode==0,"disabled category must display mode0");
    message(9);require(!state.list_active && sound==6,"disabled category must reject Down");
    message(17);require(commits==1,"Back must deliver exact snapshot to dependency chain");
    bool threw=false;
    try {hp2::frontend::Sub_001F0D10(state,17,sound,0,0,true,{});}
    catch(const hp2::NotImplemented&){threw=true;}
    require(threw,"missing Back dependency must throw");
    auto invalid=state.snapshot;invalid[0x9c]=33;
    const auto prior=state.snapshot;
    threw=false;
    try {hp2::frontend::Sub_001F02D8(state,invalid,{});}
    catch(const hp2::NotImplemented&){threw=true;}
    require(threw && state.snapshot==prior,"invalid construction must fail without replacing prior state");
    port::frontend::AutosaveState save;
    int stores=0;
    const auto store=[&]{++stores;return std::string{};};
    require(!hp2::frontend::Sub_00246138(save,1.0,store) && save.phase==1 && stores==0,
        "save probe must use strict one-second gate");
    hp2::frontend::Sub_00246138(save,0.001,store);
    require(save.phase==2 && stores==0,"save probe must precede store boundary");
    hp2::frontend::Sub_00246138(save,0.0,store);
    require(stores==1 && save.phase==6 && save.text_id==0x24ddc,"save must write once and show original saving text");
    hp2::frontend::Sub_00246138(save,2.0,store);
    require(save.phase==6,"save completion gate must be strict");
    hp2::frontend::Sub_00246138(save,0.001,store);
    require(save.phase==8 && save.text_id==0x24ddd,"successful save feedback missing");
    require(!hp2::frontend::Sub_00246138(save,2.0,store),"successful popup must remain for two seconds");
    require(hp2::frontend::Sub_00246138(save,0.001,store) && stores==1,"save must complete exactly once");
    hp2::frontend::FeScreen popup;
    for(int i=0;i<3;++i){hp2::frontend::FeWidget w;w.type=1;w.class_hash=0x5d6dd123;w.text_capacity=40;popup.widgets.push_back(w);}
    hp2::frontend::Sub_00144BA0(popup,0x5d6dd123,"first@second");
    require(popup.widgets[0].text=="first" && popup.widgets[1].text=="second" && popup.widgets[2].visible==0,
        "popup forced line breaks or trailing line hiding differ");
    hp2::frontend::Sub_00144BA0(popup,0x5d6dd123,"short");
    require(popup.widgets[0].visible==1 && popup.widgets[1].visible==0,"popup status replacement retains stale lines");
    std::puts("PASS Jukebox grouping, navigation, mode, genre and preview-state projection; not original-runtime parity");
}
