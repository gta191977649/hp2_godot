#include "original_image.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <regex>
#include <sstream>
#include <stdexcept>
namespace port::driving {
namespace {
std::uint32_t rotate(std::uint32_t a,unsigned n){return (a>>n)|(a<<(32-n));}
std::vector<std::uint8_t> bytes(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Original ELF is unavailable: "+p.string());return {std::istreambuf_iterator<char>(f),{}};}
}
std::string image_sha256(const std::vector<std::uint8_t>& source) {
    constexpr std::uint32_t k[64]={
      0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
      0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
      0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
      0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
      0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
      0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
      0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
      0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
    std::uint32_t h[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    auto data=source;const std::uint64_t bits=std::uint64_t(data.size())*8;data.push_back(0x80);
    while(data.size()%64!=56)data.push_back(0);
    for(int i=7;i>=0;--i)data.push_back(static_cast<std::uint8_t>(bits>>(8*i)));
    for(std::size_t base=0;base<data.size();base+=64){
      std::uint32_t w[64];for(unsigned i=0;i<16;++i)w[i]=(std::uint32_t(data[base+i*4])<<24)|(std::uint32_t(data[base+i*4+1])<<16)|(std::uint32_t(data[base+i*4+2])<<8)|data[base+i*4+3];
      for(unsigned i=16;i<64;++i){const auto s0=rotate(w[i-15],7)^rotate(w[i-15],18)^(w[i-15]>>3),s1=rotate(w[i-2],17)^rotate(w[i-2],19)^(w[i-2]>>10);w[i]=w[i-16]+s0+w[i-7]+s1;}
      auto a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],v=h[7];
      for(unsigned i=0;i<64;++i){const auto s1=rotate(e,6)^rotate(e,11)^rotate(e,25),choice=(e&f)^(~e&g),t1=v+s1+choice+k[i]+w[i];const auto s0=rotate(a,2)^rotate(a,13)^rotate(a,22),majority=(a&b)^(a&c)^(b&c),t2=s0+majority;v=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;}
      h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=v;
    }
    std::ostringstream out;out<<std::hex<<std::setfill('0');for(auto v:h)out<<std::setw(8)<<v;return out.str();
}
Memory read_original_image(const std::filesystem::path& path) {
    const auto data=bytes(path);
    if(image_sha256(data)!="7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2")throw std::runtime_error("Original ELF SHA-256 does not match SLUS_203.62 recovery evidence");
    const auto word=[&](std::size_t p){if(p>data.size()||4>data.size()-p)throw std::runtime_error("Truncated ELF field");return std::uint32_t(data[p])|(std::uint32_t(data[p+1])<<8)|(std::uint32_t(data[p+2])<<16)|(std::uint32_t(data[p+3])<<24);};
    const auto half=[&](std::size_t p){return word(p)&0xffffu;};
    if(word(0)!=0x464c457f||data[4]!=1||data[5]!=1||half(18)!=8)throw std::runtime_error("Expected original ELF32 little-endian MIPS image");
    Memory memory;const auto start=word(28),stride=half(42),count=half(44);
    if(stride<32)throw std::runtime_error("Invalid ELF program table");
    for(std::uint32_t i=0;i<count;++i){const auto p=std::size_t(start)+std::size_t(i)*stride;if(word(p)!=1)continue;
      const auto offset=word(p+4),address=word(p+8),filesize=word(p+16),size=word(p+20);
      if(filesize>size||offset>data.size()||filesize>data.size()-offset||size>0x2000000u||address<0x100000u||std::uint64_t(address)+size>0x2000000u)throw std::runtime_error("Original ELF segment is out of bounded EE memory");
      std::vector<std::uint8_t> region(size);std::copy_n(data.begin()+offset,filesize,region.begin());memory.map(address,region);
    }
    // Ghidra memory-block audit E-0038-B08 corroborates these data anchors.
    if(memory.word(0x002b5430+0x3c)!=0x0019d4d0u || memory.word(0x002aeaa8+0x1c)!=0x00138e08u)throw std::runtime_error("Original data anchors disagree with Ghidra evidence");
    return memory;
}
std::filesystem::path original_image_path(const std::filesystem::path& root) {
    if(const auto value=std::getenv("HP2_PS2_ELF"))if(*value)return value;
    auto directory=std::filesystem::absolute(root);
    for(unsigned depth=0;depth<8;++depth){
    const auto config=directory/"config/ps2_elf.local.json";std::ifstream file(config);
    const std::string text{std::istreambuf_iterator<char>(file),{}};std::smatch match;
    if(std::regex_search(text,match,std::regex("\"elf\"\\s*:\\s*\"([^\"]+)\""))) {
      auto value=match[1].str();std::string decoded;for(std::size_t i=0;i<value.size();++i){if(value[i]=='\\'&&i+1<value.size()&&(value[i+1]=='\\'||value[i+1]=='/'))++i;decoded+=value[i];}
      auto path=std::filesystem::path(decoded);return path.is_absolute()?path:directory/path;
    }
    if(directory==directory.parent_path())break;
    directory=directory.parent_path();
    }
    throw std::runtime_error("Configure HP2_PS2_ELF or config/ps2_elf.local.json for original numeric tables");
}
}
