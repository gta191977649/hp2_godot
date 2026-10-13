#include "game_core/indep/src/sound.hpp"
#include "platform/ps2_asset_root.hpp"
#include <algorithm>
#include <cstdio>
#include <stdexcept>

using namespace hp2::port::audio;
namespace {
void require(bool condition,const char* what) { if (!condition) throw std::runtime_error(what); }
void big(std::vector<std::uint8_t>& b,std::size_t o,std::uint32_t value) {
    for (unsigned i=0;i<4;++i) b[o+i]=static_cast<std::uint8_t>(value>>(8*(3-i)));
}
}
int main() {
    require(Sub_00273850(0xaa66)==44090 && Sub_0026D258(44090)==3762,"movie zero-pitch lookup or SPU quantization differs");
    require(Sub_00273850(32000)==32343 && Sub_0026D258(32343)==2759,"music zero-pitch lookup or SPU quantization differs");
    require(neutral_playback_pitch_scale(32000)==1.0103759765625f,"music playback factor differs");
    std::vector<std::uint8_t> fixture(80,0);
    big(fixture,0,0x56414770);big(fixture,12,32);big(fixture,16,22050);
    fixture[48]=12;fixture[50]=0x87;fixture[64]=12;fixture[65]=7;
    auto pcm=decode_vag(fixture);
    require(pcm.sample_rate==22050 && pcm.samples.size()==56,"VAG frame/rate incorrect");
    require(pcm.samples[0]==7 && pcm.samples[1]==-8,"ADPCM nibble order/sign incorrect");
    require(std::all_of(pcm.samples.begin()+28,pcm.samples.end(),[](auto x){return x==0;}),"zero residual block must decode zero");
    fixture[48]=0x50;
    bool rejected=false;
    try { decode_vag(fixture); } catch (const std::runtime_error&) { rejected=true; }
    require(rejected,"invalid predictor accepted");
    fixture[48]=12;fixture[65]=0;fixture[66]=0x12;
    big(fixture,36,0x56414778);big(fixture,44,2);
    pcm=decode_vag(fixture);
    require(pcm.channels==2 && pcm.samples.size()==56,"stereo frame/channel count wrong");
    require(pcm.samples[0]==7 && pcm.samples[1]==2 && pcm.samples[2]==-8 && pcm.samples[3]==1,
            "stereo blocks not interleaved correctly");
    fixture.assign(64,0);big(fixture,0,0x56414770);big(fixture,12,16);big(fixture,16,22050);
    fixture[48]=0x1c;fixture[50]=0x08;
    pcm=decode_vag(fixture);
    require(pcm.samples[0]==-8 && pcm.samples[1]==-7 && pcm.samples[2]==-7,
            "predictor history rounding differs");
    fixture.pop_back();rejected=false;
    try {decode_vag(fixture);}catch(const std::runtime_error&){rejected=true;}
    require(rejected,"truncated VAG accepted");
    fixture.assign(48+160,0);big(fixture,0,0x56414770);big(fixture,12,160);big(fixture,16,32000);
    big(fixture,36,0x56414778);big(fixture,44,2);
    for(unsigned block=0;block<10;++block) {
        const auto o=48+block*16;fixture[o]=12;
        const auto nibble=block<4 ? 1 : block<8 ? 2 : block==8 ? 3 : 4;
        std::fill(fixture.begin()+o+2,fixture.begin()+o+16,static_cast<std::uint8_t>(nibble*17));
    }
    pcm=decode_vag(fixture,64);
    require(pcm.samples.size()==280 && pcm.samples[0]==1 && pcm.samples[1]==2 &&
            pcm.samples[224]==3 && pcm.samples[225]==4,"channel interleave or short tail differs");
    fixture.assign(80,0);big(fixture,0,0x56414770);big(fixture,12,32);big(fixture,16,22050);
    std::fill(fixture.begin()+50,fixture.begin()+64,0x77);
    fixture[64]=0x10;std::fill(fixture.begin()+66,fixture.end(),0x77);
    pcm=decode_vag(fixture);
    require(pcm.samples[28]==32767 && pcm.samples[55]==32767,"decoder must clip history and PCM consistently");
    const auto root=hp2::platform::resolve_zzdata_root(".");
    if (!root) { std::puts("PASS synthetic codec; SKIPPED original asset checks (root not configured)");return 0; }
    const auto bank=GuestSampleBank::load_from_zzdata(*root);
    require(bank.names().size()==389,"expected 382 Guest plus 7 original common stereo entries");
    for (const char* name:{"MenuStart","MenuLeftRight","MenuUpDown","MenuSelect","MenuCancel","MenuWrong","MenuAdjustVolume"}) {
        const auto sound=bank.decode(name);
        require(sound.sample_rate==22050,"observed common menu sound rate changed");
        require(sound.channels==2,"common menu sound must decode stereo");
        require(sound.nominal_gain>0.0f && sound.nominal_gain<1.0f,"original descriptor gain missing");
        require(!sound.samples.empty(),"empty menu sound");
        require(std::any_of(sound.samples.begin(),sound.samples.end(),[](auto x){return x!=0;}),"menu sound decode all zero");
    }
    require(bank.decode("MenuUpDown").samples.size()==11704,"MenuUpDown block count differs");
    std::puts("PASS synthetic codec and all 7 original common stereo menu samples; 389 bank entries");
}
