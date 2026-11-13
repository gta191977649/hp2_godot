#ifndef NFSHP2_BWARE_HPP
#define NFSHP2_BWARE_HPP
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Original bGetComputerName dependency, individually matched at250970.
 * This native declaration belongs to the observed bWare.hpp interface;
 * no beta allocator/layout definitions are invented here. */
int32_t bGetComputerName(char* output,int32_t capacity);

/* Native monotonic source for the original 32-bit ticker domain. */
uint32_t hp2NativeTickerSource(void);

#ifdef __cplusplus
}

#include <cstddef>
#include <cstdarg>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

// The native time transport and its original bDebug difference operations
// share this existing bWare interface; no additional clock state is kept.
namespace hp2 {
constexpr std::uint64_t kTickerFrequency = 294912000ull;
constexpr float kTimerConversionFrequency = 300000000.0f;
std::uint32_t TicksFromNanoseconds(std::uint64_t elapsedNs) noexcept;
std::uint32_t TicksNow() noexcept;
float tickerMilliseconds(std::uint32_t from, std::uint32_t to) noexcept;
std::uint32_t fixedTickerDifference(std::uint32_t from, std::uint32_t to) noexcept;
// Optional native process journal, independent of game/replay ownership.
using NativeSessionSink=void(*)(const char* event,const char* detail) noexcept;
void BindSessionSink(NativeSessionSink sink) noexcept;
void RecordSessionEvent(const char* event,const char* detail=nullptr) noexcept;
}

// Native checked-boundary and little-endian transport helpers.
// Shared Windows helpers have a single owner in this interface.
namespace hp2 {
// PC-owned formatted text. Source stack scratch sizes are not limits on
// localized messages; this preserves all bytes and the printf arguments.
// These native helpers are not claimed as newly recovered original bodies.
std::string FormatTextV(const char* format,va_list arguments);
std::string FormatText(const char* format,...);
inline std::uint16_t ReadU16(const std::uint8_t* data) noexcept
{return static_cast<std::uint16_t>(data[0]|(static_cast<std::uint16_t>(data[1])<<8));}
inline std::uint32_t ReadU32(const std::uint8_t* data) noexcept
{
    return static_cast<std::uint32_t>(data[0])|(static_cast<std::uint32_t>(data[1])<<8)|
        (static_cast<std::uint32_t>(data[2])<<16)|(static_cast<std::uint32_t>(data[3])<<24);
}
inline void PutU32(std::uint8_t* data,std::uint32_t value) noexcept
{for(unsigned i=0;i!=4;++i)data[i]=static_cast<std::uint8_t>(value>>(8u*i));}
inline bool Inside(std::uint64_t begin,std::uint64_t length,std::uint64_t capacity) noexcept
{return begin<=capacity && length<=capacity-begin;}
inline void Require(bool condition,const char* description)
{
    // A successful literal check does not construct a temporary std::string.
    // Failed checks retain exactly the same runtime_error and diagnostic.
    if(!condition)throw std::runtime_error(description);
}
inline void Require(bool condition,const std::string& description)
{if(!condition)throw std::runtime_error(description);}
inline std::uint64_t HashBytes(const std::vector<std::uint8_t>& data) noexcept
{
    std::uint64_t hash=14695981039346656037ull;
    for(const auto byte:data){hash^=byte;hash*=1099511628211ull;}
    return hash;
}
}
#endif

// Native C declarations for the existing original Strings.cpp owner.
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t hashBytes(const void* bytes, size_t length);
uint32_t hashString(const char* text);
uint32_t hashStringContinue(const char* text, uint32_t seed);
size_t stringLength(const char* text);
char* stringCopy(char* destination, const char* source);
int32_t stringUpperByte(int8_t value);
int32_t stringCompareInsensitive(const char* first, const char* second);
int32_t stringToLong(const char* text);

#ifdef __cplusplus
}
#endif

#endif
