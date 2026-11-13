#include "RingBuffer.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace hp2 {

// PAL 232148..232160.
CyclicBuffer::CyclicBuffer() noexcept
    : buffer_(), capacity_(0), write_(0), read_(0) {}

// PAL 232160..2321B8. Native lifetime replaces the deleting-destructor flags.
CyclicBuffer::~CyclicBuffer() = default;

// PAL 2321B8..232220. The caller supplies a positive byte capacity. Allocation
// is native owning storage; there is no second file/audio buffer behind it.
void CyclicBuffer::Init(std::int32_t capacity) {
    if (capacity <= 0)
        throw std::invalid_argument("CyclicBuffer requires positive capacity");
    buffer_.reset();
    capacity_ = capacity;
    buffer_.reset(new std::uint8_t[static_cast<std::size_t>(capacity)]);
    write_ = 0;
    read_ = 0;
}

// PAL 232220..232264.
void CyclicBuffer::Close() noexcept {
    buffer_.reset();
    write_ = 0;
    capacity_ = 0;
    read_ = 0;
}

// PAL 232268..232290.
std::int32_t CyclicBuffer::DataAvail() const noexcept {
    if (write_ >= read_)
        return write_ - read_;
    return write_ - read_ + capacity_;
}

// PAL 232290..2322C0. A closed buffer deliberately reports -1 free bytes.
std::int32_t CyclicBuffer::SpaceAvail() const noexcept {
    return capacity_ - DataAvail() - 1;
}

// PAL 2322C0..23237C. Source/destination must not alias this private storage.
std::int32_t CyclicBuffer::AddData(const void* source, std::int32_t count) {
    if (count < 0 || (count != 0 && source == nullptr))
        throw std::invalid_argument("CyclicBuffer invalid input span");
    if (SpaceAvail() < count)
        return 0;

    const auto* input = static_cast<const std::uint8_t*>(source);
    const std::int32_t first = std::min(count, capacity_ - write_);
    if (first != 0)
        std::memcpy(buffer_.get() + write_, input,
                    static_cast<std::size_t>(first));
    write_ += first;
    if (write_ == capacity_)
        write_ = 0;

    const std::int32_t remaining = count - first;
    if (remaining != 0) {
        std::memcpy(buffer_.get(), input + first,
                    static_cast<std::size_t>(remaining));
        write_ += remaining;
    }
    return 1;
}

// PAL 232380..232488. The read cursor advances only for consume != 0, and
// insufficient input leaves both the output bytes and the cursor untouched.
std::int32_t CyclicBuffer::GetData(void* destination, std::int32_t count,
                                  std::int32_t consume) {
    if (count < 0 || (count != 0 && destination == nullptr))
        throw std::invalid_argument("CyclicBuffer invalid output span");
    if (DataAvail() < count)
        return 0;

    auto* output = static_cast<std::uint8_t*>(destination);
    const std::int32_t first = std::min(count, capacity_ - read_);
    if (first != 0)
        std::memcpy(output, buffer_.get() + read_,
                    static_cast<std::size_t>(first));
    if (consume != 0) {
        read_ += first;
        if (read_ == capacity_)
            read_ = 0;
    }

    const std::int32_t remaining = count - first;
    if (remaining != 0) {
        std::memcpy(output + first, buffer_.get(),
                    static_cast<std::size_t>(remaining));
        if (consume != 0)
            read_ += remaining;
    }
    return 1;
}

} // namespace hp2
