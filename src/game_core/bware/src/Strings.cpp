#include "bware/inc/bWare.hpp"
#include <string.h>
#include <cstdio>

namespace hp2 {
std::string FormatTextV(const char* format,va_list arguments)
{
    Require(format,"Formatted text has no format string");
    // Keep short messages on the stack; only long localized output needs
    // a second formatting pass and storage sized to the real result.
    char scratch[256];
    va_list copied;
    va_copy(copied,arguments);
    const auto length=std::vsnprintf(scratch,sizeof(scratch),format,copied);
    va_end(copied);
    Require(length>=0,"Native text formatting failed");
    const auto size=static_cast<std::size_t>(length);
    if(size<sizeof(scratch))return std::string(scratch,size);
    std::string result(size,'\0');
    va_copy(copied,arguments);
    const auto written=std::vsnprintf(result.data(),result.size()+1,format,copied);
    va_end(copied);
    Require(written==length,"Native text formatting changed between passes");
    return result;
}

std::string FormatText(const char* format,...)
{
    va_list arguments;
    va_start(arguments,format);
    try {
        auto result=FormatTextV(format,arguments);
        va_end(arguments);
        return result;
    } catch(...) {
        va_end(arguments);
        throw;
    }
}
}

uint32_t hashString(const char* text)
{
    const uint8_t* source = (const uint8_t*)text;
    uint32_t value = UINT32_MAX;
    while (*source != 0)
        value = value * 33u + *source++;
    return value;
}

uint32_t hashStringContinue(const char* text, uint32_t seed)
{
    const uint8_t* source=(const uint8_t*)text;
    uint32_t value=seed;
    while (*source!=0) value=value*33u+*source++;
    return value;
}

size_t stringLength(const char* text)
{
    size_t length = 0;
    while (text[length] != '\0') ++length;
    return length;
}

char* stringCopy(char* destination, const char* source)
{
    char* start = destination;
    do { *destination++ = *source; } while (*source++ != '\0');
    return start;
}

int32_t stringUpperByte(int8_t value)
{
    const int32_t character = value;
    if (character >= 'a' && character <= 'z')
        return character & 0x5F;
    return character;
}

int32_t stringCompareInsensitive(const char* first, const char* second)
{
    const auto* left = reinterpret_cast<const uint8_t*>(first);
    const auto* right = reinterpret_cast<const uint8_t*>(second);
    for (;;) {
        const int32_t a = *left < 128 ? *left : int32_t(*left) - 256;
        const int32_t b = *right < 128 ? *right : int32_t(*right) - 256;
        ++left;
        ++right;
        const int32_t upperA = stringUpperByte(static_cast<int8_t>(a));
        const int32_t upperB = stringUpperByte(static_cast<int8_t>(b));
        if (upperA == 0 || upperB == 0 || upperA != upperB)
            return upperA - upperB;
    }
}

int32_t stringToLong(const char* text)
{
    uint32_t value = 0;
    const auto* source = reinterpret_cast<const uint8_t*>(text);
    while (*source) {
        const uint32_t digit = static_cast<uint32_t>(*source) - '0';
        if (digit >= 10u) break;
        value = value * 10u + digit;
        ++source;
    }
    int32_t result;
    memcpy(&result, &value, sizeof(result));
    return result;
}
