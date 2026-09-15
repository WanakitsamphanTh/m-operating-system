#pragma once
#include <cstdint>
#include "kernel/mem/mem.hpp"

namespace MK {
    struct FreeRange {
        uintptr_t begin;
        uintptr_t end;
        FreeRange* next;
    };

    inline FreeRange _static_kernel_free_range {
      .begin = kernel_vaddr,
      .end = kernel_vaddr | vaddr_max,
      .next = nullptr
    };

    inline FreeRange _static_userspace_free_range {
      .begin = userspace_vaddr,
      .end = userspace_vaddr | vaddr_max,
      .next = nullptr
    };

    class VirtualAllocator {
        FreeRange range;
    public:
        VirtualAllocator(FreeRange& range);
        mstd::maybe<FreeRange> alloc(uintptr_t hint, size_t size);
    };
}