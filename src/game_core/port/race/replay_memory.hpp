#pragma once
// Port-only primitive binding for original bCountFreeMemory00250D58.
// The source ReplayManager makes the allocation decision; this provider must
// report the actual chosen port heap budget, never a constant success value.
#include <functional>
namespace port::race {
struct ReplayMemoryServices {std::function<int(int alignment_mask)> count_free_memory;};
}
