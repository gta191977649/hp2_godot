#include "game_core/port/police/cop_control.hpp"

#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

using namespace hp2::recovered;
namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, 4);
    return result;
}
template<std::size_t N> void put(std::array<std::uint8_t,N>& data, unsigned at, std::uint32_t value) {
    for (unsigned i=0;i<4;++i) data.at(at+i)=std::uint8_t(value>>(8*i));
}
template<std::size_t N> std::uint32_t get(const std::array<std::uint8_t,N>& data, unsigned at) {
    std::uint32_t result=0;
    for (unsigned i=0;i<4;++i) result |= std::uint32_t(data.at(at+i))<<(8*i);
    return result;
}
class Memory : public PoliceMemory {
public:
    std::map<std::uint32_t,std::uint32_t> words;
    mutable std::vector<std::uint32_t> reads;
    unsigned calls=0;
    bool eligible=true;
    std::uint32_t read_u32(std::uint32_t address) const override {
        reads.push_back(address);
        return words.at(address);
    }
    bool query_00231258(std::uint32_t address) override {
        require(address==0x1000,"wrong query receiver"); ++calls; return eligible;
    }
};
void mover_tests() {
    const std::map<std::uint32_t,std::uint32_t> expected = {
        {4,0x41340000},{31,0x414c0000},{32,0x41700000},{33,0x41700000},{34,0x41640000}};
    std::vector<std::uint32_t> types={0xffffffff,0xfffffffe,0x80000000,65536};
    for (unsigned type=0;type<=256;++type) types.push_back(type);
    for (auto type:types) {
        CopMoverStorage storage; storage.fill(0xa5); put(storage,8,0x1000);
        Memory memory;memory.words={{0x100c,0x2000},{0x2534,type}};
        auto want=storage;
        const auto found=expected.find(type);
        if (found!=expected.end()) put(want,0x3c,found->second);
        CopMover(storage).select_type_coefficient(memory);
        require(storage==want,"coefficient writes/unknown preservation differ");
        require(memory.reads==std::vector<std::uint32_t>{0x100c,0x2534},"coefficient read order differs");
    }
    // PS2 address arithmetic is 32-bit, including wraparound.
    CopMoverStorage storage{};put(storage,8,0xfffffffc);
    Memory memory;memory.words={{8,0xfffffff0},{0x524,4}};
    CopMover(storage).select_type_coefficient(memory);
    require(get(storage,0x3c)==0x41340000,"address wrap differs");
}
void driver_tests() {
    const std::vector<std::uint32_t> states={0,1,2,3,4,5,6,0xffffffff};
    for (auto state:states) for (bool eligible:{false,true}) for (float speed:{0.0f,1.999f,2.0f,3.0f}) {
        CopDriverStorage storage;storage.fill(0xa5);put(storage,0,0x1000);
        put(storage,0x24,state);put(storage,0x30,bits(0.5f));
        auto want=storage;
        Memory memory;memory.eligible=eligible;memory.words={{0x13c4,bits(speed)}};
        bool want_result=false;
        const bool timing_state=state==2||state==3;
        if (eligible && state<5) {
            if (timing_state && speed<2.0f) {put(want,0x30,bits(1.25f));want_result=true;}
            else put(want,0x30,0);
        }
        const bool result=CopDriver(storage).check_low_speed_timeout(0.75f,memory);
        require(result==want_result && storage==want,"timeout state/output writes differ");
        require(memory.calls==1,"external predicate call count differs");
        require(memory.reads.size()==unsigned(eligible&&timing_state),"unexpected speed read");
    }
    CopDriverStorage storage{};put(storage,0,0x1000);put(storage,0x24,2);put(storage,0x30,bits(0.5f));
    Memory memory;memory.words={{0x13c4,bits(1.0f)}};
    CopDriver driver(storage);
    require(!driver.check_low_speed_timeout(0.5f,memory),"threshold must be strictly greater than one");
    require(get(storage,0x30)==bits(1.0f),"timer bits at threshold differ");
    require(driver.check_low_speed_timeout(0.25f,memory),"increment across threshold differs");
    require(get(storage,0x30)==bits(1.25f),"timer stored as integer rather than float");
}
} // namespace
int main() {
    try {mover_tests();driver_tests();}
    catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
    std::cout<<"PASS police helpers: 262 type/address cases; 64 state/query/speed cases; threshold sequence\n";
}
