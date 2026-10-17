#include "memory.hpp"
#include <cstring>
#include <limits>
#include <stdexcept>

namespace port::driving {
Memory::Scratch::~Scratch() noexcept {
    while(memory_.allocations_.size()>count_){memory_.regions_.erase(memory_.allocations_.back());memory_.allocations_.pop_back();}
    memory_.next_=next_;
}
const std::uint8_t* Memory::resolve(Address address,std::size_t size) const {
    const auto end=std::uint64_t(address)+size;
    if(end>0x100000000ull)throw std::out_of_range("Logical address range wraps");
    auto it=regions_.upper_bound(address);
    if(it==regions_.begin())throw std::out_of_range("Unmapped logical address "+std::to_string(address));
    --it;const auto offset=std::size_t(address-it->first);
    if(offset>it->second.size()||size>it->second.size()-offset)throw std::out_of_range("Truncated logical region at "+std::to_string(address)+" length "+std::to_string(size));
    return it->second.data()+offset;
}
std::uint8_t* Memory::resolve(Address address,std::size_t size){return const_cast<std::uint8_t*>(static_cast<const Memory&>(*this).resolve(address,size));}
void Memory::map(Address address,const std::vector<std::uint8_t>& bytes){
    const auto end=std::uint64_t(address)+bytes.size();
    if(!address||bytes.empty()||end>0x100000000ull)throw std::invalid_argument("Invalid logical region");
    auto next=regions_.lower_bound(address);
    if(next!=regions_.end()&&std::uint64_t(next->first)<end)throw std::invalid_argument("Overlapping logical region");
    if(next!=regions_.begin()) {auto previous=std::prev(next);if(std::uint64_t(previous->first)+previous->second.size()>address)throw std::invalid_argument("Overlapping logical region");}
    regions_.emplace(address,bytes);
}
void Memory::map(Address address,std::uint32_t size){map(address,std::vector<std::uint8_t>(size));}
Memory::Address Memory::allocate(std::uint32_t size,std::uint32_t alignment){
    if(!size||!alignment||(alignment&(alignment-1)))throw std::invalid_argument("Invalid logical allocation");
    auto candidate=(std::uint64_t(next_)+alignment-1)&~std::uint64_t(alignment-1);
    while(candidate+size+16<=0xffffffffull){
        auto it=regions_.upper_bound(static_cast<Address>(candidate));
        if(it!=regions_.begin()) {auto previous=std::prev(it);auto end=std::uint64_t(previous->first)+previous->second.size();if(end>candidate){candidate=(end+alignment-1)&~std::uint64_t(alignment-1);continue;}}
        if(it!=regions_.end()&&candidate+size>it->first){candidate=(std::uint64_t(it->first)+it->second.size()+alignment-1)&~std::uint64_t(alignment-1);continue;}
        auto result=static_cast<Address>(candidate);map(result,size);allocations_.push_back(result);
        // An unmapped port-owned guard prevents an unobserved object field from
        // silently aliasing the next component's list links.
        next_=result+size+16;return result;
    }
    throw std::overflow_error("Logical allocator exhausted");
}
init::ByteView Memory::view(Address address,std::size_t size){return {resolve(address,size),size};}
std::uint8_t Memory::byte(Address a)const{return *resolve(a,1);}
std::int8_t Memory::signed_byte(Address a)const {auto bits=byte(a);std::int8_t v;std::memcpy(&v,&bits,1);return v;}
std::uint16_t Memory::half(Address a)const{const auto p=resolve(a,2);return std::uint16_t(p[0])|(std::uint16_t(p[1])<<8);}
std::int16_t Memory::signed_half(Address a)const{auto bits=half(a);std::int16_t v;std::memcpy(&v,&bits,2);return v;}
std::uint32_t Memory::word(Address a)const{const auto p=resolve(a,4);return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);}
std::int32_t Memory::signed_word(Address a)const{auto bits=word(a);std::int32_t v;std::memcpy(&v,&bits,4);return v;}
float Memory::scalar(Address a)const{auto bits=word(a);float v;std::memcpy(&v,&bits,4);return v;}
void Memory::byte(Address a,std::uint8_t v){*resolve(a,1)=v;}
void Memory::half(Address a,std::uint16_t v){auto p=resolve(a,2);p[0]=static_cast<std::uint8_t>(v);p[1]=static_cast<std::uint8_t>(v>>8);}
void Memory::word(Address a,std::uint32_t v){auto p=resolve(a,4);for(unsigned i=0;i<4;++i)p[i]=static_cast<std::uint8_t>(v>>(8*i));}
void Memory::scalar(Address a,float v){std::uint32_t bits;std::memcpy(&bits,&v,4);word(a,bits);}
std::array<float,4> Memory::vector(Address a)const{std::array<float,4> v;for(unsigned i=0;i<4;++i)v[i]=scalar(a+4*i);return v;}
std::array<float,16> Memory::matrix(Address a)const{std::array<float,16> v;for(unsigned i=0;i<16;++i)v[i]=scalar(a+4*i);return v;}
void Memory::vector(Address a,const std::array<float,4>& v){resolve(a,16);for(unsigned i=0;i<4;++i)scalar(a+4*i,v[i]);}
void Memory::matrix(Address a,const std::array<float,16>& v){resolve(a,64);for(unsigned i=0;i<16;++i)scalar(a+4*i,v[i]);}
void Memory::copy(Address to,Address from,std::size_t count){auto source=resolve(from,count);auto target=resolve(to,count);std::memmove(target,source,count);}
}
