// Native allocation boundary.
// Console bank headers are replaced by the host allocator. Requested 32/64/
// 128-byte alignment is preserved, and native C++ default alignment is used
// for the ordinary pool. The original MemoryPool allocator fills allocated
// blocks with 0xAA and released blocks with 0xEE before updating its free list.
// A private host header retains the actual OS extent for the same release
// poison and allocation-signature check; it is never serialized game data.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

#if defined(_WIN32)
#include <malloc.h>
#endif

namespace {
struct alignas(std::max_align_t) NativeMemoryHeader {
    void* allocation;
    std::size_t allocatedBytes;
    std::uint32_t signature;
};
constexpr std::uint32_t kAllocationSignature=0x2222u;

// The source poison is a real write before release. An indirect volatile
// endpoint prevents the host optimizer from deleting a dead-store memset
// immediately followed by free(). It adds no logging or allocation.
void* (*volatile fillReleasedMemory)(void*,int,std::size_t)=std::memset;
}

extern "C" void* bMalloc(int size, const char* name, int, int pool)
{
    if (size < 0) {
        std::fprintf(stderr, "bMalloc: invalid size %d\n", size);
        std::abort();
    }
    const std::size_t alignment = (pool & 0x40) ? 128 :
        (pool & 0x20) ? 64 : (pool & 0x10) ? 32 : alignof(std::max_align_t);
    const std::size_t bytes = size ? static_cast<std::size_t>(size) : 1;
    const std::size_t prefix=(sizeof(NativeMemoryHeader)+alignment-1)&~(alignment-1);
    const std::size_t allocatedBytes=prefix+bytes;
#if defined(_WIN32)
    void* allocation = _aligned_malloc(allocatedBytes, alignment);
#else
    void* allocation = nullptr;
    if (posix_memalign(&allocation, alignment, allocatedBytes) != 0) allocation = nullptr;
#endif
    if (!allocation) {
        std::fprintf(stderr, "bMalloc: %d bytes for %s\n", size, name ? name : "(null)");
        std::abort();
    }
    std::memset(allocation,0xAA,allocatedBytes);
    auto* result=static_cast<unsigned char*>(allocation)+prefix;
    auto* header=reinterpret_cast<NativeMemoryHeader*>(result-sizeof(NativeMemoryHeader));
    new (header) NativeMemoryHeader{allocation,allocatedBytes,kAllocationSignature};
    return result;
}

extern "C" void bFree(void* pointer)
{
    if (!pointer) return;
    auto* header=reinterpret_cast<NativeMemoryHeader*>(
        static_cast<unsigned char*>(pointer)-sizeof(NativeMemoryHeader));
    if (header->signature!=kAllocationSignature) {
        std::fprintf(stderr,"bFree: invalid allocation signature at %p\n",pointer);
        std::abort();
    }
    void* allocation=header->allocation;
    const auto allocatedBytes=header->allocatedBytes;
    header->signature=0;
    fillReleasedMemory(allocation,0xEE,allocatedBytes);
#if defined(_WIN32)
    _aligned_free(allocation);
#else
    std::free(allocation);
#endif
}
