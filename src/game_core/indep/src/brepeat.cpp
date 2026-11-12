// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/src/brepeat.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001EB050; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001eb050.md.
void bRepeat::WriteToLocation() {
    hp2::not_implemented("0x001EB050", "bRepeat::WriteToLocation (signature and full body unresolved)");
}

// ELF 0x001EB0C0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001eb0c0.md.
void bRepeat::ReadFromLocation() {
    hp2::not_implemented("0x001EB0C0", "bRepeat::ReadFromLocation (signature and full body unresolved)");
}

// ELF 0x001EB498; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001eb498.md.
void bRepeat::PrepareToRemoveData() {
    hp2::not_implemented("0x001EB498", "bRepeat::PrepareToRemoveData (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include <stdexcept>
namespace hp2::debug {
// Debug001E7480/001E74F0, E-0099-W41. Original LSB-first bits; MIPS variable
// shifts mask to31, even for sizes above32. Division truncates signed location.
void bRepeat::WriteToLocation(std::int32_t location,std::int32_t data,int bits){
 auto pointer=address_+static_cast<std::uint32_t>(location/8);unsigned bit=static_cast<unsigned>(location)&7;
 for(int i=0;i<bits;++i){const auto mask=1u<<bit,value=((static_cast<std::uint32_t>(data)>>(static_cast<unsigned>(i)&31))&1u)<<bit;m_.byte(pointer,static_cast<std::uint8_t>((m_.byte(pointer)&~mask)|value));bit=(bit+1)&7;if(!bit)++pointer;}
}
std::int32_t bRepeat::ReadFromLocation(std::int32_t location,int bits)const{
 auto pointer=address_+static_cast<std::uint32_t>(location/8);unsigned bit=static_cast<unsigned>(location)&7;std::uint32_t result=0;
 for(int i=0;i<bits;++i){result|=((m_.byte(pointer)>>bit)&1u)<<(static_cast<unsigned>(i)&31);bit=(bit+1)&7;if(!bit)++pointer;}return static_cast<std::int32_t>(result);
}
// Debug001E7550. Only Channel..end is cleared; actual header scalar stores
// preserve source order and no host structure layout is copied.
void bRepeat::PrepareToAddData(int bytes,int channels,int codeine){
 const auto header=static_cast<std::int64_t>(channels)*0x24+0x30;
 if(bytes<0x30||channels<0||header>bytes)throw std::out_of_range("bRepeat source header does not fit explicit owned buffer");
 auto& m=m_;const auto a=address_;m.word(a+4,0);m.word(a+0x10,0);m.word(a+0x20,0);m.word(a+0x24,0);m.word(a+0x28,0);m.word(a,0x12345678);
 m.word(a+8,static_cast<std::uint32_t>(header)*8);m.word(a+0x18,channels);m.word(a+0x1c,codeine);m.word(a+0xc,static_cast<std::uint32_t>(header));m.word(a+0x14,bytes);
 for(int i=0x30;i<bytes;++i)m.byte(a+static_cast<unsigned>(i),0);m.word(a+0x2c,0);
}
void bRepeat::PrepareToRemoveData(){
 auto& m=m_;const auto a=address_;m.word(a+0x24,0);m.word(a+0x10,m.word(a+0xc)*8);m.word(a+0x28,0);
 for(int i=0;i<m.signed_word(a+0x18);++i){const auto c=a+0x30+static_cast<unsigned>(i)*0x24;m.word(c+0x18,0);m.word(c+0x1c,0);}
}
// Debug001E7708. Real packed stream and repeat compression. Optional development
// monitoring uses W30's actual packet builder; a full Codeine shovel is explicit.
int bRepeat::AddData(std::int32_t data,int bits,int channel,const ::port::race::MonitorSyscall& monitor){
 if(!address_)return 0;auto& m=m_;const auto a=address_;
 const auto full=[&]{return static_cast<std::int32_t>(m.word(a+0x14)*8-0x44)<=m.signed_word(a+8);};if(full())return 0;
 if(channel<0||channel>=m.signed_word(a+0x18))throw std::out_of_range("bRepeat channel outside explicit header");
 m.word(a+0x20,m.word(a+0x20)+1);const auto c=a+0x30+static_cast<unsigned>(channel)*0x24;
 const auto repeat_bits=m.word(c),maximum=(1u<<(repeat_bits&31))-1;
 const bool repeat=m.word(c+0x10)&&m.signed_word(c+8)==bits&&m.word(c+0xc)!=maximum&&m.signed_word(c+0x14)==data;
 if(repeat){m.word(c+0xc,m.word(c+0xc)+1);WriteToLocation(m.signed_word(c+0x10),m.signed_word(c+0xc),static_cast<int>(repeat_bits));}
 else {WriteToLocation(m.signed_word(a+8),channel,4);m.word(a+8,m.word(a+8)+4);m.word(c+0x10,m.word(a+8));WriteToLocation(m.signed_word(a+8),0,static_cast<int>(repeat_bits));m.word(c+0xc,0);m.word(a+8,m.word(a+8)+repeat_bits);m.word(c+0x14,static_cast<std::uint32_t>(data));WriteToLocation(m.signed_word(a+8),data,bits);m.word(a+8,m.word(a+8)+static_cast<std::uint32_t>(bits));m.word(c+8,static_cast<std::uint32_t>(bits));}
 if(m.word(a+0x1c)){m.word(a+4,m.word(a+4)+1);if(m.word(a+4)==1){::port::driving::Memory::Temporary argument(m,4);m.word(argument.address(),a);bFunkCallASync(m,0x328658,'?',argument.address(),4,monitor);}}
 if(full()&&m.word(a+0x1c))hp2::not_implemented("0x001E75D8","actual full capture Codeine buffer shovel is not reconstructed");return 1;
}
// Debug001E7918. A mismatched channel consumes only its tag, sets error and
// returns the caller-configured default; item count still increments.
std::int32_t bRepeat::RemoveData(int bits,int channel){
 auto& m=m_;const auto a=address_;if(channel<0||channel>=m.signed_word(a+0x18))throw std::out_of_range("bRepeat removal channel outside explicit header");const auto c=a+0x30+static_cast<unsigned>(channel)*0x24;
 if(m.word(c+0x1c)&&m.signed_word(c+0x18)<ReadFromLocation(m.signed_word(c+0x1c),m.signed_word(c))){m.word(c+0x18,m.word(c+0x18)+1);m.word(a+0x24,m.word(a+0x24)+1);return m.signed_word(c+0x20);}
 std::int32_t result;
 if(m.signed_word(a+0x24)<m.signed_word(a+0x20)){const auto tag=ReadFromLocation(m.signed_word(a+0x10),4);m.word(a+0x10,m.word(a+0x10)+4);if(tag==channel){m.word(c+0x1c,m.word(a+0x10));m.word(c+0x18,0);m.word(a+0x10,m.word(a+0x10)+m.word(c));result=ReadFromLocation(m.signed_word(a+0x10),bits);m.word(c+0x20,static_cast<std::uint32_t>(result));m.word(a+0x10,m.word(a+0x10)+static_cast<std::uint32_t>(bits));}else {result=m.signed_word(c+4);m.word(a+0x28,1);}}
 else {result=m.signed_word(c+4);m.word(a+0x28,1);}m.word(a+0x24,m.word(a+0x24)+1);return result;
}
}

namespace hp2::debug {
// Debug001E7AC0, E-0099-W49. Exact asynchronous completion, original server string.
void bRepeat::TellCodeineWeAreFinishedRemovingData(const ::port::race::MonitorSyscall& monitor){bFunkCallASync(m_,0x328660,'@',0,0,monitor);}
}
